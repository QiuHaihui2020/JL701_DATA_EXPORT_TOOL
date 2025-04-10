#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_mic_node.data.bss")
#pragma data_seg(".pc_mic_node.data")
#pragma const_seg(".pc_mic_node.text.const")
#pragma code_seg(".pc_mic_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "circular_buf.h"
#include "audio_splicing.h"
#include "app_config.h"
#include "gpio.h"
#include "audio_cvp.h"
#include "media/audio_general.h"
#include "pc_mic_recoder.h"
#include "uac_stream.h"
#include "audio_config_def.h"
#include "effects/convert_data.h"
#include "media/audio_dev_sync.h"
#include "media/audio_cfifo.h"
#include "reference_time.h"


#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE


#define PCMIC_LOG_ENABLE          0
#if PCMIC_LOG_ENABLE
#define LOG_TAG     "[pcmic]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif

#define UAC_OUT_BUF_MAX_MS   TCFG_AUDIO_DAC_BUFFER_TIME_MS

//硬件通道的状态
#define PCMIC_STATE_INIT        0
#define PCMIC_STATE_OPEN        1
#define PCMIC_STATE_START       2
#define PCMIC_STATE_PREPARE     3
#define PCMIC_STATE_STOP        4
#define PCMIC_STATE_UNINIT

#define AUDIO_CFIFO_IDLE      0
#define AUDIO_CFIFO_INITED    1
#define AUDIO_CFIFO_START     2
#define AUDIO_CFIFO_CLOSE     3

struct audio_pcmic_channel_attr {
    u16 delay_time;            /*PCMIC通道延时*/
    u16 protect_time;          /*PCMIC延时保护时间*/
    u8 write_mode;             /*PCMIC写入模式*/
    u8 dev_properties;         /*关联蓝牙同步的主设备使能，只能有一个节点是主设备*/
} __attribute__((packed));

struct audio_pcmic_channel {
    struct audio_pcmic_channel_attr attr;     /*PCMIC通道属性*/
    struct audio_cfifo_channel fifo;        /*PCMIC cfifo通道管理*/
    struct pc_mic_node_hdl *node_hdl;
    u8 state;                              /*PCMIC状态*/
};

struct _pcmic_hdl {
    struct list_head sync_list;
    struct list_head dev_sync_list;
    struct audio_cfifo cfifo;        /*DAC cfifo结构管理*/
    OS_MUTEX mutex;
    u8  state;              /*硬件通道的状态*/
    u8 fifo_state;
    u8 iport_bit_width;     //保存输入节点的位宽
    u8 *out_cbuf;              //uac缓存cbuf
    u8 *frame_buf;             //pull frame后需转换成uac格式的一帧buf
    u16 frame_len;
    u32 fade_value;

};
struct _pcmic_hdl pcmic_hdl  = {0};

struct pc_mic_node_hdl {
    char name[16];
    struct audio_pcmic_channel pcmic_ch;
    void *dev_sync;         //! bang
    struct list_head entry;
    struct audio_pcmic_channel_attr attr;     /*PCMIC通道属性*/
    enum stream_scene scene;
    u8 iport_bit_width;     //保存输入节点的位宽
    u8 syncts_enabled;
    u8 start;
    u8 force_write_slience_data;
    u8 reference_network;
};

struct audio_pcmic_sync_node {
    u8 need_triggered;
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};

struct pc_mic_fmt_t {
    u8 init;
    u8 channel;
    u8 bit;
    u32 sample_rate;
};
struct pc_mic_fmt_t pc_mic_fmt = {
    .init = 0,
    .channel = MIC_CHANNEL,
    .bit = MIC_AUDIO_RES,
    .sample_rate = MIC_AUDIO_RATE,
};

static DEFINE_SPINLOCK(pc_mic_lock);
static void audio_pcmic_fade_out(struct _pcmic_hdl *hdl, void *data, u16 len)
{
    u8 ch_num = pc_mic_fmt.channel;
    if (hdl->iport_bit_width) {
        if (config_media_24bit_enable) {
            hdl->fade_value = jlstream_fade_out_32bit(hdl->fade_value, FADE_GAIN_MAX / (len / 4 / ch_num), (int *)data, len, ch_num);
        }
    } else {
        hdl->fade_value = jlstream_fade_out(hdl->fade_value, FADE_GAIN_MAX / (len / 2 / ch_num), (short *)data, len, ch_num);
    }
}

