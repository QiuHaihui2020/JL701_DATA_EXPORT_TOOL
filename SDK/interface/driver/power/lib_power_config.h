#ifndef __LIB_POWER_CONFIG_H__
#define __LIB_POWER_CONFIG_H__

//-------------------------------------------------------------------
/*调试pdown进不去的场景，影响低功耗流程
 * 打印蓝牙和系统分别可进入低功耗的时间(msec)
 * 打印当前哪些模块处于busy,用于蓝牙已经进入sniff但系统无法进入低功耗的情况，如果usr_timer处于busy则会打印对应的func地址
 */
extern const char debug_is_idle;

//-------------------------------------------------------------------
/* 调试快速起振信息，不影响低功耗流程
 */
extern const bool pdebug_xosc_resume;

//-------------------------------------------------------------------
/* 调试低功耗流程
 */
//出pdown打印信息，不影响低功耗流程
extern const bool pdebug_pdown_info;

//使能串口调试低功耗，在pdown、soff模式保持串口, pdebug_pubyte_pdown\pdebug_lp_dump_ram\pdebug_putbyte_soff\log_debug
extern const u32 pdebug_uart_lowpower;
extern const u32 pdebug_uart_port;

//使能串口putbyte调试pdown流程
extern const bool pdebug_putbyte_pdown;

//使能串口putbyte调试soff流程
extern  const bool pdebug_putbyte_soff;

//使能串口pdown/poff/soff打印所有的寄存器
extern const bool pdebug_lp_dump_ram;

//使能uart_flowing
extern const bool pdebug_uart_flowing;

//使能低功耗耗时检查
extern const bool pdebug_reserve_time;

//-------------------------------------------------------------------
/* pmu相关模块开关 */
//使能soff模块
extern const bool control_soff;

//使能pdown模块
extern const bool control_pdown;

//使能poff模块
extern const bool control_poff;

extern const bool lib_pmu_config_pdown;

extern const bool lib_pmu_config_soff;

extern const bool lib_pmu_config_poff;
#endif
