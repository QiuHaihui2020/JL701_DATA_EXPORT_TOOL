#ifndef DEVICE_IOKEY_H
#define DEVICE_IOKEY_H

#include "typedef.h"


#ifndef CONFIG_IOKEY_MAX_NUM
#define CONFIG_IOKEY_MAX_NUM  15
#endif
// enum key_connect_way {
// ONE_PORT_TO_LOW, 		//按键一个端口接低电平, 另一个端口接IO
// ONE_PORT_TO_HIGH,		//按键一个端口接高电平, 另一个端口接IO
// DOUBLE_PORT_TO_IO,		//按键两个端口接IO
// };


#define ONE_PORT_TO_LOW 		0 		//按键一个端口接低电平, 另一个端口接IO
#define ONE_PORT_TO_HIGH		1 		//按键一个端口接高电平, 另一个端口接IO
#define DOUBLE_PORT_TO_IO		2		//按键两个端口接IO
#define CUST_DOUBLE_PORT_TO_IO	3


struct one_io_key {
    u8 port;
};

struct two_io_key {
    u8 in_port;
    u8 out_port;
};

union key_type {
    struct one_io_key one_io;
    struct two_io_key two_io;
};

struct iokey_port {
    union key_type key_type;
    u8 connect_way;
    u8 key_value;
};

struct iokey_platform_data {
    u8 enable;
    u8 num;
    u8 long_press_enable;
    u8 long_press_time;
    u8 long_press_port;
    u8 long_press_level;
    const struct iokey_port *port;
};

//IOKEY API:
int iokey_init(void);
u8 io_get_key_value(void);
bool is_iokey_press_down();
int get_iokey_power_io();
const struct iokey_platform_data *get_iokey_platform_data();


#endif