static void audio_pcmic_fade_in(struct _pcmic_hdl *hdl, void *data, u16 len)
{
    if (hdl->fade_value < FADE_GAIN_MAX) {
        u8 ch_num = pc_mic_fmt.channel;
        if (hdl->iport_bit_width) {
            if (config_media_24bit_enable) {
                hdl->fade_value = jlstream_fade_in_32bit(hdl->fade_value, FADE_GAIN_MAX / (len / 4 / ch_num), (s32 *)data, len, ch_num);
            }
        } else {
            hdl->fade_value = jlstream_fade_in(hdl->fade_value, FADE_GAIN_MAX / (len / 2 / ch_num), (s16 *)data, len, ch_num);
        }
    }
}

static int pc_mic_tx_handler(void *priv, void *buf, int len);
int audio_pcmic_channel_write(struct audio_pcmic_channel *ch, void *buf, int len);
int audio_pcmic_channel_buffered_frames(struct audio_pcmic_channel *ch)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }

    int buffered_frames = 0;

    if (ch->state != AUDIO_CFIFO_START) {
        goto ret_value;
    }

    buffered_frames = audio_cfifo_channel_unread_samples(&ch->fifo);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }

ret_value:

    return buffered_frames;
}

int audio_pcmic_data_len(struct audio_pcmic_channel *ch)
{
    return audio_pcmic_channel_buffered_frames(ch);
}
static int dev_sync_output_handler(void *_hdl, void *data, int len)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }

    int wlen = audio_pcmic_channel_write((void *)&hdl->pcmic_ch, data, len);

    return wlen;
}

static void dev_sync_start(struct stream_iport *iport)
{
    if (config_dev_sync_enable) {
        struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;
        if (hdl && hdl->attr.dev_properties == SYNC_TO_MASTER_DEV) {
            if (!hdl->dev_sync) {
                struct dev_sync_params params = {0};
                params.in_sample_rate = pc_mic_fmt.sample_rate;
                params.out_sample_rate = pc_mic_fmt.sample_rate;
                params.channel = pc_mic_fmt.channel;
                params.bit_width = iport->prev->fmt.bit_wide;
                params.priv = hdl;
                params.handle = dev_sync_output_handler;
                hdl->dev_sync = dev_sync_open(&params);
                spin_lock(&pc_mic_lock);
                list_add(&hdl->entry, &pcmic_hdl.dev_sync_list);
                spin_unlock(&pc_mic_lock);
            }
        }
    }

}



extern int iis_adapter_link_to_syncts_check(void *syncts);
extern int dac_adapter_link_to_syncts_check(void *syncts);
static int audio_pcmic_add_syncts_with_timestamp(struct audio_pcmic_channel *ch, void *syncts, u32 timestamp)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }
#if TCFG_DAC_NODE_ENABLE
    if (dac_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to dac");
        return 0;
    }
#endif

    spin_lock(&pc_mic_lock);
    if (iis_adapter_link_to_syncts_check(syncts)) {
        log_debug("syncts has beed link to iis");
        spin_unlock(&pc_mic_lock);
        return 0;
    }

    struct audio_pcmic_sync_node *node = NULL;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->hdl == (u32)syncts) {
            spin_unlock(&pc_mic_lock);
            return 0;
        }
    }
    log_debug(">>>>>>>>>>>>>>>>add pcmic syncts %x\n", (u32)syncts);
    node = (struct audio_pcmic_sync_node *)zalloc(sizeof(struct audio_pcmic_sync_node));
    node->hdl = syncts;
    node->ch = ch;
    node->timestamp = timestamp;
    node->network = sound_pcm_get_syncts_network(syncts);
    list_add(&node->entry, &hdl->sync_list);
    spin_unlock(&pc_mic_lock);
    return 1;
}
static void audio_pcmmic_remove_syncts_handle(struct audio_pcmic_channel *ch, void *syncts)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return ;
    }
    struct audio_pcmic_sync_node *node;
    spin_lock(&pc_mic_lock);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->ch != (u32)ch)  {
            continue;
        }
        if (node->hdl == syncts) {
            goto remove_node;
        }
    }
    spin_unlock(&pc_mic_lock);
    return;
