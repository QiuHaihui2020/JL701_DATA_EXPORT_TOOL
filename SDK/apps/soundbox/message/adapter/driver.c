#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".driver.data.bss")
#pragma data_seg(".driver.data")
#pragma const_seg(".driver.text.const")
#pragma code_seg(".driver.text")
#endif
#include "app_msg.h"
#include "system/event.h"
#include "usb/usb_task.h"
#include "app_config.h"

void sdx_dev_event_to_user(u32 arg, u8 sdx_status, u8 sdx_index)
{

    int msg[3];

    msg[0] = arg;
    msg[1] = sdx_status;

    if (arg == (u32)DRIVER_EVENT_FROM_SD0) {
        msg[2] = (int)"sd0";
    } else {
        msg[2] = (int)"sd1";
    }

    printf("sd dev msg %x, %x, %x\n", msg[0], msg[1], msg[2]);


#if (TCFG_DEV_MANAGER_ENABLE)
    os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 3, msg);
#else
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);
#endif

}

void usb_driver_event_to_user(u32 from, u32 event, void *arg)
{
    int msg[3] = {0};

    msg[0] = from;
    msg[1] = event;
    msg[2] = (int)arg;
#if (TCFG_DEV_MANAGER_ENABLE)
    os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 3, msg);
#else
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 3, msg);
#endif
}

void usb_driver_event_from_otg(u32 from, u32 event, void *arg)
{
    os_taskq_post_msg("usb_stack", 4, USBSTACK_OTG_MSG, from, event, arg);
}


