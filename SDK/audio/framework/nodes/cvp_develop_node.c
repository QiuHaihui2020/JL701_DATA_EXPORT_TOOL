#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cvp_develop_node.data.bss")
#pragma data_seg(".cvp_develop_node.data")
#pragma const_seg(".cvp_develop_node.text.const")
#pragma code_seg(".cvp_develop_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "circular_buf.h"
#include "cvp_node.h"
#include "app_config.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif

#if TCFG_AUDIO_CVP_DEVELOP_ENABLE
#define CVP_INPUT_SIZE		256*3	//CVP输入缓存，short

struct cvp_cfg_t {
    u8 mic_num;
} __attribute__((packed));

struct cvp_node_hdl {
    struct stream_frame *frame[3];	//输入frame存储，算法输入缓存使用
    struct cvp_cfg_t cfg;
    u8 buf_cnt;						//循环输入buffer位置
    s16 *buf;
    s16 *buf_ref;
    s16 *buf_ref_1;
    u32 ref_sr;
    u16 source_uuid; //源节点uuid
};

static struct cvp_node_hdl *g_cvp_hdl;

int cvp_node_output_handle(s16 *data, u16 len)
{
    struct stream_frame *frame;
    frame = jlstream_get_frame(hdl_node(g_cvp_hdl)->oport, len);
    if (!frame) {
        return 0;
    }
    frame->len = len;
    memcpy(frame->data, data, len);
    jlstream_push_frame(hdl_node(g_cvp_hdl)->oport, frame);
    return len;
}

int cvp_node_param_cfg_read(void *priv, u8 ignore_subid)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)priv;
    int len = 0;
    u8 subid;
    if (g_cvp_hdl) {
        subid = hdl_node(g_cvp_hdl)->subid;
    } else {
        subid = 0XFF;
    }

    struct node_param ncfg = {0};
    len = jlstream_read_node_data(NODE_UUID_CVP_DEVELOP, subid, (u8 *)&ncfg);
    if (len != sizeof(ncfg)) {
        printf("cvp_dms_name read ncfg err\n");
        return -2;
    }
    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &hdl->cfg);
    }
    printf(" %s len %d, sizeof(cfg) %d\n", __func__,  len, (int)sizeof(struct cvp_cfg_t));
    if (len != sizeof(struct cvp_cfg_t)) {
        printf("cvp_develop_param read ncfg err\n");
        return -1 ;
    }
    g_printf("mic_num %d\n", hdl->cfg.mic_num);
    if (!hdl->cfg.mic_num) {
        hdl->cfg.mic_num = 1;
    }
    return len;
}

/*节点输出回调处理，可处理数据或post信号量*/
static void cvp_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)iport->node->private_data;
    s16 *dat, *tbuf, *tbuf_ref, *tbuf_ref_1;
    int wlen;
    struct stream_frame *in_frame;

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!in_frame) {
            break;
        }
#if TCFG_AUDIO_DUT_ENABLE
        //产测bypass 模式 不经过算法
        if (cvp_dut_mode_get() == CVP_DUT_MODE_BYPASS) {
            struct stream_node *node = iport->node;
            jlstream_push_frame(node->oport, in_frame);
            continue;
        }
#endif
        //模仿ADCbuff的存储方法
        if (hdl->cfg.mic_num == 1) {			//单麦第三方算法
            dat = hdl->buf + (in_frame->len / 2 * hdl->buf_cnt);
            memcpy((u8 *)dat, in_frame->data, in_frame->len);
            audio_aec_inbuf(dat, in_frame->len);
        } else if (hdl->cfg.mic_num == 2) {	//双麦第三方算法
            wlen = in_frame->len >> 2;	//单个ADC的点数
            tbuf = hdl->buf + (wlen * hdl->buf_cnt);
            tbuf_ref = hdl->buf_ref + (wlen * hdl->buf_cnt);
            dat = (s16 *)in_frame->data;
            for (int i = 0; i < wlen; i++) {
                tbuf[i]     = dat[2 * i];
                tbuf_ref[i] = dat[2 * i + 1];
            }
#if TCFG_AUDIO_DMS_MIC_MANAGE == DMS_MASTER_MIC0
            audio_aec_inbuf_ref(tbuf_ref, wlen << 1);
            audio_aec_inbuf(tbuf, wlen << 1);
#else
            audio_aec_inbuf_ref(tbuf, wlen << 1);
            audio_aec_inbuf(tbuf_ref, wlen << 1);
#endif/*TCFG_AUDIO_DMS_MIC_MANAGE*/
        } else if (hdl->cfg.mic_num == 3) {	//三麦第三方算法
            wlen = in_frame->len / 6;	//单个ADC的点数
            tbuf = hdl->buf + (wlen * hdl->buf_cnt);
            tbuf_ref = hdl->buf_ref + (wlen * hdl->buf_cnt);
            tbuf_ref_1 = hdl->buf_ref_1 + (wlen * hdl->buf_cnt);
            dat = (s16 *)in_frame->data;
            for (int i = 0; i < wlen; i++) {
                tbuf[i]         = dat[3 * i];
                tbuf_ref[i]     = dat[3 * i + 1];
                tbuf_ref_1[i]   = dat[3 * i + 2];
            }
            audio_aec_inbuf_ref(tbuf_ref, wlen << 1);
            audio_aec_inbuf_ref_1(tbuf_ref_1, wlen << 1);
            audio_aec_inbuf(tbuf, wlen << 1);
        }
        if (++hdl->buf_cnt > ((CVP_INPUT_SIZE / 256) - 1)) {	//计算下一个ADCbuffer位置
            hdl->buf_cnt = 0;
        }
        jlstream_free_frame(in_frame);	//释放iport资源
    }
}

