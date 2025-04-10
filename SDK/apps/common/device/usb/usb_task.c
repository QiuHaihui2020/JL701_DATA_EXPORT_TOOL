#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "system/event.h"
#include "os/os_api.h"
#include "app_main.h"

#if  USB_PC_NO_APP_MODE == 0
#include "app_task.h"
#endif
#include "usb/usb_config.h"
#include "usb/usb_task.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_host.h"
#include "usb/otg.h"
#include "usb/host/uac_host.h"

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if (TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0)
#include "dev_multiplex_api.h"
#endif

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#endif


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = TCFG_OTG_USB_DEV_EN,
    .slave_online_cnt = TCFG_OTG_SLAVE_ONLINE_CNT,
    .slave_offline_cnt = TCFG_OTG_SLAVE_OFFLINE_CNT,
    .host_online_cnt = TCFG_OTG_HOST_ONLINE_CNT,
    .host_offline_cnt = TCFG_OTG_HOST_OFFLINE_CNT,
#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
    .detect_mode = TCFG_OTG_MODE | OTG_DET_DM_ONLY,
#else
    .detect_mode = TCFG_OTG_MODE,
#endif
    .detect_time_interval = TCFG_OTG_DET_INTERVAL,
};
#endif

extern void usb_cdc_background_run(const usb_dev usbfd);
extern int usb_cdc_background_standby(const usb_dev usbfd);

u8 msd_in_task;
u8 msd_run_reset;
static OS_SEM msg_sem;

