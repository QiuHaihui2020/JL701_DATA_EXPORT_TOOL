#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart_update_driver.data.bss")
#pragma data_seg(".uart_update_driver.data")
#pragma const_seg(".uart_update_driver.text.const")
#pragma code_seg(".uart_update_driver.text")
#endif
/* #include "typedef.h" */
/* #include "asm/hwi.h" */
/* #include "gpio.h" */
/* #include "asm/clock.h" */
/* #include "uart_update.h" */
#include "app_config.h"
#include "update_interactive_uart.h"

#include "system/includes.h"
#include "uart.h"

#if CONFIG_UPDATE_MUTIL_CPU_UART

#if 0
int clk_get(const char *name);
#define UART_SRC_CLK    clk_get("uart")
#define UART_OT_CLK     clk_get("lsb")
static uart_update_cfg update_cfg = {.rx = -1, .tx = -1};
#endif

#define UART_NUM_IN_CPU  3
typedef struct {
    u8 uart_update_init_succ;//初始化成功标志
    /* int update_uart_sel;//uart 0、1、2选择 */
    void (*uart_update_isr_callback)(void *buf, u32 len); //回调函数
    u8 *uart_dma_buf; //传入uart驱动的DMAbuf
} UART_UP_CTL;

static UART_UP_CTL uart_up_ctl[UART_NUM_IN_CPU] = {0};
#define __this(x)  (&uart_up_ctl[x])

#define UART_DMA_LEN        544 //512 + 32  2的n次倍且大于句柄 protocal_frame 528+6
/* #define UART_NUM            -1 //-1表示uart返回可选的uart0\1\2,可能存在其他CPU需要手动选择 */
#define UART_DEFAULT_BAUD   9600

void update_interactive_uart_set_dir(int uart_dev, u8 mode)
{
}

static u8 __uart_num_check(u32 uart_num)
{
    u8 res = 0;
    if (uart_num > UART_NUM_IN_CPU) {
        y_printf(">>>[test]:uart_num over cur: %d, max: %d\n", uart_num, UART_NUM_IN_CPU);
        res = 1;
    }
    return res;
}

static void update_interactive_uart_irq_func(int uart_dev, enum uart_event event)
{
    if (event & UART_EVENT_TX_DONE) {
        /* printf("uart[%d] tx done", uart_dev); */
    }

    if (event & (UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT)) {
        /* printf("uart[%d] rx data", uart_dev); */
        /* printf(">>>[test]:uart_rx len = %d, event = %d\n", len, event); */
        if (__this(uart_dev)->uart_update_isr_callback) {
            __this(uart_dev)->uart_update_isr_callback(NULL, 0);
        }
    }

    if (event & UART_EVENT_RX_FIFO_OVF) {
        printf("uart[%d] rx fifo ovf", uart_dev);
    }
}

u32 update_interactive_uart_write(int uart_dev, u8 *data, u32 len, u32 timeout_ms)
{
    /* y_printf(">>>[test]:data = 0x%x, len = %d, timeout_ms = %d\n", data, len, timeout_ms); */
    u32 ret = 0;
    if (__uart_num_check(uart_dev)) {
        return -1;
    }
    if (__this(uart_dev)->uart_update_init_succ) {
        ret = uart_send_blocking(uart_dev, data, len, timeout_ms);
    }
    return ret;
}

u32 update_interactive_uart_read(int uart_dev, u8 *data, u32 len, u32 timeout_ms)
{
    /* y_printf(">>>[test]:data = 0x%x, len = %d, timeout_ms = %d\n", data, len, timeout_ms); */
    u32 ret = 0;
    if (__uart_num_check(uart_dev)) {
        return -1;
    }
    if (__this(uart_dev)->uart_update_init_succ) {
        ret = uart_recv_blocking(uart_dev, data, len, timeout_ms);
    }
    return ret;
}

u32 update_interactive_uart_read_no_timeout(int uart_dev, u8 *data, u32 len)
{
    /* y_printf(">>>[test]:data = 0x%x, len = %d, timeout_ms = %d\n", data, len, timeout_ms); */
    u32 ret = 0;
    if (__uart_num_check(uart_dev)) {
        return -1;
    }
    if (__this(uart_dev)->uart_update_init_succ) {
        ret = uart_recv_bytes(uart_dev, data, len);
    }
    return ret;
}


