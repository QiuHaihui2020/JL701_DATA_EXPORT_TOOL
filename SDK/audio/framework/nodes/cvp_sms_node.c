#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cvp_sms_node.data.bss")
#pragma data_seg(".cvp_sms_node.data")
#pragma const_seg(".cvp_sms_node.text.const")
#pragma code_seg(".cvp_sms_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "circular_buf.h"
#include "cvp_node.h"
#include "app_config.h"
#include "audio_iis.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif

/*(单MIC+ANS通话) || (单MIC+DNS通话)*/
#if (TCFG_AUDIO_CVP_SMS_ANS_MODE) || (TCFG_AUDIO_CVP_SMS_DNS_MODE)

#define CVP_INPUT_SIZE		256*3	//CVP输入缓存，short

//------------------stream.bin CVP参数文件解析结构-START---------------//
struct CVP_REF_MIC_CONFIG {
    u8 en;          //ref 回采硬使能
    u8 ref_mic_ch;	//ref 硬回采MIC通道选择
} __attribute__((packed));

struct CVP_PRE_GAIN_CONFIG {
    u8 en;
    float talk_mic_gain;	//主MIC前级数字增益，default:0dB(-90 ~ 40dB)
} __attribute__((packed));

struct CVP_AEC_CONFIG {
    u8 en;
    float aec_dt_aggress;   //原音回音追踪等级, default: 1.0f(1 ~ 5)
    float aec_refengthr;    //进入回音消除参考值, default: -70.0f(-90 ~ -60 dB)
} __attribute__((packed));

struct CVP_NLP_CONFIG {
    u8 en;
    float es_aggress_factor;//回音前级动态压制,越小越强,default: -3.0f(-1 ~ -5)
    float es_min_suppress;	//回音后级静态压制,越大越强,default: 4.f(0 ~ 10)
} __attribute__((packed));

struct CVP_NS_CONFIG {
    u8 en;
    float ns_aggress;    	//噪声前级动态压制,越大越强default: 1.25f(1 ~ 2)
    float ns_suppress;    	//噪声后级静态压制,越小越强default: 0.04f(0 ~ 1)
    float init_noise_lvl;	//初始噪声水平，default:-75dB,range[-100:-30]
} __attribute__((packed));

struct CVP_AGC_CONFIG {
    u8 en;
    float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
    float dt_max_gain;   	//双端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -40 dB)
    float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -40 dB)
} __attribute__((packed));

struct cvp_cfg_t {
    struct CVP_REF_MIC_CONFIG ref_mic;
    struct CVP_PRE_GAIN_CONFIG pre_gain;
    struct CVP_AEC_CONFIG aec;
    struct CVP_NLP_CONFIG nlp;
    struct CVP_NS_CONFIG ns;
    struct CVP_AGC_CONFIG agc;
} __attribute__((packed));
//------------------stream.bin CVP参数文件解析结构-END---------------//

