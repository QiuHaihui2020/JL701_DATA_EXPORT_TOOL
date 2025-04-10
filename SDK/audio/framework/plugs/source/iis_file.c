#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iis_file.data.bss")
#pragma data_seg(".iis_file.data")
#pragma const_seg(".iis_file.text.const")
#pragma code_seg(".iis_file.text")
#endif
#include "source_node.h"
#include "media/audio_splicing.h"
#include "audio_config.h"
#include "jlstream.h"
#include "iis_file.h"
#include "app_config.h"
#include "audio_dai/audio_iis.h"
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

#if TCFG_IIS_NODE_ENABLE

#define IIS_LOG_ENABLE          0
#if IIS_LOG_ENABLE
#define LOG_TAG     "[IIS-FILE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"
#else
#define log_info(fmt, ...)
#define log_error(fmt, ...)
#define log_debug(fmt, ...)
#endif


#define MODULE_IDX_SEL ((hdl->plug_uuid == NODE_UUID_IIS0_RX) ? 0 : 1)
extern const float const_out_dev_pns_time_ms;

struct iis_file_hdl {
    void *source_node;
    struct stream_node *node;
    struct stream_snode *snode;
    enum stream_scene scene;
    enum audio_iis_state_enum state;
    struct audio_iis_rx_output_hdl output;
    cbuffer_t cache_cbuffer;
    u8 *cache_buf;
    u32 buf_len;
    int value;
    int force_dump;
    u32 sample_rate;
    u16 irq_points;
    u16 ch_idx;
    u16 plug_uuid;
    u8 start;
    u8 dump_cnt;
    u8 received_data;
    u8 channel_mode;
    u8 bit_width;
    u8 module_idx;





};

__attribute__((weak))
u32 audio_iis_get_timestamp(u32 module_idx, u8 ch_idx)
{
    return audio_jiffies_usec();
}
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
static void iis_file_fade_in(struct iis_file_hdl *hdl, void *buf, int len)
{
    if (hdl->value < FADE_GAIN_MAX) {
        int fade_ms = 100;//ms
        int fade_step = FADE_GAIN_MAX / (fade_ms * hdl->sample_rate / 1000);
        if (hdl->bit_width == DATA_BIT_WIDE_16BIT) {
            hdl->value = jlstream_fade_in(hdl->value, fade_step, buf, len, AUDIO_CH_NUM(hdl->channel_mode));
        } else {
            hdl->value = jlstream_fade_in_32bit(hdl->value, fade_step, buf, len, AUDIO_CH_NUM(hdl->channel_mode));
        }
    }
}

/* IIS 接收中断测试 */
__NODE_CACHE_CODE(iis)
static void iis_rx_handle(void *priv, void *buf, int len)
{
    struct iis_file_hdl *hdl = priv;
    struct stream_frame *frame = NULL;
    if (!hdl) {
        return;
    }
    if (hdl->received_data == 0) {
        hdl->received_data = 1;
    }

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
    if (hdl->dump_cnt < 10) {
        hdl->dump_cnt++;
        return;
    }
    if (hdl->force_dump) {
        hdl->value = 0;
        return;
    }

    frame = source_plug_get_output_frame(hdl->source_node, len);
    if (!frame) {
        return;
    }
    frame->len          = len;
    frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    frame->timestamp    = audio_iis_get_timestamp(hdl->module_idx, hdl->ch_idx) * TIMESTAMP_US_DENOMINATOR;
    memcpy(frame->data, buf, frame->len);

    iis_file_fade_in(hdl, frame->data, frame->len);//淡入处理
    source_plug_put_output_frame(hdl->source_node, frame);
#else
    if (!hdl->cache_buf) {
        //申请cbuffer
        hdl->buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * ch_num;
        int buf_len = hdl->irq_points * (hdl->bit_width ? 4 : 2) * ch_num * 3;
        hdl->cache_buf = malloc(buf_len);
        if (hdl->cache_buf) {
            cbuf_init(&hdl->cache_cbuffer, hdl->cache_buf, buf_len);
        }
    }
    if (hdl->cache_buf) {
        int wlen = cbuf_write(&hdl->cache_cbuffer, buf, len);
        if (wlen != len) {
            log_debug("iis rx wlen %d != len %d\n", wlen, len);
        }
    }

    u32 cache_len = cbuf_get_data_len(&hdl->cache_cbuffer);
    if (cache_len >= hdl->buf_len) {
        if (hdl->dump_cnt < 10) {
            hdl->dump_cnt++;
            cbuf_read_updata(&hdl->cache_cbuffer, hdl->buf_len);
            return;
        }
        if (hdl->force_dump) {
            cbuf_read_updata(&hdl->cache_cbuffer, hdl->buf_len);
            hdl->value = 0;
            return;
        }

        frame = source_plug_get_output_frame(hdl->source_node, hdl->buf_len);
        if (!frame) {
            return;
        }
        frame->len          = hdl->buf_len;
#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
#else
        frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp    = audio_jiffies_usec();
#endif
        cbuf_read(&hdl->cache_cbuffer, frame->data, frame->len);

        iis_file_fade_in(hdl, frame->data, frame->len);//淡入处理

        source_plug_put_output_frame(hdl->source_node, frame);
    }
#endif

    if (hdl->scene == STREAM_SCENE_ESCO) {	//cvp读dac 参考数据
        audio_cvp_phase_align();
    }
}

