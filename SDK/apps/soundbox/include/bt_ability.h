#ifndef __BT_ABILITY_H__
#define __BT_ABILITY_H__

extern void *get_the_other_device(u8 *addr);
extern void bt_action_a2dp_play(void *device, u8 *bt_addr);
extern void bt_action_a2dp_pause(void *device, u8 *bt_addr);
#endif
