/*************************************************************************************************/
/*!
*  \file      le_audio_recorder.c
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#include "jlstream.h"
#include "le_audio_recorder.h"
#include "sdk_config.h"
#include "le_audio_stream.h"
#include "audio_config_def.h"
#include "audio_config.h"
#include "spdif_file.h"
#include "fm_file.h"
#include "media/sync/audio_syncts.h"
#include "asm/dac.h"
#include "audio_cvp.h"

struct le_audio_a2dp_recorder {
    void *stream;
    u8 btaddr[6];
    u16 retry_timer;
};

static struct le_audio_a2dp_recorder *g_a2dp_recorder = NULL;
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_LINEIN_ENABLE
struct le_audio_aux_recorder {
    void *stream;
};
static struct le_audio_aux_recorder *g_aux_recorder = NULL;
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_SPDIF_ENABLE
struct le_audio_spdif_recorder {
    void *stream;
};
static struct le_audio_spdif_recorder *g_spdif_recorder = NULL;
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_FM_ENABLE
struct le_audio_fm_recorder {
    void *stream;
};
static struct le_audio_fm_recorder *g_fm_recorder = NULL;
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_APP_IIS_EN
struct le_audio_iis_recorder {
    void *stream;
};
static struct le_audio_iis_recorder *g_iis_recorder = NULL;
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_PC_ENABLE
struct le_audio_pc_recorder {
    void *stream;
};
static struct le_audio_pc_recorder *g_pc_recorder = NULL;
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) && TCFG_AUDIO_MIC_ENABLE) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
struct le_audio_mic_recorder {
    void *stream;
};
static struct le_audio_mic_recorder *g_mic_recorder = NULL;
#endif

static void a2dp_recorder_callback(void *private_data, int event)
{
    printf("le audio a2dp recorder callback : %d\n", event);
}

static void retry_start_a2dp_player(void *p)
{
    if (g_a2dp_recorder && g_a2dp_recorder->stream) {
        int err = jlstream_start(g_a2dp_recorder->stream);
        if (err == 0) {
            sys_timer_del(g_a2dp_recorder->retry_timer);
            g_a2dp_recorder->retry_timer = 0;
        }
    }
}

int le_audio_a2dp_recorder_open(u8 *btaddr, void *arg, void *le_audio)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)arg;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"a2dp_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };

    if (!g_a2dp_recorder) {
        g_a2dp_recorder = zalloc(sizeof(struct le_audio_a2dp_recorder));
        if (!g_a2dp_recorder) {
            return -ENOMEM;
        }
    }
    g_a2dp_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_A2DP_RX);

    jlstream_set_callback(g_a2dp_recorder->stream, NULL, a2dp_recorder_callback);
    jlstream_set_scene(g_a2dp_recorder->stream, STREAM_SCENE_A2DP);

    jlstream_node_ioctl(g_a2dp_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_BTADDR, (int)btaddr);

    jlstream_node_ioctl(g_a2dp_recorder->stream, NODE_UUID_IIS0_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);
    jlstream_node_ioctl(g_a2dp_recorder->stream, NODE_UUID_IIS1_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);

    jlstream_node_ioctl(g_a2dp_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);

    /*memcpy(&fmt, arg, sizeof(struct stream_enc_fmt));*/

    memcpy(g_a2dp_recorder->btaddr, btaddr, 6);

    err = jlstream_ioctl(g_a2dp_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_a2dp_recorder->stream);
        if (err) {
            g_a2dp_recorder->retry_timer = sys_timer_add(NULL, retry_start_a2dp_player, 200);
            return 0;
        }
    }

    if (err) {
        jlstream_release(g_a2dp_recorder->stream);
        free(g_a2dp_recorder);
        g_a2dp_recorder = NULL;
        return err;
    }
    return 0;
}

