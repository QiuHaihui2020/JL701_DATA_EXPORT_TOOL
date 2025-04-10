/**
 * @file task_pc.c
 * @brief 从机模式
 * @author chenrixin@zh-jieli.com
 * @version 1.0.0
 * @date 2020-02-29
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "device/sdmmc/sdmmc.h"

#include "app_charge.h"
#include "asm/charge.h"
#include "app_main.h"

#if TCFG_USB_SLAVE_ENABLE
#if  USB_PC_NO_APP_MODE == 0
#include "app_task.h"
#endif
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/usb_task.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif

#if (TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0)
#include "dev_multiplex_api.h"
#endif

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#endif

#if TCFG_CFG_TOOL_ENABLE
#include "cfg_tool_cdc.h"
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
#include "usb/device/custom_hid.h"
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
#include "usb/device/midi.h"
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
#include "usb/device/printer.h"
#endif

#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
#include "asm/chargestore.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


extern u8 msd_in_task;
extern u8 msd_run_reset;


#if TCFG_USB_SLAVE_MSD_ENABLE
static void usb_msd_wakeup(struct usb_device_t *usb_device)
{
    os_taskq_post_msg(USB_TASK_NAME, 2, USBSTACK_MSD_RUN, usb_device);
}
static void usb_msd_reset_wakeup(struct usb_device_t *usb_device, u32 itf_num)
{
    if (msd_in_task) {
        msd_run_reset = 1;
    } else {
        msd_reset(usb_device, 0);
    }
}
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
static void usb_cdc_wakeup(struct usb_device_t *usb_device)
{
    //回调函数在中断里，正式使用不要在这里加太多东西阻塞中断，
    //或者先post到任务，由任务调用cdc_read_data()读取再执行后续工作
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 buf[64] = {0};
    u32 rlen;

    /* log_debug("cdc rx hook"); */
    //cppcheck-suppress unreadVariable
    rlen = cdc_read_data(usb_id, buf, 64);

    /* put_buf(buf, rlen);//固件三部测试使用 */
    /* cdc_write_data(usb_id, buf, rlen);//固件三部测试使用 */

#if TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)
    cfg_tool_data_from_cdc(buf, rlen);
#endif

}
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
static void custom_hid_rx_handler(void *priv, u8 *buf, u32 len)
{
    printf("%s,%d,\n", __func__, __LINE__);
    put_buf(buf, len);
    //custom_hid_tx_data(0, buf, len);
}
#endif /* #if TCFG_USB_CUSTOM_HID_ENABLE */

#if TCFG_USB_SLAVE_MIDI_ENABLE
int midi_rx_buffer_read(const usb_dev usb_id)
{
    u8 buf[64];
    u32 rlen;

    rlen = midi_rx_data(usb_id, buf);
    log_debug("%s() %d\n", __func__, usb_id);
    log_debug_hexdump((u8 *)buf, rlen);
    return 0;
}
void midi_rx_handler(const usb_dev usb_id)
{
    //回调函数在中断中被调用，因此最好post到线程里面再读数据
    int msg[4];
    msg[0] = (int)midi_rx_buffer_read;
    msg[1] = 1;
    msg[2] = (int)usb_id;
    os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, msg);
}
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
static int printer_rx_read(const usb_dev usb_id)
{
    u8 buf[64] = {0};
    u32 rlen;

    rlen = usb_g_bulk_read(usb_id, PTR_BULK_EP_OUT, buf, sizeof(buf), 0);
    log_debug("%s(), recv_len = %d\n", __func__, rlen);
    log_debug_hexdump((u8 *)buf, rlen);

    //usb_g_bulk_write(usb_id, PTR_BULK_EP_IN, buf, rlen);
    return 0;
}
static void printer_rx_handler(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    int msg[4];

    msg[0] = (int)printer_rx_read;
    msg[1] = 1;
    msg[2] = (int)usb_id;
    os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, msg);
}
#endif

