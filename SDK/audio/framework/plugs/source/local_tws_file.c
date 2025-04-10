/*************************************************************************************************/
/*!
*  \file      localtws_file.c
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "source_node.h"
#include "reference_time.h"
#include "classic/tws_api.h"
#include "app_msg.h"
#include "tws/jl_tws.h"
#include "local_tws_player.h"
#include "app_config.h"

#if TCFG_LOCAL_TWS_ENABLE

#define USE_LOCAL_REFERENCE_TIME    1 // 0 - 直接使用时间戳的TWS参考时钟，1 - 转换为本地参考时钟

struct local_tws_file_handle {
    u8 start;
    u8 reference;
    u8 timestamp_enable;
    u8 tws_channel;
    struct stream_node *node;
    u8 *tws_frame;
    int tws_frame_len;
    struct local_tws_player_param param;
};

extern int tws_conn_system_clock_init(u8 factor); //待头文件整理好后删除此extern声明
extern u32 tws_conn_master_to_local_time(u32 usec);
extern u32 bt_audio_reference_clock_time(u8 network);
static enum stream_node_state local_tws_get_frame(void *file, struct stream_frame **pframe)
{
    struct local_tws_file_handle *hdl = (struct local_tws_file_handle *)file;
    struct stream_frame *frame;
    struct jl_tws_header header;
    u8 *tws_frame = NULL;
    int frame_len = 0;
    if (hdl->tws_channel) {
        tws_frame = tws_api_data_trans_pop(hdl->tws_channel, &frame_len);
        if (!tws_frame) {
            *pframe = NULL;
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }

        frame = jlstream_get_frame(hdl->node->oport, frame_len);
        if (frame == NULL) {
            *pframe = NULL;
            return NODE_STA_RUN;
        }

        memcpy(&header, tws_frame, sizeof(header));
        /*调试阶段需要使用该打印来debug为什么播放输出设备会没数据*/
        u32 current_time = (bt_audio_reference_clock_time(1) * 625 * TIMESTAMP_US_DENOMINATOR);
        int diff = header.timestamp - current_time;
        diff /= TIMESTAMP_US_DENOMINATOR;
        if (diff <= 12000) {
            r_printf("-rx : %u, %u, %dus-\n", header.timestamp, current_time, diff);
        }

        int head_offset = sizeof(struct jl_tws_header);
        memcpy(frame->data, tws_frame + head_offset, frame_len - head_offset);
        frame->len = frame_len - head_offset;

        tws_api_data_trans_free(hdl->tws_channel, tws_frame);

        if (header.timestamp) {
#if USE_LOCAL_REFERENCE_TIME
            frame->timestamp = tws_conn_master_to_local_time(header.timestamp);
#else
            frame->timestamp = header.timestamp;
#endif
            frame->d_sample_rate = header.drift_sample_rate;
            frame->flags |= FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP | FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE;
        }
        *pframe = frame;
    } else {
        *pframe = NULL;
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    return NODE_STA_RUN;

}

void local_tws_wake_jlstream_run(void *arg)
{

    struct local_tws_file_handle *hdl = (struct local_tws_file_handle *)arg;
    if (hdl) {
        if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
            jlstream_wakeup_thread(NULL, hdl->node, NULL);
        }
    }
}




static void *local_tws_file_init(void *priv, struct stream_node *node)
{
    struct local_tws_file_handle *hdl = (struct local_tws_file_handle *)zalloc(sizeof(struct local_tws_file_handle));

    hdl->node = node;

    return hdl;
}

static int local_tws_file_set_bt_addr(struct local_tws_file_handle *hdl, void *bt_addr)
{
    hdl->tws_channel = (u8)bt_addr;

    return 0;
}

static int local_tws_file_get_fmt(struct local_tws_file_handle *hdl, struct stream_fmt *fmt)
{
#if 1
    if (!hdl->tws_channel) {
        r_printf("no tws channel.\n");
        return -EAGAIN;
    }
    fmt->coding_type = jl_tws_coding_type(hdl->param.coding_type);
    fmt->sample_rate = jl_tws_sample_rate(hdl->param.sample_rate);
    fmt->frame_dms = jl_tws_frame_duration(hdl->param.durations);
    fmt->channel_mode = hdl->param.channel_mode == JL_TWS_CH_MONO ? AUDIO_CH_MIX : AUDIO_CH_LR;
    fmt->bit_rate = hdl->param.bit_rate * 1000;
#else
    fmt->coding_type = AUDIO_CODING_JLA;
    fmt->sample_rate = 44100;
    fmt->frame_dms = 100;
    fmt->channel_mode = AUDIO_CH_LR;
    fmt->bit_rate = 128000;
#endif
    g_printf("fmt : 0x%x, %d, %d, %d, %d\n", fmt->coding_type, fmt->sample_rate, fmt->frame_dms, fmt->channel_mode, fmt->bit_rate);

    return 0;
}


static int local_tws_file_start(struct local_tws_file_handle *hdl)
{
    if (hdl->start) {
        return 0;
    }

    hdl->start = 1;

    if (hdl->tws_channel) {
        tws_api_data_trans_rx_notify_register(hdl->tws_channel, local_tws_wake_jlstream_run, hdl);
    }

    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        return 0;
    }

    hdl->timestamp_enable = 1;
#if USE_LOCAL_REFERENCE_TIME
    tws_conn_system_clock_init(TIMESTAMP_US_DENOMINATOR);
#else
    hdl->reference = audio_reference_clock_select((void *)((u32)hdl->tws_channel), 1);
#endif
    return 0;
}

static int local_tws_file_stop(struct local_tws_file_handle *hdl)
{
    if (hdl->start) {
        tws_api_data_trans_rx_notify_unregister(hdl->tws_channel);
#if USE_LOCAL_REFERENCE_TIME == 0
        if (hdl->reference) {
            audio_reference_clock_exit(hdl->reference);
        }
#endif
        hdl->start = 0;
    }

    return 0;
}

static int local_tws_file_ioctl(void *file, int cmd, int arg)
{
    int err = 0;
    struct local_tws_file_handle *hdl = (struct local_tws_file_handle *)file;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        local_tws_file_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_SET_FMT:
        memcpy(&hdl->param, (struct local_tws_player_param *)arg, sizeof(struct local_tws_player_param));
        break;
    case NODE_IOC_GET_FMT:
        err = local_tws_file_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_START:
        local_tws_file_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        local_tws_file_stop(hdl);
        break;
    }

    return err;
}

static void local_tws_file_release(void *file)
{
    struct local_tws_file_handle *hdl = (struct local_tws_file_handle *)file;

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(local_tws_file_plug) = {
    .uuid       = NODE_UUID_LOCAL_TWS_SINK,
    .init       = local_tws_file_init,
    .get_frame  = local_tws_get_frame,
    .ioctl      = local_tws_file_ioctl,
    .release    = local_tws_file_release,
};

#endif
