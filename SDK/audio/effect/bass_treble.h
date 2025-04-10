#ifndef _BASS_TREBLE_H_
#define _BASS_TREBLE_H_


#include "system/includes.h"
#include "media/includes.h"

#include "effects/audio_bass_treble_eq.h"

/*
 *获取musicc 高低音增益,节点调用
 * */
int get_music_bass_treble_parm(int arg);
/*
 *获取mic 高低音增益,节点调用
 * */
int get_mic_bass_treble_parm(int arg);


/*音乐高低音更新接口,gain以外的参数在调音节点界面配置
 *name:节点名称
 *index:0 低音  1 中音  2 高音
 *gain:增益 dB (-48~48),大于0的增益，存在失真风险，谨慎使用
 * 注意：gain范围由配置文件限制
 * */
void music_bass_treble_eq_udpate(char *name, enum bass_treble_eff index, float gain);

/*mic高低音更新接口,gain以外的参数在调音节点界面配置
 *name:节点名称
 *index:0 低音  1 中音  2 高音
 *gain:增益 dB (-48~48),大于0的增益，存在失真风险，谨慎使用
 * 注意：gain范围由配置文件限制
 * */
void mic_bass_treble_eq_udpate(char *name, enum bass_treble_eff index, float gain);


/*
 *音乐高低音eq，总增益更新
 * */
void music_bass_treble_set_global_gain(char *name, float global_gain);

/*
 *mic高低音eq，总增益更新
 * */
void mic_bass_treble_set_global_gain(char *name, float global_gain);

/*
 *music高低音 bypass控制，0：run 1:bypass
 * */
void music_bass_treble_eq_set_bypass(char *name, u32 bypass);

/*
 *mic高低音 bypass控制，0：run 1:bypass
 * */
void mic_bass_treble_eq_set_bypass(char *name, u32 bypass);

#endif
