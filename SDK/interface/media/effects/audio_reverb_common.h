#ifndef _AUDIO_REVERB_COMMON_H_
#define _AUDIO_REVERB_COMMON_H_

#include "AudioEffect_DataType.h"

typedef struct _Plate_reverb_parm_ {
    int wet;                      //0-300%
    int dry;                      //0-200%
    int pre_delay;                 //0-40ms
    int highcutoff;                //0-20k 高频截止
    int diffusion;                  //0-100%
    int decayfactor;                //0-100%
    int highfrequencydamping;       //0-100%
    int modulate;                  // 0或1
    int roomsize;                   //20%-100%
    af_DataType dataTypeobj;
} Plate_reverb_parm;

typedef struct  _EF_REVERB0_FIX_PARM {
    unsigned int sr;
    unsigned int nch;//输入通道数
} EF_REVERB0_FIX_PARM;

#endif
