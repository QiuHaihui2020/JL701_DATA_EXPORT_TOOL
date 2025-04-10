#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".a2dp_file.data.bss")
#pragma data_seg(".a2dp_file.data")
#pragma const_seg(".a2dp_file.text.const")
#pragma code_seg(".a2dp_file.text")
#endif
#include "btstack/a2dp_media_codec.h"
#include "btstack/avctp_user.h"
#include "source_node.h"
#include "classic/tws_api.h"
#include "system/timer.h"
#include "sync/audio_syncts.h"
#include "media/bt_audio_timestamp.h"
#include "os/os_api.h"
#include "jiffies.h"
#include "a2dp_streamctrl.h"
#include "reference_time.h"
#include "effects/effects_adj.h"
#include "app_config.h"
#include "media/codec/sbc_codec.h"

#define A2DP_TIMESTAMP_ENABLE       1

struct a2dp_file_params {
    u8 edr_to_local_time;
} __attribute__((packed));

struct a2dp_file_hdl {
    u8 start;
    /*u8 reassemble;*/
    u16 timer;
    u16 wake_up_timer;
    void *file;
    int media_type;
    struct stream_node *node;

    void *ts_handle;
    u32 sample_rate;
    u16 codec_version;
    u8 channel_num;
    u16 seqn;
    u32 base_time;
    u32 timestamp;
    u32 ts_sample_rate;
    u32 dts;//total frams

    u16 delay_time;
    u8 sync_step;
    u8 reference;
    struct a2dp_media_frame frame;
    int frame_len;
    void *stream_ctrl;
    u8 bt_addr[6];
    u8 tws_case;
    u8 handshake_state;
    u32 request_timeout;
    u32 handshake_timeout;
    /*struct stream_frame *reassembled_frame;*/

    u8 link_jl_dongle; //连接jl_dongle
    u8 rtp_ts_en; //使用rtp的时间戳
    u16 jl_dongle_latency ;
    u8 edr_to_local_time;
    u8 timestamp_enable;
    u32 ts_align_time;//统计时间戳对齐动作的耗时
};

extern const uint32_t CONFIG_A2DP_DELAY_TIME_AAC;
extern const uint32_t CONFIG_A2DP_DELAY_TIME_SBC;
extern const int CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
extern const int CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE;
extern const int CONFIG_A2DP_DELAY_TIME_LO;
extern const int CONFIG_A2DP_SBC_DELAY_TIME_LO;
extern const int CONFIG_BTCTLER_TWS_ENABLE;
extern const int CONFIG_DONGLE_SPEAK_ENABLE;

extern void bt_audio_reference_clock_select(void *addr, u8 network);
extern u32 bt_audio_reference_clock_time(u8 network);
extern int a2dp_get_packet_pcm_frames(struct a2dp_file_hdl *hdl, u8 *data, int len);
static int a2dp_stream_ts_enable_detect(struct a2dp_file_hdl *hdl, u8 *packet, int *drop);
static void a2dp_frame_pack_timestamp(struct a2dp_file_hdl *hdl, struct stream_frame *frame, u8 *data, int pcm_frames);
static void a2dp_file_timestamp_setup(struct a2dp_file_hdl *hdl);

extern void bt_edr_conn_system_clock_init(void *addr, u8 factor);
extern u32 bt_edr_conn_master_to_local_time(void *addr, u32 usec);

static u8 a2dp_low_latency = 0;

#define msecs_to_bt_time(m)     (((m + 1)* 1000) / 625)
#define a2dp_seqn_before(a, b)  ((a < b && (u16)(b - a) < 1000) || (a > b && (u16)(a - b) > 1000))
#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])
#define RB32(b)    (u32)(((u8 *)b)[0] << 24 | (((u8 *)b))[1] << 16 | (((u8 *)b))[2] << 8 | (((u8 *)b))[3])

#include "a2dp_handshake.c"
/*#include "a2dp_aac_demuxer.c"*/

void a2dp_file_low_latency_enable(u8 enable)
{
    a2dp_low_latency = enable;
}

u8 a2dp_file_get_low_latency_status(void)
{
    return a2dp_low_latency;
}

