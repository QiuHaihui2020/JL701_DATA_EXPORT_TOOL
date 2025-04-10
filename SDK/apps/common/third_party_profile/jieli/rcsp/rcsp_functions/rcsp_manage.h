// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _RCSP_MANAGE_H
#define _RCSP_MANAGE_H

#include "ble_user.h"

void JL_rcsp_event_to_user(u32 type, u8 event, u8 *msg, u8 size);
u8 get_rcsp_connect_status(void);
void rcsp_user_event_ble_handler(ble_state_e ble_status, u8 flag);

#endif
