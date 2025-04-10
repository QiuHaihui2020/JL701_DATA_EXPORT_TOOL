#ifndef DEVICE_ADKEY_H
#define DEVICE_ADKEY_H

#include "typedef.h"
#include "gpadc.h"

#ifndef CONFIG_ADKEY_MAX_NUM
#define CONFIG_ADKEY_MAX_NUM  20
#endif


struct adkey_platform_data {
    u8 enable;
    u8 adkey_pin;
    u8 extern_up_en;                //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u32 ad_channel;
    u8  long_press_enable;
    u8  long_press_time;
    u16 ad_value[CONFIG_ADKEY_MAX_NUM];
    u8  key_value[CONFIG_ADKEY_MAX_NUM];
};

struct adkey_rtcvdd_platform_data {
    u8 enable;
    u8 adkey_pin;
    u8  adkey_num;
    u32 ad_channel;
    u32 extern_up_res_value;                //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u16 res_value[CONFIG_ADKEY_MAX_NUM]; 	//电阻值, 从 [大 --> 小] 配置
    u8  key_value[CONFIG_ADKEY_MAX_NUM];
};

//ADKEY API:
int adkey_init(void);
u8 ad_get_key_value(void);

//RTCVDD ADKEY API:
int adkey_rtcvdd_init(void);
u8 adkey_rtcvdd_get_key_value(void);

bool is_adkey_press_down();
int get_adkey_io();

const struct adkey_platform_data *get_adkey_platform_data();

#endif

