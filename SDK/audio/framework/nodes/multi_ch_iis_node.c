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

#if TCFG_MULTI_CH_IIS_NODE_ENABLE

#define IIS_LOG_ENABLE          0
#if IIS_LOG_ENABLE
#define LOG_TAG     "[MIIS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif

#define MODULE_IDX_SEL ((hdl_node(hdl)->uuid == NODE_UUID_MULTI_CH_IIS0_TX) ? 0 : 1)
extern const int config_media_tws_en;
extern const float const_out_dev_pns_time_ms;

#define HW_CH_INIT  1
#define HW_CH_START 2
#define HW_CH_STOP  3
#define HW_CH_PAUSE 4

#ifndef IIS_USE_DOUBLE_BUF_MODE_EN
#define IIS_USE_DOUBLE_BUF_MODE_EN 0
#endif


#define MASTER_IIS_DEBUG 0


struct iis_ch_hdl {
    struct stream_frame *frame;
    struct audio_iis_channel iis_ch;
    enum stream_scene scene;
    u8 ch_status;
    u8 syncts_enabled;
    u8 timestamp_ok;
    u8 reference_network;
    u8 time_out_ts_not_align;//超时的时间戳是否需要丢弃
    u8 force_write_slience_data;
    int value;
};

struct iis_node_hdl {
    char name[16];
    u8 bit_width;
    u8 module_idx;
    u8 nch;
    u8 params_init;
    u8 master_ch_idx;
    u8 hw_ch_num;
    int sample_rate;
    struct audio_iis_channel_attr attr;
    struct iis_ch_hdl iis[4];
};
struct miis_gloabl_hdl {
    struct list_head head;
};
static OS_MUTEX hw_mutex[2];
static int miis_global_hdl_init()
{
    os_mutex_create(&hw_mutex[0]);
    os_mutex_create(&hw_mutex[1]);
    return 0;
}
platform_initcall(miis_global_hdl_init);



struct iis_write_cb_t {
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

static void iis_adapter_fade_in(struct iis_node_hdl *hdl, void *buf, int len, u8 ch_idx)
{
    struct iis_ch_hdl *iis = &hdl->iis[ch_idx];
    if (iis->iis_ch.fade_out) {
        if (iis->value < 16384) {
            int fade_ms = 100;//ms
            int fade_step = 16384 / (fade_ms * hdl->sample_rate / 1000);
            if (!hdl->bit_width) {
                iis->value = jlstream_fade_in(iis->value, fade_step, buf, len, hdl->nch);
            } else {
                iis->value = jlstream_fade_in_32bit(iis->value, fade_step, buf, len, hdl->nch);
            }
        } else {
            iis->iis_ch.fade_out = 0;
        }
    }

}

static int iis_adpater_detect_multi_timestamp(struct iis_node_hdl *hdl, struct stream_frame *frame, u8 ch_idx)
{
    struct iis_ch_hdl *iis = &hdl->iis[ch_idx];
    int diff = 0;
    int diff2 = 0;
    u32 current_time = 0;

    if (frame->flags & FRAME_FLAG_SYS_TIMESTAMP_ENABLE) {
        // sys timestamp
        return 0;
    }

    if (!(frame->flags & FRAME_FLAG_TIMESTAMP_ENABLE)) {
        if (!iis->force_write_slience_data) { //无播放同步时，强制填一段静音包
            iis->force_write_slience_data = 1;
            int slience_time_us = (hdl->attr.protect_time ? hdl->attr.protect_time : 8) * 1000;
            int slience_frames = (u64)slience_time_us * hdl->sample_rate / 1000000;
            int filled_frames[4] = {0};
            struct multi_ch ch = {0};
            for (int i = 0; i < 4; i++) {
                if (hdl->attr.ch_idx & BIT(i)) {
                    ch.ch[i] = &hdl->iis[i].iis_ch;
                }
            }
            if (slience_frames) {
                audio_iis_multi_channel_fill_slience_frames(&ch, slience_frames, filled_frames);
            }
        }
        iis->syncts_enabled = 1;
        return 0;
    }

    if (iis->syncts_enabled) {
        audio_iis_syncts_trigger_with_timestamp(&iis->iis_ch, frame->timestamp);
        return 0;
    }

    u32 local_network = 0;
    u32 reference_time = 0;
    u8 net_addr[6];
    u8 network = audio_reference_clock_network(net_addr);
#if MASTER_IIS_DEBUG
    printf("-------------iis network %s, %d, %d\n", hdl->name, network, iis->reference_network);
#endif
    if (network != ((u8) - 1)) {
        reference_time = audio_reference_clock_time();
        if (reference_time == (u32) - 1) {
            goto syncts_start;
        }
        if (network == 2) {
            current_time = reference_time * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&iis->iis_ch), hdl->sample_rate);
        } else {
            current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&iis->iis_ch), hdl->sample_rate);
        }
