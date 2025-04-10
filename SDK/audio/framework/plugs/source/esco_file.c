#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_file.data.bss")
#pragma data_seg(".esco_file.data")
#pragma const_seg(".esco_file.text.const")
#pragma code_seg(".esco_file.text")
#endif
#include "classic/hci_lmp.h"
#include "source_node.h"
#include "sync/audio_syncts.h"
#include "media/bt_audio_timestamp.h"
#include "reference_time.h"
#include "system/timer.h"


struct esco_file_hdl {
    u8 start;
    u8 stop;
    u8 esco_len;
    u8 first_timestamp;
    u8 frame_time;
    u8 reference;
    u16 timer;
    u32 clkn;
    u32 coding_type;
    struct stream_node *node;
    void *ts_handle;
    u8 bt_addr[6];
};

static void esco_file_timestamp_setup(struct esco_file_hdl *hdl);
static void esco_frame_pack_timestamp(struct esco_file_hdl *hdl, struct stream_frame *frame, u32 clkn)
{
    u32 frame_clkn = clkn;
    if (!hdl->ts_handle) {
        return;
    }

    if (hdl->first_timestamp) {
        hdl->first_timestamp = 0;
    } else {
        if (((frame_clkn - hdl->clkn) & 0x7fffff) != hdl->frame_time) {
            printf("frame_clkn =%d ,clkn %d %d\n", frame_clkn, ((frame_clkn - hdl->clkn) & 0x7fffff), hdl->frame_time);
            frame_clkn = hdl->clkn + hdl->frame_time;
        }
    }
    frame->timestamp = esco_audio_timestamp_update(hdl->ts_handle, frame_clkn);
    frame->flags |= FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    hdl->clkn = clkn;
}


static void esco_start_abandon_data(struct esco_file_hdl *hdl)
{
    lmp_private_esco_suspend_resume(1);
}

static void esco_stop_abandon_data(struct esco_file_hdl *hdl)
{
    lmp_private_esco_suspend_resume(2);
}

static void esco_packet_rx_notify(void *_hdl)
{
    struct esco_file_hdl *hdl = (struct esco_file_hdl *)_hdl;
    int timeout = ((hdl->frame_time + 1) * 625) / 1000 + 2;

    jlstream_wakeup_thread(NULL, hdl->node, NULL);

    if (hdl->timer) {
        sys_hi_timer_modify(hdl->timer, timeout);
    }
}

static void esco_rx_wakeup_timer(void *_hdl)
{
    struct esco_file_hdl *hdl = (struct esco_file_hdl *)_hdl;

    esco_packet_rx_notify(hdl);
}

static enum stream_node_state esco_get_frame(void *_hdl, struct stream_frame **_frame)
{
    int len = 0;
    u32 frame_clkn = 0;
    struct esco_file_hdl *hdl = (struct esco_file_hdl *)_hdl;
    struct stream_frame *frame;

    u8 *packet = lmp_private_get_esco_packet(&len, &frame_clkn);
    if (!packet) {
        *_frame = NULL;
        if (check_esco_conn_exist(hdl->bt_addr)) {
            return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
        } else { //esco链路已经关闭，补丢包数据进行淡出。
            len = hdl->frame_time == 12 ? 60 : 30;
            frame_clkn = hdl->clkn + hdl->frame_time;
            if (hdl->timer) {
                int timeout = ((hdl->frame_time + 1) * 625) / 1000 - 1;
                sys_hi_timer_modify(hdl->timer, timeout);
            }
        }
        /* return NODE_STA_RUN | NODE_STA_DEC_END; */
    }
    frame = jlstream_get_frame(hdl->node->oport, len);
    frame->len = len;
    if (hdl->frame_time == 0xff) {
        /*对SCO链路获取不到T_sco的容错*/
        hdl->frame_time = len >= 60 ? 12 : 6;
    }
    esco_frame_pack_timestamp(hdl, frame, frame_clkn);
    if (packet) {
        memcpy(frame->data, packet, len);
        lmp_private_free_esco_packet(packet);
    } else {
        //填丢包标志，由plc 进行补包
        memset(frame->data, 0xAA, len);
        memset(frame->data, 0x55, 2);
    }

    *_frame = frame;

    return NODE_STA_RUN;
}

static void *esco_init(void *priv, struct stream_node *node)
{
    struct esco_file_hdl *hdl = zalloc(sizeof(*hdl));
    hdl->node = node;
    node->type |= NODE_TYPE_IRQ;
    return hdl;
}

