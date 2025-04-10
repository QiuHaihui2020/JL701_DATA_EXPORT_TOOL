#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/effects_adj.h"
#include "app_config.h"
#include "aec_uart_debug.h"
#include "uartPcmSender.h"

#if TCFG_DATA_EXPORT_NODE_ENABLE

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[DATA_EXPORT-NODE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

struct data_export_cfg_t {
    u16 data_len;
} __attribute__((packed));

struct data_export_node_hdl {
    char name[16];
    void *effect_dev1;
    u32 sample_rate;
    u8 ch_num;
    u8 ch_idx;
    struct data_export_cfg_t node_cfg;
};

/*节点配置*/
struct data_export_global_info {
    u8 node_cnt;    //记录当前是第几个data export节点
    u8 node_num;    //记录数据流里面，date export节点的个数
    int total_len;  //记录通道数据总和
};

/*全局变量*/
struct data_export_global_info g_export_info = {
    .node_cnt = 0,
    .node_num = 0,
    .total_len = 0,
};

/*节点输出回调处理，可处理数据或post信号量*/
static void data_export_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;
    struct data_export_node_hdl *hdl = (struct data_export_node_hdl *)iport->node->private_data;
    struct stream_node *node = iport->node;

    while (1) {
        frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!frame) {
            break;
        }

        aec_uart_fill_v2(hdl->ch_idx, (s16 *)frame->data, frame->len);
        aec_uart_write_v2();

        if (node->oport) {
            jlstream_push_frame(node->oport, frame);	//将数据推到oport
        } else {
            jlstream_free_frame(frame);	//释放iport资源
        }
    }
}

static int data_export_param_cfg_read(struct data_export_node_hdl *hdl)
{
    /*
     *解析配置文件内效果配置
     * */
    int len = 0;
    struct node_param ncfg = {0};
    len = jlstream_read_node_data(NODE_UUID_DATA_EXPORT, hdl_node(hdl)->subid, (u8 *)&ncfg);

    if (len != sizeof(ncfg)) {
        printf("data_export_node read ncfg err\n");
        return -2;
    }

    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &hdl->node_cfg);
    }

    printf(" %s len %d, sizeof(struct data_export_cfg_t) %d\n", __func__,  len, (int)sizeof(struct data_export_cfg_t));
    if (len != sizeof(struct data_export_cfg_t)) {
        printf("data_export_param read ncfg err\n");
        return -1 ;
    }
    return len ;
}

/*节点预处理-在ioctl之前*/
static int data_export_adapter_bind(struct stream_node *node, u16 uuid)
{
    /*计算数据流data export节点的个数*/
    g_export_info.node_num++;
    return 0;
}

/*打开改节点输入接口*/
static void data_export_ioc_open_iport(struct stream_iport *iport)
{
    printf("data_export_ioc_open_iport");
    iport->handle_frame = data_export_handle_frame;				//注册输出回调
}


/*节点start函数*/
static void data_export_ioc_start(struct data_export_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    /* struct jlstream *stream = jlstream_for_node(hdl_node(hdl)); */

    printf("data_export_ioc_start");
    /*读取节点参数*/
    data_export_param_cfg_read(hdl);

    /*计算总的通道数据大小*/
    g_export_info.total_len += hdl->node_cfg.data_len;

    hdl->ch_idx = g_export_info.node_cnt;
    printf("ch_idx : %d", hdl->ch_idx);

    g_export_info.node_cnt++;

    hdl->sample_rate = fmt->sample_rate;
    hdl->ch_num = AUDIO_CH_NUM(fmt->channel_mode);

    if (g_export_info.node_cnt >= g_export_info.node_num) {
        uartSendInit();
        aec_uart_open_v2(g_export_info.node_num, g_export_info.total_len);
    }
}


/*节点stop函数*/
static void data_export_ioc_stop(struct data_export_node_hdl *hdl)
{
    printf("data_export_ioc_stop");
    g_export_info.node_num--;
    g_export_info.node_cnt--;
    if (g_export_info.node_num == 0) {
        g_export_info.total_len = 0;
        aec_uart_close_v2();
    }
}

static int data_export_ioc_update_parm(struct data_export_node_hdl *hdl, int parm)
{
    int ret = false;
    return ret;
}
static int get_data_export_ioc_parm(struct data_export_node_hdl *hdl, int parm)
{
    int ret = 0;
    return ret;
}

/*节点ioctl函数*/
static int data_export_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct data_export_node_hdl *hdl = (struct data_export_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        data_export_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_SCENE:
        break;
    case NODE_IOC_START:
        data_export_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        data_export_ioc_stop(hdl);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void data_export_adapter_release(struct stream_node *node)
{
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(data_export_node_adapter) = {
    .name       = "data_export",
    .uuid       = NODE_UUID_DATA_EXPORT,
    .bind       = data_export_adapter_bind,
    .ioctl      = data_export_adapter_ioctl,
    .release    = data_export_adapter_release,
    .hdl_size   = sizeof(struct data_export_node_hdl),
};

/* REGISTER_ONLINE_ADJUST_TARGET(data_export) = { */
/* .uuid = NODE_UUID_DATA_EXPORT, */
/* }; */

#endif