#if MASTER_IIS_DEBUG
        putchar('@');
#endif
    }
    if (iis->reference_network == AUDIO_NETWORK_LOCAL) {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_iis_data_len(&iis->iis_ch), hdl->sample_rate);
        local_network = 1;
    }

    diff = frame->timestamp - current_time;
    diff /= TIMESTAMP_US_DENOMINATOR;
    if (diff < 0) {
        /* if (__builtin_abs(diff) <= 1000) { */
        /* goto syncts_start; */
        /* } */
        if (iis->time_out_ts_not_align) { //不做对齐
            if (local_network && !config_media_tws_en) {//本地网络时钟，不需要时间戳对齐
                iis->syncts_enabled = 1;
                audio_iis_syncts_trigger_with_timestamp(&iis->iis_ch, frame->timestamp);
                return 0;
            }
        }

        log_debug("F");
        return 2;
    }

    if (diff > 1000000) {
        log_debug("iis node timestamp error : %u, %u, %u, diff : %d\n", frame->timestamp, current_time, reference_time, diff);
    }
    int slience_frames = (u64)diff * hdl->sample_rate / 1000000;

    u32 dma_len = audio_iis_fix_dma_len(hdl->module_idx, TCFG_AUDIO_DAC_BUFFER_TIME_MS, AUDIO_IIS_IRQ_POINTS, hdl->bit_width, hdl->nch);
    int point_offset = hdl->bit_width ? 2 : 1;
    int max_frames = (dma_len >> point_offset) / hdl->nch - 4;
    if (slience_frames > max_frames) {
        slience_frames = max_frames;
    }

    int filled_frames[4] = {0};
    struct multi_ch ch = {0};
    for (int i = 0; i < 4; i++) {
        if (hdl->attr.ch_idx & BIT(i)) {
            ch.ch[i] = &hdl->iis[i].iis_ch;
        }
    }
    if (slience_frames) {
        audio_iis_multi_channel_fill_slience_frames(&ch, slience_frames, filled_frames);
    }
    int ret = 0;
    for (int i = 0; i < 4; i++) {
        if (hdl->attr.ch_idx & BIT(i)) {
            log_debug("fill:iis ch[%d] filled_frames %d, slience_frames %d\n", i, filled_frames[i], slience_frames);
            if (filled_frames[i] < slience_frames) {
                ret = 1;
                continue;
            }
        }
    }
    if (ret) {
        return 1;
    }

syncts_start:

    /* printf("< %s, %u %u %d>-\n", hdl->name, frame->timestamp, current_time, diff); */
    for (int i = 0; i < 4; i++) {
        hdl->iis[i].syncts_enabled = 1;
    }
    audio_iis_syncts_trigger_with_timestamp(&iis->iis_ch, frame->timestamp);

    return 0;
}

