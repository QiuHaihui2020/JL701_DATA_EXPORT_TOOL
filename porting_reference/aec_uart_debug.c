/*
 * V1 定长交织格式发送端
 *
 * 帧布局(帧长恒为 single_size * nch + 4):
 *   [ch0 single_size][ch1 single_size]...[chN-1 single_size][CRC16][填充]
 *   CRC16 覆盖前面 single_size * nch 字节，紧跟净载荷
 *   最后 2 字节填充，接收端不解析
 *
 * V1 没有自描述信息，通道归属完全由字节在交织块里的位置决定，
 * 收发两端的 nch / single_size 必须严格一致。
 */

#include "system/includes.h"
#include "circular_buf.h"
#include "crc.h"
#include "jiffies.h"
#include "aec_uart_debug.h"
#include "uartPcmSender.h"

/*重对齐缓冲大小，与原库一致*/
#define AEC_UART_ALIGN_BUF_SIZE     1024

/*
 * 结构体标签、全局变量名、成员名与顺序都取自库里的调试信息。
 *
 * 原结构体还有 sem / finish_sem 两个信号量成员，用于 write() 与发送
 * 任务之间的握手。本实现同步发送，用不到，故未保留 —— 这不影响线上
 * 字节，只影响线程模型，详见 aec_uart_send_frame() 的说明。
 */
struct aec_uart_s {
    volatile u8  busy;
    /*
     * 整帧缓冲。原库这两个成员的类型是 u16 *，帧内一切都按半字寻址:
     * 通道 ch 的数据落在 uartSendBuf[pcm_single_size * ch]，
     * CRC 存在 uartSendBuf[pcm_packet_size - 2]。
     * 因此下面存 CRC 时不做字节序转换 —— 这依赖本机小端，与原库一致。
     * 移植到大端平台要把 CRC 改成显式小端写入。
     */
    u16         *uartSendBuf;           /*pcm_packet_size 个半字*/
    u16         *uartSendBuf1;          /*原库的第二缓冲，同步发送下不需要*/
    u8           pcm_channel;           /*通道数*/
    u16          pcm_single_size;       /*单通道半字数 = single_size / 2*/
    u16          pcm_packet_size;       /*整帧半字数 = pcm_single_size * nch + 2*/
    u8           align_buf[AEC_UART_ALIGN_BUF_SIZE];
    cbuffer_t    align_cb;              /*长度不符时用于重对齐*/
};

static struct aec_uart_s *aec_uart = 0;

/*
 * 发一帧。
 *
 * 原库是 write() 投 sem、另起的 aec_dbg 任务里做 uartSendData(发完再
 * post finish_sem)，为的是不在音频回调上下文里阻塞；write() 若发现
 * busy 还没清，会先 pend finish_sem 等上一帧发完，并打印
 * "uart baudrate limit"。
 *
 * 本实现直接同步发 —— 线上字节完全一样，只是占用调用者的时间。
 * 平台不允许在音频回调里阻塞的话: 把这里改成把 (buf, len) 投队列，
 * 在自己的发送任务里调 uartSendData，并把 uartSendBuf1 用起来做双缓冲。
 */
static void aec_uart_send_frame(void *buf, u16 len)
{
    uartSendData(buf, len);
}

int aec_uart_init(void)
{
    /*原库此处为空实现，保留以维持接口一致*/
    return 0;
}

