#ifndef __USBD_MIDI_H__
#define __USBD_MIDI_H__

#include "usb/device/usb_stack.h"


void midi_register_rx_notify(void (*func)(const usb_dev usb_id));
u32 midi_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len);
u32 midi_rx_data(const usb_dev usb_id, u8 *buffer);
u32 midi_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
void midi_register(const usb_dev usb_id);
void midi_release(const usb_dev usb_id);

//DEMO API
void PlaySong(void);

#endif
