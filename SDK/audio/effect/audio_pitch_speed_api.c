#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_pitch_speed_api.data.bss")
#pragma data_seg(".audio_pitch_speed_api.data")
#pragma const_seg(".audio_pitch_speed_api.text.const")
#pragma code_seg(".audio_pitch_speed_api.text")
#endif

#include "jlstream.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_pitch_speed_api.h"
#include "effects_default_param.h"
#include "node_uuid.h"

#if TCFG_PITCH_SPEED_NODE_ENABLE

static const float semi_tones_table[] = {-12.0, -10.0, -8.0, -6.0, -4.0, -2.0, 0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0};
static s8 g_pitch_mode = 6; //记录变调模式

u8 get_pitch_mode()
{
    return g_pitch_mode;
}

int audio_pitch_up(char *node_name)
{
    g_pitch_mode ++;
    if (g_pitch_mode > ARRAY_SIZE(semi_tones_table) - 1) {
        g_pitch_mode = ARRAY_SIZE(semi_tones_table) - 1;
    }
    printf("pitch %d\n", (int)semi_tones_table[g_pitch_mode]);
    audio_pitch_speed_set(node_name, semi_tones_table[g_pitch_mode], 1);
    return g_pitch_mode;
}

int audio_pitch_down(char *node_name)
{
    g_pitch_mode --;
    if (g_pitch_mode < 0) {
        g_pitch_mode = 0;
    }
    printf("pitch %d\n", (int)semi_tones_table[g_pitch_mode]);
    audio_pitch_speed_set(node_name, semi_tones_table[g_pitch_mode], 1);
    return g_pitch_mode;
}

int audio_pitch_speed_set(char *node_name, float semi_tones, float speed)
{
    pitch_speed_param_tool_set cfg = {
        .pitch = semi_tones,
        .speed = speed,
    };
    return jlstream_set_node_param(NODE_UUID_PITCH_SPEED, node_name, &cfg, sizeof(cfg));
}
#endif

