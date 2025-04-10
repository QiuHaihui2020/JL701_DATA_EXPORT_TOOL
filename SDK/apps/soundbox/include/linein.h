#ifndef _LINEIN_H_
#define _LINEIN_H_

#include "system/event.h"

int linein_device_event_handler(struct sys_event *event);
void app_linein_tone_play_start(u8 mix);

int  linein_start(void);
void linein_stop(void);
void linein_key_vol_up();
void linein_key_vol_down();
u8   linein_get_status(void);
int  linein_volume_pp(void);

struct app_mode *app_enter_linein_mode(int arg);
int linein_app_msg_handler(int *msg);
u8 get_need_resume_aux_flag();
void set_close_broadcast_resume_aux_val(u8 val);




#endif