static int esco_ioc_set_bt_addr(struct esco_file_hdl *hdl, u8 *bt_addr)
{
    memcpy(hdl->bt_addr, bt_addr, 6);
    return 0;
}

static void esco_ioc_get_fmt(struct esco_file_hdl *hdl, struct stream_fmt *fmt)
{
    int type = lmp_private_get_esco_packet_type();
    int media_type = type & 0xff;
    if (media_type == 0) {
        fmt->sample_rate = 8000;
        fmt->coding_type = AUDIO_CODING_CVSD;
    } else  {
        fmt->sample_rate = 16000;
        fmt->coding_type = AUDIO_CODING_MSBC;
    }
    fmt->channel_mode = AUDIO_CH_MIX;
    hdl->coding_type = fmt->coding_type;
}

static void esco_ioc_stop(struct esco_file_hdl *hdl)
{
    hdl->stop = 1;
    lmp_esco_set_rx_notify(hdl->bt_addr, NULL, NULL);
}

void esco_ts_handle_create(struct esco_file_hdl *hdl)
{
    if (!hdl) {
        return;
    }
#define ESCO_DELAY_TIME     60
#define ESCO_RECOGNTION_TIME 220

    int delay_time = ESCO_DELAY_TIME;
    /* if (get_sniff_out_status()) { */
    /* clear_sniff_out_status(); */
    /* if (ESCO_SIRI_WAKEUP()) { */
    /* [>fix : Siri出sniff蓝牙数据到音频通路延迟过长，容易引入同步的问题<] */
    /* delay_time = ESCO_RECOGNTION_TIME; */
    /* } */
    /* } */

    if (!hdl->ts_handle) {
        hdl->reference = audio_reference_clock_select(hdl->bt_addr, 0);//0 - a2dp主机，1 - tws, 2 - BLE

        hdl->frame_time = (lmp_private_get_esco_packet_type() >> 8) & 0xff;

        hdl->ts_handle = esco_audio_timestamp_create(hdl->frame_time, delay_time, TIME_US_FACTOR);
    }
    hdl->first_timestamp = 1;
}

void esco_ts_handle_release(struct esco_file_hdl *hdl)
{
    if (!hdl) {
        return;
    }
    if (hdl->ts_handle) {
        esco_audio_timestamp_close(hdl->ts_handle);
        hdl->ts_handle = NULL;
        audio_reference_clock_exit(hdl->reference);
    }
}

static void esco_file_timestamp_setup(struct esco_file_hdl *hdl)
{
    int err = stream_node_ioctl(hdl->node, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SYNCTS, 0);
    if (err) {
        return;
    }
    esco_ts_handle_create(hdl);
}

static void esco_file_timestamp_close(struct esco_file_hdl *hdl)
{
    esco_ts_handle_release(hdl);
}


static int esco_ioctl(void *_hdl, int cmd, int arg)
{
    struct esco_file_hdl *hdl = (struct esco_file_hdl *)_hdl;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        esco_ioc_set_bt_addr(hdl, (u8 *)arg);
        break;
    case NODE_IOC_GET_FMT:
        esco_ioc_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_SUSPEND:
        lmp_esco_set_rx_notify(hdl->bt_addr, NULL, NULL);
        esco_start_abandon_data(hdl);
        esco_file_timestamp_close(hdl);
        if (hdl->timer) {
            sys_hi_timer_del(hdl->timer);
            hdl->timer = 0;
        }
        break;
    case NODE_IOC_START:
        esco_stop_abandon_data(hdl);
        lmp_esco_set_rx_notify(hdl->bt_addr, hdl, esco_packet_rx_notify);
        esco_file_timestamp_setup(hdl);
        if (!hdl->timer) {
            hdl->timer = sys_hi_timer_add((void *)hdl, esco_rx_wakeup_timer, 100);
        }
        break;
    case NODE_IOC_STOP:
        esco_ioc_stop(hdl);
        esco_file_timestamp_close(hdl);
        if (hdl->timer) {
            sys_hi_timer_del(hdl->timer);
            hdl->timer = 0;
        }
        break;
    }

    return 0;
}

static void esco_release(void *_hdl)
{
    struct esco_file_hdl *hdl = (struct esco_file_hdl *)_hdl;

    free(hdl);
}


REGISTER_SOURCE_NODE_PLUG(esco_file_plug) = {
    .uuid       = NODE_UUID_ESCO_RX,
    .init       = esco_init,
    .get_frame  = esco_get_frame,
    .ioctl      = esco_ioctl,
    .release    = esco_release,
};









