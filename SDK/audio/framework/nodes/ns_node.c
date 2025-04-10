#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ns_node.data.bss")
#pragma data_seg(".ns_node.data")
#pragma const_seg(".ns_node.text.const")
#pragma code_seg(".ns_node.text")
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

#if TCFG_NS_NODE_ENABLE

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

enum {
    AUDIO_NS_TYPE_ESCO_DL = 1,	//下行降噪类型
    AUDIO_NS_TYPE_GENERAL,		//通用类型
};

struct ns_cfg_t {
    u8 bypass;//是否跳过节点处理
    u8 ns_type;//降噪类型选择，通用降噪/通话下行降噪
    u8 call_active_trigger;//接通电话触发标志, 只有通话下行降噪会使用
    u8 mode;               //ans降噪模式(0,1,2:越大越耗资源，效果越好),dns降噪时，用于选择dns算法
    float aggressfactor;   //降噪强度(越大越强:1~2)
    float minsuppress;     //降噪最小压制(越小越强:0~1)
    float noiselevel;      //初始噪声水平(评估初始噪声，加快收敛)
} __attribute__((packed));

struct ns_node_hdl {
    char name[16];
    u8 bt_addr[6];
    u8 trigger;
    u32 sample_rate;
    void *ns;
    struct stream_frame *out_frame;
    struct ns_cfg_t cfg;
    struct fixed_frame_len_handle *fixed_hdl;
};

extern int db2mag(int db, int dbQ, int magDQ);//10^db/20
int ns_param_cfg_read(struct stream_node *node)
{
    struct ns_cfg_t config;
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)node->private_data;
    int ret = 0;
    if (!hdl) {
        return -1 ;
    }
    /*
     *获取配置文件内的参数,及名字
     * */
    ret = jlstream_read_node_data_new(NODE_UUID_NOISE_SUPPRESSOR, node->subid, (void *)&config, hdl->name);
    if (ret != sizeof(config)) {
        printf("%s, read node data err %d = %d\n", __FUNCTION__, ret, (int)sizeof(config));
        return -1 ;
    }

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, &config, sizeof(config))) {
            printf("get ans online param succ\n");
        }
    }

    hdl->cfg = config;

    ns_log("bypass %d\n", hdl->cfg.bypass);
    ns_log("type %d\n", hdl->cfg.ns_type);
    ns_log("call_active_trigger %d\n", hdl->cfg.call_active_trigger);
    ns_log("mode                %d\n", hdl->cfg.mode);
    ns_log("aggressfactor  %d/1000\n", (int)(hdl->cfg.aggressfactor * 1000.f));
    ns_log("minsuppress    %d/1000\n", (int)(hdl->cfg.minsuppress * 1000.f));
    ns_log("noiselevel     %d/1000\n", (int)(hdl->cfg.noiselevel * 1000.f));

    return ret;
}

static int ns_node_fixed_frame_run(void *priv, u8 *in, u8 *out, int len)
{
    int wlen = 0;
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)priv;
    if (hdl->cfg.bypass) {
        if (hdl->ns) {
            audio_ns_close(hdl->ns);
            hdl->ns = NULL;
        }
        memcpy(out, in, len);
        return len;
    } else {
        if (!hdl->ns) {
            hdl->ns = audio_ns_open(hdl->sample_rate, hdl->cfg.mode, hdl->cfg.noiselevel, hdl->cfg.aggressfactor, hdl->cfg.minsuppress);
        }
        if (hdl->ns) {
            wlen = audio_ns_run(hdl->ns, (s16 *)in, (s16 *)out, len);
        }
    }
    return wlen;
}

/*节点输出回调处理，可处理数据或post信号量*/
static void ns_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)iport->node->private_data;
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
        float minsuppress = hdl->cfg.minsuppress;
        if (!hdl->trigger) {
            /*没有接通，降降噪效果设置成0*/
            minsuppress = 1.0f;
        }
        if (hdl->ns) {
            noise_suppress_config(NS_CMD_MINSUPPRESS, 0, &minsuppress);
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
static int ns_adapter_bind(struct stream_node *node, u16 uuid)
{
    ns_param_cfg_read(node);

    return 0;
}

/*打开改节点输入接口*/
static void ns_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = ns_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int ans_ioc_negotiate(struct stream_iport *iport)
{
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)iport->node->private_data;
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
static void ns_ioc_start(struct ns_node_hdl *hdl)
{
    /* struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt; */

    printf("ans node start");
    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();
    /*打开算法*/
    if (hdl->cfg.bypass) {
        hdl->ns = audio_ns_open(hdl->sample_rate, hdl->cfg.mode, hdl->cfg.noiselevel, hdl->cfg.aggressfactor, hdl->cfg.minsuppress);
    }

    hdl->trigger = 0;
    hdl->fixed_hdl = audio_fixed_frame_len_init(ANS_FRAME_SIZE, ns_node_fixed_frame_run, hdl);
}

/*节点stop函数*/
static void ns_ioc_stop(struct ns_node_hdl *hdl)
{
    if (hdl->ns) {
        audio_ns_close(hdl->ns);
        hdl->ns = NULL;
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

static int ans_ioc_update_parm(struct ns_node_hdl *hdl, int parm)
{
    if (hdl == NULL) {
        return false;
    }
    memcpy(&hdl->cfg, (u8 *)parm, sizeof(hdl->cfg));
    if (hdl->ns) {
        /*设置工具配置的降噪效果*/
        float aggressfactor = hdl->cfg.aggressfactor;
        noise_suppress_config(NS_CMD_AGGRESSFACTOR, 0, &aggressfactor);

        float minsuppress = hdl->cfg.minsuppress;
        noise_suppress_config(NS_CMD_MINSUPPRESS, 0, &minsuppress);
    }
    return true;
}

/*节点ioctl函数*/
static int ns_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct ns_node_hdl *hdl = (struct ns_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        u8 *bt_addr = (u8 *)arg;
        memcpy(hdl->bt_addr, bt_addr, 6);
        break;
    case NODE_IOC_OPEN_IPORT:
        ns_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= ans_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        ns_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        ns_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;

    case NODE_IOC_SET_PARAM:
        ret = ans_ioc_update_parm(hdl, arg);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void ns_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(ns_node_adapter) = {
    .name       = "ns",
    .uuid       = NODE_UUID_NOISE_SUPPRESSOR,
    .bind       = ns_adapter_bind,
    .ioctl      = ns_adapter_ioctl,
    .release    = ns_adapter_release,
    .hdl_size   = sizeof(struct ns_node_hdl),
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(noise_suppressor) = {
    .uuid = NODE_UUID_NOISE_SUPPRESSOR,
};

#endif/* TCFG_NS_NODE_ENABLE*/

