#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".a2dp_streamctrl.data.bss")
#pragma data_seg(".a2dp_streamctrl.data")
#pragma const_seg(".a2dp_streamctrl.text.const")
#pragma code_seg(".a2dp_streamctrl.text")
#endif
/*************************************************************************************************/
/*!
*  \file       a2dp_streamctrl.c
*
*  \brief      a2dp plug stream control file.
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "btstack/a2dp_media_codec.h"
#include "sync/audio_syncts.h"
#include "system/timer.h"
#include "media/audio_base.h"
#include "source_node.h"
#include "jiffies.h"
#include "a2dp_streamctrl.h"
#include "app_config.h"

extern const int CONFIG_A2DP_DELAY_TIME_AAC;
extern const int CONFIG_A2DP_DELAY_TIME_SBC;
extern const int CONFIG_A2DP_ADAPTIVE_MAX_LATENCY;
extern const int CONFIG_A2DP_DELAY_TIME_AAC_LO;
extern const int CONFIG_A2DP_DELAY_TIME_SBC_LO;
extern const int CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
extern const int CONFIG_DONGLE_SPEAK_ENABLE;
extern const int CONFIG_A2DP_MAX_BUF_SIZE;
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
_WEAK_ const int CONFIG_A2DP_DELAY_TIME_LDAC = 300;
_WEAK_ const int CONFIG_A2DP_DELAY_TIME_LDAC_LO = 300;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
_WEAK_ const int CONFIG_A2DP_DELAY_TIME_LHDC = 300;
_WEAK_ const int CONFIG_A2DP_DELAY_TIME_LHDC_LO = 300;
#endif
extern u32 bt_audio_reference_clock_time(u8 network);

#define A2DP_FLUENT_DETECT_INTERVAL         90000//ms 流畅播放延时检测时长
#define JL_DONGLE_FLUENT_DETECT_INTERVAL    30000//ms
#define A2DP_ADAPTIVE_DELAY_ENABLE          1

#define A2DP_STREAM_NO_ERR                  0
#define A2DP_STREAM_UNDERRUN                1
#define A2DP_STREAM_OVERRUN                 2
#define A2DP_STREAM_MISSED                  3
#define A2DP_STREAM_DECODE_ERR              4
#define A2DP_STREAM_LOW_UNDERRUN            5

#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])
#define bt_time_to_msecs(clk)   (((clk) * 625) / 1000)

#define DELAY_INCREMENT                     150
#define LOW_LATENCY_DELAY_INCREMENT         50

#define DELAY_DECREMENT                     50
#define LOW_LATENCY_DELAY_DECREMENT         10

#define MAX_DELAY_INCREMENT                 150
struct a2dp_stream_control {
    u8 plan;
    u8 stream_error;
    u8 frame_free;
    u8 first_in;
    u8 low_latency;
    u8 jl_dongle;
    u16 timer;
    u16 seqn;
    u16 overrun_seqn;
    u16 missed_num;
    s16 repair_frames;
    s16 initial_latency;
    s16 adaptive_latency;
    s16 adaptive_max_latency;
    s16 max_rx_interval;
    u32 detect_timeout;
    u32 codec_type;
    u32 frame_time;
    struct a2dp_media_frame frame;
    int frame_len;
    void *stream;
    void *sample_detect;
    u32 next_timestamp;
    void *underrun_signal;
    void (*underrun_callback)(void *);
};


void a2dp_stream_mark_next_timestamp(void *_ctrl, u32 next_timestamp)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;
    if (ctrl) {
        ctrl->next_timestamp = next_timestamp;
    }
}

void a2dp_stream_bandwidth_detect_handler(void *_ctrl, int pcm_frames, int sample_rate)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;
    int max_latency = 0;

    if (ctrl->low_latency) {
        return;
    }

    if (ctrl->frame_len) {
        max_latency = (CONFIG_A2DP_MAX_BUF_SIZE * pcm_frames / ctrl->frame_len) * 1000 / sample_rate * 9 / 10;
    }

    if (!max_latency) {
        return;
    }

    if (max_latency < ctrl->adaptive_max_latency) {
        ctrl->adaptive_max_latency = max_latency;
    }

    if (ctrl->adaptive_latency > ctrl->adaptive_max_latency) {
        ctrl->adaptive_latency = ctrl->adaptive_max_latency;
        a2dp_media_update_delay_report_time(ctrl->stream, ctrl->adaptive_latency);
    }
}

static void a2dp_stream_underrun_adaptive_handler(struct a2dp_stream_control *ctrl)
{
#if A2DP_ADAPTIVE_DELAY_ENABLE
    ctrl->detect_timeout = jiffies + msecs_to_jiffies(ctrl->jl_dongle ? JL_DONGLE_FLUENT_DETECT_INTERVAL : A2DP_FLUENT_DETECT_INTERVAL);

    if (!ctrl->low_latency) {
        ctrl->adaptive_latency = ctrl->adaptive_max_latency;
    } else {
        ctrl->adaptive_latency += LOW_LATENCY_DELAY_INCREMENT;

        if (ctrl->adaptive_latency < ctrl->max_rx_interval) {
            ctrl->adaptive_latency = ctrl->max_rx_interval;
        }

        if (ctrl->adaptive_latency > ctrl->adaptive_max_latency) {
            ctrl->adaptive_latency = ctrl->adaptive_max_latency;
        }
    }
    a2dp_media_update_delay_report_time(ctrl->stream, ctrl->adaptive_latency);
    /*printf("---underrun, adaptive : %dms---\n", ctrl->adaptive_latency);*/
