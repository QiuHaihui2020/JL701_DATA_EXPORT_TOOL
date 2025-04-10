#ifndef APP_RTC_H
#define APP_RTC_H
#include "rtc/rtc_dev.h"

void set_rtc_sw(void);
void set_rtc_pos(void);
void set_rtc_up(void);
void set_rtc_down(void);

struct app_mode *app_enter_rtc_mode(int arg);
int rtc_app_msg_handler(int *msg);

struct rtc_dev_data {
    u8 port;
    u8 edge;  //0 leading edge, 1 falling edge
    u8 port_en;
    u8 rtc_ldo;
    u8 clk_res;
};

#endif