struct cvp_node_hdl {
    char name[16];
    enum stream_scene scene;
    AEC_CONFIG online_cfg;
    struct stream_frame *frame[3];	//输入frame存储，算法输入缓存使用
    u8 buf_cnt;						//循环输入buffer位置
    s16 buf[CVP_INPUT_SIZE];
    s16 *buf_1; //回采buf
    u32 ref_sr;
    u16 source_uuid; //源节点uuid
    u8 talk_mic_ch;					//通话MIC
    struct CVP_REF_MIC_CONFIG ref_mic;
    u8 ref_mic_num; //回采mic个数
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

extern float eq_db2mag(float x);
void cvp_node_param_cfg_update(struct cvp_cfg_t *cfg, AEC_CONFIG *p)
{
    if (cfg == NULL) {
        return;
    }
    if (g_cvp_hdl) {
        g_cvp_hdl->ref_mic.en = cfg->ref_mic.en;
        g_cvp_hdl->ref_mic.ref_mic_ch = cfg->ref_mic.ref_mic_ch;
        if (g_cvp_hdl->ref_mic.en && (g_cvp_hdl->buf_1 == NULL)) {
            /*计算回采mic个数*/
            g_cvp_hdl->ref_mic_num = audio_get_mic_num(g_cvp_hdl->ref_mic.ref_mic_ch);
            g_cvp_hdl->buf_1 = zalloc(CVP_INPUT_SIZE * sizeof(short) * g_cvp_hdl->ref_mic_num);
        }
        u8 mic_ch = audio_adc_file_get_mic_en_map();
        for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i ++) {
            /*如果是硬回采MIC，跳过本次判断*/
            if (BIT(i) & g_cvp_hdl->ref_mic.ref_mic_ch) {
                continue;
            }
            /*查找哪个是通话MIC*/
            if (BIT(i) & mic_ch) {
                g_cvp_hdl->talk_mic_ch = BIT(i);
                break;
            }
        }
        printf("talk_mic %x, ref mic en %d, ref_mic_ch %x", g_cvp_hdl->talk_mic_ch, g_cvp_hdl->ref_mic.en, g_cvp_hdl->ref_mic.ref_mic_ch);
    }
    if (p) {
        p->adc_ref_en = cfg->ref_mic.en;

        //更新预处理参数
        struct audio_cvp_pre_param_t pre_cfg;
        pre_cfg.pre_gain_en = cfg->pre_gain.en;
        pre_cfg.talk_mic_gain = eq_db2mag(cfg->pre_gain.talk_mic_gain);
        audio_cvp_probe_param_update(&pre_cfg);

        //更新算法参数
        p->aec_mode = cfg->aec.en | (cfg->nlp.en << 1) | (cfg->ns.en << 2) | (cfg->agc.en << 4);
        p->ul_eq_en = 0;

        p->ndt_fade_in = cfg->agc.ndt_fade_in;
        p->ndt_fade_out = cfg->agc.ndt_fade_out;
        p->dt_fade_in = cfg->agc.dt_fade_in;
        p->dt_fade_out = cfg->agc.dt_fade_out;
        p->ndt_max_gain = cfg->agc.ndt_max_gain;
        p->ndt_min_gain = cfg->agc.ndt_min_gain;
        p->ndt_speech_thr = cfg->agc.ndt_speech_thr;
        p->dt_max_gain = cfg->agc.dt_max_gain;
        p->dt_min_gain = cfg->agc.dt_min_gain;
        p->dt_speech_thr = cfg->agc.dt_speech_thr;
        p->echo_present_thr = cfg->agc.echo_present_thr;
        p->aec_dt_aggress = cfg->aec.aec_dt_aggress;
        p->aec_refengthr = cfg->aec.aec_refengthr;
        p->es_aggress_factor = cfg->nlp.es_aggress_factor;
        p->es_min_suppress = cfg->nlp.es_min_suppress;
        p->ans_aggress = cfg->ns.ns_aggress;
        p->ans_suppress = cfg->ns.ns_suppress;
        p->init_noise_lvl = cfg->ns.init_noise_lvl;
    }
}

struct cvp_cfg_t global_cvp_cfg;
int cvp_param_cfg_read(void)
{
    u8 subid;
    if (g_cvp_hdl) {
        subid = hdl_node(g_cvp_hdl)->subid;
    } else {
        subid = 0XFF;
    }
    /*
     *解析配置文件内效果配置
     * */
    int len = 0;
    struct node_param ncfg = {0};
#if TCFG_AUDIO_CVP_SMS_ANS_MODE
    len = jlstream_read_node_data(NODE_UUID_CVP_SMS_ANS, subid, (u8 *)&ncfg);
#else /*TCFG_AUDIO_CVP_SMS_DNS_MODE*/
    len = jlstream_read_node_data(NODE_UUID_CVP_SMS_DNS, subid, (u8 *)&ncfg);
#endif
    if (len != sizeof(ncfg)) {
        printf("cvp_sms_node read ncfg err\n");
        return -2;
    }

    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &global_cvp_cfg);
    }

    printf(" %s len %d, sizeof(global_cvp_cfg) %d\n", __func__,  len, (int)sizeof(global_cvp_cfg));
    if (len != sizeof(global_cvp_cfg)) {
        printf("cvp_sms_param read ncfg err\n");
        return -1 ;
    }
    return 0 ;
}

u8 cvp_get_talk_mic_ch(void)
{
    u8 talk_mic_ch = audio_adc_file_get_mic_en_map();;

    if (global_cvp_cfg.ref_mic.en) { //参考数据硬回采
        u8 ref_mic_ch = global_cvp_cfg.ref_mic.ref_mic_ch;
        u8 mic_ch = audio_adc_file_get_mic_en_map();
        for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i ++) {
            /*如果是硬回采MIC，跳过本次判断*/
            if (BIT(i) & ref_mic_ch) {
                continue;
            }
            /*查找哪个是通话MIC*/
            if (BIT(i) & mic_ch) {
                talk_mic_ch = BIT(i);
                break;
            }
        }
    }

    return talk_mic_ch;
}