static void abandon_a2dp_data(void *p)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)p;
    struct a2dp_media_frame _frame;

    if (!hdl->file) {
        return;
    }
    //默认剩余1包数据，避免格式检查取不到数据
    while ((a2dp_media_get_packet_num(hdl->file) > 1) && (a2dp_media_try_get_packet(hdl->file, &_frame) > 0)) {
        a2dp_media_free_packet(hdl->file, _frame.packet);
    }
    /*a2dp_media_clear_packet_before_seqn(hdl->file, 0);*/
}

static void a2dp_file_start_abandon_data(struct a2dp_file_hdl *hdl)
{
    int role = TWS_ROLE_MASTER;

    /*if (CONFIG_BTCTLER_TWS_ENABLE) {
        role = tws_api_get_role();
    }*/
    if (role == TWS_ROLE_MASTER) {
        if (hdl->timer == 0) {
            abandon_a2dp_data(hdl); //启动丢包时清一下缓存;
            hdl->timer = sys_timer_add(hdl, abandon_a2dp_data, 50);
            puts("start_abandon_a2dp_data\n");
        }
    }
}

static void a2dp_file_stop_abandon_data(struct a2dp_file_hdl *hdl)
{
    if (hdl->timer) {
        abandon_a2dp_data(hdl); //停止丢数据清一下缓存
        sys_timer_del(hdl->timer);
        hdl->timer = 0;
    }
}


static void a2dp_source_wake_jlstream_run(void *_hdl)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}

static void a2dp_source_direct_wake_jlstream_run(void *_hdl)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;
    if (hdl) {
        if (hdl->node) {
            jlstream_wakeup_thread(NULL, hdl->node, NULL);
        }
        hdl->ts_align_time += 4;
    }
}

static enum stream_node_state a2dp_get_frame(void *_hdl, struct stream_frame **pframe)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;
    struct a2dp_media_frame _frame;
    int drop = 0;
    int stream_error = 0;

    *pframe = NULL;

#if A2DP_TIMESTAMP_ENABLE
    if (!hdl->rtp_ts_en && !hdl->ts_handle) {
        int err = a2dp_tws_media_handshake(hdl);
        if (hdl->timestamp_enable && err) {
            if (!hdl->wake_up_timer) {//快速唤醒数据流，加速tws时间戳交互的过程
                hdl->ts_align_time = 0;
                hdl->wake_up_timer = sys_hi_timer_add((void *)hdl, a2dp_source_direct_wake_jlstream_run, 4);
            }
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        log_d(">>>>handshake time %d ms<<<<\n", hdl->ts_align_time);
        if (hdl->wake_up_timer) {
            sys_hi_timer_del(hdl->wake_up_timer);
            hdl->wake_up_timer = 0;
        }
        a2dp_file_timestamp_setup(hdl);
    }
#endif
    if ((!hdl->ts_handle /* || hdl->edr_to_local_time */) && hdl->start == 0) {
        int delay = a2dp_media_get_remain_play_time(hdl->file, 1);
        if (delay < (hdl->ts_handle ? hdl->delay_time : 300)) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        hdl->start = 1;
    }

    int len = hdl->frame_len;
    if (len == 0) {
        if (hdl->stream_ctrl) {
            stream_error = a2dp_stream_control_pull_frame(hdl->stream_ctrl, &_frame, &len);
        } else {
            len = a2dp_media_try_get_packet(hdl->file, &_frame);
        }
        if (len <= 0) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        }
        hdl->frame_len = len;
        memcpy(&hdl->frame, &_frame, sizeof(struct a2dp_media_frame));
    } else {
        memcpy(&_frame, &hdl->frame, sizeof(struct a2dp_media_frame));
    }

    hdl->seqn = RB16((u8 *)_frame.packet + 2);
    int err = a2dp_stream_ts_enable_detect(hdl, _frame.packet, &drop);
    if (err) {
        if (drop) {
            if (hdl->stream_ctrl) {
                a2dp_stream_control_free_frame(hdl->stream_ctrl, &_frame);
            } else {
                a2dp_media_free_packet(hdl->file, _frame.packet);
            }
            hdl->frame_len = 0;
        }
        a2dp_tws_media_try_handshake_ack(0, hdl->seqn);
        if (!hdl->wake_up_timer) {//快速唤醒数据流，加速tws时间戳交互的过程
            hdl->ts_align_time = 0;
            hdl->wake_up_timer = sys_hi_timer_add((void *)hdl, a2dp_source_direct_wake_jlstream_run, 4);
        }
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }
    if (hdl->wake_up_timer) {
        sys_hi_timer_del(hdl->wake_up_timer);
        hdl->wake_up_timer = 0;
    }

    int head_len = a2dp_media_get_rtp_header_len(hdl->media_type, _frame.packet, len);

    struct stream_frame *frame;
    int frame_len = len - head_len;
    frame = jlstream_get_frame(hdl->node->oport, frame_len);
    if (frame == NULL) {
        return NODE_STA_RUN;
    }
    frame->len          = frame_len;
    frame->timestamp    = _frame.clkn;
    frame->flags        |= (stream_error);
    a2dp_frame_pack_timestamp(hdl, frame, _frame.packet + 4,  //时间戳的地址
                              a2dp_get_packet_pcm_frames(hdl,
                                      _frame.packet + head_len, frame_len));

    a2dp_tws_media_try_handshake_ack(1, hdl->seqn);

    memcpy(frame->data, _frame.packet + head_len, frame_len);

    if (hdl->stream_ctrl) {
        a2dp_stream_control_free_frame(hdl->stream_ctrl, &_frame);
    } else {
        a2dp_media_free_packet(hdl->file, _frame.packet);
    }
    hdl->frame_len = 0;
    hdl->start = 1;

    ASSERT(frame);
    *pframe = frame;

    return NODE_STA_RUN;
}

