#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_app_msg_soundbar_style.data.bss")
#pragma data_seg(".ui_app_msg_soundbar_style.data")
#pragma const_seg(".ui_app_msg_soundbar_style.text.const")
#pragma code_seg(".ui_app_msg_soundbar_style.text")
#endif
#include "app_config.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "app_main.h"

#if (TCFG_UI_ENABLE && (CONFIG_UI_STYLE == STYLE_JL_SOUNDBAR))

void ui_soundbar_style_power_off_wait()
{
//不同类型屏幕可能有不同的操作，请根据实际情况处理
    struct lcd_interface *lcd_hl;
    lcd_hl = lcd_get_hdl();
    lcd_hl->clear_screen(0x000000);
}

//lcd ui 消息处理
static void lcd_enter_mode(u8 mode)
{
    switch (mode) {
    case APP_MODE_IDLE:
        UI_SHOW_WINDOW(ID_WINDOW_IDLE);
        break;
    case APP_MODE_POWERON:
        UI_SHOW_WINDOW(ID_WINDOW_POWER_ON);
        break;
    case APP_MODE_BT:
        UI_SHOW_WINDOW(ID_WINDOW_BT);
        break;
    case APP_MODE_MUSIC:
        UI_SHOW_WINDOW(ID_WINDOW_MUSIC);
        break;
    case APP_MODE_FM:
        UI_SHOW_WINDOW(ID_WINDOW_FM);
        break;
    case APP_MODE_LINEIN:
        UI_SHOW_WINDOW(ID_WINDOW_LINEIN);
        break;
    case APP_MODE_SPDIF:
        UI_SHOW_WINDOW(ID_WINDOW_SPDIF);
        break;
    }
}

static int ui_lcd_msg_entry(int *msg)
{
    char *logo;
    printf("lcd msg:%d\n", msg[0]);
    switch (msg[0]) {
    case APP_MSG_ENTER_MODE:
        printf("<<<<<<lcd enter mode:%d\n", msg[1]);
        lcd_enter_mode(msg[1] & 0xff);
        break;
    case APP_MSG_EXIT_MODE:
        printf(">>>>>>lcd exit mode:%d\n", msg[1]);
        UI_HIDE_CURR_WINDOW();
        break;
    case APP_MSG_VOL_CHANGED:
        UI_MSG_POST("music_vol:vol=%4", msg[1]);
        break;
    case APP_MSG_MUTE_CHANGED:
        UI_MSG_POST("mute_sta:mute=%4", msg[1]);
        break;
    case APP_MSG_EQ_CHANGED:
        UI_MSG_POST("eq_sta:eq=%4", msg[1]);
        break;
    case APP_MSG_FM_REFLASH:
        UI_MSG_POST("fm_fre", NULL);
        break;
    case APP_MSG_FM_STATION:
        UI_MSG_POST("fm_fre", NULL);
        break;
#if (TCFG_APP_MUSIC_EN)
    case APP_MSG_MUSIC_FILE_NUM_CHANGED:
        /* printf("msg[1]:%d,msg[2]:%d\n",msg[1],msg[2]); */
        int analaz =  music_player_lrc_analy_start(music_app_get_cur_hdl());
        logo = music_app_get_dev_cur();
        UI_MSG_POST("music_start:show_lyric=%4:dev=%4:filenum=%4:total_filenum=%4", !analaz, logo, msg[1], msg[2]);
        break;
    case APP_MSG_MUSIC_PLAY_STATUS:
        logo = music_app_get_dev_cur();
        if (FILE_PLAYER_PAUSE == msg[1]) {
            //显示暂停图标
            logo = "pause";
        }
        UI_MSG_POST("music_start:dev=%4", logo);
        break;
#endif
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(lcd_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = ui_lcd_msg_entry,
};

#endif