#endif
}

/*
 * 自适应延时策略
 */
static void a2dp_stream_adaptive_detect_handler(struct a2dp_stream_control *ctrl, u8 new_frame, u32 new_frame_time)
{
#if A2DP_ADAPTIVE_DELAY_ENABLE
    if (ctrl->stream_error || !new_frame) {
        return;
    }
    int rx_interval = 0;
    if (ctrl->frame_time) {
        rx_interval = bt_time_to_msecs((new_frame_time - ctrl->frame_time) & 0x7ffffff) + 1;
    }
    ctrl->frame_time = new_frame_time;

    if (rx_interval > ctrl->max_rx_interval) {
        if (CONFIG_DONGLE_SPEAK_ENABLE && ctrl->jl_dongle) {
            return;
        }
        ctrl->max_rx_interval = rx_interval;
        if (ctrl->max_rx_interval > ctrl->initial_latency + MAX_DELAY_INCREMENT) {
            ctrl->max_rx_interval = ctrl->initial_latency + MAX_DELAY_INCREMENT;
        }
        if (ctrl->adaptive_latency < ctrl->max_rx_interval) {
            ctrl->adaptive_latency = ctrl->max_rx_interval;
            a2dp_media_update_delay_report_time(ctrl->stream, ctrl->adaptive_latency);
            ctrl->detect_timeout = jiffies + msecs_to_jiffies(A2DP_FLUENT_DETECT_INTERVAL);
            ctrl->max_rx_interval = ctrl->initial_latency;
            return;
        }
        /*printf("---rx interval, adaptive : %dms, %dms---\n", ctrl->adaptive_latency, ctrl->max_rx_interval);*/
    }

    if (time_after(jiffies, ctrl->detect_timeout)) {
        ctrl->adaptive_latency -= DELAY_DECREMENT;
        if (ctrl->adaptive_latency < ctrl->max_rx_interval) {
            ctrl->adaptive_latency = ctrl->max_rx_interval;
        }

        if (ctrl->adaptive_latency < ctrl->initial_latency) {
            ctrl->adaptive_latency = ctrl->initial_latency;
        }
        /*printf("---adaptive detect : %dms, %dms---\n", ctrl->adaptive_latency, ctrl->max_rx_interval);*/
        if (ctrl->low_latency) {
            ctrl->max_rx_interval -= 10;//ctrl->initial_latency;
        } else {
            ctrl->max_rx_interval = ctrl->initial_latency;
        }
        ctrl->detect_timeout = jiffies + msecs_to_jiffies(A2DP_FLUENT_DETECT_INTERVAL);
        a2dp_media_update_delay_report_time(ctrl->stream, ctrl->adaptive_latency);
    }
#endif
}