static void *a2dp_init(void *priv, struct stream_node *node)
{
    struct a2dp_file_hdl *hdl = zalloc(sizeof(*hdl));
    hdl->node = node;
    u16 plug_uuid = get_source_node_plug_uuid(priv);

    struct a2dp_file_params params = {0};
    jlstream_read_node_data_by_cfg_index(plug_uuid, hdl->node->subid, 0, (void *)&params, NULL);
    hdl->edr_to_local_time = params.edr_to_local_time;
    return hdl;
}

static const u32 aac_sample_rates[] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000,
};

static const u16 sbc_sample_rates[] = {16000, 32000, 44100, 48000};

static const u32 ldac_sample_rates[] = {44100, 48000, 88200, 96000};

static int a2dp_ioc_get_fmt(struct a2dp_file_hdl *hdl, struct stream_fmt *fmt)
{
    struct a2dp_media_frame _frame;
    int type = a2dp_media_get_codec_type(hdl->file);
    char *code_type;
    switch (type) {
    case A2DP_CODEC_SBC:
        fmt->coding_type = AUDIO_CODING_SBC;
        code_type = "SBC";
        break;
#if (defined(TCFG_BT_SUPPORT_AAC) && TCFG_BT_SUPPORT_AAC)
    case A2DP_CODEC_MPEG24:
        fmt->coding_type = AUDIO_CODING_AAC;
        code_type = "AAC";
        break;
#endif
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
    case A2DP_CODEC_LDAC:
        fmt->coding_type = AUDIO_CODING_LDAC;
        code_type = "LDAC";
        break;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC_V5) && TCFG_BT_SUPPORT_LHDC_V5)
    case A2DP_CODEC_LHDC_V5: //LHDC 直接从蓝牙获取格式信息。
        fmt->coding_type = AUDIO_CODING_LHDC_V5;
        fmt->sample_rate = a2dp_media_get_sample_rate(hdl->file);
        fmt->dec_bit_wide = a2dp_media_get_bit_wide(hdl->file);
        fmt->codec_version = a2dp_media_get_codec_version(hdl->file);
        fmt->channel_mode = AUDIO_CH_LR;
        hdl->media_type = type;
        hdl->codec_version = fmt->codec_version;
        hdl->sample_rate = fmt->sample_rate;
        hdl->channel_num = (fmt->channel_mode == AUDIO_CH_LR) ? 2 : 1;

        printf("a2dp  format %s, sample_rate %d, bit_wide %d, codec_version %d\n",
               "LHDC_v5", hdl->sample_rate, fmt->dec_bit_wide, fmt->codec_version);
        return 0;
        break;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
    case A2DP_CODEC_LHDC: //LHDC 直接从蓝牙获取格式信息。
        fmt->coding_type = AUDIO_CODING_LHDC;
        fmt->sample_rate = a2dp_media_get_sample_rate(hdl->file);
        fmt->dec_bit_wide = a2dp_media_get_bit_wide(hdl->file);
        fmt->codec_version = a2dp_media_get_codec_version(hdl->file);
        fmt->channel_mode = AUDIO_CH_LR;
        hdl->media_type = type;
        hdl->codec_version = fmt->codec_version;
        hdl->sample_rate = fmt->sample_rate;
        hdl->channel_num = (fmt->channel_mode == AUDIO_CH_LR) ? 2 : 1;

        printf("a2dp  format %s, sample_rate %d, bit_wide %d, codec_version %s\n",
               "LHDC", hdl->sample_rate, fmt->dec_bit_wide, ((fmt->codec_version == 500) ? "LLAC" : "LHDC V3/V4"));
        return 0;
