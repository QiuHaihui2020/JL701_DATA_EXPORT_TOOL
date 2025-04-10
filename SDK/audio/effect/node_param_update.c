#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".node_param_update.data.bss")
#pragma data_seg(".node_param_update.data")
#pragma const_seg(".node_param_update.text.const")
#pragma code_seg(".node_param_update.text")
#endif
#include "jlstream.h"
#include "node_uuid.h"
#include "effects/effects_adj.h"
#include "node_param_update.h"


/* 各模块参数更新接口 */
int stero_mix_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    stereo_mix_gain_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_STEROMIX, node_name, &cfg, sizeof(cfg));
}
int surround_effect_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    surround_effect_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_SURROUND, node_name, &cfg, sizeof(cfg));
}
int crossover_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    crossover_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    if (cfg.parm.way_num == 3) {
        ret = jlstream_set_node_param(NODE_UUID_CROSSOVER, node_name, &cfg, sizeof(cfg));
    } else {
        ret = jlstream_set_node_param(NODE_UUID_CROSSOVER_2BAND, node_name, &cfg, sizeof(cfg));
    }
    return ret;
}
int band_merge_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    multi_mix_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_3BAND_MERGE, node_name, &cfg, sizeof(cfg));
}

int two_band_merge_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    multi_mix_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_2BAND_MERGE, node_name, &cfg, sizeof(cfg));
}

int vocal_remover_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    vocal_remover_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_VOCAL_REMOVER, node_name, &cfg, sizeof(cfg));
}


int drc_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    wdrc_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_WDRC, node_name, &cfg, sizeof(cfg));
}
int bass_treble_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    bass_treble_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_BASS_TREBLE, node_name, &cfg, sizeof(cfg));
}
int autotune_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    autotune_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_AUTOTUNE, node_name, &cfg, sizeof(cfg));
}
int chorus_udpate_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    chorus_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_CHORUS, node_name, &cfg, sizeof(cfg));
}
int dynamic_eq_pro_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    dynamic_eq_pro_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_DYNAMIC_EQ_PRO, node_name, &cfg, sizeof(cfg));
}
int dynamic_eq_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    dynamic_eq_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_DYNAMIC_EQ, node_name, &cfg, sizeof(cfg));
}

int echo_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{

    echo_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_ECHO, node_name, &cfg, sizeof(cfg));
}
int howling_frequency_shift_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    howling_pitchshift_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_FREQUENCY_SHIFT, node_name, &cfg, sizeof(cfg));
}
int howling_suppress_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    notch_howling_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_HOWLING_SUPPRESS, node_name, &cfg, sizeof(cfg));
}
int gain_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    gain_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_GAIN, node_name, &cfg, sizeof(cfg));
}
int noisegate_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    noisegate_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_NOISEGATE, node_name, &cfg, sizeof(cfg));
}

int plate_reverb_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    plate_reverb_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_PLATE_REVERB, node_name, &cfg, sizeof(cfg));
}
int reverb_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    reverb_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_REVERB, node_name, &cfg, sizeof(cfg));
}

int reverb_advance_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    plate_reverb_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_PLATE_REVERB_ADVANCE, node_name, &cfg, sizeof(cfg));
}
int spectrum_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct spectrum_parm  cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_SPECTRUM, node_name, &cfg, sizeof(cfg));
}
int stereo_widener_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    stereo_widener_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_STEREO_WIDENER, node_name, &cfg, sizeof(cfg));
}
int virtual_bass_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    virtual_bass_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_VBASS, node_name, &cfg, sizeof(cfg));
}
int virtual_bass_classic_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    virtual_bass_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_VIRTUAL_BASS_CLASSIC, node_name, &cfg, sizeof(cfg));
}
int voice_changer_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    voice_changer_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_VOICE_CHANGER, node_name, &cfg, sizeof(cfg));
}
int channel_expander_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    channel_expander_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_CHANNEL_EXPANDER, node_name, &cfg, sizeof(cfg));
}
int harmonic_exciter_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    harmonic_exciter_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_HARMONIC_EXCITER, node_name, &cfg, sizeof(cfg));
}


