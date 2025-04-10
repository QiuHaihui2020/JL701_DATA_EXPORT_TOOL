#ifndef _ADC_FILE_H_
#define _ADC_FILE_H_

#include "generic/typedef.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"


//stream.bin ADC参数文件解析
struct adc_file_cfg {
    u32 mic_en_map; // BIT(ch) 1 为 ch 使能，0 为不使能
    struct adc_file_param param[AUDIO_ADC_MAX_NUM];
} __attribute__((packed));

void audio_adc_file_init(void);
void audio_all_adc_file_init(void);
void audio_adc_file_set_gain(u8 mic_index, u8 mic_gain);
u8 audio_adc_file_get_gain(u8 mic_index);
u8 audio_adc_file_get_mic_mode(u8 mic_index);
int adc_file_mic_open(struct adc_mic_ch *mic, int ch);
struct adc_file_cfg *audio_adc_file_get_cfg(void);
struct adc_platform_cfg *audio_adc_platform_get_cfg(void);
u8 audio_adc_file_get_esco_mic_num(void);
void audio_adc_fixed_digital_set_buffs(void);

u8 audio_adc_file_get_mic_en_map(void);

void audio_adc_file_set_mic_en_map(u8 mic_en_map);
extern u32 pc_mic_get_fmt_sample_rate(void);
/*根据mic通道值获取使用的第几个mic*/
u8 audio_get_mic_index(u8 mic_ch);
/*根据mic通道值获取使用了多少个mic*/
u8 audio_get_mic_num(u32 mic_ch);

u8 audio_max_adc_ch_num_get();

#endif // #ifndef _ADC_FILE_H_
