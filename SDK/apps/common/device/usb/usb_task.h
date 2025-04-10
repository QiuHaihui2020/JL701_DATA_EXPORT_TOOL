#ifndef __USB_TASK_H__
#define __USB_TASK_H__

#include "usb/usb.h"
#include "usb/ch9.h"
#include "usb/usb_phy.h"

#define     USB_TASK_NAME   "usb_stack"

enum {
    USBSTACK_OTG_MSG = 0x80,
    USBSTACK_START,
    USBSTACK_PAUSE,
    USBSTACK_STOP,
    USBSTACK_MSD_RUN,
    USBSTACK_CDC_BACKGROUND,
    USBSTACK_HOST_MSG,
    USBSTACK_HOST_MOUNT,
    USBSTACK_HOST_UNMOUNT,
    USBSTACK_HOST_REMOUNT,
    USBSTACK_HOST_MOUNT_AFTER,
    USBSTACK_HOST_UNMOUNT_AFTER,
};


void usb_message_to_stack(int msg, void *arg, u8 sync);

#endif
