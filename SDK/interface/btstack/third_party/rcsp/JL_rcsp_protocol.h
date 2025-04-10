#ifndef __JL_PROTOCOL_H__
#define __JL_PROTOCOL_H__

#include "typedef.h"
#include "JL_rcsp_packet.h"

enum {
    JL_NOT_NEED_RESPOND = 0,
    JL_NEED_RESPOND,
};

typedef enum __JL_ERR {
    JL_ERR_NONE = 0x0,
    JL_ERR_SEND_DATA_OVER_LIMIT,
    JL_ERR_SEND_BUSY,
    JL_ERR_SEND_NOT_READY,
    JL_ERR_EXIT,
    JL_ERR_SLAVE_SEND,
    JL_ERR_MAX,
} JL_ERR;

typedef enum __JL_PRO_STATUS {
    JL_PRO_STATUS_SUCCESS = 0x0,
    JL_PRO_STATUS_FAIL,
    JL_PRO_STATUS_UNKOWN_CMD,
    JL_PRO_STATUS_BUSY,
    JL_PRO_STATUS_NO_RESPONSE,
    JL_PRO_STATUS_CRC_ERR,
    JL_PRO_STATUS_ALL_DATA_CRC_ERR,
    JL_PRO_STATUS_PARAM_ERR,
    JL_PRO_STATUS_RESP_DATA_OVER_LIMIT,

} JL_PRO_STATUS;

///*< JL_CMD、JL_CMD_response、JL_DATA、JL_DATA_response packet send functions>*/
JL_ERR JL_CMD_send(u8 OpCode, u8 *data, u16 len, u8 request_rsp, u16 ble_con_handle, u8 *spp_remote_addr);
JL_ERR JL_CMD_response_send(u8 OpCode, u8 status, u8 sn, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
JL_ERR JL_DATA_send(u8 OpCode, u8 CMD_OpCode, u8 *data, u16 len, u8 request_rsp, u16 ble_con_handle, u8 *spp_remote_addr);
JL_ERR JL_DATA_response_send(u8 OpCode, u8 status, u8 sn, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

///*<JL_CMD、JL_CMD_response、JL_DATA、JL_DATA_response recieve>*/
typedef struct __JL_PRO_CB {
    /*send function callback, SPP or ble*/
    void *priv;
    // bool (*fw_ready)(void *priv);
    // s32(*fw_send)(void *priv, void *buf, u16 len);

    /*JL_CMD、JL_CMD_response、JL_DATA、JL_DATA_response packet recieve callback*/
    void (*CMD_resp)(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    void (*DATA_resp)(void *priv, u8 OpCode_SN, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    void (*CMD_no_resp)(void *priv, u8 OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    void (*DATA_no_resp)(void *priv, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    void (*CMD_recieve_resp)(void *priv, u8 OpCode, u8 status, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    void (*DATA_recieve_resp)(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
    u8(*wait_resp_timeout)(void *priv, u8 OpCode, u8 counter);
    // void (*auth_pass_callback)(void *priv);
} JL_PRO_CB;

#pragma pack(1)
typedef struct _rcsp_bound_hdl {
    u16 ble_con_handle;
    u8 spp_remote_addr[6];
} JL_rcsp_bound_hdl;
#pragma pack()

// 一拖二设备标识与底层buf的绑定
void rcsp_protocol_bound(u16 ble_con_handle, u8 *spp_remote_addr);
// 一拖二设备标识与底层buf的解除绑定
void rcsp_protocol_reset_bound(u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief 设置协议绑定信息
 *
 * @param hdl 第一个连接设备的协议绑定信息
 * @param hdl1 第二个连接设备的协议绑定信息
 */
void rcsp_protocol_set_bound_hdl(JL_rcsp_bound_hdl *hdl, JL_rcsp_bound_hdl *hdl1);

/**
 * @brief 获取协议绑定信息
 *
 * @param hdl 第一个连接设备的协议绑定信息
 * @param hdl1 第二个连接设备的协议绑定信息
 */
void rcsp_protocol_get_bound_hdl(JL_rcsp_bound_hdl *hdl, JL_rcsp_bound_hdl *hdl1);

extern u32 rcsp_protocol_need_buf_size(void);

extern void JL_protocol_init(u8 *buf, u32 len);
extern void JL_protocol_exit(void);

extern void JL_protocol_dev_switch(const JL_PRO_CB *cb);

extern void JL_protocol_data_recieve(void *priv, void *buf, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void JL_protocol_resume(void);

extern bool rcsp_send_list_is_empty(void);

extern void JL_send_packet_process(void);
extern void JL_recieve_packet_parse_process(void);


#endif//__JL_PROTOCOL_H__