remove_node:
    log_debug(">>>>>>>>>>>>>>>>>>>del pcmic syncts %x\n", (u32)syncts);
    list_del(&node->entry);
    free(node);
    spin_unlock(&pc_mic_lock);
}

static int audio_pcmic_syncts_handler(struct stream_iport *iport, int arg)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;
    struct audio_syncts_ioc_params *params = (struct audio_syncts_ioc_params *)arg;

    if (!params) {
        return 0;
    }

    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        if (hdl->attr.dev_properties != SYNC_TO_MASTER_DEV) {
            audio_pcmic_add_syncts_with_timestamp(&hdl->pcmic_ch, (void *)params->data[0], params->data[1]);
        }
        if (hdl->reference_network == 0xff) {
            hdl->reference_network = params->data[2];
        }

        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        audio_pcmmic_remove_syncts_handle(&hdl->pcmic_ch, (void *)params->data[0]);
        break;
    }
    return 0;
}
void audio_pcmic_syncts_trigger_with_timestamp(struct audio_pcmic_channel *ch, u32 timestamp)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return ;
    }
    spin_lock(&pc_mic_lock);

    int time_diff = 0;
    struct audio_pcmic_sync_node *node;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->need_triggered || ((u32)node->ch != (u32)ch)) {
            continue;
        }
        node->need_triggered = 1;
    }
    spin_unlock(&pc_mic_lock);
}
void audio_pcmic_force_use_syncts_frames(struct pc_mic_node_hdl *_hdl, int frames, u32 timestamp)
{

    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return ;
    }
    struct audio_pcmic_sync_node *node;
    spin_lock(&pc_mic_lock);
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->triggered) {
            if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
                u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
                if (!timestamp_enable) {
                    continue;
                }
            }
            int time_diff = node->timestamp - timestamp;
            if (time_diff > 0) {
                continue;
            }
            if (node->hdl) {
                sound_pcm_update_frame_num(node->hdl, frames);
            }
        }
    }
    spin_unlock(&pc_mic_lock);
}

static int audio_pcmic_channel_fifo_write(struct audio_pcmic_channel *ch, void *data, int len, u8 is_fixed_data)
{
    if (len == 0) {
        return 0;
    }
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }

    os_mutex_pend(&hdl->mutex, 0);
    int w_len = 0;
    if (is_fixed_data) {
        w_len = audio_cfifo_channel_write_fixed_data(&ch->fifo, (s16)data, len);
    } else {
        w_len = audio_cfifo_channel_write(&ch->fifo, data, len);
    }
    os_mutex_post(&hdl->mutex);
    return w_len;
}

