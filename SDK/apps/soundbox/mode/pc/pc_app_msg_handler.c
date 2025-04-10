#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_app_msg_handler.data.bss")
#pragma data_seg(".pc_app_msg_handler.data")
#pragma const_seg(".pc_app_msg_handler.text.const")
#pragma code_seg(".pc_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "audio_config.h"
#include "usb/device/hid.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "pc_spk_player.h"
#include "uac_stream.h"
#include "app_le_auracast.h"
#include "le_broadcast.h"
#include "rcsp_pc_func.h"

#if TCFG_APP_PC_EN
int pc_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_PC)) {
        return 0;
    }
    printf("pc_app_msg type:0x%x", msg[0]);
    u8 msg_type = msg[0];
#if  LEA_BIG_CTRLER_RX_EN && (LEA_BIG_FIX_ROLE==2) && !TCFG_KBOX_1T3_MODE_EN
    if (get_broadcast_connect_status() &&
        (msg_type == APP_MSG_MUSIC_PP
         || msg_type == APP_MSG_MUSIC_NEXT || msg_type == APP_MSG_MUSIC_PREV
#if LEA_BIG_VOL_SYNC_EN
         || msg_type == APP_MSG_VOL_UP || msg_type == APP_MSG_VOL_DOWN
#endif
         || msg_type == APP_MSG_PC_START
        )) {

        printf("BIS receiving state does not support the event %d", msg_type);

        return 0;

    }
#endif

    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_PC_START:
        printf("app msg pc start\n");
        if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }
        break;
#if TCFG_USB_SLAVE_HID_ENABLE
    case APP_MSG_MUSIC_PP:
        printf("APP_MSG_MUSIC_PP\n");
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2)
        //在pc模式下作为接收端时，控制mute（即播放暂停响应）
        u8 pc_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (get_broadcast_role() == 0) {
            hid_key_handler(0, USB_AUDIO_PP);
        } else {
            //作为广播接收端播歌中
            pc_volume_mute_mark ^= 1;
            audio_app_mute_en(pc_volume_mute_mark);
        }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==2)
        //在pc模式下作为接收端时，控制mute（即播放暂停响应）
        u8 pc_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (get_auracast_role() == 0) {
            hid_key_handler(0, USB_AUDIO_PP);
        } else {
            //作为广播接收端播歌中
            pc_volume_mute_mark ^= 1;
            audio_app_mute_en(pc_volume_mute_mark);
        }
#else
        hid_key_handler(0, USB_AUDIO_PP);
#endif
        break;
    case APP_MSG_MUSIC_PREV:
        printf("APP_MSG_MUSIC_PREV\n");
        hid_key_handler(0, USB_AUDIO_PREFILE);
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("APP_MSG_MUSIC_NEXT\n");
        hid_key_handler(0, USB_AUDIO_NEXTFILE);
        break;
    case APP_MSG_VOL_UP:
        printf("APP_MSG_VOL_UP\n");
        hid_key_handler(0, USB_AUDIO_VOLUP);
        /* printf(">>>pc vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE)); */
        break;
    case APP_MSG_VOL_DOWN:
        printf("APP_MSG_VOL_DOWN\n");
        hid_key_handler(0, USB_AUDIO_VOLDOWN);
        /* printf(">>>pc vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE)); */
        break;
#if 0
    case APP_MSG_VOCAL_REMOVE:
        printf("APP_MSG_VOCAL_REMOVE\n");
        music_vocal_remover_switch();
        break;
    case APP_MSG_MIC_EFFECT_ON_OFF:
        printf("APP_MSG_MIC_EFFECT_ON_OFF\n");
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        } else {
            mic_effect_player_open();
        }
        break;
#endif
#endif
    default:
        app_common_key_msg_handler(msg);
        break;
    }

#if (RCSP_MODE && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE)
    rcsp_pc_msg_deal(msg[0]);
#endif

    return 0;
}

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

static u8 g_le_audio_flag;
static u8 wait_pc_open_broadcast_cb_deal_flag = 0;

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取pc模式广播音频流开启标志
 *
 * @return 广播音频流开启标志
 */
/* ----------------------------------------------------------------------------*/
u8 get_pc_le_audio_flag(void)
{
    return g_le_audio_flag;
}


static int pc_mode_broadcast_deal_callback(int deal_music_status)
{
    if (!app_in_mode(APP_MODE_PC)) {
        return 0;
    }
    printf(">>>>>[PC] Enter %s Func! deal_music_status:%d!!\n", __func__, deal_music_status);
    wait_pc_open_broadcast_cb_deal_flag = 0;
    le_audio_scene_deal(deal_music_status);
    return 0;
}
/* --------------------------------------------------------------------------*/
/**
 * @brief 通过线程打开pc模式的广播，避免在中断里处理有关
 * 		  调用app_broadcast_deal( A ) 函数，传入的参数A就是 broadcast_music_status
 *
 * @return 广播音频流开启标志
 */
/* ----------------------------------------------------------------------------*/
int pc_mode_broadcast_deal_by_taskq(int broadcast_music_status)
{
    int msg[3];
    int ret = -1;
    if (wait_pc_open_broadcast_cb_deal_flag == 0) {
        msg[0] = (int)pc_mode_broadcast_deal_callback;
        msg[1] = 1; //broadcast_music_status;
        msg[2] = broadcast_music_status;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (ret == 0) {
            wait_pc_open_broadcast_cb_deal_flag = 1;
        } else {
            ret = -1;
        }
    }
    return ret;
}


static int get_pc_play_status(void)
{
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }

    if (pc_get_status()) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
}

static int pc_local_audio_open(void)
{
    if (get_pc_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        pc_spk_player_open();
#endif
    }
    return 0;
}

static int pc_local_audio_close(void)
{
    if (get_pc_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        if (pc_spk_player_runing()) {
            //关闭本地播放
            pc_spk_player_close();
        }
    }
    return 0;
}

static void *pc_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;
#if defined(TCFG_USB_SLAVE_AUDIO_SPK_ENABLE) && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    if (1) {//(get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        g_le_audio_flag = 1;
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        err = le_audio_pc_recorder_open((void *)&params->fmt, le_audio, params->latency);
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
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    update_app_auracast_deal_scene(LE_AUDIO_MUSIC_START);
#endif
#endif
    return le_audio;
}

static int pc_tx_le_audio_close(void *le_audio)
{
#if defined(TCFG_USB_SLAVE_AUDIO_SPK_ENABLE) && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    if (!le_audio) {
        return -EPERM;
    }
    g_le_audio_flag = 0;
    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_pc_recorder_close();
    le_audio_stream_free(le_audio);
    /* app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1); */
#endif
    return 0;
}

static int pc_rx_le_audio_open(void *rx_audio, void *args)
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

static int pc_rx_le_audio_close(void *rx_audio)
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

const struct le_audio_mode_ops le_audio_pc_ops = {
    .local_audio_open = pc_local_audio_open,
    .local_audio_close = pc_local_audio_close,
    .tx_le_audio_open = pc_tx_le_audio_open,
    .tx_le_audio_close = pc_tx_le_audio_close,
    .rx_le_audio_open = pc_rx_le_audio_open,
    .rx_le_audio_close = pc_rx_le_audio_close,
    .play_status = get_pc_play_status,
};

#endif
#endif
