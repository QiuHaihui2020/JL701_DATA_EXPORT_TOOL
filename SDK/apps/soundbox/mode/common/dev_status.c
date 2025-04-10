#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_status.data.bss")
#pragma data_seg(".dev_status.data")
#pragma const_seg(".dev_status.text.const")
#pragma code_seg(".dev_status.text")
#endif
#include "app_main.h"
#include "app_default_msg_handler.h"
#include "dev_status.h"
#include "dev_update.h"
#include "dev_multiplex_api.h"
#include "dev_manager.h"
#include "usb/host/usb_host.h"
#include "usb/usb_task.h"
#include "undef_func.h"
#include "app_config.h"

#if RCSP_MODE
#include "rcsp.h"
#endif

usb_dev g_usb_id = (usb_dev) - 1;
//*----------------------------------------------------------------------------*/
/**@brief    设备上线预处理
   @param
   			 event：设备事件
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_add_prepare(int *msg)
{
    int ret = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        //传入0 sd0 1：sd1
        mult_sd_online_mount_before(!!(msg[0] - DRIVER_EVENT_FROM_SD0), g_usb_id);
        break;
//#if TCFG_UDISK_ENABLE || TCFG_HOST_AUDIO_ENABLE
//    case DEVICE_EVENT_FROM_OTG:
//        ///host mount
//        mult_usb_mount_before(g_usb_id);
//
//        ret = usb_host_mount(g_usb_id,
//                             MOUNT_RETRY,
//                             MOUNT_RESET,
//                             MOUNT_TIMEOUT);
//        break;
//#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备上线后处理
   @param
   			 event：设备事件
   			 err：设备上线错误码
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_add_after(int *msg, int err)
{
    int ret = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        //传入0 sd0 1：sd1
        mult_sd_online_mount_after(!!(msg[0] - DRIVER_EVENT_FROM_SD0), g_usb_id, err);
        break;
//#if TCFG_UDISK_ENABLE
//    case DEVICE_EVENT_FROM_OTG:
//        mult_usb_online_mount_after(g_usb_id, err);
//        break;
//#endif
#if TCFG_UDISK_ENABLE
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    case DEVICE_EVENT_FROM_USB_HOST:
        int msg2[4];
        msg2[0] = g_usb_id;
        msg2[1] = err;
        usb_message_to_stack(USBSTACK_HOST_MOUNT_AFTER, msg2, 1);
        break;
#endif
#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备下线预处理
   @param
   			 event：设备事件
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_del_prepare(int *msg)
{
    int ret = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        mult_sd_offline_before((char *)msg[2], g_usb_id);
        break;
//#if TCFG_UDISK_ENABLE || TCFG_HOST_AUDIO_ENABLE
//    case DEVICE_EVENT_FROM_OTG:
//        ///host umount
//        usb_host_unmount(g_usb_id);
//        break;
//#endif
    }
    return ret;
}
//*----------------------------------------------------------------------------*/
/**@brief    设备下线后处理
   @param
   			 event：设备事件
   			 err：设备下线错误码
   @return
   @note	 根据特殊情景，这里可以处理一些例如io复用处理
*/
/*----------------------------------------------------------------------------*/
static int dev_manager_del_after(int *msg, int err)
{
    int ret = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        break;
//#if TCFG_UDISK_ENABLE
//    case DEVICE_EVENT_FROM_OTG:
//        mult_usb_mount_offline(g_usb_id);
//        g_usb_id = (usb_dev) - 1;
//        break;
//#endif
#if TCFG_UDISK_ENABLE
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    case DEVICE_EVENT_FROM_USB_HOST:
        int msg2[4];
        msg2[0] = g_usb_id;
        msg2[1] = err;
        usb_message_to_stack(USBSTACK_HOST_UNMOUNT_AFTER, msg2, 1);
        break;
#endif
#endif
    }
    return ret;
}



