#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_tx_node.data.bss")
#pragma data_seg(".esco_tx_node.data")
#pragma const_seg(".esco_tx_node.text.const")
#pragma code_seg(".esco_tx_node.text")
#endif
#include "jlstream.h"
#include "classic/hci_lmp.h"
#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "app_config.h"

struct esco_tx_sync_node {
    u8 trigger;
    void *syncts;
    struct list_head entry;
};

struct esco_tx_hdl {
    u8 start;
    u8 reference_network;
    u8 tx_frames;
    u8 bt_addr[6];
    struct list_head sync_list;
};

extern void bt_edr_conn_system_clock_init(void *addr, u8 factor);
extern u32 bt_edr_conn_master_to_local_time(void *addr, u32 usec);

static void esco_tx_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct stream_frame *frame;

    while (1) {
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        /*printf("esco_tx: %d\n", frame->len);*/
        lmp_private_send_esco_packet(NULL, frame->data, frame->len);

        jlstream_free_frame(frame);
    }
}

void esco_tx_tick_handler(void *priv, u32 clkn)
{
    struct esco_tx_hdl *hdl = (struct esco_tx_hdl *)priv;
    struct esco_tx_sync_node *node = NULL;

    u32 time = bt_edr_conn_master_to_local_time(hdl->bt_addr, (clkn * 625));
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (!node->trigger) {
            node->trigger = 1;
            sound_pcm_syncts_latch_trigger(node->syncts);
        }
        sound_pcm_update_frame_num(node->syncts, hdl->tx_frames);
        sound_pcm_update_frame_num_and_time(node->syncts, 0, time, 0);
    }

}

static int esco_tx_bind(struct stream_node *node, u16 uuid)
{
    return 0;
}

static void esco_tx_open_iport(struct stream_iport *iport)
{
    struct esco_tx_hdl *hdl = (struct esco_tx_hdl *)iport->node->private_data;

    INIT_LIST_HEAD(&hdl->sync_list);
    hdl->reference_network = 0xff;
    iport->handle_frame = esco_tx_handle_frame;
}

static void esco_tx_set_bt_addr(struct esco_tx_hdl *hdl, void *bt_addr)
{
    memcpy(hdl->bt_addr, bt_addr, 6);


    bt_edr_conn_system_clock_init(hdl->bt_addr, 1);
}

static int esco_tx_ioc_fmt_nego(struct stream_iport *iport)
{
    struct esco_tx_hdl *hdl = (struct esco_tx_hdl *)iport->node->private_data;
    struct stream_fmt *in_fmt = &iport->prev->fmt;

    int type = lmp_private_get_esco_packet_type();
    int frame_time = (lmp_private_get_esco_packet_type() >> 8) & 0xff;
    int media_type = type & 0xff;
    if (media_type == 0) {
        in_fmt->sample_rate = 8000;
        in_fmt->coding_type = AUDIO_CODING_CVSD;
        hdl->tx_frames = frame_time == 12 ? 60 : 30;
    } else  {
        in_fmt->sample_rate = 16000;
        in_fmt->coding_type = AUDIO_CODING_MSBC;
        hdl->tx_frames = frame_time == 12 ? 120 : 60;
    }
    in_fmt->channel_mode = AUDIO_CH_MIX;

    return NEGO_STA_ACCPTED;
}

static int esco_tx_mount_syncts(struct esco_tx_hdl *hdl, void *syncts, u32 timestamp, u8 network)
{
    struct esco_tx_sync_node *node = NULL;
    list_for_each_entry(node, &hdl->sync_list, entry) {
        if ((u32)node->syncts == (u32)syncts) {
            return 0;
        }
    }

    node = (struct esco_tx_sync_node *)zalloc(sizeof(struct esco_tx_sync_node));
    node->syncts = syncts;

    /*g_printf("esco tx mount syncts : 0x%x, %u\n", (u32)syncts, timestamp);*/
    if (hdl->reference_network == 0xff) {
        hdl->reference_network = network;
    }
    list_add(&node->entry, &hdl->sync_list);

    return 0;
}

static void esco_tx_unmount_syncts(struct esco_tx_hdl *hdl, void *syncts)
{
    struct esco_tx_sync_node *node;

    list_for_each_entry(node, &hdl->sync_list, entry) {
        if (node->syncts == syncts) {
            goto unmount;
        }
    }

    return;

unmount:
    /*g_printf("esco tx unmount syncts : 0x%x\n", syncts);*/
    list_del(&node->entry);
    free(node);
}

static int esco_tx_syncts_handler(struct esco_tx_hdl *hdl, struct audio_syncts_ioc_params *params)
{
    if (!params) {
        return 0;
    }

    switch (params->cmd) {
    case AUDIO_SYNCTS_MOUNT_ON_SNDPCM:
        esco_tx_mount_syncts(hdl, (void *)params->data[0], params->data[1], params->data[2]);
        break;
    case AUDIO_SYNCTS_UMOUNT_ON_SNDPCM:
        esco_tx_unmount_syncts(hdl, (void *)params->data[0]);
        break;
    }

    return 0;
}

static int esco_tx_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct esco_tx_hdl *hdl = (struct esco_tx_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        esco_tx_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_OPEN_IPORT:
        esco_tx_open_iport(iport);
        break;
    case NODE_IOC_START:
        lmp_esco_set_tx_notify(hdl->bt_addr, hdl, esco_tx_tick_handler);
        break;
    case NODE_IOC_STOP:
        lmp_esco_set_tx_notify(hdl->bt_addr, NULL, NULL);
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= esco_tx_ioc_fmt_nego(iport);
        break;
    case NODE_IOC_SYNCTS:
        esco_tx_syncts_handler(hdl, (struct audio_syncts_ioc_params *)arg);
        break;
    case NODE_IOC_GET_DELAY:
        return lmp_private_get_esco_tx_packet_num() * 75;
    }

    return 0;
}

static void esco_tx_release(struct stream_node *node)
{

}


REGISTER_STREAM_NODE_ADAPTER(esco_tx_adapter) = {
    .name       = "esco_tx",
    .uuid       = NODE_UUID_ESCO_TX,
    .bind       = esco_tx_bind,
    .ioctl      = esco_tx_ioctl,
    .release    = esco_tx_release,
    .hdl_size   = sizeof(struct esco_tx_hdl),
};

