#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rdeckey_config.data.bss")
#pragma data_seg(".rdeckey_config.data")
#pragma const_seg(".rdeckey_config.text.const")
#pragma code_seg(".rdeckey_config.text")
#endif
#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key_driver.h"

#ifdef TCFG_RDEC_KEY_ENABLE
//RDEC0配置
#define TCFG_RDEC0_ECODE1_PORT					IO_PORTA_03
#define TCFG_RDEC0_ECODE2_PORT					IO_PORTA_04
#define TCFG_RDEC0_KEY0_VALUE 				 	0
#define TCFG_RDEC0_KEY1_VALUE 				 	1

//RDEC1配置
#define TCFG_RDEC1_ECODE1_PORT					IO_PORTA_05
#define TCFG_RDEC1_ECODE2_PORT					IO_PORTA_06
#define TCFG_RDEC1_KEY0_VALUE 				 	2
#define TCFG_RDEC1_KEY1_VALUE 				 	3

//RDEC2配置
#define TCFG_RDEC2_ECODE1_PORT					IO_PORTB_00
#define TCFG_RDEC2_ECODE2_PORT					IO_PORTB_01
#define TCFG_RDEC2_KEY0_VALUE 				 	4
#define TCFG_RDEC2_KEY1_VALUE 				 	5
#endif
