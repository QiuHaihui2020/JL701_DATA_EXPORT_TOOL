/*************************************************************************************************/
/*!
*  \file      local_tws_source_node.c
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "jlstream.h"
#include "tws/jl_tws.h"
#include "classic/tws_data_trans.h"
#include "classic/tws_api.h"
#include "codec/sbc_enc.h"
#include "effects/effects_adj.h"
#include "app_config.h"

#if TCFG_LOCAL_TWS_ENABLE

#define LOCAL_TWS_SEND_DEBUG    0

struct local_tws_param_data {
    u8 frame_num;
} __attribute__((packed));

struct local_tws_source_context {
    struct jl_tws_audio_format fmt;
    sbc_t sbc_fmt;
    int offset;
    u8 head_coding_type;
    u8 head_sample_rate;
    u8 head_frame_duration;
    u8 frame_num;
    int look_ahead_latency;
    void *packet;
    int packet_len;
    struct stream_frame *frame;
    u8 tws_channel;
    u8 send_start;
    u16 sleep;
    u32 frame_latency;
    u32 send_timestamp;
    struct stream_node *node;
    u32 timestamp;
    u32 tws_timestamp;
};

extern int tws_conn_system_clock_init(u8 factor);
extern u32 tws_conn_local_to_master_time(u32 usec);
extern u32 bt_audio_reference_clock_time(u8 network);
static void local_tws_source_ioc_stop(struct stream_iport *iport);

static int local_tws_pack_header(struct local_tws_source_context *ctx, void *data, struct stream_frame *frame)
{
    u32 tws_timestamp = 0;
    if (frame->flags & FRAME_FLAG_TIMESTAMP_ENABLE) {
        tws_timestamp = tws_conn_local_to_master_time((frame->timestamp - ctx->look_ahead_latency));
    }
    /*g_printf("-send : %u, %u, %u-\n", frame->timestamp, tws_timestamp, (bt_audio_reference_clock_time(1) * 625 * TIMESTAMP_US_DENOMINATOR));*/
    struct jl_tws_header header = {

        .timestamp = tws_timestamp,
        .frame_num = ctx->frame_num,
        .drift_sample_rate = frame->d_sample_rate,
    };

    memcpy(data, &header, sizeof(header));

    return sizeof(header);
}

/***********该函数用于释放已经发送成功的buf*************/
static void tws_api_data_trans_buf_free(struct local_tws_source_context *ctx)
{
    int tmp_len;
    void *frame_buf;
    while (1) {
        frame_buf = tws_api_data_trans_pop(ctx->tws_channel, &tmp_len);
        if (frame_buf) {
            tws_api_data_trans_free(ctx->tws_channel, frame_buf);
            /* r_printf("pop=%d\n", (u32)tmp_len); */
        } else {
            break;
        }
    }
}

static int local_tws_send_frame(struct local_tws_source_context *ctx, struct stream_frame *frame)
{
    int offset = 0;

    if (!ctx->tws_channel) {
        return frame->len;
    }

    if (!ctx->packet) {
        ctx->packet = tws_api_data_trans_buf_alloc(ctx->tws_channel, ctx->packet_len);
        if (!ctx->packet) {
            printf("SEND PACKET ERROR\n");
            goto __exit;
        }
        ctx->offset = local_tws_pack_header(ctx, ctx->packet, frame);
    }

    if (frame->len > ctx->packet_len - ctx->offset) {
        printf("LEN ERROR!!!!!!!!!!!!!! :%d,%d,%d,%d\n", __LINE__, frame->len, ctx->packet_len, ctx->offset);
        return 0;
    }
    memcpy((u8 *)ctx->packet + ctx->offset, (u8 *)frame->data, frame->len);
    ctx->offset += frame->len;
    if (ctx->offset >= ctx->packet_len) {
#if LOCAL_TWS_SEND_DEBUG
        struct jl_tws_header header;
        memcpy(&header, ctx->packet, sizeof(header));
        g_printf("-send : %u, %d, %d-\n", header.timestamp, frame->len, ctx->packet_len);
#endif
        tws_api_data_trans_push(ctx->tws_channel, ctx->packet, ctx->offset);
        ctx->packet = NULL;
    }

__exit:
    tws_api_data_trans_buf_free(ctx);
    if (!ctx->send_start) {
        ctx->send_start = 1;
    }
    ctx->send_timestamp = frame->timestamp;

    return frame->len;
}

