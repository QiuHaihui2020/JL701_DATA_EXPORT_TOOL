#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tws_a2dp_play.data.bss")
#pragma data_seg(".tws_a2dp_play.data")
#pragma const_seg(".tws_a2dp_play.text.const")
#pragma code_seg(".tws_a2dp_play.text")
#endif
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "classic/tws_api.h"
#include "os/os_api.h"
#include "bt_slience_detect.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "app_main.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "btstack/a2dp_media_codec.h"
#include "soundbox.h"
#include "scene_switch.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "bt_key_func.h"
#include "bt_tws.h"

#if(TCFG_USER_TWS_ENABLE)

enum {
    CMD_A2DP_PLAY = 1,
    CMD_A2DP_SLIENCE_DETECT,
    CMD_A2DP_CLOSE,
    CMD_SET_A2DP_VOL,
};

static u8 g_play_addr[6];
static u8 g_slience_addr[6];
static u8 a2dp_play_status = 0;

void app_set_a2dp_play_status(u8 st)
{
    a2dp_play_status = st;
}

u8 app_get_a2dp_play_status(void)
{
    return a2dp_play_status;
}

u8 *get_g_play_addr(void)
{
    return g_play_addr;
}

void tws_a2dp_play_send_cmd(u8 cmd, u8 *data, u8 len, u8 tx_do_action);

void tws_a2dp_player_close(u8 *bt_addr)
{
    puts("tws_a2dp_player_close\n");
    put_buf(bt_addr, 6);
    a2dp_player_close(bt_addr);
    bt_stop_a2dp_slience_detect(bt_addr);
    a2dp_media_close(bt_addr);
    memset(g_play_addr, 0xff, 6);
    app_set_a2dp_play_status(0);
}

