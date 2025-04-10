#ifndef __AUDIO_MULTIBAND_LIMITER__
#define __AUDIO_MULTIBAND_LIMITER__

#include "AudioEffect_DataType.h"
#include "audio_limiter.h"
#include "audio_eq.h"
#include "multi_ch_mix.h"
#include "effects/audio_crossover.h"
#include "effects/audio_hw_crossover.h"
#include "audio_drc_common.h"
#include "effects/audio_multiband_crossover.h"


struct multiband_limiter_param_tool_set {//多带 LIMITER界面参数
    struct mdrc_common_param common_param;
    struct limiter_param_tool_set parm[4];//[0]低频 [1]中频  [2]高频  [3]全频
};

struct multiband_limiter_open_param {
    struct multiband_limiter_param_tool_set param;
    u32 sample_rate;
    u8 channel;
    u8 run32bit;                                  //运行位宽32bit使能
    u8 Qval;                                      //数据饱和值15：16bit  23:24bit
    u8 eq_core;                                   //硬件分频器的所使用的eq核
};
struct multiband_limiter_hdl {
    struct audio_limiter *limiter[4];              //DRC句柄
    struct multiband_limiter_param_tool_set param;
    struct multiband_crossover mcross;
    u8 mlimiter_init;
};


struct multiband_limiter_hdl *audio_multiband_limiter_open(struct multiband_limiter_open_param *param);
int audio_multiband_limiter_close(struct multiband_limiter_hdl *hdl);
int audio_multiband_limiter_run(struct multiband_limiter_hdl *hdl, s16 *indata, s16 *outdata, u32 len);
void audio_multiband_limiter_update_parm(struct multiband_limiter_hdl *hdl, struct multiband_limiter_param_tool_set *update_param);

#endif