void usb_start(const usb_dev usbfd)
{
    u32 class = 0;

#if TCFG_USB_SLAVE_MSD_ENABLE
    class |= MASSSTORAGE_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    class |= SPEAKER_CLASS;
#endif
#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    class |= MIC_CLASS;
#endif
#if TCFG_USB_SLAVE_HID_ENABLE
    class |= HID_CLASS;
#endif
#if TCFG_USB_SLAVE_CDC_ENABLE
    class |= CDC_CLASS;
#endif
#if TCFG_USB_CUSTOM_HID_ENABLE
    class |= CUSTOM_HID_CLASS;
#endif
#if TCFG_USB_SLAVE_MIDI_ENABLE
    class |= MIDI_CLASS;
#endif
#if TCFG_USB_SLAVE_PRINTER_ENABLE
    class |= PRINTER_CLASS;
#endif
    g_printf("USB_DEVICE_CLASS_CONFIG:%x", class);
    usb_device_mode(usbfd, class);


#if TCFG_USB_SLAVE_MSD_ENABLE
    //没有复用时候判断 sd开关
    //复用时候判断是否参与复用
#if (!TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_SD0_ENABLE)\
     ||(TCFG_SD0_ENABLE && TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_DM_MULTIPLEX_WITH_SD_PORT != 0)
    msd_register_disk("sd0", NULL);
#endif

#if (!TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_SD1_ENABLE)\
     ||(TCFG_SD1_ENABLE && TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_DM_MULTIPLEX_WITH_SD_PORT != 1)
    msd_register_disk("sd1", NULL);
#endif

#if TCFG_NOR_FAT
    msd_register_disk("fat_nor", NULL);
#endif

#if TCFG_NANDFLASH_DEV_ENABLE
    msd_register_disk("nandflash_ftl", NULL);
#endif

#if TCFG_VIR_UDISK_ENABLE
    msd_register_disk("vir_udisk0", NULL);
#endif

    msd_set_wakeup_handle(usb_msd_wakeup);
    msd_set_reset_wakeup_handle(usb_msd_reset_wakeup);
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
    cdc_set_wakeup_handler(usb_cdc_wakeup);
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
    custom_hid_set_rx_hook(NULL, custom_hid_rx_handler);
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
    midi_register_rx_notify(midi_rx_handler);
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
    printer_set_wakeup_handle(printer_rx_handler);
#endif
}

void usb_pause(const usb_dev usbfd)
{
    log_info("usb pause");

    usb_sie_disable(usbfd);

#if TCFG_USB_SLAVE_MSD_ENABLE
    if (msd_set_wakeup_handle(NULL)) {
        msd_unregister_all();
    }
#endif


    usb_device_mode(usbfd, 0);
}

void usb_stop(const usb_dev usbfd)
{
    log_info("usb stop");

    usb_pause(usbfd);

    usb_sie_close(usbfd);
}

int usb_standby(const usb_dev usbfd)
{
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    usb_otg_suspend(usbfd, OTG_KEEP_STATE);
    mult_sdio_suspend();
    usb_pause(usbfd);
    mult_sdio_resume();
#else
    usb_pause(usbfd);
#endif
    return 1;
}

#if TCFG_USB_CDC_BACKGROUND_RUN
void usb_cdc_background_run(const usb_dev usbfd)
{
    g_printf("CDC is running in the background");
    usb_device_mode(usbfd, CDC_CLASS);
    cdc_set_wakeup_handler(usb_cdc_wakeup);
}

int usb_cdc_background_standby(const usb_dev usbfd)
{
    int ret = 0;
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    mult_sdio_suspend();
    usb_cdc_background_run(usbfd);
#else
#if TCFG_PC_ENABLE
#if TWFG_APP_POWERON_IGNORE_DEV
    if ((jiffies_to_msecs(jiffies) < TWFG_APP_POWERON_IGNORE_DEV)\
        && (!app_in_mode(APP_MODE_IDLE))) {
        usb_cdc_background_run(usbfd);
    } else
#endif
    {
        usb_pause(usbfd);
        ret = 1;
    }
#else
    usb_cdc_background_run(usbfd);
#endif
#endif
    return ret;
}
#endif

int pc_device_event_handler(int *msg)
{
    if (msg[0] != DEVICE_EVENT_FROM_OTG) {
        return false;
    }

    int switch_app_case = false;
    const char *usb_msg = (const char *)msg[2];

    if (usb_msg[0] == 's') {
        log_debug("usb event : %d DEVICE_EVENT_FROM_OTG %s", msg[1], usb_msg);
        if (msg[1] == DEVICE_EVENT_IN) {
            log_info("usb %c online", usb_msg[2]);
            switch_app_case = 1;
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            log_info("usb %c offline", usb_msg[2]);
            if (true == app_in_mode(APP_MODE_PC)) {
                switch_app_case = 2;
            } else {
                usb_message_to_stack(USBSTACK_STOP, 0, 1);
            }
        }
    }

    return switch_app_case;
}

#if ((TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE) \
     && TCFG_CHARGESTORE_PORT == IO_PORT_DP)
static void otg_sof_check_before_hook()
{
    log_debug("sof_check_before");
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 0);
    chargestore_api_stop();
}

static void otg_sof_check_after_hook()
{
    log_debug("sof_check_after");
    chargestore_api_restart();
    p33_io_wakeup_enable(TCFG_CHARGESTORE_PORT, 1);
}

static int otg_sof_check_hook_register()
{
    //需要在otg定时器启动之前（即devices_init()之前）注册钩子函数
    usb_otg_sof_check_register_hooks(otg_sof_check_before_hook,
                                     otg_sof_check_after_hook);
    return 0;
}
early_initcall(otg_sof_check_hook_register);
#endif

#endif