#endif
    default:
        code_type = "unknown";
        break;
    }

    if (hdl->sample_rate) {
        fmt->sample_rate = hdl->sample_rate;
        fmt->channel_mode = hdl->channel_num == 2 ? AUDIO_CH_LR : AUDIO_CH_MIX;
        return 0;
    }
    hdl->media_type = type;

__again:
    int len = a2dp_media_try_get_packet(hdl->file, &_frame);
    if (len <= 0) {
        return -EAGAIN;
    }
    u8 *packet = _frame.packet;

    int head_len = a2dp_media_get_rtp_header_len(type, packet, len);
    if (head_len >= len) {
        a2dp_media_free_packet(hdl->file, packet);
        goto __again;
    }
    /*put_buf(packet, head_len + 8);*/
    u8 *frame = packet + head_len;
    if (frame[0] == 0x47) {    				//常见mux aac格式
#if (defined(TCFG_BT_SUPPORT_AAC) && TCFG_BT_SUPPORT_AAC)
        u8 sr = (frame[5] & 0x3C) >> 2;
        /* u8 ch = ((frame[5] & 0x3) << 2) | ((frame[6] & 0xC0) >> 6); */
        fmt->channel_mode = AUDIO_CH_LR;
        fmt->sample_rate  = aac_sample_rates[sr];
    } else if (frame[0] == 0x20) {			//特殊LATM aac格式
        u8 sr = ((frame[2] & 0x7) << 1) | ((frame[3] & 0x80) >> 7);
        /* u8 ch = ((frame[3] & 0x78) >> 3) ; */
        fmt->channel_mode = AUDIO_CH_LR;
        fmt->sample_rate = aac_sample_rates[sr];
#endif
    } else if (frame[0] == 0x9C) {          //sbc 格式
        /*
         * 检查数据是否为AAC格式,
         * 可以切换AAC和SBC格式的手机可能切换后数据格式和type不对应
         */
        head_len = a2dp_media_get_rtp_header_len(A2DP_CODEC_MPEG24, packet, len);
        if (head_len < len) {
            if (packet[head_len] == 0x47 || packet[head_len] == 0x20) {
                a2dp_media_free_packet(hdl->file, packet);
                goto __again;
            }
        }

        u8 sr = (frame[1] >> 6) & 0x3;
        u8 ch = (frame[1] >> 2) & 0x3;
        if (ch == 0) {
            fmt->channel_mode = AUDIO_CH_MIX;
        } else {
            fmt->channel_mode = AUDIO_CH_LR;
        }
        fmt->sample_rate  = sbc_sample_rates[sr];
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
    } else if (frame[1] == 0xAA) {
        u8 sr = (frame[2] >> (8 - 3)) & 0x7;
        int chconfig_id = (frame[2] >> (8 - 5)) & 0x03;

        fmt->channel_mode = AUDIO_CH_LR;
        fmt ->sample_rate = ldac_sample_rates[sr];
        fmt->chconfig_id = chconfig_id;
        //printf(" %x  %x  %x\n",frame[0],frame[1],frame[2]);
        //printf("sr:%d, sample_rate : %d  chconfig_id : %d\n",sr,fmt->sample_rate,chconfig_id);
#endif
    } else {
        /*
         * 小米8手机先播sbc,暂停后切成AAC格式点播放,有时第一包数据还是sbc格式
         * 导致这里获取头信息错误
         */
        a2dp_media_free_packet(hdl->file, packet);
        goto __again;
    }
    a2dp_media_put_packet(hdl->file, packet);

    hdl->sample_rate = fmt->sample_rate;
    hdl->channel_num = (fmt->channel_mode == AUDIO_CH_LR) ? 2 : 1;
    printf("a2dp %d, format %s\n", hdl->sample_rate, code_type);
    return 0;
}