void le_audio_a2dp_recorder_close(u8 *btaddr)
{
    struct le_audio_a2dp_recorder *a2dp_recorder = g_a2dp_recorder;

    if (!a2dp_recorder) {
        return;
    }

    if (memcmp(a2dp_recorder->btaddr, btaddr, 6)) {
        return;
    }

    if (a2dp_recorder->stream) {
        jlstream_stop(a2dp_recorder->stream, 0);
        jlstream_release(a2dp_recorder->stream);
    }

    if (g_a2dp_recorder->retry_timer) {
        sys_timer_del(g_a2dp_recorder->retry_timer);
        g_a2dp_recorder->retry_timer = 0;
    }

    free(a2dp_recorder);
    g_a2dp_recorder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"a2dp_le_audio");
}

u8 *le_audio_a2dp_recorder_get_btaddr(void)
{
    if (!g_a2dp_recorder) {
        return NULL;
    } else {
        return g_a2dp_recorder->btaddr;
    }

}

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_LINEIN_ENABLE
static void aux_recorder_callback(void *private_data, int event)
{
    printf("le audio aux recorder callback : %d\n", event);
}
int le_audio_linein_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)params;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"linein_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };

    if (!g_aux_recorder) {
        g_aux_recorder = zalloc(sizeof(struct le_audio_aux_recorder));
        if (!g_aux_recorder) {
            return -ENOMEM;
        }
    }
    g_aux_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LINEIN);
    jlstream_set_callback(g_aux_recorder->stream, NULL, aux_recorder_callback);
    jlstream_set_scene(g_aux_recorder->stream, STREAM_SCENE_LINEIN);

    jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_IIS0_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);
    jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_IIS1_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);


    err = jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    if (latency == 0) {
        latency = 100000;
    }
    jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    //设置中断点数
    jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);

    jlstream_node_ioctl(g_aux_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    err = jlstream_ioctl(g_aux_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_aux_recorder->stream);
    }

    if (err) {
        jlstream_release(g_aux_recorder->stream);
        free(g_aux_recorder);
        g_aux_recorder = NULL;
        return err;
    }

    return 0;
}

void le_audio_linein_recorder_close(void)
{
    struct le_audio_aux_recorder *aux_recorder = g_aux_recorder;
    if (!aux_recorder) {
        return;
    }
    if (aux_recorder->stream) {
        jlstream_stop(aux_recorder->stream, 0);
        jlstream_release(aux_recorder->stream);
    }

    free(aux_recorder);
    g_aux_recorder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"linein_le_audio");
}
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_APP_IIS_EN
static void iis_recorder_callback(void *private_data, int event)
{
    printf("le audio iis recorder callback : %d\n", event);
}
int le_audio_iis_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)params;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"iis_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };

    if (!g_iis_recorder) {
        g_iis_recorder = zalloc(sizeof(struct le_audio_iis_recorder));
        if (!g_iis_recorder) {
            return -ENOMEM;
        }
    }
    g_iis_recorder->stream = jlstream_pipeline_parse_by_node_name(uuid, "IIS0_RX2");
    jlstream_set_callback(g_iis_recorder->stream, NULL, iis_recorder_callback);
    jlstream_set_scene(g_iis_recorder->stream, STREAM_SCENE_IIS);


    jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_IIS0_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);
    jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_IIS1_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);

    err = jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    if (latency == 0) {
        latency = 100000;
    }
    jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    //设置中断点数
    jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_IIS_IRQ_POINTS);
    jlstream_node_ioctl(g_iis_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_IIS_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    err = jlstream_ioctl(g_iis_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_iis_recorder->stream);
    }
    if (err) {
        jlstream_release(g_iis_recorder->stream);
        free(g_iis_recorder);
        g_iis_recorder = NULL;
        return err;
    }
    return 0;
}

