#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein_file.data.bss")
#pragma data_seg(".linein_file.data")
#pragma const_seg(".linein_file.text.const")
#pragma code_seg(".linein_file.text")
#endif
#include "source_node.h"
#include "asm/audio_adc.h"
#include "media/audio_splicing.h"
#include "linein_file.h"
#include "effects/effects_adj.h"
#include "sync/audio_clk_sync.h"

#if 0
#define adc_file_log	printf
#else
#define adc_file_log(...)
#endif/*log_en*/

extern struct audio_adc_hdl adc_hdl;
extern const u8 const_adc_async_en;

extern const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM];

#define LINEIN_ADC_BUF_NUM        2	//linein_adc采样buf个数

static struct linein_file_cfg linein_cfg_g;

int adc_file_linein_open(struct adc_linein_ch *linein, int ch)
{
    int ch_index = 0;
    int linein_gain;
    int linein_pre_gain;
    const struct adc_platform_cfg *cfg_table = adc_platform_cfg_table;
    struct linein_open_param linein_param = {0};

    for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
        if (ch == BIT(i)) {
            ch_index = i;
            break;
        }
    }
    audio_linein_param_fill(&linein_param, &cfg_table[ch_index]);
    linein_gain                   = linein_cfg_g.param[ch_index].mic_gain;
    linein_pre_gain               = linein_cfg_g.param[ch_index].mic_pre_gain;
    printf(">> linein %d, linein_ain_sel %d, linein_mode %d, linein_dcc %d, linein_gain %d, linein_pre_gain %d", ch_index, linein_param.linein_ain_sel, linein_param.linein_mode, linein_param.linein_dcc, linein_gain, linein_pre_gain);
    audio_adc_linein_open(linein, ch, &adc_hdl, &linein_param);
    audio_adc_linein_set_gain(linein, ch, linein_gain);
    audio_adc_linein_gain_boost(ch, linein_pre_gain);
    return 0;
}

#if TCFG_AUDIO_LINEIN_ENABLE


void audio_linein_file_init()
{
    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    int len = jlstream_read_form_data(mode_index, "linein_adc", cfg_index, &linein_cfg_g);
    if (len != sizeof(struct linein_file_cfg)) {
        printf("linein_file read cfg data err !!!\n");
    }
    adc_file_log(" %s len %d, sizeof(cfg) %d\n", __func__,  len, (int)sizeof(struct linein_file_cfg));
#if 1
    adc_file_log(" linein_cfg_g.mic_en_map = %x\n", linein_cfg_g.mic_en_map);
    for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
        adc_file_log(" linein_cfg_g.param[%d].mic_gain      = %d\n", i, linein_cfg_g.param[i].mic_gain);
        adc_file_log(" linein_cfg_g.param[%d].mic_pre_gain  = %d\n", i, linein_cfg_g.param[i].mic_pre_gain);
    }
#endif
    for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
        if (linein_cfg_g.mic_en_map & BIT(i)) {
            audio_adc_add_ch(&adc_hdl, i);
        }
    }
}



/**
 * @brief       LINEIN 的中断回调函数
 *
 * @param _hdl  LINEIN 节点的操作句柄
 * @param data  LINEIN 中断采集到的数据地址
 * @param len   LINEIN 单个通道中断采集到的数据字节长度
 */
