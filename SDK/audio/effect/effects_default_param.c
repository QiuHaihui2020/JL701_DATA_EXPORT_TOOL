#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".effects_default_param.data.bss")
#pragma data_seg(".effects_default_param.data")
#pragma const_seg(".effects_default_param.text.const")
#pragma code_seg(".effects_default_param.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "jlstream.h"
#include "cfg_tool.h"
#include "effects/effects_adj.h"
#include "effects/audio_eq.h"
#include "effects/eq_config.h"
#include "effects/audio_wdrc.h"
#include "effects/audio_pitchspeed.h"
#include "effects/audio_spk_eq.h"
#include "media/audio_energy_detect.h"
#include "bass_treble.h"
#include "scene_switch.h"
#include "effects_default_param.h"
#include "ascii.h"
#include "effects/spectrum/spectrum_eq.h"
#include "effects/spectrum/spectrum_fft.h"

#if ((defined TCFG_AUDIO_SPATIAL_EFFECT_ENABLE) && TCFG_AUDIO_SPATIAL_EFFECT_ENABLE)
#include "spatial_effects_process.h"
#endif

struct effect_default_hdl {
    u16 pitchV;
};

struct effect_default_hdl effect_default = {
    .pitchV = 32768,
};

static u32 effect_strcmp(const char *str1, const char *str2);

u8 __attribute__((weak)) get_current_scene()
{
    return 0;
}
u8 __attribute__((weak)) get_mic_current_scene()
{
    return 0;
}
u8 __attribute__((weak)) get_music_eq_preset_index(void)
{
    return 0;
}

float powf(float x, float y);
static int get_pitchV(float pitch)
{
    int pitchV = 32768 * powf(2.0f, pitch / 12);
    pitchV = (pitchV >= 65536) ? 65534 : pitchV; //传入算法是u16类型，作特殊处理防止溢出
    return pitchV;
}

u16 audio_pitch_default_parm_set(u8 pitch_mode)
{
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    effect_default.pitchV = get_pitchV(pitch_param_table[pitch_mode]);
    return effect_default.pitchV;
}

static void audio_energy_det_handler(void *prive, u8 event, u8 ch, u8 ch_total)
{
    char name[16];
    memcpy(name, prive, strlen(prive));
    //name 模块名称，唯一标识
    // ch < ch_total 时：表示通道(ch)触发的事件
    // ch = ch_total 时：表示全部通道都触发的事件

    if (ch < ch_total) {
        /*
         *针对独立通道，比如立体声场景，这里就分别显示ch0和ch1的能量
         *适用于独立控制声道mute的场景
         */
        printf(">>>>name:%s ch:%d %s\n", name, ch, event ? ("MUTE") : ("UNMUTE"));
    }

    if (ch == ch_total) {
        /*
         *针对所有通道，比如立体声场景，只有ch0和ch1都是mute，all_ch才是mute
         *适用于控制整体
         */
        printf(">>>>name:%s ch_total %s\n", name,  event ? ("MUTE") : ("UNMUTE"));
    }
}
//注册频谱能量计算的回调函数，有注册则可以通过该回调返回频谱值，
//没注册的情况下,也可直接通过jlstream_get_node_param接口获取,例子见user_spectrum_advance_get_param()
//每次输出一帧频点的频谱值
static void audio_spectrum_advance_handler(void *prive, void *param)
{
#if 0 //帧率统计
    static u32 test_cnt = 0;
    static u32 next_frame_period = 0;
    if (!next_frame_period) {
        next_frame_period = jiffies_usec() + 1000000;
    }
    test_cnt ++;
    if (!time_before(jiffies_usec(), next_frame_period)) {
        printf("test_cnt %d\n", test_cnt);
        test_cnt = 0;
        next_frame_period = jiffies_usec() + 1000000;
    }
#endif

#if 0
    char name[16];
    memcpy(name, prive, strlen(prive));
    //name 模块名称，唯一标识
    printf("%s\n",  name);//节点名
    //可在此获取当前的频谱值
    //频点的个数,频谱值的摆放见结构体struct spectrum_advance_parm
    struct spectrum_advance_parm *par = (struct spectrum_advance_parm *)param;
    for (int i = 0; i < par->section; i++) {
        printf("left:dB[%d] %d, right:dB[%d] %d\n", i, (int)par->db_data[0][i], i, (int)par->db_data[1][i]);//dB值是浮点
    }
#endif
}


