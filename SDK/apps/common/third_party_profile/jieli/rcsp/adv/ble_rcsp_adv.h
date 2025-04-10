// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_RCSP_ADV_H
#define _BLE_RCSP_ADV_H

#include <stdint.h>
#include "bt_common.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)

/**
 * 开启BLE广播
 */
void bt_ble_rcsp_adv_enable(void);
/**
 * 关闭BLE广播
 */
void bt_ble_rcsp_adv_disable(void);
/**
 * 关闭BLE广播定时器
 */
void bt_ble_rcsp_adv_disable_timer(void);
u8 get_ble_adv_notify(void);
void set_ble_adv_notify(u8 en);
u8 adv_tws_both_in_charge_box(u8 type);
int rcsp_make_set_rsp_data(void);
int rcsp_make_set_adv_data(void);
void bt_ble_init_do(void);
int bt_ble_adv_ioctl(u32 cmd, u32 priv, u8 mode);
void set_connect_flag(u8 value);
u8 get_connect_flag(void);
void upay_ble_mode_enable(u8 enable);
int upay_ble_adv_data_set(void);
int upay_ble_send_data(const uint8_t *data, u16 len);
void upay_ble_regiest_recv_handle(void (*handle)(const uint8_t *data, u16 len));
void adv_seq_vaule_sync(void);

u8 get_ble_adv_modify(void);

/**
 * tws主从切换后会调用此函数进行ble的开关
 */
void adv_role_switch_handle(u8 role);

/**
 * @abstract 单备份升级新回连，定义升级广播包内容
 *
 * @result 广播包长度
 */
u16 rebuild_adv_rcsp_info(u8 *adv_rsp_buf, u16 buf_size, u8 type, u8 *addr);

#endif

#endif
