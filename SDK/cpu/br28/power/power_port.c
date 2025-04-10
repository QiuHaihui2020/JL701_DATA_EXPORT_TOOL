#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".power_port.data.bss")
#pragma data_seg(".power_port.data")
#pragma const_seg(".power_port.text.const")
#pragma code_seg(".power_port.text")
#endif
#include "asm/power_interface.h"
#include "app_config.h"
#include "includes.h"
#include "iokey.h"
#include "irkey.h"
#include "adkey.h"

static u8 gpiousb = 0x3;

/* cpu公共流程：
 * 请勿添加板级相关的流程，例如宏定义
 * 可以重写改流程
 * 所有io保持原状，除usb io
 */
static struct gpio_value soff_gpio_config = {
    .gpioa = 0xffff,
    .gpiob = 0xffff,
    .gpioc = 0xffff,
    .gpiod = 0xffff,
    .gpioe = 0xffff,
    .gpiog = 0xffff,
    .gpiop = 0x1,//
    .gpiousb = 0x3,
};

void soff_gpio_protect(u32 gpio)
{
    if (gpio < IO_MAX_NUM) {
        port_protect((u16 *)&soff_gpio_config, gpio);
    } else if (gpio == IO_PORT_DP) {
        soff_gpio_config.gpiousb &= ~BIT(0);
    } else if (gpio == IO_PORT_DM) {
        soff_gpio_config.gpiousb &= ~BIT(1);
    }
}

/* cpu公共流程：
 * 请勿添加板级相关的流程，例如宏定义
 * 可以重写改流程
 * 释放除内置flash外的所有io
 */
void board_set_soft_poweroff_common(void *priv)
{
    if (pdebug_uart_lowpower) {
        soff_gpio_protect(pdebug_uart_port);
    }

    //flash电源
    if (get_sfc_port() == 0) {
        soff_gpio_protect(SPI0_PWR_A);
        soff_gpio_protect(SPI0_CS_A);
        soff_gpio_protect(SPI0_CLK_A);
        soff_gpio_protect(SPI0_DO_D0_A);
        soff_gpio_protect(SPI0_DI_D1_A);
        if (get_sfc_bit_mode() == 4) {
            soff_gpio_protect(SPI0_WP_D2_A);
            soff_gpio_protect(SPI0_HOLD_D3_A);
        }
    } else {
        soff_gpio_protect(SPI0_PWR_B);
        soff_gpio_protect(SPI0_CS_B);
        soff_gpio_protect(SPI0_CLK_B);
        soff_gpio_protect(SPI0_DO_D0_B);
        soff_gpio_protect(SPI0_DI_D1_B);
        if (get_sfc_bit_mode() == 4) {
            soff_gpio_protect(SPI0_WP_D2_B);
            soff_gpio_protect(SPI0_HOLD_D3_B);
        }
    }

    gpio_set_mode(PORTA, soff_gpio_config.gpioa, PORT_HIGHZ);
    gpio_set_mode(PORTB, soff_gpio_config.gpiob, PORT_HIGHZ);
    gpio_set_mode(PORTC, soff_gpio_config.gpioc, PORT_HIGHZ);
    gpio_set_mode(PORTD, soff_gpio_config.gpiod, PORT_HIGHZ);
    gpio_set_mode(PORTE, soff_gpio_config.gpioe, PORT_HIGHZ);
    gpio_set_mode(PORTG, soff_gpio_config.gpiog, PORT_HIGHZ);
    gpio_set_mode(PORTP, soff_gpio_config.gpiop, PORT_HIGHZ);

    gpio_set_mode(PORTUSB, soff_gpio_config.gpiousb, PORT_HIGHZ); //dp dm
}

/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
void gpio_config_soft_poweroff(void)
{
#if TCFG_IOKEY_ENABLE
    soff_gpio_protect(get_iokey_power_io());
#endif

#if TCFG_ADKEY_ENABLE
    soff_gpio_protect(get_adkey_io());
#endif

#if TCFG_IRKEY_ENABLE
    soff_gpio_protect(get_irkey_io());
#endif

    board_set_soft_poweroff_common(NULL);
}


