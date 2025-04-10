#ifndef CVP_NODE_H
#define CVP_NODE_H

#include "audio_config.h"
#include "audio_cvp.h"
#include "effects/effects_adj.h"
#include "adc_file.h"

int cvp_node_param_cfg_read(void *priv, u8 ignore_subid);
int cvp_node_output_handle(s16 *data, u16 len);

int cvp_param_cfg_read(void);
u8 cvp_get_talk_mic_ch(void);
u8 cvp_get_talk_ref_mic_ch(void);
u8 cvp_get_talk_fb_mic_ch(void);

#endif/*CVP_NODE_H*/

