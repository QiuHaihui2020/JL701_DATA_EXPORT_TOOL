#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uartPcmSender.data.bss")
#pragma data_seg(".uartPcmSender.data")
#pragma const_seg(".uartPcmSender.text.const")
#pragma code_seg(".uartPcmSender.text")
#endif
#include "app_config.h"
#include "uartPcmSender.h"
#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
#include "system/includes.h"
#include "uart.h"

struct uart_send_hdl_t {
    int uart;
    u8 *dma_buf;
    int dma_buf_size;
};
struct uart_send_hdl_t *uart_send_hdl = NULL;

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
            int wlen = uart_send_blocking(hdl->uart, hdl->dma_buf, hdl->dma_buf_size, 100);
            if (wlen != hdl->dma_buf_size) {
                putchar('f');
            }
        }
    }
    return;

}

void uartSendInit()
{
    if (uart_send_hdl) {
        return;
    }
    struct uart_send_hdl_t *hdl = zalloc(sizeof(*hdl));
    uart_send_hdl = hdl;
    struct uart_config ut = {
        .baud_rate = PCM_UART1_BAUDRATE,
        .tx_pin = PCM_UART1_TX_PORT,
        .rx_pin = PCM_UART1_RX_PORT,
		.tx_wait_mutex = 1,
    };

    hdl->uart = uart_init(-1, &ut);
    if (hdl->uart < 0) {
        printf("open uart dev err\n");
        hdl->uart  = -1;
    }

    struct uart_dma_config dma_config = {
        .event_mask = UART_EVENT_TX_DONE,
    };
    uart_dma_init(hdl->uart, &dma_config);

    int clock_lock(const char *name, u32 clk);
    clock_lock("sys", clk_get_max_frequency());
}

void uartSendExit()
{
    struct uart_send_hdl_t *hdl = uart_send_hdl;
    if (hdl) {
        if (hdl->uart != -1) {
            uart_deinit(hdl->uart);
            hdl->uart = -1 ;
        }

        if (hdl->dma_buf) {
            dma_free(hdl->dma_buf);
            hdl->dma_buf = NULL;
        }
        free(hdl);
        hdl = NULL;
        uart_send_hdl = NULL;
    }
}

#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/
