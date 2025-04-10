#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spatial_effects_node.data.bss")
#pragma data_seg(".spatial_effects_node.data")
#pragma const_seg(".spatial_effects_node.text.const")
#pragma code_seg(".spatial_effects_node.text")
#endif
#include "app_config.h"
#if (TCFG_AUDIO_SPATIAL_EFFECT_ENABLE)
#include "jlstream.h"
#include "media/audio_base.h"
#include "circular_buf.h"
#include "cvp_node.h"
#include "spatial_effects_process.h"
#include "spatial_effect.h"


struct ANGLE_CONIFG {
    float track_sensitivity;
    float angle_reset_sensitivity;
} __attribute__((packed));

struct RP_CONIFG {
    int trackKIND;    //角度合成算法用哪一种 :0或者1
    int ReverbKIND;   //2或者3
    int reverbance;   //湿声比例 : 0~100
    int dampingval;   //高频decay  ：0~80
} __attribute__((packed));

struct SPATIAL_CONIFG {
    float radius;//半径
    int bias_angle;//偏角
} __attribute__((packed));

struct SPATIAL_EFFECT_CONFIG {
    u8 bypass;//是否跳过节点处理
    struct ANGLE_CONIFG     angle;
    struct RP_CONIFG        v1_parm_reverb;
    struct SPATIAL_CONIFG   v2_parm_spatial;
} __attribute__((packed));

struct spatial_effects_node_hdl {
    char name[16];
    u8 out_channel;
    u8 tmp_out_channel;
    volatile u8 pend_frame;
    int sample_rate;
    spinlock_t lock;
    struct SPATIAL_EFFECT_CONFIG effect_cfg;
    spatial_effect_cfg_t online_cfg;
    u8 bypass;//是否跳过节点处理
    struct node_port_data_wide data_wide;
};
static struct spatial_effects_node_hdl *spatial_effects_hdl = NULL;

void spatial_effect_node_param_cfg_updata(struct SPATIAL_EFFECT_CONFIG *effect_cfg, spatial_effect_cfg_t *param)
{
    printf("spatial_effect_node_param_cfg_updata");
    struct spatial_effects_node_hdl *hdl = spatial_effects_hdl;

    if (!effect_cfg) {
        return;
    }

    if (param) {
        param->angle.track_sensitivity = effect_cfg->angle.track_sensitivity;
        param->angle.angle_reset_sensitivity = effect_cfg->angle.angle_reset_sensitivity;
        /*V2.0.0版本音效算法参数*/
        param->sag.radius = effect_cfg->v2_parm_spatial.radius;
        param->sag.bias_angle = effect_cfg->v2_parm_spatial.bias_angle;
        if (hdl) {
            param->scfi.sampleRate = hdl->sample_rate;
            param->pcm_info.IndataBit = hdl->data_wide.iport_data_wide;
            param->pcm_info.OutdataBit = hdl->data_wide.oport_data_wide;
        }
        /*V1.0.0版本音效算法参数*/
        param->rp_parm.trackKIND = effect_cfg->v1_parm_reverb.trackKIND;
        param->rp_parm.ReverbKIND = effect_cfg->v1_parm_reverb.ReverbKIND;
        param->rp_parm.reverbance = effect_cfg->v1_parm_reverb.reverbance;
        param->rp_parm.dampingval = effect_cfg->v1_parm_reverb.dampingval;
    }

    if (hdl) {
        hdl->bypass = effect_cfg->bypass;
        printf("spatial node bypass %d", hdl->bypass);
    }
}

int spatial_effects_node_param_cfg_read(void *param, int size)
{
    int len = 0;
    struct spatial_effects_node_hdl *hdl = spatial_effects_hdl;

    /*
     *获取配置文件内的参数,及名字
     * */
    len = jlstream_read_node_data_new(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (void *)(&hdl->effect_cfg), hdl->name);
    if (len != sizeof(hdl->effect_cfg)) {
        printf("%s, read node data err %d != %d\n", __FUNCTION__, len, (int)sizeof(hdl->effect_cfg));
        return 0 ;
    }
    printf("%s, read node cfg succ : %d \n", __FUNCTION__, len);

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        if (hdl) {
            if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, (void *)(&hdl->effect_cfg), len)) {
                printf("get spatial effects online param\n");
            }
        }
    }

    spatial_effect_node_param_cfg_updata(&hdl->effect_cfg, (spatial_effect_cfg_t *)param);

    return sizeof(spatial_effect_cfg_t);
}

u8 get_spatial_effects_node_out_channel(void)
{
    struct spatial_effects_node_hdl *hdl = spatial_effects_hdl;
    if (hdl) {
        /* printf("out _channel %x", hdl->out_channel); */
        return hdl->out_channel;
    }
    return 0;
}

u8 get_spatial_effect_node_bit_width()
{
    struct spatial_effects_node_hdl *hdl = spatial_effects_hdl;
    if (hdl) {
        return hdl->data_wide.iport_data_wide;
    }
    return 0;
}

u8 get_spatial_effect_node_bypass()
{
    struct spatial_effects_node_hdl *hdl = spatial_effects_hdl;
    if (hdl) {
        return hdl->bypass;
    }
    return 0;
}

