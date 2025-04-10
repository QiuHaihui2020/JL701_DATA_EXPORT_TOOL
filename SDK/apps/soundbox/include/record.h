#ifndef _RECORD_H_
#define _RECORD_H_

#include "system/event.h"


struct app_mode *app_enter_record_mode(int arg);
int record_app_msg_handler(int *msg);
int record_device_msg_handler(int *msg);

#endif
