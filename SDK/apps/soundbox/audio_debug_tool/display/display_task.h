#ifndef _DISPLAY_TASK_H_
#define _DISPLAY_TASK_H_
#include "typedef.h"

int oled_display_init();
int oled_display_exit();
int oled_dispaly_task_post(int msg, int *arg);

enum {
    OLED_DISPLAY_NULL = 0,
    OLED_DISPLAY_SD_ON,
    OLED_DISPLAY_SD_OFF,
    OLED_DISPLAY_CH,
    OLED_DISPLAY_LEN,
    OLED_DISPLAY_BAUD,
    OLED_DISPLAY_RUN_TIPS,
    OLED_DISPLAY_LOST,
    OLED_DISPLAY_SET_NEXT,
    OLED_DISPLAY_FMT,       /*自动嗅探出的载荷格式，取值见 RAW_FMT_xxx*/



};

#endif