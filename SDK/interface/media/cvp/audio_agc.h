#ifndef _AUDIO_AGC_H_
#define _AUDIO_AGC_H_

#include "generic/typedef.h"

typedef struct {
    float AGC_max_lvl;          //最大幅度压制，range[0 : -90] dB
    float AGC_fade_in_step;     //淡入步进，range[0.1 : 5] dB
    float AGC_fade_out_step;    //淡出步进，range[0.1 : 5] dB
    float AGC_max_gain;         //放大上限, range[-90 : 40] dB
    float AGC_min_gain;         //放大下限, range[-90 : 40] dB
    float AGC_speech_thr;       //放大阈值, range[-70 : -40] dB
    int AGC_samplerate;         //采样率
    int AGC_frame_size;         //帧长(short)，处理数据长度是帧长的倍数时，没有数据缓存延时
} agc_param_t;

int audio_agc_run(void *hdl, short *in, short *out, int len);
void *audio_agc_open(agc_param_t *param);
int audio_agc_close(void *hdl);


#endif/*_AUDIO_AGC_H_*/
