#ifndef  _LF_PINGPONG_ECHO_API_H_
#define _LF_PINGPONG_ECHO_API_H_

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


typedef struct _EPINGPONG_PARM_SET_ {
    unsigned int nch;
    unsigned int samplerate;
    unsigned int delay;                      //回声的延时时间 0 to max_delay_ms
    float decayval;                          // -0.5 to  -60.0
    unsigned int filt_enable;                //滤波器使能标志
    unsigned int lpf_cutoff;                 //0-20k
    unsigned int wetgain;                     //0-300%
    unsigned int drygain;                     //0-100%
    int predelay;                             //0 to max_delay_ms
    int max_delay_ms;
    af_DataType dataTypeobj;
    int wet_bit_wide;                        //固件层定义的变量，用于控制固件层选用湿声精度的接口
} EPINGPONG_PARM_SET;




/*open 跟 run 都是 成功 返回 RET_OK，错误返回 RET_ERR*/
/*魔音结构体*/
typedef struct __EPINGPONG_FUNC_API_ {
    u32(*need_buf)(void *vc_parm);
    u32(*tmp_buf_size)(void *vc_parm);
    int (*init)(void *ptr, void *vc_parm, void *tmpbuf);       //中途改变参数，可以调init
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);        //中途改变参数，可以调init
    int (*run)(void *ptr, void *indata, void *outdata, int PointsPerChannel);    //len是 每声道多少点
    int (*update)(void *ptr, void *vc_parm);        //中途改变参数，可以调init
} EPINGPONG_FUNC_API;

#ifdef __cplusplus
extern "C"
{
#endif
extern EPINGPONG_FUNC_API *get_epingpong_func_api();
extern EPINGPONG_FUNC_API *get_epingpong24r16_func_api();
#ifdef __cplusplus
}
#endif
#endif
