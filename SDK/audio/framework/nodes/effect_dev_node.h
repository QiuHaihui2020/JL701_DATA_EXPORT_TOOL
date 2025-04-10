#ifndef __EFFECT_DEV_NODE__H
#define __EFFECT_DEV_NODE__H


#include "system/includes.h"
#include "config/config_interface.h"
#include "crc.h"
#include "fs/resfile.h"
#include "effects/AudioEffect_DataType.h"
#include "media/framework/include/jlstream.h"
#include "media_memory.h"


struct user_effect_tool_param {
    int   int_param[8];
    float float_param[8];
};


#define debug_digital(x)  __builtin_abs((int)((x - (int)x) * 100)) //此处使用该宏定义，获取浮点两位小数，转int型后打印

#endif
