#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".power_app.data.bss")
#pragma data_seg(".power_app.data")
#pragma const_seg(".power_app.text.const")
#pragma code_seg(".power_app.text")
#endif
#include "asm/power_interface.h"
#include "app_config.h"
#include "includes.h"
#include "gpio_config.h"

//-------------------------------------------------------------------
/*config
 */
#define CONFIG_UART_DEBUG_ENABLE	CONFIG_DEBUG_ENABLE
#ifdef TCFG_DEBUG_UART_TX_PIN
#define CONFIG_UART_DEBUG_PORT		TCFG_DEBUG_UART_TX_PIN
#else
#define CONFIG_UART_DEBUG_PORT		-1
#endif

#define DO_PLATFORM_UNINITCALL()	do_platform_uninitcall()

static u32 usb_io_con = 0;
void sleep_enter_callback()
{
    u32 value = 0xffff;
    putchar('<');

    //USB IO打印引脚特殊处理
#if (CONFIG_DEBUG_ENABLE && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_DP) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_UART_COMM) && ((TCFG_ONLINE_TX_PORT == IO_PORT_DP) || (TCFG_ONLINE_RX_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)) || \
    TCFG_USB_HOST_ENABLE || TCFG_PC_ENABLE
    usb_io_con = JL_USB_IO->CON0;
#if TCFG_CHARGESTORE_PORT == IO_PORT_DP
    //fix: 串口升级配了DP脚下拉唤醒，进低功耗的时候把DP拉低了又唤醒了，就一直没怎么进低功耗
    value &= ~BIT(0);
#endif
#endif

    gpio_close(PORTUSB, value);
}

void sleep_exit_callback()
{
#if (CONFIG_DEBUG_ENABLE && ((TCFG_DEBUG_UART_TX_PIN == IO_PORT_DP) || (TCFG_DEBUG_UART_TX_PIN == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_UART_COMM) && ((TCFG_ONLINE_TX_PORT == IO_PORT_DP) || (TCFG_ONLINE_RX_PORT == IO_PORT_DM))) || \
    (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)) || \
    TCFG_USB_HOST_ENABLE || TCFG_PC_ENABLE
    JL_USB_IO->CON0 = usb_io_con;
#endif

    putchar('>');
}

static void __mask_io_cfg()
{
    struct boot_soft_flag_t boot_soft_flag = {0};

    boot_soft_flag.flag1.misc.usbdm = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag1.misc.usbdp = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag1.misc.uart_key_port = 0;
    boot_soft_flag.flag1.misc.ldoin = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag2.pg2_pg3.pg2 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag2.pg2_pg3.pg3 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag3.pg4_res.pg4 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag4.fast_boot_ctrl.fast_boot = 0;
    boot_soft_flag.flag4.fast_boot_ctrl.flash_stable_delay_sel = 0;

    mask_softflag_config(&boot_soft_flag);
}

u8 power_soff_callback()
{
    DO_PLATFORM_UNINITCALL();

    __mask_io_cfg();

    gpio_config_uninit();

    void gpio_config_soft_poweroff(void);
    gpio_config_soft_poweroff();

    return 0;
}

void power_early_flowing()
{
    /* // 默认关闭长按复位0，由key_driver配置 */
    /* gpio_longpress_pin0_reset_config(IO_PORTB_01, 0, 0, 1, 1); */
    // 不开充电功能，将长按复位关闭
#if (!TCFG_CHARGE_ENABLE)
    gpio_longpress_pin1_reset_config(IO_LDOIN_DET, 0, 0, 0);
#endif

    power_early_init(0);
}

//early_initcall(_power_early_init);

static int power_later_flowing()
{
    pmu_trim(0, 0);

    power_later_init(0);

    return 0;
}

late_initcall(power_later_flowing);