void le_audio_iis_recorder_close(void)
{
    struct le_audio_iis_recorder *iis_recorder = g_iis_recorder;
    if (!iis_recorder) {
        return;
    }
    if (iis_recorder->stream) {
        jlstream_stop(iis_recorder->stream, 0);
        jlstream_release(iis_recorder->stream);
    }

    free(iis_recorder);
    g_iis_recorder = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"iis_le_audio");
}
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_PC_ENABLE
static void pc_recorder_callback(void *private_data, int event)
{
    printf("le audio pc recorder callback : %d\n", event);
}
int le_audio_pc_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)params;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"pc_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };
    if (!g_pc_recorder) {
        g_pc_recorder = zalloc(sizeof(struct le_audio_pc_recorder));
        if (!g_pc_recorder) {
            return -ENOMEM;
        }
    }
    g_pc_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PC_SPK);
    jlstream_set_callback(g_pc_recorder->stream, NULL, pc_recorder_callback);
    jlstream_set_scene(g_pc_recorder->stream, STREAM_SCENE_PC_SPK);
    if (latency == 0) {
        latency = 100000;
    }
    jlstream_node_ioctl(g_pc_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    err = jlstream_node_ioctl(g_pc_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    //设置中断点数
    jlstream_node_ioctl(g_pc_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 192);
    jlstream_node_ioctl(g_pc_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, 192);//四声道时，指定声道合并单个声道的点数
    err = jlstream_ioctl(g_pc_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_pc_recorder->stream);
    }
    if (err) {
        jlstream_release(g_pc_recorder->stream);
        free(g_pc_recorder);
        g_pc_recorder = NULL;
        return err;
    }
    return 0;
}

void le_audio_pc_recorder_close(void)
{
    struct le_audio_pc_recorder *pc_recorder = g_pc_recorder;
    if (!pc_recorder) {
        return;
    }
    if (pc_recorder->stream) {
        jlstream_stop(pc_recorder->stream, 0);
        jlstream_release(pc_recorder->stream);
    }
    free(pc_recorder);
    g_pc_recorder = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"pc_le_audio");
}
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_SPDIF_ENABLE
static void spdif_recorder_callback(void *private_data, int event)
{
    printf("le audio spdif recorder callback : %d\n", event);
}
int le_audio_spdif_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)params;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"spdif_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };

    if (!g_spdif_recorder) {
        g_spdif_recorder = zalloc(sizeof(struct le_audio_spdif_recorder));
        if (!g_spdif_recorder) {
            return -ENOMEM;
        }
    }
    g_spdif_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SPDIF);
    jlstream_set_callback(g_spdif_recorder->stream, NULL, spdif_recorder_callback);
    jlstream_set_scene(g_spdif_recorder->stream, STREAM_SCENE_SPDIF);

    jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_IIS0_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);
    jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_IIS1_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);

    err = jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    if (latency == 0) {
        latency = 100000;
    }
    jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, SPDIF_DATA_DMA_LEN);
    jlstream_node_ioctl(g_spdif_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_SPDIF_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    err = jlstream_ioctl(g_spdif_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_spdif_recorder->stream);
    }
    if (err) {
        jlstream_release(g_spdif_recorder->stream);
        free(g_spdif_recorder);
        g_spdif_recorder = NULL;
        return err;
    }
    return 0;
}

void le_audio_spdif_recorder_close(void)
{
    struct le_audio_spdif_recorder *spdif_recorder = g_spdif_recorder;
    if (!spdif_recorder) {
        return;
    }
    if (spdif_recorder->stream) {
        jlstream_stop(spdif_recorder->stream, 0);
        jlstream_release(spdif_recorder->stream);
    }

    free(spdif_recorder);
    g_spdif_recorder = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"spdif_le_audio");
}
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_FM_ENABLE
static void fm_recorder_callback(void *private_data, int event)
{
    printf("le audio fm recorder callback : %d\n", event);
}
int le_audio_fm_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_format *le_audio_fmt = (struct le_audio_stream_format *)params;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"fm_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };

    if (!g_fm_recorder) {
        g_fm_recorder = zalloc(sizeof(struct le_audio_fm_recorder));
        if (!g_fm_recorder) {
            return -ENOMEM;
        }
    }
    g_fm_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_FM);
    jlstream_set_callback(g_fm_recorder->stream, NULL, fm_recorder_callback);
    jlstream_set_scene(g_fm_recorder->stream, STREAM_SCENE_FM);

    jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_IIS0_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);
    jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_IIS1_TX, NODE_IOC_SET_PARAM, AUDIO_NETWORK_LOCAL);

    err = jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    if (latency == 0) {
        latency = 100000;
    }
    jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, FM_ADC_IRQ_POINTS);
    jlstream_node_ioctl(g_fm_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, FM_ADC_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    err = jlstream_ioctl(g_fm_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_fm_recorder->stream);
    }
    if (err) {
        jlstream_release(g_fm_recorder->stream);
        free(g_fm_recorder);
        g_fm_recorder = NULL;
        return err;
    }
    return 0;
}

