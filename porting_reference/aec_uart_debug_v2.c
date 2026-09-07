/*
 * V2 包流格式发送端
 *
 * 分两层，容易混淆:
 *
 * 内层(自描述包): 每次 fill 生成一个包，长度可变
 *   [包头 20 字节][data len 字节]
 *   包头逐字节布局(全部小端):
 *     偏移  0  u16  magic      固定 0x2B5A
 *     偏移  2  u16  ch         通道号
 *     偏移  4  u16  seqn       该通道包序号，每包自增
 *     偏移  6  u16  crc        CRC16(data, len)，只覆盖数据，不含包头
 *     偏移  8  u16  len        本包 data 字节数
 *     偏移 10  u16  (填充)     对齐空洞，不承载信息
 *     偏移 12  u32  timestamp  发送时刻 tick
 *     偏移 16  u32  total_len  该通道累计字节数，含本包
 *
 *   偏移 10 那两字节必须占位。库的调试信息里 data_export_header 的成员
 *   是 magic@0 / ch@2 / seqn@4 / crc@6 / len@8 / timestamp@12 / total_len@16
 *   —— 字节 10 处没有任何具名成员，那是编译器为 4 字节对齐的 timestamp
 *   留的空洞，但它确实被整块写进了字节流。少了它 timestamp / total_len
 *   会整体前移 2 字节。
 *
 * 外层(传输帧): 把包流当成无边界字节流，攒够 512 就发一帧
 *   [包流 512 字节][CRC16][填充]      帧长恒为 516
 *   不足 512 就攒着不发，所以包会跨帧、一帧里也可能有多个包的碎片。
 *   接收端按字节流找同步头，不要求包与帧对齐。
 */

#include "system/includes.h"
#include "circular_buf.h"
#include "crc.h"
#include "jiffies.h"
#include "aec_uart_debug.h"
#include "uartPcmSender.h"

#define V2_MAGIC            0x2B5AU
#define V2_HDR_LEN          20U
#define V2_FRAME_PAYLOAD    512U
#define V2_FRAME_LEN        516U     /*512 + CRC 2 + 填充 2*/

/*
 * 结构体标签、全局变量名、成员名与顺序都取自库里的调试信息。
 * 注意 V2 和 V1 各有一份同名的 aec_uart_s，成员不同 —— 它们在原库里
 * 也是两个独立的 .c，各自的 static 全局，互不可见。
 *
 * 原结构体还有 sem / finish_sem 两个信号量成员，用于 write_v2() 与
 * 发送任务之间的握手。本实现同步发送，用不到，故未保留 —— 这不影响
 * 线上字节，只影响线程模型，详见 aec_uart_send_frame() 的说明。
 */
struct aec_uart_s {
    volatile u8  busy;
    volatile u8  state;                 /*fill / write 的总开关，open 置 1*/
    u16         *uartSendBuf;           /*原库有此成员，V2 路径上未使用*/
    /*
     * 516 字节的外层帧组包缓冲。原库这个成员的类型是 u16 *，
     * CRC 直接存在 uartSendBuf1[256](= 字节偏移 512)，不做字节序转换。
     */
    u16         *uartSendBuf1;
    u8           pcm_channel;           /*通道数*/
    u16          pcm_single_size;
    u16          pcm_packet_size;       /*open_v2 传进来的尺寸依据*/
    cbuffer_t    align_cb;              /*包流缓冲*/
    spinlock_t   s_lock;                /*fill 与发送侧互斥*/
    u16         *seqn;                  /*每通道包序号*/
    u32         *total_len;             /*每通道累计字节数*/
};

static struct aec_uart_s *aec_uart = 0;

/*
 * 包头。成员名与顺序取自库的调试信息，只有这 7 个 —— 没有占位成员。
 *
 * 注意这里刻意不加 packed: 原库就是让编译器按自然对齐排布，
 * 5 个 u16 占到偏移 10 之后，为了 4 字节对齐的 timestamp 自动插入
 * 2 字节空洞，总长恰好 20。那个空洞不是协议字段，内容是未初始化的
 * 栈内容，但它会被整块 cbuf_write 进字节流 —— 所以接收端解析时必须
 * 显式占位，否则 timestamp / total_len 会整体前移 2 字节。
 *
 * 按成员赋值同样不做字节序转换，依赖本机小端，与原库一致。
 * 移植到大端平台要改成显式小端序列化。
 */
struct data_export_header {
    u16 magic;
    u16 ch;
    u16 seqn;
    u16 crc;
    u16 len;
    u32 timestamp;
    u32 total_len;
};

_Static_assert(sizeof(struct data_export_header) == V2_HDR_LEN,
               "data_export_header must be 20 bytes");

