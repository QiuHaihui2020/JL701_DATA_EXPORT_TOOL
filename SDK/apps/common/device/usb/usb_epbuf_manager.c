#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb_config.h"
#include "init.h"
#include "app_config.h"
#include "lbuf.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_HOST_ENABLE || TCFG_USB_SLAVE_ENABLE

#ifndef USB_HW_20
#if (FUSB_MODE == 0)
#error "USB1.1 IP is not support high-speed mode"
#endif
#endif

#define USB_DMA_BUF_ALIGN	(8)

//附加的字节数用于ep rx接收crc（占用4字节，dual buf则8字节）以及分配lbuf头信息
//占用。lbuf头结构体20字节，假设需要8对齐，需要占用24字节，另外分配出来的头+buf
//总长度也需要8对齐，in/out ep各调用1次所以要留2份。
#define TO_ALIGN(size, align)       (((size) & ((align) - 1)) ? ((align) - ((size) & ((align) - 1))) : 0)
#define _APPEND(size, align, eps)   ((size) + TO_ALIGN(size, align) + 24 * (eps) + 8)
#define APPEND(size, eps)           _APPEND(size, USB_DMA_BUF_ALIGN, eps)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~ usb slave begin    ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if TCFG_USB_SLAVE_MSD_ENABLE
#define     MSD_DMA_SIZE            APPEND(MAXP_SIZE_BULKOUT * 2, 1)
#else
#define     MSD_DMA_SIZE            0
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
#define     HID_DMA_SIZE            APPEND(64, 2)
#else
#define     HID_DMA_SIZE            0
#endif

#if TCFG_USB_CUSTOM_HID_ENABLE
#define     CUSTOM_HID_DMA_SIZE     APPEND(64 * 2, 2)
#else
#define     CUSTOM_HID_DMA_SIZE     0
#endif

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
#if USB_EP_PROTECT
#define     AUDIO_SPK_DMA_SIZE      APPEND(1024, 1)
#else
#define     AUDIO_SPK_DMA_SIZE      APPEND(192, 1)
#endif
#else
#define     AUDIO_SPK_DMA_SIZE      0
#endif

#if TCFG_USB_SLAVE_AUDIO_MIC_ENABLE
#define     AUDIO_MIC_DMA_SIZE      APPEND(192, 1)
#else
#define     AUDIO_MIC_DMA_SIZE      0
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#if CDC_INTR_EP_ENABLE
#define     CDC_DMA_SIZE            APPEND(64 * 4, 3) //bulk: 64x2+64 + intr: 64
#else
#define     CDC_DMA_SIZE            APPEND(64 * 3, 2) //bulk: 64x2+64
#endif
#else
#define     CDC_DMA_SIZE            0
#endif

#if TCFG_USB_SLAVE_MIDI_ENABLE
#define     MIDI_DMA_SIZE           APPEND(64 * 2, 2)
#else
#define     MIDI_DMA_SIZE           0
#endif

#if TCFG_USB_SLAVE_PRINTER_ENABLE
#define     PRINTER_DMA_SIZE        APPEND(64 * 2, 2)
#else
#define     PRINTER_DMA_SIZE        0
#endif

#define USB_DEVICE_DMA_BUF_MAX_SIZE (HID_DMA_SIZE + AUDIO_SPK_DMA_SIZE + AUDIO_MIC_DMA_SIZE + MSD_DMA_SIZE + CDC_DMA_SIZE  + CUSTOM_HID_DMA_SIZE + MIDI_DMA_SIZE + PRINTER_DMA_SIZE)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~ usb slave end      ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~ usb host begin     ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if TCFG_UDISK_ENABLE
#define     H_MSD_DMA_SIZE          APPEND(MAXP_SIZE_BULKOUT * 2 + MAXP_SIZE_BULKIN, 2)
#else
#define     H_MSD_DMA_SIZE          0
#endif

#if TCFG_HID_HOST_ENABLE
#define     H_HID_DMA_SIZE          APPEND(64, 1)
#else
#define     H_HID_DMA_SIZE          0
#endif

#if TCFG_AOA_ENABLE
#define     H_AOA_DMA_SIZE          APPEND(64 + 64, 2)
#else
#define     H_AOA_DMA_SIZE          0
#endif

#if TCFG_ADB_ENABLE
#define     H_ADB_DMA_SIZE          APPEND(64 + 64, 2)
#else
#define     H_ADB_DMA_SIZE          0
#endif

#if TCFG_HOST_AUDIO_ENABLE
#define     H_AUDIO_DMA_SIZE        APPEND(192 + 192, 2)
#else
#define     H_AUDIO_DMA_SIZE        0
#endif

#define USB_HOST_DMA_BUF_MAX_SIZE  (H_MSD_DMA_SIZE + H_HID_DMA_SIZE + H_AOA_DMA_SIZE + H_ADB_DMA_SIZE + H_AUDIO_DMA_SIZE)
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~ usb host end       ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifndef USB_DMA_BUF_MAX_SIZE
#if (USB_DEVICE_DMA_BUF_MAX_SIZE >= USB_HOST_DMA_BUF_MAX_SIZE)
#define USB_DMA_BUF_MAX_SIZE    USB_DEVICE_DMA_BUF_MAX_SIZE
#else
#define USB_DMA_BUF_MAX_SIZE    USB_HOST_DMA_BUF_MAX_SIZE
#endif
#endif//USB_DMA_BUF_MAX_SIZE

static u8 ep0_dma_buffer[USB_MAX_HW_NUM][64 + 4] __attribute__((aligned(4)));




#if USB_EPBUF_ALLOC_USE_LBUF

//除了lbuf分配的每份buf之和，还有留一些给lbuf自身
static u8 usb_dma_buf[USB_DMA_BUF_MAX_SIZE + 48] SEC(.usb.data.bss.exchange) __attribute__((aligned(USB_DMA_BUF_ALIGN)));
struct lbuff_head *usb_dma_lbuf = NULL;

int usb_memory_init(void)
{
    usb_dma_lbuf = lbuf_init(usb_dma_buf, sizeof(usb_dma_buf), USB_DMA_BUF_ALIGN, 0);
    log_info("%s() total dma size %d @%x", __func__, sizeof(usb_dma_buf), usb_dma_buf);
    return 0;
}
//early_initcall(usb_memory_init);

void *usb_epbuf_alloc(const usb_dev usb_id, u32 ep, u32 size)
{
    if ((ep & 0xf) == 0) {
        return ep0_dma_buffer[usb_id];
    }
    if (ep & USB_DIR_IN) {
        size += 8;  //fusb的rx，需要额外4字节存放接收的crc，使用ping-pong buffer就需要两份，即4 x 2
    }
    return lbuf_alloc(usb_dma_lbuf, size);
}

void usb_epbuf_free(const usb_dev usb_id, void *buf)
{
    if (buf == ep0_dma_buffer[usb_id]) {
        return;
    }
    lbuf_free(buf);
}




#else /* USB_EPBUF_ALLOC_USE_LBUF */

void *usb_epbuf_alloc(const usb_dev usb_id, u32 ep, u32 size)
{
    if ((ep & 0xf) == 0) {
        return ep0_dma_buffer[usb_id];
    }
    if (ep & USB_DIR_IN) {
        size += 8;  //fusb的rx，需要额外4字节存放接收的crc，使用ping-pong buffer就需要两份，即4 x 2
    }
    return dma_malloc(size);
}

void usb_epbuf_free(const usb_dev usb_id, void *buf)
{
    if (buf == ep0_dma_buffer[usb_id]) {
        return;
    }
    dma_free(buf);
}

#endif

#endif
