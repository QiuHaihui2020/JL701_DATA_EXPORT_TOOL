#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".irkey.data.bss")
#pragma data_seg(".irkey.data")
#pragma const_seg(".irkey.text.const")
#pragma code_seg(".irkey.text")
#endif
#include "key_driver.h"
#include "irkey.h"
#include "gpio.h"
#include "ir_decoder.h"
#include "app_config.h"

#if TCFG_IRKEY_ENABLE

static const struct irkey_platform_data *__this = NULL;


//按键驱动扫描参数列表
struct key_driver_para irkey_scan_param = {
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .key_type		  = KEY_DRIVER_TYPE_IR,
    .filter_time  	  = 0,				//按键消抖延时;
    .long_time 		  = 6,  			//按键判定长按数量
    .hold_time 		  = 9,  	//按键判定HOLD数量
    .click_delay_time = 2,				//按键被抬起后等待连击延时数量
    .scan_time 	  	  = 110,				//按键扫描频率, 单位: ms
};


u8 ir_get_key_value(void)
{
    u8 tkey = 0xff;
    u8 key_value = 0xff;
    /* tkey = ir_decoder_get_command_value(); */
    tkey = ir_decoder_get_command_value_uncheck();
    if (tkey == 0xff) {
        return tkey;
    }

    /* printf("ir tkey : 0x%02x", tkey); */
    /* return 0xff; */

    for (u8 i = 0; i < __this->num; i++) {
        if (tkey == __this->IRff00_2_keynum[i].source_value) {
            key_value = __this->IRff00_2_keynum[i].key_value;
            break;
        }
    }
    /* printf("ir key_value: %d\n", key_value); */
    /* return 0xff; */

    return key_value;
}

__attribute__((weak))
const struct irkey_platform_data *get_irkey_platform_data()
{
    return NULL;
}

int irkey_init(void)
{
    __this = get_irkey_platform_data();

    if (__this == NULL) {
        return -EINVAL;
    }
    if (!__this->enable) {
        return KEY_NOT_SUPPORT;
    }
    printf("irkey_init ");

    const struct gptimer_config ir_decode_config = {
        .capture.filter = 0,//38000,
        .capture.max_period = 110 * 1000, //110ms
        .capture.port = __this->port / IO_GROUP_NUM, //PORTA,
        .capture.pin = BIT(__this->port % IO_GROUP_NUM), //BIT(1),
        .irq_cb = NULL,
        .irq_priority = 1,
        //根据红外模块的 idle 电平状态，选择边沿触发方式
        .mode = GPTIMER_MODE_CAPTURE_EDGE_FALL,
        /* .mode = GPTIMER_MODE_CAPTURE_EDGE_RISE, */
    };
    ir_decoder_init(&ir_decode_config); //红外信号接收IO_PORTA_O1


    return 0;
}

REGISTER_KEY_OPS(irkey) = {
    .idle_query_en    = 1,
    .param            = &irkey_scan_param,
    .get_value 		  = ir_get_key_value,
    .key_init         = irkey_init,
};

#endif

