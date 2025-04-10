#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "iis_file.h"
#include "app_config.h"
#include "audio_iis.h"
#include "sync/audio_clk_sync.h"
#include "gpio.h"
#include "audio_config.h"
#include "audio_cvp.h"
#include "effects/effects_adj.h"
#include "media/audio_general.h"
#include "circular_buf.h"

/************************ ********************* ************************/
/************************ 该文件是 IIS 输入用的 ************************/
/************************ ********************* ************************/

#if TCFG_MULTI_CH_IIS_RX_NODE_ENABLE

#define IIS_LOG_ENABLE          0
#if IIS_LOG_ENABLE
#define LOG_TAG     "[MIIS-FILE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif


#define MODULE_IDX_SEL ((hdl->plug_uuid == NODE_UUID_MULTI_CH_IIS0_RX) ? 0 : 1)
struct audio_iis_rx_channel {
    u16 ch_idx;                /*IIS通道 ch0 bit0  ch1 bit1 ch2 bit2 ch3 bit3*/
};


struct iis_rx_ch {
    cbuffer_t cache_cbuffer;
    u8 dump_cnt;
    int value;
    u8 *cache_buf;
    u32 buf_len;
};
struct iis_file_hdl {
    void *source_node;
    struct stream_node *node;
    struct stream_snode *snode;
    struct audio_iis_rx_output_hdl output[4];
    struct iis_rx_ch ch[4];
    enum stream_scene scene;
    enum audio_iis_state_enum state;
    u32 sample_rate;
    u32 timestamp;
    int force_dump;
    u16 irq_points;
    u16 plug_uuid;
    struct audio_iis_rx_channel attr;
    u8 start;
    u8 channel_mode;
    u8 bit_width;
    u8 module_idx;
    u8 timestamp_init_ch;
};

/*
 * 24bit转32bit，处理符号位
 * bit23如果是1，高8位补1;如果是0，高8位补0*/
__NODE_CACHE_CODE(iis)
static void audio_data_24bit_to_32bit(void *buf, int npoint)
{
    s32 *data = (s32 *)buf;
    for (int i = 0; i < npoint; i++) {
        if (data[i] & 0x00800000) {
            data[i] = data[i] | 0xff000000;
        } else {
            data[i] = data[i] & 0x00ffffff;
        }
    }
}

__NODE_CACHE_CODE(iis)
static void iis_file_fade_in(struct iis_file_hdl *hdl, void *buf, int len, int ch_idx)
{
    struct iis_rx_ch *ch = &hdl->ch[ch_idx];
    if (ch->value < FADE_GAIN_MAX) {
        int fade_ms = 100;//ms
        int fade_step = FADE_GAIN_MAX / (fade_ms * hdl->sample_rate / 1000);
        if (hdl->bit_width == DATA_BIT_WIDE_16BIT) {
            ch->value  = jlstream_fade_in(ch->value, fade_step, buf, len, AUDIO_CH_NUM(hdl->channel_mode));
        } else {
            ch->value  = jlstream_fade_in_32bit(ch->value, fade_step, buf, len, AUDIO_CH_NUM(hdl->channel_mode));
        }
    }
}

