#ifndef ___VOICE_CHANGER_API__
#define ___VOICE_CHANGER_API__
#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/audio_voice_changer.h"
#include "effects/effects_adj.h"


//参数调节的趋势：shiftv越小，音高越高， formant_shift越小，音色越明亮
typedef enum {
    VOICE_CHANGER_NONE,//原声
    VOICE_CHANGER_UNCLE,//大叔
    VOICE_CHANGER_GODDESS,//女神
    VOICE_CHANGER_BABY,//娃娃音
    VOICE_CHANGER_MAGIC,//魔音女声
    VOICE_CHANGER_MONSTER,//怪兽音
    VOICE_CHANGER_DONALD_DUCK,//唐老鸭
    VOICE_CHANGER_MINIONS,//小黄人
    VOICE_CHANGER_ROBOT,//机器音
    VOICE_CHANGER_WHISPER,//气音
    VOICE_CHANGER_MELODY,//固定旋律音
    VOICE_CHANGER_FEEDBACK,//调制音

    // VOICE_CHANGER_MAX,
} VOICE_CHANGER_MODE;


void audio_voice_changer_mode_switch(u16 uuid, char *name, VOICE_CHANGER_MODE mode);

void audio_esco_ul_voice_change(u32 sound_mode);

#endif