void muliti_ch_iis_node_write_callback_add(const char *name, u8 scene, void (*cb)(void *, int))
{
    if (!iis_gloabl_hdl.init) {
        iis_gloabl_hdl.init = 1;
        INIT_LIST_HEAD(&iis_gloabl_hdl.head);
    }
    struct iis_write_cb_t *bulk = zalloc(sizeof(struct iis_write_cb_t));
    bulk->name = name;
    bulk->write_callback = cb;
    bulk->scene = scene;
    list_add(&bulk->entry, &iis_gloabl_hdl.head);
}

void muliti_ch_iis_node_write_callback_del(const char *name)
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




static void muliti_ch_iis_node_write_callback_deal(u8 scene, struct stream_frame *frame)
{
    struct iis_write_cb_t *bulk;
    struct iis_write_cb_t *temp;
    if (iis_gloabl_hdl.init) {
        list_for_each_entry_safe(bulk, temp, &iis_gloabl_hdl.head, entry) {
            if (bulk->write_callback) {
                if ((bulk->scene == scene) || (bulk->scene == 0XFF)) {
                    bulk->write_callback(frame->data, frame->len);
                }
            }
        }
    }
}


static int iis_detect_multi_timestamp(struct iis_node_hdl *hdl, struct stream_frame *frame)
{
    if (!frame) {
        return 1;
    }
    struct iis_ch_hdl *iis = &hdl->iis[hdl->master_ch_idx];
    int point_offset = hdl->bit_width ? 2 : 1;
    int err = iis_adpater_detect_multi_timestamp(hdl, frame, hdl->master_ch_idx);
    if (err == 1) { //需要继续填充数据至时间戳
        for (int i = 0; i < 4; i++) {
            hdl->iis[i].timestamp_ok = 0;
        }
        return 0;
    }

    if (err == 2) { //需要直接丢弃改帧数据
        audio_iis_force_use_syncts_frames(&iis->iis_ch, (frame->len >> point_offset) / hdl->nch, frame->timestamp);
        for (int i = 0 ; i < 4; i++) {
            if (hdl->iis[i].frame) {
                jlstream_free_frame(hdl->iis[i].frame);
                hdl->iis[i].frame = NULL;
                hdl->iis[i].timestamp_ok = 0;
            }
        }
        return 2;
    }
    for (int i = 0 ; i < 4; i++) { //check
        hdl->iis[i].timestamp_ok = 1;
#if !IIS_USE_DOUBLE_BUF_MODE_EN//MASTER_IIS_DEBUG
        if (hdl->attr.ch_idx & BIT(i)) {
            if (audio_iis_get_buffered_frames(hdl->iis[i].iis_ch.iis, i) < 10) {
                if (hdl->module_idx) {
                    putchar('S');
                } else {
                    putchar('s');
                }
                putchar('A' + i);
                log_debug("module[%d] iis[%d] will empty\n", hdl->module_idx, i);
            }
        }
#endif
    }

    return 1;
}

static int iis_multi_write(struct iis_node_hdl *hdl)
{
    int *data = zalloc(sizeof(int) * 4);
    int *remain = zalloc(sizeof(int) * 4);
    int *wlen = zalloc(sizeof(int) * 4);

    struct multi_ch mch = {0};
    for (int i = 0; i < 4; i++) {
        if (hdl->attr.ch_idx & BIT(i)) {
            mch.ch[i] = &hdl->iis[i].iis_ch;
            if (hdl->iis[i].frame) {
                data[i] = (int)(hdl->iis[i].frame->data + hdl->iis[i].frame->offset);
                remain[i] = hdl->iis[i].frame->len - hdl->iis[i].frame->offset;
                iis_adapter_fade_in(hdl, (void *)data[i], remain[i], i);
            }
        }
    }
    audio_iis_multi_channel_write(&mch, data, remain, wlen);
    u8 num = 0;
    for (int i = 0; i < 4; i++) {
        if (hdl->iis[i].frame) {
            hdl->iis[i].frame->offset += wlen[i];
            if (wlen[i] < remain[i]) {
                num++;
            }
        }
    }
    free(data);
    free(remain);
    free(wlen);
    if (num == hdl->hw_ch_num) { //未消耗完
        return 0;
    }

    return 1;
}
static int iis_free(struct iis_node_hdl *hdl, struct stream_frame *frame, u8 ch_idx)
{
    struct iis_ch_hdl *iis = &hdl->iis[ch_idx];
    if (frame->offset == frame->len) {
        jlstream_free_frame(frame);
        iis->frame = NULL;
    }
    return 0;
}

