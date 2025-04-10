#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dns_node.data.bss")
#pragma data_seg(".dns_node.data")
#pragma const_seg(".dns_node.text.const")
#pragma code_seg(".dns_node.text")
#endif

#include "jlstream.h"
#include "audio_config.h"
#include "media/audio_base.h"
#include "audio_ns.h"
#include "btstack/avctp_user.h"
#include "effects/effects_adj.h"
#include "frame_length_adaptive.h"
#include "app_config.h"
#include "overlay_code.h"

#if 1
#define ns_log	printf
#else
#define ns_log(...)
#endif/*log_en*/

#if TCFG_DNS_NODE_ENABLE

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

enum {
    AUDIO_NS_TYPE_ESCO_DL = 1,	//下行降噪类型
    AUDIO_NS_TYPE_GENERAL,		//通用类型
};

struct ns_cfg_t {
    u8 bypass;
    u8 ns_type;//降噪类型选择，通用降噪/通话下行降噪
    u8 call_active_trigger;//接通电话触发标志, 只有通话下行降噪会使用
    float aggressfactor;   //降噪强度(越大越强:1~2)
    float minsuppress;     //降噪最小压制(越小越强:0~1)
    float noiselevel;      //初始噪声水平(评估初始噪声，加快收敛)
} __attribute__((packed));

struct dns_node_hdl {
    char name[16];
    u8 bt_addr[6];
    u8 trigger;
    u32 sample_rate;
    void *dns;
    struct stream_frame *out_frame;
    struct ns_cfg_t cfg;
    struct fixed_frame_len_handle *fixed_hdl;
};

extern int db2mag(int db, int dbQ, int magDQ);//10^db/20
int dns_param_cfg_read(struct stream_node *node)
{
    struct ns_cfg_t config;
    struct dns_node_hdl *hdl = (struct dns_node_hdl *)node->private_data;
    int ret = 0;
    if (!hdl) {
        return -1 ;
    }
    /*
     *获取配置文件内的参数,及名字
     * */
    ret = jlstream_read_node_data_new(NODE_UUID_DNS_NOISE_SUPPRESSOR, node->subid, (void *)&config, hdl->name);
    if (ret != sizeof(config)) {
        printf("%s, read node data err %d = %d\n", __FUNCTION__, ret, (int)sizeof(config));
        return -1 ;
    }

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        ret = jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, &config, sizeof(config));
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, &config, sizeof(config))) {
            printf("get dns online param succ\n");
        }
    }

    hdl->cfg = config;

    ns_log("bypass %d\n", hdl->cfg.bypass);
    ns_log("type %d\n", hdl->cfg.ns_type);
    ns_log("call_active_trigger %d\n", hdl->cfg.call_active_trigger);
    ns_log("aggressfactor  %d/1000\n", (int)(hdl->cfg.aggressfactor * 1000.f));
    ns_log("minsuppress    %d/1000\n", (int)(hdl->cfg.minsuppress * 1000.f));
    ns_log("noiselevel     %d/1000\n", (int)(hdl->cfg.noiselevel * 1000.f));

    return ret;
}

static int ns_node_fixed_frame_run(void *priv, u8 *in, u8 *out, int len)
{
    int wlen = 0;
    struct dns_node_hdl *hdl = (struct dns_node_hdl *)priv;
    if (hdl->cfg.bypass) {
        if (hdl->dns) {
            audio_dns_close(hdl->dns);
            hdl->dns = NULL;
        }
        memcpy(out, in, len);
        return len;
    } else {
        if (!hdl->dns) {
            dns_param_t param = {
                .DNS_OverDrive = hdl->cfg.aggressfactor,
                .DNS_GainFloor = hdl->cfg.minsuppress,
                .DNS_NoiseLevel = hdl->cfg.noiselevel,
                .DNS_highGain = 2.5f,
                .DNS_rbRate = 0.3f,
                .sample_rate = hdl->sample_rate,
            };
            hdl->dns = audio_dns_open(&param);
        }
        if (hdl->dns) {
            wlen = audio_dns_run(hdl->dns, (s16 *)in, (s16 *)out, len);
        }
    }
    return wlen;
}
/*节点输出回调处理，可处理数据或post信号量*/
static void dns_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct dns_node_hdl *hdl = (struct dns_node_hdl *)iport->node->private_data;
    struct stream_node *node = iport->node;
    struct stream_frame *in_frame;
    int wlen;
    int out_frame_len;

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!in_frame) {
            break;
        }

        if (hdl->cfg.ns_type == AUDIO_NS_TYPE_ESCO_DL) {
            if (hdl->cfg.call_active_trigger && (!hdl->trigger)) {
                if (bt_get_call_status_for_addr(hdl->bt_addr) == BT_CALL_ACTIVE) {
                    hdl->trigger = 1;
                }
            }
        } else {
            hdl->trigger = 1;
        }

        /*设置工具配置的降噪效果*/
        float DNS_GainFloor = hdl->cfg.minsuppress;
        float DNS_OverDrive = hdl->cfg.aggressfactor;
        if (!hdl->trigger) {
            /*没有接通，降降噪效果设置成0*/
            DNS_GainFloor = 1.0f;
        }
        if (hdl->dns) {
            audio_dns_updata_param(hdl->dns, DNS_GainFloor, DNS_OverDrive);
        }

        out_frame_len = get_fixed_frame_len_output_len(hdl->fixed_hdl, in_frame->len);
        if (out_frame_len) {
            hdl->out_frame = jlstream_get_frame(node->oport, out_frame_len);
            if (!hdl->out_frame) {
                return;
            }
        }
        wlen = audio_fixed_frame_len_run(hdl->fixed_hdl, in_frame->data, hdl->out_frame->data, in_frame->len);

        if (wlen && hdl->out_frame) {
            hdl->out_frame->len = wlen;
            jlstream_push_frame(node->oport, hdl->out_frame);	//将数据推到oport
            hdl->out_frame = NULL;
        }
        jlstream_free_frame(in_frame);	//释放iport资源
    }
}