/*
 * 发一帧。
 *
 * 原库是 write_v2() 投 sem、aec_dbg 任务里循环取 512 组帧发送; write_v2()
 * 若发现 busy 还没清，只打印一个 'b' 就返回，不等 —— 那是"喂数据比发得快"
 * 的提示，不是错误(V1 那边则会 pend finish_sem 等上一帧发完)。
 *
 * 本实现直接同步发完 —— 线上字节完全一样，只是占用调用者的时间。
 * 要改异步就把这里换成投队列，注意 uartSendBuf1 只有一份，需要拷贝或
 * 双缓冲，并且要确认 spin_lock 在你的平台上是真实现。
 */
static void aec_uart_send_frame(void *buf, u16 len)
{
    uartSendData(buf, len);
}

int aec_uart_init_v2(void)
{
    /*原库此处为空实现，保留以维持接口一致*/
    return 0;
}

int aec_uart_open_v2(u8 nch, u16 single_size)
{
    struct aec_uart_s *hdl;
    u8 *align_buf;
    u32 size;

    if (aec_uart) {
        return -1;
    }
    if ((nch == 0U)) {
        return -1;
    }

    printf("aec_uart_open_v2 ch:%d, pcm_packet_size:%d\n", nch, single_size);

    hdl = (struct aec_uart_s *)zalloc(sizeof(*hdl));
    if (!hdl) {
        printf("[err] aec_uart zalloc err\n");
        return -1;
    }
    hdl->pcm_channel     = nch;
    hdl->pcm_packet_size = single_size;

    /*
     * 缓冲尺寸沿用原库算法: (nch * 包头长 + single_size) * 2。
     * single_size 由调用方传各通道数据量之和(data_export_node 传的是
     * 各节点 data_len + 2048 的累加)。
     */
    size = ((u32)nch * V2_HDR_LEN + (u32)single_size) * 2U;
    if (size < V2_FRAME_PAYLOAD) {
        size = V2_FRAME_PAYLOAD;
    }

    hdl->seqn         = (u16 *)zalloc((u32)nch * sizeof(u16));
    hdl->total_len    = (u32 *)zalloc((u32)nch * sizeof(u32));
    hdl->uartSendBuf1 = (u16 *)zalloc(V2_FRAME_LEN);
    align_buf         = (u8  *)zalloc(size);
    if (!hdl->seqn || !hdl->total_len) {
        printf("[err] seqn || total_len zalloc err !!!\n");
    }
    if (!hdl->uartSendBuf1) {
        printf("[err] uartSendBuf1 zalloc err\n");
    }
    if (!hdl->seqn || !hdl->total_len || !hdl->uartSendBuf1 || !align_buf) {
        if (hdl->seqn)         free(hdl->seqn);
        if (hdl->total_len)    free(hdl->total_len);
        if (hdl->uartSendBuf1) free(hdl->uartSendBuf1);
        if (align_buf)         free(align_buf);
        free(hdl);
        return -1;
    }
    /*包流缓冲的指针由 align_cb 持有，close 时从 align_cb.begin 取回来释放*/
    cbuf_init(&hdl->align_cb, align_buf, size);

    hdl->state = 1U;
    aec_uart   = hdl;
    /*
     * 原库这里还会 task_create 出 aec_dbg 任务、打印
     * "=====AEC_UART_TASK======"。同步发送下不需要任务。
     */
    printf("aec_uart_init ok\n");
    return 0;
}