static int a2dp_ioc_set_bt_addr(struct a2dp_file_hdl *hdl, u8 *bt_addr)
{
    hdl->file = a2dp_open_media_file(bt_addr);
    if (!hdl->file) {
        printf("open_file_faild\n");
        put_buf(bt_addr, 6);
        return -EINVAL;
    }
    memcpy(hdl->bt_addr, bt_addr, 6);

    if (CONFIG_DONGLE_SPEAK_ENABLE) {
        if (btstack_get_dev_type_for_addr(hdl->bt_addr) == REMOTE_DEV_DONGLE_SPEAK) {
            hdl->link_jl_dongle = 1;
            hdl->jl_dongle_latency = CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
            if (!CONFIG_JL_DONGLE_PLAYBACK_DYNAMIC_LATENCY_ENABLE) {
                hdl->rtp_ts_en = 1;
            }
        }
    }

    return 0;
}

static void a2dp_ioc_stop(struct a2dp_file_hdl *hdl)
{
    if (hdl->wake_up_timer) {
        sys_hi_timer_del(hdl->wake_up_timer);
        hdl->wake_up_timer = 0;
    }
    /*hdl->sample_rate = 0;*/
    if (hdl->frame_len) {
        if (hdl->stream_ctrl) {
            a2dp_stream_control_free_frame(hdl->stream_ctrl, &hdl->frame);
        } else {
            a2dp_media_free_packet(hdl->file, hdl->frame.packet);
        }
        hdl->frame_len = 0;
    }
    a2dp_file_stop_abandon_data(hdl);
    /*a2dp_media_clear_packet_before_seqn(hdl->file, 0);*/
    a2dp_media_stop_play(hdl->file);
    hdl->start = 0;
}

static int sbc_get_packet_pcm_frames(u8 *data, int len)
{
    u8 subbands = (data[1] & 0x01) ? 8 : 4;
    u8 blocks = (((data[1] >> 4) & 0x03) + 1) * 4;
    int  frame_num = sbc_audio_frame_num_get(data, len);
    /* printf("frame_num = %d\n",frame_num); */
    return (frame_num) * (blocks * subbands);
}

static int lhdc_get_packet_pcm_frames(void *_hdl, u8 *data, int len, int unit)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;
    int point = 0;

    u8 frame_num = (data[0] & 0x3C) >> 2;

    //目前测试LHDC 每帧输出点数是这样的。如果存在兼容性问题。这里需要同步兼容
    if (hdl->codec_version == 400) { //lhdc_v4
        point = 256;
    } else if (hdl->codec_version == 500) {
        /* if(hdl->sample_rate == 48000){ */
        /* point = 240 ; */
        /* }else if(hdl->sample_rate == 44100){ */
        /* point = 220; */
        /* } */
        point = hdl->sample_rate / 1000 * 5; //5ms 的数据量
    } else if (hdl->codec_version == 300) { //lhdc_v3
        point = 256;
    }

    return frame_num * point * (unit);

}

static int lhdc_v5_get_packet_pcm_frames(void *_hdl, u8 *data, int len, int unit)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;
    int point = 0;

    u8 frame_num = (data[0] & 0x3C) >> 2;
    u32 sample_rate = hdl->sample_rate;
    //目前测试LHDC 每帧输出点数是这样的。如果存在兼容性问题。这里需要同步兼容
    if (sample_rate == 44100) { //lhdc_v4
        point = 240;
    } else {
        point = sample_rate / 1000 * 5; //5ms 的数据量
    }
    return frame_num * point * (unit);
}

static int a2dp_get_packet_pcm_frames(struct a2dp_file_hdl *hdl, u8 *data, int len)
{
    u8 unit = 1;
    u32 frames = 0;
    u8 codec_type = hdl->media_type;

    switch (hdl->media_type) {
    case A2DP_CODEC_SBC:
        frames = sbc_get_packet_pcm_frames(data, len);//frame_num * 128 * (unit);
        break;
    case A2DP_CODEC_MPEG24:
        u32 audio_mux_element = 0xffffffff;
        memcpy(&audio_mux_element, data, len >= sizeof(audio_mux_element) ? sizeof(audio_mux_element) : len);
        if (audio_mux_element == 0xFC47 || audio_mux_element == 0x10120020) {
            frames = 1024 * (unit);
        }
        break;
#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
    case A2DP_CODEC_LHDC:
        frames = lhdc_get_packet_pcm_frames(hdl, data, len, unit);//frame_num * point * (unit);
        break;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC_V5) && TCFG_BT_SUPPORT_LHDC_V5)
    case A2DP_CODEC_LHDC_V5:
        frames = lhdc_v5_get_packet_pcm_frames(hdl, data, len, unit);//frame_num * point * (unit);
        break;
