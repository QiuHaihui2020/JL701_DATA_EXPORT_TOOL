#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iis_node.data.bss")
#pragma data_seg(".iis_node.data")
#pragma const_seg(".iis_node.text.const")
#pragma code_seg(".iis_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "circular_buf.h"
#include "audio_splicing.h"
#include "audio_dai/audio_iis.h"
#include "app_config.h"
#include "gpio.h"
#include "audio_cvp.h"
#include "media/audio_general.h"
#include "reference_time.h"
#include "audio_config_def.h"
#include "effects/effects_adj.h"
#include "media/audio_dev_sync.h"

#if TCFG_IIS_NODE_ENABLE

#define IIS_LOG_ENABLE          0
#if IIS_LOG_ENABLE
#define LOG_TAG     "[IIS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif

#define MODULE_IDX_SEL ((hdl_node(hdl)->uuid == NODE_UUID_IIS0_TX) ? 0 : 1)
extern const int config_media_tws_en;
extern const float const_out_dev_pns_time_ms;

#ifndef IIS_USE_DOUBLE_BUF_MODE_EN
#define IIS_USE_DOUBLE_BUF_MODE_EN 0
#endif

#define MASTER_IIS_DEBUG 0

struct iis_node_hdl {
    char name[16];
    u8 iis_start;
    u8 iis_init;
    u8 bit_width;
    u8 module_idx;
    u8 syncts_enabled;
    u8 nch;
    u8 force_write_slience_data;
    u8 syncts_mount_en;        /*响应前级同步节点的挂载动作，默认1,,特殊流程才会配置0*/
    int sample_rate;
    struct stream_frame *frame;
    enum stream_scene scene;
    struct audio_iis_channel iis_ch;
    struct audio_iis_channel_attr attr;
    void *dev_sync;


    int value;
    u8 timestamp_ok;
    u8 reference_network;
    u8 time_out_ts_not_align;//超时的时间戳是否需要丢弃
};

struct iis_write_cb_t {
    u8 is_after_write_over;
    u8 scene;
    const char *name;
    struct list_head entry;
    void (*write_callback)(void *buf, int len);
};

struct iis_gloabl_hdl_t {
    u8 init;
    struct list_head head;
};

static struct iis_gloabl_hdl_t iis_gloabl_hdl;

static void iis_adapter_fade_in(struct iis_node_hdl *hdl, void *buf, int len)
{
    if (hdl->iis_ch.fade_out) {
        if (hdl->value < 16384) {
            int fade_ms = 100;//ms
            int fade_step = 16384 / (fade_ms * hdl->sample_rate / 1000);
            if (!hdl->bit_width) {
                hdl->value = jlstream_fade_in(hdl->value, fade_step, buf, len, hdl->nch);
            } else {
                hdl->value = jlstream_fade_in_32bit(hdl->value, fade_step, buf, len, hdl->nch);
            }
        } else {
            hdl->iis_ch.fade_out = 0;
        }
    }

}


static int iis_adpater_detect_timestamp(struct iis_node_hdl *hdl, struct stream_frame *frame)
{
    int diff = 0;
    u32 current_time = 0;

    if (frame->flags & FRAME_FLAG_SYS_TIMESTAMP_ENABLE) {
        // sys timestamp
        return 0;
    }

    if (!(frame->flags & FRAME_FLAG_TIMESTAMP_ENABLE)) {
        if (!hdl->force_write_slience_data) { //无播放同步时，强制填一段静音包
            hdl->force_write_slience_data = 1;
            int slience_time_us = (hdl->attr.protect_time ? hdl->attr.protect_time : 8) * 1000;
            int slience_frames = (u64)slience_time_us * hdl->sample_rate / 1000000;
            int filled_frames = audio_iis_fill_slience_frames(&hdl->iis_ch, slience_frames);
        }
        hdl->syncts_enabled = 1;
        return 0;
    }

    if (hdl->syncts_enabled) {
        audio_iis_syncts_trigger_with_timestamp(&hdl->iis_ch, frame->timestamp);
        return 0;
    }

    /*printf("----[%d %d %d]----\n", bt_audio_reference_clock_time(0xff), audio_iis_data_len(&hdl->iis_ch), hdl->sample_rate);*/
    u32 local_network = 0;
    u32 reference_time = 0;
    u8 net_addr[6];
    u8 network = audio_reference_clock_network(net_addr);
#if MASTER_IIS_DEBUG
    printf("-------------iis network %s, %d, %d\n", hdl->name, network, hdl->reference_network);
#endif
    if (network != ((u8) - 1)) {
        reference_time = audio_reference_clock_time();
        if (reference_time == (u32) - 1) {
            goto syncts_start;
        }
        /* current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_dac_buffered_frames(&dac_hdl), hdl->sample_rate); */
        if (network == 2) {
            current_time = reference_time * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&hdl->iis_ch), hdl->sample_rate);
        } else {
            current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&hdl->iis_ch), hdl->sample_rate);
        }
