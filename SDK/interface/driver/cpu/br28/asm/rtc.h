
#ifndef __RTC_H__
#define __RTC_H__

#include "typedef.h"
#include "utils/sys_time.h"
#include "rtc/rtc_dev.h"

int rtc_init(const struct rtc_dev_platform_data *arg);
int rtc_ioctl(u32 cmd, u32 arg);
void set_alarm_ctrl(u8 set_alarm);
void write_sys_time(const struct sys_time *curr_time);
void read_sys_time(struct sys_time *curr_time);
void write_alarm(const struct sys_time *alarm_time);
void read_alarm(struct sys_time *alarm_time);

#endif // __RTC_API_H__
