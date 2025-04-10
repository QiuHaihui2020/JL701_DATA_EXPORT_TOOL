#ifndef APP_TASK_H
#define APP_TASK_H

#include "typedef.h"
#define NULL_VALUE 0

enum {
    APP_POWERON_TASK  = 1,
    APP_POWEROFF_TASK = 2,
    APP_BT_TASK       = 3,
    APP_MUSIC_TASK    = 4,
    APP_FM_TASK       = 5,
    APP_RECORD_TASK   = 6,
    APP_LINEIN_TASK   = 7,
    APP_RTC_TASK      = 8,
    APP_PC_TASK       = 9,
    APP_SPDIF_TASK    = 10,
    APP_IDLE_TASK     = 11,
    APP_WATCH_UPDATE_TASK     = 12,
    APP_RCSP_ACTION_TASK  = 13,
    APP_TASK_MAX_INDEX,
};

enum {
    APP_MSG_SYS_EVENT = Q_EVENT,

    /* 用户自定义消息 */
    APP_MSG_SWITCH_TASK = Q_USER + 1,
    APP_MSG_USER        = Q_USER + 2,

};

extern u8 app_curr_task;
extern u8 app_next_task;
extern u8 app_prev_task;

int app_task_switch_check(u8 app_task);
int app_task_switch_to(u8 app_task, int priv);
int app_task_switch_back();
u8 app_task_exitting();
void app_task_switch_next(void);
void app_task_switch_prev(void);
u8 app_get_curr_task(void);

#endif