#if MASTER_IIS_DEBUG
        putchar('@');
#endif
    } else {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&hdl->iis_ch), hdl->sample_rate);
        local_network = 1;
    }
    if (hdl->reference_network == AUDIO_NETWORK_LOCAL) {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&hdl->iis_ch), hdl->sample_rate);
        local_network = 1;
    }
    diff = frame->timestamp - current_time;
    diff /= TIMESTAMP_US_DENOMINATOR;
    if (diff < 0) {
        if (__builtin_abs(diff) <= 1000) {
            goto syncts_start;
        }
        if (hdl->time_out_ts_not_align) { //不做对齐
            if (local_network && !config_media_tws_en) {//本地网络时钟，不需要时间戳对齐
                hdl->syncts_enabled = 1;
                audio_iis_syncts_trigger_with_timestamp(&hdl->iis_ch, frame->timestamp);
#if MASTER_IIS_DEBUG
                putchar('V');
#endif
                return 0;
            }
        }
#if MASTER_IIS_DEBUG
        putchar('C');
        printf("[%s, %u %u %d]\n", hdl->name, frame->timestamp, current_time, diff);
#endif
        return 2;
    }

    if (diff > 1000000) {
        printf("iis node timestamp error : %s, %u, %u, %u, diff : %d\n", hdl->name, frame->timestamp, current_time, reference_time, diff);
    }
    int slience_frames = (u64)diff * hdl->sample_rate / 1000000;

    int filled_frames = audio_iis_fill_slience_frames(&hdl->iis_ch, slience_frames);

#if MASTER_IIS_DEBUG
    putchar('F');
    printf("iis[%d] slience_frames %d\n", hdl->attr.ch_idx, filled_frames);
#endif
    if (filled_frames < slience_frames) {
        return 1;
    }

syncts_start:

    printf("< %s, %u %u %d>-\n", hdl->name, frame->timestamp, current_time, diff);
    hdl->syncts_enabled = 1;
    audio_iis_syncts_trigger_with_timestamp(&hdl->iis_ch, frame->timestamp);

    return 0;
}

void iis_node_write_callback_add(const char *name, u8 is_after_write_over, u8 scene, void (*cb)(void *, int))
{
    if (!iis_gloabl_hdl.init) {
        iis_gloabl_hdl.init = 1;
        INIT_LIST_HEAD(&iis_gloabl_hdl.head);
    }
    struct iis_write_cb_t *bulk = zalloc(sizeof(struct iis_write_cb_t));
    bulk->name = name;
    bulk->write_callback = cb;
    bulk->scene = scene;
    bulk->is_after_write_over = is_after_write_over;
    list_add(&bulk->entry, &iis_gloabl_hdl.head);
}

void iis_node_write_callback_del(const char *name)
{
    struct iis_write_cb_t *bulk;
    struct iis_write_cb_t *temp;
    list_for_each_entry_safe(bulk, temp, &iis_gloabl_hdl.head, entry) {
        if (!strcmp(bulk->name, name)) {
            list_del(&bulk->entry);
            free(bulk);
        }
    }
}

