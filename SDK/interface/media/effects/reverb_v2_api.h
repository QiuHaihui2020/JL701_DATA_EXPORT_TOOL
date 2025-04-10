#ifndef reverb_v2_api_h__
#define reverb_v2_api_h__


#include "AudioEffect_DataType.h"
#include "audio_reverb_common.h"


#define EF_REVERB_FIX_PARM_SET EF_REVERB0_FIX_PARM

typedef struct REVERB_V2_PARM_SET {
    int dry;               // 0-200%
    int wet;                //0-300%
    int Erwet;             // 0-100%
    int lateRefbuf_factor;        //0-100    <========================>    重新申请buf
    int delaybuf_max_ms;     //50-250ms     <========================>    重新申请buf
    int roomsize;          //50-250ms
    int pre_delay;          //50-250ms
    int decayfactor;              //200-29000ms
    int Erfactor;         // 50%-100%      , early  roomsize
    int Ewidth;           // -100% - 100%  ,early  stereo
    int Ertolate;          // 0- 300%      , early out rate
    int diffusion;         //0% - 100%
    int highfrequencydamping;        //0-100
    int lowcutoff;           //lateReflect lowcut  ,0-20k
    int highcutoff;          //lateReflect highcut, 0-20k
    int late_type;        //0或1
    int early_taps;        //0-9
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} REVERB_V2_PARM_SET;



typedef struct __REVERB_V2_FUNC_API_ {
    unsigned int (*need_buf)(REVERB_V2_PARM_SET *preverb_parm, EF_REVERB_FIX_PARM_SET *echo_fix_parm);
    int (*open)(unsigned int *ptr, REVERB_V2_PARM_SET *preverb_parm, EF_REVERB_FIX_PARM_SET *echo_fix_parm);
    int (*init)(unsigned int *ptr, REVERB_V2_PARM_SET *preverb_parm);
    int (*run)(unsigned int *ptr, short *inbuf, short *outdata, int len);
} REVERB_V2_FUNC_API;

extern REVERB_V2_FUNC_API *get_reverb_24_16_func_api();
extern REVERB_V2_FUNC_API *get_reverb_func_api();



#endif // reverb_api_h__
