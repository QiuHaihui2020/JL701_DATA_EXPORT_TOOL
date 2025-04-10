#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb_config.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb/device/hid.h"
#include "usb/device/uac_audio.h"
#include "usb/device/descriptor.h"
#include "irq.h"
#include "init.h"
#include "gpio.h"
#include "app_config.h"

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_ENABLE

static u32 host_type[USB_MAX_HW_NUM];
static u32 dev_desc_len[USB_MAX_HW_NUM];
static u32 cfg_desc_len[USB_MAX_HW_NUM];
void usb_reset_host_type(const usb_dev usb_id)
{
    host_type[usb_id] = HOST_TYPE_ERR;
    dev_desc_len[usb_id] = 0;
    cfg_desc_len[usb_id] = 0;
}
u32 usb_get_host_type(const usb_dev usb_id)
{
    return host_type[usb_id];
}
void usb_set_host_type(const usb_dev usb_id, u32 type)
{
    host_type[usb_id] = type;
}

extern void go_mask_usb_updata();
static const u8 user_stirng[] = {
    24,
    0x03,
    'U', 0x00,
    'S', 0x00,
    'B', 0x00,
    'A', 0x00,
    'u', 0x00,
    'd', 0x00,
    'i', 0x00,
    'o', 0x00,
    '1', 0x00,
    '.', 0x00,
    '0', 0x00,
};

#if TCFG_USB_APPLE_DOCK_EN
static const u8 IAP_interface_string[]  = {
    0x1c, 0x03,
    //iAP Interface
    0x69, 0x00, 0x41, 0x00, 0x50, 0x00, 0x20, 0x00, 0x49, 0x00, 0x6e, 0x00, 0x74, 0x00,
    0x65, 0x00, 0x72, 0x00, 0x66, 0x00, 0x61, 0x00, 0x63, 0x00, 0x65, 0x00
};
#endif