static void local_tws_source_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;
    struct stream_frame *frame;

    ctx->sleep = 0;

    if (note->state & NODE_STA_ENC_END) {
        local_tws_source_ioc_stop(iport);
    }

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }

        local_tws_send_frame(ctx, frame);

        jlstream_free_frame(frame);
    }

    if (note && (note->state & NODE_STA_DEC_PAUSE)) {
        if (ctx->offset && ctx->packet) {
            tws_api_data_trans_push(ctx->tws_channel, ctx->packet, ctx->offset);
            ctx->packet = NULL;
            int tmp_len;
            void *frame_buf = tws_api_data_trans_pop(ctx->tws_channel, &tmp_len);
            if (frame_buf) {
                tws_api_data_trans_free(ctx->tws_channel, frame_buf);
            }
        }
    }
    if (note && note->sleep) {
        ctx->sleep = note->sleep;
    }
}

static int local_tws_param_data_init(struct stream_node *node, u16 uuid, struct local_tws_param_data *data)
{
    struct cfg_info info  = {0};             //节点配置相关信息（参数存储的目标地址、配置项大小）
    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    int ret = !jlstream_read_form_node_info_base(mode_index, "tws_source", cfg_index, &info);
    int len = 0;
    struct local_tws_param_data enc_cfg = {0};
    if (ret) {
        len = jlstream_read_form_cfg_data(&info, (void *)&enc_cfg);
        if (len) {
            /* printf("frame_num:%d", enc_cfg.frame_num); */
            data->frame_num = enc_cfg.frame_num;
        }
    }
    if (len != sizeof(enc_cfg)) { //默认参数
        data->frame_num = 2;
    }
    return 0;
}

static int jla_look_ahead_pcm_frames(int sample_rate, int frame_dms)
{
    if (frame_dms == 100 || frame_dms == 50 || frame_dms == 25) {
        if (sample_rate == 44100) {
            return 48 * 25 / 10;
        }
        return sample_rate * 25 / 10000; /*48K采样率的look ahead延迟为2.5ms，返回值为2.5ms的采样数量*/
    } else if (frame_dms == 75) {
        if (sample_rate == 44100) {
            return 48 * 40 / 10;
        }
        return sample_rate * 40 / 10000; /*48K采样率@7.5ms的look ahead延迟为4ms，返回值为4ms的采样数量*/
    }
    return 0;
}

static u32 jla_frame_latency(int sample_rate, int frame_dms)
{
    if (sample_rate == 44100) {
        return (u32)frame_dms * 100 * 48000 / 44100;
    }
    return (u32)frame_dms * 100;
}

static int sbc_look_ahead_pcm_frames(u8 subbands)
{
    if (subbands == SBC_SB_8) {
        return 73;
    } else {
        return 37;
    }
}

static int local_tws_source_bind(struct stream_node *node, u16 uuid)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)node->private_data;

    if (!ctx) {
        return -ENOMEM;
    }
    struct local_tws_param_data params;

    ctx->node = node;
    local_tws_param_data_init(node, uuid, &params);

    ctx->frame_num = params.frame_num;
    return 0;
}

static void local_tws_source_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = local_tws_source_handle_frame;
}

static void local_tws_source_close_iport(struct stream_iport *iport)
{
}