static int iis_ioc_get_delay(struct iis_node_hdl *hdl, struct audio_iis_channel *ch);
__NODE_CACHE_CODE(iis)
static int iis_iport_write(struct stream_iport *iport, struct stream_note *note)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];

    int ret = 0;
    if (!iis->frame) {
        iis->frame = jlstream_pull_frame(iport, NULL);
        if (!iis->frame) {
            return 0;
        }
        iis->frame->offset = 0;
        muliti_ch_iis_node_write_callback_deal(iis->scene, iis->frame);
    }
    for (int i = 0; i < 4; i++) {
        if (hdl->attr.ch_idx & BIT(i)) {
            if (!hdl->iis[i].frame) {
                ret++;
            }
        }
    }

    if (ret) { //未全部拿到数据
        return 0;
    }
    //将同步时间戳的检测行为和iis写行为分开，避免多通道写的中间接入较多延时，引起通道之间的相位差
    os_mutex_pend(&hw_mutex[hdl->module_idx], 0);
    ret = iis_detect_multi_timestamp(hdl, hdl->iis[hdl->master_ch_idx].frame);
    if (ret == 2) {//有超时释放德帧，重新取数据
        os_mutex_post(&hw_mutex[hdl->module_idx]);
        return 1;
    } else if (ret == 0) {
        os_mutex_post(&hw_mutex[hdl->module_idx]);
        return 1;
    }

    //多通道写数
    ret = iis_multi_write(hdl);


    for (int i = 0; i < 4; i++) {
        if (hdl->iis[i].frame) {
            iis_free(hdl, hdl->iis[i].frame, i);
            /* printf("aft:ch[%d] delay:%d\n", i, (int)iis_ioc_get_delay(hdl, &hdl->iis[i].iis_ch)); */
        }
    }
    os_mutex_post(&hw_mutex[hdl->module_idx]);
    if (!ret) { //写不进去了
        note->state |= NODE_STA_OUTPUT_BLOCKED;
        return 0;
    }
    note->state &= ~NODE_STA_OUTPUT_BLOCKED;

    return 1;
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


