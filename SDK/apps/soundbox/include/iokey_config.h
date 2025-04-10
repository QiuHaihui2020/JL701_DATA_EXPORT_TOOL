#ifndef IOKEY_CONFIG_H
#define IOKEY_CONFIG_H

#include "typedef.h"

struct iokey_info {
    u8 key_value;                   //键值
    u8 key_io;                      //GPIO
    u8 detect;                      //0 低电平按下, 1 高电平按下
    u8 long_press_reset_enable;     //长按复位使能
    u8 long_press_reset_time;       //长按复位时间(s)
};

u8 uuid2keyValue(u16 uuid);

#endif