static void tws_a2dp_play_in_task(u8 *data)
{
    u8 btaddr[6];
    u8 dev_vol;
    u8 *bt_addr = data + 2;

    switch (data[0]) {
    case CMD_A2DP_SLIENCE_DETECT:
        puts("CMD_A2DP_SLIENCT_DETECE\n");
        put_buf(bt_addr, 6);
        a2dp_player_close(bt_addr);
        bt_start_a2dp_slience_detect(bt_addr, 50);    //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        memset(g_slience_addr, 0xff, 6);
        break;
    case CMD_A2DP_PLAY:
        g_printf("app_msg_bt_a2dp_play:%d\n", data[8]);
        app_set_a2dp_play_status(1);        //从机播歌过程中加入链接需要置上播放标志
        put_buf(bt_addr, 6);
        dev_vol = data[8];
#if (TCFG_BT_BACKGROUND_ENABLE)
        if (bt_background_active()) {
            g_printf("background active start slience\n");
            bt_start_a2dp_slience_detect(bt_addr, 50); //处理后台已经执行挂起但是蓝牙模式还没退出的情况，不打开a2dp_player
            return;
        }
#endif
        //更新一下音量再开始播放
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
        //更新一下音量再开始播放
        if (dev_vol > 127) {    //返回值0xff说明不支持音量同步
            y_printf("device no support sync vol, use sys volume:%d\n", app_var.music_volume);
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
            app_audio_set_volume_def_state(0);
        } else {
            set_music_device_volume(dev_vol);
        }

        bt_stop_a2dp_slience_detect(bt_addr);
        memcpy(g_play_addr, bt_addr, 6);
        a2dp_player_low_latency_enable(tws_api_get_low_latency_state());
        if (le_audio_scene_deal(LE_AUDIO_A2DP_START) > 0) {
            break;
        }
        int err = a2dp_player_open(bt_addr);
        if (err == -EBUSY) {
            bt_start_a2dp_slience_detect(bt_addr, 50); //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        }
        /* memset(g_play_addr, 0xff, 6); */
        musci_vocal_remover_update_parm();
        break;
    case CMD_A2DP_CLOSE:
        if (le_audio_scene_deal(LE_AUDIO_A2DP_STOP) > 0) {
            memset(g_play_addr, 0xff, 6);
            a2dp_media_close(bt_addr);
            break;
        }
        tws_a2dp_player_close(bt_addr);
        if (bt_slience_get_detect_addr(btaddr)) {
            bt_stop_a2dp_slience_detect(btaddr);
            a2dp_media_close(btaddr);
        }
        break;
    case CMD_SET_A2DP_VOL:
        dev_vol = data[8];
        set_music_device_volume(dev_vol);
        break;
    }
    if (data[1] != 2) {
        free(data);
    }
}

static void tws_a2dp_play_callback(u8 *data, u8 len)
{
    int msg[4];

    u8 *buf = malloc(len);
    if (!buf) {
        return;
    }
    memcpy(buf, data, len);
    msg[0] = (int)tws_a2dp_play_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;

    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void tws_a2dp_player_data_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (data[1] == 0 && !rx) {
        return;
    }
    tws_a2dp_play_callback(data, len);
}
REGISTER_TWS_FUNC_STUB(tws_a2dp_player_stub) = {
    .func_id = 0x076AFE82,
    .func = tws_a2dp_player_data_in_irq,
};

void tws_a2dp_play_send_cmd(u8 cmd, u8 *_data, u8 len, u8 tx_do_action)
{
    u8 data[16];
    data[0] = cmd;
    data[1] = tx_do_action;
    memcpy(data + 2, _data, len);
    int err = tws_api_send_data_to_sibling(data, len + 2, 0x076AFE82);
    if (err) {
        data[1] = 2;
        tws_a2dp_play_in_task(data);
    } else {
        if (cmd == CMD_A2DP_PLAY) {
            memcpy(g_play_addr, _data, 6);
        } else if (cmd == CMD_A2DP_SLIENCE_DETECT) {
            memcpy(g_slience_addr, _data, 6);
        }
    }
}

void tws_a2dp_sync_play(u8 *bt_addr, bool tx_do_action)
{
    u8 data[8];
    memcpy(data, bt_addr, 6);
    data[6] = bt_get_music_volume(bt_addr);
    /* if (data[6] > 127) { */
    /*     data[6] = app_audio_bt_volume_update(bt_addr, APP_AUDIO_STATE_MUSIC); */
    /* } */
    tws_a2dp_play_send_cmd(CMD_A2DP_PLAY, data, 7, tx_do_action);
}

void tws_a2dp_slience_detect(u8 *bt_addr, bool tx_do_action)
{
    tws_a2dp_play_send_cmd(CMD_A2DP_SLIENCE_DETECT, bt_addr, 6, tx_do_action);
}


static int a2dp_bt_status_event_handler(int *event)
{
    int ret;
    u8 data[8];
    u8 btaddr[6];
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case BT_STATUS_A2DP_MEDIA_START:
        puts("BT_STATUS_A2DP_MEDIA_START\n");
        put_buf(bt->args, 6);
        if (app_var.goto_poweroff_flag) {
            break;
        }
        app_set_a2dp_play_status(1);
        if (tws_api_get_role() == TWS_ROLE_MASTER &&
            bt_get_call_status_for_addr(bt->args) == BT_CALL_INCOMING) {
            //小米11来电挂断偶现没有hungup过来，hfp链路异常，重新断开hfp再连接
            puts("<<<<<<<<waring a2dp start hfp_incoming\n");
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_DISCONNECT, 0, NULL);
            bt_cmd_prepare_for_addr(bt->args, USER_CTRL_HFP_CMD_CONN, 0, NULL);
        }
        if (esco_player_runing()) {
            a2dp_media_close(bt->args);
            break;
        }
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }

        if (a2dp_player_get_btaddr(btaddr)) {
            if (memcmp(btaddr, bt->args, 6) == 0) {
                tws_a2dp_sync_play(bt->args, 1);
            } else {
                tws_a2dp_slience_detect(bt->args, 1);
            }
        } else {
            tws_a2dp_sync_play(bt->args, 1);
        }
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        puts("BT_STATUS_A2DP_MEDIA_STOP\n");
        app_set_a2dp_play_status(0);
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        tws_a2dp_play_send_cmd(CMD_A2DP_CLOSE, bt->args, 6, 1);
        break;
    case BT_STATUS_AVRCP_VOL_CHANGE:
        //判断是当前地址的音量值才更新
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
        ret = a2dp_player_get_btaddr(data);
        if (ret && memcmp(data, bt->args, 6) == 0) {
            data[6] = bt->value;
            tws_a2dp_play_send_cmd(CMD_SET_A2DP_VOL, data, 7, 1);
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_stack_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = a2dp_bt_status_event_handler,
};