#endif
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
    case A2DP_CODEC_LDAC:
        u8 frame_num = data[0] & 0xf;
        /* printf("ldac : frame_num = %d\n",frame_num); */
        if (hdl->sample_rate <= 48000) {
            frames = frame_num * 128 * (unit);//frame_num * point * (unit);
        } else {
            frames = frame_num * 256 * (unit);
        }
        break;
#endif
    default:
        log_e("unsupport codec_type : 0x%x\n", hdl->media_type);
        break;
    }

    /* printf("frames %d\n", frames); */
    return frames;
}


static void a2dp_stream_control_open(struct a2dp_file_hdl *hdl)
{
    /*
     * 策略选择：
     * 1、是否低延时
     * 2、解码格式？
     * 3、策略方案 - 默认0，其他为定制方案
     */
    if (hdl->stream_ctrl) {
        return;
    }
    if (hdl->link_jl_dongle) {
        hdl->stream_ctrl = a2dp_stream_control_plan_select(hdl->file, a2dp_low_latency, hdl->media_type, A2DP_STREAM_JL_DONGLE_CONTROL);

    } else {
        hdl->stream_ctrl = a2dp_stream_control_plan_select(hdl->file, a2dp_low_latency, hdl->media_type, 0);
    }
    if (hdl->stream_ctrl) {
        hdl->delay_time = a2dp_stream_control_delay_time(hdl->stream_ctrl);
        a2dp_stream_control_set_underrun_callback(hdl->stream_ctrl, hdl, a2dp_source_wake_jlstream_run);
    }
}

static void a2dp_stream_control_close(struct a2dp_file_hdl *hdl)
{
    if (hdl->stream_ctrl) {
        a2dp_stream_control_free(hdl->stream_ctrl);
        hdl->stream_ctrl = NULL;
    }
}

static u32 a2dp_stream_update_base_time(struct a2dp_file_hdl *hdl)
{
    struct a2dp_media_frame frame;
    int distance_time = 0;
    int len = a2dp_media_try_get_packet(hdl->file, &frame);
    if (len > 0) {
        u32 base_clkn = frame.clkn;
        /* if (CONFIG_BTCTLER_TWS_ENABLE && a2dp_low_latency) { */
        /* base_clkn = bt_audio_reference_clock_time(0); */
        /* } */
        a2dp_media_put_packet(hdl->file, frame.packet);
        u32 base_time =  base_clkn + msecs_to_bt_time((hdl->delay_time < 100 ? 100 : hdl->delay_time));
        if ((int)(base_time - bt_audio_reference_clock_time(0)) < msecs_to_bt_time(150)) {//启动过程耗时很长，此处为避免时间戳超时，加上150ms
            base_time = bt_audio_reference_clock_time(0) + msecs_to_bt_time(150);
        }
        return base_time;
    }
    distance_time = a2dp_low_latency ? hdl->delay_time : (hdl->delay_time - a2dp_media_get_remain_play_time(hdl->file, 1));
    if (!a2dp_low_latency) {
        distance_time = hdl->delay_time;
    } else if (distance_time < 100) {
        distance_time = 100;
    }
    /*printf("distance time : %d, %d, %d\n", hdl->delay_time, a2dp_media_get_remain_play_time(hdl->file, 1), distance_time);*/
    return bt_audio_reference_clock_time(0) + msecs_to_bt_time(distance_time);
}


