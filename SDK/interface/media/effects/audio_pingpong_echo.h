#ifndef _AUDIO_pingpong_echo_API_H_
#define _AUDIO_pingpong_echo_API_H_
#include "system/includes.h"
#include "effects/lf_pingpong_echo_api.h"


struct pingpong_echo_udpate {
    unsigned int delay;                      //回声的延时时间 0 to max_delay_ms
    float decayval;                          // -0.5 to  -60.0
    unsigned int filt_enable;                //滤波器使能标志
    unsigned int lpf_cutoff;                 //0-20k
    unsigned int wetgain;                     //0-300%
    unsigned int drygain;                     //0-100%
    int predelay;                             //0 to max_delay_ms
    int max_delay_ms;
    int wet_bit_wide;
};

struct pingpong_echo_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct pingpong_echo_udpate parm;
};

#endif

