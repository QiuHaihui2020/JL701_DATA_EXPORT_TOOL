#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_file.data.bss")
#pragma data_seg(".fm_file.data")
#pragma const_seg(".fm_file.text.const")
#pragma code_seg(".fm_file.text")
#endif
#include "source_node.h"
#include "asm/audio_adc.h"
#include "media/audio_splicing.h"
#include "fm_file.h"
#include "fm_manage.h"
#include "asm/fm_inside_api.h"
#include "fm_api.h"
#include "sync/audio_clk_sync.h"
#include "effects/effects_adj.h"
#include "app_msg.h"
#include "gpio_config.h"
#include "linein_file.h"

#if 1
#define adc_file_log	printf
#else
#define adc_file_log(...)
#endif/*log_en*/

#if TCFG_AUDIO_FM_ENABLE

struct fm_file_hdl {
    void *source_node;
    struct adc_linein_ch linein_ch;
    struct audio_adc_output_hdl adc_output;
    s16 *adc_buf;
    u8 start;
    u8 dump_cnt;
    u8 ch_num;
    u8 mute_en;
    u8 adc_seq;
    u16 irq_points;
    u16 sample_rate;
};

struct fm_file_common {
    u8 read_flag;				//读取配置的标志
    u8 get_logo_flag;			//获取到logo信息的标志
    char logo[20];
    struct fm_file_hdl *hdl;	//当前ADC节点的句柄
    /* struct fm_inside_file_cfg inside_cfg;	//inside fm */
    struct fm_outside_file_cfg outside_cfg;	//outside fm
};

extern struct audio_adc_hdl adc_hdl;

static struct fm_file_common adc_f;

struct fm_outside_file_cfg *audio_outside_fm_file_get_cfg(void)
{
    return &adc_f.outside_cfg;
}

void fm_file_set_logo(char *logo, int len)
{
    memcpy(adc_f.logo, logo, len);
    adc_f.get_logo_flag = 1;
    y_printf(">>>>>>>>> Func:%s, Logo:%s\n", __func__, adc_f.logo);
}

// 获取读取到的参数 clk io 的uuid值，返回 -1 表示获取失败
int fm_qn8035_get_clk_io_uuid(void)
{
    if (adc_f.read_flag) {
        return adc_f.outside_cfg.clk_io_uuid;
    }
    return -1;
}

/*
 * 设备使能了，返回1，没有使能返回0，找不到注册的设备返回0
 * logo 区分大小写，logo 见各驱动文件
 * 使用示例：fm_get_device_en("qn3085");
 */
int fm_get_device_en(char *logo)
{
#if (TCFG_FM_QN8035_ENABLE)
    if (memcmp(logo, "qn8035", strlen(logo)) == 0) {
        return TCFG_FM_QN8035_ENABLE;
    }
#endif
#if (TCFG_FM_RDA5807_ENABLE)
    if (memcmp(logo, "rda5807", strlen(logo)) == 0) {
        return TCFG_FM_RDA5807_ENABLE;
    }
#endif
#if (TCFG_FM_BK1080_ENABLE)
    if (memcmp(logo, "bk1080", strlen(logo)) == 0) {
        return TCFG_FM_BK1080_ENABLE;
    }
#endif
#ifdef  TCFG_FM_INSIDE_ENABLE
    if (memcmp(logo, "fm_inside", strlen(logo)) == 0) {
        return TCFG_FM_INSIDE_ENABLE;
    }
#endif
    return 0;
}

void audio_fm_file_init(void)
{
    if (!adc_f.read_flag) {
        //解析配置文件内fm配置
        int rlen = syscfg_read(CFG_FM_OUTSIDE_CFG_ID, &adc_f.outside_cfg, sizeof(adc_f.outside_cfg));
        if (rlen) {
            g_printf(">>>> Read FM Outside Success!");
            y_printf(">>>> Outside :");
            printf("linein_en_map : %d", adc_f.outside_cfg.linein_en_map);
            printf("clk_io_uuid : %d, gpio:%d", adc_f.outside_cfg.clk_io_uuid, uuid2gpio(adc_f.outside_cfg.clk_io_uuid));
        } else {
            r_printf(">>>> %s, %d, Read fm info failed!\n", __func__, __LINE__);
            goto __exit;
        }
        adc_f.read_flag = 1;
    }

__exit:
    return;
}

