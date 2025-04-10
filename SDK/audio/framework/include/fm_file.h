#ifndef _FM_FILE_H_
#define _FM_FILE_H_

#include "generic/typedef.h"
#include "media/includes.h"
#include "app_config.h"

// struct fm_inside_file_cfg {
// u8 inside_fm_en;
// u8 inside_fm_agc_en;	//FM AGC使能
// u8 inside_fm_agc_val;	//FM AGC 初值
// u8 inside_fm_strreo_en;	//立体声使能
// } __attribute__((packed));

#define FM_ADC_BUF_NUM        2		//linein_adc采样buf个数
#define FM_ADC_IRQ_POINTS     256	//linein adc 中断点数
#define FM_ADC_SAMPLE_RATE    44100	//linein adc 采样率

struct fm_outside_file_cfg {
    u32 linein_en_map;
    u16 clk_io_uuid;	//使用uuid2gpio函数将该值转为gpio值
} __attribute__((packed));

extern void audio_fm_file_init(void);
extern int adc_file_fm_open(struct adc_linein_ch *linein, int ch);
extern struct fm_file_cfg *audio_fm_file_get_cfg(void);

int fm_get_device_en(char *logo);

#endif // #ifndef _FM_FILE_H_