static u8 local_tws_data_trans_open(struct local_tws_source_context *ctx)
{
    u8 local_tws_channel = 0;
    u8 args[8];
    /* tws_conn_tx_unsniff_req(); */
    local_tws_channel = tws_api_data_trans_open(0, TWS_DTC_LOCAL_MEDIA, 10 * 1024);
    printf("tws_api_data_trans_open=%x\n", local_tws_channel);
    if (local_tws_channel) {
        args[0] = ctx->fmt.channel == 1 ? JL_TWS_CH_MONO : JL_TWS_CH_STEREO;
        args[1] = ctx->head_sample_rate;
        args[2] = ctx->head_coding_type;
        args[3] = ctx->head_frame_duration;
        args[4] = ctx->fmt.bit_rate / 1000;
#if 0
        printf("send_tws_ch:%d", local_tws_channel);
        printf("send_channel_mod:%x", args[0]);
        printf("send_sr:%d", ctx->head_sample_rate);
        printf("send_coding_type:%d", ctx->head_coding_type);
        printf("send_durations:%d", ctx->head_frame_duration);
        printf("send_bit_rate:%d * 1000", ctx->fmt.bit_rate / 1000);
#endif
        int err = tws_api_data_trans_start(local_tws_channel, args, 5);
        printf("tws_api_data_trans_start=%x\n", err);
    }
    return local_tws_channel;
}

static int local_tws_source_ioc_start(struct stream_iport *iport)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;
    if (ctx->fmt.coding_type == AUDIO_CODING_SBC) {
        ctx->sbc_fmt.frequency = SBC_FREQ_44100;
        ctx->sbc_fmt.blocks = SBC_BLK_16;
        ctx->sbc_fmt.subbands = SBC_SB_8;
        ctx->sbc_fmt.mode = SBC_MODE_STEREO;
        ctx->sbc_fmt.allocation = 0;
        ctx->sbc_fmt.endian = SBC_LE;
        ctx->sbc_fmt.bitpool = 53;
    }
    ctx->head_coding_type = jl_tws_coding_type_id(ctx->fmt.coding_type);
    ctx->head_frame_duration = jl_tws_match_frame_duration(ctx->fmt.frame_dms);
    int frame_len = 0;
    if (ctx->fmt.coding_type == AUDIO_CODING_JLA) {
        frame_len = ctx->fmt.bit_rate / 8 * ctx->fmt.frame_dms / 1000 / 10 + 2;
    } else if (ctx->fmt.coding_type == AUDIO_CODING_SBC) {
        u8 subbands = ctx->sbc_fmt.subbands ? 8 : 4;
        u8 blocks = ((ctx->sbc_fmt.blocks) + 1) * 4;
        u8 channels = ctx->sbc_fmt.mode  == SBC_MODE_MONO ? 1 : 2;
        u8 joint = ctx->sbc_fmt.mode  == SBC_MODE_JOINT_STEREO ? 1 : 0;
        frame_len = 4 + ((4 * subbands * channels) >> 3);
        if (ctx->sbc_fmt.mode >= SBC_MODE_STEREO) {
            frame_len += (((joint ? subbands : 0) + blocks * ctx->sbc_fmt.bitpool) + 7) >> 3;
        } else {
            frame_len += ((blocks * channels * ctx->sbc_fmt.bitpool) + 7) >> 3;
        }
        printf("sbc_len = %d\n", frame_len);
    }
    ctx->packet_len = sizeof(struct jl_tws_header) + frame_len * ctx->frame_num;
    printf("packet : %d, %lu, %d\n", ctx->packet_len, sizeof(struct jl_tws_header), frame_len * ctx->frame_num);
    if (!ctx->tws_channel) {
        ctx->tws_channel = local_tws_data_trans_open(ctx);
        tws_conn_system_clock_init(TIMESTAMP_US_DENOMINATOR);
    }
    return 0;
}

static void local_tws_source_ioc_stop(struct stream_iport *iport)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;
    if (ctx->tws_channel) {
        int tmp_len;
        void *frame_buf = tws_api_data_trans_pop(ctx->tws_channel, &tmp_len);
        if (frame_buf) {
            tws_api_data_trans_free(ctx->tws_channel, frame_buf);
        }
        if (ctx->packet) {
            tws_api_data_trans_free(ctx->tws_channel, ctx->packet);
            ctx->packet = NULL;
        }
        tws_api_data_trans_close(ctx->tws_channel);
        ctx->tws_channel = 0;
    }

    ctx->send_start = 0;
}