int audio_pcmic_channel_write(struct audio_pcmic_channel *ch, void *buf, int len)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }
    return audio_pcmic_channel_fifo_write(ch, buf, len, 0);
}
int audio_pcmic_fill_slience_frames(struct audio_pcmic_channel *ch,  int frames)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }

    int wlen = 0;

    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    wlen = audio_pcmic_channel_fifo_write(ch, (void *)0, frames * pc_mic_fmt.channel << point_offset, 1);

    return (wlen >> point_offset) / pc_mic_fmt.channel;
}
__attribute__((always_inline))
static int audio_pcmic_update_syncts_frames(struct _pcmic_hdl *hdl, int frames)
{
    struct audio_pcmic_sync_node *node;

    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->triggered) {
            continue;
        }
        if (!node->need_triggered) {
            continue;
        }

        if (audio_syncts_support_use_trigger_timestamp(node->hdl)) {
            u8 timestamp_enable = audio_syncts_get_trigger_timestamp(node->hdl, &node->timestamp);
            if (!timestamp_enable) {
                continue;
            }
        }
        sound_pcm_syncts_latch_trigger(node->hdl);
        node->triggered = 1;
    }


    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->triggered) {
            continue;
        }
        /*
         * 这里需要将其他通道延时数据大于音频同步计数通道的延迟数据溢出值通知到音频同步
         * 锁存模块，否则会出现硬件锁存不同通道数据的溢出误差
         */
        int diff_samples = audio_cfifo_channel_unread_diff_samples(&((struct audio_pcmic_channel *)node->ch)->fifo);
        sound_pcm_update_overflow_frames(node->hdl, diff_samples);

        sound_pcm_update_frame_num(node->hdl, frames);
        if (node->network == AUDIO_NETWORK_LOCAL && (((struct audio_pcmic_channel *)node->ch)->state == AUDIO_CFIFO_START)) {
            if (audio_syncts_latch_enable(node->hdl)) {
                u32 time = audio_jiffies_usec();
                sound_pcm_update_frame_num_and_time(node->hdl, 0, time, 0);
            }
        }
    }

    return 0;
}
static void audio_pcmic_channel_read(struct _pcmic_hdl *hdl,  s16 *addr, int len)
{
    u32 point_offset = hdl->iport_bit_width ? 2 : 1;

    if (hdl->fifo_state  == AUDIO_CFIFO_INITED) {
        u32 rlen = audio_cfifo_read_data(&hdl->cfifo, addr, len);
        if (rlen) {
            /* putchar('R'); */
            audio_pcmic_update_syncts_frames(hdl, (rlen >> point_offset) / pc_mic_fmt.channel);
            if (rlen < len) {
                audio_pcmic_fade_out(hdl, addr, rlen);
                memset((void *)((u32)addr + rlen), 0x0, len - rlen);
            } else {
                audio_pcmic_fade_in(hdl, addr, len);
            }
        } else {
            putchar('M');
            audio_pcmic_fade_out(hdl, addr, len);
            /* puts("pcmic_empty\n"); */
        }
    }
}

u8 audio_pcmic_is_working(struct _pcmic_hdl *hdl)
{
    if (!hdl) {
        return 0;
    }
    if ((hdl->state != PCMIC_STATE_OPEN) && (hdl->state != PCMIC_STATE_START)) {
        return 0;
    }

    return 1;
}
int audio_pcmic_new_channel(struct pc_mic_node_hdl *hdl, struct audio_pcmic_channel *ch)
{
    if (!hdl || !ch) {
        return -EINVAL;
    }
    memset(ch, 0x0, sizeof(struct audio_pcmic_channel));
    ch->node_hdl = hdl;
    ch->state = AUDIO_CFIFO_IDLE;
    return 0;
}

void audio_pcmic_channel_set_attr(struct audio_pcmic_channel *ch, void *attr)
{
    if (ch) {
        if (!attr) {
            return;
        }
        memcpy(&ch->attr, attr, sizeof(struct audio_pcmic_channel_attr));
    }
}

void audio_pcmic_channel_start(struct audio_pcmic_channel *ch)
{
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        log_e("PCMIC channel start error! PCMIC_hdl is NULL\n");
        return;
    }

    os_mutex_pend(&hdl->mutex, 0);
    spin_lock(&pc_mic_lock);
    if (!audio_pcmic_is_working(hdl)) {
        hdl->fade_value = 0;
        set_uac_mic_tx_handler((void *)hdl, pc_mic_tx_handler);

        hdl->state = PCMIC_STATE_OPEN;
        hdl->fifo_state = AUDIO_CFIFO_IDLE;
    }

    if (hdl->fifo_state != AUDIO_CFIFO_INITED) {
        u32 point_size = (!hdl->iport_bit_width) ? 2 : 4;
        u32 pc_mic_buf_size = UAC_OUT_BUF_MAX_MS * ((AUDIO_DAC_MAX_SAMPLE_RATE + 999) / 1000) * pc_mic_fmt.channel * point_size; //20ms延时的buf
        if (!hdl->out_cbuf) {
            hdl->out_cbuf = malloc(pc_mic_buf_size);
        }

        hdl->cfifo.bit_wide = hdl->iport_bit_width;
        audio_cfifo_init(&hdl->cfifo, hdl->out_cbuf, pc_mic_buf_size, pc_mic_fmt.sample_rate, pc_mic_fmt.channel);
        hdl->fifo_state = AUDIO_CFIFO_INITED;
    }
    if (hdl->state != PCMIC_STATE_START) {
        audio_cfifo_channel_add(&hdl->cfifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
        ch->state = AUDIO_CFIFO_START;
        hdl->state = PCMIC_STATE_START;
    } else {
        if (ch->state != AUDIO_CFIFO_START) {
            audio_cfifo_channel_add(&hdl->cfifo, &ch->fifo, ch->attr.delay_time, ch->attr.write_mode);
            ch->state = AUDIO_CFIFO_START;
        }
    }
    spin_unlock(&pc_mic_lock);
    os_mutex_post(&hdl->mutex);
}

