#include "system/includes.h"
#include "app_config.h"
#include "scene_update.h"
#include "scene_switch.h"
#include "effects/effects_adj.h"
#include "node_param_update.h"
#include "audio_config_def.h"
#include "media_config.h"

#if defined(TCFG_SCENE_UPDATE_ENABLE) && TCFG_SCENE_UPDATE_ENABLE

#if 0 //打印较多，debug时打开
#define scene_update_log	printf
#else
#define scene_update_log(...)
#endif

#define NODE_UUID_SOURCE_TAG    0xEB56
#define NODE_UUID_SINK_TAG      0x603A

#define NODE_PARAM_READ_UPDATE_SIMPLE  1    //通用音效更新

struct pipeline_node_hdl {
    struct list_head entry;
    u16 uuid;
    u8 subid;
};
struct pipeline_node_list {
    struct list_head head;
};
struct node_port {
    u16 uuid;
    u8  number;
    u8  port;
};
struct node_link {
    struct node_port out;
    struct node_port in;
};

//储存流程中节点uuid、subid的数组，使用链表管理
static struct pipeline_node_list music_node_list;
#if TCFG_MIC_EFFECT_ENABLE
static struct pipeline_node_list mic_node_list;
#endif

//媒体音效
void get_music_pipeline_node_uuid()
{
    INIT_LIST_HEAD(&music_node_list.head);
    u16 pipeline = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"a2dp");
    u8 *pipeline_data;
    struct node_link *link;
    int len = jlstream_read_pipeline_data(pipeline, &pipeline_data);
    scene_update_log("pipeline 0x%x, len %d", pipeline, len);
    if (len == 0) {
        return;
    }
    for (int i = 0; i < len; i += 8) {
        link = (struct node_link *)(pipeline_data + i);

        struct node_param ncfg = {0};
        int node_len = jlstream_read_node_data(link->out.uuid, link->out.number, (u8 *)&ncfg);
        if (node_len != sizeof(ncfg)) {
            scene_update_log("uuid 0x%x, subid 0x%x, node no name\n", link->out.uuid, link->out.number);
            //过滤无参数配置的节点
            continue;
        }
        if (link->in.uuid == NODE_UUID_SOURCE_TAG || link->out.uuid == NODE_UUID_SINK_TAG) {
            //过滤sink source节点
            continue;
        }

        u8 ret = 0;
        struct pipeline_node_hdl *_hdl = NULL;
        local_irq_disable();
        list_for_each_entry(_hdl, &music_node_list.head, entry) {
            if (_hdl && (_hdl->uuid == link->out.uuid) && (_hdl->subid == link->out.number)) {
                //节点信息已经在链表中
                ret = 1;
                break;
            }
        }
        local_irq_enable();

        if (ret) {
            scene_update_log("node already in list");
            continue;
        }
        scene_update_log("uuid 0x%x, subid 0x%x, name %s\n", link->out.uuid, link->out.number, ncfg.name);
        struct pipeline_node_hdl *hdl = zalloc(sizeof(struct pipeline_node_hdl)); //不释放
        hdl->uuid = link->out.uuid;
        hdl->subid = link->out.number;
        local_irq_disable();
        list_add_tail(&hdl->entry, &music_node_list.head);
        local_irq_enable();
    }
    free(pipeline_data);
}

void update_music_pipeline_node_list(u8 mode_index, u8 cfg_index)
{
    struct pipeline_node_hdl *_hdl = NULL;
    list_for_each_entry(_hdl, &music_node_list.head, entry) {
        if (_hdl) {
            struct node_param ncfg = {0};
            int node_len = jlstream_read_node_data(_hdl->uuid, _hdl->subid, (u8 *)&ncfg);
            if (node_len == sizeof(ncfg)) {
                node_param_update(_hdl->uuid, ncfg.name, mode_index, cfg_index);
            }
        }
    }
}