int cvp_node_param_cfg_read(void *priv, u8 ignore_subid)
{
    AEC_CONFIG *p = (AEC_CONFIG *)priv;
    struct cvp_cfg_t cfg;
    u8 subid;
    if (g_cvp_hdl) {
        subid = hdl_node(g_cvp_hdl)->subid;
    } else {
        subid = 0XFF;
    }
    /*
     *解析配置文件内效果配置
     * */
    int len = 0;
    struct node_param ncfg = {0};
#if TCFG_AUDIO_CVP_SMS_ANS_MODE
    len = jlstream_read_node_data(NODE_UUID_CVP_SMS_ANS, subid, (u8 *)&ncfg);
#else /*TCFG_AUDIO_CVP_SMS_DNS_MODE*/
    len = jlstream_read_node_data(NODE_UUID_CVP_SMS_DNS, subid, (u8 *)&ncfg);
#endif
    if (len != sizeof(ncfg)) {
        printf("cvp_sms_node read ncfg err\n");
        return 0;
    }

    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &cfg);
    }
    printf(" %s len %d, sizeof(cfg) %d\n", __func__,  len, (int)sizeof(cfg));
    if (len != sizeof(cfg)) {
        return 0 ;
    }
    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        if (g_cvp_hdl) {
            memcpy(g_cvp_hdl->name, ncfg.name, sizeof(ncfg.name));
            if (jlstream_read_effects_online_param(hdl_node(g_cvp_hdl)->uuid, g_cvp_hdl->name, &cfg, sizeof(cfg))) {
                printf("get cvp online param\n");
            }
        }
    }
    cvp_node_param_cfg_update(&cfg, p);

    return sizeof(AEC_CONFIG);
}

/*节点输出回调处理，可处理数据或post信号量*/
static void cvp_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)iport->node->private_data;
    s16 *dat, *mic_buf, *ref_buf;
    u8 mic_index = 0, ref_index[2] = {0};
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
        if (hdl->ref_mic.en) { //参考数据硬回采
            wlen = in_frame->len / ((1 + hdl->ref_mic_num) * 2);	//一个ADC的点数
            //模仿ADCbuff的存储方法
            mic_buf = hdl->buf + (wlen * hdl->buf_cnt);
            ref_buf = hdl->buf_1 + (wlen * hdl->buf_cnt * hdl->ref_mic_num);
            if (++hdl->buf_cnt > ((CVP_INPUT_SIZE / 256) - 1)) {
                hdl->buf_cnt = 0;
            }

            /*查找对应mic的数据index*/
            u8 mic_cnt = 0, ref_cnt = 0;
            for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
                if (hdl->ref_mic.ref_mic_ch & BIT(i)) {
                    ref_index[ref_cnt++] = mic_cnt++;
                    continue;
                }
                if (hdl->talk_mic_ch & BIT(i)) {
                    mic_index = mic_cnt++;
                    continue;
                }
            }

            dat = (s16 *)in_frame->data;
            if (hdl->ref_mic_num == 1) {
                /*分类2个mic的数据*/
                for (int i = 0; i < wlen; i++) {
                    mic_buf[i] = dat[2 * i + mic_index];
                    ref_buf[i] = dat[2 * i + ref_index[0]];
                }

            } else if (hdl->ref_mic_num == 2) {
                /*分类3个mic的数据*/
                for (int i = 0; i < wlen; i++) {
                    mic_buf[i]          = dat[3 * i + mic_index];
                    ref_buf[2 * i]      = dat[3 * i + ref_index[0]];
                    ref_buf[2 * i + 1]  = dat[3 * i + ref_index[1]];
                }
            }
            audio_aec_refbuf(ref_buf, NULL, wlen << hdl->ref_mic_num);
            audio_aec_inbuf(mic_buf, wlen << 1);
        } else {//参考数据软回采
            dat = hdl->buf + (in_frame->len / 2 * hdl->buf_cnt);
            //模仿ADCbuff的存储方法
            memcpy((u8 *)dat, in_frame->data, in_frame->len);
            audio_aec_inbuf(dat, in_frame->len);
            if (++hdl->buf_cnt > ((CVP_INPUT_SIZE / 256) - 1)) {
                hdl->buf_cnt = 0;
            }
        }
        jlstream_free_frame(in_frame);	//释放iport资源
    }
}

/*节点预处理-在ioctl之前*/
static int cvp_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)node->private_data;

    node->type = NODE_TYPE_ASYNC;
    g_cvp_hdl = hdl;
    /*先读取节点需要用的参数*/
    cvp_node_param_cfg_read(NULL, 0);
    return 0;
}

/*打开改节点输入接口*/
static void cvp_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = cvp_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int cvp_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct stream_oport *oport = iport->node->oport;
    int ret = NEGO_STA_ACCPTED;
    int nb_sr, wb_sr, nego_sr;

#if (TCFG_AUDIO_CVP_BAND_WIDTH_CFG == CVP_WB_EN)
    nb_sr = 16000;
    wb_sr = 16000;
    nego_sr  = 16000;
#elif (TCFG_AUDIO_CVP_BAND_WIDTH_CFG == CVP_NB_EN)
    nb_sr = 8000;
    wb_sr = 8000;
    nego_sr  = 8000;
