#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".chargestore.data.bss")
#pragma data_seg(".chargestore.data")
#pragma const_seg(".chargestore.text.const")
#pragma code_seg(".chargestore.text")
#endif
#include "generic/typedef.h"
#include "gpio.h"
#include "asm/power_interface.h"
#include "asm/hwi.h"
#include "gpio.h"
#include "asm/charge.h"
#include "asm/chargestore.h"
#include "update.h"
#include "app_config.h"
#include "clock.h"
#include "uart.h"

#if (TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE)
struct chargestore_handle {
    const struct chargestore_platform_data *data;
    s32 udev;
};

#define DMA_BUF_LEN 544
#define __this  (&hdl)
static struct chargestore_handle hdl;
u8 uart_rx_buf[DMA_BUF_LEN] __attribute__((aligned(4)));
u8 uart_dma_buf[DMA_BUF_LEN] __attribute__((aligned(4)));

enum {
    UPGRADE_NULL = 0,
    UPGRADE_USB_HARD_KEY,
    UPGRADE_USB_SOFTKEY,
    UPGRADE_UART_SOFT_KEY,
    UPGRADE_UART_ONE_WIRE_HARD_KEY,
};

extern void nvram_set_boot_state(u32 state);
void chargestore_set_update_ram(void)
{
#if defined(CONFIG_CPU_BR28)
    if (__this->data->io_port != IO_PORT_LDOIN) {
        u8 *p = (u8 *)BOOT_STATUS_ADDR;
        memcpy(p, "UART_UPDATE_CUSTOM", sizeof("UART_UPDATE_CUSTOM"));
    } else
#endif
    {
        //需要补充设置ram
        int tmp;
        __asm__ volatile("%0 =icfg" : "=r"(tmp));
        tmp &= ~(3 << 8);
        __asm__ volatile("icfg = %0" :: "r"(tmp));//GIE1
        local_irq_disable();
        nvram_set_boot_state(UPGRADE_UART_SOFT_KEY);
    }
}

void __attribute__((weak)) chargestore_data_deal(u8 cmd, u8 *data, u32 len)
{

}

void __attribute__((weak)) chargestore_uart_data_deal(u8 *data, u8 len)
{

}

static u8 chargestore_get_f95_det_res(u32 equ_res)
{
    u8 det_res = (equ_res + 50) / 100;
    if (det_res > 0) {
        det_res -= 1;
    }
    if (det_res > 0x0f) {
        det_res = 0x0f;
    }
    return det_res;
}

u8 chargestore_get_det_level(u8 chip_type)
{
    u32 res = 1600;
    switch (chip_type) {
    case TYPE_F95:
        if (IS_L5V_LOAD_EN()) {
            res = (GET_L5V_RES_DET_S_SEL() + 1) * 50;
        }
        return chargestore_get_f95_det_res(res);
    case TYPE_NORMAL:
    default:
        return 0x0f;
    }
}

static void chargestore_uart_isr_cb(uart_dev uart_num, enum uart_event event)
{
    s32 rx_len;
    if (event == UART_EVENT_RX_DATA) {
    } else if (event == UART_EVENT_TX_DONE) {
        if (__this->data->io_port == IO_PORT_LDOIN) {
            gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_INPUT_FLOATING);
        }
        chargestore_data_deal(CMD_COMPLETE, NULL, 0);
    } else if (event == UART_EVENT_RX_TIMEOUT) {
        rx_len = uart_recv_bytes(__this->udev, uart_rx_buf, DMA_BUF_LEN);
        if (rx_len > 0) {
            chargestore_data_deal(CMD_RECVDATA, uart_rx_buf, rx_len);
            chargestore_uart_data_deal(uart_rx_buf, rx_len);
        }
    }
}

void chargestore_write(u8 *data, u8 len)
{
    if (__this->udev < 0) {
        return;
    }
    if (((u32)data) % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    uart_send_bytes(__this->udev, (const void *)data, (u32)len);
}

void chargestore_open(u8 mode)
{
    s32 ret;
    u32 uart_timeout;
    struct uart_config config = {0};
    struct uart_dma_config dma = {0};

#if (TCFG_CHARGE_ENABLE && (TCFG_CHARGESTORE_PORT == IO_PORT_LDOIN))
    if (mode == MODE_RECVDATA) {
        charge_set_ldo5v_detect_stop(0);
    } else {
        charge_set_ldo5v_detect_stop(1);
    }
#endif

    if (__this->udev >= 0) {
        return;
    }

    config.baud_rate = __this->data->baudrate;
    config.tx_pin = __this->data->io_port;
    config.rx_pin = __this->data->io_port;
    config.parity = UART_PARITY_DISABLE;
    __this->udev = uart_init(-1, &config);
    if (__this->udev < 0) {
        goto __err_exit;
    }

    //如果为VPWR引脚,则关闭驱动开启的上拉
    if (__this->data->io_port == IO_PORT_LDOIN) {
        gpio_set_mode(IO_PORT_SPILT(__this->data->io_port), PORT_INPUT_FLOATING);
    }

    uart_timeout = 20 * 1000000 / __this->data->baudrate;//us
    dma.rx_timeout_thresh = uart_timeout;
    dma.frame_size = DMA_BUF_LEN;
    dma.event_mask = UART_EVENT_RX_DATA | UART_EVENT_RX_TIMEOUT | UART_EVENT_TX_DONE;
    dma.irq_priority = 2;
    dma.irq_callback = chargestore_uart_isr_cb;
    dma.rx_cbuffer = uart_dma_buf;
    dma.rx_cbuffer_size = DMA_BUF_LEN;
    ret = uart_dma_init(__this->udev, &dma);
    if (ret < 0) {
        goto __err_exit;
    }
    return;

__err_exit:
    ASSERT(0, "chargebox open err!\n");
    if (__this->udev >= 0) {
        uart_deinit(__this->udev);
    }
    __this->udev = -1;
}

void chargestore_close(void)
{
    if (__this->udev < 0) {
        return;
    }
    uart_deinit(__this->udev);
    __this->udev = -1;
#if (TCFG_CHARGE_ENABLE && (TCFG_CHARGESTORE_PORT == IO_PORT_LDOIN))
    charge_set_ldo5v_detect_stop(0);
#endif
}

void chargestore_set_baudrate(u32 baudrate)
{
    u32 uart_timeout;
    if (__this->udev < 0) {
        return;
    }
    uart_timeout = 20 * 1000000 / baudrate;
    uart_set_baudrate(__this->udev, baudrate);
    uart_set_rx_timeout_thresh(__this->udev, uart_timeout);
}

void chargestore_init(const struct chargestore_platform_data *data)
{
    __this->data = (struct chargestore_platform_data *)data;
    ASSERT(data);
    __this->udev = -1;
}

#endif


