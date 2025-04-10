#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm_app_msg_handler.data.bss")
#pragma data_seg(".fm_app_msg_handler.data")
#pragma const_seg(".fm_app_msg_handler.text.const")
#pragma code_seg(".fm_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "fm_api.h"
#include "fm_player.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "rcsp_fm_func.h"

#if (TCFG_SPI_LCD_ENABLE)
#include "ui/ui_api.h"
#endif

#if TCFG_APP_FM_EN
int fm_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_FM)) {
        return 0;
    }

#if (TCFG_SPI_LCD_ENABLE)
    if (key_is_ui_takeover()) {
        return false;
    }
#endif

    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    //暂停播放
    case APP_MSG_MUSIC_PP:
        printf("app msg fm pp\n");
        fm_volume_pp();
        break;
    //全自动搜台
    case APP_MSG_FM_SCAN_ALL:
    case APP_MSG_FM_SCAN_ALL_DOWN:
    case APP_MSG_FM_SCAN_ALL_UP:
        fm_scan_all();
        break;
    //半自动搜台
    case APP_MSG_FM_SCAN_DOWN:
        fm_scan_down();
        break;
    //半自动搜台
    case APP_MSG_FM_SCAN_UP:
        fm_scan_up();
        break;
    //上一台
    case APP_MSG_FM_PREV_STATION:
        fm_prev_station();
        break;
    //下一台
    case APP_MSG_FM_NEXT_STATION:
        fm_next_station();
        break;
    //上一个频率
    case APP_MSG_FM_PREV_FREQ:
        fm_prev_freq();
        break;
    //下一个频率
    case APP_MSG_FM_NEXT_FREQ:
        fm_next_freq();
        break;
    //增加音量
    case APP_MSG_VOL_UP:
        fm_volume_up();
        break;
    //降低音量
    case APP_MSG_VOL_DOWN:
        fm_volume_down();
        break;
    case APP_MSG_PITCH_UP:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg fm pitch up\n");
        if (fm_player_runing()) {
            app_var.pitch_mode = fm_file_pitch_up(); //返回当前变调模式
        }
#endif
        break;
    case APP_MSG_PITCH_DOWN:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg fm pitch down\n");
        if (fm_player_runing()) {
            app_var.pitch_mode = fm_file_pitch_down();
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

#if (RCSP_MODE)
    rcsp_fm_msg_deal(msg[0]);
#endif

    return 0;
}

#endif