/* IIS 接收中断测试 */
__NODE_CACHE_CODE(iis)
static void iis_rxx_handle(void *priv, void *buf, int len, int ch_idx)
{
    struct iis_file_hdl *hdl = priv;
    struct stream_frame *frame = NULL;
    if (!hdl) {
        return;
    }
    struct iis_rx_ch *ch = &hdl->ch[ch_idx];

    if (hdl->start == 0) {
        return;
    }
    if (hdl->bit_width == DATA_BIT_WIDE_24BIT) {
        audio_data_24bit_to_32bit(buf, len >> 2);
    }
    u8 ch_num = AUDIO_CH_NUM(hdl->channel_mode);
    if (ch_num == 1) {
        if (hdl->bit_width == DATA_BIT_WIDE_24BIT) {
            pcm_dual_to_single_32bit(buf, buf, len);
        } else {
            pcm_dual_to_single(buf, buf, len);
        }
        len >>= 1;
    }

#if defined(IIS_USE_DOUBLE_BUF_MODE_EN) && IIS_USE_DOUBLE_BUF_MODE_EN
    if (ch->dump_cnt < 10) {
        ch->dump_cnt++;
        return;
    }
    if (hdl->force_dump) {
        ch->value = 0;
        return;
    }

    frame = source_plug_get_output_frame_by_id(hdl->source_node, ch_idx, len);
    if (!frame) {
        return;
    }
    frame->len          = len;
    if (!hdl->timestamp) {
        hdl->timestamp = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
        hdl->timestamp_init_ch = ch_idx;
    }
    if (hdl->timestamp_init_ch == ch_idx) {
        hdl->timestamp = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
    }

    frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    frame->timestamp    = hdl->timestamp;
    memcpy(frame->data, buf, frame->len);
    iis_file_fade_in(hdl, frame->data, frame->len, ch_idx);//淡入处理
    source_plug_put_output_frame_by_id(hdl->source_node, ch_idx, frame);
#else
    if (!ch->cache_buf) {
        //申请cbuffer
        ch->buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * ch_num;
        int buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * ch_num * 3;
        ch->cache_buf = malloc(buf_len);
        if (ch->cache_buf) {
            cbuf_init(&ch->cache_cbuffer, ch->cache_buf, buf_len);
        }
    }
    if (ch->cache_buf) {
        int wlen = cbuf_write(&ch->cache_cbuffer, buf, len);
        if (wlen != len) {
            log_debug("iis rx wlen %d != len %d\n", wlen, len);
        }
    }

    u32 cache_len = cbuf_get_data_len(&ch->cache_cbuffer);
    if (cache_len >= ch->buf_len) {
        if (ch->dump_cnt < 10) {
            ch->dump_cnt++;
            cbuf_read_updata(&ch->cache_cbuffer, ch->buf_len);
            return;
        }
        if (hdl->force_dump) {
            cbuf_read_updata(&ch->cache_cbuffer, ch->buf_len);
            ch->value = 0;
            return;
        }

        frame = source_plug_get_output_frame_by_id(hdl->source_node, ch_idx, ch->buf_len);
        if (!frame) {
            return;
        }
        frame->len          = ch->buf_len;
        if (!hdl->timestamp) {
            hdl->timestamp = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
            hdl->timestamp_init_ch = ch_idx;
        }
        if (hdl->timestamp_init_ch == ch_idx) {
            hdl->timestamp = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
        }

#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = hdl->timestamp;
#else
        frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp    = audio_jiffies_usec();
#endif
        cbuf_read(&ch->cache_cbuffer, frame->data, frame->len);

        iis_file_fade_in(hdl, frame->data, frame->len, ch_idx);//淡入处理

        source_plug_put_output_frame_by_id(hdl->source_node, ch_idx, frame);
    }
#endif

}
static void iis_rx0_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 0);
}
static  void iis_rx1_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 1);
}
static  void iis_rx2_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 2);
}
static void iis_rx3_handle(void *priv, void *addr, int len)	//中断回调
{
    iis_rxx_handle(priv, addr, len, 3);
}
static int iis_rx_handle[4] = {(int)iis_rx0_handle, (int)iis_rx1_handle, (int)iis_rx2_handle, (int)iis_rx3_handle};

static void iis_rx_start(struct iis_file_hdl *hdl)
{
    if (hdl && hdl->state == AUDIO_IIS_STATE_INIT) {
        audio_iis_open(iis_hdl[hdl->module_idx], 0);
        audio_iis_set_rx_irq_points(iis_hdl[hdl->module_idx], hdl->irq_points);
        for (int i = 0; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                hdl->output[i].enable = 1;
                hdl->output[i].ch_idx = i;
                hdl->output[i].priv = hdl;
                hdl->output[i].handler = (void (*)(void *, void *, int))iis_rx_handle[i];
            }
        }
        audio_iis_add_multi_channel_rx_output_handler(iis_hdl[hdl->module_idx], hdl->output);

        audio_iis_start(iis_hdl[hdl->module_idx]);
        hdl->state = AUDIO_IIS_STATE_START;
    }
}

/*
 * @description: 初始化iis rx
 * @return：iis_file_hdl 结构体指针
 * @note: 无
 */
static void iis_rx_init(struct iis_file_hdl *hdl)
{
    if (hdl == NULL) {
        return;
    }
    log_debug(" --- iis_init ---\n");

    struct audio_general_params *params = audio_general_get_param();
    // 初始化iis rx
    // IIS采样率有所差距，配置
    hdl->sample_rate = params->sample_rate;	//默认采样率值
    jlstream_read_node_data_by_cfg_index(hdl->plug_uuid, hdl->node->subid, 0, (void *)&hdl->attr, NULL);

    if (!iis_hdl[hdl->module_idx]) {
        struct alink_param aparams = {0};
        aparams.module_idx = hdl->module_idx;
        aparams.dma_size   = audio_iis_fix_dma_len(hdl->module_idx, TCFG_AUDIO_DAC_BUFFER_TIME_MS, AUDIO_IIS_IRQ_POINTS, hdl->bit_width, 2);
        aparams.sr         = hdl->sample_rate;
        aparams.bit_width  = hdl->bit_width;
        iis_hdl[hdl->module_idx] = audio_iis_init(aparams);
    }
    if (!iis_hdl[hdl->module_idx]) {
        log_debug("iis module_idx %d rx init err\n", hdl->module_idx);
        return;
    }
    hdl->state = AUDIO_IIS_STATE_INIT;
}


/*
 * @description: iis file 初始化
 * @return: iis_file_hdl 结构体类型的指针
 * @note: 无
 */