static int iis_ioc_get_delay(struct iis_node_hdl *hdl, struct audio_iis_channel *ch);
__NODE_CACHE_CODE(iis)
static void iis_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    struct stream_node *node = iport->node;
    struct stream_iport *_iport = iport;
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];

    os_mutex_pend(&hw_mutex[hdl->module_idx], 0);
    if (iis->ch_status == HW_CH_INIT) {
        struct multi_ch mch = {0};
        for (int i = 0; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                mch.ch[i] = &hdl->iis[i].iis_ch;
            }
        }
        audio_iis_multi_channel_start(&mch);

        for (int i = 0 ; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                iis = &hdl->iis[i];
                iis->ch_status = HW_CH_START;
                if (!audio_iis_ch_alive(iis_hdl[hdl->module_idx], i)) {
                    hdl->params_init = 0;
                    log_error("iis mode_index %d, cur ch %d is not init\n", hdl->module_idx, i);
                    continue;
                }
            }
        }
    }
    os_mutex_post(&hw_mutex[hdl->module_idx]);


    int ret = 0;
    /*
     * 遍历iport
     */
    _iport = iport;

    do {
        ret = 0;
        do {
            ret += iis_iport_write(_iport, note);
            _iport = _iport->sibling;
            if (!_iport) {
                _iport = hdl_node(hdl)->iport;
            }
        } while (_iport != iport);
    } while (ret);


}

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
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];
    u32 sample_rate = 0;

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

    if (iis->ch_status || audio_iis_is_working(iis_hdl[hdl->module_idx])) {
        sample_rate = audio_iis_get_sample_rate(iis_hdl[hdl->module_idx]);
        if (in_fmt->sample_rate != sample_rate) {
            if (in_fmt->sample_rate > sample_rate && sample_rate < 44100) {
                if (params->sample_rate == 0 && !(iis->scene & STREAM_SCENE_TONE)) {
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
        if (iis->scene != STREAM_SCENE_ESCO) {
            if (hdl->sample_rate) {
                sample_rate = hdl->sample_rate;
            } else {
                if (params->sample_rate) {
                    sample_rate = params->sample_rate;
                } else {
                    sample_rate = in_fmt->sample_rate;
                }
            }
        } else {
            sample_rate = in_fmt->sample_rate;
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
    //log_debug("iis out ch_num %d, sr %d, %d\n", hdl->nch, hdl->sample_rate, params->sample_rate);

    return ret;
}



static void iis_adapter_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = iis_handle_frame;
}

static void iis_ioc_start(struct iis_node_hdl *hdl, struct stream_iport *iport)
{
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];
    u8 ch_idx = iport->id;
    hdl->bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;

    if (hdl->params_init == 0) {
        u8 cfg_index = scene_cfg_index_get(iis->scene);
        int ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, cfg_index, (void *)&hdl->attr, hdl->name);
        if (!ret) {
            ret = jlstream_read_node_data_by_cfg_index(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, 0, (void *)&hdl->attr, hdl->name);
        }
        if (!ret) {
            hdl->attr.ch_idx = BIT(0);
            hdl->attr.delay_time = 50;
            hdl->attr.protect_time = 8;
            hdl->attr.write_mode = WRITE_MODE_BLOCK;
            log_error("iis node param read err, set default\n");
        }
        u8 select_idx = 0xff;
        hdl->hw_ch_num = 0;
        for (int i = 0; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                if (select_idx == 0xff) {
                    select_idx = i;//选择找到的第一个做同步
                }
                hdl->hw_ch_num++;
            }
        }
        hdl->master_ch_idx = select_idx;
        if (select_idx == 0xff) {
            log_error("multi ch iis node cfg err\n");
            return;
        }

        hdl->params_init = 1;
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
        hdl->params_init = 0;
        log_error("iis out init err\n");
        return;
    }

    u8 select = 0;
    for (int i = 0; i < 4; i++) {
        if (hdl->attr.ch_idx & BIT(i)) {
            if (ch_idx == i) {
                select = 1;
                break;
            }
        }
    }

    if (!select) {
        log_error("please select iis[%d]ch[%d]\n", hdl->module_idx, ch_idx);
        return;
    }
    log_debug(">>>>>module_idx[%d] iis ch[%d] channel fifo init<<<<<<<\n", hdl->module_idx, ch_idx);
    audio_iis_new_channel(iis_hdl[hdl->module_idx], (void *)&iis->iis_ch);
    struct audio_iis_channel_attr attr = {0};
    memcpy(&attr, &hdl->attr, sizeof(attr));
    attr.ch_idx = ch_idx;//界面参数的ch_idx内存储多个通道的选配信息，此处需要做个转换后，再更新给对应通道
    audio_iis_channel_set_attr(&iis->iis_ch, &attr);
    iis->ch_status = HW_CH_INIT;

    audio_iis_check_hw_cfg_status(hdl->module_idx, ch_idx, ALINK_DIR_TX);

}