static void adc_linein_output_handler(void *_hdl, s16 *data, int len)
{

    struct linein_file_hdl *hdl = (struct linein_file_hdl *)_hdl;
    struct stream_frame *frame;

    if (hdl->dump_cnt < 10) {
        hdl->dump_cnt++;
        return;
    }
    frame = source_plug_get_output_frame(hdl->source_node, (len * hdl->ch_num));
    if (frame) {
        if (hdl->ch_num != adc_hdl.max_adc_num && const_adc_async_en) {
            //printf("l:%d,%d,%d\n",hdl->adc_seq,hdl->ch_num,adc_hdl.max_adc_num);
            if (adc_hdl.bit_width != ADC_BIT_WIDTH_16) {
                s32 *s32_src = (s32 *)data;
                s32 *s32_dst = (s32 *)frame->data;
                for (int i = 0; i < len / 4; i++) {
                    for (int j = 0; j < hdl->ch_num; j++) {
                        s32_dst[hdl->ch_num * i + j] = s32_src[adc_hdl.max_adc_num * i + hdl->adc_seq + j];
                    }
                }
            } else {
                s16 *s16_src = data;
                s16 *s16_dst = (s16 *)frame->data;
                for (int i = 0; i < len / 2; i++) {
                    for (int j = 0; j < hdl->ch_num; j++) {
                        s16_dst[hdl->ch_num * i + j] = s16_src[adc_hdl.max_adc_num * i + hdl->adc_seq + j];
                    }
                }
            }
        } else {
            memcpy(frame->data, data, (len * hdl->ch_num));
        }
        len *= hdl->ch_num;
        if (hdl->mute_en) {	//mute ADC
            memset((u8 *)frame->data, 0x0, len);
        }
        if (hdl->output_fade_in) {
            if (adc_hdl.bit_width == ADC_BIT_WIDTH_16) {
                s16 tmp_data;
                s16 *tmp_data_ptr = (s16 *)frame->data;
                /* printf("fade:%d\n",hdl->output_fade_in_gain); */
                for (int i = 0; i < len / 2; i++) {
                    tmp_data = tmp_data_ptr[i];
                    tmp_data_ptr[i] = tmp_data * hdl->output_fade_in_gain >> 8;
                }
                hdl->output_fade_in_gain += 1;
                if (hdl->output_fade_in_gain >= 256) {
                    hdl->output_fade_in = 0;
                }
            } else {
                s32 tmp_data;
                s32 *tmp_data_ptr = (s32 *)frame->data;
                /* printf("fade:%d\n",hdl->output_fade_in_gain); */
                for (int i = 0; i < len / 4; i++) {
                    tmp_data = tmp_data_ptr[i];
                    tmp_data_ptr[i] = tmp_data * hdl->output_fade_in_gain >> 8;
                }
                hdl->output_fade_in_gain += 1;
                if (hdl->output_fade_in_gain >= 256) {
                    hdl->output_fade_in = 0;
                }
            }
        }
        frame->len          = len;
#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = adc_hdl.timestamp * TIMESTAMP_US_DENOMINATOR;
#else
        frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp    = adc_hdl.timestamp;
#endif
        source_plug_put_output_frame(hdl->source_node, frame);
    }
}

static void *linein_init(void *source_node, struct stream_node *node)
{
    struct linein_file_hdl *hdl = zalloc(sizeof(*hdl));
    hdl->source_node = source_node;
    node->type |= NODE_TYPE_IRQ;

    for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
        if (linein_cfg_g.mic_en_map & BIT(i)) {
            hdl->ch_num++;
        }
    }

    adc_file_log("adc ch_num %d\n", hdl->ch_num);
    return hdl;
}

static void adc_ioc_get_fmt(struct linein_file_hdl *hdl, struct stream_fmt *fmt)
{
    fmt->coding_type = AUDIO_CODING_PCM;
    switch (hdl->ch_num) {
    case 2:
        fmt->sample_rate    = 44100;
        fmt->channel_mode   = AUDIO_CH_LR;
        break;
    default:
        fmt->sample_rate    = 44100;
        fmt->channel_mode   = AUDIO_CH_L;
        break;
    }
    hdl->sample_rate = fmt->sample_rate;

    if (adc_hdl.bit_width == ADC_BIT_WIDTH_24) {
        fmt->bit_wide = DATA_BIT_WIDE_24BIT;
    } else {
        fmt->bit_wide = DATA_BIT_WIDE_16BIT;
    }
}

static int adc_ioc_set_fmt(struct linein_file_hdl *hdl, struct stream_fmt *fmt)
{
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}

