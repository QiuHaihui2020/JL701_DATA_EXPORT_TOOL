#include "app_config.h"
#include "system/includes.h"
#include "circular_buf.h"
#include "uart.h"
#include "dev_manager.h"
#include "display/display_task.h"
#include "audio_raw_writer.h"

#define PCM_UART1_TX_PORT			-1
#define PCM_UART1_RX_PORT			IO_PORTA_02

/*vm参数读取失败时的默认参数*/
#define PCM_UART1_BAUDRATE			4000000		/*数据导出波特率,不用修改，和接收端设置一直*/
#define PCM_CH                      3
#define PCM_SINGLE_LEN              512

#define SD_CBUF_CNT     30 //cbuf大小为SD_CBUF_CNT * 1.5k byte

#define AUDIO_UART_TASK_NAME    "a_uart_rec"
#define AUDIO_SDWRITE_TASK_NAME "a_sd_write"

struct audio_dbg_hdl_t {
    int uart;
    u8 *uart_dma_buf;
    int uart_dma_buf_size;
    u32 uart_baud_rate;
    int uart_frame_size;
    u16 pcm_rx_single_size;
    u8 pcm_channel;

    OS_SEM uart_sem;
    u16 *uart_tmp_buf;
    u8 *uart_buf;
    cbuffer_t uart_cbuf;

    int sd_write_frame_size;
    OS_SEM sd_sem;
    u16 *sd_tmp_buf;
    u8 *sd_buf;
    cbuffer_t sd_cbuf;
    u8 raw_ready;              /*分通道 raw 输出是否就绪*/
    /*
     * 定时落盘请求。由定时器回调置位、sd 任务清零，volatile 保证可见性。
     * 这里不需要临界区: 单字节标志，且漏掉一次只是把落盘推后一个周期，无副作用。
     */
    volatile u8 flush_req;

    u16 lost_packet;

};
static struct audio_dbg_hdl_t *aud_dbg_hdl = NULL;

#if 0
void uartSendData(void *buf, u16 len) 			//发送数据的接口。
{
    struct uart_send_hdl_t *hdl = uart_send_hdl;
    uartSendInit();
    if (hdl) {
        if (hdl->uart != -1) {
            if (hdl->dma_buf == NULL) {
                printf("%s : %d", __func__, __LINE__);
                hdl->dma_buf_size = len;
                hdl->dma_buf = dma_malloc(hdl->dma_buf_size);
            }
            if (hdl->dma_buf_size != len) {
                printf("%s : %d", __func__, __LINE__);
                dma_free(hdl->dma_buf);
                hdl->dma_buf_size = len;
                hdl->dma_buf = dma_malloc(hdl->dma_buf_size);
            }
            memcpy(hdl->dma_buf, buf, len);
            /* uart_send_bytes(hdl->uart, hdl->dma_buf, len); */
            uart_send_blocking(hdl->uart, hdl->dma_buf, hdl->dma_buf_size, 100);
        }
    }
    return;

}
#endif

/*
 * 丢帧时往 cbuf 补一帧零，保持交给 raw_writer 的字节流长度连续。
 * V1 靠字节在交织块内的位置判断通道归属，少一帧会让后续所有通道整体错位；
 * V2 有包头可精确定位，多出的这帧零会因找不到同步头而被自然丢弃。
 * 补零只在本任务里做，raw_writer 依然是单生产者单消费者，无需加锁。
 */
static void audio_dbg_pad_lost_frame(struct audio_dbg_hdl_t *hdl)
{
    u32 payload = (u32)hdl->uart_frame_size - 4;

    memset(hdl->uart_tmp_buf, 0, payload);
    if (cbuf_write(&hdl->sd_cbuf, hdl->uart_tmp_buf, payload) == payload) {
        os_sem_post(&hdl->sd_sem);
    }
}

