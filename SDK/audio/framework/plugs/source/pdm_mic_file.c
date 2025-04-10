#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pdm_mic_file.data.bss")
#pragma data_seg(".pdm_mic_file.data")
#pragma const_seg(".pdm_mic_file.text.const")
#pragma code_seg(".pdm_mic_file.text")
#endif
#include "source_node.h"
#include "audio_dai/audio_pdm.h"
#include "asm/audio_adc.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_cvp.h"
#include "gpio_config.h"
#include "effects/effects_adj.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if 0
#define pdm_mic_file_log	printf
#else
#define pdm_mic_file_log(...)
#endif/*log_en*/

#if TCFG_PDM_NODE_ENABLE

#define ESCO_PDM_SCLK          2400000 /*1M-4M,SCLK/SR需为整数且在1-4096范围*/
#define ESCO_PDM_IRQ_POINTS    256 /*采样中断点数*/
/*读不到配置时的默认io*/
#define	PLNK_SCLK_PIN			IO_PORTB_02
#define PLNK_DAT0_PIN			IO_PORTB_04
#define PLNK_DAT1_PIN			IO_PORTB_00

/* 保存配置文件信息的结构体 */
struct pdm_file_cfg {
    u8 plnk_ch_num;
    u16 io_sclk_uuid;
    u16 io_ch0_uuid;
    u16 io_ch1_uuid;
} __attribute__((packed));

struct pdm_mic_file_hdl {
    char name[16];
    void *source_node;
    enum stream_scene scene;
    u8 start;
    u8 dump_cnt;
    u16 sample_rate;
    u16 irq_points;
    s16 *buf;
    PLNK_PARM *pdm_mic;
};

static void pdm_mic_output_handler(void *priv, void *data, u32 len)
{
    struct pdm_mic_file_hdl *hdl = (struct pdm_mic_file_hdl *)priv;
    struct stream_frame *frame;

    if (hdl->dump_cnt < 10) {
        hdl->dump_cnt++;
        return;
    }
    if (audio_common_mic_mute_en_get()) {	//mute pdm_mic
        memset((u8 *)data, 0x0, len);
    }

    if (hdl->scene == STREAM_SCENE_ESCO) {	//cvp读dac 参考数据
        audio_cvp_phase_align();
    }

    frame = source_plug_get_output_frame(hdl->source_node, len);
    if (frame) {
        memcpy(frame->data, (u8 *)data, len);
        frame->len = len;
        frame->flags        = FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_PERIOD_SAMPLE | FRAME_FLAG_UPDATE_TIMESTAMP;
        frame->timestamp    = audio_jiffies_usec() * TIMESTAMP_US_DENOMINATOR;
        source_plug_put_output_frame(hdl->source_node, frame);
    }
}

static void *pdm_mic_init(void *source_node, struct stream_node *node)
{
    struct pdm_mic_file_hdl *hdl = zalloc(sizeof(*hdl));
    pdm_mic_file_log("%s\n", __func__);
    hdl->source_node = source_node;
    node->type |= NODE_TYPE_IRQ;
    return hdl;
}

static void pdm_mic_ioc_get_fmt(struct pdm_mic_file_hdl *hdl, struct stream_fmt *fmt)
{
    fmt->coding_type = AUDIO_CODING_PCM;

    switch (hdl->scene) {
    case STREAM_SCENE_ESCO:
        fmt->sample_rate    = 16000;
        fmt->channel_mode   = AUDIO_CH_MIX;
        break;
    default:
        fmt->sample_rate    = 16000;
        fmt->channel_mode   = AUDIO_CH_MIX;
        break;
    }
    hdl->sample_rate = fmt->sample_rate;
}

static int pdm_mic_ioc_set_fmt(struct pdm_mic_file_hdl *hdl, struct stream_fmt *fmt)
{
    hdl->sample_rate = fmt->sample_rate;
    return 0;
}


