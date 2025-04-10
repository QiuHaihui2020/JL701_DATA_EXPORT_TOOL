


#ifndef _audio_echo_h__
#define _audio_echo_h__

#include "effects/AudioEffect_DataType.h"

typedef struct _EF_ECHO__PARM_ {
    unsigned int delay;                      //回声的延时时间 0-300ms
    unsigned int decayval;                   // 0-70%
    unsigned int filt_enable;                //滤波器使能标志
    unsigned int lpf_cutoff;                 //0-20k
    unsigned int wetgain;                     //0-200%
    unsigned int drygain;                     //0-100%
    af_DataType dataTypeobj;
} ECHO_PARM_SET;

struct echo_update_parm {
    unsigned int delay;                      //回声的延时时间 0-300ms
    unsigned int decayval;                   // 0-70%
    unsigned int filt_enable;                //滤波器使能标志
    unsigned int lpf_cutoff;                 //0-20k
    unsigned int wetgain;                     //0-200%
    unsigned int drygain;                     //0-100%
} ;


typedef struct  _EF_REVERB_FIX_PARM {
    unsigned int sr;
    unsigned int max_ms;
} EF_REVERB_FIX_PARM;



/*open 跟 run 都是 成功 返回 RET_OK，错误返回 RET_ERR*/
/*魔音结构体*/
typedef struct __ECHO_FUNC_API_ {
    unsigned int (*need_buf)(unsigned int *ptr, ECHO_PARM_SET *echo_parm, EF_REVERB_FIX_PARM *echo_fix_parm);
    int (*open)(unsigned int *ptr, ECHO_PARM_SET *echo_parm, EF_REVERB_FIX_PARM *echo_fix_parm);
    int (*init)(unsigned int *ptr, ECHO_PARM_SET *echo_parm);
    int (*run)(unsigned int *ptr, short *inbuf, short *outdata, int len);
    void (*reset_wetdry)(unsigned int *ptr, int wetgain, int drygain);  //新增
} ECHO_FUNC_API;


typedef struct __EF_ECHO_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct echo_update_parm parm;
    u32 wet_bit_wide;
} echo_param_tool_set;


struct audio_echo_parm {
    ECHO_PARM_SET echo_parm;
    EF_REVERB_FIX_PARM fixparm;
    u16 in_channel;
    u16 wet_bit_wide;
    u8 bypass;
};


struct audio_echo {
    void *workbuf;
    ECHO_FUNC_API *ops;           //函数指针
    struct audio_echo_parm parm;
    u8 status;
    u8 update;
};
extern ECHO_FUNC_API *get_echo24r16_func_api();
extern ECHO_FUNC_API *get_echo_func_api();
extern ECHO_FUNC_API *get_echo_func_api_mask();
/*
 * 打开echo声卡混响模块
 */
struct audio_echo *audio_echo_open(struct audio_echo_parm *parm);
/*
 * echo声卡混响处理
 */
int audio_echo_run(struct audio_echo *hdl, short *in, short *out, int len);
/*
 * 关闭echo声卡混响模块
 */
void  audio_echo_close(struct audio_echo *hdl);

/*
 *  plate声卡混响参数更新
 */
void audio_echo_update_parm(struct audio_echo *hdl, struct echo_update_parm *parm);

void audio_echo_bypass(struct audio_echo *hdl, u8 bypass);


#endif // reverb_echo_h__
