#ifndef BT_SLIENCE_DETECT_H
#define BT_SLIENCE_DETECT_H

#include "generic/typedef.h"

enum {
    BT_SLIENCE_NO_DETECTING,
    BT_SLIENCE_NO_ENERGY,
    BT_SLIENCE_HAVE_ENERGY,
};

void bt_start_a2dp_slience_detect(u8 *bt_addr, int ingore_packet_num);

void bt_stop_a2dp_slience_detect(u8 *bt_addr);

int bt_slience_detect_get_result(u8 *bt_addr);

void bt_reset_a2dp_slience_detect();

bool bt_a2dp_slience_detecting();

int bt_slience_get_detect_addr(u8 *bt_addr);

#endif
