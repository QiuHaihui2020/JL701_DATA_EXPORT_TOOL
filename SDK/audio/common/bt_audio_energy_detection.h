#ifndef _BT_AUDIO_ENERGY_DETECTION_H_
#define _BT_AUDIO_ENERGY_DETECTION_H_

#include "btstack/a2dp_media_codec.h"


int bt_audio_energy_detect_run(u8 codec_type, void *packet, u16 frame_len);

void bt_audio_energy_detect_close(u8 codec_type);



#endif//_BT_AUDIO_ENERGY_DETECTION_H_
