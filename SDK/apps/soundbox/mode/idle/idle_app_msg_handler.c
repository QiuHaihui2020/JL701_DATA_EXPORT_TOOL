#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".idle_app_msg_handler.data.bss")
#pragma data_seg(".idle_app_msg_handler.data")
#pragma const_seg(".idle_app_msg_handler.text.const")
#pragma code_seg(".idle_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "dev_status.h"
#include "init.h"
#include "idle.h"

int idle_app_msg_handler(int *msg)
{
    char *logo = NULL;
    char *evt_logo = NULL;
    int ret = 0;

    if (false == app_in_mode(APP_MODE_IDLE)) {
        return -1;
    }

    switch (msg[0]) {
    case APP_MSG_KEY_POWER_ON:
    case APP_MSG_KEY_POWER_ON_HOLD:
        idle_key_poweron_deal(msg[0]);
        break;
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
        if (msg[1] == DEVICE_EVENT_OUT) {
            ///下线处理,设备管理卸载
            printf("[idle]DEVICE_EVENT_OUT\n");
            extern void audio_uart_exit();
            audio_uart_exit();
        } else {
            ///上线处理, 设备管理挂载
            printf("[idle]DEVICE_EVENT_IN\n");
            extern void audio_uart_init();
            audio_uart_init();
            extern int SD_test(void);
            /* SD_test(); */
        }

        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

int idle_app_device_event_handler(int *msg)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
        ret = dev_status_event_filter(msg);///解码设备上下线， 设备挂载等处理
        printf("idle ret %d", ret);
        if (ret == true) {
            if (msg[1] == DEVICE_EVENT_OUT) {
                ///下线处理,设备管理卸载
                printf("[idle]DEVICE_EVENT_OUT\n");
            } else {
                ///上线处理, 设备管理挂载
                printf("[idle]DEVICE_EVENT_IN\n");
            }
        }
        break;
    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        ret = -1;
        break;
    }

    return ret;
}