static int cvp_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)node->private_data;


    node->type = NODE_TYPE_ASYNC;
    cvp_node_param_cfg_read(hdl, 0);
    //根据算法单麦/双麦分配对应的空间
    hdl->buf = (s16 *)malloc(CVP_INPUT_SIZE << 1);
    if (hdl->cfg.mic_num == 2) {
        hdl->buf_ref = (s16 *)malloc(CVP_INPUT_SIZE << 1);
    } else if (hdl->cfg.mic_num == 3) {
        hdl->buf_ref = (s16 *)malloc(CVP_INPUT_SIZE << 1);
        hdl->buf_ref_1 = (s16 *)malloc(CVP_INPUT_SIZE << 1);
    }
    g_cvp_hdl = hdl;

    return 0;
}

/*打开改节点输入接口*/
static void cvp_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = cvp_handle_frame;
}


/*节点start函数*/
static void cvp_ioc_start(struct cvp_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    struct audio_aec_init_param_t init_param;
    init_param.sample_rate = fmt->sample_rate;
    init_param.ref_sr = hdl->ref_sr;
    init_param.mic_num = hdl->cfg.mic_num;

    if (hdl->source_uuid == NODE_UUID_ADC) {
        if (audio_adc_file_get_esco_mic_num() != hdl->cfg.mic_num) {
#if TCFG_AUDIO_DUT_ENABLE
            //使能产测时，只有算法模式才需判断
            if (cvp_dut_mode_get() == CVP_DUT_MODE_ALGORITHM) {
                ASSERT(0, "CVP_develop ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), hdl->cfg.mic_num);
            }
#else
            ASSERT(0, "CVP_develop ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), hdl->cfg.mic_num);
#endif
        }
    }

    audio_aec_init(&init_param);
}

/*节点stop函数*/
static void cvp_ioc_stop(struct cvp_node_hdl *hdl)
{
    if (hdl) {
        /* for (int i = 0; i < 3; i++) { */
        /* if (hdl->frame[i] != NULL) {		//检查是否存在未释放的iport缓存 */
        /* jlstream_free_frame(hdl->frame[i]);	//释放iport缓存 */
        /* } */
        /* } */
        audio_aec_close();
    }
}

/*节点ioctl函数*/
static int cvp_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        cvp_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_FMT:
        hdl->ref_sr = (u32)arg;
        break;
    case NODE_IOC_START:
        cvp_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        cvp_ioc_stop(hdl);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->source_uuid = (u16)arg;
        printf("source_uuid %x", (int)hdl->source_uuid);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void cvp_adapter_release(struct stream_node *node)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)node->private_data;
    //根据算法单麦/双麦释放对应的空间
    free(hdl->buf);
    if (hdl->cfg.mic_num == 2) {
        free(hdl->buf_ref);
        hdl->buf_ref = NULL;
    } else if (hdl->cfg.mic_num == 3) {
        free(hdl->buf_ref);
        hdl->buf_ref = NULL;
        free(hdl->buf_ref_1);
        hdl->buf_ref_1 = NULL;
    }
    g_cvp_hdl = NULL;
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(cvp_node_adapter) = {
    .name       = "cvp_develop",
    .uuid       = NODE_UUID_CVP_DEVELOP,
    .bind       = cvp_adapter_bind,
    .ioctl      = cvp_adapter_ioctl,
    .release    = cvp_adapter_release,
    .hdl_size   = sizeof(struct cvp_node_hdl),
};

#endif/*TCFG_CVP_DEVELOP_ENABLE*/