/*
 *获取需要指定得默认配置
 * */
int get_eff_default_param(int arg)
{
    int ret = 0;

    /* struct node_id *nodeid = (struct node_id *)arg; */
    struct _name {
        char name[16];
    };
    //cppcheck-suppress unreadVariable
    struct _name *name = (struct _name *)arg;
#if TCFG_VIRTUAL_BASS_PRO_MODULE_NODE_ENABLE
//virtual bass pro 模块节点默认参数配置, virtual bass pro节点名需配置成：VBassPro,此处默认配置才会生效
    char virtual_bass_pro[16];
#if TCFG_EQ_ENABLE
    char *sw_eqname_tab[] = {"HPEQ", "PEAKEQ"};//子节点名
    for (int i = 0; i < ARRAY_SIZE(sw_eqname_tab); i++) {
        jlstream_module_node_get_name(sw_eqname_tab[i], "VBassPro", virtual_bass_pro);
        if (!strcmp(name->name, virtual_bass_pro)) {
            struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
            get_eq_parm->cfg_index = 0;
            get_eq_parm->mode_index = get_current_scene();
            ret = 1;
            break;
        }
    }
#endif

    char *virtual_bass_classic_name[] = {"MFreqGen", "HPDRC"};//子节点名
    for (int i = 0; i < ARRAY_SIZE(virtual_bass_classic_name); i++) {
        jlstream_module_node_get_name(virtual_bass_classic_name[i], "VBassPro", virtual_bass_pro);
        if (!strcmp(name->name, virtual_bass_pro)) {
            struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
            get_parm->mode_index = get_current_scene();
            get_parm->cfg_index = 0;//目标配置项
            ret = 1;
            break;
        }
    }
#endif


#if TCFG_3D_PLUS_MODULE_NODE_ENABLE
//3D plus 模块节点默认参数配置, 3D Plus节点名需配置成：3dPlus,此处默认配置才会生效
    char threeD_Plus[16];
#if TCFG_EQ_ENABLE
    char *eqname_tab[] = {"LowEQ", "MidEQ"};//子节点名
    for (int i = 0; i < ARRAY_SIZE(eqname_tab); i++) {
        jlstream_module_node_get_name(eqname_tab[i], "3dPlus", threeD_Plus);
        if (!strcmp(name->name, threeD_Plus)) {
            struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
            get_eq_parm->cfg_index = 0;
            get_eq_parm->mode_index = get_current_scene();
            ret = 1;
            break;
        }
    }
#endif

    char *_3dPlus_name[] = {"LRGain", "MidSMix", "LowSMix", "MixerGain"};//子节点名
    for (int i = 0; i < ARRAY_SIZE(_3dPlus_name); i++) {
        jlstream_module_node_get_name(_3dPlus_name[i], "3dPlus", threeD_Plus);
        if (!strcmp(name->name, threeD_Plus)) {
            struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
            get_parm->mode_index = get_current_scene();
            get_parm->cfg_index = 0;//目标配置项
            ret = 1;
            break;
        }
    }
#endif

#if TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
// virtual surround pro/2to4/2to5 模块节点默认参数配置, virtual surround pro/2to4/2to5节点名需配置成：VSPro,此处默认配置才会生效
    char out[16];
#if TCFG_EQ_ENABLE
    char *vsp_eqname_tab[] = {"CEq", "LRSEq"}; //子节点名
    for (int i = 0; i < ARRAY_SIZE(vsp_eqname_tab); i++) {
        jlstream_module_node_get_name(vsp_eqname_tab[i], "VSPro", out);
        if (!strcmp(name->name, out)) {
            struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
            get_eq_parm->cfg_index = 0;
            get_eq_parm->mode_index = get_current_scene();
            ret = 1;
            break;
        }
    }
#endif

    char *vspro_name[] = {"PreLimiter", "LRLimiter", "CLimiter", "LRSLimiter",
                          "CDrcAdv", "LRSDrcAdv", "LRCross", "LRBand", "LR3Band", "LSCBand", "RSCBand",
                          "LRPcmDly", "LRSNsGate", "UpMix2to5", "RLSCBand", "RRSCBand",
                          "SPWider"
                         };//子节点名

    for (int i = 0; i < ARRAY_SIZE(vspro_name); i++) {
        jlstream_module_node_get_name(vspro_name[i], "VSPro", out);
        if (!strcmp(name->name, out)) {
            struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
            get_parm->mode_index = get_current_scene();
            get_parm->cfg_index = 0;//目标配置项
            ret = 1;
            break;
        }
    }

    char *vspro_media_name[] = {"MBLimiter0Media", "VBassMedia"};
    for (int i = 0; i < ARRAY_SIZE(vspro_media_name); i++) {
        if (!strcmp(name->name, vspro_media_name[i])) {
            struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
            get_parm->mode_index = get_current_scene();
            get_parm->cfg_index = 0;//目标配置项
            ret = 1;
            break;
        }
    }

#endif

#if TCFG_SPEAKER_EQ_NODE_ENABLE
    if (!strcmp(name->name, "spk_eq")) {
        ret = spk_eq_read_from_vm((void *)arg);
    }
#endif

#if TCFG_PITCH_SPEED_NODE_ENABLE
    if (!strncmp(name->name, "PitchSpeed", strlen("PitchSpeed"))) { //音乐变速变调 默认参数获取
        struct pitch_speed_update_parm *get_parm = (struct pitch_speed_update_parm *)arg;
        get_parm->speedV = 80;
        get_parm->pitchV = effect_default.pitchV;
        ret = 1;
    }
#endif

#if TCFG_EQ_ENABLE
    if (!strncmp(name->name, "MusicEq", strlen("MusicEq"))) { //音乐eq命名默认： MusicEq + 类型，例如蓝牙音乐eq：MusicEqBt
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        /*
         *默认系数使用eq文件内的哪个配置表
         * */
        char tar_cfg_index = 0;
        get_cur_eq_num(&tar_cfg_index);//获取当前配置项序号
        get_eq_parm->cfg_index = tar_cfg_index;

        /*
         *是否使用sdk内的默认系数表
         * */
        get_eq_parm->default_tab.seg_num = get_cur_eq_tab(&get_eq_parm->default_tab.global_gain, &get_eq_parm->default_tab.seg);
        ret = 1;
    }
#endif

#if TCFG_ENERGY_DETECT_NODE_ENABLE
    if (!strncmp(name->name, "EnergyDet", strlen("EnergyDet"))) {//能量检查 回调接口配置
        struct energy_detect_get_parm *get_parm = (struct energy_detect_get_parm *)arg;
        if (get_parm->type == SET_ENERGY_DET_EVENT_HANDLER) {
            get_parm->event_handler = audio_energy_det_handler;
            ret = 1;
        }
    }
#endif

#if TCFG_WDRC_NODE_ENABLE
    if (!strncmp(name->name, "MusicDrc", strlen("MusicDrc"))) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;//目标配置项
        ret = 1;
    }
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
    if (!strncmp(name->name, "MusicBassTre", strlen("MusicBassTre")) || (!effect_strcmp(name->name, "BassMedia"))) {
        ret = get_music_bass_treble_parm(arg);
    }
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
    if (!strncmp(name->name, "MicBassTre", strlen("MicBassTre")) || (!effect_strcmp(name->name, "BassTreEff"))) {
        ret = get_mic_bass_treble_parm(arg);
    }
