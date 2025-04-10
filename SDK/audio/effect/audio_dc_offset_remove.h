#ifndef _AUDIO_DC_OFFSET_REMOVE_H
#define _AUDIO_DC_OFFSET_REMOVE_H

#include "effects/eq_config.h"

void *audio_dc_offset_remove_open(u32 samprate, u32 ch_num);

void audio_dc_offset_remove_run(void *hdl, void *data, u32 len);

void audio_dc_offset_remove_close(void *hdl);

#endif
