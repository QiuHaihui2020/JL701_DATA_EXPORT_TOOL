#ifndef __AUDIO_MULTIBAND_CROSSOVER__
#define __AUDIO_MULTIBAND_CROSSOVER__

#include "AudioEffect_DataType.h"
#include "audio_limiter.h"
#include "audio_eq.h"
#include "multi_ch_mix.h"
#include "effects/audio_crossover.h"
#include "effects/audio_hw_crossover.h"
#include "audio_drc_common.h"



struct crossover_eq {
    s16 *tmp_data;                              //分频后pcm数据的缓存buf
    struct audio_eq *eq[4];//2band:0:pre_eq, 1:ext_eq  3band:
};

struct multiband_crossover {
    struct crossover_eq cross[3];                  //分频器
    float *mlimiter_coeff[4];                      //分频器系数 对应3带滤波器每条通路的4个滤波器,2band 通路对应[0] [2]
    void *priv;
    u32(*run)(void *priv, void *in, void *out, u32 len); // 算法run
    void *limiter[4];
    u32 tmp_len;
    u32 sample_rate;
    u8 channel;
    u8 run32bit;                                  //运行位宽32bit使能
    u8 eq_core;                                   //硬件分频器的所使用的eq核
    u8 mlimiter_section;                          //阶数转换后对应的eq段数
    u8 Qval;                                      //数据饱和值15：16bit  23:24bit
};


void audio_multiband_crossover_init(struct multiband_crossover *hdl, struct mdrc_common_param *common_param);
int audio_multiband_crossover_open(struct multiband_crossover *hdl, struct mdrc_common_param *common_param);
void audio_multiband_crossover_update(struct multiband_crossover *hdl, struct mdrc_common_param *common_param);
int audio_multiband_crossover_run(struct multiband_crossover *hdl, struct mdrc_common_param *common_param, s16 *indata, s16 *outdata, u32 len, u8 bypass[4]);
void audio_multiband_crossover_close(struct multiband_crossover *hdl);

#endif