#else
    nb_sr = 8000;
    wb_sr = 16000;
    nego_sr  = 16000;
#endif
    //要求输入为8K或者16K
    if (in_fmt->sample_rate != nb_sr && in_fmt->sample_rate != wb_sr) {
        in_fmt->sample_rate = nego_sr;
        oport->fmt.sample_rate = in_fmt->sample_rate;
        ret = NEGO_STA_CONTINUE | NEGO_STA_SAMPLE_RATE_LOCK;
    }

    //要求输入16bit位宽的数据
    if (in_fmt->bit_wide != DATA_BIT_WIDE_16BIT) {
        in_fmt->bit_wide = DATA_BIT_WIDE_16BIT;
        in_fmt->Qval = AUDIO_QVAL_16BIT;
        oport->fmt.bit_wide = in_fmt->bit_wide;
        oport->fmt.Qval = in_fmt->Qval;
        ret = NEGO_STA_CONTINUE;
    }

    //固定输出单声道数据
    oport->fmt.channel_mode = AUDIO_CH_MIX;
    return ret;
}

/*节点start函数*/
static void cvp_ioc_start(struct cvp_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    struct audio_aec_init_param_t init_param;
    init_param.sample_rate = fmt->sample_rate;
    init_param.ref_sr = hdl->ref_sr;
    init_param.ref_channel = hdl->ref_mic_num;
    u8 mic_num; //算法需要使用的MIC个数

#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
    audio_cvp_ref_src_open(hdl->scene, audio_iis_get_sample_rate(iis_hdl[0]), fmt->sample_rate, 2);
#endif

    audio_aec_init(&init_param);

    if (hdl->source_uuid == NODE_UUID_ADC) {
        if (hdl->ref_mic.en) {
            /*硬回采需要开2/3个MIC*/
            mic_num = 1 + hdl->ref_mic_num;
        } else {
            /*硬回采需要开1个MIC*/
            mic_num = 1;
        }
        if (audio_adc_file_get_esco_mic_num() != mic_num) {
#if TCFG_AUDIO_DUT_ENABLE
            //使能产测时，只有算法模式才需判断
            if (cvp_dut_mode_get() == CVP_DUT_MODE_ALGORITHM) {
                ASSERT(0, "CVP_SMS, ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), mic_num);
            }
#else
            ASSERT(0, "CVP_SMS, ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), mic_num);
#endif
        }
    }
}

/*节点stop函数*/
static void cvp_ioc_stop(struct cvp_node_hdl *hdl)
{
    if (hdl) {
        audio_aec_close();
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
        audio_cvp_ref_src_close();
#endif
    }
}

static int cvp_ioc_update_parm(struct cvp_node_hdl *hdl, int parm)
{
    int ret = false;
    struct cvp_cfg_t *cfg = (struct cvp_cfg_t *)parm;
    if (hdl) {
        cvp_node_param_cfg_update(cfg, &hdl->online_cfg);
        aec_cfg_update(&hdl->online_cfg);
        ret = true;
    }
    return ret;
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
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= cvp_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_FMT:
        hdl->ref_sr = (u32)arg;
        break;
    case NODE_IOC_SET_SCENE:
        hdl->scene = arg;
        break;
    case NODE_IOC_START:
        cvp_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        cvp_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = cvp_ioc_update_parm(hdl, arg);
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
    if (g_cvp_hdl->buf_1) {
        free(g_cvp_hdl->buf_1);
        g_cvp_hdl->buf_1 = NULL;
    }
    g_cvp_hdl = NULL;
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(cvp_node_adapter) = {
#if TCFG_AUDIO_CVP_SMS_ANS_MODE
    .name       = "cvp_sms_ans",
    .uuid       = NODE_UUID_CVP_SMS_ANS,
#else /*TCFG_AUDIO_CVP_SMS_DNS_MODE*/
    .name       = "cvp_sms_dns",
    .uuid       = NODE_UUID_CVP_SMS_DNS,
#endif
    .bind       = cvp_adapter_bind,
    .ioctl      = cvp_adapter_ioctl,
    .release    = cvp_adapter_release,
    .hdl_size   = sizeof(struct cvp_node_hdl),
    .ability_channel_convert = 1, //支持声道转换
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(cvp_sms) = {
#if TCFG_AUDIO_CVP_SMS_ANS_MODE
    .uuid       = NODE_UUID_CVP_SMS_ANS,
#else /*TCFG_AUDIO_CVP_SMS_DNS_MODE*/
    .uuid       = NODE_UUID_CVP_SMS_DNS,
#endif
};

#endif/*TCFG_AUDIO_CVP_SMS_ANS_MODE || TCFG_AUDIO_CVP_SMS_DNS_MODE*/