void a2dp_ts_handle_create(struct a2dp_file_hdl *hdl)
{
    if (!hdl || (hdl->rtp_ts_en)) {
        return;
    }
    if (hdl->ts_handle) {
        return;
    }
#if A2DP_TIMESTAMP_ENABLE
    if (!hdl->timestamp_enable) {
        return;
    }

    hdl->base_time = a2dp_stream_update_base_time(hdl);
    int check_diff = hdl->base_time - bt_audio_reference_clock_time(0);
    if (check_diff < 0) {
        printf("a2dp base_time is outdated: %d ms\n", (int)(check_diff * 0.625f));
    } else {
        printf("a2dp features play time: after %d ms\n", (int)(check_diff * 0.625f));
    }
    printf("a2dp timestamp base time : %d, %d\n", hdl->base_time, bt_audio_reference_clock_time(0));
    hdl->ts_handle = a2dp_audio_timestamp_create(hdl->sample_rate, hdl->base_time, TIMESTAMP_US_DENOMINATOR);
    if (hdl->edr_to_local_time) {
        bt_edr_conn_system_clock_init(hdl->bt_addr, TIMESTAMP_US_DENOMINATOR);
        /*printf("--[%s - %d] bt edr system clock init : %u, %lu--\n", __FUNCTION__, __LINE__, hdl->base_time, jiffies_usec());*/
    }
    hdl->sync_step = 0;
    hdl->frame_len = 0;
    hdl->dts = 0;
#endif
}

void a2dp_ts_handle_release(struct a2dp_file_hdl *hdl)
{
    if (!hdl) {
        return;
    }
#if A2DP_TIMESTAMP_ENABLE
    if (hdl->ts_handle) {
        a2dp_audio_timestamp_close(hdl->ts_handle);
        hdl->ts_handle = NULL;
        a2dp_tws_audio_conn_offline();
    }
#endif
}

static void a2dp_frame_pack_timestamp(struct a2dp_file_hdl *hdl, struct stream_frame *frame, u8 *data, int pcm_frames)
{
    if (CONFIG_DONGLE_SPEAK_ENABLE) {
        if (hdl->link_jl_dongle && hdl->rtp_ts_en) {
            u32 ts = RB32(data);
            frame->timestamp    = ts + hdl->jl_dongle_latency * 1000 * 32;
            frame->flags        |= (FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP);
            /* printf("ts : %u, %u,  %u\n",ts,frame->timestamp, bt_audio_reference_clock_time(0)); */
            return;
        }
    }
    if (!hdl->ts_handle || pcm_frames == 0) {
        frame->flags &= ~FRAME_FLAG_TIMESTAMP_ENABLE;
        return;
    }

    if (CONFIG_BTCTLER_TWS_ENABLE && (frame->flags & FRAME_FLAG_RESET_TIMESTAMP_BIT)) {
        /*printf("----stream error : resume----\n");*/
        tws_a2dp_share_timestamp(hdl->ts_handle);
    }
    u32 timestamp = a2dp_audio_update_timestamp(hdl->ts_handle, hdl->seqn, hdl->dts);
    int delay_time = hdl->stream_ctrl ? a2dp_stream_control_delay_time(hdl->stream_ctrl) : hdl->delay_time;
    int frame_delay = (timestamp - (frame->timestamp * 625 * TIME_US_FACTOR)) / 1000 / TIME_US_FACTOR;
    /*int distance_time = (int)(timestamp - (frame->timestamp * 625 * TIME_US_FACTOR)) / 1000 / TIME_US_FACTOR - delay_time;*/
    int distance_time = frame_delay - delay_time;
    if (frame->flags & FRAME_FLAG_FILL_PACKET) { /*补包数据不进行延时调整*/
        distance_time = 0;
    }
    a2dp_audio_delay_offset_update(hdl->ts_handle, distance_time);
    frame->flags |= (FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP | FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE);
    a2dp_stream_mark_next_timestamp(hdl->stream_ctrl, timestamp + PCM_SAMPLE_TO_TIMESTAMP(pcm_frames, hdl->sample_rate));
    if (hdl->edr_to_local_time) {
        timestamp = bt_edr_conn_master_to_local_time(hdl->bt_addr, timestamp);
    }
    frame->timestamp = timestamp;
    frame->d_sample_rate = a2dp_audio_sample_rate(hdl->ts_handle) - hdl->sample_rate;
    /*printf("drift : %d\n", frame->d_sample_rate);*/
    /*printf("-%u, %u, %u-\n", timestamp, bt_edr_conn_master_to_local_time(hdl->bt_addr, timestamp), local_time);*/
    hdl->dts += pcm_frames;
    a2dp_stream_bandwidth_detect_handler(hdl->stream_ctrl, pcm_frames, hdl->sample_rate);
}

