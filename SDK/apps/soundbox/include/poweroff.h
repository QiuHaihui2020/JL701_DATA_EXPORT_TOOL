#ifndef APP_POWEROFF_H
#define APP_POWEROFF_H



enum poweroff_reason {
    POWEROFF_NORMAL,        //普通关机,默认播放关机提示音
    POWEROFF_RESET,         //系统复位
    POWEROFF_POWER_KEEP,    //充电仓没电情况下的关机
    POWEROFF_NORMAL_TWS,    //TWS同时关机,默认播放关机提示音
};



void sys_enter_soft_poweroff(enum poweroff_reason reason);

void sys_auto_shut_down_disable(void);

void sys_auto_shut_down_enable(void);

#endif
