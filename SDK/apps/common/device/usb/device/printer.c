#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb/usb_config.h"
#include "usb/device/cdc.h"
#include "app_config.h"
#include "os/os_api.h"
#include "printer.h"  //need redefine __u8, __u16, __u32

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[Printer]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_PRINTER_ENABLE

static const u8 printer_desc[] = {
///Interface
    USB_DT_INTERFACE_SIZE,   //Length
    USB_DT_INTERFACE,   //DescriptorType:Inerface
    0x00,   //InterfaceNum:0
    0x00,   //AlternateSetting:0
    0x02,   //NumEndpoint:0
    USB_CLASS_PRINTER,   //InterfaceClass:Printer
    0x01,   //Printers
    0x02,   //InterfaceProtocol
    0x00,   //Interface String
///Endpoint IN
    USB_DT_ENDPOINT_SIZE,
    USB_DT_ENDPOINT,
    PTR_BULK_EP_IN | USB_DIR_IN,
    USB_ENDPOINT_XFER_BULK,
    LOBYTE(MAXP_SIZE_PTR_EPIN), HIBYTE(MAXP_SIZE_PTR_EPIN),
    0x00,
///Endpoint OUT
    USB_DT_ENDPOINT_SIZE,
    USB_DT_ENDPOINT,
    PTR_BULK_EP_OUT,
    USB_ENDPOINT_XFER_BULK,
    LOBYTE(MAXP_SIZE_PTR_EPOUT), HIBYTE(MAXP_SIZE_PTR_EPOUT),
    0x00,
};

struct usb_printer_handle {
    u8 *ep_out_dmabuffer;
    u8 *ep_in_dmabuffer;
    void (*wakeup_handle)(struct usb_device_t *usb_device);

};

struct usb_printer_handle *printer_handle = NULL;
#if USB_MALLOC_ENABLE
#else
struct usb_printer_handle _printer_handle SEC(.usb.data.bss.exchange);
#endif


u32 printer_set_wakeup_handle(void (*handle)(struct usb_device_t *usb_device))
{
    if (printer_handle) {
        printer_handle->wakeup_handle = handle;
        return 1;
    }
    return 0;
}

static void printer_wakeup(struct usb_device_t *usb_device, u32 ep)
{
    if ((printer_handle) && (printer_handle->wakeup_handle)) {
        printer_handle->wakeup_handle(usb_device);
    }
}

static void printer_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    usb_g_ep_config(usb_id, PTR_BULK_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 0, printer_handle->ep_in_dmabuffer, MAXP_SIZE_PTR_EPIN);

    usb_g_ep_config(usb_id, PTR_BULK_EP_OUT, USB_ENDPOINT_XFER_BULK, 1, printer_handle->ep_out_dmabuffer, MAXP_SIZE_PTR_EPOUT);
    usb_g_set_intr_hander(usb_id, PTR_BULK_EP_OUT, printer_wakeup);

    usb_enable_ep(usb_id, PTR_BULK_EP_IN);
}

static void printer_reset_wakeup(struct usb_device_t *usb_device, u32 itf_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
#if USB_ROOT2
    usb_disable_ep(PTR_BULK_EP_IN);
#else
    printer_endpoint_init(usb_device, itf_num);
#endif
}

static const u8 device_id[] = {
    /* 0x00, 0x0D, 0x50, 0x72, 0x69, 0x6E, 0x74, 0x65, 0x72, 0x20, 0x56, 0x30, 0x2E, 0x31, 0x00 */
    0x00, 0x50, 0x4d, 0x46, 0x47, 0x3a, 0x50, 0x6f, 0x73, 0x74, 0x65, 0x4b, 0x50, 0x72, 0x69, 0x6e,
    0x74, 0x65, 0x72, 0x73, 0x3b, 0x4d, 0x44, 0x4c, 0x3a, 0x50, 0x4f, 0x53, 0x54, 0x45, 0x4b, 0x20,
    0x51, 0x38, 0x2f, 0x32, 0x30, 0x30, 0x3b, 0x43, 0x4d, 0x44, 0x3a, 0x50, 0x50, 0x4c, 0x49, 0x3b,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static u32 printer_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    log_info("%s() %x %x", __func__, bRequestType, setup->bRequest);
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (setup->bRequest) {
        case USB_REQ_GET_STATUS:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_GET_INTERFACE:
            tx_len = 1;
            u8 i = 0;
            usb_set_data_payload(usb_device, setup, &i, tx_len);
            break;
        case USB_REQ_SET_INTERFACE:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        default:
            break;
        }
        break;
    case USB_TYPE_CLASS:
        switch (setup->bRequest) {
        case 0:
            tx_len = sizeof(device_id);
            memcpy(tx_payload, device_id, sizeof(device_id));
            usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}

u32 printer_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_debug("printer interface num: %d", *cur_itf_num);
    memcpy(ptr, printer_desc, sizeof(printer_desc));
    ptr[2] = *cur_itf_num;
    if (usb_set_interface_hander(usb_id, *cur_itf_num, printer_itf_hander) != *cur_itf_num) {
        ASSERT(0, "printer set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, printer_reset_wakeup) != *cur_itf_num) {
        ASSERT(0, "printer set interface_reset_hander fail");
    }
    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(printer_desc);
}

u32 printer_register(const usb_dev usb_id)
{
    if (printer_handle == NULL) {
#if USB_MALLOC_ENABLE
        printer_handle = (struct usb_printer_handle *)zalloc(sizeof(struct usb_printer_handle));
        if (printer_handle == NULL) {
            log_error("printer_register err");
            return -1;
        }
#else
        memset(&_printer_handle, 0, sizeof(usb_printer_handle));
        printer_handle = &_printer_handle;
#endif
        log_info("printer_handle = %x", printer_handle);

        printer_handle->ep_out_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_OUT, MAXP_SIZE_PTR_EPOUT);
        printer_handle->ep_in_dmabuffer = usb_alloc_ep_dmabuffer(usb_id, PTR_BULK_EP_IN | USB_DIR_IN, MAXP_SIZE_PTR_EPIN);
    }
    return 0;
}

u32 printer_release(const usb_dev usb_id)
{
    if (printer_handle) {
        if (printer_handle->ep_out_dmabuffer) {
            usb_free_ep_dmabuffer(usb_id, printer_handle->ep_out_dmabuffer);
        }
        if (printer_handle->ep_in_dmabuffer) {
            usb_free_ep_dmabuffer(usb_id, printer_handle->ep_in_dmabuffer);
        }
#if USB_MALLOC_ENABLE
        free(printer_handle);
        printer_handle = NULL;
#else
#endif
    }
    return 0;
}

#endif