int audio_pcmic_channel_close(struct audio_pcmic_channel *ch)
{
    int ret = 0;
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;
    if (!hdl) {
        return 0;
    }
    os_mutex_pend(&hdl->mutex, 0);
    spin_lock(&pc_mic_lock);
    if (ch->state != AUDIO_CFIFO_IDLE && ch->state != AUDIO_CFIFO_CLOSE) {
        if (ch->state == AUDIO_CFIFO_START) {
            audio_cfifo_channel_del(&ch->fifo);
            ch->state = AUDIO_CFIFO_CLOSE;
        }
    }
    if (hdl->fifo_state != AUDIO_CFIFO_IDLE) {
        if (audio_cfifo_channel_num(&hdl->cfifo) == 0) {
            hdl->state = PCMIC_STATE_STOP;
            hdl->fifo_state = AUDIO_CFIFO_CLOSE;
            set_uac_mic_tx_handler((void *)NULL, NULL);
            if (hdl->out_cbuf) {
                free(hdl->out_cbuf);
                hdl->out_cbuf = NULL;
            }
            if (hdl->frame_buf) {
                free(hdl->frame_buf);
                hdl->frame_buf = NULL;
            }
            hdl->fade_value = 0;
        }
    }
    spin_unlock(&pc_mic_lock);
    os_mutex_post(&hdl->mutex);
    return ret;
}


static int pcmic_adpater_detect_timestamp(struct pc_mic_node_hdl *hdl, struct stream_frame *frame)
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
            int slience_frames = (u64)slience_time_us * pc_mic_fmt.sample_rate / 1000000;
            audio_pcmic_fill_slience_frames(&hdl->pcmic_ch, slience_frames);
        }
        hdl->syncts_enabled = 1;
        return 0;
    }

    if (hdl->syncts_enabled) {
        audio_pcmic_syncts_trigger_with_timestamp(&hdl->pcmic_ch, frame->timestamp);
        return 0;
    }


    if (hdl->reference_network == AUDIO_NETWORK_LOCAL) {
        current_time = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_pcmic_data_len(&hdl->pcmic_ch), pc_mic_fmt.sample_rate);
    } else {
        u32 reference_time = 0;
        u8 net_addr[6];
        u8 network = audio_reference_clock_network(net_addr);
        if (network != ((u8) - 1)) {
            reference_time = audio_reference_clock_time();
            if (reference_time == (u32) - 1) {
                goto syncts_start;
            }
            if (network == 2) {
                current_time = reference_time * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_pcmic_data_len(&hdl->pcmic_ch), pc_mic_fmt.sample_rate);
            } else {
                current_time = reference_time * 625 * TIMESTAMP_US_DENOMINATOR + PCM_SAMPLE_TO_TIMESTAMP(audio_pcmic_data_len(&hdl->pcmic_ch), pc_mic_fmt.sample_rate);
            }
        }
    }
    diff = frame->timestamp - current_time;
    diff /= TIMESTAMP_US_DENOMINATOR;
    if (diff < 0) {
        if (__builtin_abs(diff) <= 1000) {
            goto syncts_start;
        }
        log_debug("ts %u cur_t %u, diff_us %d\n", frame->timestamp, current_time, diff);
        log_debug("dump frame, %s network %d\n", hdl->name, hdl->reference_network);
        return 2;
    }

    int slience_frames = (u64)diff * pc_mic_fmt.sample_rate / 1000000;
    int filled_frames = audio_pcmic_fill_slience_frames(&hdl->pcmic_ch, slience_frames);
    if (filled_frames < slience_frames) {
        return 1;
    }

