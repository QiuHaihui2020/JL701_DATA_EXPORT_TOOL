#ifndef  _LF_FlangerStereo_API_H_
#define  _LF_FlangerStereo_API_H_


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


typedef  struct   _flangerStereo_parm_ {
    unsigned int nch;
    unsigned int samplerate;
    int feedback;             //0-100
    float delay;
    float mod_rate;
    int mod_depth;               // 0-100
    int wetgain;
    int drygain;
    int LRphasediff;           //0-360
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} flangerStereo_parm;


typedef struct _FLAGNER_STEREO_FUNC_API_ {
    u32(*need_buf)(void *vc_parm);
    u32(*tmp_buf_size)(void *vc_parm);
    int (*init)(void *ptr, void *vc_parm, void *tmpbuf);       //中途改变参数，可以调init
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);        //中途改变参数，可以调init
    int (*run)(void *ptr, void *indata, void *outdata, int PointsPerChannel);    //len是 每声道多少点
    int (*update)(void *ptr, void *vc_parm);        //中途改变参数，可以调init
} FLAGNER_STEREO_FUNC_API;

#ifdef __cplusplus
extern "C"
{
extern FLAGNER_STEREO_FUNC_API *get_flanger_stereo_ops();
}
#else
extern FLAGNER_STEREO_FUNC_API *get_flanger_stereo_ops();
#endif

#endif