void *a2dp_stream_control_plan_select(void *stream, int low_latency, u32 codec_type, u8 plan)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)zalloc(sizeof(struct a2dp_stream_control));

    if (!ctrl) {
        return NULL;
    }

    switch (plan) {

    case A2DP_STREAM_JL_DONGLE_CONTROL:
        if (CONFIG_DONGLE_SPEAK_ENABLE) {
            ctrl->initial_latency = CONFIG_JL_DONGLE_PLAYBACK_LATENCY;
            ctrl->adaptive_latency = ctrl->initial_latency;
            ctrl->max_rx_interval = ctrl->initial_latency;
            ctrl->adaptive_max_latency = low_latency ? (ctrl->initial_latency + MAX_DELAY_INCREMENT) : CONFIG_A2DP_ADAPTIVE_MAX_LATENCY;
            ctrl->detect_timeout = jiffies + msecs_to_jiffies(A2DP_FLUENT_DETECT_INTERVAL);
            ctrl->jl_dongle = 1;
        }
        break;
    default:
        if (low_latency) {
            if (codec_type == A2DP_CODEC_MPEG24) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_AAC_LO;
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
            } else if (codec_type == A2DP_CODEC_LDAC) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_LDAC_LO;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
            } else if (codec_type == A2DP_CODEC_LHDC) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_LHDC_LO;
#endif
            } else {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_SBC_LO;
            }
        } else {
            if (codec_type == A2DP_CODEC_MPEG24) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_AAC;
#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
            } else if (codec_type == A2DP_CODEC_LDAC) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_LDAC;
#endif
#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
            } else if (codec_type == A2DP_CODEC_LHDC) {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_LHDC;
#endif
            } else {
                ctrl->initial_latency = CONFIG_A2DP_DELAY_TIME_SBC;
            }
        }
        ctrl->adaptive_max_latency = low_latency ? (ctrl->initial_latency + MAX_DELAY_INCREMENT) : CONFIG_A2DP_ADAPTIVE_MAX_LATENCY;
        ctrl->adaptive_latency = ctrl->initial_latency;
        ctrl->max_rx_interval = ctrl->initial_latency;
        ctrl->detect_timeout = jiffies + msecs_to_jiffies(A2DP_FLUENT_DETECT_INTERVAL);
        break;
    }

    ctrl->low_latency = low_latency;
    ctrl->first_in = 1;
    ctrl->codec_type = codec_type;
    ctrl->plan = plan;
    ctrl->stream = stream;
    a2dp_media_update_delay_report_time(ctrl->stream, ctrl->adaptive_latency);
    return ctrl;
}

static void a2dp_stream_underrun_signal(void *arg)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)arg;

    local_irq_disable();
    if (ctrl->underrun_callback) {
        ctrl->underrun_callback(ctrl->underrun_signal);
    }
    ctrl->timer = 0;
    local_irq_enable();
}

static int a2dp_audio_is_underrun(struct a2dp_stream_control *ctrl)
{
    int underrun_time = ctrl->low_latency ? 2 : 30;
    if (ctrl->next_timestamp) {
        u32 reference_clock = bt_audio_reference_clock_time(0);
        if (reference_clock == (u32) - 1) {
            return true;
        }
        u32 reference_time = reference_clock * 625 * TIMESTAMP_US_DENOMINATOR;
        int distance_time = ctrl->next_timestamp - reference_time;
        if (distance_time > 67108863L || distance_time < -67108863L) {
            if (ctrl->next_timestamp > reference_time) {
                distance_time = ctrl->next_timestamp - 0xffffffff - reference_time;
            } else {
                distance_time = 0xffffffff - reference_time + ctrl->next_timestamp;
            }
        }
        distance_time = distance_time / 1000 / TIMESTAMP_US_DENOMINATOR;
        /*printf("distance_time %d %u %u\n", distance_time, ctrl->next_timestamp, reference_time);*/
        if (distance_time < underrun_time) {
            return true;
        }

        local_irq_disable();
        if (ctrl->timer) {
            sys_hi_timeout_del(ctrl->timer);
            ctrl->timer = 0;
        }
        ctrl->timer = sys_hi_timeout_add(ctrl, a2dp_stream_underrun_signal, distance_time - underrun_time);
        local_irq_enable();
    }

    return 0;
}