syncts_start:
    log_debug("%s, ts %u cur_t %u, diff_us %d\n", hdl->name, frame->timestamp, current_time, diff);
    hdl->syncts_enabled = 1;
    audio_pcmic_syncts_trigger_with_timestamp(&hdl->pcmic_ch, frame->timestamp);
    return 0;
}

void pcm_data_convert_to_uac_data(struct _pcmic_hdl *hdl, void *in, void *out, int pcm_frames)
{
    if (pc_mic_fmt.bit == 16) {
        if (hdl->iport_bit_width) {
            audio_convert_data_32bit_to_16bit_round((s32 *)in, (s16 *)out, pcm_frames);
        }
    } else if (pc_mic_fmt.bit == 24) {
        if (hdl->iport_bit_width) {
            audio_convert_data_4byte24bit_to_3byte24bit((s32 *)in, (s32 *)out, pcm_frames);
        } else {
            audio_convert_data_16bit_to_3byte24bit((s16 *)in, (s32 *)out, pcm_frames);
        }
    }
}
//pc mic 驱动拿数接口
static int pc_mic_tx_handler(void *priv, void *buf, int len)
{
    spin_lock(&pc_mic_lock);
    struct _pcmic_hdl *hdl = (struct _pcmic_hdl *)&pcmic_hdl;

    int rlen = len;
    if (pc_mic_fmt.bit == 16 && !hdl->iport_bit_width) {//16->16
        audio_pcmic_channel_read(hdl, (s16 *)buf, rlen);
        if (config_dev_sync_enable) {
            struct pc_mic_node_hdl *node_hdl;
            list_for_each_entry(node_hdl, &hdl->dev_sync_list, entry) {
                if (node_hdl->dev_sync) {
                    dev_sync_calculate_output_samplerate(node_hdl->dev_sync, len, 0);
                }
            }
        }
    } else {
        u32 point_offset = hdl->iport_bit_width ? 2 : 1;
        int pcm_frames;
        if (pc_mic_fmt.bit == 16) {
            pcm_frames	= (rlen / 2) / pc_mic_fmt.channel;
        } else {
            pcm_frames = (rlen * 8 / 24) / pc_mic_fmt.channel;
        }
        rlen = (pcm_frames << point_offset) * pc_mic_fmt.channel;
        if (rlen > hdl->frame_len) {
            if (hdl->frame_buf) {
                free(hdl->frame_buf);
                hdl->frame_buf = NULL;
            }
        }
        if (!hdl->frame_buf) {
            hdl->frame_buf = zalloc(rlen);
            hdl->frame_len = rlen;
        }

        audio_pcmic_channel_read(hdl, (s16 *)hdl->frame_buf, rlen);
        pcm_data_convert_to_uac_data(hdl, hdl->frame_buf, buf, pcm_frames * pc_mic_fmt.channel);
        if (config_dev_sync_enable) {
            struct pc_mic_node_hdl *node_hdl;
            list_for_each_entry(node_hdl, &hdl->dev_sync_list, entry) {
                if (node_hdl->dev_sync) {
                    dev_sync_calculate_output_samplerate(node_hdl->dev_sync, rlen, 0);
                }
            }
        }
    }
    spin_unlock(&pc_mic_lock);
    return len;
}

int pc_mic_channel_latch_time(struct pc_mic_node_hdl *_hdl, u32 *latch_time, u32(*get_time)(u32 reference_network), u32 reference_network)
{
    int buffered_frames = 0;
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)_hdl;
    if (!hdl) {
        return 0;
    }
    os_mutex_pend(&pcmic_hdl.mutex, 0);
    spin_lock(&pc_mic_lock);
    *latch_time = get_time(reference_network);

    buffered_frames = audio_pcmic_data_len(&hdl->pcmic_ch);
    if (buffered_frames < 0) {
        buffered_frames = 0;
    }
    spin_unlock(&pc_mic_lock);
    os_mutex_post(&pcmic_hdl.mutex);
    return buffered_frames;
}