/*节点预处理-在ioctl之前*/
static int dns_adapter_bind(struct stream_node *node, u16 uuid)
{
    dns_param_cfg_read(node);

    return 0;
}

/*打开改节点输入接口*/
static void dns_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = dns_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int dns_ioc_negotiate(struct stream_iport *iport)
{
    struct dns_node_hdl *hdl = (struct dns_node_hdl *)iport->node->private_data;
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
    hdl->sample_rate = in_fmt->sample_rate;
    return ret;
}


/*节点start函数*/
static void dns_ioc_start(struct dns_node_hdl *hdl)
{
    /* struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt; */
    printf("dns node start");

    dns_param_t param = {
        .DNS_OverDrive = hdl->cfg.aggressfactor,
        .DNS_GainFloor = hdl->cfg.minsuppress,
        .DNS_NoiseLevel = hdl->cfg.noiselevel,
        .DNS_highGain = 2.5f,
        .DNS_rbRate = 0.3f,
        .sample_rate = hdl->sample_rate,
    };
    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();
    /*打开算法*/
    hdl->dns = audio_dns_open(&param);
    hdl->trigger = 0;
    hdl->fixed_hdl = audio_fixed_frame_len_init(DNS_FRAME_SIZE, ns_node_fixed_frame_run, hdl);
}

/*节点stop函数*/
static void dns_ioc_stop(struct dns_node_hdl *hdl)
{
    if (hdl->dns) {
        audio_dns_close(hdl->dns);
        hdl->dns = NULL;
    }
    aec_code_movable_unload();
    if (hdl->fixed_hdl) {
        audio_fixed_frame_len_exit(hdl->fixed_hdl);
        hdl->fixed_hdl = NULL;
    }
    if (hdl->out_frame) {
        jlstream_free_frame(hdl->out_frame);
        hdl->out_frame = NULL;
    }
    hdl->trigger = 0;
}

static int dns_ioc_update_parm(struct dns_node_hdl *hdl, int parm)
{
    if (hdl == NULL) {
        return false;
    }
    memcpy(&hdl->cfg, (u8 *)parm, sizeof(hdl->cfg));
    if (hdl->dns) {
        /*设置工具配置的降噪效果*/
        float DNS_GainFloor = hdl->cfg.minsuppress;
        float DNS_OverDrive = hdl->cfg.aggressfactor;
        audio_dns_updata_param(hdl->dns, DNS_GainFloor, DNS_OverDrive);
    }
    return true;
}

/*节点ioctl函数*/
static int dns_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct dns_node_hdl *hdl = (struct dns_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        u8 *bt_addr = (u8 *)arg;
        memcpy(hdl->bt_addr, bt_addr, 6);
        break;
    case NODE_IOC_OPEN_IPORT:
        dns_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= dns_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        dns_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        dns_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;

    case NODE_IOC_SET_PARAM:
        ret = dns_ioc_update_parm(hdl, arg);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void dns_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(dns_node_adapter) = {
    .name       = "dns",
    .uuid       = NODE_UUID_DNS_NOISE_SUPPRESSOR,
    .bind       = dns_adapter_bind,
    .ioctl      = dns_adapter_ioctl,
    .release    = dns_adapter_release,
    .hdl_size   = sizeof(struct dns_node_hdl),
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(dns_noise_suppressor) = {
    .uuid = NODE_UUID_DNS_NOISE_SUPPRESSOR,
};

#endif/* TCFG_DNS_NODE_ENABLE*/