int aec_uart_fill_v2(u8 ch, void *buf, u16 size)
{
    struct aec_uart_s *hdl = aec_uart;
    struct data_export_header header;

    /*
     * 返回值语义: 成功返回 size，失败返回 0。cbuf 的写入是"全有或全无"
     * (装不下直接返回 0，不做部分写)，所以只有这两种取值。
     *
     * 注意原库编译出来的行为与此不同: 它只有一个出口、无条件
     * return size，句柄为空或容量不足被丢弃时也返回 size，调用方
     * 拿返回值判断不了成败。这里按设计意图实现。
     */
    if (!hdl || !hdl->state) {
        return 0;
    }
    if ((ch >= hdl->pcm_channel) || (buf == 0) || (size == 0U)) {
        return 0;
    }

    /*
     * 原库在这里上锁，一直持到函数唯一的出口才解锁 —— 序号自增、
     * 累计长度累加、两次 cbuf_write 必须是一个原子整体，否则并发下
     * 会出现包头与数据被别的通道插进来劈开。
     */
    spin_lock(&hdl->s_lock);

    /*
     * 累计长度和包序号都在容量检查之前就前进，与原库一致。这不是随手
     * 写的顺序: 包因缓冲满被丢弃时，seqn 跳一格、total_len 跳一段，
     * 接收端凭这两个信号就能算出发送端自己丢了多少字节。
     * 若挪到检查之后，两个计数器只统计真正入队的数据，发送端的丢包
     * 对接收端就完全隐形了，只剩链路丢包可查。
     */
    hdl->total_len[ch] += size;

    header.magic     = V2_MAGIC;
    header.ch        = ch;
    header.seqn      = hdl->seqn[ch]++;             /*填自增前的值*/
    header.len       = size;
    header.timestamp = jiffies;
    header.crc       = CRC16(buf, size);   /*只校验数据，不含包头*/
    header.total_len = hdl->total_len[ch];
    /*偏移 10 的对齐空洞不赋值，与原库一致(内容无意义，接收端不解析)*/

    /*
     * 整包装不下就丢弃，绝不能只写一半 —— 半个包会让接收端一直失步，
     * 要重新找同步头才能恢复。
     */
    if (cbuf_is_write_able(&hdl->align_cb, (u32)size + V2_HDR_LEN) == 0) {
        /*丢包这条路径不能静默，否则现场只能看到接收端有洞、查不到源头*/
        printf("aec uart ch %d fill err %d  %d --\n",
                     ch, (int)size + (int)V2_HDR_LEN,
                     (int)cbuf_get_data_len(&hdl->align_cb));
        spin_unlock(&hdl->s_lock);
        return 0;
    }

    /*
     * 两次写入都查返回值。cbuf 是"全有或全无"，加上前面已经问过
     * is_write_able，正常不会走到告警; 真打出来说明并发保护失效了
     * (比如把发送改成异步却没把 spin_lock 换成真实现)。
     */
    if (cbuf_write(&hdl->align_cb, &header, sizeof(header)) != sizeof(header)) {
        printf("aec uart cbuf write err1");
    }
    if (cbuf_write(&hdl->align_cb, buf, size) != size) {
        printf("aec uart cbuf write err2");
    }

    spin_unlock(&hdl->s_lock);
    return (int)size;
}

void aec_uart_write_v2(void)
{
    struct aec_uart_s *hdl = aec_uart;
    u32 data_len;
    u16 crc;

    if (!hdl || !hdl->state) {
        return;
    }

    /*
     * 原库在此检查 busy: 上一轮还没发完就打印一个 'b' 直接返回，不等 ——
     * 这是 V2 与 V1 的有意差异(V1 会 pend finish_sem 等)。
     * 同步发送下 busy 进来恒为 0，走不到。
     */
    if (hdl->busy) {
        printf("b");
    }
    hdl->busy = 1U;
    /*攒够 512 才发，不足的留在 cbuf 里等下一次*/
    data_len = cbuf_get_data_len(&hdl->align_cb);
    while (data_len > (V2_FRAME_PAYLOAD - 1U)) {
        /*
         * 原库只在这一下持锁: 取走 512 字节要和 fill 的写入互斥，
         * 但后面的 CRC 计算和串口发送不碰 cbuf，不必占着锁。
         */
        spin_lock(&hdl->s_lock);
        cbuf_read(&hdl->align_cb, hdl->uartSendBuf1, V2_FRAME_PAYLOAD);
        spin_unlock(&hdl->s_lock);

        crc = CRC16(hdl->uartSendBuf1, V2_FRAME_PAYLOAD);
        /*
         * CRC 存在半字索引 256，即字节偏移 512，紧跟净载荷。
         * 最后一个半字是填充，原库不写它 —— 这里也不写，缓冲只 zalloc
         * 一次且这个位置无人触碰，所以恒为 0。接收端不解析这 2 字节。
         */
        hdl->uartSendBuf1[V2_FRAME_PAYLOAD / 2U] = crc;

        aec_uart_send_frame(hdl->uartSendBuf1, V2_FRAME_LEN);
        data_len -= V2_FRAME_PAYLOAD;
    }
    hdl->busy = 0U;
}

int aec_uart_close_v2(void)
{
    struct aec_uart_s *hdl = aec_uart;
    void *align_buf;

    if (!hdl) {
        return -1;
    }
    printf("aec_uart_close\n");
    aec_uart   = 0;
    hdl->state = 0U;

    align_buf = hdl->align_cb.begin;
    if (align_buf) {
        free(align_buf);
    }
    if (hdl->uartSendBuf1) {
        free(hdl->uartSendBuf1);
    }
    if (hdl->total_len) {
        free(hdl->total_len);
    }
    if (hdl->seqn) {
        free(hdl->seqn);
    }
    free(hdl);
    return 0;
}
