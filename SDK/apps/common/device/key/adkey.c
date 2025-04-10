#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adkey.data.bss")
#pragma data_seg(".adkey.data")
#pragma const_seg(".adkey.text.const")
#pragma code_seg(".adkey.text")
#endif
#include "key_driver.h"
#include "adkey.h"
#include "gpio.h"
#include "app_config.h"
#include "asm/power_interface.h"


#if TCFG_ADKEY_ENABLE

static const struct adkey_platform_data *__this = NULL;

//按键驱动扫描参数列表
struct key_driver_para adkey_scan_param = {
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .key_type		  = KEY_DRIVER_TYPE_AD,
    .filter_time  	  = 2,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 20,				//按键被抬起后等待连击延时数量
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
};

u8 ad_get_key_value(void)
{
    u8 i;
    u16 ad_data;

    if (!__this->enable) {
        return NO_KEY;
    }

    /* ad_data = adc_get_voltage(__this->ad_channel); */
    ad_data = adc_get_value(__this->ad_channel);
    for (i = 0; i < CONFIG_ADKEY_MAX_NUM; i++) {
        if ((ad_data <= __this->ad_value[i]) && (__this->ad_value[i] < 0x3ffL)) {
            return __this->key_value[i];
        }
    }
    return NO_KEY;
}

__attribute__((weak))
const struct adkey_platform_data *get_adkey_platform_data()
{
    return NULL;
}

int adkey_init(void)
{
    __this = get_adkey_platform_data();

    if (!__this) {
        return -EINVAL;
    }

    if (!__this->enable) {
        return KEY_NOT_SUPPORT;
    }
    adc_add_sample_ch(__this->ad_channel);          //注意：初始化AD_KEY之前，先初始化ADC
#if (TCFG_ADKEY_LED_IO_REUSE || TCFG_ADKEY_IR_IO_REUSE || TCFG_ADKEY_LED_SPI_IO_REUSE)
#else
    if (__this->extern_up_en) {
        gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_FLOATING);
    } else {
        gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_PULLUP_10K);
    }
    gpio_set_function(IO_PORT_SPILT(__this->adkey_pin), PORT_FUNC_GPADC);
#endif

    if (__this->long_press_enable) {
        gpio_longpress_pin0_reset_config(__this->adkey_pin, 0, __this->long_press_time,  1,  !__this->extern_up_en);
    }

    return 0;
}

#if (TCFG_ADKEY_LED_IO_REUSE || TCFG_ADKEY_IR_IO_REUSE || TCFG_ADKEY_LED_SPI_IO_REUSE)

#if TCFG_ADKEY_IR_IO_REUSE
static u8 ir_io_sus = 0;
extern u8 ir_io_suspend(void);
extern u8 ir_io_resume(void);
#endif
#if TCFG_ADKEY_LED_IO_REUSE
static u8 led_io_sus = 0;
extern u8 led_io_suspend(void);
extern u8 led_io_resume(void);
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
static u8 led_spi_sus = 0;
extern u8 led_spi_suspend(void);
extern u8 led_spi_resume(void);
#endif
u8 adc_io_reuse_enter(u32 ch)
{
    if (ch == __this->ad_channel) {
#if TCFG_ADKEY_IR_IO_REUSE
        if (ir_io_suspend()) {
            return 1;
        } else {
            ir_io_sus = 1;
        }
#endif
#if TCFG_ADKEY_LED_IO_REUSE
        if (led_io_suspend()) {
            return 1;
        } else {
            led_io_sus = 1;
        }
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
        if (led_spi_suspend()) {
            return 1;
        } else {
            led_spi_sus = 1;
        }
#endif
        if (__this->extern_up_en) {
            gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_HIGHZ);
        } else {
            gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_PULLUP_10K);
            gpio_set_function(IO_PORT_SPILT(__this->adkey_pin), PORT_FUNC_GPADC);
        }
    }
    return 0;
}

u8 adc_io_reuse_exit(u32 ch)
{
    if (ch == __this->ad_channel) {
#if TCFG_ADKEY_IR_IO_REUSE
        if (ir_io_sus) {
            ir_io_sus = 0;
            ir_io_resume();
        }
#endif
#if TCFG_ADKEY_LED_IO_REUSE
        if (led_io_sus) {
            led_io_sus = 0;
            led_io_resume();
        }
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
        if (led_spi_sus) {
            led_spi_sus = 0;
            led_spi_resume();
        }
#endif
    }
    return 0;
}

#endif

REGISTER_KEY_OPS(adkey) = {
    .idle_query_en    = 1,
    .param            = &adkey_scan_param,
    .get_value        = ad_get_key_value,
    .key_init         = adkey_init,
};
#endif  /* #if TCFG_ADKEY_ENABLE */





