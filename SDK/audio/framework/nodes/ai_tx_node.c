#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ai_tx_node.data.bss")
#pragma data_seg(".ai_tx_node.data")
#pragma const_seg(".ai_tx_node.text.const")
#pragma code_seg(".ai_tx_node.text")
#endif

#include "jlstream.h"
/* #include "classic/hci_lmp.h" */
#include "media/audio_base.h"
#include "app_config.h"

#if TCFG_AI_TX_NODE_ENABLE

struct ai_tx_hdl {
    u8 start;
    struct stream_fmt fmt;
};


extern int rec_enc_output(void *priv, void *buf, int len);
static void ai_tx_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
#if (BT_MIC_EN)
        rec_enc_output(NULL, frame->data, frame->len);
#endif
        jlstream_free_frame(frame);
    }
}

static int ai_tx_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

static void ai_tx_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = ai_tx_handle_frame;
}

static int ai_tx_ioc_fmt_nego(struct ai_tx_hdl *hdl, struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;

    in_fmt->coding_type = hdl->fmt.coding_type;
    in_fmt->sample_rate = 16000;
    in_fmt->channel_mode = AUDIO_CH_MIX;

    return NEGO_STA_ACCPTED;
}

static int ai_tx_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct ai_tx_hdl *hdl = (struct ai_tx_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        ai_tx_open_iport(iport);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_SET_FMT:
        struct stream_fmt *fmt = (struct stream_fmt *)arg;
        hdl->fmt.coding_type = fmt->coding_type;
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= ai_tx_ioc_fmt_nego(hdl, iport);
        break;
    case NODE_IOC_GET_DELAY:
        break;
        /* return lmp_private_get_ai_tx_packet_num() * 75; */
    }

    return 0;
}

static void ai_tx_release(struct stream_node *node)
{

}


REGISTER_STREAM_NODE_ADAPTER(ai_tx_adapter) = {
    .name       = "ai_tx",
    .uuid       = NODE_UUID_AI_TX,
    .bind       = ai_tx_bind,
    .ioctl      = ai_tx_ioctl,
    .release    = ai_tx_release,
    .hdl_size   = sizeof(struct ai_tx_hdl),
};

#endif

