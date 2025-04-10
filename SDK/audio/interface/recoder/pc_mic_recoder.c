#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_mic_recoder.data.bss")
#pragma data_seg(".pc_mic_recoder.data")
#pragma const_seg(".pc_mic_recoder.text.const")
#pragma code_seg(".pc_mic_recoder.text")
#endif
#include "pc_mic_recoder.h"
#include "sdk_config.h"
#include "media/includes.h"
#include "jlstream.h"
#include "uac_stream.h"
#include "asm/dac.h"
#include "audio_config_def.h"
#include "app_main.h"
#include "volume_node.h"
#include "audio_cvp.h"
#include "pc_spk_player.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[pcmic]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE

extern struct audio_dac_hdl dac_hdl;

typedef enum {
    PC_MIC_STA_CLOSE,
    PC_MIC_STA_WAIT_CLOSE,
    PC_MIC_STA_OPEN,
    PC_MIC_STA_WAIT_OPEN,
} pc_mic_state_t;
pc_mic_state_t g_pc_mic_state;

struct pc_mic_recoder {
    struct jlstream *stream;
};
static struct pc_mic_recoder *g_pc_mic_recoder = NULL;

static u8 pcmic_volume_wait_set_flag = 0;
static u8 pcm_mic_recoder_check = 0;
static OS_MUTEX mic_rec_mutex;
static int pcm_mic_recoder_init(void)
{
    os_mutex_create(&mic_rec_mutex);
    return 0;
}
__initcall(pcm_mic_recoder_init);

void pcm_mic_recoder_dump(int force_dump)
{
    os_mutex_pend(&mic_rec_mutex, 0);
    struct pc_mic_recoder *recoder = g_pc_mic_recoder;
    if (recoder && recoder->stream) {
        jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_FORCE_DUMP_PACKET, force_dump);
    } else {
        pcm_mic_recoder_check = force_dump;
    }
    os_mutex_post(&mic_rec_mutex);
}

static void pc_mic_recoder_callback(void *private_data, int event)
{
    struct pc_mic_recoder *recoder = g_pc_mic_recoder;
    struct jlstream *stream = (struct jlstream *)private_data;
    u16 volume;
    struct volume_cfg cfg;
    int err;

    switch (event) {
    case STREAM_EVENT_START:
        uac_mic_stream_get_volume(&volume);
        cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
        cfg.cur_vol = volume;
        err = jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, "Vol_PcMic", (void *)&cfg, sizeof(struct volume_cfg));
        log_info(">>> pc mic vol: %d, ret:%d", volume, err);
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
        /*打开pc mic，没有开skp，忽略外部参考数据*/
        printf("STREAM_EVENT_START");
        if (!pc_spk_player_runing()) {
            printf("CVP_OUTWAY_REF_IGNORE, 1");
            audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 1, NULL);
        }
#endif
        break;
    }
}

int pc_mic_recoder_open(void)
{
    os_mutex_pend(&mic_rec_mutex, 0);
    struct pc_mic_recoder *recoder = NULL;
    u16 source_uuid;
    if (g_pc_mic_recoder) {
        log_error("## %s, pc mic recoder is busy!\n", __func__);
        os_mutex_post(&mic_rec_mutex);
        return -EBUSY;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"pc_mic");
    if (uuid == 0) {
        os_mutex_post(&mic_rec_mutex);
        return -EFAULT;
    }
    recoder = zalloc(sizeof(*recoder));
    if (!recoder) {
        os_mutex_post(&mic_rec_mutex);
        return -ENOMEM;
    }
    recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    source_uuid = NODE_UUID_ADC;

    if (!recoder->stream) {
        goto __exit0;
    }

    //设置ADC的中断点数
    int err = jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);
    if (err) {
        goto __exit1;
    }

    u16 node_uuid = get_cvp_node_uuid();
    //根据回音消除的类型，将配置传递到对应的节点
    if (node_uuid) {
#if !(TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE))
        u32 ref_sr = audio_dac_get_sample_rate(&dac_hdl);
        jlstream_node_ioctl(recoder->stream, node_uuid, NODE_IOC_SET_FMT, (int)ref_sr);
#endif
        err = jlstream_node_ioctl(recoder->stream, node_uuid, NODE_IOC_SET_PRIV_FMT, source_uuid);
        if (err && (err != -ENOENT)) {	//兼容没有cvp节点的情况
            goto __exit1;
        }
    }


    jlstream_set_callback(recoder->stream, recoder->stream, pc_mic_recoder_callback);
    jlstream_set_scene(recoder->stream, STREAM_SCENE_PC_MIC);
    err = jlstream_start(recoder->stream);
    if (err) {
        goto __exit1;
    }

    g_pc_mic_state = PC_MIC_STA_OPEN;
    g_pc_mic_recoder = recoder;

    pcm_mic_recoder_dump(pcm_mic_recoder_check);
    os_mutex_post(&mic_rec_mutex);
    return 0;

