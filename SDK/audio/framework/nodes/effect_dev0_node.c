#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/effects_adj.h"
#include "app_config.h"
#include "effect_dev_node.h"
#include "effects_dev.h"
#include "audio_splicing.h"

#if TCFG_EFFECT_DEV0_NODE_ENABLE

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECT_DEV0-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   code_seg(".effect_dev0.text")
#pragma   data_seg(".effect_dev0.data")
#pragma  const_seg(".effect_dev0.text.const")
#endif

/* 音效算法处理帧长
 * 0   : 等长输入输出，输入数据算法需要全部处理完
 * 非0 : 按照帧长输入数据到算法处理接口
 */
#define EFFECT_DEV0_FRAME_POINTS  0 //(256)


/*
 *声道转换类型选配
 *支持立体声转4声道协商使能,须在第三方音效节点后接入声道拆分节点
 * */
#define CHANNEL_ADAPTER_AUTO   0 //自动协商,通常用于无声道数转换的场景,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_2TO4   1 //立体声转4声道协商使能,支持2to4,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_1TO2   2 //单声道转立体声协商使能,支持1to2,结果随数据流配置自动适配
#define CHANNEL_ADAPTER_TYPE   CHANNEL_ADAPTER_AUTO //默认无声道转换

struct effect_dev0_node_hdl {
    char name[16];
    void *user_priv;  //用户可使用该变量做模块指针传递
    struct user_effect_tool_param cfg;//工具界面参数
    struct packet_ctrl dev;
};

/* 自定义算法，初始化
 * hdl->dev.sample_rate:采样率
 * hdl->dev.in_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.out_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.bit_width:位宽 0，16bit  1，32bit
 **/
static void audio_effect_dev0_init(struct effect_dev0_node_hdl *hdl)
{
    //do something
}

/* 自定义算法，运行
 * hdl->dev.sample_rate:采样率
 * hdl->dev.in_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.out_ch_num:通道数，单声道 1，立体声 2, 四声道 4
 * hdl->dev.bit_width:位宽 0，16bit  1，32bit
 * *indata:输入数据地址
 * *outdata:输出数据地址
 * indata_len :输入数据长度,byte
 * */
static void audio_effect_dev0_run(struct effect_dev0_node_hdl *hdl, s16 *indata, s16 *outdata, u32 indata_len)
{
#if 0
    //test 2to4
    if (hdl->dev.bit_width && ((hdl->dev.out_ch_num == 4) && (hdl->dev.in_ch_num == 2))) {
        pcm_dual_to_qual_with_slience_32bit(outdata, indata, indata_len, 0);
    }
#endif
    //do something
    /* printf("effect dev0 do something here\n"); */
}
/* 自定义算法，关闭
 **/
static void audio_effect_dev0_exit(struct effect_dev0_node_hdl *hdl)
{
    //do something

}

/* 自定义算法，更新参数
 **/
static void audio_effect_dev0_update(struct effect_dev0_node_hdl *hdl)
{
    //打印在线调音发送下来的参数
    printf("effect dev0 name : %s \n", hdl->name);
    for (int i = 0 ; i < 8; i++) {
        printf("cfg.int_param[%d] %d\n", i, hdl->cfg.int_param[i]);
    }
    for (int i = 0 ; i < 8; i++) {
        printf("cfg.float_param[%d] %d.%02d\n", i, (int)hdl->cfg.float_param[i], debug_digital(hdl->cfg.float_param[i]));
    }
    //do something

}


/*节点输出回调处理，可处理数据或post信号量*/
static void effect_dev0_handle_frame(struct stream_iport *iport, struct stream_note *note)
{

    struct effect_dev0_node_hdl *hdl = (struct effect_dev0_node_hdl *)iport->node->private_data;


    effect_dev_process(&hdl->dev, iport,  note); //音效处理
}

/*节点预处理-在ioctl之前*/
static int effect_dev0_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct effect_dev0_node_hdl *hdl = (struct effect_dev0_node_hdl *)node->private_data;
    memset(hdl, 0, sizeof(*hdl));
    return 0;
}

