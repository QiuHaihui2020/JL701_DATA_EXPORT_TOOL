#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein_app_msg_handler.data.bss")
#pragma data_seg(".linein_app_msg_handler.data")
#pragma const_seg(".linein_app_msg_handler.text.const")
#pragma code_seg(".linein_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "linein.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "linein_player.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "le_broadcast.h"
#include "audio_config.h"
#include "app_le_auracast.h"

#include "rcsp_linein_func.h"

#if TCFG_APP_LINEIN_EN

static u8 linein_last_onoff = (u8) - 1;

extern u8 linein_get_status(void);

int linein_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_LINEIN)) {
        return 0;
    }

    printf("linein_app_msg type:0x%x", msg[0]);
    u8 msg_type = msg[0];
#if  LEA_BIG_CTRLER_RX_EN && (LEA_BIG_FIX_ROLE==2) && !TCFG_KBOX_1T3_MODE_EN
    if (get_broadcast_connect_status() &&
        (msg_type == APP_MSG_MUSIC_PP
         || msg_type == APP_MSG_MUSIC_NEXT || msg_type == APP_MSG_MUSIC_PREV
#if LEA_BIG_VOL_SYNC_EN
         || msg_type == APP_MSG_VOL_UP || msg_type == APP_MSG_VOL_DOWN
#endif
         || msg_type == APP_MSG_LINEIN_START
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
    case APP_MSG_LINEIN_START:
        printf("app msg linein start\n");
        if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            linein_last_onoff = 1;
            break;
        }
        linein_start();
        linein_last_onoff = 1;
        /* UI_REFLASH_WINDOW(true);//刷新主页并且支持打断显示 */
        break;
    case APP_MSG_MUSIC_PP:
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2)) && !TCFG_KBOX_1T3_MODE_EN
        //固定为接收端
        u8 linein_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (get_broadcast_role() == 2) {
            //接收端已连上
            linein_volume_mute_mark ^= 1;
            audio_app_mute_en(linein_volume_mute_mark);
        } else {
            if (linein_volume_mute_mark == 1) {
                //没有连接情况下，如果之前是mute住了，那么先解mute
                linein_volume_mute_mark ^= 1;
                audio_app_mute_en(linein_volume_mute_mark);
                break;
            }
        }
        linein_last_onoff = linein_volume_pp();
        app_send_message(APP_MSG_LINEIN_PLAY_STATUS, linein_last_onoff);
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==2)
        //固定为接收端
        u8 linein_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (get_auracast_role() == 2) {
            //接收端已连上
            linein_volume_mute_mark ^= 1;
            audio_app_mute_en(linein_volume_mute_mark);
        } else {
            if (linein_volume_mute_mark == 1) {
                //没有连接情况下，如果之前是mute住了，那么先解mute
                linein_volume_mute_mark ^= 1;
                audio_app_mute_en(linein_volume_mute_mark);
                break;
            }
        }
        linein_last_onoff = linein_volume_pp();
        app_send_message(APP_MSG_LINEIN_PLAY_STATUS, linein_last_onoff);
#else
        linein_last_onoff = linein_volume_pp();
        app_send_message(APP_MSG_LINEIN_PLAY_STATUS, linein_last_onoff);
#endif
        break;
    case APP_MSG_VOL_UP:
        linein_key_vol_up();
        break;
    case APP_MSG_VOL_DOWN:
        linein_key_vol_down();
        break;
    case APP_MSG_PITCH_UP:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg linein pitch up\n");
        if (linein_player_runing()) {
            app_var.pitch_mode = linein_file_pitch_up(); //返回当前变调模式
        }
#endif
        break;
    case APP_MSG_PITCH_DOWN:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg linein pitch down\n");
        if (linein_player_runing()) {
            app_var.pitch_mode = linein_file_pitch_down();
        }
#endif
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
    default:
        app_common_key_msg_handler(msg);
        break;
    }


#if RCSP_MODE
    rcsp_linein_msg_deal(msg[0], 0);
#endif

    return 0;
}




#endif
