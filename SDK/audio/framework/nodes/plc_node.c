#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".plc_node.data.bss")
#pragma data_seg(".plc_node.data")
#pragma const_seg(".plc_node.text.const")
#pragma code_seg(".plc_node.text")
#endif
#include "jlstream.h"
#include "audio_plc.h"
#include "tech_lib/LFaudio_plc_api.h"
#include "app_config.h"
#include "effects/effects_adj.h"
#include "media/audio_splicing.h"

#if TCFG_PLC_NODE_ENABLE

#define TCFG_MUSIC_PLC_ENABLE	1

enum audio_plc_type {
    AUD_PLC_BYPASS = 0, //不做任何处理，但是仍然会有缓存;
    AUD_PLC_NORMAL,     //仅修复
    AUD_PLC_WITH_FADE   //修复的同时做淡入淡出的处理
};

extern int tws_api_get_low_latency_state();
struct plc_node_hdl {
    u8 start;
    u8 channel_num;
    enum stream_scene scene;	//当前场景
#if TCFG_MUSIC_PLC_ENABLE
    struct music_plc *plc;
#endif
    void *esco_plc;
    char *out_buf;
    u16 out_buf_len;
    struct node_port_data_wide data_wide;
    u8 Qval;//数据的饱和位宽
};
struct music_plc {
    LFaudio_PLC_API *plc_ops;
    void *plc_mem;
    af_DataType datatype;
};

struct music_plc *music_plc_open(struct plc_node_hdl *hdl, u32 sr, u8 ch_num)
{

    struct music_plc *plc = zalloc(sizeof(struct music_plc));
    if (plc) {
        plc->datatype.IndataBit  = hdl->data_wide.iport_data_wide;
        plc->datatype.OutdataBit = hdl->data_wide.oport_data_wide;
        plc->datatype.IndataInc  = (ch_num == 2) ? 2 : 1;
        plc->datatype.OutdataInc = (ch_num == 2) ? 2 : 1;
        plc->plc_ops = get_lfaudioPLC_api();
        int plc_mem_size = plc->plc_ops->need_buf(ch_num, &plc->datatype); // 3660bytes，请衡量是否使用该空间换取PLC处理
        plc->plc_mem = malloc(plc_mem_size);
        if (!plc->plc_mem) {
            plc->plc_ops = NULL;
            free(plc);
            return NULL;
        }
        plc->plc_ops->open(plc->plc_mem, ch_num, sr, tws_api_get_low_latency_state() ? 4 : 0, &plc->datatype); //4是延时最低 16个点
    }
    return plc;
}


void music_plc_run(struct music_plc *plc, s16 *data, u16 len, u8 repair)
{
    if (plc && plc->plc_ops) {
        u16 point_offset = plc->datatype.IndataBit ? 2 : 1;
        u16 plc_type = repair ? AUD_PLC_WITH_FADE : AUD_PLC_BYPASS;
        plc->plc_ops->run(plc->plc_mem, data, data, len >> point_offset, plc_type);
    }
}

void music_plc_set_bit_wide(struct music_plc *plc, struct node_port_data_wide data_wide)
{
    //TODO
}
void music_plc_close(struct music_plc *plc)
{
    if (plc) {
        if (plc->plc_mem) {
            free(plc->plc_mem);
            plc->plc_mem = NULL;
        }
        free(plc);
    }
}
/*
 *通话plc只支持单声道
 * */
void *esco_plc_open(struct plc_node_hdl *hdl, u32 sr, u8 ch_num)
{
    hdl->channel_num = ch_num;
    af_DataType dataTypeobj = {0};
    dataTypeobj.IndataBit = hdl->data_wide.iport_data_wide;
    dataTypeobj.OutdataBit = hdl->data_wide.oport_data_wide;
    dataTypeobj.IndataInc = 1;
    dataTypeobj.OutdataInc = 1;
    dataTypeobj.Qval = hdl_node(hdl)->oport->fmt.Qval;
    void *esco_plc = audio_plc_open(sr, 1, &dataTypeobj);
    return esco_plc;
}

