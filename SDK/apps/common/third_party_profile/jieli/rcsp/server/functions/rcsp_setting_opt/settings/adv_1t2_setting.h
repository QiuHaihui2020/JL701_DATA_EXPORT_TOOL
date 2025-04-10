#ifndef __ADV_1T2_SETTING_H__
#define __ADV_1T2_SETTING_H__

#include "system/includes.h"

/**
 * @brief 获取rcsp_1t2开关
 *
 * @result bool
 */
bool rcsp_get_1t2_switch();

/**
 * @brief 设置rcsp_1t2开关
 *
 * @param mode_switch 开关
 */
void rcsp_set_1t2_switch(bool mode_switch, u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief edr连接的时候记录地址与手机名
 *
 * @param addr
 * @param device_name // 最长32bytes
 */
void rcsp_1t2_set_edr_info(u8 *addr, u8 *device_name);

/**
 * @brief edr断开连接的时候记录地址与手机名
 *
 * @param addr
 */
void rcsp_1t2_reset_edr_info_for_edr_disconn(u8 *addr);

/**
 * @brief edr断开连接的时候记录地址与手机名
 *
 * @param addr
 */
void rcsp_1t2_reset_edr_info_for_ble_disconn(u16 ble_con_handle);

// 初始化内部buf
void rcsp_1t2_setting_reset();

/**
 * @brief 获取rcsp_1t2已连接蓝牙设备主机(如手机)名列表
 */
void rcsp_get_1t2_bt_device_name_list(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief 通知rcsp_1t2已连接蓝牙设备主机(如手机)名列表
 */
void rcsp_notify_1t2_bt_device_name_list();

/**
 * @brief 1t2信息同步到从机
 */
void rcsp_1t2_setting_tws_sync(void);

#endif
