#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_voice_changer_api.data.bss")
#pragma data_seg(".audio_voice_changer_api.data")
#pragma const_seg(".audio_voice_changer_api.text.const")
#pragma code_seg(".audio_voice_changer_api.text")
#endif

#include "jlstream.h"
#include "app_config.h"
#include "audio_voice_changer_api.h"

#if TCFG_VOICE_CHANGER_NODE_ENABLE

static const VOICECHANGER_PARM vparm[] = {
    {0, 0, 0},
    {EFFECT_VOICECHANGE_PITCHSHIFT, 130, 100},
    {EFFECT_VOICECHANGE_SPECTRUM,    56,  90},
    {EFFECT_VOICECHANGE_PITCHSHIFT,  50, 100},
    {EFFECT_VOICECHANGE_PITCHSHIFT,  75,  80},
    {EFFECT_VOICECHANGE_PITCHSHIFT, 160, 100},
    {EFFECT_VOICECHANGE_CARTOON,     60, 170},
    {EFFECT_VOICECHANGE_CARTOON,     50,  60},
    {EFFECT_VOICECHANGE_ROBORT,      70,  80},
    {EFFECT_VOICECHANGE_WHISPER,     70,  80},
    {EFFECT_VOICECHANGE_MELODY,      70,  80},
    {EFFECT_VOICECHANGE_FEEDBACK,   150,  80}
};

void audio_voice_changer_mode_switch(u16 uuid, char *name, VOICE_CHANGER_MODE mode)
{
    voice_changer_param_tool_set cfg = {0};
    cfg.is_bypass = 0;
    if (mode > ARRAY_SIZE(vparm)) {
        return;
    }
    memcpy(&cfg.parm, &vparm[mode], sizeof(VOICECHANGER_PARM));
    if (mode == VOICE_CHANGER_NONE) {
        cfg.is_bypass = 1;
    }
    jlstream_set_node_param(uuid, name, &cfg, sizeof(cfg));
}
#endif
