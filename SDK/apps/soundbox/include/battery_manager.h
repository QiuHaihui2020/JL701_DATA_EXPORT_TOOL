#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include "typedef.h"

enum battery_msg {
    BAT_MSG_LOWPOWER = 0x20,
    BAT_MSG_CHARGE_START,
    BAT_MSG_CHARGE_FULL,
    BAT_MSG_CHARGE_CLOSE,
    BAT_MSG_CHARGE_ERR,
    BAT_MSG_CHARGE_LDO5V_OFF,

    POWER_EVENT_POWER_NORMAL,
    POWER_EVENT_POWER_WARNING,
    POWER_EVENT_POWER_LOW,
    POWER_EVENT_POWER_CHANGE,
    POWER_EVENT_SYNC_TWS_VBAT_LEVEL,
    POWER_EVENT_POWER_CHARGE,
};




extern void batmgr_send_msg(enum battery_msg msg, int arg);
u8 get_cur_battery_level(void);












#endif
