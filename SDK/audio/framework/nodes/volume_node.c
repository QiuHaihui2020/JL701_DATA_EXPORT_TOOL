#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".volume_node.data.bss")
#pragma data_seg(".volume_node.data")
#pragma const_seg(".volume_node.text.const")
#pragma code_seg(".volume_node.text")
#endif
#include "jlstream.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "app_main.h"
#include "audio_config.h"
#include "audio_dvol.h"
#include "effects/effects_adj.h"
#include "volume_node.h"
#include "scene_switch.h"
#include "app_config.h"

#define SYS_LOG_ENABLE          0
#if SYS_LOG_ENABLE
#define LOG_TAG     "[VOL-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif

struct volume_hdl {
    char name[16]; //节点名称
    enum stream_scene scene;
    u8 Qval;//数据的饱和位宽
    u8 state;
    u8 bypass;//是否跳过节点处理
    u8 online_update_disable;//是否支持在线音量更新
    u16 target_dig_volume;//优化底噪时的固定数字音量
    u16 cfg_len;//配置数据实际大小
    struct volume_cfg *vol_cfg;
    dvol_handle *dvol_hdl; //音量操作句柄
    struct node_port_data_wide data_wide;
};

//默认音量配置
struct volume_cfg default_vol_cfg = {
    .bypass = 0,
    .cfg_level_max = 16,
    .cfg_vol_min = -45,
    .cfg_vol_max = 0,
    .vol_table_custom = 0,
    .cur_vol = 11,
};

__NODE_CACHE_CODE(volume)
static void volume_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;
    struct volume_hdl *hdl = (struct volume_hdl *)iport->node->private_data;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        if (!hdl->bypass) {
            audio_digital_vol_run(hdl->dvol_hdl, (s16 *)frame->data, frame->len);
        }
        if (node->oport) {
            jlstream_push_frame(node->oport, frame);
        } else {
            jlstream_free_frame(frame);
        }
        break;
    }
}

static int volume_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

static void volume_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = volume_handle_frame;
}


void audio_digital_vol_cfg_init(dvol_handle *dvol, struct volume_cfg *vol_cfg) //初始化配置
{
    if (dvol) {
        dvol-> cfg_vol_max = vol_cfg->cfg_vol_max;
        dvol-> cfg_vol_min = vol_cfg->cfg_vol_min;
        dvol-> cfg_level_max = vol_cfg->cfg_level_max;
        dvol-> vol_table_custom = vol_cfg->vol_table_custom;
#if VOL_TAB_CUSTOM_EN
        if (dvol->vol_table_custom == VOLUME_TABLE_CUSTOM_EN) {
            dvol->vol_table = vol_cfg->vol_table ;
        }
#endif
        dvol->vol_limit = (dvol->vol_limit > dvol->cfg_level_max) ? dvol->cfg_level_max : dvol->vol_limit;
        u16 vol_level = dvol->vol * dvol->vol_limit / dvol->vol_max;
        if (vol_level == 0) {
            dvol->vol_target = 0;
        } else if (dvol->vol_table) {
            dvol->vol_target = dvol->vol_table[vol_level - 1];
        } else {
            extern float eq_db2mag(float x);
            u16 step = (dvol->cfg_level_max - 1 > 0) ? (dvol->cfg_level_max - 1) : 1;
            float step_db = (dvol->cfg_vol_max - dvol->cfg_vol_min) / (float)step;
            float dvol_db = dvol->cfg_vol_min  + (vol_level - 1) * step_db;
            float dvol_gain = eq_db2mag(dvol_db);//dB转换倍数
            dvol->vol_target = (s16)(DVOL_MAX_FLOAT  * dvol_gain + 0.5f);
            log_debug("vol param:%d,(%d/100)dB,cur:%d,max:%d,(%d/100)dB", dvol->vol_target, (int)step_db * 100, vol_level, vol_cfg->cfg_level_max, (int)dvol_db * 100);
#if 0
//打印音量表每一级的目标音量值和dB值,调试用
            log_debug("=========================================vol table=================================================================");
            int i = 1;
            for (i = 1; i < dvol->cfg_level_max + 1; i++) {
                float debug_db = dvol->cfg_vol_min  + (i - 1) * step_db;
                float debug_gain = eq_db2mag(debug_db);//dB转换倍数
                int debug_target = (s16)(DVOL_MAX_FLOAT  * debug_gain + 0.5f);
                log_debug("dvol[%d] = %d,(%d / 100)dB", i, debug_target, (int)(debug_db * 100));
            }
            log_debug("====================================================================================================================");
#endif
        }

        dvol->vol_fade = 0; //从0开始淡入到目标音量
        if ((dvol->cfg_vol_min == -45) && (dvol->cfg_level_max == 31) && (dvol->cfg_vol_max == 0)) {
            dvol-> vol_table_default = 1; //使用默认的音量表
        }
    }
}