static int a2dp_stream_underrun_handler(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame)
{
    if (!a2dp_audio_is_underrun(ctrl)) {
        /*putchar('x');*/
        return 0;
    }
    putchar('X');

    a2dp_stream_underrun_adaptive_handler(ctrl);
    if (ctrl->stream_error != A2DP_STREAM_UNDERRUN) {
        if (!ctrl->stream_error) {

        }
        ctrl->stream_error = A2DP_STREAM_UNDERRUN;
    }
    memcpy(frame, &ctrl->frame, sizeof(ctrl->frame));
    ctrl->repair_frames++;

    return ctrl->frame_len;
}

static void a2dp_stream_control_free_frames(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame)
{
    if (frame && frame->packet) {
        a2dp_media_free_packet(ctrl->stream, frame->packet);
        if ((void *)frame->packet == (void *)ctrl->frame.packet) {
            ctrl->frame.packet = NULL;
        }
    }

    if (ctrl->frame.packet) {
        a2dp_media_free_packet(ctrl->stream, ctrl->frame.packet);
        ctrl->frame.packet = NULL;
    }
    ctrl->frame_len = 0;
}


static int a2dp_stream_overrun_handler(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame, int *len)
{
    while (1) {
        if (1) {//!ctrl->low_latency) {
            int msecs = a2dp_media_get_remain_play_time(ctrl->stream, 1);
            if (msecs < ctrl->adaptive_latency) {
                /*printf("adaptive latency %d, msecs %d\n", ctrl->adaptive_latency, msecs);*/
                break;
            }
        }

        int rlen = a2dp_media_try_get_packet(ctrl->stream, frame);
        if (rlen <= 0) {
            break;
        }
        a2dp_stream_control_free_frames(ctrl, NULL);
        memcpy(&ctrl->frame, frame, sizeof(ctrl->frame));
        ctrl->frame_len = rlen;
        *len = rlen;
        putchar('n');
        return 1;
    }

    memcpy(frame, &ctrl->frame, sizeof(ctrl->frame));
    *len = ctrl->frame_len;
    putchar('o');
    return 0;
}

static int a2dp_stream_missed_handler(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame, int *len)
{
    memcpy(frame, &ctrl->frame, sizeof(ctrl->frame));
    *len = ctrl->frame_len;
    if (--ctrl->missed_num == 0) {
        return 1;
    }
    return 0;
}

static int a2dp_stream_error_filter(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame, int len)
{
    int err = 0;

    int header_len = a2dp_media_get_rtp_header_len(ctrl->codec_type, frame->packet, len);
    if (header_len >= len) {
        printf("##A2DP header error : %d\n", header_len);
        a2dp_stream_control_free_frames(ctrl, frame);
        return -EFAULT;
    }

    u16 seqn = RB16(frame->packet + 2);
    if (ctrl->first_in) {
        ctrl->first_in = 0;
        goto __exit;
    }
    if (seqn != ctrl->seqn) {
        if (ctrl->stream_error == A2DP_STREAM_UNDERRUN) {
            int missed_frames = (u16)(seqn - ctrl->seqn) - 1;
            if (missed_frames > ctrl->repair_frames) {
                ctrl->stream_error = A2DP_STREAM_MISSED;
                ctrl->missed_num = 2;//missed_frames - ctrl->repair_frames + 1;
                /*printf("case 0 : %d, %d\n", missed_frames, ctrl->repair_frames);*/
                err = -EAGAIN;
            } else if (missed_frames < ctrl->repair_frames) {
                ctrl->stream_error = A2DP_STREAM_OVERRUN;
                ctrl->overrun_seqn = seqn + ctrl->repair_frames - missed_frames;
                /*printf("case 1 : %d, %d, seqn : %d, %d\n", missed_frames, ctrl->repair_frames, seqn, ctrl->overrun_seqn);*/
                err = -EAGAIN;
            }
        } else if (!ctrl->stream_error && (u16)(seqn - ctrl->seqn) > 1) {
            err = -EAGAIN;
            if ((u16)(seqn - ctrl->seqn) > 32768) {
                a2dp_stream_control_free_frames(ctrl, frame);
                return err;
            }
            ctrl->stream_error = A2DP_STREAM_MISSED;
            ctrl->missed_num = 2;//(u16)(seqn - ctrl->seqn);
            /*printf("case 2 : %d, %d, %d\n", seqn, ctrl->seqn, ctrl->missed_num); */
        }
        ctrl->repair_frames = 0;
    }

__exit:
    ctrl->seqn = seqn;
    memcpy(&ctrl->frame, frame, sizeof(ctrl->frame));
    ctrl->frame_len = len;
    return err;
}

