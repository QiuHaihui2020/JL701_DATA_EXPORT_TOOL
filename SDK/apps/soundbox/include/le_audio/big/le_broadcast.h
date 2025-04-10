/*********************************************************************************************
    *   Filename        : broadcast_api.h

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-07-07 14:37

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _BROADCAST_API_H
#define _BROADCAST_API_H

/*  Include header */
#include "system/includes.h"
#include "typedef.h"
#include "big.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! \brief 广播角色枚举 */
enum {
    BROADCAST_ROLE_UNKNOW,
    BROADCAST_ROLE_TRANSMITTER,
    BROADCAST_ROLE_RECEIVER,
};

//time 类型
enum {
    CURRENT_TIME,
    PACKET_RX_TIME,
};

/*! \brief 广播同步的参数 */
struct broadcast_sync_info {
    // 状态同步
    u8 volume;
    // 解码参数同步
    int coding_type;
    int sample_rate;
    int bit_rate;
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
extern const big_callback_t big_tx_cb;
extern const big_callback_t big_rx_cb;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* ***************************************************************************/
/**
 * @brief open broadcast as transmitter
 *
 * @return available_big_hdl:success
 */
/* *****************************************************************************/
int broadcast_transmitter(big_parameter_t *params);

/* ***************************************************************************/
/**
 * @brief open broadcast as receiver
 *
 * @return available_big_hdl:success
 */
/* *****************************************************************************/
int broadcast_receiver(big_parameter_t *params);

/* ***************************************************************************/
/**
 * @brief close broadcast function
 *
 * @param big_hdl:need closed of big_hdl
 *
 * @return 0:success
 */
/* *****************************************************************************/
int broadcast_close(u8 big_hdl);

/* --------------------------------------------------------------------------*/
/**
 * @brief get current broadcast role
 *
 * @return broadcast role
 */
/* ----------------------------------------------------------------------------*/
u8 get_broadcast_role(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief get current broadcast connect status
 *
 * @return connect status (1:connected, 0:disconnected)
 */
/* ----------------------------------------------------------------------------*/
u8 get_broadcast_connect_status(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief init broadcast params
 */
/* ----------------------------------------------------------------------------*/
void broadcast_init(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief uninit broadcast params
 */
/* ----------------------------------------------------------------------------*/
void broadcast_uninit(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 设置需要同步的状态数据
 *
 * @param big_hdl:big句柄
 * @param data:数据buffer
 * @param length:数据长度
 *
 * @return -1:fail，0:success
 */
/* ----------------------------------------------------------------------------*/
int broadcast_set_sync_data(u8 big_hdl, void *data, size_t length);

/* --------------------------------------------------------------------------*/
/**
 * @brief 初始化同步的状态数据的内容
 *
 * @param data:用来同步的数据
 */
/* ----------------------------------------------------------------------------*/
void broadcast_sync_data_init(struct broadcast_sync_info *data);

/* --------------------------------------------------------------------------*/
/**
 * @brief 进入配对模式接口
 *
 * @param role:进入配对模式的角色
 * @param mode:配对模式，0-广播配对，1-连接配对
 * @param pair_event_cb:配对事件回调函数
 * @param user_pair_code:非0时，底层使用该配对码配对
 *
 * @return 0:成功
 */
/* ----------------------------------------------------------------------------*/
int broadcast_enter_pair(u8 role, u8 mode, void *pair_event_cb, u32 user_pair_code);

/* --------------------------------------------------------------------------*/
/**
 * @brief 退出配对模式接口
 *
 * @param role:退出配对模式的角色
 *
 * @return 0:成功
 */
/* ----------------------------------------------------------------------------*/
int broadcast_exit_pair(u8 role);

/* --------------------------------------------------------------------------*/
/**
 * @brief 发送端开启广播成功后的流程开启接口
 *
 * @param priv:big_hdl_t
 * @param mode:保留
 *
 * @return 0:成功
 */
/* ----------------------------------------------------------------------------*/
int broadcast_transmitter_connect_deal(void *priv, u8 mode);

/* --------------------------------------------------------------------------*/
/**
 * @brief 接收端开启广播成功后的流程开启接口
 *
 * @param priv:big_hdl_t
 *
 * @return 0:成功
 */
/* ----------------------------------------------------------------------------*/
int broadcast_receiver_connect_deal(void *priv);

/* --------------------------------------------------------------------------*/
/**
 * @brief 接收端开启广播监听失败后的流程关闭接口
 *
 * @param priv:big句柄
 *
 * @return 0:成功
 */
/* ----------------------------------------------------------------------------*/
int broadcast_receiver_disconnect_deal(void *priv);

/* --------------------------------------------------------------------------*/
/**
 * @brief 供外部手动重置recorder模块
 *
 * @param big_hdl:recorder模块所对应的big_hdl
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int broadcast_audio_recorder_reset(u16 big_hdl);

/* --------------------------------------------------------------------------*/
/**
 * @brief 供外部手动关闭recorder模块
 *
 * @param big_hdl:recorder模块所对应的big_hdl
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int broadcast_audio_recorder_close(u16 big_hdl);

/* --------------------------------------------------------------------------*/
/**
 * @brief 自定义数据发送接口
 *
 * @param aux_hdl:数据通道句柄
 * @param data:需要发送的数据
 * @param length:数据长度
 *
 * @return 实际发送出去的数据长度
 */
/* ----------------------------------------------------------------------------*/
int broadcast_send_custom_data(u16 aux_hdl, void *data, size_t length);

int broadcast_audio_all_close(u16 big_hdl);
int broadcast_audio_all_open(u16 big_hdl);
#ifdef __cplusplus
};
#endif

#endif //_BROADCASE_API_H