void esco_plc_run(struct plc_node_hdl *hdl, s16 *data, u16 len, u8 repair_flag)
{
    if (!hdl->esco_plc) {
        return;
    }
    u8 ch_num = hdl->channel_num;
    u8 bit_wide = hdl->data_wide.oport_data_wide;
    if (ch_num == 2) {
        if (hdl->out_buf_len < len / 2) {
            if (hdl->out_buf) {
                free(hdl->out_buf);
                hdl->out_buf = NULL;
            }
        }
        if (!hdl->out_buf) {
            hdl->out_buf = malloc(len / 2);
            hdl->out_buf_len = len / 2;
        }
        if (hdl->out_buf) {
            if (bit_wide) {
                if (config_media_24bit_enable) {
                    pcm_dual_to_single_32bit(hdl->out_buf, data, len);
                    audio_plc_run(hdl->esco_plc, (s16 *)hdl->out_buf, len / 2, repair_flag);
                    pcm_single_to_dual_32bit(data, hdl->out_buf, len / 2);
                }
            } else {
                pcm_dual_to_single(hdl->out_buf, data, len);
                audio_plc_run(hdl->esco_plc, (s16 *)hdl->out_buf, len / 2, repair_flag);
                pcm_single_to_dual(data, hdl->out_buf, len / 2);
            }
        }
    } else {
        audio_plc_run(hdl->esco_plc, (s16 *)data, len, repair_flag);
    }
}

void esco_plc_close(void *esco_plc)
{
    if (!esco_plc) {
        return;
    }

    audio_plc_close(esco_plc);
}

static void plc_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;
    struct plc_node_hdl *hdl = (struct plc_node_hdl *)iport->node->private_data;
    u8 repair_flag = 0;

    frame = jlstream_pull_frame(iport, note);
    if (!frame) {
        return;
    }
    int flag = FRAME_FLAG_FILL_PACKET & ~FRAME_FLAG_FLUSH_OUT;
    if ((frame->flags & flag) == flag) {
        repair_flag = 1;
    }
    if (hdl->scene == STREAM_SCENE_ESCO || hdl->scene == STREAM_SCENE_SPDIF) {
        esco_plc_run(hdl, (s16 *)frame->data, frame->len, repair_flag);
    } else {
#if TCFG_MUSIC_PLC_ENABLE
        music_plc_run(hdl->plc, (s16 *)frame->data, frame->len, repair_flag);
#endif
    }
    if (node->oport) {
        jlstream_push_frame(node->oport, frame);
    } else {
        jlstream_free_frame(frame);
    }
}

/*节点预处理-在ioctl之前*/
static int plc_adapter_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

/*打开改节点输入接口*/
static void plc_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = plc_handle_frame;
}


/*节点start函数*/
static void plc_ioc_start(struct plc_node_hdl *hdl, u32 sr, u8 ch_num)
{

    hdl->data_wide.iport_data_wide = hdl_node(hdl)->iport->prev->fmt.bit_wide;
    hdl->data_wide.oport_data_wide = hdl_node(hdl)->oport->fmt.bit_wide;
    /*log_d("%s bit_wide, %d %d %d\n", __FUNCTION__, hdl->data_wide.iport_data_wide, hdl->data_wide.oport_data_wide, hdl_node(hdl)->oport->fmt.Qval);*/
    if (hdl->scene == STREAM_SCENE_ESCO || hdl->scene == STREAM_SCENE_SPDIF) {
        hdl->esco_plc = esco_plc_open(hdl, sr, ch_num);
    } else {
#if TCFG_MUSIC_PLC_ENABLE
        hdl->plc = music_plc_open(hdl, sr,  ch_num);
#endif
    }
}

/*节点stop函数*/
static void plc_ioc_stop(struct plc_node_hdl *hdl)
{
    if (hdl->scene == STREAM_SCENE_ESCO || hdl->scene == STREAM_SCENE_SPDIF) {
        if (hdl->esco_plc) {
            esco_plc_close(hdl->esco_plc);
            hdl->esco_plc = NULL;
        }
        if (hdl->out_buf) {
            free(hdl->out_buf);
            hdl->out_buf = NULL;
        }
    } else {
#if TCFG_MUSIC_PLC_ENABLE
        if (hdl->plc) {
            music_plc_close(hdl->plc);
            hdl->plc = NULL;
        }
#endif

    }
}

/*节点ioctl函数*/
static int plc_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct plc_node_hdl *hdl = (struct plc_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        plc_ioc_open_iport(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = (enum stream_scene)arg;
        break;
    case NODE_IOC_START:
        plc_ioc_start(hdl, iport->prev->fmt.sample_rate, AUDIO_CH_NUM(iport->prev->fmt.channel_mode));
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        plc_ioc_stop(hdl);
        break;
    }
    return ret;
}

/*节点用完释放函数*/
static void plc_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(plc_node_adapter) = {
    .name       = "plc",
    .uuid       = NODE_UUID_PLC,
    .bind       = plc_adapter_bind,
    .ioctl      = plc_adapter_ioctl,
    .release    = plc_adapter_release,
    .hdl_size   = sizeof(struct plc_node_hdl),
};

#endif