static int a2dp_stream_ts_enable_detect(struct a2dp_file_hdl *hdl, u8 *packet, int *drop)
{
    if (hdl->sync_step) {
        return 0;
    }

    if (!drop) {
        printf("wrong argument 'drop'!\n");
    }
    if (CONFIG_BTCTLER_TWS_ENABLE && hdl->ts_handle) {
        if (hdl->tws_case != 1 && \
            !a2dp_audio_timestamp_is_available(hdl->ts_handle, hdl->seqn, 0, drop)) {
            if (*drop) {
                hdl->base_time = a2dp_stream_update_base_time(hdl);
                a2dp_audio_set_base_time(hdl->ts_handle, hdl->base_time);
            }
            return -EINVAL;
        }
    }
    log_d(">>>>ts align time %d ms<<<<\n", hdl->ts_align_time);
    hdl->sync_step = 2;
    return 0;
}

static void a2dp_media_reference_time_setup(struct a2dp_file_hdl *hdl)
{
#if A2DP_TIMESTAMP_ENABLE
    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        err = stream_node_ioctl(hdl->node, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SYNCTS, 0);
        if (err) {
            return;
        }
        /* hdl->edr_to_local_time = 1; //由界面配置*/
    }
    if (CONFIG_BTCTLER_TWS_ENABLE) {
        a2dp_tws_audio_conn_delete();//处理两边交流时主机吴判断从机离线，结束对齐时间戳的情况
    }

    hdl->timestamp_enable = 1;
    if (!hdl->edr_to_local_time) {
        hdl->reference = audio_reference_clock_select(hdl->bt_addr, 0);//0 - a2dp主机，1 - tws, 2 - BLE
    }
#endif
}

static void a2dp_media_reference_time_close(struct a2dp_file_hdl *hdl)
{
#if A2DP_TIMESTAMP_ENABLE
    if (!hdl->edr_to_local_time) {
        audio_reference_clock_exit(hdl->reference);
    }
#endif
}

static void a2dp_file_timestamp_setup(struct a2dp_file_hdl *hdl)
{
    a2dp_stream_control_open(hdl);
    a2dp_ts_handle_create(hdl);
}

static void a2dp_file_timestamp_close(struct a2dp_file_hdl *hdl)
{
    a2dp_tws_media_handshake_exit(hdl);
    a2dp_stream_control_close(hdl);
    a2dp_ts_handle_release(hdl);
}

static int a2dp_ioctl(void *_hdl, int cmd, int arg)
{
    int err = 0;
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        err = a2dp_ioc_set_bt_addr(hdl, (u8 *)arg);
        break;
    case NODE_IOC_GET_BTADDR:
        memcpy((u8 *)arg, hdl->bt_addr, 6);
        break;
    case NODE_IOC_GET_FMT:
        err = a2dp_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_START:
        a2dp_media_start_play(hdl->file);
        a2dp_media_set_rx_notify(hdl->file, hdl, a2dp_source_wake_jlstream_run);
        a2dp_file_stop_abandon_data(hdl);
        a2dp_media_reference_time_setup(hdl);
        break;
    case NODE_IOC_SUSPEND:
        /*hdl->sample_rate = 0;*/
        a2dp_media_set_rx_notify(hdl->file, NULL, NULL);
        a2dp_ioc_stop(hdl);
        a2dp_file_timestamp_close(hdl);
        a2dp_media_reference_time_close(hdl);
        a2dp_file_start_abandon_data(hdl);
        break;
    case NODE_IOC_STOP:
        a2dp_media_set_rx_notify(hdl->file, NULL, NULL);
        a2dp_ioc_stop(hdl);
        a2dp_file_timestamp_close(hdl);
        a2dp_media_reference_time_close(hdl);
        break;
    }

    return err;
}

static void a2dp_release(void *_hdl)
{
    struct a2dp_file_hdl *hdl = (struct a2dp_file_hdl *)_hdl;

    a2dp_close_media_file(hdl->file);
    a2dp_file_stop_abandon_data(hdl);

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(a2dp_file_plug) = {
    .uuid       = NODE_UUID_A2DP_RX,
    .frame_size = 1024,
    .init       = a2dp_init,
    .get_frame  = a2dp_get_frame,
    .ioctl      = a2dp_ioctl,
    .release    = a2dp_release,
};









