#ifndef _APP_BLE_SPP_API_H_
#define _APP_BLE_SPP_API_H_

#include "system/includes.h"
#include "btstack/bluetooth.h"
#include "btstack/avctp_user.h"
#include "list.h"
#include "timer.h"
#include "spp_user.h"
#include "btstack/le/le_common_define.h"
#include "btcontroller_modules.h"
#include "btstack/btstack_typedef.h"

/**********************************************
                    adv_type
**********************************************/

#define APP_ADV_IND             (0)    /*Connectable and scannable undirected advertising*/
#define APP_ADV_DIRECT_IND      (1)    /*Connectable high duty cycle directed advertising */
#define APP_ADV_SCAN_IND        (2)    /*Scannable undirected advertising*/
#define APP_ADV_NONCONN_IND     (3)    /*Non connectable undirected advertising*/
#define APP_ADV_DIRECT_IND_LOW  (4)    /*Connectable low duty cycle directed advertising*/


/**********************************************
                    adv_channel
**********************************************/
#define APP_ADV_CHANNEL_37   (BIT(0))
#define APP_ADV_CHANNEL_38   (BIT(1))
#define APP_ADV_CHANNEL_39   (BIT(2))
#define APP_ADV_CHANNEL_ALL  (BIT(0)|BIT(1)|BIT(2))


extern int app_ble_set_adv_param(void *_hdl, u32 adv_interval, u8 adv_type, u8 adv_channel);
/**********************************************
                    ble adv
**********************************************/
extern int app_ble_adv_data_set(void *_hdl, u8 *data, u8 length);
extern int app_ble_rsp_data_set(void *_hdl, u8 *data, u8 length);
extern int app_ble_adv_enable(void *_hdl, u8 en);

// ble adv 5.0
extern int app_ble_adv_data_set_5_0(void *_hdl, le_set_ext_adv_data_t *param);
extern int app_ble_rsp_data_set_5_0(void *_hdl, le_set_ext_adv_data_t *param);
extern int app_ble_set_adv_param_5_0(void *_hdl, le_set_ext_adv_param_t *param);
extern int app_ble_adv_enable_5_0(void *_hdl, le_set_ext_adv_en_t *param);


extern int app_ble_set_mac_addr(void *_hdl, void *addr);
extern int app_ble_adv_address_type_set(void *_hdl, u8 address_type);
extern int app_ble_adv_skip(void *_hdl, u8 skip_cnt);
extern int app_ble_adv_state_get(void *_hdl);