static int dev_sync_output_handler(void *_hdl, void *data, int len)
{
    struct iis_node_hdl *hdl = _hdl;


    int wlen = audio_iis_channel_write((void *)&hdl->iis_ch, data, len);
    if (config_dev_sync_enable) {
        dev_sync_calculate_output_samplerate(hdl->dev_sync, wlen, audio_iis_data_len(&hdl->iis_ch));
    }
    return wlen;
}

static void dev_sync_start(struct stream_iport *iport)
{
    if (config_dev_sync_enable) {
        struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            struct dev_sync_params params = {0};
            params.in_sample_rate = hdl->sample_rate;
            params.out_sample_rate = hdl->sample_rate;
            params.channel = hdl->nch;
            params.bit_width = iport->prev->fmt.bit_wide;
            params.priv = hdl;
            params.handle = dev_sync_output_handler;
            if (!hdl->dev_sync) {
                hdl->dev_sync = dev_sync_open(&params);
            }
        }
    }
}

static void iis_synchronize_with_main_device(struct stream_iport *iport, struct stream_frame *frame)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    if ((hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) && hdl->timestamp_ok) {
        struct sync_rate_params params = {0};
        params.d_sample_rate = frame->d_sample_rate;
        params.buffered_frames = audio_iis_channel_latch_time(&hdl->iis_ch, &params.current_time, dev_sync_latch_time, hdl->reference_network);
        params.timestamp = frame->timestamp;
        params.name = hdl->name;
        dev_sync_update_rate(hdl->dev_sync, &params);
    }
}



static void iis_node_write_callback_deal(u8 is_after_write_over, struct iis_node_hdl *hdl, void *data, int len)
{
    struct iis_write_cb_t *bulk;
    struct iis_write_cb_t *temp;
    if (iis_gloabl_hdl.init) {
        list_for_each_entry_safe(bulk, temp, &iis_gloabl_hdl.head, entry) {
            if (bulk->write_callback) {
                if ((bulk->scene == hdl->scene) || (bulk->scene == 0XFF)) {
                    if (is_after_write_over == bulk->is_after_write_over) {
                        bulk->write_callback(data, len);
                    }
                }
            }
        }
    }
}


__NODE_CACHE_CODE(iis)
static void iis_write_data(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct stream_node *node = iport->node;
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    u16 point_offset = 1;
    if (iport->prev->fmt.bit_wide) {
        point_offset = 2;
    }
    while (1) {
        frame = jlstream_pull_frame(iport, NULL);
        if (!frame) {
            break;
        }
        if (config_dev_sync_enable) {
            if (frame->offset == 0) {
                iis_synchronize_with_main_device(iport, frame);
            }
        }

        iis_node_write_callback_deal(0, hdl, frame->data, frame->len);

        int err = iis_adpater_detect_timestamp(hdl, frame);
        if (err == 1) { //需要继续填充数据至时间戳
            hdl->timestamp_ok = 0;
            jlstream_return_frame(iport, frame);
            break;
        }

        if (err == 2) { //需要直接丢弃改帧数据
            audio_iis_force_use_syncts_frames(&hdl->iis_ch, (frame->len >> point_offset) / hdl->nch, frame->timestamp);
            jlstream_free_frame(frame);
            hdl->timestamp_ok = 0;
            continue;
        }
        hdl->timestamp_ok = 1;
        s16 *data = (s16 *)(frame->data + frame->offset);
        int remain = frame->len - frame->offset;
        int wlen = 0;

#if !IIS_USE_DOUBLE_BUF_MODE_EN//MASTER_IIS_DEBUG
        if (audio_iis_get_buffered_frames(hdl->iis_ch.iis, hdl->attr.ch_idx) < 10) {
            if (hdl->module_idx) {
                putchar('T');
            } else {
                putchar('t');
            }
            putchar('A' + hdl->attr.ch_idx);
            log_debug("module[%d] iis[%d] will empty\n", hdl->module_idx, hdl->attr.ch_idx);
        }
#endif
        iis_adapter_fade_in(hdl, data, remain);
        if (hdl->dev_sync) {
            if (config_dev_sync_enable) {
                wlen = dev_sync_write(hdl->dev_sync, data, remain);
            }
        } else {
            wlen = audio_iis_channel_write((void *)&hdl->iis_ch, data, remain);
        }
        iis_node_write_callback_deal(1, hdl, data, wlen);

        frame->offset += wlen;
        if (wlen < remain) {
            note->state |= NODE_STA_OUTPUT_BLOCKED;
            jlstream_return_frame(iport, frame);
            break;
        }
        note->state &= ~NODE_STA_OUTPUT_BLOCKED;
        jlstream_free_frame(frame);
    }
}

