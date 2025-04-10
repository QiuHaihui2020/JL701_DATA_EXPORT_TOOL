#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_app_msg_led7.data.bss")
#pragma data_seg(".ui_app_msg_led7.data")
#pragma const_seg(".ui_app_msg_led7.text.const")
#pragma code_seg(".ui_app_msg_led7.text")
#endif
#include "app_config.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "app_main.h"

#if (TCFG_UI_ENABLE && (CONFIG_UI_STYLE == STYLE_JL_LED7))

static void led7_enter_mode(u8 mode)
{
    printf("ui led7_MODE:%d\n", mode);
    switch (mode) {
    case APP_MODE_IDLE:
        UI_SHOW_WINDOW(ID_WINDOW_IDLE);
        break;
    case APP_MODE_POWERON:
        UI_SHOW_MENU(MENU_POWER_UP, 0, 0, NULL);
        break;
    case APP_MODE_BT:
        UI_SHOW_WINDOW(ID_WINDOW_BT);
        UI_SHOW_MENU(MENU_BT, 1000, 0, NULL);
        break;
    case APP_MODE_MUSIC:
        UI_SHOW_WINDOW(ID_WINDOW_MUSIC);
        UI_SHOW_MENU(MENU_WAIT, 0, 0, NULL);
        break;
    case APP_MODE_FM:
        UI_SHOW_WINDOW(ID_WINDOW_FM);
        /* UI_SHOW_MENU(MENU_FM_WAIT, 0, 0, NULL); */
        break;
    case APP_MODE_RECORD:
        UI_SHOW_WINDOW(ID_WINDOW_REC);
        break;
    case APP_MODE_LINEIN:
        UI_SHOW_WINDOW(ID_WINDOW_LINEIN);
        /* UI_SHOW_MENU(MENU_AUX, 0, 0, NULL); */
        break;
    case APP_MODE_RTC:
        UI_SHOW_WINDOW(ID_WINDOW_CLOCK);
        break;
    case APP_MODE_PC:
        UI_SHOW_WINDOW(ID_WINDOW_PC);
        /* UI_SHOW_MENU(MENU_PC, 1000, 0, NULL); */
        break;
    case APP_MODE_SPDIF:
        UI_SHOW_WINDOW(ID_WINDOW_SPDIF);
        UI_SHOW_MENU(MENU_SPDIF, 0, 0, NULL);
        break;
    case APP_MODE_SINK:
        UI_SHOW_WINDOW(ID_WINDOW_SINK);
        UI_SHOW_MENU(MENU_SINK, 0, 0, NULL);
        break;
    case APP_MODE_IIS:
        UI_SHOW_WINDOW(ID_WINDOW_IIS);
        /* UI_SHOW_MENU(MENU_IIS, 0, 0, NULL); */
        break;
    }
}


//*----------------------------------------------------------------------------*/
/**@brief   led7 ui处理
   @param   msg:主线程转发过来的msg
   @return  0
   @note	集中处理ui信息
*/
/*----------------------------------------------------------------------------*/
static int ui_led7_msg_entry(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_ENTER_MODE:
        led7_enter_mode(msg[1] & 0xff);
        break;
    case APP_MSG_EXIT_MODE:
        UI_HIDE_CURR_WINDOW();
        break;
    case APP_MSG_VOL_CHANGED:
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, msg[1], NULL);
        break;
    case APP_MSG_MUSIC_FILE_NUM_CHANGED:
        UI_SHOW_MENU(MENU_FILENUM, 1000, msg[1], NULL);
        break;
    case APP_MSG_REPEAT_MODE_CHANGED:
        UI_SHOW_MENU(MENU_MUSIC_REPEATMODE, 1000, msg[1], NULL);
        break;
    case APP_MSG_FM_INIT_OK:
    case APP_MSG_FM_REFLASH:
        UI_REFLASH_WINDOW(true);
        break;
    case APP_MSG_FM_STATION:
        UI_SHOW_MENU(MENU_FM_STATION, 1000, msg[1], NULL);
        break;
    case APP_MSG_INPUT_FILE_NUM:
        UI_SHOW_MENU(MENU_FILENUM, 4 * 1000, msg[1], NULL);
        break;
    case APP_MSG_RTC_SET:
        UI_SHOW_MENU(MENU_RTC_SET, 10 * 1000, 0, (void (*)(int))msg[1]);
        break;
    case APP_MSG_EQ_CHANGED:
        UI_SHOW_MENU(MENU_SET_EQ, 1000, msg[1], NULL);
        break;
    case APP_MSG_MUTE_CHANGED:
        if (msg[1]) { //mute状态
            UI_SHOW_MENU(MENU_SET_MUTE, 0, 0, NULL);
        } else {    //恢复ui
            UI_REFLASH_WINDOW(true);
        }
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(led7_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = ui_led7_msg_entry,
};

#endif