static void *iis_file_init(void *source_node, struct stream_node *node)
{
    struct iis_file_hdl *hdl = zalloc(sizeof(*hdl));
    log_debug("--- iis_file_init ---\n");



    hdl->source_node = source_node;
    hdl->node = node;
    hdl->snode = (struct stream_snode *)node;
    node->type |= NODE_TYPE_IRQ;
    hdl->bit_width = audio_general_in_dev_bit_width();
    hdl->plug_uuid = get_source_node_plug_uuid(source_node);
    hdl->module_idx = MODULE_IDX_SEL;
    iis_rx_init(hdl);
    return hdl;
}

static void iis_ioc_get_fmt(struct iis_file_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  iis_ioc_get_fmt  ========== \n");
    fmt->coding_type = AUDIO_CODING_PCM;	//默认PCM
    switch (hdl->scene) {
    case STREAM_SCENE_ESCO:
        fmt->sample_rate = 16000;
        if (fmt->sample_rate != hdl->sample_rate) {
            ASSERT(fmt->sample_rate == hdl->sample_rate, "iis rx sr[%d] is not 16000 in esco", hdl->sample_rate);
        }
        hdl->channel_mode   = AUDIO_CH_MIX;
        break;
    case STREAM_SCENE_MIC_EFFECT:
        hdl->channel_mode   = AUDIO_CH_MIX;
        break;
    default:
        hdl->channel_mode   = AUDIO_CH_LR;
        break;
    }
    fmt->channel_mode = hdl->channel_mode;
    fmt->sample_rate = hdl->sample_rate;
    fmt->bit_wide = hdl->bit_width;
}

static int iis_ioc_set_fmt(struct iis_file_hdl *hdl, struct stream_fmt *fmt)
{
    log_debug("==========  iis_ioc_set_fmt  ==========\n");
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}

static void iis_file_start(struct iis_file_hdl *hdl)
{
    if (hdl->start == 0) {
        hdl->start = 1;
        if (hdl->state == AUDIO_IIS_STATE_INIT) {
            log_debug(">>> %s, %d, iis state is AUDIO_IIS_STATE_INIT\n", __func__, __LINE__);
            iis_rx_start(hdl);
        } else if (hdl->state == AUDIO_IIS_STATE_CLOSE) {
            log_debug(">>> %s, %d, iis state is AUDIO_IIS_STATE_CLOSE\n", __func__, __LINE__);

            iis_rx_init(hdl);
            iis_rx_start(hdl);
        }
        for (int i = 0; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                struct iis_rx_ch *ch = &hdl->ch[i];
                ch->dump_cnt = 0;
                ch->value = 0;
                audio_iis_check_hw_cfg_status(hdl->module_idx, i, ALINK_DIR_RX);
            }
        }
    }
}

static void iis_file_stop(struct iis_file_hdl *hdl)
{
    if (hdl->start) {
        hdl->start = 0;
        //停止 IIS 接收
        int ret = 0;
        audio_iis_del_multi_channel_rx_output_handler(iis_hdl[hdl->module_idx], hdl->output);

        for (int i = 0; i < 4; i++) {
            if (hdl->attr.ch_idx & BIT(i)) {
                audio_iis_stop(iis_hdl[hdl->module_idx], i);
                struct iis_rx_ch *ch = &hdl->ch[i];
                if (ch->cache_buf) {
                    free(ch->cache_buf);
                    ch->cache_buf = NULL;
                }
            }
        }

        // 释放IIS RX
        audio_iis_close(iis_hdl[hdl->module_idx]);
        ret = audio_iis_uninit(iis_hdl[hdl->module_idx]);
        if (ret) {
            iis_hdl[hdl->module_idx] = NULL;
        }

        hdl->state = AUDIO_IIS_STATE_CLOSE;
    }
}

static int iis_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct iis_file_hdl *hdl = (struct iis_file_hdl *)_hdl;
    switch (cmd) {
    case NODE_IOC_GET_FMT:
        iis_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = iis_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->irq_points = arg;
        /*iis rx 是固定双声道数据输入*/
        log_debug("set iis rx irq points %d\n", hdl->irq_points);
        break;
    case NODE_IOC_START:
        iis_file_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        iis_file_stop(hdl);
        break;
    case NODE_IOC_FORCE_DUMP_PACKET:
        hdl->force_dump = arg;
        break;

    }
    return ret;
}

/*
 * @description: 释放掉 iis 申请的内存
 * @param: 参数传入 iis_file_hdl 结构体指针类型的参数
 * @return：无
 * @note: 无
 */
static void iis_release(void *_hdl)
{
    struct iis_file_hdl *hdl = (struct iis_file_hdl *)_hdl;
    log_debug(" --- iis_release ---\n");

    free(hdl);
    hdl = NULL;

    log_debug(">>[%s] : iis rx release succ\n", __func__);
}

REGISTER_SOURCE_NODE_PLUG(mulit_ch_iis0_file_plug) = {
    .uuid       = NODE_UUID_MULTI_CH_IIS0_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
REGISTER_SOURCE_NODE_PLUG(mulit_ch_iis1_file_plug) = {
    .uuid       = NODE_UUID_MULTI_CH_IIS1_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
#endif