static u8 scene_cfg_index_get(u8 scene)
{
    //按可视化工具中的配置项顺序排列
    switch (scene) {
    case STREAM_SCENE_A2DP:
        return 1;
    case STREAM_SCENE_LINEIN:
        return 2;
    case STREAM_SCENE_MUSIC:
        return 3;
    case STREAM_SCENE_FM:
        return 4;
    case STREAM_SCENE_SPDIF:
        return 5;
    default:
        return 0;
    }
}

#if MASTER_IIS_DEBUG
void *iis_ch_test;
int mast_iis_frames()
{
    extern void *iis_ch_test;
    printf("iis_ch[0] %d\n", audio_iis_get_buffered_frames(iis_ch_test, 0));
    return 0;
}
#endif

__NODE_CACHE_CODE(iis)
static void iis_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    /* struct stream_node *node = iport->node; */

    if (!hdl->iis_start) {
        return;
    }
    audio_iis_channel_start((void *)&hdl->iis_ch);
#if MASTER_IIS_DEBUG
    if (hdl->attr.ch_idx == 0) {
        iis_ch_test = hdl->iis_ch.iis;
    }
#endif
    if (!audio_iis_ch_alive(iis_hdl[hdl->module_idx], hdl->attr.ch_idx)) {
        hdl->iis_start = 0;
        log_error("iis mode_index %d, cur ch %d is not init\n", hdl->module_idx, hdl->attr.ch_idx);
        return;
    }

    dev_sync_start(iport);

    iis_write_data(iport, note);
}

// flag   0:time  1:points
static int iis_ioc_get_delay(struct iis_node_hdl *hdl, struct audio_iis_channel *ch)
{
    int len = audio_iis_data_len(ch);
    if (len == 0) {
        return 0;
    }
    int rate = audio_iis_get_sample_rate(iis_hdl[hdl->module_idx]);
    ASSERT(rate != 0);

    return (len * 10000) / rate;
}

