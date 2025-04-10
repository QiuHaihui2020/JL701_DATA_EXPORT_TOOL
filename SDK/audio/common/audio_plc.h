#ifndef _AUDIO_PLC_H_
#define _AUDIO_PLC_H_

#include "generic/typedef.h"
#include "effects/AudioEffect_DataType.h"

void *audio_plc_open(u16 sr, u16 ch_num, af_DataType *dataTypeobj);
void audio_plc_run(void *_plc, s16 *data, u16 len, u8 repair);
int audio_plc_close(void *_plc);

#endif/*_AUDIO_PLC_H_*/