/**
 * @brief       adc fm 的中断回调函数
 *
 * @param _hdl  FM 节点的操作句柄
 * @param data  FM ADC 中断采集到的数据地址
 * @param len   单个通道中断采集到的数据字节长度
 */
extern volatile u8 fm_mute_flag;
static void adc_outside_fm_output_handler(void *_hdl, s16 *data, int len)
{
    struct fm_file_hdl *hdl = (struct fm_file_hdl *)_hdl;

    struct stream_frame *frame;

    if (hdl->start == 0) {
        return;
    }

    if (hdl->dump_cnt < 10) {
        hdl->dump_cnt++;
        return;
    }

    frame = source_plug_get_output_frame(hdl->source_node, (len * hdl->ch_num));
    if (frame) {
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
        len *= hdl->ch_num;

        frame->len          = len;
#if 1
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
#else
        frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
        frame->timestamp    = audio_jiffies_usec();
#endif

        if (fm_mute_flag == 1) {
            memset(frame->data, 0, frame->len);
        }

        source_plug_put_output_frame(hdl->source_node, frame);
    }
}

// fm 外设，打开adc采样模拟信号输出
static void fm_outside_adc_open(void)
{
    y_printf("=========== %s ===========\n", __func__);
    struct fm_file_hdl *hdl = adc_f.hdl;
    int i = 0;
    for (i = 0; i < AUDIO_ADC_LINEIN_MAX_NUM; i++) {
        if (adc_f.outside_cfg.linein_en_map & BIT(i)) {
            adc_file_linein_open(&hdl->linein_ch, BIT(i));
            hdl->ch_num++;
        }
    }
    if (!hdl->adc_buf) {
        hdl->irq_points = FM_ADC_IRQ_POINTS;
        hdl->adc_buf = zalloc(FM_ADC_BUF_NUM * hdl->irq_points * ((adc_hdl.bit_width == ADC_BIT_WIDTH_16) ? 2 : 4) * (adc_hdl.max_adc_num));
        if (!hdl->adc_buf) {
            r_printf(">>>> error , %s, %d\n", __func__, __LINE__);
            return;
        }
    }
    int ret = audio_adc_linein_set_buffs(&hdl->linein_ch, hdl->adc_buf, hdl->irq_points * 2, FM_ADC_BUF_NUM);
    if (ret && hdl->adc_buf) {
        free(hdl->adc_buf);
        hdl->adc_buf = NULL;
        r_printf(">>>> error , %s, %d\n", __func__, __LINE__);
        //return;
    }

    y_printf(">> %s, %d, hdl->ch_num:%d, hdl->samle_rate:%d\n", __func__, __LINE__, hdl->ch_num, hdl->sample_rate);
    audio_adc_linein_set_sample_rate(&hdl->linein_ch, hdl->sample_rate);
    hdl->adc_output.priv    = hdl;
    hdl->adc_output.handler = adc_outside_fm_output_handler;
    audio_adc_add_output_handler(&adc_hdl, &hdl->adc_output);

    audio_adc_linein_start(&hdl->linein_ch);
    g_printf(">>>>>> %s open success!\n",  __func__);
}


static void fm_outside_adc_close(void)
{
    struct fm_file_hdl *hdl = adc_f.hdl;
    audio_adc_linein_close(&hdl->linein_ch);
    audio_adc_del_output_handler(&adc_hdl, &hdl->adc_output);
}

__NODE_CACHE_CODE(fm)
void fm_sample_output_handler(s16 *data, int len)
{
    struct fm_file_hdl *hdl = adc_f.hdl;
    struct stream_frame *frame;

    frame = source_plug_get_output_frame(hdl->source_node, len);
    if (!frame) {
        return;
    }
    frame->len          = len;
#if 1
    frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
#else
    frame->flags        = FRAME_FLAG_SYS_TIMESTAMP_ENABLE;
    frame->timestamp    = audio_jiffies_usec();
#endif
    memcpy(frame->data, data, len);

    source_plug_put_output_frame(hdl->source_node, frame);
    return;
}

