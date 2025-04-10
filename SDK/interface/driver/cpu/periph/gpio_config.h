#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include "generic/typedef.h"
#include "gpio.h"

struct gpio_cfg_item {
    u8 gpio;
    u8 mode;
    u8 hd;
};

u8 uuid2gpio(u16 uuid);

int gpio_config_init();
int gpio_config_uninit();
void gpio_enter_sleep_config();
void gpio_exit_sleep_config();

#endif