static int a2dp_get_frame_and_check_errors(struct a2dp_stream_control *ctrl, struct a2dp_media_frame *frame, int *len)
{
    int rlen = 0;
    int err = 0;
    u8 new_packet = 0;

try_again:
    switch (ctrl->stream_error) {
    case A2DP_STREAM_OVERRUN:
        new_packet = a2dp_stream_overrun_handler(ctrl, frame, len);
        break;
    case A2DP_STREAM_MISSED:
        new_packet = a2dp_stream_missed_handler(ctrl, frame, len);
        if (frame->packet) {
            break;
        }
    //注意：这里不break是因为很有可能由于位流的错误导致补包无法再正常补上
    default:
        rlen = a2dp_media_try_get_packet(ctrl->stream, frame);
        if (rlen <= 0) {
            rlen = a2dp_stream_underrun_handler(ctrl, frame);
        } else {
            if (ctrl->frame.packet) {
                a2dp_media_free_packet(ctrl->stream, ctrl->frame.packet);
                ctrl->frame.packet = NULL;
            }
            ctrl->frame_len = 0;
            new_packet = 1;
        }
        *len = rlen;
        break;
    }

    if (*len <= 0) {
        return 0;
    }
    err = a2dp_stream_error_filter(ctrl, frame, *len);
    if (err) {
        if (-err == EAGAIN) {
            goto try_again;
        }
        *len = 0;
        return 0;
    }

    a2dp_stream_adaptive_detect_handler(ctrl, new_packet, frame->clkn);

    if (ctrl->stream_error) {
        if (new_packet) {
            ctrl->stream_error = 0;
            return FRAME_FLAG_RESET_TIMESTAMP_BIT;
        }
        return FRAME_FLAG_FILL_PACKET;
    }

    return 0;
}

int a2dp_stream_control_pull_frame(void *_ctrl, struct a2dp_media_frame *frame, int *len)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;

    if (!ctrl) {
        *len = 0;
        return 0;
    }

    switch (ctrl->plan) {
    default:
        return a2dp_get_frame_and_check_errors(ctrl, frame, len);
    }

    return 0;
}

void a2dp_stream_control_free_frame(void *_ctrl, struct a2dp_media_frame *frame)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;

    switch (ctrl->plan) {
    default:
        if (ctrl->frame.packet == frame->packet) {
            ctrl->frame_free = 1;
        }
    }
}

void a2dp_stream_control_set_underrun_callback(void *_ctrl, void *priv, void (*callback)(void *priv))
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;

    ctrl->underrun_signal = priv;
    ctrl->underrun_callback = callback;
}

int a2dp_stream_control_delay_time(void *_ctrl)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;

    if (ctrl) {
        return ctrl->adaptive_latency;
    }

    return CONFIG_A2DP_DELAY_TIME_AAC;
}

void a2dp_stream_control_free(void *_ctrl)
{
    struct a2dp_stream_control *ctrl = (struct a2dp_stream_control *)_ctrl;

    if (!ctrl) {
        return;
    }

    a2dp_stream_control_free_frames(ctrl, NULL);

    local_irq_disable();
    if (ctrl->timer) {
        sys_hi_timeout_del(ctrl->timer);
        ctrl->timer = 0;
    }
    local_irq_enable();
    free(ctrl);
}
