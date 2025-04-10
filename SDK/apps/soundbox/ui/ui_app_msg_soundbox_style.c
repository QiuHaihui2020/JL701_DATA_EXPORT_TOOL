#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_app_msg_soundbox_style.data.bss")
#pragma data_seg(".ui_app_msg_soundbox_style.data")
#pragma const_seg(".ui_app_msg_soundbox_style.text.const")
#pragma code_seg(".ui_app_msg_soundbox_style.text")
#endif
#include "app_config.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "app_main.h"


#if (TCFG_UI_ENABLE && (CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

static int ui_lcd_bt_stack_msg_entry(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    printf("lcd bt stack:0x%x\n", bt->event);
    UI_MSG_POST("bt_status:event=%4", bt->event);

    return 0;
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
    case APP_MODE_RECORD:
        UI_SHOW_WINDOW(ID_WINDOW_REC);
        break;
    case APP_MODE_LINEIN:
        UI_SHOW_WINDOW(ID_WINDOW_LINEIN);
        break;
    case APP_MODE_RTC:
        UI_SHOW_WINDOW(ID_WINDOW_CLOCK);
        break;
    case APP_MODE_PC:
        UI_SHOW_WINDOW(ID_WINDOW_PC);
        break;
    case APP_MODE_SPDIF:
        UI_SHOW_WINDOW(ID_WINDOW_SPDIF);
        break;
    case APP_MODE_SINK:
        UI_SHOW_WINDOW(ID_WINDOW_SINK);
        break;
    }
}


//*----------------------------------------------------------------------------*/
/**@brief   lcd ui处理
   @param   msg:主线程转发过来的msg
   @return  0
   @note	集中处理ui信息
*/
/*----------------------------------------------------------------------------*/
static int ui_lcd_msg_entry(int *msg)
{
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
    case APP_MSG_MUSIC_FILE_NUM_CHANGED:
        /* printf("msg[1]:%d,msg[2]:%d\n",msg[1],msg[2]); */
        int analaz =  music_player_lrc_analy_start(music_app_get_cur_hdl());
        char *logo = music_app_get_dev_cur();
        UI_MSG_POST("music_start:show_lyric=%4:dev=%4:filenum=%4:total_filenum=%4", !analaz, logo, msg[1], msg[2]);
        break;
    case APP_MSG_REPEAT_MODE_CHANGED:
        break;
    case APP_MSG_FM_REFLASH:
        UI_REFLASH_WINDOW(true);
        UI_MSG_POST("fm_fre", NULL);
        break;
    case APP_MSG_FM_STATION:
        UI_MSG_POST("fm_fre", NULL);
        break;
    case APP_MSG_INPUT_FILE_NUM:
        break;
    case APP_MSG_RTC_SET:
        break;

    //转发按键类消息
    case APP_MSG_LCD_OK:
        ui_key_msg_post(UI_KEY_OK);
        break;
    case APP_MSG_LCD_MENU:
        ui_key_msg_post(UI_KEY_MENU);
        break;
    case APP_MSG_LCD_UP:
        ui_key_msg_post(UI_KEY_UP);
        break;
    case APP_MSG_LCD_DOWN:
        ui_key_msg_post(UI_KEY_DOWN);
        break;
    case APP_MSG_LCD_VOL_INC:
        ui_key_msg_post(UI_KEY_VOLUME_INC);
        break;
    case APP_MSG_LCD_VOL_DEC:
        ui_key_msg_post(UI_KEY_VOLUME_DEC);
        break;
    case APP_MSG_LCD_MODE:
        ui_key_msg_post(UI_KEY_MODE);
        break;
    case APP_MSG_LCD_POWER:
        break;
    case APP_MSG_LCD_POWER_START:
        break;
    //转发按键类消息

    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(lcd_bt_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = ui_lcd_bt_stack_msg_entry,
};

APP_MSG_HANDLER(lcd_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = ui_lcd_msg_entry,
};

#endif