/* eq参数更新接口 */
/*  return: 0 返回成功，非0 ：返回失败 */
int eq_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (!ret) {
        struct eq_tool *tab = zalloc(info.size);
        if (!jlstream_read_form_cfg_data(&info, tab)) {
            printf("user eq cfg parm read err\n");
            free(tab);
            return -1;
        }

        //运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_IS_BYPASS_CMD;
        eff.param.is_bypass = tab->is_bypass;
        ret = jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff)); //更新bypass 标志
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  tab->global_gain;
        eff.fade_parm.fade_time = 1;        //en
        eff.fade_parm.fade_step = 0.1f;  //滤波器增益淡入步进
        eff.fade_parm.q_fade_step = 0.1f;//滤波器q值淡入步进
        eff.fade_parm.g_fade_step = 0.1f;//总增益淡入步进
        eff.fade_parm.f_fade_step = 100;//滤波器中心截止频率淡入步进
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新总增益

        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = tab->seg_num;
        jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器段数

        for (int i = 0; i < tab->seg_num; i++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &tab->seg[i], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));//更新滤波器系数
        }

        free(tab);
        return ret;
    }
    return -1;
}

/*
 * 该接口可直接更新整个EQ系数表及总增益，
 * 切换系数表时的po声表现优于调用eq_update_parm更新
 * 需要使能const变量：config_audio_eq_xfade_enable = 1
 */
int eq_update_tab(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info  = {0};
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (!ret) {
        struct eq_tool *tab = zalloc(info.size);
        if (!jlstream_read_form_cfg_data(&info, tab)) {
            printf("user eq cfg parm read err\n");
            free(tab);
            return -1;
        }

        //运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_TAB_CMD;
        eff.param.tab.global_gain = tab->global_gain;
        eff.param.tab.seg_num = tab->seg_num;
        eff.param.tab.seg = tab->seg; //系数表指针赋值
        ret = jlstream_set_node_param(NODE_UUID_EQ, node_name, &eff, sizeof(eff));

        free(tab);
    }
    return ret;
}
/* 软件EQ参数更新接口 */
/*  return: 0 返回成功，非0 ：返回失败 */
int sw_eq_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (!ret) {
        struct eq_tool *tab = zalloc(info.size);
        if (!jlstream_read_form_cfg_data(&info, tab)) {
            printf("user eq cfg parm read err\n");
            free(tab);
            return -1;
        }

        //运行时，直接设置更新
        struct eq_adj eff = {0};
        eff.type = EQ_IS_BYPASS_CMD;
        eff.param.is_bypass = tab->is_bypass;
        ret = jlstream_set_node_param(NODE_UUID_SOF_EQ, node_name, &eff, sizeof(eff));//更新bypass标志
        eff.type = EQ_GLOBAL_GAIN_CMD;
        eff.param.global_gain =  tab->global_gain;
        eff.fade_parm.fade_time = 1;        //en
        eff.fade_parm.fade_step = 0.1f;  //滤波器增益淡入步进
        eff.fade_parm.q_fade_step = 0.1f;//滤波器q值淡入步进
        eff.fade_parm.g_fade_step = 0.1f;//总增益淡入步进
        eff.fade_parm.f_fade_step = 100;//滤波器中心截止频率淡入步进
        jlstream_set_node_param(NODE_UUID_SOF_EQ, node_name, &eff, sizeof(eff));//更新总增益

        eff.type = EQ_SEG_NUM_CMD;
        eff.param.seg_num = tab->seg_num;
        jlstream_set_node_param(NODE_UUID_SOF_EQ, node_name, &eff, sizeof(eff));//更新滤波器段数

        for (int i = 0; i < tab->seg_num; i++) {
            eff.type = EQ_SEG_CMD;
            memcpy(&eff.param.seg, &tab->seg[i], sizeof(struct eq_seg_info));
            jlstream_set_node_param(NODE_UUID_SOF_EQ, node_name, &eff, sizeof(eff));//更新滤波器系数
        }

        free(tab);
        return ret;
    }
    return -1;
}



int multiband_drc_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info = {0};
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (ret) {
        return -1;
    }
    u8 *data_buf = zalloc(info.size);
    if (!jlstream_read_form_cfg_data(&info, data_buf)) {
        printf("user mdrc cfg parm read err\n");
        free(data_buf);
        return -1;
    }
    struct mdrc_param_tool_set *mdrc_parm = zalloc(sizeof(struct mdrc_param_tool_set));
    multiband_drc_param_set(mdrc_parm, data_buf);
    free(data_buf);
    multiband_drc_param_debug(mdrc_parm);


    //common_parm 更新
    struct mdrc_common_param_update {
        int type;
        struct mdrc_common_param data;
    };
    struct mdrc_common_param_update *common_parm = zalloc(sizeof(struct mdrc_common_param_update));
    common_parm->type = COMMON_PARM;
    memcpy(&common_parm->data, &mdrc_parm->common_param, sizeof(struct mdrc_common_param));
    ret = jlstream_set_node_param(NODE_UUID_MDRC, node_name, common_parm, sizeof(struct mdrc_common_param_update));

    //drc更新
    struct mdrc_drc_param_update {
        int type;
        int len;
        u8 data[0];
    };
    struct mdrc_drc_param_update *drc_parm = zalloc(wdrc_advance_get_param_size(64) + sizeof(int) * 2);
    u8 drc_parm_type[] = {LOW_DRC_PARM, MID_DRC_PARM, HIGH_DRC_PARM, WHOLE_DRC_PARM};
    for (u8 i = 0; i < 4; i++) {
        drc_parm->type = drc_parm_type[i];
        drc_parm->len = mdrc_parm->drc_len[i];
        memcpy(drc_parm->data, mdrc_parm->drc_param[i], drc_parm->len);
        jlstream_set_node_param(NODE_UUID_MDRC, node_name, drc_parm, drc_parm->len + sizeof(int) * 2);
    }


    multiband_drc_param_free(mdrc_parm);
    free(mdrc_parm);
    free(drc_parm);
    free(common_parm);
    return ret;
}

