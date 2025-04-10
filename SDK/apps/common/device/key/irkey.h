#ifndef DEVICE_IRKEY_H
#define DEVICE_IRKEY_H

#include "typedef.h"


#ifndef CONFIG_IRKEY_MAX_NUM
#define CONFIG_IRKEY_MAX_NUM  35
#endif

struct ff00_2_keynum {
    u8 key_value;
    u8 source_value;
};

struct irkey_platform_data {
    u8 enable;
    u8 port;
    u8 num;
    const struct ff00_2_keynum *IRff00_2_keynum;
};

struct irkey_info {
    u8 key_io;
    const struct ff00_2_keynum *irkey_table;
};


u8 ir_get_key_value(void);
int irkey_init(void);

int get_irkey_io();
const struct irkey_platform_data *get_irkey_platform_data();
#endif

