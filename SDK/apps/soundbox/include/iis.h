#ifndef __IIS_H
#define __IIS_H

#include "system/event.h"

int iis_start(void);
void iis_stop(void);
int iis_volume_pp(void);
void iis_key_vol_up(void);
void iis_key_vol_down(void);
struct app_mode *app_enter_iis_mode(int arg);
int iis_app_msg_handler(int *msg);
u8 iis_get_status(void);

#endif

