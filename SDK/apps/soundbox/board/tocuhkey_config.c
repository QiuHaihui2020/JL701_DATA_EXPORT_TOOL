#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tocuhkey_config.data.bss")
#pragma data_seg(".tocuhkey_config.data")
#pragma const_seg(".tocuhkey_config.text.const")
#pragma code_seg(".tocuhkey_config.text")
#endif
#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/iokey.h"
#include "key_driver.h"

#if TCFG_RDEC_KEY_ENABLE
//key0配置
#define TCFG_TOUCH_KEY0_PRESS_DELTA	   	    100//变化阈值，当触摸产生的变化量达到该阈值，则判断被按下，每个按键可能不一样，可先在驱动里加到打印，再反估阈值
#define TCFG_TOUCH_KEY0_PORT 				IO_PORTB_06 //触摸按键key0 IO配置
#define TCFG_TOUCH_KEY0_VALUE 				0x12 		//触摸按键key0 按键值
//key1配置
#define TCFG_TOUCH_KEY1_PRESS_DELTA	   	    100//变化阈值，当触摸产生的变化量达到该阈值，则判断被按下，每个按键可能不一样，可先在驱动里加到打印，再反估阈值
#define TCFG_TOUCH_KEY1_PORT 				IO_PORTB_07 //触摸按键key1 IO配置
#define TCFG_TOUCH_KEY1_VALUE 				0x34        //触摸按键key1 按键值
#endif
