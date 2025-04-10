#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".surround_demo_node.data.bss")
#pragma data_seg(".surround_demo_node.data")
#pragma const_seg(".surround_demo_node.text.const")
#pragma code_seg(".surround_demo_node.text")
#endif
#include "app_config.h"
#include "jlstream.h"
#include "overlay_code.h"

#if TCFG_SURROUND_DEMO_NODE_ENABLE

struct surround_demo_node_hdl {
    u8 bypass;
    u8 out_channel;
};

int surround_demo_data_run(void *priv, s16 *data, int len)
{
    int wlen = len;
    /* struct surround_demo_node_hdl *hdl = (struct surround_demo_node_hdl *)priv; */

    return wlen;
}

static void surround_demo_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *in_frame;
    struct stream_frame *out_frame;
    struct stream_node *node = iport->node;
    struct surround_demo_node_hdl *hdl = (struct surround_demo_node_hdl *)iport->node->private_data;

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);
        if (!in_frame) {
            break;
        }
        if (hdl->bypass) {
            /*bypas时，输入输出通道一样，数据直接往下推*/
            if (node->oport) {
                jlstream_push_frame(node->oport, in_frame);
            } else {
                jlstream_free_frame(in_frame);
            }
        } else {
            /*输入数据是双声道*/
            out_frame = jlstream_get_frame(node->oport, in_frame->len);
            memcpy(out_frame->data, in_frame->data, in_frame->len);
            out_frame->len = in_frame->len;

            /*音效处理*/
            surround_demo_data_run(hdl, (s16 *)out_frame->data, out_frame->len);

            /*输入数据是双声道，根据输出声道做声道转换*/
            int pcm_frames = (out_frame->len >> 2);//点数
            s16 *pcm_buf = (s16 *)out_frame->data;
            int i = 0, tmp = 0;
            switch (hdl->out_channel) {
            case AUDIO_CH_L:
                for (i = 0; i < pcm_frames; i++) {
                    pcm_buf[i] = pcm_buf[i * 2];
                }
                out_frame->len /= 2;
                break;
            case AUDIO_CH_R:
                for (i = 0; i < pcm_frames; i++) {
                    pcm_buf[i] = pcm_buf[i * 2 + 1];
                }
                out_frame->len /= 2;
                break;
            case AUDIO_CH_MIX:
                for (i = 0; i < pcm_frames; i++) {
                    tmp = pcm_buf[i * 2] + pcm_buf[i * 2 + 1];
                    pcm_buf[i] = tmp / 2;
                }
                out_frame->len /= 2;
                break;
            default:
                break;
            }
            jlstream_push_frame(node->oport, out_frame);
            jlstream_free_frame(in_frame);

        }
    }
}

/*节点预处理-在ioctl之前*/
static int surround_demo_adapter_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

/*打开改节点输入接口*/
static void surround_demo_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = surround_demo_handle_frame;
}

/*节点参数协商*/
static int surround_demo_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_oport *oport = iport->node->oport;
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct surround_demo_node_hdl *hdl = (struct surround_demo_node_hdl *)iport->node->private_data;
    int ret = NEGO_STA_ACCPTED;

    /*数据通道协商*/
    if (!hdl->bypass) {
        /*固定要求输入为双声道*/
        if (in_fmt->channel_mode != AUDIO_CH_LR) {
            in_fmt->channel_mode = AUDIO_CH_LR;
            ret = NEGO_STA_CONTINUE;
        }
    }

    /*记录输出通道数*/
    hdl->out_channel = oport->fmt.channel_mode;

    return ret;
}

/*节点start函数*/
static void surround_demo_ioc_start(void)
{

}

/*节点stop函数*/
static void surround_demo_ioc_stop(void)
{

}

/*节点ioctl函数*/
static int surround_demo_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct surround_demo_node_hdl *hdl = (struct surround_demo_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        surround_demo_ioc_open_iport(iport);
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= surround_demo_ioc_negotiate(iport);
        break;
    case NODE_IOC_START:
        surround_demo_ioc_start();
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        surround_demo_ioc_stop();
        break;
    }
    return ret;
}

/*节点用完释放函数*/
static void surround_demo_adapter_release(struct stream_node *node)
{
}

REGISTER_STREAM_NODE_ADAPTER(surround_demo_node_adapter) = {
    .name       = "surround_demo",
    .uuid       = NODE_UUID_SURROUND_DEMO,
    .bind       = surround_demo_adapter_bind,
    .ioctl      = surround_demo_adapter_ioctl,
    .release    = surround_demo_adapter_release,
    .hdl_size   = sizeof(struct surround_demo_node_hdl),
};

#endif