static int iis_ioc_negotiate(struct stream_iport *iport, int nego_state)
{
    int ret = NEGO_STA_ACCPTED;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;

    struct audio_general_params *params = audio_general_get_param();
    u8 bit_width = audio_general_out_dev_bit_width();
    if (bit_width) {
        if (in_fmt->bit_wide != DATA_BIT_WIDE_24BIT) {
            in_fmt->bit_wide = DATA_BIT_WIDE_24BIT;
            ret = NEGO_STA_CONTINUE;
        }
    } else {
        if (in_fmt->bit_wide != DATA_BIT_WIDE_16BIT) {
            in_fmt->bit_wide = DATA_BIT_WIDE_16BIT;
            ret = NEGO_STA_CONTINUE;
        }
    }

    if (in_fmt->channel_mode != AUDIO_CH_LR) {
        in_fmt->channel_mode = AUDIO_CH_LR;
        ret = NEGO_STA_CONTINUE;
    }

    hdl->module_idx = MODULE_IDX_SEL;
    u32 sample_rate = 0;

    if (!iis_hdl[hdl->module_idx]) {//节点被打断suspend后，iis硬件被关闭的情况下，直接将iis_start状态清空，防止sr是0协商不过
        hdl->iis_start = 0;
    }
    if (hdl->iis_start || audio_iis_is_working(iis_hdl[hdl->module_idx])) {
        sample_rate = audio_iis_get_sample_rate(iis_hdl[hdl->module_idx]);
        if (in_fmt->sample_rate != sample_rate) {
            if (in_fmt->sample_rate > sample_rate && sample_rate < 44100) {
                if (params->sample_rate == 0 && !(hdl->scene & STREAM_SCENE_TONE)) {
                    return NEGO_STA_SUSPEND;
                }
            }
            if (nego_state & NEGO_STA_SAMPLE_RATE_LOCK) {
                return NEGO_STA_SUSPEND;
            }
            in_fmt->sample_rate = sample_rate;
            ret = NEGO_STA_CONTINUE;
        }
    } else {
        if (hdl->sample_rate) {
            sample_rate = hdl->sample_rate;
        } else {
            if (params->sample_rate) {
                sample_rate = params->sample_rate;
            } else {
                sample_rate = in_fmt->sample_rate;
            }
        }
        if (in_fmt->sample_rate != sample_rate) {
            if (!(nego_state & NEGO_STA_SAMPLE_RATE_LOCK)) {
                in_fmt->sample_rate = sample_rate;
                ret = NEGO_STA_CONTINUE;
            }
        }
    }

    hdl->sample_rate = in_fmt->sample_rate;
    hdl->nch = AUDIO_CH_NUM(in_fmt->channel_mode);
    log_debug("iis out ch_num %d, sr %d, %d\n", hdl->nch, hdl->sample_rate, params->sample_rate);

    // 保证上一个节点输入的是PCM格式，IIS输出数据格式目前只能是PCM
    if (in_fmt->coding_type != AUDIO_CODING_PCM) {
        /* r_printf("%s, %d, iis output nonsupport coding_type except PCM", __func__, __LINE__); */
        /* ret = NEGO_STA_ABORT; */
        //in_fmt->coding_type = AUDIO_CODING_PCM;
        //ret = NEGO_STA_CONTINUE;
    }

    return ret;
}



static void iis_adapter_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = iis_handle_frame;
}

static void iis_ioc_start(struct iis_node_hdl *hdl)
{
    /* open iis tx */
    if (hdl->iis_init == 0 || !iis_hdl[hdl->module_idx]) {
        hdl->iis_init = 1;
        /* 申请 iis需要的资源，初始化iis配置 */
        hdl->bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;

        if (hdl->iis_start == 0) {
            hdl->iis_start = 1;
            u8 cfg_index = scene_cfg_index_get(hdl->scene);
            int ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, cfg_index, (void *)&hdl->attr, hdl->name);
            if (!ret) {
                ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, 0, (void *)&hdl->attr, hdl->name);
            }
            if (!ret) {
                hdl->attr.ch_idx = 0;
                hdl->attr.delay_time = 50;
                hdl->attr.protect_time = 8;
                hdl->attr.write_mode = WRITE_MODE_BLOCK;
                log_error("iis node param read err, set default\n");
            }
            if (hdl->attr.dev_properties != SYNC_TO_MASTER_DEV) { //从设备不需要挂载前级的同步节点
                hdl->syncts_mount_en = 1;
            } else {
                hdl->syncts_mount_en = 0;
            }
        }

        if (!iis_hdl[hdl->module_idx]) {
            struct alink_param params = {0};
            params.module_idx = hdl->module_idx;
            params.dma_size   = audio_iis_fix_dma_len(hdl->module_idx, TCFG_AUDIO_DAC_BUFFER_TIME_MS, AUDIO_IIS_IRQ_POINTS, hdl->bit_width, hdl->nch);
            params.sr         = hdl->sample_rate;
            params.bit_width  = hdl->bit_width;
            params.fixed_pns  = const_out_dev_pns_time_ms;
            iis_hdl[hdl->module_idx] = audio_iis_init(params);
        }
        if (!iis_hdl[hdl->module_idx]) {
            hdl->iis_start = 0;
            log_error("iis out init err\n");
            return;
        }

        audio_iis_new_channel(iis_hdl[hdl->module_idx], (void *)&hdl->iis_ch);
        audio_iis_channel_set_attr(&hdl->iis_ch, &hdl->attr);

        audio_iis_check_hw_cfg_status(hdl->module_idx, hdl->attr.ch_idx, ALINK_DIR_TX);

    }
}