static u8 root2_testing;
u32 usb_root2_testing()
{
#if USB_ROOT2
    return root2_testing;
#else
    return 0;
#endif
}
u32 check_ep_vaild(u32 ep)
{
    u32 en = 0;
    switch (ep) {
    case 0:
        en = 1;
        break;
#if TCFG_USB_SLAVE_MSD_ENABLE
    case 1:
        en = 1;
        break;
#endif
#if TCFG_USB_SLAVE_HID_ENABLE
    case 2:
        en = 1;
        break;
#endif
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
    case 3:
        en = 1;
        break;
#endif
    case 4:
        break;
    }
    return en;
}
static u32 setup_endpoint(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{

    u32 ep = LOBYTE(req->wIndex) & 0x0f;
    if (check_ep_vaild(ep) == 0) {
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        return 1;// not zero user handle this request
    }


    return 0;
}
static u32 setup_device(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    switch (req->bRequest) {
    case USB_REQ_GET_DESCRIPTOR:
        switch (HIBYTE(req->wValue)) {
        case USB_DT_STRING:
            switch (LOBYTE(req->wValue)) {
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE || TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
            case SPEAKER_STR_INDEX:
            case MIC_STR_INDEX:
                if (usb_device->bDeviceStates == USB_DEFAULT) {
                    ret = 0;
                } else {
                    usb_set_data_payload(usb_device, req, user_stirng, sizeof(user_stirng));
                    ret = 1;
                }
                break;
#endif
#if TCFG_USB_APPLE_DOCK_EN
            case MSD_STR_INDEX:
                if (apple_mfi_chip_online_lib()) {
                    y_printf("MSD_STR_INDEX \n");
                    usb_set_data_payload(usb_device, req, IAP_interface_string, sizeof(IAP_interface_string));
                    apple_mfi_pass_ready_set_api();
                    extern void apple_usb_msd_wakeup(struct usb_device_t *usb_device);
                    apple_usb_msd_wakeup(usb_device);
                    ret = 1;
                    break;
                }
#endif
            default:
                break;
            }
            break;
        case USB_DT_DEVICE:
#if USB_ROOT2
            if (req->wLength == 0xffff) {
                root2_testing = 1;
            }
#endif
            get_device_descriptor(tx_payload);
            for (int i = 0; i < 8; i++) {
                //为了兼容windows7等老电脑，配置成不同类时应该使用不同pid
                if (usb_device->wDeviceClass & BIT(i)) {
                    tx_payload[11] += i + 1;
                }
            }
            //为了兼容macOS，必须设成复合设备，否则macbook会不认设备
            tx_payload[0x4] = 0xef; //device class should be Multi-interface Function 0xEF
            tx_payload[0x5] = 0x2; //device subclass should be Multi-interface Function 2
            tx_payload[0x6] = 0x1; //device protocol should be Multi-interface Function 1
            usb_set_data_payload(usb_device, req, tx_payload, tx_payload[0]);
            ret = 1;
            if (dev_desc_len[usb_id] == 0) {
                dev_desc_len[usb_id] = req->wLength;
            }
            break;
        case USB_DT_CONFIG:
            if (cfg_desc_len[usb_id] == 0) {
                cfg_desc_len[usb_id] = req->wLength;
            }
            break;
        }
        break;
    case USB_REQ_SET_CONFIGURATION:
        switch (dev_desc_len[usb_id]) {
        case 18:
            if (cfg_desc_len[usb_id] == 255) {
                host_type[usb_id] = HOST_TYPE_WINDOWS;
                log_info("host_type = %d, windows, line:%d\n", host_type[usb_id], __LINE__);
            } else if (cfg_desc_len[usb_id] == 9) {
                host_type[usb_id] = HOST_TYPE_ANDROID;
                log_info("host_type = %d, android, line:%d\n", host_type[usb_id], __LINE__);
            } else {
                host_type[usb_id] = HOST_TYPE_UNKNOW;
                log_info("host_type = %d, unknow, line:%d\n", host_type[usb_id], __LINE__);
            }
            break;
        case 8:
            if (cfg_desc_len[usb_id] == 9) {
                host_type[usb_id] = HOST_TYPE_IOS;
                log_info("host_type = %d, ios, line:%d\n", host_type[usb_id], __LINE__);
            } else {
                host_type[usb_id] = HOST_TYPE_UNKNOW;
                log_info("host_type = %d, unknow, line:%d\n", host_type[usb_id], __LINE__);
            }
            break;
        default:
            host_type[usb_id] = HOST_TYPE_UNKNOW;
            log_info("host_type = %d, unknow, line:%d\n", host_type[usb_id], __LINE__);
            break;
        }
        break;
    case USB_REQ_SET_ADDRESS:
        /* if(req->wLength || req->wIndex){                        */
        /*     ret = 1;                                            */
        /*     usb_set_setup_phase(usb_device, USB_EP0_SET_STALL); */
        /*     dump_setup_request(req);                            */
        /*     log_debug_hexdump((u8 *)req, 8);                    */
        /* }                                                       */
        break;
    }
    return ret;
}

static u32 setup_other(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u8 ack_data[8] = {0};
    u32 ret = 0;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    switch (req->bRequest) {
    case USB_REQ_GET_STATUS:
#if TCFG_TYPEC_EARPHONE_TEST_FILTER
        tx_payload[0] = 0xfe;
        usb_set_data_payload(usb_device, req, tx_payload, 1);
#endif
        if ((req->wValue == 0xfc) && (req->wIndex == 0xfe)) {
            if (req->wLength == sizeof(ack_data)) { //支持该私有命令响应
                log_info("support private update command\n");
                memset(ack_data, 0xff, sizeof(ack_data));
                usb_set_data_payload(usb_device, req, ack_data, sizeof(ack_data));
            } else if (req->wLength == 0x00) {  //准备跳转升级模式
                log_info("run func go_mask_usb_updata()\n");
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                go_mask_usb_updata();
            }
            ret = 1;
        }
        break;
    default:
        ret = 1 ;
        break;
    }
    return ret;
}
static u32 user_setup_filter(struct usb_device_t *usb_device, struct usb_ctrlrequest *request)
{
    // dump_setup_request(request);
    // log_debug_hexdump((u8 *)request, 8);
    u32 ret = 0;
    u32 recip = request->bRequestType & USB_RECIP_MASK;
    switch (recip) {
    case USB_RECIP_DEVICE:
        ret = setup_device(usb_device, request);
        break;
    case USB_RECIP_INTERFACE:
        break;
    case USB_RECIP_ENDPOINT:
        ret = setup_endpoint(usb_device, request);
        break;
    case USB_RECIP_OTHER:
        ret = setup_other(usb_device, request);
        break;
    }
#if 0
    const char *statas[] = {"USB_ATTACHED", "USB_POWERED", "USB_DEFAULT",
                            "USB_ADDRESS", "USB_CONFIGURED", "USB_SUSPENDED"
                           };

    const char *phases[] = {"SETUP", "IN", "OUT",
                            "STALL",
                           };

    printf("state:%s phase: %s", statas[usb_device->bDeviceStates],
           phases[usb_device->bsetup_phase]);
#endif
    return ret;// not zero user handle this request
}
void user_setup_filter_install(struct usb_device_t *usb_device)
{
    usb_set_setup_hook(usb_device, user_setup_filter);
}
#endif
