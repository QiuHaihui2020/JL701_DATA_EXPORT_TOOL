#ifndef _PC_H_
#define _PC_H_

#include "system/event.h"
#include "music/music_player.h"


struct app_mode *app_enter_pc_mode(int arg);
int pc_app_msg_handler(int *msg);


#endif
