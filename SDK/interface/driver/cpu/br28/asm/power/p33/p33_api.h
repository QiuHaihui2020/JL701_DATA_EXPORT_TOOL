#ifndef __P33_API_H__
#define __P33_API_H__


void pvdd_config(u32 lev, u32 low_lev, u32 output);
void pvdd_output(u32 output);


//
//
//                    pinr
//
//
//
//******************************************************************
void gpio_longpress_pin0_reset_config(u32 pin, u32 level, u32 time, u32 release, u32 pull_enable);
void gpio_longpress_pin1_reset_config(u32 pin, u32 level, u32 time, u32 release);

/*
1.类型：支持普通io / 模拟io
2.滤波：普通io无滤波参数、模拟io可配置滤波参数
3.边沿：支持普通io单边沿，模拟io可配置双边沿
*/
#define MAX_WAKEUP_PORT     12  //最大同时支持数字io输入个数
#define MAX_WAKEUP_ANA_PORT 3   //最大同时支持模拟io输入个数

#endif