void le_audio_fm_recorder_close(void)
{
    struct le_audio_fm_recorder *fm_recorder = g_fm_recorder;
    if (!fm_recorder) {
        return;
    }
    if (fm_recorder->stream) {
        jlstream_stop(fm_recorder->stream, 0);
        jlstream_release(fm_recorder->stream);
    }

    free(fm_recorder);
    g_fm_recorder = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"fm_le_audio");
}
#endif

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && TCFG_AUDIO_MIC_ENABLE
static void mic_recorder_callback(void *private_data, int event)
{
    printf("le audio mic recorder callback : %d\n", event);
}

int le_audio_mic_recorder_open(void *params, void *le_audio, int latency)
{
    int err = 0;
    struct le_audio_stream_params *lea_params = params;
    struct le_audio_stream_format *le_audio_fmt = &lea_params->fmt;
    u16 source_uuid;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_le_audio");
    struct stream_enc_fmt fmt = {
        .coding_type = le_audio_fmt->coding_type,
        .channel = le_audio_fmt->nch,
        .bit_rate = le_audio_fmt->bit_rate,
        .sample_rate = le_audio_fmt->sample_rate,
        .frame_dms = le_audio_fmt->frame_dms,
    };
    if (!g_mic_recorder) {
        g_mic_recorder = zalloc(sizeof(struct le_audio_mic_recorder));
        if (!g_mic_recorder) {
            return -ENOMEM;
        }
    }
    g_mic_recorder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    source_uuid = NODE_UUID_ADC;

    if (!g_mic_recorder->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    jlstream_set_callback(g_mic_recorder->stream, NULL, mic_recorder_callback);
    if (lea_params->service_type == LEA_SERVICE_CALL) {
        //printf("LEA Recoder:LEA_CALL\n");
        jlstream_set_scene(g_mic_recorder->stream, STREAM_SCENE_LEA_CALL);
        jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256);
    } else {
        //printf("LEA Recoder:MIC\n");
        jlstream_set_scene(g_mic_recorder->stream, STREAM_SCENE_MIC);
        jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 160);
    }
    if (latency == 0) {
        latency = 100000;
    }

    jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_CAPTURE_SYNC, NODE_IOC_SET_PARAM, latency);
    err = jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_LE_AUDIO_SOURCE, NODE_IOC_SET_BTADDR, (int)le_audio);
    //设置中断点数
    /* jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS); */
    jlstream_node_ioctl(g_mic_recorder->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    u16 node_uuid = get_cvp_node_uuid();
    if (node_uuid) {
        u32 ref_sr = audio_dac_get_sample_rate(&dac_hdl);
        jlstream_node_ioctl(g_mic_recorder->stream, node_uuid, NODE_IOC_SET_FMT, (int)ref_sr);
        err = jlstream_node_ioctl(g_mic_recorder->stream, node_uuid, NODE_IOC_SET_PRIV_FMT, source_uuid);
        //根据回音消除的类型，将配置传递到对应的节点
        if (err && (err != -ENOENT)) {	//兼容没有cvp节点的情况
            goto __exit1;
        }
    }

    err = jlstream_ioctl(g_mic_recorder->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
    if (err == 0) {
        err = jlstream_start(g_mic_recorder->stream);
    }
    if (err) {
        goto __exit1;
    }

    printf("le_audio mic recorder open success  \n");
    return 0;

__exit1:
    jlstream_release(g_mic_recorder->stream);
__exit0:
    free(g_mic_recorder);
    g_mic_recorder = NULL;
    return err;
}

void le_audio_mic_recorder_close(void)
{
    struct le_audio_mic_recorder *mic_recorder = g_mic_recorder;

    printf("le_audio_mic_recorder_close\n");

    if (!mic_recorder) {
        return;
    }
    if (mic_recorder->stream) {
        jlstream_stop(mic_recorder->stream, 0);
        jlstream_release(mic_recorder->stream);
    }
    free(mic_recorder);
    g_mic_recorder = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"mic_le_audio");
}
#endif

