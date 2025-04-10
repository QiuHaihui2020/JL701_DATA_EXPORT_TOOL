#ifndef  __USBD_PRINTER_H__
#define  __USBD_PRINTER_H__

u32 printer_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
u32 printer_set_wakeup_handle(void (*handle)(struct usb_device_t *usb_device));
u32 printer_register(const usb_dev usb_id);
u32 printer_release(const usb_dev usb_id);



#endif  /*PRINTER_H*/