static void pc_mic_synchronize_with_main_device(struct stream_iport *iport, struct stream_frame *frame)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;

    if (hdl->dev_sync) {
        struct sync_rate_params params = {0};
        params.d_sample_rate = frame->d_sample_rate;
        params.buffered_frames = pc_mic_channel_latch_time(hdl, &params.current_time, dev_sync_latch_time, hdl->reference_network);
        params.timestamp = frame->timestamp;
        params.name = hdl->name;
        dev_sync_update_rate(hdl->dev_sync, &params);
    }

}
// 数据流节点回调，做数据缓存
__NODE_CACHE_CODE(pc_mic)
static void pc_mic_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;
    struct stream_frame *frame = NULL;


    int wlen = 0;
    if (hdl->start == 0) {
        return;
    }
    audio_pcmic_channel_start(&hdl->pcmic_ch);
    dev_sync_start(iport);

    while (1) {
        frame = jlstream_pull_frame(iport, NULL);
        if (!frame) {
            break;
        }

        if (config_dev_sync_enable) {
            if (frame->offset == 0) {
                pc_mic_synchronize_with_main_device(iport, frame);
            }
        }

        int err = pcmic_adpater_detect_timestamp(hdl, frame);
        if (err == 1) {
            jlstream_return_frame(iport, frame);
            break;
        }

        if (err == 2) { //需要直接丢弃改帧数据
            u32 point_offset = hdl->iport_bit_width ? 2 : 1;
            audio_pcmic_force_use_syncts_frames(hdl, (frame->len >> point_offset) / pc_mic_fmt.channel, frame->timestamp);
            jlstream_free_frame(frame);
            continue;
        }
        s16 *data = (s16 *)(frame->data + frame->offset);
        u32 remain = frame->len - frame->offset;

        if (hdl->dev_sync) {
            if (config_dev_sync_enable) {
                wlen = dev_sync_write(hdl->dev_sync, data, remain);
            }
        } else {
            wlen = audio_pcmic_channel_write(&hdl->pcmic_ch, data, remain);
        }
        frame->offset += wlen;
        if (wlen < remain) {
            note->state |= NODE_STA_OUTPUT_BLOCKED;
            jlstream_return_frame(iport, frame);
            break;
        }

        jlstream_free_frame(frame);
    }
}

static void pc_mic_ioc_start(struct pc_mic_node_hdl *hdl)
{
    spin_lock(&pc_mic_lock);
    hdl->iport_bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;
    hdl->start = 1;

    pcmic_hdl.iport_bit_width = audio_general_out_dev_bit_width();;
    log_debug("---------------------- %s, pcmic_hdl.iport_bit_width %d\n", hdl->name, pcmic_hdl.iport_bit_width);
    audio_pcmic_new_channel(hdl, (void *)&hdl->pcmic_ch);
    audio_pcmic_channel_set_attr(&hdl->pcmic_ch, &hdl->attr);

    spin_unlock(&pc_mic_lock);
}

static void pc_mic_ioc_stop(struct pc_mic_node_hdl *hdl)
{
    hdl->start = 0;
    hdl->syncts_enabled = 0;
    hdl->force_write_slience_data = 0;

    audio_pcmic_channel_close(&hdl->pcmic_ch);

    if (config_dev_sync_enable) {
        if (hdl->dev_sync) {
            spin_lock(&pc_mic_lock);
            list_del(&hdl->entry);
            spin_unlock(&pc_mic_lock);

            dev_sync_close(hdl->dev_sync);
            hdl->dev_sync = NULL;
        }
    }
    log_debug("--------------%s, ioc_stop\n", hdl->name);
}


static void pc_mic_adapter_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = pc_mic_handle_frame;
}