/**********************************************
                    ble callback
**********************************************/
typedef void (*app_ble_packet_handler_t)(void *_hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
typedef uint16_t (*app_ble_att_read_callback_t)(void *_hdl, uint16_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
typedef int (*app_ble_att_write_callback_t)(void *_hdl, uint16_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
typedef void (*app_ble_sm_event_callback_t)(void *_hdl, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);


extern int app_ble_att_read_callback_register(void *_hdl, app_ble_att_read_callback_t read_callback);
extern int app_ble_att_write_callback_register(void *_hdl, app_ble_att_write_callback_t write_callback);
extern int app_ble_att_server_packet_handler_register(void *_hdl, app_ble_packet_handler_t handler);
extern int app_ble_hci_event_callback_register(void *_hdl, app_ble_packet_handler_t handler);
extern int app_ble_l2cap_packet_handler_register(void *_hdl, app_ble_packet_handler_t handler);
extern int app_ble_sm_event_callback_register(void *_hdl, app_ble_sm_event_callback_t callback);



/**********************************************
                    ble module
**********************************************/
extern int app_ble_init(void);
extern int app_ble_exit(void);
extern void *app_ble_hdl_alloc(void);
extern int app_ble_hdl_free(void *_hdl);
extern int app_ble_sm_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en);
extern int app_ble_profile_set(void *_hdl, const uint8_t *db);
extern int app_ble_set_filter_con_handle(void *_hdl, u16 con_handle);
extern int app_ble_disconnect(void *_hdl);
extern ble_cmd_ret_e app_ble_att_send_data(void *_hdl, u16 att_handle, u8 *data, u16 len, att_op_type_e att_op_type);
extern int app_ble_check_hdl_vaild(void *_hdl);
extern u16 app_ble_get_hdl_con_handle(void *_hdl);
extern int app_ble_att_vaild_len_get(void *_hdl);
extern int app_ble_no_profile_flag_set(void *_hdl, u8 flag);

extern u8 *app_ble_adv_addr_get(void *_hdl);
extern u8 *app_ble_local_mac_addr_get(void *_hdl);
extern u8 *app_ble_remote_mac_addr_get(void *_hdl);
extern int app_ble_common_core_data_size(void);
extern int app_ble_common_core_data_get(u8 *data);
extern int app_ble_common_core_data_set(u8 *data);
extern int app_ble_hdl_core_data_size(void);
extern int app_ble_hdl_core_data_get(void *_hdl, u8 *data);
extern int app_ble_hdl_core_data_set(void *_hdl, u8 *data);
extern int app_ble_adv_handle_get(void *_hdl);
// 如果BLE句柄设置了UUID，可以直接一次性同步所有设置了UUID的BLE句柄
extern int app_ble_hdl_uuid_set(void *_hdl, u32 uuid);
extern int app_ble_all_sync_data_size(void);
extern int app_ble_all_sync_data_get(u8 *sync_buf);
extern int app_ble_all_sync_data_set(u8 *sync_buf, int buf_len);


/**********************************************
                    spp module
**********************************************/
extern int app_spp_init(void);
extern int app_spp_exit(void);
extern void *app_spp_hdl_alloc(u8 local_id);
extern int app_spp_hdl_free(void *_hdl);
extern int app_spp_data_send(void *_hdl, u8 *buf, u16 len);
extern int app_spp_set_filter_remote_addr(void *_hdl, u8 *remote_addr);
extern int app_spp_clean_filter_remote_addr(void *_hdl);
extern int app_spp_disconnect(void *_hdl);
extern int app_spp_check_hdl_vaild(void *_hdl);
extern u8 *app_spp_get_hdl_remote_addr(void *_hdl);
extern int app_spp_hdl_core_data_size(void);
extern int app_spp_hdl_core_data_get(void *_hdl, u8 *data);
extern int app_spp_hdl_core_data_set(void *_hdl, u8 *data);

// 如果SPP句柄设置了UUID，可以直接一次性同步所有设置了UUID的BLE句柄
extern int app_spp_hdl_uuid_set(void *_hdl, u32 uuid);
extern int app_spp_all_sync_data_size(void);
extern int app_spp_all_sync_data_get(u8 *sync_buf);
extern int app_spp_all_sync_data_set(u8 *sync_buf, int buf_len);


/**********************************************
                    spp callback
**********************************************/
typedef void (*app_spp_recieve_callback_t)(void *hdl, void *remote_addr, u8 *buf, u16 len);
typedef void (*app_spp_state_callback_t)(void *hdl, void *remote_addr, u8 state);
typedef void (*app_spp_send_wakeup_callback_t)(void *hdl);

extern int app_spp_recieve_callback_register(void *_hdl, app_spp_recieve_callback_t callback);
extern int app_spp_state_callback_register(void *_hdl, app_spp_state_callback_t callback);
extern int app_spp_wakeup_callback_register(void *_hdl, app_spp_send_wakeup_callback_t callback);

extern int app_ble_att_handle_enable(void *_hdl, uint16_t start_handle, uint16_t end_handle, uint8_t enable);
extern int app_ble_att_handle_clear(void *_hdl);


/**********************************************
                    common
**********************************************/
#define APP_BLE_CONNECTION_COMPLETE                 (0x01)
#define APP_BLE_DISCONNECTION_COMPLETE              (0x02)
#define APP_SPP_CONNECTION_COMPLETE                 (0x81)
#define APP_SPP_DISCONNECTION_COMPLETE              (0x82)
typedef void (*app_state_update_callback_t)(void *_hdl, uint8_t state, uint8_t *packet, uint16_t size);
extern int app_ble_state_update_callback_regitster(app_state_update_callback_t callback);
extern int app_spp_state_update_callback_regitster(app_state_update_callback_t callback);

#endif  // _APP_BLE_SPP_API_H_

