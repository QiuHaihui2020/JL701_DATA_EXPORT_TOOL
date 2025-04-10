#ifndef APP_MODE_IDLE_H
#define APP_MODE_IDLE_H



enum {
    IDLE_MODE_CHARGE,
    IDLE_MODE_PLAY_POWEROFF,
    IDLE_MODE_WAIT_POWEROFF,
    IDLE_MODE_WAIT_DEVONLINE,
};


extern unsigned char  goto_poweroff_first_flag;
extern int idle_app_msg_handler(int *msg);
extern struct app_mode *app_enter_idle_mode(int arg);
extern void idle_key_poweron_deal(int msg);
extern void power_off_deal(int msg);
extern void power_off_instantly();
#endif
