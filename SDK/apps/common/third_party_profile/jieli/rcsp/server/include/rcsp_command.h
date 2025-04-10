#ifndef __RCSP_COMMAND_H__
#define __RCSP_COMMAND_H__

#include "typedef.h"
#include "app_config.h"

void rcsp_command_send_bt_scan_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi);
void rcsp_command_send_conncecting_bt_status(u8 *addr, u8 status);
void earphone_mute_handler(u8 *other_opt, u32 msec);
void find_decice_tws_connect_handle(u8 flag, u8 *param);
void rcsp_stop_find_device(void *priv);

#endif // __RCSP_COMMAND_H__