static int a2dp_bt_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        app_set_a2dp_play_status(0);
        tws_a2dp_player_close(bt->args);
        break;
    }

    return 0;
}
APP_MSG_HANDLER(a2dp_hci_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = a2dp_bt_hci_event_handler,
};

static int a2dp_app_msg_handler(int *msg)
{
    u8 *bt_addr = (u8 *)(msg +  1);

    switch (msg[0]) {
    case APP_MSG_BT_A2DP_PAUSE:
        puts("app_msg_bt_a2dp_pause\n");
        if (a2dp_player_is_playing(bt_addr)) {
            tws_a2dp_slience_detect(bt_addr, 1);
        }
        break;
    case APP_MSG_BT_A2DP_PLAY:
        puts("app_msg_bt_a2dp_play\n");
        tws_a2dp_sync_play(bt_addr, 1);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_app_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = a2dp_app_msg_handler,
};

static int a2dp_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state = evt->args[1];
    u8 *bt_addr = evt->args + 3;
    u8 addr[6];

    switch (evt->event) {
    case TWS_EVENT_MONITOR_START:
        if (role == TWS_ROLE_SLAVE) {
            break;
        }
        if (a2dp_player_is_playing(bt_addr)) {
            if (memcmp(g_slience_addr, bt_addr, 6)) {
                puts("--monitor_start_play\n");
                tws_a2dp_sync_play(bt_addr, 0);
            }
        } else {
            int result = bt_slience_detect_get_result(bt_addr);
            if (result != BT_SLIENCE_NO_DETECTING) {
                if (memcmp(g_play_addr, bt_addr, 6)) {
                    puts("--monitor_start_slience_detect\n");
                    tws_a2dp_slience_detect(bt_addr,  0);
                }
            }
        }
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        tws_a2dp_player_close(evt->args + 3);
        break;
    case TWS_EVENT_ROLE_SWITCH:
        while (bt_slience_get_detect_addr(addr)) {
            bt_stop_a2dp_slience_detect(addr);
            a2dp_media_close(addr);
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(a2dp_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = a2dp_tws_msg_handler,
};

static void tws_low_latency_tone_cb(int fname_uuid, enum stream_event event)
{
    if (event == STREAM_EVENT_START) {
        int uuid = tone_player_get_fname_uuid(get_tone_files()->low_latency_in);
        if (uuid == fname_uuid) {
            tws_api_low_latency_enable(1);
            a2dp_player_low_latency_enable(1);
            g_printf("tws_api_low_latency_enable 1");
        } else {
            tws_api_low_latency_enable(0);
            a2dp_player_low_latency_enable(0);
            g_printf("tws_api_low_latency_enable 0");
        }
    }
};
REGISTER_TWS_TONE_CALLBACK(tws_low_latency_entry) = {
    .func_uuid = 0x6F90E37B,
    .callback = tws_low_latency_tone_cb,
};

void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms)
{
    /*
     * 未连接手机,操作无效
     */
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_PHONE_DISCONNECTED) {
        return;
    }

    //不在蓝牙模式不操作
    if (app_in_mode(APP_MODE_BT) == 0) {
        return;
    }

    const char *fname = enable ? get_tone_files()->low_latency_in :
                        get_tone_files()->low_latency_out;
    g_printf("bt_set_low_latency_mode=%d\n", enable);
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (delay_ms < 100) {
            delay_ms = 100;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            tws_play_tone_file_alone_callback(fname, delay_ms, 0x6F90E37B);
        }
    } else {
        play_tone_file_alone(fname);
        tws_api_low_latency_enable(enable);
        a2dp_player_low_latency_enable(enable);
    }

#if TCFG_BT_DUAL_CONN_ENABLE
    if (enable) {
        if (bt_get_total_connect_dev()) {
            lmp_hci_write_scan_enable(0);
        }

    } else {
        tws_dual_conn_state_handler();
    }
#endif
}

