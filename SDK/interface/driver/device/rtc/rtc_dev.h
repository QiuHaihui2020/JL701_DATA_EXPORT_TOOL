#ifndef __RTC_DEV_H__
#define __RTC_DEV_H__

#include "typedef.h"
#include "utils/sys_time.h"

enum RTC_CLK {
    CLK_SEL_32K = 1,
    CLK_SEL_BTOSC_DIV1 = 2,
    CLK_SEL_12M = 2,
    CLK_SEL_BTOSC_DIV2 = 3,
    CLK_SEL_24M = 3,
    CLK_SEL_LRC = 4,
};

enum RTC_SEL {
    HW_RTC,
    VIR_RTC,
};

enum {
    RTC_UNACCESSIBLE = 0,
    RTC_ACCESSIBLE_TIME_UNRELIABLE,
    RTC_ACCESSIBLE_TIME_RELIABLE,
};

struct rtc_dev_platform_data {
    const struct sys_time *default_sys_time;
    const struct sys_time *default_alarm;
    enum RTC_CLK rtc_clk;
    enum RTC_SEL rtc_sel;
    /* u8 clk_sel; */
    void (*cbfun)(u8);
    u8 x32xs;
    u8 trim_t;  //单位：分钟
};

#define RTC_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct rtc_dev_platform_data data = {

#define RTC_DEV_PLATFORM_DATA_END()  \
    .x32xs = 0, \
};

extern const struct device_operations rtc_dev_ops;

/* int rtc_init(const struct rtc_dev_platform_data *arg); */
/* int rtc_ioctl(u32 cmd, u32 arg); */
/* void rtc_wakup_source(); */
/* void set_alarm_ctrl(u8 set_alarm); */
/* void write_sys_time(struct sys_time *curr_time); */
/* void read_sys_time(struct sys_time *curr_time); */
/* void write_alarm(struct sys_time *alarm_time); */
/* void read_alarm(struct sys_time *alarm_time); */

bool leapyear(u16 year);
u16 year_to_day(u16 year);
u16 month_to_day(u16 year, u8 month);
void day_to_ymd(u16 day, struct sys_time *sys_time);
u16 ymd_to_day(const struct sys_time *time);
u8 caculate_weekday_by_time(struct sys_time *r_time);

int rtc_port_pr_read(u8 port);
int rtc_port_pr_out(u8 port, u8 value);
int rtc_port_pr_dir(u8 port, u8 dir);
int rtc_port_pr_die(u8 port, u8 die);
int rtc_port_pr_pu(u8 port, u8 value);
int rtc_port_pr_pu1(u8 port, u8 value);
int rtc_port_pr_pd(u8 port, u8 value);
int rtc_port_pr_pd1(u8 port, u8 value);
int rtc_port_pr_hd0(u8 port, u8 value);
int rtc_port_pr_hd1(u8 port, u8 value);


struct rtc_dev_fun_cfg {
    void (*rtc_init)(const struct rtc_dev_platform_data *arg);
    void (*rtc_get_time)(struct sys_time *curr_time);
    void (*rtc_set_time)(struct sys_time *curr_time);
    void (*rtc_get_alarm)(struct sys_time *alarm_time);
    void (*rtc_set_alarm)(struct sys_time *alarm_time);
    void (*rtc_time_dump)(void);
    void (*rtc_alm_en)(u8 set_alarm);
    u32(*get_rtc_alm_en)(void);

};

void rtc_config_init(const struct rtc_dev_platform_data *arg);
void rtc_read_time(struct sys_time *curr_time);
void rtc_write_time(struct sys_time *curr_time);
void rtc_read_alarm(struct sys_time *alarm_time);
void rtc_write_alarm(struct sys_time *alarm_time);
void rtc_debug_dump(void);
void rtc_alarm_en(u8 set_alarm);
u32 rtc_get_alarm_en(void);

#endif // __RTC_API_H__