static int pc_mic_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    u8 bit_width = audio_general_out_dev_bit_width();
    int ret = NEGO_STA_ACCPTED;
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

    if (in_fmt->sample_rate != pc_mic_get_fmt_sample_rate()) {
        in_fmt->sample_rate = pc_mic_get_fmt_sample_rate();
        ret = NEGO_STA_CONTINUE;
    }
    u8 channel_mode = AUDIO_CH_DIFF;
    if (pc_mic_fmt.channel == 2) {
        channel_mode = AUDIO_CH_LR;
    }

    if (in_fmt->channel_mode != channel_mode) {
        if (channel_mode == AUDIO_CH_DIFF) {
            if (in_fmt->channel_mode != AUDIO_CH_L &&
                in_fmt->channel_mode != AUDIO_CH_R &&
                in_fmt->channel_mode != AUDIO_CH_MIX) {
                in_fmt->channel_mode = AUDIO_CH_MIX;
                ret = NEGO_STA_CONTINUE;
            }
        } else {
            in_fmt->channel_mode = channel_mode;
            ret = NEGO_STA_CONTINUE;
        }
    }

    log_debug("## Func:%s, negotiate_state : %d, output_rate:%d, channel_mode:%d, sr: %d, bit_width %d\n", __func__, ret, in_fmt->sample_rate, channel_mode, in_fmt->sample_rate, in_fmt->bit_wide);
    return ret;
}

static int audio_pcmic_buffer_delay_time(struct stream_iport *iport)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;
    if (!hdl) {
        return 0;
    }
    u32 points_per_ch = audio_pcmic_data_len(&hdl->pcmic_ch);
    int rate = pc_mic_fmt.sample_rate;
    ASSERT(rate != 0);

    return (points_per_ch * 10000) / rate;//10000:表示ms*10
}

static int pc_mic_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        pc_mic_adapter_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= pc_mic_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_GET_DELAY:
        return audio_pcmic_buffer_delay_time(iport);
    case NODE_IOC_START:
        pc_mic_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        pc_mic_ioc_stop(hdl);
        break;
    case NODE_IOC_SYNCTS:
        audio_pcmic_syncts_handler(iport, arg);
        break;
    case NODE_IOC_SET_PARAM:
        hdl->reference_network = arg;
        break;

    default:
        break;
    }
    return ret;
}



static int pc_mic_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct pc_mic_node_hdl *hdl = (struct pc_mic_node_hdl *)node->private_data;

    static u8 init = 0;
    if (!init) {
        os_mutex_create(&pcmic_hdl.mutex);
        INIT_LIST_HEAD(&pcmic_hdl.sync_list);
        INIT_LIST_HEAD(&pcmic_hdl.dev_sync_list);
        init = 1;
    }

    int ret = jlstream_read_node_data_new(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (void *)&hdl->attr, hdl->name);
    if (!ret) {
        hdl->attr.delay_time = 50;
        hdl->attr.protect_time = 8;
        hdl->attr.write_mode = WRITE_MODE_BLOCK;
        log_error("pcmic node param read err, set default\n");
    }
    hdl->reference_network = 0xff;
    return 0;
}

static void pc_mic_adapter_release(struct stream_node *node)
{
}

// 设置pc mic 的数据格式，传入0不设置
void pc_mic_set_fmt(u8 channel, u8 bit, u32 sample_rate)
{
    log_info("----------- Call %s, bit:%d, sr:%d\n", __func__, bit, sample_rate);
    pc_mic_fmt.init = 1;
    if (channel != 0) {
        pc_mic_fmt.channel = channel;
    }
    if (bit != 0) {
        pc_mic_fmt.bit = bit;
    }
    if (sample_rate != 0) {
        pc_mic_fmt.sample_rate = sample_rate;
    }
}

u32 pc_mic_get_fmt_sample_rate(void)
{
    log_info("Mic Sr : %d\n", pc_mic_fmt.sample_rate);
    return pc_mic_fmt.sample_rate;
}


REGISTER_STREAM_NODE_ADAPTER(pc_mic_node_adapter) = {
    .name       = "pc_mic",
    .uuid       = NODE_UUID_PC_MIC,
    .bind       = pc_mic_adapter_bind,
    .ioctl      = pc_mic_adapter_ioctl,
    .release    = pc_mic_adapter_release,
    .hdl_size   = sizeof(struct pc_mic_node_hdl),
};

#endif