static void usb_task(void *p)
{
    int ret;
    int msg[16];
    int from, event;
    char *usb_msg;
    usb_dev usb_id;
    //cppcheck-suppress unusedVariable
    struct device *dev;
    u8 disk_msg = 0;

#if USB_EPBUF_ALLOC_USE_LBUF
    extern int usb_memory_init(void);
    usb_memory_init();
#endif
#if TCFG_OTG_MODE
    extern int usb_otg_init(const struct dev_node * node,  void *arg);
    usb_otg_init(NULL, &otg_data);
#endif
#if TCFG_HOST_AUDIO_ENABLE
    uac_host_init();
#endif

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }
        switch (msg[1]) {
        case USBSTACK_OTG_MSG:
            from = msg[2];
            event = msg[3];
            usb_msg = (char *)msg[4];
            if (from == DEVICE_EVENT_FROM_OTG) {
                usb_id = usb_msg[2] - '0';
                if (usb_msg[0] == 's') {
#if TCFG_USB_SLAVE_ENABLE
                    if (event == DEVICE_EVENT_IN) {
#if   USB_PC_NO_APP_MODE
                        usb_start(usb_id);
#else
#if TCFG_USB_CDC_BACKGROUND_RUN
                        ret = usb_cdc_background_standby(usb_id);
#else
                        ret = usb_standby(usb_id);
#endif
                        //如果需要切入pc模式，将otg事件分发给app mode
                        if (ret) {
                            usb_driver_event_to_user(from, event, usb_msg);
                        }
#endif
                    } else if (event == DEVICE_EVENT_OUT) {
#if   USB_PC_NO_APP_MODE
                        usb_stop(usb_id);
#else
                        usb_driver_event_to_user(from, event, usb_msg);
#endif
                    }
#endif
                } else if (usb_msg[0] == 'h') {
#if TCFG_USB_HOST_ENABLE
                    if (event == DEVICE_EVENT_IN) {
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
                        mult_usb_mount_before(usb_id);
#endif
                        //cppcheck-suppress unreadVariable
                        ret = usb_host_mount(usb_id,
                                             TCFG_USB_HOST_MOUNT_RETRY,
                                             TCFG_USB_HOST_MOUNT_RESET,
                                             TCFG_USB_HOST_MOUNT_TIMEOUT);
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
                        mult_usb_online_mount_after(usb_id, ret);
#endif
                    } else if (event == DEVICE_EVENT_OUT) {
                        usb_host_unmount(usb_id);
                    }
#endif
                }
            }
            break;

#if TCFG_USB_SLAVE_ENABLE
        case USBSTACK_START:
            usb_id = msg[2];
#if TCFG_USB_CDC_BACKGROUND_RUN
            usb_stop(usb_id);
#endif
            usb_start(usb_id);
            os_sem_post(&msg_sem);
            break;

        case USBSTACK_PAUSE:
            usb_id = msg[2];
            usb_pause(usb_id);
#if TCFG_USB_CDC_BACKGROUND_RUN
            usb_cdc_background_run(usb_id);
#endif
            os_sem_post(&msg_sem);
            break;

        case USBSTACK_STOP:
            usb_id = msg[2];
            usb_stop(usb_id);
            os_sem_post(&msg_sem);
            break;

#if TCFG_USB_SLAVE_MSD_ENABLE
        case USBSTACK_MSD_RUN:
            msd_in_task = 1;
#if TCFG_USB_APPLE_DOCK_EN
            apple_mfi_link((void *)msg[2]);
#else
            USB_MassStorage((void *)msg[2]);
#endif
            if (msd_run_reset) {
                msd_reset((struct usb_device_t *)msg[2], 0);
                msd_run_reset = 0;
            }
            msd_in_task = 0;
            break;
#endif

#if TCFG_USB_CDC_BACKGROUND_RUN
        case USBSTACK_CDC_BACKGROUND:
            usb_id = msg[2];
            usb_cdc_background_run(usb_id);
            os_sem_post(&msg_sem);
            break;
#endif
#endif

#if TCFG_USB_HOST_ENABLE
        case USBSTACK_HOST_MSG:
            from = DEVICE_EVENT_FROM_USB_HOST;
            event = msg[2];
            usb_msg = (char *)msg[3];
            if (!strncmp(usb_msg, "udisk", 5)) {
                if (event == DEVICE_EVENT_IN) {
                    dev = dev_open(usb_msg, NULL);
                    if (dev) {
                        dev_close(dev);
                        disk_msg = 1;
                    } else {
                        disk_msg = 0;
                    }
                } else if (event == DEVICE_EVENT_OUT) {
                } else if (event == DEVICE_EVENT_CHANGE) {
                }
                //只有U盘尝试open成功才将usb host事件分发给app mode
                //offline事件同理，尝试打开不成功则不分发事件
                if (disk_msg) {
                    usb_driver_event_to_user(from, event, usb_msg);
                }
            } else if (!strncmp(usb_msg, "audio", 5)) {
#if TCFG_HOST_AUDIO_ENABLE
                if (event == DEVICE_EVENT_IN) {
                    log_info("uac_host IN %s", usb_msg);
                    uac_host_mount(usb_msg[5] - '0');
                } else if (event == DEVICE_EVENT_OUT) {
                    log_info("uac_host OUT %s", usb_msg);
                    uac_host_unmount(usb_msg[5] - '0');
                }
#endif
            }
            break;
        case USBSTACK_HOST_REMOUNT:
            usb_id = ((int *)msg[2])[0];
            ret = usb_host_remount(usb_id, TCFG_USB_HOST_MOUNT_RETRY, TCFG_USB_HOST_MOUNT_RESET, TCFG_USB_HOST_MOUNT_TIMEOUT, 0);
            ((int *)msg[2])[1] = ret;
            os_sem_post(&msg_sem);
            break;

#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
        case USBSTACK_HOST_MOUNT_AFTER:
            usb_id = ((int *)msg[2])[0];
            ret = ((int *)msg[2])[1];
            mult_usb_online_mount_after(usb_id, ret);
            os_sem_post(&msg_sem);
            break;

        case USBSTACK_HOST_UNMOUNT_AFTER:
            usb_id = ((int *)msg[2])[0];
            ret = ((int *)msg[2])[1];
            mult_usb_mount_offline(usb_id);
            os_sem_post(&msg_sem);
            break;
#endif
#endif

        default:
            break;
        }
    }

#if TCFG_HOST_AUDIO_ENABLE
    uac_host_deinit();
#endif
}

static int usb_stack_init(void)
{
    r_printf("%s()", __func__);
    int err;
    os_sem_create(&msg_sem, 0);
    err = task_create(usb_task, NULL, USB_TASK_NAME);
    if (err != OS_NO_ERR) {
        r_printf("usb_msd task creat fail %x\n", err);
    }
    return 0;
}
late_initcall(usb_stack_init);

void usb_message_to_stack(int msg, void *arg, u8 sync)
{
    //先将sem清零0，否则当sync为0时不会pend，另一边post会使counter累加
    os_sem_set(&msg_sem, 0);
    os_taskq_post_msg(USB_TASK_NAME, 2, msg, arg);
    if (sync) {
        os_sem_pend(&msg_sem, 200);
    }
}


#endif
