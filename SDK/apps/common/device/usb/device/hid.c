#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "os/os_api.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb_config.h"

#include "app_config.h"

#if TCFG_USB_SLAVE_USER_HID
#undef TCFG_USB_SLAVE_HID_ENABLE
#define TCFG_USB_SLAVE_HID_ENABLE           0
#endif

#if TCFG_USB_SLAVE_HID_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
static const u8 sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x01,                      // NumEndpoint
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x00, 0x01,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0,//LOW(ReportLength)
    0, //HIGH(ReportLength)

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDIN), HIBYTE(MAXP_SIZE_HIDIN),// Maximum packet size
    HID_INTR_INTERVAL,          // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms */

//@Endpoint Descriptor:
    /* USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_OUT | HID_EP_OUT,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDOUT), HIBYTE(MAXP_SIZE_HIDOUT),// Maximum packet size
    HID_INTR_INTERVAL,          // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms */
};

static const u8 sHIDReportDesc[] = {
    USAGE_PAGE(1, CONSUMER_PAGE),
    USAGE(1, CONSUMER_CONTROL),
    COLLECTION(1, APPLICATION),

    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0x01),

    USAGE(1, VOLUME_INC),
    USAGE(1, VOLUME_DEC),
    USAGE(1, MUTE),
    USAGE(1, PLAY_PAUSE),
    USAGE(1, SCAN_NEXT_TRACK),
    USAGE(1, SCAN_PREV_TRACK),
    USAGE(1, FAST_FORWARD),
    USAGE(1, STOP),

    USAGE(1, TRACKING_INC),
    USAGE(1, TRACKING_DEC),
    USAGE(1, STOP_EJECT),
    USAGE(1, VOLUME),
    USAGE(2, BALANCE_LEFT),
    USAGE(2, BALANCE_RIGHT),
    USAGE(1, PLAY),
    USAGE(1, PAUSE),

    REPORT_SIZE(1, 0x01),
    REPORT_COUNT(1, 0x10),
    INPUT(1, 0x42),
    END_COLLECTION,

};

static const u8 sHIDReportDesc_IOS[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x02,        //   Report Count (2)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xCF,        //   Usage (0xCF)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x0c,        //   Report Count (12)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x04,        //   Report ID (4)
    0x09, 0x00,        //   Usage (Unassigned)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x26,        //   Report Count (38)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x85, 0x05,        //   Report ID (5)
    0x09, 0x00,        //   Usage (Unassigned)
    0x95, 0x22,        //   Report Count (34)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    // 51 bytes
};

static u32 get_hid_report_desc_len(u32 index)
{
    u32 len = 0;
    if (usb_get_host_type(0) == HOST_TYPE_IOS) {
        len = sizeof(sHIDReportDesc_IOS);
    } else {
        len = sizeof(sHIDReportDesc);
    }
    return len;
}
static void *get_hid_report_desc(u32 index)
{
    u8 *ptr  = NULL;
    if (usb_get_host_type(0) == HOST_TYPE_IOS) {
        ptr = (u8 *)sHIDReportDesc_IOS;
    } else {
        ptr = (u8 *)sHIDReportDesc;
    }
    return ptr;
}

static void hid_output_report_handler(u8 *buf, u32 len)
{
    log_info("hid output");
    log_debug_hexdump(buf, len);

}

static u8 *hid_ep_in_dma;
static u8 *hid_ep_out_dma;

static u32 hid_tx_data(struct usb_device_t *usb_device, const u8 *buffer, u32 len)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    return usb_g_intr_write(usb_id, HID_EP_IN, buffer, len);
}
static void hid_rx_data(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 rx_len = usb_g_intr_read(usb_id, ep, NULL, MAXP_SIZE_HIDOUT, 0);
    /* hid_tx_data(usb_device, hid_ep_out_dma, rx_len); */
    hid_output_report_handler(hid_ep_out_dma, rx_len);
}

static void hid_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_g_ep_config(usb_id, HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, hid_ep_in_dma, MAXP_SIZE_HIDIN);
    usb_enable_ep(usb_id, HID_EP_IN);

    /* usb_g_set_intr_hander(usb_id, HID_EP_OUT, hid_rx_data); */
    /* usb_g_ep_config(usb_id, HID_EP_OUT, USB_ENDPOINT_XFER_INT, 1, hid_ep_out_dma, MAXP_SIZE_HIDOUT); */
}

u32 hid_register(const usb_dev usb_id)
{
    hid_ep_in_dma = usb_alloc_ep_dmabuffer(usb_id, HID_EP_IN | USB_DIR_IN, MAXP_SIZE_HIDIN);

    /* hid_ep_out_dma = usb_alloc_ep_dmabuffer(usb_id, HID_EP_OUT, MAXP_SIZE_HIDOUT); */
    return 0;
}

void hid_release(const usb_dev usb_id)
{
    if (hid_ep_in_dma) {
        usb_free_ep_dmabuffer(usb_id, hid_ep_in_dma);
        hid_ep_in_dma = NULL;
    }
    //if (hid_ep_out_dma) {
    //    usb_free_ep_dmabuffer(usb_id, hid_ep_out_dma);
    //    hid_ep_out_dma = NULL;
    //}
    return ;
}