static void volume_ioc_start(struct volume_hdl *hdl)
{
    /*
     *获取配置文件内的参数,及名字
     * */
    int len = 0;
    struct audio_vol_params params = {0};
    struct volume_cfg *vol_cfg = NULL;
    if (!hdl->vol_cfg) {
        struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
        int ret = jlstream_read_node_info_data(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, hdl->name, &info);
        /* log_debug("enter volume_node.c %d,%d,%d\n", __LINE__, ret, info.size); */
        if (ret) {
            hdl->cfg_len = info.size;
            vol_cfg = hdl->vol_cfg =  zalloc(info.size);
            len = jlstream_read_form_cfg_data(&info, (void *)vol_cfg);
            if (info.size > sizeof(struct volume_cfg)) { //有自定义音量表,dB转成对应音量
                for (int i = 0; i < vol_cfg->cfg_level_max ; i++) {
                    /* log_debug("custom dvol [%d] = %d / 100 dB", i, (int)(hdl->vol_cfg->vol_table[i] * 100)); */
                    float dvol_gain = eq_db2mag(hdl->vol_cfg->vol_table[i]);//dB转换倍数
                    hdl->vol_cfg->vol_table [i]  = (s16)(DVOL_MAX_FLOAT  * dvol_gain + 0.5f);
                    /* log_debug("custom dvol[%d] = %d", i, (int)hdl->vol_cfg->vol_table[i]); */
                }
            }
        }
    } else {
        len = hdl->cfg_len;
        vol_cfg = hdl->vol_cfg;
    }
    if (!len) {
        log_debug("%s, read node data err\n", __FUNCTION__);
    }

    hdl->data_wide.iport_data_wide = hdl_node(hdl)->iport->prev->fmt.bit_wide;
    hdl->data_wide.oport_data_wide = hdl_node(hdl)->oport->fmt.bit_wide;
    /*log_d("%s bit_wide, %d %d %d\n", __FUNCTION__, hdl->data_wide.iport_data_wide, hdl->data_wide.oport_data_wide, hdl_node(hdl)->oport->fmt.Qval);*/

    log_debug("%s len %d, sizeof(cfg) %lu\n", __func__,  len, sizeof(struct volume_cfg));
    if (len != hdl->cfg_len) {
        if (hdl->vol_cfg) {
            free(hdl->vol_cfg);
            hdl->vol_cfg = NULL;
        }
        vol_cfg = &default_vol_cfg;
    } else {
        log_debug("vol read config ok :%d,%d,%d,%d,%d\n", vol_cfg->cfg_level_max, (int)vol_cfg->cfg_vol_min, (int)vol_cfg->cfg_vol_max, vol_cfg->vol_table_custom, vol_cfg->bypass);
    }

    /*
       *获取在线调试的临时参数
       * */
    if (config_audio_cfg_online_enable) {
        u32 online_cfg_len = sizeof(struct volume_cfg) + DIGITAL_VOLUME_LEVEL_MAX * sizeof(float);
        struct volume_cfg *online_vol_cfg = zalloc(online_cfg_len);
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, online_vol_cfg, online_cfg_len)) {
            /* printf("cfg_level_max = %d\n", online_vol_cfg->cfg_level_max); */
            /* printf("cfg_vol_min = %d\n", online_vol_cfg->cfg_vol_min); */
            /* printf("cfg_vol_max = %d\n", online_vol_cfg->cfg_vol_max); */
            /* printf("vol_table_custom = %d\n", online_vol_cfg->vol_table_custom); */
            /* printf("cur_vol = %d\n", online_vol_cfg->cur_vol); */
            /* printf("tab_len = %d\n", online_vol_cfg->tab_len); */
            hdl->vol_cfg-> cfg_vol_max =  online_vol_cfg->cfg_vol_max;
            hdl->vol_cfg-> cfg_vol_min = online_vol_cfg->cfg_vol_min;
            hdl->vol_cfg-> bypass = online_vol_cfg->bypass;
#if VOL_TAB_CUSTOM_EN
            if (hdl->vol_cfg->tab_len == online_vol_cfg->tab_len && hdl->vol_cfg->tab_len) {
                for (int i = 0; i < hdl->vol_cfg->cfg_level_max ; i++) {//重新计算音量表的值
                    log_debug("custom dvol [%d] = %d / 100 dB", i, (int)(online_vol_cfg->vol_table[i] * 100));
                    float dvol_gain = eq_db2mag(online_vol_cfg->vol_table[i]);//dB转换倍数
                    hdl->vol_cfg->vol_table [i]  = (s16)(DVOL_MAX_FLOAT  * dvol_gain + 0.5f);
                    log_debug("custom dvol[%d] = %d", i, (int)online_vol_cfg->vol_table[i]);
                }
            }
#endif
            log_debug("get volume online param\n");
        }
        free(online_vol_cfg);
    }
    hdl->bypass = vol_cfg->bypass;
    switch (hdl->scene) {
    case STREAM_SCENE_TONE:
        hdl->state = APP_AUDIO_STATE_WTONE;
#if TONE_BGM_FADEOUT
        audio_digital_vol_bg_fade(1);
#endif
        params.vol        = app_audio_get_volume(APP_AUDIO_STATE_WTONE);
        params.vol_max    = vol_cfg->cfg_level_max;
        params.fade_step  = TONE_DVOL_FS;
        params.vol_limit  = -1;
        break;
    case STREAM_SCENE_RING:
        hdl->state = APP_AUDIO_STATE_RING;
#if TONE_BGM_FADEOUT
        audio_digital_vol_bg_fade(1);
#endif
        params.vol        = app_audio_get_volume(APP_AUDIO_STATE_RING);
        params.vol_max    = vol_cfg->cfg_level_max;
        params.fade_step  = TONE_DVOL_FS;
        params.vol_limit  = -1;
        break;
    case STREAM_SCENE_KEY_TONE:
        /*puts("set_tone_volume\n");*/
        hdl->state = APP_AUDIO_STATE_KTONE;
#if TONE_BGM_FADEOUT
        /*printf("tone_player.c %d tone play,BGM fade_out automatically\n", __LINE__);*/
        audio_digital_vol_bg_fade(1);
#endif/*TONE_BGM_FADEOUT*/
        params.vol        = app_audio_get_volume(APP_AUDIO_STATE_KTONE);
        params.vol_max    = vol_cfg->cfg_level_max;
        params.fade_step  = TONE_DVOL_FS;
        params.vol_limit  = -1;
        break;
    case STREAM_SCENE_A2DP:
    case STREAM_SCENE_LINEIN:
    case STREAM_SCENE_IIS:
    case STREAM_SCENE_SPDIF:
    case STREAM_SCENE_PC_SPK:
    case STREAM_SCENE_PC_MIC:
    case STREAM_SCENE_MUSIC:
    case STREAM_SCENE_FM:
    case STREAM_SCENE_MIC_EFFECT:
    case STREAM_SCENE_HEARING_AID:
    case STREAM_SCENE_WIRELESS_MIC:
    case STREAM_SCENE_MIC_EFFECT2:
    case STREAM_SCENE_LE_AUDIO:
    case STREAM_SCENE_LOCAL_TWS:
    case STREAM_SCENE_TDM:
        /*puts("set_a2dp_volume\n");*/
        hdl->state = APP_AUDIO_STATE_MUSIC;
        params.vol        = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        params.vol_max    = vol_cfg->cfg_level_max;
        params.fade_step  = MUSIC_DVOL_FS;
        params.vol_limit  = -1;
        break;
    case STREAM_SCENE_LEA_CALL:
    case STREAM_SCENE_ESCO:
        /*puts("set_esco_volume\n");*/
        hdl->state = APP_AUDIO_STATE_CALL;
        params.vol        = app_audio_get_volume(APP_AUDIO_STATE_CALL);
        params.vol_max    = vol_cfg->cfg_level_max;
        params.fade_step  = CALL_DVOL_FS;
        params.vol_limit  = -1;
        break;
    default:
        break;
    }
    params.bit_wide   = hdl_node(hdl)->oport->fmt.bit_wide;
    if (!hdl->dvol_hdl) {
        hdl->dvol_hdl =  audio_digital_vol_open(params);
    }
    audio_digital_vol_cfg_init(hdl->dvol_hdl, vol_cfg);

    if (hdl->dvol_hdl) {
        hdl->dvol_hdl->mute_en = app_audio_get_mute_state(hdl->state);
        if (hdl->dvol_hdl->mute_en) {
            hdl->dvol_hdl->vol_fade = 0;
        }
    }
    if ((hdl->scene == STREAM_SCENE_MIC_EFFECT) || (hdl->scene == STREAM_SCENE_MIC_EFFECT2)) {
        return;
    }
