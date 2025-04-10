#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".virtual_bass_demo.data.bss")
#pragma data_seg(".virtual_bass_demo.data")
#pragma const_seg(".virtual_bass_demo.text.const")
#pragma code_seg(".virtual_bass_demo.text")
#endif

#include "effects/effects_adj.h"
#include "effects/audio_vbass.h"
#include "node_uuid.h"
#include "audio_config.h"

void user_virtual_bass_udpate_parm_demo()
{
    virtual_bass_param_tool_set cfg = {0};
    /*
     *解析配置文件内效果配置
     * */
    char mode_index       = 0;               //模式序号（当前节点无多参数组，mode_index是0）
    char *node_name       = "vbass_name";     //节点名称（节点内的第一参数，用户自定义,长度小于等于15byte）
    char cfg_index        = 0;               //目标配置项序号（当前节点无多参数组，cfg_index是0）
    int ret = jlstream_read_form_data(mode_index, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err\n");
        return;
    }
    /*
     *将配置文件内获取得到的参数更新到目标节点
     * */
    jlstream_set_node_param(NODE_UUID_VBASS, node_name, &cfg, sizeof(cfg));
}

#if AUDIO_VBASS_LINK_VOLUME
/*
 * 1、需要在每一个数据流callback里的start事件内调用该函数初始化一次。
 * 2、需要在app_audio_set_volume中调用。
 * */
int vbass_link_volume()
{
    u8 cfg_index;
    char *node_name = "VBassMedia";
    virtual_bass_param_tool_set cfg = {0};
    s16 cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    s16 max_vol = app_audio_get_max_volume();

    //以下音量判断逻辑请根据实际样机调试效果自行定义。
    if ((0 <= cur_vol) && (cur_vol <= (max_vol / 4))) {
        cfg_index = 0;
    } else if (((max_vol / 4) < cur_vol) && (cur_vol <= (max_vol * 2 / 4))) {
        cfg_index = 1;
    } else if (((max_vol * 2 / 4) < cur_vol) && (cur_vol <= (max_vol * 3 / 4))) {
        cfg_index = 2;
    } else if (((max_vol * 3 / 4) < cur_vol) && (cur_vol <= max_vol)) {
        cfg_index = 3;
    } else {
        cfg_index = 0;
    }

    int ret = jlstream_read_form_data(0, node_name, cfg_index, &cfg);
    if (!ret) {
        printf("read parm err\n");
        return -1;
    }
    printf("vbass link volume : cfg_index %d, is_bypass %d, ratio %d, boost %d, fc %d, ReserveLowFreqEnable %d\n",
           cfg_index, cfg.is_bypass, cfg.parm.ratio, cfg.parm.boost, cfg.parm.fc, cfg.parm.ReserveLowFreqEnable);

    /*
     *将配置文件内获取得到的参数更新到目标节点
     * */
    ret = jlstream_set_node_param(NODE_UUID_VBASS, node_name, &cfg, sizeof(cfg));
    if (!ret) {
        printf("set parm err\n");
        return -1;
    }
    return ret;
}
#endif
