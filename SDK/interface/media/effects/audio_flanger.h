#ifndef _AUDIO_FLANGER_API_H_
#define _AUDIO_FLANGER_API_H_
#include "system/includes.h"
#include "effects/lf_flangerStereo_api.h"

struct flanger_udpate {
    int feedback;             //0-100
    float mod_rate;
    float delay;
    int mod_depth;               // 0-100
    int LRphasediff;           //0-360
    int wetgain;
    int drygain;
    int reserved0;
    int reserved1;
};

struct flanger_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct flanger_udpate parm;
};

#endif

