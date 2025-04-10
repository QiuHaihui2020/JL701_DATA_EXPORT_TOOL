#ifndef _AUDIO_PHASER_API_H_
#define _AUDIO_PHASER_API_H_
#include "system/includes.h"
#include "effects/phaser_stereo_api.h"

struct phaser_udpate {
    int feedback;             //0-100
    float mod_rate;             //0-100hz
    float delay;
    int mod_depth;               // 1-100
    int LRphasediff;           //0-360
    int wetgain;
    int drygain;
    int nstages;               //0-6
    int reserved0;
    int reserved1;
};

struct phaser_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct phaser_udpate parm;
};


#endif