//*----------------------------------------------------------------------------*/
/**@brief    设备事件响应处理
   @param
   			 event：设备事件
   @return
   @note	 设备上线和下线会动态添加到设备管理链表中，
   			 通过dev_manager系列接口可以进行设备操作，例如：例如查找、检查在线等
*/
/*----------------------------------------------------------------------------*/
int dev_status_event_filter(int *msg)
{
    int ret = true;
    int err = 0;
    char *add = NULL;
    char *del = NULL;
    static u32 sdx_mount_err_cnt = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        printf("DEVICE_EVENT_FROM_SD%d\n", msg[0] - DRIVER_EVENT_FROM_SD0);
        if (msg[1] == DEVICE_EVENT_IN) {
            ///上线处理, 设备管理挂载
            printf("DEVICE_EVENT_IN\n");
            add = (char *)msg[2];
        } else {
            ///下线处理,设备管理卸载
            printf("DEVICE_EVENT_OUT\n");
            del = (char *)msg[2];
        }
        break;
#if TCFG_UDISK_ENABLE
    case DEVICE_EVENT_FROM_USB_HOST:
        char *usb_msg = (char *)msg[2];
        printf("DEVICE_EVENT_FROM_USB_HOST %s\n", usb_msg);
        g_usb_id = usb_msg[strlen(usb_msg) - 1] - '0';
        if (!strncmp(usb_msg, "udisk", 5)) {
            ///是host, 准备挂载设备, usb host mount/umout
            if (msg[1] == DEVICE_EVENT_IN) {
                ///加入到设备管理列表
                printf("DEVICE_EVENT_IN\n");
                add = usb_msg;
            } else {
                printf("DEVICE_EVENT_OUT\n");
                del = usb_msg;
            }
        }
        break;
#endif
    default:
        ret = false;
        break;
    }

    ////dev manager add/del操作
    if (add) {
        err = dev_manager_add_prepare(msg);
        if (!err) {
            err = dev_manager_add(add);
            if (!err) {
                sdx_mount_err_cnt = 0;
#if (MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN)
                music_task_dev_online_start(add);
#endif
#if (TCFG_DEV_UPDATE_IF_NOFILE_ENABLE == 0)
                if (app_in_mode(APP_MODE_PC) == false) {
                    ///检查设备升级
                    dev_update_check(add);
                }
#endif/* TCFG_DEV_UPDATE_IF_NOFILE_ENABLE*/
            } else {
#if (defined(TCFG_SD_ALWAY_ONLINE_ENABLE) && (TCFG_SD_ALWAY_ONLINE_ENABLE == DISABLE))
                sdx_mount_err_cnt++;
                if (sdx_mount_err_cnt < 3) {         // 挂载失败会重试 3 次。由于重新挂载的问题，SD IN/OUT 的消息不成对
                    if (((!strncmp(add, "sd0", strlen("sd0"))) || (!strncmp(add, "sd1", strlen("sd1"))))) {
                        dev_manager_del(add); // 添加设备失败，删除
                        sd_dev_ops.ioctl(NULL, IOCTL_RESET_DET_STATUS, (u32)event->arg - DRIVER_EVENT_FROM_SD0);   // 复位 sd 的检测状态
                        printf("mount sd err, try mount again %d \n", sdx_mount_err_cnt);
                    }
                }
#endif
                ret = false;
            }
        } else {
            ret = false;
        }
        dev_manager_add_after(msg, err);
    }
    if (del) {
        dev_manager_del_prepare(msg);
        dev_manager_del(del);
        dev_manager_del_after(msg, 0);
        if (dev_manager_get_total(1) == 0 && app_in_mode(APP_MODE_MUSIC)) {        //这里防止没有设备在线了,还不退出音乐模式
            app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        }
    }
#if (RCSP_MODE && RCSP_DEVICE_STATUS_ENABLE && TCFG_DEV_MANAGER_ENABLE)
    rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO));
#endif
    return ret;
}


