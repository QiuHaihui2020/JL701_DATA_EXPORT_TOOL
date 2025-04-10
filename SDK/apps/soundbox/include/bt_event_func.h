#ifndef BT_EVENT_FUNC_H
#define BT_EVENT_FUNC_H


void bt_send_keypress(u8 key);
void bt_send_pair(u8 en);
void bt_init_ok_search_index(void);
void bt_status_init_ok(void);
void bt_read_remote_name(u8 status, u8 *addr, u8 *name);
void user_get_bt_music_info(u8 type, u32 time, u8 *info, u16 len);
void spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size);
int bt_get_battery_value();
void bt_fast_test_api(void);
void bt_dut_api(u8 param);
void bt_fix_fre_api(u8 fre);
void bt_fix_txrx_api(u8 mode, u8 *mac_addr, u8 fre, u8 packet_type, u16 payload);
void bt_updata_fix_rx_result();
void bt_bredr_exit_dut_mode();
void bt_hci_event_inquiry(struct bt_event *bt);
void bt_discovery_and_connectable_using_loca_mac_addr(u8 inquiry_scan_en, u8 page_scan_en);
void bt_hci_event_disconnect(struct bt_event *bt);
void bt_hci_event_connection_timeout(struct bt_event *bt);
u8 bt_sco_state(void);
void bt_music_player_time_timer_deal(u8 en);

int bt_app_msg_handler(int *msg);
void bt_close_discoverable_and_connectable(void);
void ble_standard_dut_test_init(void);

#endif
