
#ifndef _BTSTACK_RCSP_USER_H_
#define _BTSTACK_RCSP_USER_H_

#include "typedef.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_packet.h"
#include "JL_rcsp_protocol.h"
#include "btstack/le/att.h"

extern void *rcsp_server_ble_hdl;		// ble连接handle
extern void *rcsp_server_ble_hdl1;		// ble连接handle，rcsp双连的时候有效

/**
 * @brief rcsp ble/spp handle初始化
 */
void bt_rcsp_interface_init(const uint8_t *rcsp_profile_data);

/**
 * @brief rcsp ble/spp handle退出并销毁
 */
void bt_rcsp_interface_exit(void);

/**
 * @brief 获取rcsp已连接设备数目
 *
 * @result 返回连接的手机设备数
 */
u8 bt_rcsp_device_conn_num(void);
/**
 * @brief 获取当前已连接ble数目
 *
 * @result 返回连接的ble手机设备数
 */
u8 bt_rcsp_ble_conn_num(void);
/**
 * @brief 获取当前已连接spp数目
 *
 * @result 返回连接的spp手机设备数
 */
u8 bt_rcsp_spp_conn_num(void);

/**
 * @brief 在连接或断开的时候，设置rcsp蓝牙连接信息
 *
 * @param con_handle ble连接或断开时输入
 * @param remote_addr spp连接或断开时输入
 * @param isconn true:连接；false:断开
 */
void bt_rcsp_set_conn_info(u16 con_handle, void *remote_addr, bool isconn);

/**
 * @brief 用于tws同步，从机接收到主机的tws数据后同步到本地
 *
 * @param bt handle数据
 * @param bt handle数据长度
 */
void rcsp_interface_set_bt_hdl_with_tws_data(u8 *recieve_buf, u16 recieve_len);


/**
 * @brief rcsp用户自定义ble广播名字
 */
// _WEAK_ const char *rcsp_ble_name()

/**
 * @brief 用于外部接收ble/spp自定义数据使用
 *
 * @param ble_con_hdl ble发送句柄
 * @param remote_addr spp发送地址
 * @param buf 接收数据
 * @param len 接收数据的长度
 * @param att_handle ble_con_hdl有值时，ble的特征值，一般是用户自定义的特征
 */
// _WEAK_ void bt_rcsp_custom_recieve_callback(u16 ble_con_hdl, void *remote_addr, u8 *buf, u16 len, uint16_t att_handle)

/**
 * @brief 用于发送ble/spp自定义数据使用
 *
 * @param ble_con_hdl ble发送句柄
 * @param remote_addr spp发送地址	注：当ble_con_hdl与remote_addr都不填时，给所有的设备都发数据
 * @param buf 发送的数据
 * @param len 发送的数据长度
 * @param att_handle ble_con_hdl有值时，可填用户自定义的特征, 为0是rcsp的特征值
 * @param att_op_type 参考att_op_type_e枚举的排序
 */
void bt_rcsp_custom_data_send(u16 ble_con_hdl, u8 *remote_addr, u8 *buf, u16 len, uint16_t att_handle, att_op_type_e att_op_type);

/**
 *	@brief 用于rcsp接收ble/spp数据使用
 *
 *	@param hdl ble/spp发送句柄
 *	@param remote_addr spp发送地址
 *	@param buf 接收数据
 *	@param len 接收数据的长度
 */
void bt_rcsp_recieve_callback(void *hdl, void *remote_addr, u8 *buf, u16 len);

/**
 *	@brief 设置rcsp的ble的默认发送特征
 *
 *	@param att_handle ble_con_hdl有值时，可填用户自定义的特征, 为0是rcsp的特征值
 *	@param att_op_type 参考att_op_type_e枚举的排序，为0是rcsp的特征值
 */
void bt_rcsp_set_default_send_att_msg(uint16_t att_handle, att_op_type_e att_op_type);

/**
 *	@brief 用于发送rcsp的数据使用
 *
 *	@param ble_con_hdl ble发送句柄
 *	@param remote_addr spp发送地址	注：当ble_con_hdl与remote_addr都不填时，给所有的设备都发数据
 *	@param buf 发送的数据
 *	@param len 发送的数据长度
 *	@param att_handle ble_con_hdl有值时，可填用户自定义的特征, 为0是rcsp的特征值，或者是bt_rcsp_set_default_send_att_msg设置的
 *	@param att_op_type 参考att_op_type_e枚举的排序，为0是rcsp默认配置，或者是bt_rcsp_set_default_send_att_msg设置的
 */
int bt_rcsp_data_send(u16 ble_con_hdl, u8 *remote_addr, u8 *buf, u16 len, uint16_t att_handle, u8 att_op_type);

/**
 * @brief 获取当前ble连接设备的mac地址
 */
u8 *rcsp_get_ble_hdl_remote_mac_addr(u16 ble_con_handle);

/**
 * @brief 获取tws同步的bt handle的总buf大小
 */
u16 rcsp_interface_tws_sync_buf_size();

/**
 * @brief rcsp主机同步bt handle数据给从机
 *
 * @param send_buf 外部malloc的一个指针，size根据rcsp_interface_tws_sync_buf_size获取
 */
void rcsp_interface_tws_sync_buf_content(u8 *send_buf);

/**
 * @brief	tws同步rcsp bt_handle信息，用于tws同步ble句柄信息
 *			已实现于rcsp_bt_manage.c
 */
// _WEAK_ void rcsp_interface_bt_handle_tws_sync(void)

/**
 *	@brief 用于外部自定义att_read_callback的使用
 *
 *	@param hdl ble的结构体句柄
 *	@param connection_handle ble的连接句柄
 *	@param att_handle ble特征值
 *	@param offset
 *	@param buffer 读特征的数据
 *	@param buffer_size 读特征的数据长度
 */
// _WEAK_ uint16_t bt_rcsp_custom_att_read_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)

/**
 *	@brief 用于外部自定义att_write_callback的使用
 *
 *	@param hdl ble的结构体句柄
 *	@param connection_handle ble的连接句柄
 *	@param att_handle ble特征值
 *	@param transaction_mode
 *	@param offset
 *	@param buffer 写特征的数据
 *	@param buffer_size 写特征的数据长度
 */
// _WEAK_ void bt_rcsp_custom_att_write_callback(void *hdl, hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)

#endif // _BTSTACK_RCSP_USER_H_