static void *fm_init(void *source_node, struct stream_node *node)
{
    struct fm_file_hdl *hdl = zalloc(sizeof(*hdl));
    adc_f.hdl = hdl;
    hdl->source_node = source_node;
    if (memcmp(adc_f.logo, "fm_inside", strlen((char *)(adc_f.logo))) != 0) {
        //外挂FM
        y_printf(">>>>> Open FM Outside!\n");
        hdl->ch_num = 0;
    } else {
        // 内置FM
        y_printf(">>>>> Open FM Inside!\n");
        adc_file_log("adc ch_num %d\n", hdl->ch_num);
        hdl->ch_num = 2;
    }
    node->type |= NODE_TYPE_IRQ;
    return hdl;
}

static void adc_ioc_get_fmt(struct fm_file_hdl *hdl, struct stream_fmt *fmt)
{
    if (memcmp(adc_f.logo, "fm_inside", strlen((char *)(adc_f.logo))) != 0) {
        //外挂FM
        fmt->coding_type = AUDIO_CODING_PCM;
        fmt->sample_rate    = FM_ADC_SAMPLE_RATE;
        fmt->channel_mode   = AUDIO_CH_L;
        hdl->sample_rate = fmt->sample_rate;
        if (adc_hdl.bit_width == ADC_BIT_WIDTH_24) {
            fmt->bit_wide = DATA_BIT_WIDE_24BIT;
        } else {
            fmt->bit_wide = DATA_BIT_WIDE_16BIT;
        }
    } else {
        //内置FM
        fmt->coding_type = AUDIO_CODING_PCM;
        fmt->sample_rate    = 37500;
        fmt->channel_mode   = AUDIO_CH_LR;
        hdl->sample_rate = fmt->sample_rate;
        fmt->bit_wide = DATA_BIT_WIDE_16BIT;
    }
}

static int adc_ioc_set_fmt(struct fm_file_hdl *hdl, struct stream_fmt *fmt)
{
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}

static int fm_ioctl(void *_hdl, int cmd, int arg)
{
    u32 i = 0;
    int ret = 0;
    struct fm_file_hdl *hdl = (struct fm_file_hdl *)_hdl;
    switch (cmd) {
    case NODE_IOC_GET_FMT:
        adc_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = adc_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        break;
    case NODE_IOC_START:
        if (hdl->start == 0) {
            hdl->dump_cnt = 0;
            if (memcmp(adc_f.logo, "fm_inside", strlen((char *)(adc_f.logo))) != 0) {
                //外挂FM
                hdl->adc_seq = get_adc_seq(&adc_hdl, adc_f.outside_cfg.linein_en_map); //查询模拟linein对应的ADC通道
                fm_outside_adc_open();
            } else {
                //内置FM
#if TCFG_FM_INSIDE_ENABLE
                fm_inside_start();
#endif
            }
            hdl->start = 1;
        }
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        if (hdl->start) {
            hdl->start = 0;
            if (memcmp(adc_f.logo, "fm_inside", strlen((char *)(adc_f.logo))) != 0) {
                //外挂FM
                fm_outside_adc_close();
            } else {
                //内置FM
#if TCFG_FM_INSIDE_ENABLE
                fm_inside_pause();
#endif
            }
        }
        break;
    }

    return ret;
}

static void fm_release(void *_hdl)
{
    struct fm_file_hdl *hdl = (struct fm_file_hdl *)_hdl;
    if (hdl->adc_buf) {
        //在 adc 驱动中释放这个buffer
        /* free(hdl->adc_buf); */
        hdl->adc_buf = NULL;
    }
    free(hdl);
    adc_f.hdl = NULL;
}

REGISTER_SOURCE_NODE_PLUG(fm_file_plug) = {
    .uuid       = NODE_UUID_FM,
    .init       = fm_init,
    .ioctl      = fm_ioctl,
    .release    = fm_release,
};
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