#if TCFG_MIC_EFFECT_ENABLE
//麦克风音效
void get_mic_pipeline_node_uuid()
{
    INIT_LIST_HEAD(&mic_node_list.head);
    u16 pipeline = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_effect");
    u8 *pipeline_data;
    struct node_link *link;
    int len = jlstream_read_pipeline_data(pipeline, &pipeline_data);
    scene_update_log("pipeline 0x%x, len %d", pipeline, len);
    if (len == 0) {
        return;
    }
    for (int i = 0; i < len; i += 8) {
        link = (struct node_link *)(pipeline_data + i);

        struct node_param ncfg = {0};
        int node_len = jlstream_read_node_data(link->out.uuid, link->out.number, (u8 *)&ncfg);
        if (node_len != sizeof(ncfg)) {
            scene_update_log("uuid 0x%x, subid 0x%x, node no name\n", link->out.uuid, link->out.number);
            //过滤无参数配置的节点
            continue;
        }
        if (link->in.uuid == NODE_UUID_SOURCE_TAG || link->out.uuid == NODE_UUID_SINK_TAG) {
            //过滤sink source节点
            continue;
        }

        u8 ret = 0;
        struct pipeline_node_hdl *_hdl = NULL;
        local_irq_disable();
        list_for_each_entry(_hdl, &mic_node_list.head, entry) {
            if (_hdl && (_hdl->uuid == link->out.uuid) && (_hdl->subid == link->out.number)) {
                //节点信息已经在链表中
                ret = 1;
                break;
            }
        }
        local_irq_enable();

        if (ret) {
            scene_update_log("node already in list");
            continue;
        }
        scene_update_log("uuid 0x%x, subid 0x%x, name %s\n", link->out.uuid, link->out.number, ncfg.name);

        struct pipeline_node_hdl *hdl = zalloc(sizeof(struct pipeline_node_hdl)); //不释放
        hdl->uuid = link->out.uuid;
        hdl->subid = link->out.number;
        local_irq_disable();
        list_add_tail(&hdl->entry, &mic_node_list.head);
        local_irq_enable();
    }
    free(pipeline_data);
}

void update_mic_pipeline_node_list(u8 mode_index, u8 cfg_index)
{
    struct pipeline_node_hdl *_hdl = NULL;
    list_for_each_entry(_hdl, &mic_node_list.head, entry) {
        if (_hdl) {
            struct node_param ncfg = {0};
            int node_len = jlstream_read_node_data(_hdl->uuid, _hdl->subid, (u8 *)&ncfg);
            if (node_len == sizeof(ncfg)) {
                node_param_update(_hdl->uuid, ncfg.name, mode_index, cfg_index);
            }
        }
    }
}
#endif


