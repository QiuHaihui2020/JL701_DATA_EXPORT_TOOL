#include "asm/power_interface.h"


//-------------------------------------------------------------------
/*config
 */
#include "cpu/includes.h"
#include "app_config.h"
#ifdef TCFG_DEBUG_UART_TX_PIN
#define CONFIG_UART_DEBUG_PORT		TCFG_DEBUG_UART_TX_PIN
#else
#define CONFIG_UART_DEBUG_PORT		-1
#endif

//-------------------------------------------------------------------
/*调试pdown进不去的场景，影响低功耗流程
 * 打印蓝牙和系统分别可进入低功耗的时间(msec)
 * 打印当前哪些模块处于busy,用于蓝牙已经进入sniff但系统无法进入低功耗的情况，如果usr_timer处于busy则会打印对应的func地址
 */
const char debug_is_idle = 0;

//-------------------------------------------------------------------
/* 调试快速起振信息，不影响低功耗流程
 */
const bool pdebug_xosc_resume = 0;

//-------------------------------------------------------------------
/* 调试低功耗流程
 */
//出pdown打印信息，不影响低功耗流程
const bool pdebug_pdown_info = 0;

//使能串口调试低功耗，在pdown、soff模式保持串口, pdebug_pubyte_pdown\pdebug_lp_dump_ram\pdebug_putbyte_soff\log_debug
const u32 pdebug_uart_lowpower = 0;
#ifdef TCFG_DEBUG_UART_TX_PIN
const u32 pdebug_uart_port = TCFG_DEBUG_UART_TX_PIN;
#else
const u32 pdebug_uart_port = -1;
#endif

//使能串口putbyte调试pdown流程
const bool pdebug_putbyte_pdown = 0;

//使能串口putbyte调试soff流程
const bool pdebug_putbyte_soff = 0;

//使能串口pdown/poff/soff打印所有的寄存器
const bool pdebug_lp_dump_ram = 0;

//使能uart_flowing
const bool pdebug_uart_flowing = 0;

//-------------------------------------------------------------------
/* pmu相关模块开关 */
//使能pdown模块
#if (TCFG_LOWPOWER_LOWPOWER_SEL)
const bool control_pdown = 1;
#else
const bool control_pdown = 0;
#endif

/* //使能soff模块 */
/* const bool control_soff = 1; */
/*  */
/* //使能poff模块 */
/* #if (TCFG_LOWPOWER_LOWPOWER_SEL==DEEP_SLEEP_EN) */
/* const bool control_poff = 1; */
/* #else */
/* const bool control_poff = 0; */
/* #endif */

//使能pdown模块
const bool lib_pmu_config_pdown = 1;

//使能soff模块
const bool lib_pmu_config_soff = 1;

//使能poff模块
const bool lib_pmu_config_poff = 0;


