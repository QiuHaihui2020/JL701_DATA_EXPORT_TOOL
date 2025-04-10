#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bass_treble.data.bss")
#pragma data_seg(".bass_treble.data")
#pragma const_seg(".bass_treble.text.const")
#pragma code_seg(".bass_treble.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "effects/effects_adj.h"
#include "jlstream.h"
#include "user_cfg_id.h"
#include "online_db_deal.h"
#include "bass_treble.h"
#include "scene_switch.h"

#if TCFG_BASS_TREBLE_NODE_ENABLE
struct bass_treble_default_parm  music_bass_treble_par = {0};
struct bass_treble_default_parm  mic_bass_treble_par = {0};

/*
 *获取musicc 高低音增益,节点调用
 * */
int get_music_bass_treble_parm(int arg)
{
    int ret = 0;
    struct bass_treble_default_parm *par = (struct bass_treble_default_parm *)arg;
    if (par->type & BASS_TREBLE_PARM_GET) {
        if (music_bass_treble_par.type & (BASS_TREBLE_PARM_SET | BASS_TREBLE_PARM_SET_GLOBAL_GAIN | BASS_TREBLE_PARM_SET_BYPASS)) {
            memcpy(par, &music_bass_treble_par, sizeof(*par));
            ret = 1;
        }
    }
    par->cfg_index = 0;
    par->mode_index = get_current_scene();

    return ret;
}
__attribute__((weak))
u8 get_mic_current_scene()
{
    return 0;
}
/*
 *获取mic 高低音增益,节点调用
 * */
int get_mic_bass_treble_parm(int arg)
{
    int ret = 0;
    struct bass_treble_default_parm *par = (struct bass_treble_default_parm *)arg;
    if (par->type & BASS_TREBLE_PARM_GET) {
        if (mic_bass_treble_par.type & (BASS_TREBLE_PARM_SET | BASS_TREBLE_PARM_SET_GLOBAL_GAIN | BASS_TREBLE_PARM_SET_BYPASS)) {
            memcpy(par, &mic_bass_treble_par, sizeof(*par));
            ret = 1;
        }
    }
    par->cfg_index = 0;
    par->mode_index = get_mic_current_scene();
    return ret;
}



/*音乐高低音更新接口,gain以外的参数在调音节点界面配置
 *name:节点名称
 *index:0 低音  1 中音  2 高音
 *gain:增益 dB (-48~48),大于0的增益，存在失真风险，谨慎使用
 *注意：gain范围由配置文件限制
 * */
void music_bass_treble_eq_udpate(char *name, enum bass_treble_eff index, float gain)
{
    struct bass_treble_parm par = {0};
    par.type = BASS_TREBLE_PARM_SET;
    par.gain.index = index;
    par.gain.gain = gain;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    music_bass_treble_par.type |= par.type;
    music_bass_treble_par.gain[par.gain.index] = par.gain.gain;//记录增益
}

/*mic高低音更新接口,gain以外的参数在调音节点界面配置
 *name :节点名称
 *index:0 低音  1 中音  2 高音
 *gain:增益 dB (-48~48),大于0的增益，存在失真风险，谨慎使用
 *注意：gain范围由配置文件限制
 * */
void mic_bass_treble_eq_udpate(char *name, enum bass_treble_eff index, float gain)
{
    struct bass_treble_parm par = {0};
    par.type = BASS_TREBLE_PARM_SET;
    par.gain.index = index;
    par.gain.gain = gain;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    mic_bass_treble_par.type |= par.type;
    mic_bass_treble_par.gain[par.gain.index] = par.gain.gain;//记录增益
}

/*
 *音乐高低音eq，总增益更新
 * */
void music_bass_treble_set_global_gain(char *name, float global_gain)
{
    struct bass_treble_parm par = {0};
    par.type = BASS_TREBLE_PARM_SET_GLOBAL_GAIN;
    par.global_gain = global_gain;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    music_bass_treble_par.type |= par.type;
    music_bass_treble_par.global_gain = par.global_gain;
}

/*
 *mic高低音eq，总增益更新
 * */
void mic_bass_treble_set_global_gain(char *name, float global_gain)
{
    struct bass_treble_parm par = {0};
    par.type = BASS_TREBLE_PARM_SET_GLOBAL_GAIN;
    par.global_gain = global_gain;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    mic_bass_treble_par.type |= par.type;
    mic_bass_treble_par.global_gain = par.global_gain;
}
/*
 *music高低音 bypass控制，0：run 1:bypass
 * */
void music_bass_treble_eq_set_bypass(char *name, u32 bypass)
{
    struct bass_treble_parm par = {0};
    par.is_bypass = bypass;
    par.type = BASS_TREBLE_PARM_SET_BYPASS;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    music_bass_treble_par.type |= par.type;
    music_bass_treble_par.is_bypass = par.is_bypass;
}
/*
 *mic高低音 bypass控制，0：run 1:bypass
 * */
void mic_bass_treble_eq_set_bypass(char *name, u32 bypass)
{
    struct bass_treble_parm par = {0};
    par.is_bypass = bypass;
    par.type = BASS_TREBLE_PARM_SET_BYPASS;
    jlstream_set_node_param(NODE_UUID_BASS_TREBLE, name, &par, sizeof(par));

    mic_bass_treble_par.type |= par.type;
    mic_bass_treble_par.is_bypass = par.is_bypass;
}

//兼容旧接口
void mix_out_high_bass(u32 cmd, struct high_bass *hb)
{
    if (cmd == AUDIO_EQ_HIGH) {
        music_bass_treble_eq_udpate("MusicBassTre", BASS_TREBLE_HIGH, hb->gain);
    } else if (cmd == AUDIO_EQ_BASS) {
        music_bass_treble_eq_udpate("MusicBassTre", BASS_TREBLE_LOW, hb->gain);
    }
}

void test_mid_eq(void *p)
{
    static  float gain = 0;
    if (gain++ > 0) {
        gain = -48;
    }
    printf("====gain %d\n", (int)gain);
    music_bass_treble_eq_udpate("MusicBassTre", BASS_TREBLE_MID, gain);

    /* float test_global_gain = -1; */
    /* music_bass_treble_set_global_gain("MusicBassTre", test_global_gain); */
}



#endif
