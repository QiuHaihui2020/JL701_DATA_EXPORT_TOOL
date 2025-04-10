#ifndef _APP_CHARGE_H_
#define _APP_CHARGE_H_

#include "typedef.h"
#include "system/event.h"

#define LDO5V_OFF_TYPE_NORMAL_ON        0 //正常拔出开机
#define LDO5V_OFF_TYPE_NORMAL_OFF       1 //正常拔出关机
#define LDO5V_OFF_TYPE_LOWPOWER_OFF     2 //低电关机
#define LDO5V_OFF_TYPE_CHARGESTORE_OFF  3 //智能充电舱关机

struct app_charge_handler {
    int (*handler)(int msg, int type);
};

#define APP_CHARGE_HANDLER(charge_handler, prio) \
    const struct app_charge_handler charge_handler sec(.app_charge_handler.prio)

extern const struct app_charge_handler app_charge_handler_begin[];
extern const struct app_charge_handler app_charge_handler_end[];

#define for_each_app_charge_handler(p) \
    for (p = app_charge_handler_begin; p < app_charge_handler_end; p++)


extern void charge_close_deal(void);
extern void charge_start_deal(void);
extern void ldo5v_keep_deal(void);
extern void charge_full_deal(void);
extern void charge_ldo5v_in_deal(void);
extern void charge_ldo5v_off_deal(void);
extern u8 get_charge_full_flag(void);
extern void app_charge_power_off_keep_mode();


#endif    //_APP_CHARGE_H_