void update_interactive_uart_set_baud(int uart_dev, u32 baudrate)
{
    y_printf(">>>[test]:baud = %d\n", baudrate);
    if (__uart_num_check(uart_dev)) {
        return;
    }
    if (__this(uart_dev)->uart_update_init_succ) {
        uart_set_baudrate(uart_dev, baudrate);
        u32 timeout_thresh = 1 * 1000000 * 10 * 5 / baudrate; // 1 / baudrate * 1000000 * 10 * n字节（5-10）设置完波特率之后，修改rx起pend的时间
        y_printf(">>>[test]: timeout_thresh = %d\n", timeout_thresh);
        uart_set_rx_timeout_thresh(uart_dev, timeout_thresh);
    }
}

void update_interactive_uart_hw_init(int uart_dev, void (*cb)(void *, u32))
{
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
    if (__uart_num_check(uart_dev)) {
        return;
    }

    __this(uart_dev)->uart_dma_buf = (u8 *)dma_malloc(UART_DMA_LEN);
    __this(uart_dev)->uart_update_isr_callback = cb;

    struct uart_config uart_up_config = {-1};
    uart_up_config.baud_rate = UART_DEFAULT_BAUD; //默认初始波特率为9600
    if (uart_dev == TCFG_UPDATE_UART_DEV) {
        uart_up_config.tx_pin = TCFG_UART_UPDATE_TX_PIN;
        uart_up_config.rx_pin = TCFG_UART_UPDTAE_RX_PIN;
    }
    if (uart_dev == CONFIG_UPDATE_MUTIL_CPU_UART_DEV) {
        uart_up_config.tx_pin = CONFIG_UPDATE_MUTIL_CPU_UART_TX_PIN;
        uart_up_config.rx_pin = CONFIG_UPDATE_MUTIL_CPU_UART_RX_PIN;
    }

    memset((void *)__this(uart_dev)->uart_dma_buf, 0, UART_DMA_LEN);
    struct uart_dma_config uart_up_dma = {
        .rx_timeout_thresh = 1370,
        .frame_size = UART_DMA_LEN,
        .event_mask = UART_EVENT_TX_DONE | UART_EVENT_RX_DATA | UART_EVENT_RX_FIFO_OVF | UART_EVENT_RX_TIMEOUT,
        /* .tx_wait_mutex = 0, */
        /* .irq_priority= 3, */
        .irq_callback = update_interactive_uart_irq_func,
        .rx_cbuffer = __this(uart_dev)->uart_dma_buf,
        .rx_cbuffer_size = UART_DMA_LEN,
    };

    int uart_sel = uart_init(uart_dev, &uart_up_config);
    r_printf(">>>[test]:init uart = %d\n", uart_sel);
    if (uart_sel < 0) {
        printf("uart_init error %d", uart_sel);
        return;
    }
    int r = uart_dma_init(uart_dev, &uart_up_dma);
    if (r < 0) {
        printf("uart_dma_init error %d", r);
        return;
    }
    /* uart_dump(); */
    __this(uart_dev)->uart_update_init_succ = 1;


#if 0
    u8 *frame = (u8 *)dma_malloc(512);
    int cnt = 0;

    while (1) {
        cnt += 2;
        memset(frame, 0, 512);
        r = uart_recv_blocking(uart_dev, frame, 512, 0);
        printf(">>>[test]:recv r = %d\n", r);
        put_buf(frame, 16);

        memset(frame, cnt & 0xff, 512);
        r = uart_send_blocking(uart_dev, frame, 512, 0);
        printf(">>>[test]:send r = %d\n", r);
        put_buf(frame, 16);
    }
#endif
}



void update_interactive_uart_close_deal(int uart_dev)
{
    if (__uart_num_check(uart_dev)) {
        return;
    }
    if (__this(uart_dev)->uart_dma_buf) {
        dma_free(__this(uart_dev)->uart_dma_buf);
        __this(uart_dev)->uart_dma_buf = NULL;
    }
    if (__this(uart_dev)->uart_update_init_succ) {
        uart_deinit(uart_dev);
    }
    __this(uart_dev)->uart_update_init_succ = 0;
}

/* void uart_close(void) */
/* { */
/*     uart_close_deal(); */
/* } */

#endif

