
#ifndef _AUDIO_ANC_FADE_CTR_H_
#define _AUDIO_ANC_FADE_CTR_H_

#include "generic/typedef.h"

//默认淡入增益，dB换算公式 fade_gain = 10^(dB/20) * 16384;
#define AUDIO_ANC_FADE_GAIN_DEFAULT		16384	//(0dB)

//ANC全部通道
#define AUDIO_ANC_FDAE_CH_ALL  AUDIO_ANC_FADE_CH_LFF | AUDIO_ANC_FADE_CH_LFB | AUDIO_ANC_FADE_CH_RFF | AUDIO_ANC_FADE_CH_RFB
#define AUDIO_ANC_FDAE_CH_FF  AUDIO_ANC_FADE_CH_LFF | AUDIO_ANC_FADE_CH_RFF
#define AUDIO_ANC_FDAE_CH_FB  AUDIO_ANC_FADE_CH_LFB | AUDIO_ANC_FADE_CH_RFB

enum anc_fade_mode_t {
    ANC_FADE_MODE_RESET = 0,		//复位
    ANC_FADE_MODE_SWITCH,			//ANC模式切换
    ANC_FADE_MODE_MUSIC_DYNAMIC,	//音乐动态增益
    ANC_FADE_MODE_SCENE_ADAPTIVE,	//ANC场景噪声自适应
    ANC_FADE_MODE_WIND_NOISE,		//ANC风噪检测
    ANC_FADE_MODE_SUSPEND,			//ANC挂起
    ANC_FADE_MODE_HOWLDET,			//啸叫检测
    //可再此继续添加模式
    ANC_FADE_MODE_USER,				//用户模式
};


/*
   ANC淡入淡出增益设置
	param:  mode 		场景模式
			ch  		设置目标通道(支持多通道)
			gain 设置增益
	notes: ch 支持配置多个通道，但mode 必须与 ch配置一一对应;
			当设置gain = 16384, 会自动删除对应模式
    cpu差异：AC700N/JL701N 仅支持AUDIO_ANC_FDAE_CH_ALL;
             JL708N 支持所有通道独立配置;
 */
void audio_anc_fade_ctr_set(enum anc_fade_mode_t mode, u8 ch, u16 gain);

//删除fade mode
void audio_anc_fade_ctr_del(enum anc_fade_mode_t mode);

//fade ctr 初始化
void audio_anc_fade_ctr_init(void);


#endif/*_AUDIO_ANC_FADE_CTR_H_*/

