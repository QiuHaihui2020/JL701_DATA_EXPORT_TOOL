#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".idle_app_msg_handler.data.bss")
#pragma data_seg(".idle_app_msg_handler.data")
#pragma const_seg(".idle_app_msg_handler.text.const")
#pragma code_seg(".idle_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "idle.h"

int idle_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_IDLE)) {
        return 0;
    }

    switch (msg[0]) {
    case APP_MSG_KEY_POWER_ON:
    case APP_MSG_KEY_POWER_ON_HOLD:
        idle_key_poweron_deal(msg[0]);
    default:
        break;
    }

    return 0;
}

