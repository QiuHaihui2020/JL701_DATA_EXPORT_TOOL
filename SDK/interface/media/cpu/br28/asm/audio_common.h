#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

#include "generic/typedef.h"
#include "system/includes.h"

void audio_delay(int time_ms);
int audio_adc_analog_status_add_check(u8 ch_index, int add);
int audio_adc_digital_status_add_check(int add);

#endif // _AUDIO_COMMON_H_