static int local_tws_source_buffer_time(struct stream_iport *iport)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;

    if (ctx->sleep) {
        return ctx->sleep;
    }
    if (!ctx->tws_channel || !ctx->send_start) {
        return 0;
    }
    u32 current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
    u32 frame_latency_time = ctx->send_timestamp + ctx->frame_latency * TIMESTAMP_US_DENOMINATOR;
    int latency = (int)(frame_latency_time - current_time) / TIMESTAMP_US_DENOMINATOR;//播放时间戳减去当前延迟则为待播放的缓冲
    return latency / 100;
}

static int local_tws_source_ioc_fmt_nego(struct stream_iport *iport)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    int ret = NEGO_STA_ACCPTED;


    ctx->fmt.coding_type = in_fmt->coding_type;
    ctx->fmt.frame_dms = in_fmt->frame_dms;
    ctx->fmt.channel = (in_fmt->channel_mode == AUDIO_CH_LR ? 2 : 1);
    ctx->fmt.bit_rate = in_fmt->bit_rate;
#if 0
    printf("coding_type_id:%x", in_fmt->coding_type);
    printf("frame_dms:%d", in_fmt->frame_dms);
    printf("nch:%d", ctx->fmt.channel);
    printf("bit_rate:%d", ctx->fmt.bit_rate);
#endif

    if (in_fmt->sample_rate == 0) {
        ret = NEGO_STA_CONTINUE;
    } else {
        ctx->fmt.sample_rate = in_fmt->sample_rate;
        ctx->head_sample_rate = jl_tws_match_sample_rate(in_fmt->sample_rate);
        if (ctx->fmt.coding_type == AUDIO_CODING_JLA) {
            ctx->look_ahead_latency = (int)((u64)jla_look_ahead_pcm_frames(in_fmt->sample_rate, ctx->fmt.frame_dms) * TIMESTAMP_US_DENOMINATOR * 1000000 / in_fmt->sample_rate);
            ctx->frame_latency = jla_frame_latency(in_fmt->sample_rate, ctx->fmt.frame_dms);
        } else if (ctx->fmt.coding_type == AUDIO_CODING_SBC) {
            ctx->look_ahead_latency = (int)((u64)sbc_look_ahead_pcm_frames(ctx->sbc_fmt.subbands) * TIMESTAMP_US_DENOMINATOR * 1000000 / in_fmt->sample_rate);
            ctx->frame_latency = 128 * 1000000 / in_fmt->sample_rate;
        }
    }
    g_printf("local tws sample rate : %d, look_ahead_latency : %dus, frame latency : %dus\n", ctx->fmt.sample_rate, ctx->look_ahead_latency / TIMESTAMP_US_DENOMINATOR, ctx->frame_latency);

    return ret;
}

static int local_tws_source_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct local_tws_source_context *ctx = (struct local_tws_source_context *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        local_tws_source_open_iport(iport);
        break;
    case NODE_IOC_CLOSE_IPORT:
        local_tws_source_close_iport(iport);
        break;
    case NODE_IOC_START:
        local_tws_source_ioc_start(iport);
        if (ctx && ctx->fmt.coding_type == AUDIO_CODING_SBC) {
            jlstream_node_ioctl(jlstream_for_node(ctx->node), NODE_UUID_ENCODER,
                                NODE_IOC_SET_PARAM, (int)(&ctx->sbc_fmt));
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        local_tws_source_ioc_stop(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= local_tws_source_ioc_fmt_nego(iport);
        break;
    case NODE_IOC_GET_DELAY:
        return local_tws_source_buffer_time(iport);
    default:
        break;
    }
    return 0;
}

static void local_tws_source_release(struct stream_node *node)
{
}

REGISTER_STREAM_NODE_ADAPTER(local_tws_source_adapter) = {
    .name       = "local_tws_source",
    .uuid       = NODE_UUID_LOCAL_TWS_SOURCE,
    .bind       = local_tws_source_bind,
    .ioctl      = local_tws_source_ioctl,
    .release    = local_tws_source_release,
    .hdl_size	= sizeof(struct local_tws_source_context),
};
#endif