void iis_rx_start(struct iis_file_hdl *hdl)
{
    if (hdl && hdl->state == AUDIO_IIS_STATE_INIT) {
        hdl->value = 0;
        audio_iis_open(iis_hdl[hdl->module_idx], hdl->ch_idx);
        audio_iis_set_rx_irq_points(iis_hdl[hdl->module_idx], hdl->irq_points);
        hdl->output.ch_idx = hdl->ch_idx;
        hdl->output.priv = hdl;
        hdl->output.handler = (void (*)(void *, void *, int))iis_rx_handle;
        audio_iis_add_rx_output_handler(iis_hdl[hdl->module_idx], &hdl->output);
        audio_iis_start(iis_hdl[hdl->module_idx]);
        hdl->state = AUDIO_IIS_STATE_START;
    }
}

/*
 * @description: 初始化iis rx
 * @return：iis_file_hdl 结构体指针
 * @note: 无
 */
void iis_rx_init(struct iis_file_hdl *hdl)
{
    if (hdl == NULL) {
        return;
    }
    log_debug(" --- iis_init ---\n");

#if 0 //使用wm8978模块作IIS输入
    u8 WM8978_Init(u8 dacen, u8 adcen);
    WM8978_Init(0, 1);
#endif

    struct audio_general_params *general_params = audio_general_get_param();
    // 初始化iis rx
    // IIS采样率有所差距，配置
    hdl->sample_rate = general_params->sample_rate;	//默认采样率值
    jlstream_read_node_data_by_cfg_index(hdl->plug_uuid, hdl->node->subid, 0, (void *)&hdl->ch_idx, NULL);
    if (!iis_hdl[hdl->module_idx]) {
        struct alink_param params = {0};
        params.module_idx = hdl->module_idx;
        params.dma_size   = audio_iis_fix_dma_len(hdl->module_idx, TCFG_AUDIO_DAC_BUFFER_TIME_MS, AUDIO_IIS_IRQ_POINTS, hdl->bit_width, 2);
        params.sr         = hdl->sample_rate;
        params.bit_width  = hdl->bit_width;
        params.fixed_pns  = const_out_dev_pns_time_ms;
        iis_hdl[hdl->module_idx] = audio_iis_init(params);
    }
    if (!iis_hdl[hdl->module_idx]) {
        log_debug("iis module_idx %d rx init err\n", hdl->module_idx);
        return;
    }
    audio_iis_check_hw_cfg_status(hdl->module_idx, hdl->ch_idx, ALINK_DIR_RX);

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
        hdl->dump_cnt = 0;
    }
}

static void iis_file_stop(struct iis_file_hdl *hdl)
{
    if (hdl->start) {
        hdl->start = 0;
        //停止 IIS 接收
        int ret = 0;
        hdl->output.ch_idx = hdl->ch_idx;
        audio_iis_del_rx_output_handler(iis_hdl[hdl->module_idx], &hdl->output);
        audio_iis_stop(iis_hdl[hdl->module_idx], hdl->ch_idx);
        // 释放IIS RX
        audio_iis_close(iis_hdl[hdl->module_idx]);
        ret = audio_iis_uninit(iis_hdl[hdl->module_idx]);
        if (ret) {
            iis_hdl[hdl->module_idx] = NULL;
        }
        if (hdl->cache_buf) {
            free(hdl->cache_buf);
            hdl->cache_buf = NULL;
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
void iis_release(void *_hdl)
{
    struct iis_file_hdl *hdl = (struct iis_file_hdl *)_hdl;
    log_debug(" --- iis_release ---\n");

    free(hdl);
    hdl = NULL;

    log_debug(">>[%s] : iis rx release succ\n", __func__);
}

REGISTER_SOURCE_NODE_PLUG(iis_file_plug) = {
    .uuid       = NODE_UUID_IIS0_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};
REGISTER_SOURCE_NODE_PLUG(iis1_file_plug) = {
    .uuid       = NODE_UUID_IIS1_RX,
    .init       = iis_file_init,
    .ioctl      = iis_ioctl,
    .release    = iis_release,
};

#endif