/*打开改节点输入接口*/
static void effect_dev0_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = effect_dev0_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int effect_dev0_ioc_negotiate(struct stream_iport *iport)
{
    int ret = 0;
    ret = NEGO_STA_ACCPTED;
    struct stream_oport *oport = iport->node->oport;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct effect_dev0_node_hdl *hdl = (struct effect_dev0_node_hdl *)iport->node->private_data;

    if (oport->fmt.channel_mode == 0xff) {
        return 0;
    }

    hdl->dev.out_ch_num = AUDIO_CH_NUM(oport->fmt.channel_mode);
    hdl->dev.in_ch_num = AUDIO_CH_NUM(in_fmt->channel_mode);
#if (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_2TO4)
    if (hdl->dev.out_ch_num == 4) {
        if (hdl->dev.in_ch_num != 2) {
            in_fmt->channel_mode = AUDIO_CH_LR;
            ret = NEGO_STA_CONTINUE;
        }
    }
#elif (CHANNEL_ADAPTER_TYPE == CHANNEL_ADAPTER_1TO2)
    if (hdl->dev.out_ch_num == 2) {
        if (hdl->dev.in_ch_num != 1) {
            in_fmt->channel_mode = AUDIO_CH_MIX;
            ret = NEGO_STA_CONTINUE;
        }
    }
#endif

    log_debug(" effecs_dev in_ch_num %d, out_ch_num %d\n", hdl->dev.in_ch_num, hdl->dev.out_ch_num);


    return ret;
}

/*节点start函数*/
static void effect_dev0_ioc_start(struct effect_dev0_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    /* struct jlstream *stream = jlstream_for_node(hdl_node(hdl)); */


    hdl->dev.sample_rate = fmt->sample_rate;


    /*
     *获取配置文件内的参数,及名字
     * */
    int len = jlstream_read_node_data_new(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (void *)&hdl->cfg, hdl->name);
    if (!len) {
        log_error("%s, read node data err\n", __FUNCTION__);
        return;
    }

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, &hdl->cfg, sizeof(hdl->cfg))) {
            log_debug("get effect dev0 online param\n");
        }
    }
    printf("effect dev0 name : %s \n", hdl->name);
    for (int i = 0 ; i < 8; i++) {
        printf("cfg.int_param[%d] %d\n", i, hdl->cfg.int_param[i]);
    }
    for (int i = 0 ; i < 8; i++) {
        printf("cfg.float_param[%d] %d.%02d\n", i, (int)hdl->cfg.float_param[i], debug_digital(hdl->cfg.float_param[i]));
    }

    hdl->dev.bit_width = hdl_node(hdl)->iport->prev->fmt.bit_wide;
    hdl->dev.qval = hdl_node(hdl)->iport->prev->fmt.Qval;
    printf("effect_dev0_ioc_start, sr: %d, in_ch: %d, out_ch: %d, bitw: %d, %d", hdl->dev.sample_rate, hdl->dev.in_ch_num, hdl->dev.out_ch_num, hdl->dev.bit_width, hdl->dev.qval);


    hdl->dev.node_hdl = hdl;
    hdl->dev.effect_run = (void (*)(void *, s16 *, s16 *, u32))audio_effect_dev0_run;
    effect_dev_init(&hdl->dev, EFFECT_DEV0_FRAME_POINTS);
    audio_effect_dev0_init(hdl);
}


/*节点stop函数*/
static void effect_dev0_ioc_stop(struct effect_dev0_node_hdl *hdl)
{
    audio_effect_dev0_exit(hdl);
    effect_dev_close(&hdl->dev);
}

static int effect_dev0_ioc_update_parm(struct effect_dev0_node_hdl *hdl, int parm)
{
    int ret = false;
    return ret;
}
static int get_effect_dev0_ioc_parm(struct effect_dev0_node_hdl *hdl, int parm)
{
    int ret = 0;
    return ret;
}

static int effect_ioc_update_parm(struct effect_dev0_node_hdl *hdl, int parm)
{
    int ret = false;
    struct user_effect_tool_param *cfg = (struct user_effect_tool_param *)parm;
    if (hdl) {
        memcpy(&hdl->cfg, cfg, sizeof(struct user_effect_tool_param));

        audio_effect_dev0_update(hdl);

        ret = true;
    }

    return ret;
}


/*节点ioctl函数*/
static int effect_dev0_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct effect_dev0_node_hdl *hdl = (struct effect_dev0_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        effect_dev0_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_SCENE:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= effect_dev0_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        effect_dev0_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        effect_dev0_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;

    case NODE_IOC_SET_PARAM:
        ret = effect_ioc_update_parm(hdl, arg);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void effect_dev0_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(effect_dev0_node_adapter) = {
    .name       = "effect_dev0",
    .uuid       = NODE_UUID_EFFECT_DEV0,
    .bind       = effect_dev0_adapter_bind,
    .ioctl      = effect_dev0_adapter_ioctl,
    .release    = effect_dev0_adapter_release,
    .hdl_size   = sizeof(struct effect_dev0_node_hdl),
#if (CHANNEL_ADAPTER_TYPE != CHANNEL_ADAPTER_AUTO)
    .ability_channel_out = 0x80 | 1 | 2 | 4,
    .ability_channel_convert = 1,
#endif

};

REGISTER_ONLINE_ADJUST_TARGET(effect_dev0) = {
    .uuid = NODE_UUID_EFFECT_DEV0,
};

#endif