static void audio_uart_task(void *priv)
{
    struct audio_dbg_hdl_t *hdl = (struct audio_dbg_hdl_t *)priv;
    int recv_len = 0;
    u16 crc16 = 0;
    int wlen = 0;
    u32  rec_cnt = 0;
    u32  last_rec_cnt = 0;
    while(1) {
        os_sem_pend(&hdl->uart_sem, 0);
        if (hdl && hdl->uart >= 0) {	//uart句柄>=0即有效(0也是合法句柄),不能用>0否则会漏掉句柄0导致收不到数据
            recv_len = uart_recv_bytes(hdl->uart, hdl->uart_tmp_buf, hdl->uart_frame_size); 
            if (recv_len == hdl->uart_frame_size) {
                crc16 = CRC16(hdl->uart_tmp_buf, hdl->uart_frame_size - 4);
                if (crc16 == hdl->uart_tmp_buf[(hdl->uart_frame_size - 4) / 2]) {
                    wlen = cbuf_write(&hdl->sd_cbuf, hdl->uart_tmp_buf, recv_len - 4); 
                    if (wlen == (recv_len - 4)) {
                        os_sem_post(&hdl->sd_sem);
                    } else {
                        printf("[error] sd cbuf full, cbuf data len %d\n", hdl->sd_cbuf.data_len);
                        hdl->lost_packet++;
                    }
                } else {
                    printf("uart crc err");
                    hdl->lost_packet++;
                    audio_dbg_pad_lost_frame(hdl);
                }
            } else {
                printf("uart recv read err, %d %d\n", recv_len, hdl->uart_frame_size);
                hdl->lost_packet++;
                audio_dbg_pad_lost_frame(hdl);
            }
            rec_cnt++;
            if ((rec_cnt % 100) == 0) {
                if (rec_cnt != last_rec_cnt) {
                    last_rec_cnt = rec_cnt;
                    oled_dispaly_task_post(OLED_DISPLAY_LOST, (int *)((int)hdl->lost_packet));
                }

            }

        }
    }

}
#define AUDIO_WRITE_DEVICE_LOGO "sd0"
#define AUDIO_WRITE_FOLDER_NAME "JL_DEBUG"

#define AUDIO_SD_FLUSH_INTERVAL 2000            /*定时落盘间隔，单位ms*/

static u32 flush_timer = 0;

/*
 * 定时落盘: 每通道要攒满一个块才写盘，数据率低或录制刚开始时块可能长时间攒不满，
 * 此时断电/拔卡就会丢掉缓冲里的全部内容。这里周期性地强制落一次盘缩小该窗口。
 * 回调里只置标志并唤醒 sd 任务，真正的落盘必须回到 sd 任务上下文执行:
 * raw_writer 内部无锁，且 fwrite 是阻塞操作，都不适合在定时器回调里做。
 */
static void audio_dbg_flush_timer(void *priv)
{
    struct audio_dbg_hdl_t *hdl = (struct audio_dbg_hdl_t *)priv;

    if (!hdl || !hdl->raw_ready) {
        return;
    }
    hdl->flush_req = 1;
    os_sem_post(&hdl->sd_sem);
}

static void audio_sdwrite_task(void *priv)
{
    struct audio_dbg_hdl_t *hdl = (struct audio_dbg_hdl_t *)priv;
    u32 rlen = 0, dlen = 0;
    u32 write_cnt = 0;
    u8  moved;                          /*本轮是否真的搬到了数据*/
    char logo[] = {AUDIO_WRITE_DEVICE_LOGO};        //sd卡录音
    char folder[] = {AUDIO_WRITE_FOLDER_NAME};
    char *root_path = dev_manager_get_root_path_by_logo(logo);

    if (!root_path) {
        printf("sd dev not found, skip raw writer open\n");
    } else if (raw_writer_open(root_path, folder,
                               hdl->pcm_channel, hdl->pcm_rx_single_size) < 0) {
        printf("raw writer open fail\n");
    } else {
        hdl->raw_ready = 1;
    }

    while(1) {
        os_sem_pend(&hdl->sd_sem, 0);
        if (!hdl->raw_ready) {
            continue;
        }
        /*
         * 有多少搬多少。原先固定按整块长度读，不足一块的尾部数据
         * 会一直卡在 cbuf 里出不来，录音末尾必然缺一截。
         */
        moved = 0;
        do {
            dlen = cbuf_get_data_len(&hdl->sd_cbuf);
            if (dlen > (u32)hdl->sd_write_frame_size) {
                dlen = (u32)hdl->sd_write_frame_size;
            }
            rlen = dlen ? cbuf_read(&hdl->sd_cbuf, hdl->sd_tmp_buf, dlen) : 0;
            if (rlen) {
                raw_writer_input((u8 *)hdl->sd_tmp_buf, rlen);
                putchar('W');
                write_cnt++;
                moved = 1;
            }
        } while (rlen);

        /*搬完 cbuf 再处理定时落盘，保证落下去的是当前最全的数据*/
        if (hdl->flush_req) {
            hdl->flush_req = 0;
            raw_writer_flush();
        }

        /*
         * w+ 和红灯只在真正搬到数据时才闪。
         * sd_sem 除了收到数据，还会被定时落盘唤醒，
         * 少了 moved 这个前置条件的话空转唤醒也会让它闪，就不再表示"正在收数据"了。
         */
        if (moved && ((write_cnt % 10) == 0)) {
            oled_dispaly_task_post(OLED_DISPLAY_RUN_TIPS, NULL);
        }
    }
}