#endif

#if TCFG_SURROUND_NODE_ENABLE
    if (!effect_strcmp(name->name, "SurMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_CROSSOVER_NODE_ENABLE
    if (!effect_strcmp(name->name, "CrossMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if (TCFG_3BAND_MERGE_ENABLE || TCFG_2BAND_MERGE_ENABLE)
    if (!effect_strcmp(name->name, "BandMedia")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
    if (!effect_strcmp(name->name, "BassMedia")) {
        struct bass_treble_default_parm *get_bass_parm = (struct bass_treble_default_parm *)arg;
        get_bass_parm->cfg_index = 0;
        get_bass_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_STEROMIX_NODE_ENABLE
    if (!effect_strcmp(name->name, "Smix*Media")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_WDRC_NODE_ENABLE
    if (!effect_strcmp(name->name, "Drc*Media")) {
        struct eff_default_parm *get_parm = (struct eff_default_parm *)arg;
        get_parm->cfg_index = 0;
        get_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_EQ_ENABLE
    if (!effect_strcmp(name->name, "Eq*Media")) {
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        if (!effect_strcmp(name->name, "Eq0Media")) {
            get_eq_parm->cfg_index = get_music_eq_preset_index();
        } else {
            get_eq_parm->cfg_index = 0;
        }
        get_eq_parm->mode_index = get_current_scene();
        ret = 1;
    }
#endif

#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    if (!effect_strcmp(name->name, "VocalRemovMedia")) {
    }
#endif

#if TCFG_EQ_ENABLE
    if (!effect_strcmp(name->name, "EscoDlEq") || !effect_strcmp(name->name, "EscoUlEq")) {
        struct eq_default_parm *get_eq_parm = (struct eq_default_parm *)arg;
        int type = lmp_private_get_esco_packet_type();
        int media_type = type & 0xff;
        if (media_type == 0) {//narrow band
            get_eq_parm->cfg_index = 1;
        } else {//wide band
            get_eq_parm->cfg_index = 0;
        }
        ret = 1;
    }
#endif

#if TCFG_SPECTRUM_ADVANCE_NODE_ENABLE
    if (!strncmp(name->name, "SpectrumAdv", strlen("SpectrumAdv"))) {//频谱检测 回调接口配置
        struct spectrum_advance_set_handler *get_handler = (struct spectrum_advance_set_handler *)arg;
        if (get_handler->type == SET_SPECTRUM_ADVANCE_HANDLER) {
            get_handler->fps  = 60;//1秒内输出的帧数
            get_handler->cbuf_len = 4096 * 4;
            get_handler->read_len = 4096;
            get_handler->handler = audio_spectrum_advance_handler;
        }
        ret = 1;
    }
#endif

#if TCFG_SPECTRUM_NODE_ENABLE
    if (!strncmp(name->name, "Spectrum", strlen("Spectrum"))) {//频谱检测 缓冲buf配置
        struct spectrum_set_handler *get_handler = (struct spectrum_set_handler *)arg;
        if (get_handler->type == SET_SPECTRUM_HANDLER) {
            get_handler->cbuf_len = 4096 * 4;
            get_handler->read_len = 4096;
        }
        ret = 1;
    }
#endif

#if TCFG_EQ_ENABLE
#if ((defined TCFG_AUDIO_SPATIAL_EFFECT_ENABLE) && TCFG_AUDIO_SPATIAL_EFFECT_ENABLE)
    if (spatial_effect_eq_default_parm_set(name->name, (struct eq_default_parm *)arg)) {
        ret = 1;
    }
#endif
#endif

    return ret;
}

static u32 effect_strcmp(const char *str1, const char *str2)
{
    return ASCII_StrCmp(str1, str2, strlen(str1) + 1);
}
