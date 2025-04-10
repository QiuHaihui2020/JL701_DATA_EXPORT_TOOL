#ifndef ADKEY_CONFIG_H
#define ADKEY_CONFIG_H

#include "typedef.h"
#include "key/adkey.h"

struct adkey_res_value {
    u8 key_value;
    u16 res_value;
};

struct adkey_info {
    u8 key_io;
    u8 pull_up_type;                // 0 内部上拉,  1 外部上拉
    u8 pull_up_value;               // 上拉电阻组织(欧)
    u8 long_press_reset_enable;
    u8 long_press_reset_time;
    u16 max_ad_value;               // AD采样最大值
    const struct adkey_res_value *res_table;
};

u8 uuid2adkeyValue(u16 uuid);

#endif