static int linein_file_ioc_update_parm(struct linein_file_hdl *hdl, int parm)
{
    int ret = false;
    struct linein_file_cfg *cfg = (struct linein_file_cfg *)parm;
    if (hdl) {
        for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
            if (cfg->mic_en_map & BIT(i)) {
                audio_adc_linein_set_gain(&hdl->linein_ch, BIT(i), cfg->param[i].mic_gain);
                audio_adc_linein_gain_boost(BIT(i), cfg->param[i].mic_pre_gain);
            }
        }
        ret = true;
    }
    return ret;
}

static int linein_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct linein_file_hdl *hdl = (struct linein_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_GET_FMT:
        adc_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = adc_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->irq_points = arg;
        if (!adc_hdl.hw_buf) {
            hdl->adc_buf = zalloc(LINEIN_ADC_BUF_NUM * hdl->irq_points * ((adc_hdl.bit_width == ADC_BIT_WIDTH_16) ? 2 : 4) * adc_hdl.max_adc_num);
        }
        if (!hdl->adc_buf) {
            ret = -1;
        }
        adc_file_log("adc_buf points %d\n", hdl->irq_points);
        break;
    case NODE_IOC_START:
        if (hdl->start == 0) {
            hdl->output_fade_in = 1;
            hdl->start = 1;
            hdl->dump_cnt = 0;
            int linein_en_map = 0;
            for (int i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
                if (linein_cfg_g.mic_en_map & BIT(i)) {
                    adc_file_linein_open(&hdl->linein_ch, BIT(i));
                    linein_en_map |= BIT(i);
                }
            }
            hdl->adc_seq = get_adc_seq(&adc_hdl, linein_en_map); //查询模拟linein对应的ADC通道
            if (!hdl->adc_buf && !adc_hdl.hw_buf) {	//避免没有设置ADC的中断点数，以及数据流stop之后重新start申请buffer
                if (!hdl->irq_points) {
                    hdl->irq_points = 256;
                }
                hdl->adc_buf = zalloc(LINEIN_ADC_BUF_NUM  * hdl->irq_points * ((adc_hdl.bit_width == ADC_BIT_WIDTH_16) ? 2 : 4)  * (adc_hdl.max_adc_num));
                if (!hdl->adc_buf) {
                    ret = -1;
                    break;
                }
            }
            ret = audio_adc_linein_set_buffs(&hdl->linein_ch, hdl->adc_buf, hdl->irq_points * 2, LINEIN_ADC_BUF_NUM);
            if (ret && hdl->adc_buf) {
                free(hdl->adc_buf);
                hdl->adc_buf = NULL;
            }
            audio_adc_linein_set_sample_rate(&hdl->linein_ch, hdl->sample_rate);

            hdl->adc_output.priv    = hdl;
            hdl->adc_output.handler = adc_linein_output_handler;
            audio_adc_add_output_handler(&adc_hdl, &hdl->adc_output);

            audio_adc_linein_start(&hdl->linein_ch);
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
            audio_adc_linein_close(&hdl->linein_ch);
            if (!adc_hdl.hw_buf) {
                hdl->adc_buf = NULL; //在adc 驱动中释放了这个buffer
            }
            audio_adc_del_output_handler(&adc_hdl, &hdl->adc_output);
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = linein_file_ioc_update_parm(hdl, arg);
        break;
    }

    return ret;
}

static void linein_release(void *_hdl)
{
    struct linein_file_hdl *hdl = (struct linein_file_hdl *)_hdl;
    if (hdl->adc_buf) {
        /* free(hdl->adc_buf);  */ //由adc 驱动释放buffer
    }
    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(linein_file_plug) = {
    .uuid       = NODE_UUID_LINEIN,
    .init       = linein_init,
    .ioctl      = linein_ioctl,
    .release    = linein_release,
};

REGISTER_ONLINE_ADJUST_TARGET(linein) = {
    .uuid = NODE_UUID_LINEIN,
};
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
