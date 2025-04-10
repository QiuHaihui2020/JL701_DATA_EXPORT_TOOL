


#ifndef _audio_reverb_v2_h__
#define _audio_reverb_v2_h__

#include "effects/reverb_v2_api.h"

struct reverb_update_parm { /*与REVERB_V2_PARM_SET结构关联*/
    int dry;               // 0-200%
    int wet;                //0-300%
    int Erwet;             // 0-100%

    int Erfactor;         // 50%-100%      , early  roomsize
    int Ewidth;           // -100% - 100%  ,early  stereo
    int early_taps;        //0-9

    int roomsize;          //50-250ms
    int pre_delay;          //50-250ms
    int decayfactor;              //200-29000ms
    int Ertolate;          // 0- 300%      , early out rate
    int diffusion;         //0% - 100%
    int highfrequencydamping;        //0-100
    int lowcutoff;           //lateReflect lowcut  ,0-20k
    int highcutoff;          //lateReflect highcut, 0-20k
    int late_type;        //0或1
    int reserved0;
    int reserved1;

    int lateRefbuf_factor;        //0-100    <========================>    重新申请buf
    int delaybuf_max_ms;     //50-250ms     <========================>    重新申请buf

    int wet_bit_wide;

};

typedef struct  _reverb_parm_tool_set {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct reverb_update_parm parm;
} reverb_param_tool_set;

struct audio_reverb_parm {
    REVERB_V2_PARM_SET parm;
    EF_REVERB_FIX_PARM_SET fixparm;
    u16 wet_bit_wide;
    u8 bypass;
};


struct audio_reverb {
    void *workbuf;
    REVERB_V2_FUNC_API *ops;           //函数指针
    struct audio_reverb_parm parm;
    u8 status;
    u8 update;
};



/*
 * 打开混响模块
 */
struct audio_reverb *audio_reverb_open(struct audio_reverb_parm *parm);
/*
 * 混响处理
 */
int audio_reverb_run(struct audio_reverb *hdl, short *in, short *out, int len);
/*
 * 关闭混响模块
 */
void  audio_reverb_close(struct audio_reverb *hdl);

/*
 *  混响参数更新
 */
void audio_reverb_update_parm(struct audio_reverb *hdl, reverb_param_tool_set *cfg);


void audio_reverb_printf(reverb_param_tool_set *cfg);
#endif // reverb_echo_h__
