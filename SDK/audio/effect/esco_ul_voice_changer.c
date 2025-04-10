#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_ul_voice_changer.data.bss")
#pragma data_seg(".esco_ul_voice_changer.data")
#pragma const_seg(".esco_ul_voice_changer.text.const")
#pragma code_seg(".esco_ul_voice_changer.text")
#endif

#include "audio_voice_changer_api.h"
#include "esco_recoder.h"
#include "app_config.h"

#if TCFG_VOICE_CHANGER_NODE_ENABLE

void audio_esco_ul_voice_change(u32 sound_mode)
{
    audio_voice_changer_mode_switch(NODE_UUID_VOICE_CHANGER, "esco_vchanger", sound_mode);
}
#endif
