#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".battery.data.bss")
#pragma data_seg(".battery.data")
#pragma const_seg(".battery.text.const")
#pragma code_seg(".battery.text")
#endif
#include "battery_manager.h"
#include "app_msg.h"


void batmgr_send_msg(enum battery_msg _msg, int arg)
{
    int msg[2];

    msg[0] = _msg;
    msg[1] = arg;
    os_taskq_post_type("app_core", MSG_FROM_BATTERY, 2, msg);
}

void charge_event_to_user(u8 event)
{
    batmgr_send_msg(event, 0);
}
