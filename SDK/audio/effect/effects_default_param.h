#ifndef _EFFECTS_DEFAULT_PARAM_
#define _EFFECTS_DEFAULT_PARAM_

#include "generic/typedef.h"

enum _pitch_level {
    PITCH_12N = 0x0, //-12 半音符
    PITCH_10N,       //-10
    PITCH_8N,        //-8
    PITCH_6N,        //-6
    PITCH_4N,        //-4
    PITCH_2N,        //-2
    PITCH_0,         //0
    PITCH_2,         //2
    PITCH_4,         //4
    PITCH_6,         //6
    PITCH_8,         //8
    PITCH_10,        //10
    PITCH_12,        //12
};

u16 audio_pitch_default_parm_set(u8 pitch_mode);

#endif