u8 spatial_effect_node_is_running()
{
    return (spatial_effects_hdl ? 1 : 0);
}

/*节点输出回调处理，可处理数据或post信号量*/
static void spatial_effects_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_node *node = iport->node;
    struct spatial_effects_node_hdl *hdl = (struct spatial_effects_node_hdl *)iport->node->private_data;
    s16 *dat;
    int len;
    struct stream_frame *in_frame;
    if (hdl == NULL) {
        return;
    }
    in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
    if (!in_frame) {
        return;
    }
    dat = (s16 *)in_frame->data;
    len = in_frame->len;
#if SPATIAL_AUDIO_EFFECT_OUT_STEREO_EN
    /*节点固定输出立体声*/
    in_frame->len = audio_spatial_effects_data_handler(AUDIO_CH_LR, dat, len);
#else
    in_frame->len = audio_spatial_effects_data_handler(hdl->out_channel, dat, len);
#endif
    jlstream_push_frame(node->oport, in_frame);
}

static int spatial_effects_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct spatial_effects_node_hdl *hdl = (struct spatial_effects_node_hdl *)node->private_data;

    spin_lock_init(&hdl->lock);
    spatial_effects_hdl = hdl;
    return 0;
}

/*打开改节点输入接口*/
static void spatial_effects_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = spatial_effects_handle_frame;
}

/*节点参数协商*/
static int spatial_effects_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_oport *oport = iport->node->oport;
    /* struct stream_fmt *in_fmt = &iport->prev->fmt; */
    struct spatial_effects_node_hdl *hdl = (struct spatial_effects_node_hdl *)iport->node->private_data;

    if (hdl) {
        /*如果已经设置过通道，则用记录的输出通道*/
        if (hdl->tmp_out_channel) {
            hdl->out_channel = hdl->tmp_out_channel;
        } else {
            /*如果没有设置过输出通道，根据下一个节点设置*/
            hdl->out_channel = oport->fmt.channel_mode;
        }
        hdl->sample_rate = oport->fmt.sample_rate;
        printf("%s, out _channel %x", __func__, hdl->out_channel);
    }

    return 0;
}

/*节点start函数*/
static void spatial_effects_ioc_start(struct spatial_effects_node_hdl *hdl)
{
    if (hdl) {
        printf("spatial_effects_ioc_start");
        /*读取节点位宽信息*/
        hdl->data_wide.iport_data_wide = hdl_node(hdl)->iport->prev->fmt.bit_wide;
        hdl->data_wide.oport_data_wide = hdl_node(hdl)->oport->fmt.bit_wide;
        /* jlstream_read_node_port_data_wide(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (u8 *)&hdl->data_wide);  */
        /*读取bypass参数*/
        spatial_effects_node_param_cfg_read(NULL, 0);
        printf("%s bit_wide, %d %d\n", __FUNCTION__, hdl->data_wide.iport_data_wide, hdl->data_wide.oport_data_wide);

        audio_effect_process_start();
    }
}

/*节点stop函数*/
static void spatial_effects_ioc_stop(struct spatial_effects_node_hdl *hdl)
{
    if (hdl) {
        printf("spatial_effects_ioc_stop");

        audio_effect_process_stop();
    }
}

static int spatial_effects_ioc_update_parm(struct spatial_effects_node_hdl *hdl, int parm)
{
    int ret = false;
    if (hdl) {
        spatial_effect_node_param_cfg_updata((struct SPATIAL_EFFECT_CONFIG *)parm, &hdl->online_cfg);
        spatial_effect_online_updata((void *)(&hdl->online_cfg));
        ret = true;
    }
    return ret;
}

/*节点ioctl函数*/
static int spatial_effects_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct spatial_effects_node_hdl *hdl = (struct spatial_effects_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        spatial_effects_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= spatial_effects_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_FMT:
        break;
    case NODE_IOC_START:
        spatial_effects_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        spatial_effects_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = spatial_effects_ioc_update_parm(hdl, arg);
        break;
    case NODE_IOC_SET_CHANNEL:
        if (hdl) {
            /*记录输出通道*/
            hdl->tmp_out_channel = (u8)arg;
            /*设置输出通道*/
            hdl->out_channel = hdl->tmp_out_channel;
            printf("spatial effect set out_channel : 0x%x", hdl->out_channel);
        }
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void spatial_effects_adapter_release(struct stream_node *node)
{
    spatial_effects_hdl = NULL;
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(spatial_effects_node_adapter) = {
    .name       = "spatial_effects",
    .uuid       = NODE_UUID_SPATIAL_EFFECTS,
    .bind       = spatial_effects_adapter_bind,
    .ioctl      = spatial_effects_adapter_ioctl,
    .release    = spatial_effects_adapter_release,
    .hdl_size   = sizeof(struct spatial_effects_node_hdl),
    .ability_channel_in = 2, //输入只支持双声道
    .ability_channel_out =  1 | 2, //输出单声道或者双声道
    .ability_channel_convert = 1, //支持声道转换
};


//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(spatial_effects) = {
    .uuid       = NODE_UUID_SPATIAL_EFFECTS,
};
#endif
