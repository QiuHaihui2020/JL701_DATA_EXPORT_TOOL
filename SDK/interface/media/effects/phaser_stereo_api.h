
#ifndef _PHASER_STEREO_API_H__
#define _PHASER_STEREO_API_H__


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



typedef  struct  _PHASER_STEREO_PARM_ {
    unsigned int nch;
    unsigned int samplerate;
    int feedback;             //0-100
    float mod_rate;             //0-100hz
    float delay;
    int mod_depth;               // 1-100
    int LRphasediff;           //0-360
    int wetgain;
    int drygain;
    int nstages;               //0-6

    int tone;                  //200-12000
    int spread;                //0-100
    int shaperness;            //0 到 200
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} PHASER_STEREO_PARM;

typedef struct _PHASER_STE_FUNC_API_ {
    u32(*need_buf)(void *vc_parm);
    u32(*tmp_buf_size)(void *vc_parm);
    int (*init)(void *ptr, void *vc_parm, void *tmpbuf);       //中途改变参数，可以调init
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);        //中途改变参数，可以调init
    int (*run)(void *ptr, void *indata, void *outdata, int PointsPerChannel);    //len是 每声道多少点
    int (*update)(void *ptr, void *vc_parm);        //中途改变参数，可以调init
} PHASER_STE_FUNC_API;


#ifdef __cplusplus
extern "C"
{
extern PHASER_STE_FUNC_API *get_phaser_stereo_ops();
}
#else
extern PHASER_STE_FUNC_API *get_phaser_stereo_ops();
#endif

#endif // reverb_api_h__