static s16 my_testbuf[1024];
//推送到调用者的线程执行
static void uart_irq_callback(uart_dev uart_num, enum uart_event event)
{
    struct audio_dbg_hdl_t *hdl = aud_dbg_hdl;
    int err = 0;
    if ((!hdl) || (uart_num != hdl->uart)) {
        return; 
    }

    /*数据帧接收完成*/
    if (event & UART_EVENT_RX_TIMEOUT) {
        putchar('E'); 
    }
    /*开始接收到数据*/
    if (event & (UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT)) {
        putchar('R'); 
        err = os_sem_post(&hdl->uart_sem); 
    }
    /*接收缓冲区溢出*/
    if (event & UART_EVENT_RX_FIFO_OVF) {
        printf("UART_EVENT_RX_FIFO_OVF"); 
    }
    /*奇偶校验错误*/
    if (event & UART_EVENT_PARITY_ERR) {
        printf("UART_EVENT_PARITY_ERR"); 
    }
        
    /*数据发送完成*/
    if (event & UART_EVENT_TX_DONE) {
    
    }
    
}

u8 audio_uart_init_runing()
{
    if (aud_dbg_hdl) {
        return 1;
    } else {
        return 0;
    }
}

static u32 printf_timer = 0;
static void sys_info_trace(void *priv)
{
    //task_info_output(0);

    int cbuf_data_len = 0;
    if (aud_dbg_hdl) {
        cbuf_data_len = aud_dbg_hdl->sd_cbuf.data_len;
    }
    int usage[3] = { 0, 0, 0 };
    int a = os_cpu_usage(NULL, usage);
    
    if (a < 0) {
        return;
    }
    int usage_max = MAX(usage[0], usage[1]);
    int curr_clk = clk_get("sys");
    
    printf("cpu0: %d , cpu1: %d , clk:%d, cbuf data_len: %d\n", usage[0], usage[1], curr_clk, cbuf_data_len);

    a = os_cpu_usage(AUDIO_UART_TASK_NAME, NULL);
    printf("task : %s: %d\n", AUDIO_UART_TASK_NAME, a);
    a = os_cpu_usage(AUDIO_SDWRITE_TASK_NAME, NULL);
    printf("task : %s: %d\n", AUDIO_SDWRITE_TASK_NAME, a);
    a = os_cpu_usage("od_dispaly", NULL);
    printf("task : %s: %d\n", "od_dispaly", a);
    
    task_info_reset();
    mem_stats();
}

