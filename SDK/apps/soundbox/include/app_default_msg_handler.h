#ifndef APP_DEFAULT_MSG_HANDLER_H
#define APP_DEFAULT_MSG_HANDLER_H


#include "system/includes.h"
#include "system/event.h"


u8 get_sys_aduio_mute_statu(void);
void app_default_msg_handler(int *msg);
void app_common_key_msg_handler(int *msg);
void app_common_device_event_handler(int *msg);































#endif

