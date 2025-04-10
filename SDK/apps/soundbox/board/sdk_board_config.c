#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sdk_board_config.data.bss")
#pragma data_seg(".sdk_board_config.data")
#pragma const_seg(".sdk_board_config.text.const")
#pragma code_seg(".sdk_board_config.text")
#endif
#include "asm/power_interface.h"
#include "gpadc.h"
#include "app_config.h"
#include "app_power_manage.h"
#include "fm_manage.h"
#include "gpio_config.h"
#include "key/irkey.h"
#include "iokey_config.h"
#include "adkey_config.h"
#include "key/key_driver.h"




#include "sdk_config.c"

#include "../iokey_config.c"
#include "../adkey_config.c"
#include "../irkey_config.c"


#if TCFG_APP_FM_EN
#include "fm_manage.h"

FM_DEV_PLATFORM_DATA_BEGIN(fm_dev_data)
.iic_hdl = 0,
 .iic_delay = 50,
  FM_DEV_PLATFORM_DATA_END();

#endif

#if (defined(TCFG_PSRAM_DEV_ENABLE) && TCFG_PSRAM_DEV_ENABLE)
#include "asm/psram_api.h"
#include "gpio.h"
PSRAM_PLATFORM_DATA_BEGIN(psram_config)
.power_port = TCFG_PSRAM_POWER_PORT,
 .port = TCFG_PSRAM_PORT_SEL,
  .psram_clk_mhz = TCFG_PSRAM_INIT_CLK,
   .data_width = TCFG_PSRAM_MODE,
    PSRAM_PLATFORM_DATA_END()

    void psram_early_init(void)
{
    psram_init(&psram_config);
}

#else
void psram_early_init(void)
{
    return;
}
#endif

void gpio_config_set(const struct gpio_cfg_item *table, int table_size)
{
    for (int i = 0; i < table_size; i++) {
        struct gpio_config config = {
            .pin   = BIT(table[i].gpio % 16),
            .mode  = table[i].mode,
            .hd    = table[i].hd,
        };
        gpio_init(table[i].gpio / 16, &config);
    }
}

int gpio_config_init()
{
#if TCFG_IO_CFG_AT_POWER_ON
    puts("gpio_cfg_at_power_on\n");
    gpio_config_set(g_io_cfg_at_poweron, ARRAY_SIZE(g_io_cfg_at_poweron));
#endif
    return 0;
}

int gpio_config_uninit()
{
#if TCFG_IO_CFG_AT_POWER_OFF
    for (int i = 0; i < ARRAY_SIZE(g_io_cfg_at_poweroff); i++) {
        soff_gpio_protect(g_io_cfg_at_poweroff[i].gpio);
    }
    gpio_config_set(g_io_cfg_at_poweroff, ARRAY_SIZE(g_io_cfg_at_poweroff));
#endif

    return 0;
}

void board_init()
{
    board_power_init();

#if ((defined TCFG_IIS_NODE_ENABLE) && (TCFG_IIS_NODE_ENABLE == 1))
    clk_set_api("pll1", 240000000);
#endif

#if TCFG_UPDATE_UART_IO_EN
    {
#include "uart_update.h"
        uart_update_cfg  update_cfg = {
            .rx = IO_PORTA_02,
            .tx = IO_PORTA_03,
        };
        uart_update_init(&update_cfg);
    }
#endif

#if CONFIG_UPDATE_MUTIL_CPU_UART
    {
#include "update_interactive_uart.h"
        update_interactive_uart_cfg  update_interactive_cfg = {
            .rx = CONFIG_UPDATE_MUTIL_CPU_UART_TX_PIN,
            .tx = CONFIG_UPDATE_MUTIL_CPU_UART_RX_PIN,
        };
        y_printf(">>>[test]:new uart update\n");
        update_interactive_uart_init(&update_interactive_cfg);
    }
#endif

    adc_init();

#if TCFG_BATTERY_CURVE_ENABLE
    vbat_curve_init(g_battery_curve_table, ARRAY_SIZE(g_battery_curve_table));
#endif

#if (CONFIG_CPU_BR27 || CONFIG_CPU_BR29)
    extern void bt_wl_sync_clk_en(void);
    bt_wl_sync_clk_en();
#endif

#if TCFG_APP_FM_EN
    y_printf(">> Func:%s, Line:%d, call: fm_dev_init Func!\n", __func__, __LINE__);
    fm_dev_init((void *)(&fm_dev_data));
#endif

}