void audio_uart_init()
{
    if (aud_dbg_hdl) {
        return;
    }

    int clock_lock(const char *name, u32 clk);
    printf("======================== max clk: %d\n", clk_get_max_frequency());
    clock_lock("sys", clk_get_max_frequency());

    struct audio_dbg_hdl_t *hdl = zalloc(sizeof(*hdl));
    ASSERT(hdl);

    printf_timer = sys_timer_add(NULL, sys_info_trace, 5000);

    hdl->uart_dma_buf_size = 4096;
    hdl->uart_baud_rate = 2000000;
    hdl->pcm_rx_single_size = 512;
    hdl->pcm_channel = 3;

    int ret = 0;
    /*读取通道数ch*/
    ret = syscfg_read(CFG_UART_PCM_RX_CH, &hdl->pcm_channel, 1);
    if (ret < 0) {
        printf("pcm channel read err, use default\n");
        hdl->pcm_channel = PCM_CH;
        syscfg_write(CFG_UART_PCM_RX_CH, &hdl->pcm_channel, 1);
        printf("use default pcm channel : %d\n", hdl->pcm_channel);
    }
    printf("=================================== pcm channel : %d\n", hdl->pcm_channel);
    //oled_dispaly_task_post(OLED_DISPLAY_CH, (int *)(hdl->pcm_channel));

    /*读取单个通道的数据长度*/
    ret = syscfg_read(CFG_UART_PCM_RX_SIG_SIZE, &hdl->pcm_rx_single_size, 2);
    if (ret < 0) {
        printf("pcm_rx_single_size read err, use default\n");
        hdl->pcm_rx_single_size = PCM_SINGLE_LEN;
        syscfg_write(CFG_UART_PCM_RX_SIG_SIZE, &hdl->pcm_rx_single_size, 2);
    }
    printf("=================================== pcm_rx_single_size : %d\n", hdl->pcm_rx_single_size);
    //oled_dispaly_task_post(OLED_DISPLAY_LEN, (int *)(hdl->pcm_rx_single_size));

    /*读取波特率*/
    ret = syscfg_read(CFG_UART_PCM_RX_BAUD_RATE, &hdl->uart_baud_rate, 4);
    if (ret < 0) {
        printf("uart_baud_rate read err, use default\n");
        hdl->uart_baud_rate = PCM_UART1_BAUDRATE;
        syscfg_write(CFG_UART_PCM_RX_BAUD_RATE, &hdl->uart_baud_rate, 4);
    }
    printf("=================================== uart_baud_rate : %d\n", hdl->uart_baud_rate);
    //oled_dispaly_task_post(OLED_DISPLAY_BAUD, (int *)(hdl->uart_baud_rate));

    hdl->uart_frame_size = hdl->pcm_rx_single_size * hdl->pcm_channel + 4;

    hdl->sd_write_frame_size = 512 * 3;
    hdl->lost_packet = 0;

    hdl->sd_tmp_buf = zalloc(hdl->sd_write_frame_size);
    ASSERT(hdl->sd_tmp_buf);
    hdl->sd_buf = zalloc(hdl->sd_write_frame_size * SD_CBUF_CNT);
    ASSERT(hdl->sd_buf);
    cbuf_init(&hdl->sd_cbuf, hdl->sd_buf, hdl->sd_write_frame_size * SD_CBUF_CNT);
    os_sem_create(&hdl->sd_sem, 0);
    task_create(audio_sdwrite_task, hdl, AUDIO_SDWRITE_TASK_NAME);

    hdl->uart_tmp_buf = zalloc(hdl->uart_frame_size);
    ASSERT(hdl->uart_tmp_buf);
    hdl->uart_buf = zalloc(hdl->uart_frame_size * 3);
    ASSERT(hdl->uart_buf);
    cbuf_init(&hdl->uart_cbuf, hdl->uart_buf, hdl->uart_frame_size * 3);

    os_sem_create(&hdl->uart_sem, 0);
    task_create(audio_uart_task, hdl, AUDIO_UART_TASK_NAME);

    hdl->uart_dma_buf = dma_malloc(hdl->uart_dma_buf_size);
    ASSERT(hdl->uart_dma_buf);

    struct uart_config ut = {
        .baud_rate = hdl->uart_baud_rate,
        .tx_pin = PCM_UART1_TX_PORT,
        .rx_pin = PCM_UART1_RX_PORT,
    };

    hdl->uart = uart_init(-1, &ut);
    if (hdl->uart < 0) {
        printf("open uart dev err\n");
        hdl->uart  = -1;
    }

    struct uart_dma_config dma_config = {
        .rx_timeout_thresh = 1000,
        .frame_size = hdl->uart_frame_size,
        .event_mask = UART_EVENT_RX_TIMEOUT | UART_EVENT_RX_DATA | UART_EVENT_RX_FIFO_OVF | UART_EVENT_PARITY_ERR,
        .irq_callback = uart_irq_callback,
        .rx_cbuffer = hdl->uart_dma_buf,
        .rx_cbuffer_size = hdl->uart_dma_buf_size,
    };
    uart_dma_init(hdl->uart, &dma_config);

    /*回调内会先检查 raw_ready，此时 sd 任务尚未打开文件也是安全的*/
    flush_timer = sys_timer_add(hdl, audio_dbg_flush_timer, AUDIO_SD_FLUSH_INTERVAL);

    aud_dbg_hdl = hdl;
}

void audio_uart_exit()
{
    struct audio_dbg_hdl_t *hdl = aud_dbg_hdl;
    if (hdl) {
        if (hdl->uart != -1) {
            uart_deinit(hdl->uart);
            hdl->uart = -1 ;
        }
        task_kill(AUDIO_UART_TASK_NAME);
        task_kill(AUDIO_SDWRITE_TASK_NAME);

        if (printf_timer) {
            sys_timer_del(printf_timer);
        }
        /*必须在 free(hdl) 之前删掉，否则回调会访问已释放的 hdl*/
        if (flush_timer) {
            sys_timer_del(flush_timer);
            flush_timer = 0;
        }
        /*务必在 sd 写任务被 kill 之后再关，里面会把残留数据落盘*/
        if (hdl->raw_ready) {
            raw_writer_close();
            hdl->raw_ready = 0;
        }

        if (hdl->uart_dma_buf) {
            dma_free(hdl->uart_dma_buf);
            hdl->uart_dma_buf = NULL;
        }

        if (hdl->uart_tmp_buf) {
            free(hdl->uart_tmp_buf);
            hdl->uart_tmp_buf = NULL;
        }
        if (hdl->uart_buf) {
            free(hdl->uart_buf);
            hdl->uart_buf = NULL;
        }

        if (hdl->sd_tmp_buf) {
            free(hdl->sd_tmp_buf);
            hdl->sd_tmp_buf = NULL;
        }
        if (hdl->sd_buf) {
            free(hdl->sd_buf);
            hdl->sd_buf = NULL;
        }

        free(hdl);
        hdl = NULL;
        aud_dbg_hdl = NULL;
    }
}
