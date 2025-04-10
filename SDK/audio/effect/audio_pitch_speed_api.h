#ifndef __AUDIO_PITCH_SPEED_ADJ_API__
#define __AUDIO_PITCH_SPEED_ADJ_API__
#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/audio_pitchspeed.h"
#include "node_uuid.h"

/* 获取当前的变调模式 */
u8 get_pitch_mode();

/* 升高音调 */
int audio_pitch_up(char *node_name);

/* 降低音调 */
int audio_pitch_down(char *node_name);

/*
 * 变速变调设置接口
 * node_name：工具中定义的节点名称
 * semi_tones：半音符，范围 -12~12
 * speed：变速倍数，范围 >0.5
 * */
int audio_pitch_speed_set(char *node_name, float semi_tones, float speed);
#endif
