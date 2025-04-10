#ifndef _GFPS_PLATFORM_API_H_
#define _GFPS_PLATFORM_API_H_

#include "system/includes.h"

extern void *gfps_app_ble_hdl;
extern int gfps_all_init(void);
extern int gfps_all_exit(void);
extern void gfps_ble_profile_init(void);
extern int gfps_bt_ble_adv_enable(u8 enable);
extern void gfps_set_model_id(uint8_t *model_id);
extern void gfps_set_anti_spoofing_public_key(char *public_key);
extern void gfps_set_anti_spoofing_private_key(char *private_key);
extern void gfps_is_tws_master_callback_register(bool (*handler)(void));
extern int gfps_message_deal_handler(int id, int opcode, u8 *data, u32 len);
extern void gfps_message_callback_register(int (*handler)(int id, int opcode, u8 *data, u32 len));

#define GFPS_ANC_TRANSPARENT_MODE   0x80
#define GFPS_OFF_MODE               0x20
#define GFPS_ANC_MODE               0x08
#define GFPS_ANC_ALL_MODE           (GFPS_ANC_TRANSPARENT_MODE | GFPS_OFF_MODE | GFPS_ANC_MODE)
extern void gfps_hearable_controls_enable(u8 enable);
extern void gfps_hearable_controls_update(u8 display_flag, u8 settable_flag, u8 current_state);

extern int gfps_tws_data_deal(u8 *data, int len);
extern bool get_gfps_pair_state(void);
extern void set_gfps_pair_state(u8 state);
extern void gfps_set_battery_ui_enable(uint8_t enable);
extern void gfps_set_pair_mode(void *priv);
extern void gfps_all_info_recover(u8 name_reset);
extern void gfps_sibling_sync_info();
extern int gfps_disconnect(void *addr);
extern void gfps_adv_interval_set(u16 value);
extern void gfps_sibling_data_send(u8 type, u8 *data, u16 len);
extern int gfps_ble_adv_enable(u8 enable);
extern void gfps_battery_update(void);
extern u8 gfps_rfcomm_connect_state_get(void);
extern void gfps_rfcomm_connect_state_set(u8 state);

extern void gfps_sync_info_to_new_master(void);

#endif

