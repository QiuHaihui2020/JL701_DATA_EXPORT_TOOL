
#ifndef _JL_RCSP_LIB_API_H_
#define _JL_RCSP_LIB_API_H_

#include "typedef.h"
#include "uart.h"

#define USE_LITTLE_ENDIAN  0
#define USE_BIG_ENDIAN     1

#define USE_ENDIAN_TYPE    USE_LITTLE_ENDIAN


#define AI_LICENCE_LEN    16

enum {
    TULING = 0,
    DEEPBRAIN,
};

#pragma pack(1)
struct _AI_platform {
    u8 platform;
    u8 license[AI_LICENCE_LEN];
};
#pragma pack()

#pragma pack(1)
typedef struct _rcsp_auth_hdl {
    u16 ble_con_handle;
    u8 spp_remote_addr[6];
    bool rcsp_auth_flag;				// 是否认证成功
} JL_rcsp_auth_hdl;
#pragma pack()

u16 app_htons(u16 n);
u16 app_ntohs(u16 n);

u32 app_htonl(u32 n);
u32 app_ntohl(u32 n);

void JL_rcsp_auth_init(int (*send)(u16, u8 *, u8 *, u16, uint16_t, u8), u8 *link_key, u8 *addr);
void JL_rcsp_auth_reset(void);

/**
 * @brief 判断buf是否是rcsp协议头
 *
 * @param buf
 * @param len
 */
bool rcsp_protocol_head_check(u8 *buf, u16 len);

/**
 * @brief 设备断开ble/spp时调用，重置重置认证情况
 *
 * @param ble_con_handle	ble的con_handle句柄
 * @param spp_remote_addr	spp:mac_addr
 */
void JL_rcsp_reset_bthdl_auth(u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief 获取设备是否rcsp验证
 *
 * @param ble_con_handle	ble的con_handle句柄
 * @param spp_remote_addr	spp:mac_addr
 */
bool JL_rcsp_get_auth_flag_with_bthdl(u16 ble_con_handle, u8 *spp_remote_addr);

u8 JL_rcsp_get_auth_flag(void);
void JL_rcsp_set_auth_flag(u8 auth_flag);

/**
 * @brief rcsp验证函数
 *
 * @param ble_con_handle	ble的con_handle句柄
 * @param spp_remote_addr	spp:mac_addr
 * @param buffer			手机发过来的验证数据
 * @param len				buffer 's length
 */
void JL_rcsp_auth_recieve(u16 ble_con_handle, u8 *spp_remote_addr, u8 *buffer, u16 len);

u8 get_rcsp_version(void);

/**
 * @brief 设置设备rcsp验证情况
 *
 * @param hdl 第一个连接设备的认证情况
 * @param hdl1 第二个连接设备的认证情况
 */
void JL_rcsp_set_auth_hdl(JL_rcsp_auth_hdl *hdl, JL_rcsp_auth_hdl *hdl1);

/**
 * @brief 获取设备是否rcsp验证
 *
 * @param hdl 第一个连接设备的认证情况
 * @param hdl1 第二个连接设备的认证情况
 */
void JL_rcsp_get_auth_hdl(JL_rcsp_auth_hdl *hdl, JL_rcsp_auth_hdl *hdl1);

/**
 * @brief 设备连接认证/断开连接后lib会调用这个函数，
 * 			应用层需要调用JL_rcsp_get_auth_hdl获取手机的认证状态，
 * 			并把手机的认证情况同步到tws对端
 */
void JL_rcsp_auth_flag_tws_sync(void);

#endif //_JL_RCSP_LIB_H_
