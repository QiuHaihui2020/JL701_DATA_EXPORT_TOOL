
#ifndef _EXCITOR_H__
#define _EXCITOR_H__


#include "AudioEffect_DataType.h"

#ifndef u8
#define u8  unsigned char
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef s16
#define s16 short
#endif


#ifndef u32
#define u32 unsigned int
#endif


#ifndef s32
#define s32 int
#endif

#ifndef s16
#define s16 signed short
#endif



typedef  struct  _EXCITER_PARM_ {
    unsigned int wet_highpass_freq;
    unsigned int wet_lowpass_freq;
    int wetgain;               //0-100
    int drygain;
    int excitType;
    af_DataType dataTypeobj;
} EXCITER_PARM;

typedef struct _EXCITER_FUNC_API_ {
    u32(*need_buf)(EXCITER_PARM *vc_parm);
    int (*open)(void *ptr, u32 sr, u32 nch, EXCITER_PARM *vc_parm);       //中途改变参数，可以调init
    void (*run)(void *ptr, short *indata, short *outdata, int PointsPerChannel);    //len是 每声道多少点
    void (*init)(void *ptr, EXCITER_PARM *vc_parm);        //中途改变参数，可以调init
} EXCITE_FUNC_API;


extern  EXCITE_FUNC_API *get_excitor_ops();

#endif // reverb_api_h__

