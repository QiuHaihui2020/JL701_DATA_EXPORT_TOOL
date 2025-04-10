#ifndef _SWIFT_PAIR_API_H_
#define _SWIFT_PAIR_API_H_

#include "system/includes.h"

extern void swift_pair_ble_name_set(const char *ble_name);
extern void swift_pair_adv_interval_set(u16 interval);
extern void swift_pair_pair_time_ms_set(u32 msec);
extern void swift_pair_class_of_device_set(u8 *device_class);
extern void swift_pair_paired_type_set(u8 type);
extern void swift_pair_ble_addr_set(u8 *ble_addr);

extern void swift_pair_app_disconnect(void);

extern void swift_pair_enter_pair_mode(void);
extern void swift_pair_exit_pair_mode(void);

extern void swift_pair_all_init(void);
extern void swift_pair_all_exit(void);

#endif


