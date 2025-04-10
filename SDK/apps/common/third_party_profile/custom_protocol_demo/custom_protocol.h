#ifndef _CUSTOM_PROTOCOL_H_
#define _CUSTOM_PROTOCOL_H_

#include "system/includes.h"

void custom_demo_all_init(void);
void custom_demo_all_exit(void);
int custom_demo_adv_enable(u8 enable);
int custom_demo_ble_send(u8 *data, u32 len);
int custom_demo_spp_send(u8 *data, u32 len);

#endif

