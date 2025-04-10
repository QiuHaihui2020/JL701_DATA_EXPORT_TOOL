// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_RCSP_SERVER_H
#define _BLE_RCSP_SERVER_H

#include <stdint.h>
#include "bt_common.h"
#include "ble_user.h"
#include "btstack/btstack_typedef.h"
#include "app_config.h"

#if TCFG_PAY_ALIOS_ENABLE
#define  UPAY_ONE_PROFILE          1  //同一个pofile
#else
#define  UPAY_ONE_PROFILE          0
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL&RCSP_MODE_EN)

static const uint8_t profile_data[1];

#if TCFG_PAY_ALIOS_ENABLE

#if UPAY_ONE_PROFILE == 0
static const uint8_t profile_data_upay[] = {
    //////////////////////////////////////////////////////
    //
    // 0x0001 PRIMARY_SERVICE    1800
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,

    /* CHARACTERISTIC,    2a00, READ | WRITE | DYNAMIC, */
    // 0x0002 CHARACTERISTIC 2a00 READ | WRITE | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03, 0x28, 0x0a, 0x03, 0x00, 0x00, 0x2a,
    // 0x0003 VALUE 2a00 READ | WRITE | DYNAMIC
    0x08, 0x00, 0x0a, 0x01, 0x03, 0x00, 0x00, 0x2a,

    //////////////////////////////////////////////////////
    //
    // 0x0004 PRIMARY_SERVICE    1801
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x28, 0x01, 0x18,

    /* CHARACTERISTIC,    2a05, INDICATE, */
    // 0x0005 CHARACTERISTIC 2a05 INDICATE
    0x0d, 0x00, 0x02, 0x00, 0x05, 0x00, 0x03, 0x28, 0x20, 0x06, 0x00, 0x05, 0x2a,
    // 0x0006 VALUE 2a05 INDICATE
    0x08, 0x00, 0x20, 0x00, 0x06, 0x00, 0x05, 0x2a,
    // 0x0007 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x07, 0x00, 0x02, 0x29, 0x00, 0x00,


    //////////////////////////////////////////////////////
    //
    // 0x0030 PRIMARY_SERVICE    3802
    //
    //////////////////////////////////////////////////////
    0x0a, 0x00, 0x02, 0x00, 0x30, 0x00, 0x00, 0x28, 0x02, 0x38,

    /* CHARACTERISTIC,    4a02, READ | WRITE | NOTIFY | DYNAMIC, */
    // 0x0031 CHARACTERISTIC 4a02 READ | WRITE | NOTIFY | DYNAMIC
    0x0d, 0x00, 0x02, 0x00, 0x31, 0x00, 0x03, 0x28, 0x1a, 0x32, 0x00, 0x02, 0x4a,
    // 0x0032 VALUE 4a02 READ | WRITE | NOTIFY | DYNAMIC
    0x08, 0x00, 0x1a, 0x01, 0x32, 0x00, 0x02, 0x4a,
    // 0x0033 CLIENT_CHARACTERISTIC_CONFIGURATION
    0x0a, 0x00, 0x0a, 0x01, 0x33, 0x00, 0x02, 0x29, 0x00, 0x00,

    // END
    0x00, 0x00,
};

#endif

//
// characteristics <--> handles
//
#define ATT_CHARACTERISTIC_4a02_01_VALUE_HANDLE 0x0032
#define ATT_CHARACTERISTIC_4a02_01_CLIENT_CONFIGURATION_HANDLE 0x0033
#endif


enum {
    BT_ADV_ENABLE,
    BT_ADV_DISABLE,
    BT_ADV_SET_EDR_CON_FLAG,
    BT_ADV_SET_BAT_CHARGE_L,
    BT_ADV_SET_BAT_CHARGE_R,
    BT_ADV_SET_BAT_CHARGE_C,
    BT_ADV_SET_BAT_PERCENT_L,
    BT_ADV_SET_BAT_PERCENT_R,
    BT_ADV_SET_BAT_PERCENT_C,
    BT_ADV_SET_NOTIFY_EN,
};

enum {
    TWS_ADV_SEQ_CHANGE = 0,
    TWS_VERSON_INFO,
    TWS_UPDATE_INFO,
};

void rcsp_ble_profile_init(void);
void bt_ble_init(void);
void bt_ble_exit(void);
void rcsp_bt_ble_init(void);
void rcsp_bt_ble_exit(void);
void rcsp_bt_ble_adv_enable(u8 enable);
void ble_module_enable(u8 en);

extern void bt_adv_seq_change(void);
void ble_app_disconnect(void);

void notify_update_connect_parameter(u8 table_index);

/**
 *	@brief 设置ble的地址
 */
void rcsp_app_ble_set_mac_addr(void *addr);

// 返回当前设备支持的最大连接数
u8 rcsp_max_support_con_dev_num();
// 根据已连接设备数量判断是否开关蓝牙广播
void rcsp_ble_adv_enable_with_con_dev();

/**
 * @brief 断开指定的ble
 *
 * @param ble_con_handle ble_con_handle
 */
void rcsp_disconn_designated_ble(u16 ble_con_handle);

/**
 * @brief 断开另一个ble
 *
 * @param ble_con_handle 保留的ble_con_handle，输入为0的时候，全部断开
 */
void rcsp_disconn_other_ble(u16 ble_con_handle);

/**
 * @brief 一定时间设置是否关闭可发现可连接
 * 			对应TCFG_DUAL_CONN_INQUIRY_SCAN_TIME功能配置
 *
 * @param close_inquiry_scan 是否关闭可发现可连接
 */
void rcsp_close_inquiry_scan(bool close_inquiry_scan);

// ble主机相关，by lingxuanfeng
/**
 * @brief ble主机接收回调
 *
 * @param buf
 * @param len
 */
void rcsp_ble_master_recieve_callback(void *buf, u16 len);

#endif // (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_RCSP_DEMO)
#endif // _BLE_RCSP_SERVER_H