__exit1:
    jlstream_release(recoder->stream);
__exit0:
    free(recoder);
    os_mutex_post(&mic_rec_mutex);
    return -ENOMEM;
}


void pc_mic_recoder_close(void)
{
    os_mutex_pend(&mic_rec_mutex, 0);
    struct pc_mic_recoder *recoder = g_pc_mic_recoder;

    if (!recoder) {
        os_mutex_post(&mic_rec_mutex);
        return;
    }
    if (recoder->stream) {
        jlstream_stop(recoder->stream, 0);
        jlstream_release(recoder->stream);
    }

    free(recoder);
    g_pc_mic_recoder = NULL;
    pcm_mic_recoder_check = 0;

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"pc_mic");
    os_mutex_post(&mic_rec_mutex);
    g_pc_mic_state = PC_MIC_STA_CLOSE;
}

//重启pc mic
static void pc_mic_recoder_restart(void)
{
    if (g_pc_mic_state == PC_MIC_STA_OPEN) {
        pc_mic_recoder_close();
        pc_mic_recoder_open();
    }
}

bool pc_mic_recoder_runing()
{
    return g_pc_mic_recoder != NULL;
}

int pc_mic_recoder_open_by_taskq(void)
{
    int msg[2];
    int ret = 0;
    if (g_pc_mic_state == PC_MIC_STA_CLOSE ||
        g_pc_mic_state == PC_MIC_STA_WAIT_CLOSE) {

        g_pc_mic_state = PC_MIC_STA_WAIT_OPEN;
        msg[0] = (int)pc_mic_recoder_open;
        msg[1] = 0;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    }
    return ret;
}

int pc_mic_recoder_close_by_taskq(void)
{
    int msg[2];
    int ret = 0;
    if (g_pc_mic_state == PC_MIC_STA_OPEN ||
        g_pc_mic_state == PC_MIC_STA_WAIT_OPEN) {

        g_pc_mic_state = PC_MIC_STA_WAIT_CLOSE;
        msg[0] = (int)pc_mic_recoder_close;
        msg[1] = 0;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    }
    return ret;
}

int pc_mic_recoder_restart_by_taskq(void)
{
    int msg[2];
    int ret = 0;
    msg[0] = (int)pc_mic_recoder_restart;
    msg[1] = 0;
    ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    return ret;
}

static void pc_mic_set_volume(int mic_vol)
{
    pcmic_volume_wait_set_flag = 0;
    if (app_get_current_mode()->name == APP_MODE_PC) {
        s16 volume = (u16)mic_vol;

        struct volume_cfg cfg = {0};
        cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
        cfg.cur_vol = volume;

        int err = jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, "Vol_PcMic", (void *)&cfg, sizeof(struct volume_cfg));
        log_debug(">>> pc mic vol: %d, ret:%d", mic_vol, err);
    }
}

int pc_mic_set_volume_by_taskq(u32 mic_vol)
{
    int ret = 0;
    if (pcmic_volume_wait_set_flag == 0) {
        int msg[3];
        msg[0] = (int)pc_mic_set_volume;
        msg[1] = 1;
        msg[2] = (int)mic_vol;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, sizeof(msg) / sizeof(int), msg);
        if (ret == 0) {
            pcmic_volume_wait_set_flag = 1;
        }
    }
    return ret;
}

#endif


