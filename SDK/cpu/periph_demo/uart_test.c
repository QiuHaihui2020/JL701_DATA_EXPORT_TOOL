#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart_test.data.bss")
#pragma data_seg(".uart_test.data")
#pragma const_seg(".uart_test.text.const")
#pragma code_seg(".uart_test.text")
#endif
#include "system/includes.h"
#include "uart.h"
#include "debug.h"

static void uart_irq_func(int uart_num, enum uart_event event)
{
    if (event & UART_EVENT_TX_DONE) {
        printf("uart[%d] tx done", uart_num);
    }

    if (event & UART_EVENT_RX_TIMEOUT) {
        printf("uart[%d] rx timerout data", uart_num);
    }

    if (event & UART_EVENT_RX_FIFO_OVF) {
        printf("uart[%d] rx fifo ovf", uart_num);
    }
}

struct uart_frame {
    u16 crc;
    u16 length;
    u8 data[0];
};
void uart_sync_demo(void *p)
{
    const struct uart_config config = {
        .baud_rate = 2000000,
        .tx_pin = IO_PORTA_04,
        .rx_pin = IO_PORTA_02,
        .parity = UART_PARITY_DISABLE,
        .tx_wait_mutex = 0,//1:不支持中断调用,互斥,0:支持中断,不互斥
    };

    void *uart_rx_ptr = dma_malloc(768);

    const struct uart_dma_config dma = {
        .rx_timeout_thresh = 100,//us,需大于1byte时间
        .event_mask = UART_EVENT_TX_DONE | UART_EVENT_RX_FIFO_OVF | UART_EVENT_RX_TIMEOUT,
        .irq_priority = 3,
        .irq_callback = uart_irq_func,
        .rx_cbuffer = uart_rx_ptr,
        .rx_cbuffer_size = 768,
        .frame_size = 768,//=rx_cbuffer_size
    };

    int r;
    uart_dev uart_id = 1;
    int ut = uart_init(uart_id, &config);
    if (ut < 0) {
        printf("init error %d", ut);
    } else {
        printf("init ok %d", ut);
    }
    r = uart_dma_init(uart_id, &dma);
    if (r < 0) {
        printf("dma init error %d", ut);
    } else {
        printf("dma init ok %d", ut);
    }

    uart_dump();
    struct uart_frame *frame = (struct uart_frame *)dma_malloc(512);

    while (1) {
        r = uart_recv_blocking(uart_id, frame, 512, 10);
        if (r > 0) { //ok
            printf("r:%d\n", r);
            printf_buf((u8 *)frame, r);
        }
        r = uart_send_blocking(uart_id, frame, r, 20);
    }
}
void uart_demo()
{
    int err = task_create(uart_sync_demo, NULL, "periph_demo");
    if (err != OS_NO_ERR) {
        r_printf("creat fail %x\n", err);
    }
}