/* 添加新的音效模块需要在这里手动添加参数更新接口 */
int node_param_update(u16 uuid, char *node_name, u8 mode_index, u8 cfg_index)
{
#if NODE_PARAM_READ_UPDATE_SIMPLE
    /*
     *简化版更新,如有特殊更新，需自行添加，否则使用通用更新node_param_update_parm
     * */
    int ret = 0;
    switch (uuid) {
#if TCFG_EQ_ENABLE
    case NODE_UUID_EQ:
        if (config_audio_eq_xfade_enable) {
            ret = eq_update_tab(mode_index, node_name, cfg_index);
        } else {
            ret = eq_update_parm(mode_index, node_name, cfg_index);
        }
        return ret;
#endif

#if TCFG_SOFWARE_EQ_NODE_ENABLE
    case NODE_UUID_SOF_EQ:
        ret = sw_eq_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif

#if TCFG_MULTI_BAND_DRC_NODE_ENABLE
    case NODE_UUID_MDRC:
        ret = multiband_drc_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif

    default://通用更新
        ret = node_param_update_parm(uuid, mode_index, node_name, cfg_index);
        return ret;
    }

#else

    int ret = 0;
    switch (uuid) {
#if TCFG_STEROMIX_NODE_ENABLE
    case NODE_UUID_STEROMIX:
        ret = stero_mix_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_SURROUND_NODE_ENABLE
    case NODE_UUID_SURROUND:
        ret = surround_effect_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_CROSSOVER_NODE_ENABLE
    case NODE_UUID_CROSSOVER:
    case NODE_UUID_CROSSOVER_2BAND:
        ret = crossover_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_3BAND_MERGE_ENABLE
    case NODE_UUID_3BAND_MERGE:
        ret = band_merge_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_2BAND_MERGE_ENABLE
    case NODE_UUID_2BAND_MERGE:
        ret = two_band_merge_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    case NODE_UUID_VOCAL_REMOVER:
        ret = vocal_remover_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_WDRC_NODE_ENABLE
    case NODE_UUID_WDRC:
        ret = drc_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_BASS_TREBLE_NODE_ENABLE
    case NODE_UUID_BASS_TREBLE:
        ret = bass_treble_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_AUTOTUNE_NODE_ENABLE
    case NODE_UUID_AUTOTUNE:
        ret = autotune_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_CHORUS_NODE_ENABLE
    case NODE_UUID_CHORUS:
        ret = chorus_udpate_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_DYNAMIC_EQ_NODE_ENABLE
    case NODE_UUID_DYNAMIC_EQ:
        ret = dynamic_eq_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_ECHO_NODE_ENABLE
    case NODE_UUID_ECHO:
        ret = echo_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
    case NODE_UUID_FREQUENCY_SHIFT:
        ret = howling_frequency_shift_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_NOTCH_HOWLING_NODE_ENABLE
    case NODE_UUID_HOWLING_SUPPRESS:
        ret = howling_suppress_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_GAIN_NODE_ENABLE
    case NODE_UUID_GAIN:
        ret = gain_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_NOISEGATE_NODE_ENABLE
    case NODE_UUID_NOISEGATE:
        ret = noisegate_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_PLATE_REVERB_NODE_ENABLE
    case NODE_UUID_PLATE_REVERB:
        ret = plate_reverb_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_REVERB_NODE_ENABLE
    case NODE_UUID_REVERB:
        ret = reverb_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
    case NODE_UUID_PLATE_REVERB_ADVANCE:
        ret = reverb_advance_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_SPECTRUM_NODE_ENABLE
    case NODE_UUID_SPECTRUM:
        ret = spectrum_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_STEREO_WIDENER_NODE_ENABLE
    case NODE_UUID_STEREO_WIDENER:
        ret = stereo_widener_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_VBASS_NODE_ENABLE
    case NODE_UUID_VBASS:
        ret = virtual_bass_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_VOICE_CHANGER_NODE_ENABLE
    case NODE_UUID_VOICE_CHANGER:
        ret = voice_changer_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_CHANNEL_EXPANDER_NODE_ENABLE
    case NODE_UUID_CHANNEL_EXPANDER:
        ret = channel_expander_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_HARMONIC_EXCITER_NODE_ENABLE
    case NODE_UUID_HARMONIC_EXCITER:
        ret = harmonic_exciter_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EQ_ENABLE
    case NODE_UUID_EQ:
        if (config_audio_eq_xfade_enable) {
            ret = eq_update_tab(mode_index, node_name, cfg_index);
        } else {
            ret = eq_update_parm(mode_index, node_name, cfg_index);
        }
        return ret;
#endif
#if TCFG_MULTI_BAND_DRC_NODE_ENABLE
    case NODE_UUID_MDRC:
        ret = multiband_drc_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_WDRC_ADVANCE_NODE_ENABLE
    case NODE_UUID_WDRC_ADVANCE:
        ret = wdrc_advance_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_VIRTUAL_SURROUND_PRO_NODE_ENABLE
    case NODE_UUID_UPMIX_2TO5:
        ret = virtual_surround_pro_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_LIMITER_NODE_ENABLE
    case NODE_UUID_LIMITER:
        ret = limiter_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_MULTI_BAND_LIMITER_NODE_ENABLE
    case NODE_UUID_MULTIBAND_LIMITER:
        ret = multiband_limiter_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_PCM_DELAY_NODE_ENABLE
    case NODE_UUID_PCM_DELAY:
        ret = pcm_delay_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_HOWLING_GATE_ENABLE
    case NODE_UUID_HOWLING_GATE:
        ret = howling_gate_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_NOISEGATE_PRO_NODE_ENABLE
    case NODE_UUID_NOISEGATE_PRO:
        ret = noisegate_pro_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_PHASER_NODE_ENABLE
    case NODE_UUID_PHASER:
        ret = phaser_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_FLANGER_NODE_ENABLE
    case NODE_UUID_FLANGER:
        ret = flanger_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_CHORUS_ADVANCE_NODE_ENABLE
    case NODE_UUID_CHORUS_ADVANCE:
        ret = chorus_advance_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_PINGPONG_ECHO_NODE_ENABLE
    case NODE_UUID_PINGPONG_ECHO:
        ret = pingpong_echo_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_STEREO_SPATIAL_WIDER_NDOE_ENABLE
    case NODE_UUID_STEREO_SPATIAL_WIDER:
        ret = stereo_spatial_wider_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EFFECT_DEV0_NODE_ENABLE
    case NODE_UUID_EFFECT_DEV0:
        ret = effect_dev0_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EFFECT_DEV1_NODE_ENABLE
    case NODE_UUID_EFFECT_DEV1:
        ret = effect_dev1_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EFFECT_DEV2_NODE_ENABLE
    case NODE_UUID_EFFECT_DEV2:
        ret = effect_dev2_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EFFECT_DEV3_NODE_ENABLE
    case NODE_UUID_EFFECT_DEV3:
        ret = effect_dev3_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_EFFECT_DEV4_NODE_ENABLE
    case NODE_UUID_EFFECT_DEV4:
        ret = effect_dev4_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
#if TCFG_SOFWARE_EQ_NODE_ENABLE
    case NODE_UUID_SOF_EQ:
        ret = sw_eq_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif

#if TCFG_SPLIT_GAIN_NODE_ENABLE
    case NODE_UUID_SPLIT_GAIN:
        ret = split_gain_update_parm(mode_index, node_name, cfg_index);
        return ret;
#endif
    default:
        scene_update_log("param_update_err, uuid 0x%x, node_name %s", uuid, node_name);
        return ret;
    }
#endif

}

#endif
