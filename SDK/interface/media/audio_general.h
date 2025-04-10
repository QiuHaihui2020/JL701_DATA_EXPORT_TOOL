#ifndef _AUDIO_GENERAL_H
#define _AUDIO_GENERAL_H

#include "generic/typedef.h"
struct audio_general_params {
    int sample_rate;
    u8 system_bit_width;
    u8 media_bit_width;
    u8 esco_bit_width;
    u8 mic_bit_width;
    u8 usb_audio_bit_width;
};

struct audio_general_params *audio_general_get_param(void);
int audio_general_out_dev_bit_width();
int audio_general_in_dev_bit_width();
int audio_general_init();
int audio_general_set_global_sample_rate(int sample_rate);

#endif
