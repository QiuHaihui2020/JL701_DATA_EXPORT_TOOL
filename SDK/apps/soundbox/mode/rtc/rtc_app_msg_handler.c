#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rtc_app_msg_handler.data.bss")
#pragma data_seg(".rtc_app_msg_handler.data")
#pragma const_seg(".rtc_app_msg_handler.text.const")
#pragma code_seg(".rtc_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "rtc.h"

#if TCFG_APP_RTC_EN

int rtc_app_msg_handler(int *msg)
{
    int msg_argc[4];

    if (false == app_in_mode(APP_MODE_RTC)) {
        return 0;
    }

    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("APP_MSG_CHANGE_MODE\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        app_send_message_from(MSG_FROM_RTC, sizeof(msg_argc), msg_argc);
        break;
    case APP_MSG_RTC_UP:
        printf("APP_MSG_RTC_UP \n");
        set_rtc_up();
        app_send_message_from(MSG_FROM_RTC, sizeof(msg_argc), msg_argc);
        break;
    case APP_MSG_RTC_DOWN:
        printf("APP_MSG_RTC_DOWN \n");
        set_rtc_down();
        app_send_message_from(MSG_FROM_RTC, sizeof(msg_argc), msg_argc);
        break;
    case APP_MSG_RTC_SW:
        printf("APP_MSG_RTC_SW \n");
        set_rtc_sw();
        app_send_message_from(MSG_FROM_RTC, sizeof(msg_argc), msg_argc);
        break;
    case APP_MSG_RTC_SW_POS:
        printf("APP_MSG_RTC_SW_POS \n");
        set_rtc_pos();
        app_send_message_from(MSG_FROM_RTC, sizeof(msg_argc), msg_argc);
        break;
    default:
        break;
    }

    return 0;
}

#endif