static void hid_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, HID_EP_IN);
#else
    hid_endpoint_init(usb_device, itf);
#endif
}

static u32 hid_recv_output_report(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[EP0_SETUP_LEN];
    usb_read_ep0(usb_id, read_ep, MIN(sizeof(read_ep), setup->wLength));
    ret = USB_EP0_STAGE_SETUP;

    //wValue = (repot_type << 8 | report_id)
    if (HIBYTE(setup->wValue) == 0x02) {
        //set output report
        hid_output_report_handler(read_ep, setup->wLength);
    } else if (HIBYTE(setup->wValue) == 0x03) {
        //set feature report
    }

    return ret;
}

static u32 hid_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            switch (HIBYTE(req->wValue)) {
            case USB_HID_DT_HID:
                tx_payload = (u8 *)sHIDDescriptor + USB_DT_INTERFACE_SIZE;
                tx_len = 9;
                tx_payload = usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                tx_payload[7] = LOBYTE(get_hid_report_desc_len(req->wIndex));
                tx_payload[8] = HIBYTE(get_hid_report_desc_len(req->wIndex));
                break;
            case USB_HID_DT_REPORT:
                hid_endpoint_init(usb_device, req->wIndex);
                tx_len = get_hid_report_desc_len(req->wIndex);
                tx_payload = get_hid_report_desc(req->wIndex);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                break;
            }// USB_REQ_GET_DESCRIPTOR
            break;
        case USB_REQ_SET_DESCRIPTOR:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                //只有一个interface 没有Alternate
                if (req->wValue == 0) { //alt 0
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (req->wValue || (req->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        }//bRequest @ USB_TYPE_STANDARD
        break;

    case USB_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_GET_IDLE:
            tx_len = 1;
            tx_payload[0] = 0;
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            break;
        case USB_REQ_SET_REPORT:
            usb_set_setup_recv(usb_device, hid_recv_output_report);
            break;
        case USB_REQ_GET_REPORT:
            //wValue = (repot_type << 8 | report_id)
            tx_len = req->wLength;
            if (HIBYTE(req->wValue) == 0x1) {
                //get input report
            } else if (HIBYTE(req->wValue) == 0x3) {
                //get feature report
            }
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            break;
        }//bRequest @ USB_TYPE_CLASS
        break;
    }
    return 0;
}

u32 hid_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_debug("hid interface num:%d\n", *cur_itf_num);
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    u8 *_ptr = ptr;
    memcpy(ptr, sHIDDescriptor, sizeof(sHIDDescriptor));
    ptr[2] = *cur_itf_num;
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        ptr[9 + 9 + 6] = HID_INTR_INTERVAL_FS;
        ptr[9 + 9 + 7 + 6] = HID_INTR_INTERVAL_FS;
    }
#endif
    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_itf_hander) != *cur_itf_num) {
        ASSERT(0, "hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_reset) != *cur_itf_num) {
        ASSERT(0, "hid set interface_reset_hander fail");
    }

    ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(0));
    ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(0));
    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(sHIDDescriptor) ;
}

void hid_key_handler(struct usb_device_t *usb_device, u32 hid_key)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 key_buf[4] = {0};
    if (usb_get_ep_status(usb_id, HID_EP_IN)) {
        return;
    }

    if (usb_get_host_type(usb_id) == HOST_TYPE_IOS) {
        key_buf[0] = 0x01;  //report id
        key_buf[1] = LOBYTE(hid_key);
        key_buf[2] = HIBYTE(hid_key);
        hid_tx_data(usb_device, key_buf, 3);
        os_time_dly(2);
        key_buf[1] = 0;
        key_buf[2] = 0;
        hid_tx_data(usb_device, key_buf, 3);
    } else {
        key_buf[0] = LOBYTE(hid_key);
        key_buf[1] = HIBYTE(hid_key);
        hid_tx_data(usb_device, key_buf, 2);
        os_time_dly(2);
        key_buf[0] = 0;
        key_buf[1] = 0;
        hid_tx_data(usb_device, key_buf, 2);
    }
}

void hid_key_handler_send_one_packet(struct usb_device_t *usb_device, u32 hid_key)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 key_buf[4] = {0};
    if (usb_get_ep_status(usb_id, HID_EP_IN)) {
        return;
    }

    if (usb_get_host_type(usb_id) == HOST_TYPE_IOS) {
        key_buf[0] = 0x01;
        key_buf[1] = LOBYTE(hid_key);
        key_buf[2] = HIBYTE(hid_key);
        hid_tx_data(usb_device, key_buf, 3);
    } else {
        key_buf[0] = LOBYTE(hid_key);
        key_buf[1] = HIBYTE(hid_key);
        hid_tx_data(usb_device, key_buf, 2);
    }
}

struct hid_button {
    u8 report_id;
    u8 button1: 1;
    u8 button2: 1;
    u8 button3: 1;
    u8 no_button: 5;
    u8 X_axis;
    u8 Y_axis;
};
struct hid_button hid_key;
void hid_test(struct usb_device_t *usb_device)
{
    static u32 tx_count = 0;

    hid_key_handler(usb_device, BIT(tx_count));
    tx_count ++;
    if (BIT(tx_count) > USB_AUDIO_PAUSE) {
        tx_count = 0;
    }
}
#endif