static void iis_ioc_stop(struct iis_node_hdl *hdl)
{
    if (config_dev_sync_enable) {
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            if (hdl->dev_sync) {
                dev_sync_close(hdl->dev_sync);
                hdl->dev_sync = NULL;
            }
        }
    }
    if (hdl->iis_start && iis_hdl[hdl->module_idx]) {
        audio_iis_channel_close((void *)&hdl->iis_ch);
        int ret = audio_iis_uninit(iis_hdl[hdl->module_idx]);
        if (ret) {
            iis_hdl[hdl->module_idx] = NULL;
            log_debug("hw[%d] iis hdl:%x", hdl->module_idx, (int)iis_hdl[hdl->module_idx]);
            log_debug(">>>>>>>>>>>>>>>>>>>>>>>>tx uninit <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        }
    }

    hdl->syncts_enabled = 0;
    hdl->force_write_slience_data = 0;
    hdl->iis_start = 0;
    hdl->value = 0;
    hdl->iis_init = 0;
}

static int iis_adapter_syncts_ioctl(struct iis_node_hdl *hdl, struct audio_syncts_ioc_params *params)
{
    if (!params) {
        return 0;
    }
    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        log_debug("=====iis node name: %s\n", hdl->name);
        if (hdl->syncts_mount_en) {
            audio_iis_add_syncts_with_timestamp(&hdl->iis_ch, (void *)params->data[0], params->data[1]);
        }
        if (hdl->reference_network == 0xff) {
            hdl->reference_network = params->data[2];
        }

        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        if (hdl->syncts_mount_en) {
            audio_iis_remove_syncts_handle(&hdl->iis_ch, (void *)params->data[0]);
        }
        break;
    }
    return 0;
}

static int iis_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        iis_adapter_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= iis_ioc_negotiate(iport, *(int *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_GET_DELAY:
        int self_delay = iis_ioc_get_delay(hdl, &hdl->iis_ch);
        if (hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            return self_delay - 50;//多输出从设备查询到延时减5ms，减小流程延迟导致依据的主设备延迟偏差
        }

        return self_delay;
    case NODE_IOC_START:
        iis_ioc_start(hdl);

        break;
    case NODE_IOC_SUSPEND:
        hdl->syncts_enabled = 0;
        hdl->force_write_slience_data = 0;
        if (hdl->iis_start == 0) {
            hdl->sample_rate = 0;
        }
        hdl->value = 0;
        break;

    case NODE_IOC_STOP:
        iis_ioc_stop(hdl);
        break;
    case NODE_IOC_SYNCTS:
        iis_adapter_syncts_ioctl(hdl, (struct audio_syncts_ioc_params *)arg);
        break;
    case NODE_IOC_GET_ODEV_CACHE:
        return audio_iis_data_len(&hdl->iis_ch);
    case NODE_IOC_SET_PARAM:
        hdl->reference_network = arg;
        break;
    case NODE_IOC_SET_TS_PARM:
        hdl->time_out_ts_not_align = arg;
        break;
    case NODE_IOC_SET_FMT:
        struct stream_fmt *fmt = (struct stream_fmt *)arg;
        hdl->sample_rate = fmt->sample_rate;
        break;
    default:
        break;
    }

    return ret;
}


static int iis_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)node->private_data ;

    hdl->reference_network = 0xff;
    return 0;
}


static void iis_adapter_release(struct stream_node *node)
{


}

REGISTER_STREAM_NODE_ADAPTER(iis_node_adapter) = {
    .name       = "iis",
    .uuid       = NODE_UUID_IIS0_TX,
    .bind       = iis_adapter_bind,
    .ioctl      = iis_adapter_ioctl,
    .release    = iis_adapter_release,
    .hdl_size   = sizeof(struct iis_node_hdl),
};
REGISTER_STREAM_NODE_ADAPTER(iis1_node_adapter) = {
    .name       = "iis1",
    .uuid       = NODE_UUID_IIS1_TX,
    .bind       = iis_adapter_bind,
    .ioctl      = iis_adapter_ioctl,
    .release    = iis_adapter_release,
    .hdl_size   = sizeof(struct iis_node_hdl),
};

#endif

