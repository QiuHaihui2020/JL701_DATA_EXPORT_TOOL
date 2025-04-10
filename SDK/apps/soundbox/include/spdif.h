#ifndef _SPDIF_APP_H_
#define _SPDIF_APP_H_

#include "system/event.h"

struct app_mode *app_enter_spdif_mode(int arg);
int spdif_app_msg_handler(int *msg);
struct le_audio_stream_params *spdif_get_le_audio_params(void);
void *spdif_get_le_audio_hdl(void);

#endif