int wdrc_advance_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info = {0};
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (ret) {
        return -1;
    }
    wdrc_advance_param_tool_set *wdrc_advance_parm = zalloc(info.size);
    if (!jlstream_read_form_cfg_data(&info, wdrc_advance_parm)) {
        printf("user wdrc_advance_parm cfg parm read err\n");
        free(wdrc_advance_parm);
        return -1;
    }
    ret = jlstream_set_node_param(NODE_UUID_WDRC_ADVANCE, node_name, wdrc_advance_parm, wdrc_advance_get_param_size(wdrc_advance_parm->parm.threshold_num));
    free(wdrc_advance_parm);
    return ret;
}


int virtual_surround_pro_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct virtual_surround_pro_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_UPMIX_2TO5, node_name, &cfg, sizeof(cfg));
}

int limiter_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct limiter_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_LIMITER, node_name, &cfg, sizeof(cfg));
}


int user_limiter_update_parm(u8 mode_index, char *node_name, u8 cfg_index, float threshold)
{
    struct limiter_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }

    cfg.parm.threshold = threshold * 1000;
    return jlstream_set_node_param(NODE_UUID_LIMITER, node_name, &cfg, sizeof(cfg));
}


int multiband_limiter_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct multiband_limiter_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_MULTIBAND_LIMITER, node_name, &cfg, sizeof(cfg));

}

int pcm_delay_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    pcm_delay_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_PCM_DELAY, node_name, &cfg, sizeof(cfg));

}
int howling_gate_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    howling_gate_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_HOWLING_GATE, node_name, &cfg, sizeof(cfg));
}

int noisegate_pro_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct noisegate_pro_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_NOISEGATE_PRO, node_name, &cfg, sizeof(cfg));
}

int phaser_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct phaser_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_PHASER, node_name, &cfg, sizeof(cfg));
}

int flanger_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct flanger_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_FLANGER, node_name, &cfg, sizeof(cfg));
}

int chorus_advance_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct chorus_advance_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_CHORUS_ADVANCE, node_name, &cfg, sizeof(cfg));
}

int pingpong_echo_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct pingpong_echo_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_PINGPONG_ECHO, node_name, &cfg, sizeof(cfg));
}

int stereo_spatial_wider_update_parm(u8 mode_index, char *node_name, u8 cfg_index)
{
    struct stereo_spatial_wider_param_tool_set cfg = {0};
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, node_name);
        return -1;
    }
    return jlstream_set_node_param(NODE_UUID_STEREO_SPATIAL_WIDER, node_name, &cfg, sizeof(cfg));
}

/*
 *通用音效模块更新
 * */
int node_param_update_parm(u16 uuid, u8 mode_index, char *node_name, u8 cfg_index)
{
    struct cfg_info info = {0};
    int ret = jlstream_read_form_node_info_base(mode_index, node_name, cfg_index, &info);
    if (ret) {
        printf("read info err, %s, %s, uuid:0x%x\n", __func__, node_name, uuid);
        return -1;
    }
    u8 *cfg = zalloc(info.size);
    if (!cfg) {
        printf("read alloc err, %s, %s, uuid:0x%x\n", __func__, node_name, uuid);
        return -1;
    }
    ret = jlstream_read_form_cfg_data(&info, cfg);
    if (!ret) {
        printf("read data err, %s, %s, uuid:0x%x\n", __func__, node_name, uuid);
        free(cfg);
        return -1;
    }
    ret = jlstream_set_node_param(uuid, node_name, cfg, ret);
    free(cfg);
    return ret;
}