int aec_uart_open(u8 nch, u16 single_size)
{
    struct aec_uart_s *hdl;
    u32 frame_bytes;

    if (aec_uart) {
        return -1;
    }
    if ((nch == 0U) || (single_size == 0U)) {
        return -1;
    }
    /*帧内按半字排布，单通道字节数必须是偶数，否则通道边界落在半字中间*/
    if ((single_size & 1U) != 0U) {
        return -1;
    }

    printf("aec_uart_open ch:%d, single_size:%d\n", nch, single_size);

    hdl = (struct aec_uart_s *)zalloc(sizeof(*hdl));
    if (!hdl) {
        printf("[err] aec_uart zalloc err\n");
        return -1;
    }

    hdl->pcm_channel     = nch;
    hdl->pcm_single_size = (u16)(single_size >> 1);
    /*+2 个半字 = 4 字节，就是帧尾的 CRC 加填充*/
    hdl->pcm_packet_size = (u16)(hdl->pcm_single_size * nch + 2U);

    frame_bytes = (u32)hdl->pcm_packet_size * 2U;
    hdl->uartSendBuf = (u16 *)zalloc(frame_bytes);
    if (!hdl->uartSendBuf) {
        printf("[err] uartSendBuf zalloc err\n");
        free(hdl);
        return -1;
    }
    /*
     * 原库还会 zalloc 一块等长的 uartSendBuf1 作第二缓冲(失败时打印
     * "[err] uartSendBuf1 zalloc err")，并 task_create 出 aec_dbg 任务、
     * 打印 "=====AEC_UART_TASK======"。同步发送下这两样都不需要，
     * 故未保留 —— 改异步时要一并补回来。
     */
    cbuf_init(&hdl->align_cb, hdl->align_buf, AEC_UART_ALIGN_BUF_SIZE);

    aec_uart = hdl;
    printf("aec_uart_init ok\n");
    return 0;
}

int aec_uart_fill(u8 ch, void *buf, u16 size)
{
    struct aec_uart_s *hdl = aec_uart;
    u32 single_bytes;

    if (!hdl) {
        return 0;               /*原库未打开时静默返回 0*/
    }
    if ((ch >= hdl->pcm_channel) || (buf == 0)) {
        return 0;
    }

    single_bytes = (u32)hdl->pcm_single_size * 2U;

    if ((u32)(size >> 1) != (u32)hdl->pcm_single_size) {
        /*
         * 长度不符，走重对齐。
         * 注意这里会改写调用方的 buf —— 原库就是这么做的(所以 buf 参数
         * 不是 const)，调用方不能假设 buf 内容不变。
         */
        cbuf_write(&hdl->align_cb, buf, (u32)(size >> 1) * 2U);

        /*严格大于，与原库一致*/
        if (cbuf_get_data_len(&hdl->align_cb) > single_bytes) {
            cbuf_read(&hdl->align_cb, buf, single_bytes);
        } else {
            /*还不够，本轮该通道补零，保持帧长和通道对位不变*/
            printf("aec ch %d buf clear 0:%d\n", ch, size);
            memset(buf, 0, single_bytes);
        }
    }

    /*按半字寻址: 通道 ch 的槽位在 uartSendBuf[pcm_single_size * ch]*/
    memcpy(&hdl->uartSendBuf[(u32)hdl->pcm_single_size * ch], buf, single_bytes);
    return 0;
}

void aec_uart_write(void)
{
    struct aec_uart_s *hdl = aec_uart;
    u32 payload;
    u16 crc;

    if (!hdl) {
        return;
    }
    /*净载荷 = 整帧 - 4(CRC + 填充)*/
    payload = (u32)hdl->pcm_packet_size * 2U - 4U;

    crc = CRC16(hdl->uartSendBuf, payload);
    /*
     * CRC 紧跟净载荷: 半字索引 pcm_packet_size - 2 就是字节偏移 payload。
     * 最后一个半字是填充，原库不写它，发出去的是缓冲残留 —— 这里也不写，
     * 因为缓冲只 zalloc 一次、这个位置从头到尾没人碰，所以恒为 0。
     * 接收端两个版本都不解析这 2 字节。
     */
    hdl->uartSendBuf[hdl->pcm_packet_size - 2U] = crc;

    /*
     * 原库在此检查 busy: 上一帧还没发完就打印 "uart baudrate limit"
     * 并 pend finish_sem 等它发完 —— 那是波特率跟不上数据率的告警。
     * 同步发送下 busy 进来恒为 0，这条路走不到; 改异步后必须把等待补上，
     * 否则会把正在发送的缓冲改掉。
     */
    if (hdl->busy) {
        printf("uart baudrate limit\n");
    }
    hdl->busy = 1U;
    aec_uart_send_frame(hdl->uartSendBuf, (u16)(payload + 4U));
    hdl->busy = 0U;
}

int aec_uart_close(void)
{
    struct aec_uart_s *hdl = aec_uart;

    if (!hdl) {
        return -1;
    }
    printf("aec_uart_close\n");
    aec_uart = 0;
    if (hdl->uartSendBuf) {
        free(hdl->uartSendBuf);
    }
    free(hdl);
    return 0;
}
