#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pitch_speed_demo.data.bss")
#pragma data_seg(".pitch_speed_demo.data")
#pragma const_seg(".pitch_speed_demo.text.const")
#pragma code_seg(".pitch_speed_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/audio_pitchspeed.h"
#include "node_uuid.h"


void user_pitch_speed_update_parm_demo()
{
    struct pitch_speed_update_parm cfg = {0};
    char *node_name = "pict_name";//节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    cfg.pitchV = 50000; 	 //32767 是原始音调  >32768是音调变高，《32768 音调变低，建议范围20000 - 50000
    cfg.speedV = 80;         //>80变快,<80 变慢，建议范围36-130
    jlstream_set_node_param(NODE_UUID_PITCH_SPEED, node_name, &cfg, sizeof(cfg));
}