int pdm_mic_file_param_init(PLNK_PARM *pdm_mic)
{
    if (!pdm_mic) {
        return -1;
    }

    struct pdm_file_cfg pdm_cfg;
    u8 plnk_ch_num = 0;
    char name[16];
    /*
     *获取配置文件内的参数,及名字
     * */
    int len = jlstream_read_node_data_new(NODE_UUID_PDM_MIC, 0XFF, (void *)&pdm_cfg, name);
    if (!len) {
        printf("%s, read node data err\n", __FUNCTION__);
    }
    printf(" %s len %d, sizeof(cfg) %d\n", __func__,  len, (int)sizeof(pdm_cfg));
    if (len == sizeof(pdm_cfg)) {
        pdm_mic->sclk_io = uuid2gpio(pdm_cfg.io_sclk_uuid);
        plnk_ch_num = pdm_cfg.plnk_ch_num;
        if (plnk_ch_num > 0) {
            pdm_mic->data_cfg[0].en = 1;
            pdm_mic->data_cfg[0].io = uuid2gpio(pdm_cfg.io_ch0_uuid);
        }
        if (plnk_ch_num > 1) {
            pdm_mic->data_cfg[1].en = 1;
            pdm_mic->data_cfg[1].io = uuid2gpio(pdm_cfg.io_ch1_uuid);
        }
    } else {
        pdm_mic->sclk_io = PLNK_SCLK_PIN;
        //plnk_ch_num = 1;
        /* if (plnk_ch_num > 0) { */
        pdm_mic->data_cfg[0].en = 1;
        pdm_mic->data_cfg[0].io = PLNK_DAT0_PIN;
        /* } */
        /* if (plnk_ch_num > 1) {
            pdm_mic->data_cfg[1].en = 1;
            pdm_mic->data_cfg[1].io = PLNK_DAT1_PIN;
        } */
    }
    y_printf("plnk_ch_num %d", plnk_ch_num);
    y_printf("sclk_io %d", pdm_mic->sclk_io);
    y_printf("ch0_io %d", pdm_mic->data_cfg[0].io);
    y_printf("ch1_io %d", pdm_mic->data_cfg[1].io);

    /*SCLK:1M-4M,SCLK/SR需为整数且在1-4096范围*/
    pdm_mic->sclk_fre = ESCO_PDM_SCLK;
    if (!pdm_mic->sr) {
        pdm_mic->sr = 16000;
    }
    if (pdm_mic->sclk_fre % pdm_mic->sr) {
        r_printf("[warn]SCLK/SR需为整数且在1-4096范围\n");
    }
    if (plnk_ch_num > 0) {
        pdm_mic->ch_cfg[0].en = 1;
        pdm_mic->ch_cfg[0].mode = DATA0_SCLK_RISING_EDGE;
        pdm_mic->ch_cfg[0].mic_type = DIGITAL_MIC_DATA;
    }
    if (plnk_ch_num > 1) {
        pdm_mic->ch_cfg[1].en = 1;
        pdm_mic->ch_cfg[1].mode = DATA0_SCLK_FALLING_EDGE;
        pdm_mic->ch_cfg[1].mic_type = DIGITAL_MIC_DATA;
    }

    if (!pdm_mic->dma_len) {	//避免没有设置pdm_mic的中断点数，默认256
        pdm_mic->dma_len = (ESCO_PDM_IRQ_POINTS << 1);
    }
    return len;
}

int pdm_node_param_cfg_read(void *priv)
{
    struct pdm_mic_file_hdl *hdl = (struct pdm_mic_file_hdl *)priv;

    if (!hdl->pdm_mic) {
        return -1;
    }

    hdl->pdm_mic->sr = hdl->sample_rate;
    hdl->pdm_mic->dma_len = hdl->irq_points << 1;
    if (!hdl->irq_points) {	//避免没有设置pdm_mic的中断点数，默认256
        hdl->pdm_mic->dma_len = (ESCO_PDM_IRQ_POINTS << 1);
    }
    hdl->pdm_mic->isr_cb = pdm_mic_output_handler;
    hdl->pdm_mic->private_data = hdl;	//保存私有指针

    int len = pdm_mic_file_param_init(hdl->pdm_mic);

    return len;
}

static void pdm_mic_start(struct pdm_mic_file_hdl *hdl)
{
    if (hdl->start == 0) {
        hdl->start = 1;
        hdl->pdm_mic = zalloc(sizeof(PLNK_PARM));
        audio_mic_pwr_ctl(MIC_PWR_ON);
        if (hdl->pdm_mic) {
            /*读取pdm节点的配置*/
            pdm_node_param_cfg_read(hdl);

            hdl->pdm_mic = plnk_init(hdl->pdm_mic);
            plnk_start(hdl->pdm_mic);
        }
    }
}

static void pdm_mic_stop(struct pdm_mic_file_hdl *hdl)
{
    if (hdl->start) {
        hdl->start = 0;
        if (hdl->pdm_mic) {
            plnk_uninit(hdl->pdm_mic);
            audio_mic_pwr_ctl(MIC_PWR_OFF);
            free(hdl->pdm_mic);
            hdl->pdm_mic = NULL;
        }
    }
}

static int pdm_mic_ioctl(void *_hdl, int cmd, int arg)
{
    int ret = 0;
    struct pdm_mic_file_hdl *hdl = (struct pdm_mic_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_GET_FMT:
        pdm_mic_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_FMT:
        ret = pdm_mic_ioc_set_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->irq_points = arg;
        pdm_mic_file_log("pdm_mic_buf points %d\n", hdl->irq_points);
        break;
    case NODE_IOC_START:
        pdm_mic_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        pdm_mic_stop(hdl);
        break;
    }

    return ret;
}

static void pdm_mic_release(void *_hdl)
{
    struct pdm_mic_file_hdl *hdl = (struct pdm_mic_file_hdl *)_hdl;
    if (hdl->buf) {
        free(hdl->buf);
    }
    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(pdm_mic_file_plug) = {
    .uuid       = NODE_UUID_PDM_MIC,
    .init       = pdm_mic_init,
    .ioctl      = pdm_mic_ioctl,
    .release    = pdm_mic_release,
};

#endif/*TCFG_PDM_NODE_ENABLE*/