static void iis_ioc_stop(struct stream_iport *iport)
{
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;

    os_mutex_pend(&hw_mutex[hdl->module_idx], 0);
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];
    if (iis->frame) {
        jlstream_free_frame(iis->frame);
        iis->frame = NULL;
    }

    if (iis->ch_status && iis_hdl[hdl->module_idx]) {
        audio_iis_channel_close((void *)&iis->iis_ch);
        int ret = audio_iis_uninit(iis_hdl[hdl->module_idx]);
        if (ret) {
            iis_hdl[hdl->module_idx] = NULL;
            log_debug("hw[%d] iis hdl:%x", hdl->module_idx, (int)iis_hdl[hdl->module_idx]);
            log_debug(">>>>>>>>>>>>>>>>>>>>>>>>tx uninit <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        }
    }

    iis->ch_status = 0;
    iis->value = 0;
    iis->syncts_enabled = 0;
    iis->force_write_slience_data = 0;
    os_mutex_post(&hw_mutex[hdl->module_idx]);
}

static int iis_adapter_syncts_ioctl(struct stream_iport *iport, struct audio_syncts_ioc_params *params)
{

    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];
    if (!params) {
        return 0;
    }
    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        log_debug("=====iis node name: %s, ch[%d]\n", hdl->name, iport->id);
        os_mutex_pend(&hw_mutex[hdl->module_idx], 0);
        audio_iis_add_syncts_with_timestamp(&iis->iis_ch, (void *)params->data[0], params->data[1]);
        os_mutex_post(&hw_mutex[hdl->module_idx]);
        if (iis->reference_network == 0xff) {
            iis->reference_network = params->data[2];
        }

        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        os_mutex_pend(&hw_mutex[hdl->module_idx], 0);
        audio_iis_remove_syncts_handle(&iis->iis_ch, (void *)params->data[0]);
        os_mutex_post(&hw_mutex[hdl->module_idx]);
        break;
    }
    return 0;
}

static int iis_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)iport->node->private_data;
    struct iis_ch_hdl *iis = &hdl->iis[iport->id];

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        iis_adapter_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= iis_ioc_negotiate(iport, *(int *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        iis->scene = arg;
        break;
    case NODE_IOC_GET_DELAY:
        int self_delay = iis_ioc_get_delay(hdl, &iis->iis_ch);
        return self_delay;
    case NODE_IOC_START:
        iis_ioc_start(hdl, iport);
        break;
    case NODE_IOC_SUSPEND:
        do {
            if (iis->frame) {
                jlstream_free_frame(iis->frame);
                iis->frame = NULL;
            }
            iis->frame = jlstream_pull_frame(iport, NULL);
        } while (iis->frame);
    case NODE_IOC_STOP:
        iis_ioc_stop(iport);
        break;
    case NODE_IOC_SYNCTS:
        iis_adapter_syncts_ioctl(iport, (struct audio_syncts_ioc_params *)arg);
        break;
    case NODE_IOC_GET_ODEV_CACHE:
        return audio_iis_data_len(&iis->iis_ch);
    case NODE_IOC_SET_PARAM:
        iis->reference_network = arg;
        break;
    case NODE_IOC_SET_TS_PARM:
        iis->time_out_ts_not_align = arg;
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
    struct iis_node_hdl *hdl = (struct iis_node_hdl *)node->private_data;

    hdl->iis[0].reference_network = 0xff;
    hdl->iis[1].reference_network = 0xff;
    hdl->iis[2].reference_network = 0xff;
    hdl->iis[3].reference_network = 0xff;

    return 0;
}


static void iis_adapter_release(struct stream_node *node)
{
}

REGISTER_STREAM_NODE_ADAPTER(multi_ch_iis0_node_adapter) = {
    .name       = "multi_ch_iis0",
    .uuid       = NODE_UUID_MULTI_CH_IIS0_TX,
    .bind       = iis_adapter_bind,
    .ioctl      = iis_adapter_ioctl,
    .release    = iis_adapter_release,
    .hdl_size   = sizeof(struct iis_node_hdl),
};
REGISTER_STREAM_NODE_ADAPTER(multi_ch_iis1_node_adapter) = {
    .name       = "multi_ch_iis1",
    .uuid       = NODE_UUID_MULTI_CH_IIS1_TX,
    .bind       = iis_adapter_bind,
    .ioctl      = iis_adapter_ioctl,
    .release    = iis_adapter_release,
    .hdl_size   = sizeof(struct iis_node_hdl),
};

#endif

