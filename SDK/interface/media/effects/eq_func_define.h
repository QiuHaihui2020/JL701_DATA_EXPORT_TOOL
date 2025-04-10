#ifndef _EQ_FUNC_DEFINE_H
#define _EQ_FUNC_DEFINE_H

#include "generic/typedef.h"

extern const int AUDIO_EQ_MAX_SECTION;
extern const int const_eq_debug;
extern const int config_audio_eq_en;
enum {
    EQ_EN = BIT(0),//eq模式使能
    EQ_HW_CROSSOVER_TYPE0_EN  = BIT(1),//支持硬件分频器,且分频器使用序列进序列出，需使能BIT(13)
    EQ_HW_CROSSOVER_TYPE1_EN  = BIT(2),//硬件分频器使用使用块出方式，会增加mem(该方式仅支持单声道处理)
    EQ_FADE_TACTICS_SEL = BIT(3),//eq系数淡入策略选择，使能时淡入策略耗时较长
    EQ_FADE_DISABLE = BIT(4),        //系数淡入关闭
    EQ_ASYNC_EN = BIT(5),       //异步输出逻辑使能
};

#define hw_crossover_type0             (config_audio_eq_en & EQ_HW_CROSSOVER_TYPE0_EN)
#define hw_crossover_type1             (config_audio_eq_en & EQ_HW_CROSSOVER_TYPE1_EN)
#define config_eq_fade_tactics         (config_audio_eq_en & EQ_FADE_TACTICS_SEL)
#define config_eq_fade_en              (!(config_audio_eq_en & EQ_FADE_DISABLE))
#define config_eq_async_en             (config_audio_eq_en & EQ_ASYNC_EN)

#endif
