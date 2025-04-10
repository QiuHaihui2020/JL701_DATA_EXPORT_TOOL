
#ifndef _CHOURS_API_H__
#define _CHOURS_API_H__


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



typedef  struct  _CHORUS_PARM_ {
    int wetgain;               //0-100
    int drygain;
    int mod_depth;
    int mod_rate;
    int feedback;
    int delay_length;
    af_DataType dataTypeobj;
} CHORUS_PARM;

typedef struct _CHORUS_FUNC_API_ {
    u32(*need_buf)(CHORUS_PARM *vc_parm);
    int (*open)(void *ptr, u32 sr, u32 nch, CHORUS_PARM *vc_parm);       //中途改变参数，可以调init
    void (*run)(void *ptr, short *indata, short *outdata, int PointsPerChannel);    //len是 每声道多少点
    void (*init)(void *ptr, CHORUS_PARM *vc_parm);        //中途改变参数，可以调init
} CHORUS_FUNC_API;


extern  CHORUS_FUNC_API *get_chorus_ops();

#endif // reverb_api_h__