#ifdef DVOL_2P1_CH_DVOL_ADJUST_NODE
#if (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_LR)
    char *substr = strstr(hdl->name, "Music");
    if (!strcmp(substr, "Music") || !strcmp(substr, "Music2")) { //找到默认初始化为最大音量的节点
        //printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->dvol_hdl->mute_en = 0;
        hdl->online_update_disable = 1;
    } else {
        //printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#elif (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_SW)
    char *substr = strstr(hdl->name, "Music");
    if (!strcmp(substr, "Music") || !strcmp(substr, "Music1")) { //找到默认初始化为最大音量的节点
        // printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->dvol_hdl->mute_en = 0;
        hdl->online_update_disable = 1;
    } else {
        // printf("enter volume_node.c %d,%p,%s\n", __LINE__, hdl->dvol_hdl, substr);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#else
    if (memchr(hdl->name, '1', 16) || memchr(hdl->name, '2', 16)) { //找到默认初始化为最大音量的节点
        //printf("enter volume_node.c %d,%p\n", __LINE__, hdl->dvol_hdl);
        audio_digital_vol_set(hdl->dvol_hdl, hdl->dvol_hdl->vol_max);
        hdl->online_update_disable = 1;
        hdl->dvol_hdl->mute_en = 0;
    } else {
        //printf("enter volume_node.c %d,%p\n", __LINE__, hdl->dvol_hdl);
        app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
    }
#endif
#else
    app_audio_state_switch(hdl->state, vol_cfg->cfg_level_max, hdl->dvol_hdl);
#endif
}

static void volume_ioc_stop(struct volume_hdl *hdl)
{
    if ((hdl->scene != STREAM_SCENE_MIC_EFFECT) && (hdl->scene != STREAM_SCENE_MIC_EFFECT2)) {
        app_audio_state_exit(hdl->state);
    }
#if TONE_BGM_FADEOUT
    log_debug("tone_player.c %d tone play,BGM fade_out close\n", __LINE__);
    audio_digital_vol_bg_fade(0);
#endif/*TONE_BGM_FADEOUT*/
    audio_digital_vol_close(hdl->dvol_hdl);
    hdl->dvol_hdl = NULL;
}

int volume_ioc_get_cfg(const char *name, struct volume_cfg *vol_cfg)
{
    char mode_index = get_current_scene();
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    int ret = jlstream_read_info_data(mode_index, name, 0, &info);
    if (!ret) {
        *vol_cfg = default_vol_cfg;
    } else {
        struct volume_cfg *temp_vol_cfg =  zalloc(info.size);
        int len = jlstream_read_form_cfg_data(&info, (void *)temp_vol_cfg);
        if (len == info.size) {
            *vol_cfg = *temp_vol_cfg;
        } else {
            *vol_cfg = default_vol_cfg;
        }
        free(temp_vol_cfg); //赋值完结构体释放内存
        /* printf("volume node read cfg name %s", name); */
        /* printf("cfg_level_max = %d\n", vol_cfg->cfg_level_max); */
        /* printf("cfg_vol_min = %d\n", vol_cfg->cfg_vol_min); */
        /* printf("cfg_vol_max = %d\n", vol_cfg->cfg_vol_max); */
        /* printf("vol_table_custom = %d\n", vol_cfg->vol_table_custom); */
        /* printf("cur_vol = %d\n", vol_cfg->cur_vol); */
    }
    return ret;
}

u16 volume_ioc_get_max_level(const char *name)
{
    struct volume_cfg vol_cfg;
    volume_ioc_get_cfg(name, &vol_cfg);
    return vol_cfg.cfg_level_max;
}


float volume_ioc_2_dB(struct volume_hdl *hdl, s16 volume)
{
    if (hdl->dvol_hdl) {
        dvol_handle *dvol = hdl->dvol_hdl;
        u8 vol_level = volume * dvol->vol_limit / dvol->vol_max;
        u16 step = (dvol->cfg_level_max - 1 > 0) ? (dvol->cfg_level_max - 1) : 1;
        float step_db = (dvol->cfg_vol_max - dvol->cfg_vol_min) / (float)step;
        float dvol_db = dvol->cfg_vol_min  + (vol_level - 1) * step_db;
        printf("vol_dB :%d,%d\n", __LINE__, (int)dvol_db);
        return (dvol_db);
    }
    return 0;
}

static int volume_ioc_update_parm(struct volume_hdl *hdl, int parm)
{
    struct volume_cfg *vol_cfg = (struct volume_cfg *)parm;
    int ret = false;
    int cmd = (vol_cfg->bypass & 0xf0);
    int value = vol_cfg->cur_vol;

    switch (cmd) {
    case VOLUME_NODE_CMD_SET_VOL:
        s16 volume = value & 0xffff;
        if (volume < 0) {
            volume = 0;
        }
        if (hdl && hdl->dvol_hdl) {
#if defined(VOL_NOISE_OPTIMIZE) &&( VOL_NOISE_OPTIMIZE)
            if (volume) {
                if (volume_ioc_2_dB(hdl, volume) < TARGET_DIG_DB && hdl->state == APP_AUDIO_STATE_MUSIC) {
                    if (!hdl->target_dig_volume) {
                        s16 temp_volume = 0;
                        for (temp_volume = hdl->dvol_hdl->vol_max; volume_ioc_2_dB(hdl, temp_volume) > TARGET_DIG_DB; temp_volume--) {};
                        hdl->target_dig_volume = temp_volume + 1;
                    }

                    //printf("enter volume_node.c %d,%d/100,%d/100,%d,%d,%d\n",__LINE__,(int)(volume_ioc_2_dB(hdl,volume)  * 100),(int)(volume_ioc_2_dB(hdl,hdl->target_dig_volume)* 100), hdl->target_dig_volume,volume,hdl->dvol_hdl->vol);

                    float dac_dB =  volume_ioc_2_dB(hdl, volume) - volume_ioc_2_dB(hdl, hdl->target_dig_volume)  ;

                    app_audio_dac_set_dB(dac_dB);
                    volume = hdl->target_dig_volume;
                } else { //把DAC 设回0dB
                    app_audio_dac_set_dB(0);
                }
            }
#endif
            audio_digital_vol_set(hdl->dvol_hdl, volume);
            log_debug("SET VOL volume update success : %d\n", volume);
            ret = true;
        }
        break;
    case VOLUME_NODE_CMD_SET_MUTE:
        s16 mute_en = value & 0xffff;
        if (hdl && hdl->dvol_hdl) {
            audio_digital_vol_mute_set(hdl->dvol_hdl, mute_en);
            log_debug("SET MUTE mute update success : %d\n", mute_en);
            ret = true;
        }
        break;
    default:
        if (hdl && hdl->dvol_hdl) {
            if (!hdl->online_update_disable) {
                hdl->dvol_hdl-> cfg_vol_max =  vol_cfg->cfg_vol_max;
                hdl->dvol_hdl-> cfg_vol_min = vol_cfg->cfg_vol_min;
#if VOL_TAB_CUSTOM_EN
                if (hdl->vol_cfg->tab_len == vol_cfg->tab_len && hdl->vol_cfg->tab_len) {
                    for (int i = 0; i < hdl->vol_cfg->cfg_level_max ; i++) {//重新计算音量表的值
                        //printf("custom dvol [%d] = %d / 100 dB", i, (int)(vol_cfg->vol_table[i] * 100));
                        float dvol_gain = eq_db2mag(vol_cfg->vol_table[i]);//dB转换倍数
                        hdl->vol_cfg->vol_table [i]  = (s16)(DVOL_MAX_FLOAT  * dvol_gain + 0.5f);
                        //printf("custom dvol[%d] = %d", i, (int)vol_cfg->vol_table[i]);
                    }
                    if (hdl->dvol_hdl->vol_table_custom == VOLUME_TABLE_CUSTOM_EN) {
                        hdl->dvol_hdl->vol_table = hdl->vol_cfg->vol_table ;
                    }
                }
#endif
                hdl->bypass = vol_cfg->bypass;
                audio_digital_vol_set(hdl->dvol_hdl, vol_cfg->cur_vol);
                if ((hdl->scene != STREAM_SCENE_MIC_EFFECT) || (hdl->scene != STREAM_SCENE_MIC_EFFECT2)) {//混响在线调试音量不更新音量状态的值
                    app_audio_change_volume(app_audio_get_state(), vol_cfg->cur_vol);
                }
            }
            /* printf("cfg_level_max = %d\n", vol_cfg->cfg_level_max); */
            /* printf("cfg_vol_min = %d\n", vol_cfg->cfg_vol_min); */
            /* printf("cfg_vol_max = %d\n", vol_cfg->cfg_vol_max); */
            /* printf("vol_table_custom = %d\n", vol_cfg->vol_table_custom); */
            /* printf("cur_vol = %d\n", vol_cfg->cur_vol); */
            log_debug("volume update success : %d\n", vol_cfg->cur_vol);
            ret = true;
            return ret;
        }
        log_debug("parm update failed : %x\n", value);
        break;
    }
    return ret;
}

static int get_volume_ioc_parm(struct volume_hdl *hdl, int parm)
{
    int ret = 0;
    struct volume_cfg *cfg = (struct volume_cfg *)parm;
    if (hdl && hdl->dvol_hdl) {
        hdl->vol_cfg->cur_vol = hdl->dvol_hdl->vol;
    }
    memcpy(cfg, hdl->vol_cfg, sizeof(struct volume_cfg));
    ret = sizeof(struct volume_cfg);
    return ret;
}

static int volume_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct volume_hdl *hdl = (struct volume_hdl *)iport->node->private_data;

    int ret = 0;
    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        volume_open_iport(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = (enum stream_scene)arg;
        break;
    case NODE_IOC_START:
        volume_ioc_start(hdl);
        break;
    case NODE_IOC_STOP:
    case NODE_IOC_SUSPEND:
        volume_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        /* printf("volume_node name match :%s,%s\n", hdl->name, (const char *)arg); */
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = volume_ioc_update_parm(hdl, arg);
        break;
    case NODE_IOC_GET_PARAM:
        ret = get_volume_ioc_parm(hdl, arg);
        break;

    }

    return ret;
}

static void volume_release(struct stream_node *node)
{
    struct volume_hdl *hdl = (struct volume_hdl *)node->private_data;
    if (hdl->vol_cfg) {
        free(hdl->vol_cfg);
        hdl->vol_cfg = NULL;
    }
}


REGISTER_STREAM_NODE_ADAPTER(volume_node_adapter) = {
    .name       = "volume",
    .uuid       = NODE_UUID_VOLUME_CTRLER,
    .bind       = volume_bind,
    .ioctl      = volume_ioctl,
    .release    = volume_release,
    .hdl_size   = sizeof(struct volume_hdl),
};

REGISTER_ONLINE_ADJUST_TARGET(volume) = {
    .uuid = NODE_UUID_VOLUME_CTRLER,
};

