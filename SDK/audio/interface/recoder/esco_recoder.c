#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_recoder.data.bss")
#pragma data_seg(".esco_recoder.data")
#pragma const_seg(".esco_recoder.text.const")
#pragma code_seg(".esco_recoder.text")
#endif
#include "jlstream.h"
#include "esco_recoder.h"
#include "encoder_node.h"
#include "sdk_config.h"
#include "media/includes.h"
#include "app_config.h"
#include "asm/dac.h"
#include "audio_cvp.h"
#include "esco_player.h"


#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif

extern struct audio_dac_hdl dac_hdl;
struct esco_recoder {
    struct jlstream *stream;
};

static struct esco_recoder *g_esco_recoder = NULL;


static void esco_recoder_callback(void *private_data, int event)
{
    struct esco_recoder *recoder = g_esco_recoder;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int esco_recoder_open(u8 link_type, void *bt_addr)
{
    int err;
    struct encoder_fmt enc_fmt;
    struct esco_recoder *recoder;
    u16 source_uuid = 0;

    if (g_esco_recoder) {
        return -EBUSY;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"esco");
    if (uuid == 0) {
        return -EFAULT;
    }

    recoder = malloc(sizeof(*recoder));
    if (!recoder) {
        return -ENOMEM;
    }


    recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    source_uuid = NODE_UUID_ADC;
    if (!recoder->stream) {
        recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PDM_MIC);
        source_uuid = NODE_UUID_PDM_MIC;
    }
    if (!recoder->stream) {
        recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);
        source_uuid = NODE_UUID_IIS0_RX;
    }
    if (!recoder->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

#if TCFG_AUDIO_SIDETONE_ENABLE && TCFG_SWITCH_NODE_ENABLE
    /*设置switch开头丢掉多少帧数据*/
    jlstream_node_ioctl(recoder->stream, NODE_UUID_SWITCH, NODE_IOC_SET_PRIV_FMT, 0);
#endif

    //设置ADC的中断点数
    err = jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);
    if (err) {
        goto __exit1;
    }

    jlstream_node_ioctl(recoder->stream, NODE_UUID_ESCO_TX, NODE_IOC_SET_BTADDR, (int)bt_addr);
    //设置源节点是哪个
    u16 node_uuid = get_cvp_node_uuid();
    u32 ref_sr = audio_dac_get_sample_rate(&dac_hdl);
    if (node_uuid) {
#if !(TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE))
#if (TCFG_ESCO_DL_CVSD_SR_USE_16K > 1)
        jlstream_node_ioctl(recoder->stream, node_uuid, NODE_IOC_SET_FMT, (int)ref_sr);
#endif
#endif
        err = jlstream_node_ioctl(recoder->stream, node_uuid, NODE_IOC_SET_PRIV_FMT, source_uuid);
        if (err && (err != -ENOENT)) {	//兼容没有cvp节点的情况
            goto __exit1;
        }
    }

    if (link_type == JL_DOGLE_ACL) { //连接方式为ACL  msbc 编码需要用软件; aec 参考采样率为48000;
        enc_fmt.sw_hw_option = 1; //连接方式时ACL msbc 编码需要用软件
        //设置编码参数
        err = jlstream_node_ioctl(recoder->stream, NODE_UUID_ENCODER, NODE_IOC_SET_PRIV_FMT, (int)(&enc_fmt));
        //根据回音消除的类型，将配置传递到对应的节点
        if (node_uuid) {
            err = jlstream_node_ioctl(recoder->stream, node_uuid, NODE_IOC_SET_FMT, (int)ref_sr);
        }
        if (err && (err != -ENOENT)) {	//兼容没有节点的情况
            goto __exit1;
        }
    }


    jlstream_set_callback(recoder->stream, recoder->stream, esco_recoder_callback);
    jlstream_set_scene(recoder->stream, STREAM_SCENE_ESCO);
    err = jlstream_start(recoder->stream);
    if (err) {
        goto __exit1;
    }

    g_esco_recoder = recoder;

    return 0;

__exit1:
    jlstream_release(recoder->stream);
__exit0:
    free(recoder);
    return err;
}

void esco_recoder_close()
{
    struct esco_recoder *recoder = g_esco_recoder;

    if (!recoder) {
        return;
    }
    jlstream_stop(recoder->stream, 0);
    jlstream_release(recoder->stream);

    free(recoder);
    g_esco_recoder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"esco");
#if TCFG_AUDIO_DUT_ENABLE
    //退出通话默认设置为算法模式
    cvp_dut_mode_set(CVP_DUT_MODE_ALGORITHM);
#endif
}


extern bool esco_player_runing();
int esco_recoder_switch(u8 en)
{
    if (!esco_player_runing()) {
        printf("esco player close now, non-operational!");
        return 1;
    }
    if (en) {
        //固定LINK_SCO类型，dongle不跑switch
        u8 bt_addr[6];
        esco_player_get_btaddr(bt_addr);
        esco_recoder_open(COMMON_SCO, bt_addr);
    } else {
        esco_recoder_close();
    }
    return 0;
}

//esco_recoder复位流程，目前提供产测使用
int esco_recoder_reset(void)
{
    if (!esco_player_runing()) {
        printf("esco player close now, non-operational!");
        return 1;
    }
    u8 bt_addr[6];
    //产测流程，固定LINK_SCO类型
    esco_recoder_close();
    esco_player_get_btaddr(bt_addr);
    esco_recoder_open(COMMON_SCO, bt_addr);
    return 0;
}

#if TCFG_AUDIO_SIDETONE_ENABLE
static u8 sidetone_state = 1;
/*
*********************************************************************
*                  Audio Sidetone Open
* Description: 打开通话监听
* Arguments  : None.
* Return	 : 0成功 其他失败
* Note(s)    : None.
*********************************************************************
*/
int audio_sidetone_open(void)
{
    sidetone_state = 1;
    return 0;
}

/*
*********************************************************************
*                  Audio Sidetone Close
* Description: 关闭通话监听
* Arguments  : None.
* Return	 : 0成功 其他失败
* Note(s)    : None.
*********************************************************************
*/
int audio_sidetone_close(void)
{
    sidetone_state = 0;
    return 0;
}

int get_audio_sidetone_state()
{
    return sidetone_state;
}
#endif