int bt_get_low_latency_mode()
{
    return tws_api_get_low_latency_state();
}

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
static int get_a2dp_play_status(void)
{
    r_printf("a2dp_play_status:%d %d %d %d\n", get_le_audio_app_mode_exit_flag(), a2dp_play_status, get_a2dp_decoder_status(), a2dp_player_runing());
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_ERR;
    }

    if (bt_get_call_status() != BT_CALL_HANGUP) {
        return LOCAL_AUDIO_PLAYER_STATUS_ERR;
    }
    if (a2dp_play_status || a2dp_player_runing()) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_ERR;
    }

    return 0;
}

static int a2dp_local_audio_open(void)
{
    if (get_a2dp_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
        int err = a2dp_player_open(get_g_play_addr());
        if (err == -EBUSY) {
            bt_start_a2dp_slience_detect(get_g_play_addr(), 50); //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        }
    }
    return 0;
}

static int a2dp_local_audio_close(void)
{
    if (get_a2dp_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //关闭本地播放
        a2dp_player_close(get_g_play_addr());
    }
    return 0;
}

static void *a2dp_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;

    if (get_a2dp_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        err = le_audio_a2dp_recorder_open(get_g_play_addr(), (void *)&params->fmt, le_audio);
        if (err != 0) {
            ASSERT(0, "recorder open fail");
        }
#if LEA_LOCAL_SYNC_PLAY_EN
        err = le_audio_player_open(le_audio, params);
        if (err != 0) {
            ASSERT(0, "player open fail");
        }
#endif
    }

    return le_audio;
}

static int a2dp_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_a2dp_recorder_close(get_g_play_addr());
    le_audio_stream_free(le_audio);

    return 0;
}

static int a2dp_rx_le_audio_open(void *rx_audio, void *args)
{
    int err;
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //打开广播音频播放
    struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
    rx_audio_hdl->le_audio = le_audio_stream_create(params->conn, &params->fmt);
    rx_audio_hdl->rx_stream = le_audio_stream_rx_open(rx_audio_hdl->le_audio, params->fmt.coding_type);
    err = le_audio_player_open(rx_audio_hdl->le_audio, params);
    if (err != 0) {
        ASSERT(0, "player open fail");
    }

    return 0;
}

static int a2dp_rx_le_audio_close(void *rx_audio)
{
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
    le_audio_player_close(rx_audio_hdl->le_audio);
    le_audio_stream_rx_close(rx_audio_hdl->rx_stream);
    le_audio_stream_free(rx_audio_hdl->le_audio);

    return 0;
}

const struct le_audio_mode_ops le_audio_a2dp_ops = {
    .local_audio_open = a2dp_local_audio_open,
    .local_audio_close = a2dp_local_audio_close,
    .tx_le_audio_open = a2dp_tx_le_audio_open,
    .tx_le_audio_close = a2dp_tx_le_audio_close,
    .rx_le_audio_open = a2dp_rx_le_audio_open,
    .rx_le_audio_close = a2dp_rx_le_audio_close,
    .play_status = get_a2dp_play_status,
};

#endif

#endif
