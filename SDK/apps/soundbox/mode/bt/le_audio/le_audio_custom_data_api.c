/*********************************************************************************************
    *   Filename        : le_connected_data_interchange.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-21 15:53

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "app_le_broadcast.h"
#include "app_msg.h"

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG             "[SOUNDBOX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define EVENT_POOL_SIZE         64

/**************************************************************************************************
  Data Types
**************************************************************************************************/
struct custom_data_transmit {
    u8 type;
    u8 length;
    /* u16 crc; */
    u8 uuid;
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/* --------------------------------------------------------------------------*/
/**
 * @brief 发送数据给远端设备
 *
 * @param uuid:注册回调时配置的ID
 * @param data:需要发送的数据
 * @param len:发送数据的长度
 * @param device_channel:远端设备的标识
 *
 * @return 实际发送的数据长度
 */
/* ----------------------------------------------------------------------------*/
int wireless_custom_data_send_to_sibling(u8 uuid, void *data, u16 len, u8 device_channel)
{
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (!app_cis_is_connected(device_channel)) {
        return 0;
    }
#endif

    int buffer_len = sizeof(struct custom_data_transmit) + len;
    struct custom_data_transmit *dt = zalloc(buffer_len);
    if (!dt) {
        return -ENOMEM;
    }

    dt->type = WIRELESS_CUSTOM_DATA_TRANSMIT_TYPE;   //第一个Byte记录传输数据的类型
    dt->uuid = uuid;
    dt->length = len;
    /* dt->crc = CRC16(data, len); */
    memcpy((u8 *)dt + sizeof(*dt), data, len);

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    int slen = app_connected_send_custom_data(device_channel, (void *)dt, buffer_len);
#endif
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    int slen = app_broadcast_send_custom_data(device_channel, (void *)dt, buffer_len);
#endif
    if (!slen) {
        r_printf("wireless_custom_data_send_to_sibling err");
        free(dt);
    }

    return slen;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 数据发送成功接口
 *
 * @param data:实际发送出去的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int wireless_custom_data_send_suss(void *data, int len)
{
    struct custom_data_transmit *dt = (struct custom_data_transmit *)data;
    if (dt->type != WIRELESS_CUSTOM_DATA_TRANSMIT_TYPE) {
        return false;
    }
    free(data);
    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 数据接收成功接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:实际接收到的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int wireless_custom_data_receive_suss(u16 acl_hdl, const void *const data, int len)
{
    int msg[6];
    struct custom_data_transmit dt;
    memcpy(&dt, data, sizeof(struct custom_data_transmit));
    if (dt.type != WIRELESS_CUSTOM_DATA_TRANSMIT_TYPE) {
        return false;
    }

    /* u16 crc_check; */
    /* crc_check = CRC16((u8 *)data + sizeof(struct custom_data_transmit), dt.length); */
    /* if (crc_check != dt.crc) { */
    /*     log_error("connected data crc check fail"); */
    /*     return false; */
    /* } */

    u8 find = 0;
    const struct wireless_custom_data_trans_stub *p;
    list_for_each_wireless_data_trans(p) {
        if (p->uuid == dt.uuid) {
            find = 1;
            break;
        }
    }
    if (!find) {
        r_printf("data trans callback func not found!");
        return false;
    }

    u8 *buffer = zalloc(dt.length);
    if (!buffer) {
        ASSERT(0);
    }
    memcpy(buffer, (u8 *)data + sizeof(struct custom_data_transmit), dt.length);
    if (!p->task_name) {
        msg[0] = WIRELESS_SYNC_CALL_RX;
        msg[1] = (int)buffer;
        msg[2] = dt.length;
        p->func(acl_hdl, 3, msg);
    } else {
        msg[0] = (int)p->func;
        msg[1] = 4;
        msg[2] = acl_hdl;
        msg[3] = WIRELESS_SYNC_CALL_RX;
        msg[4] = (int)buffer;
        msg[5] = dt.length;

        int os_err = os_taskq_post_type(p->task_name, Q_CALLBACK, 6, msg);
        if (os_err != OS_ERR_NONE) {
            log_error("data_trans_err: %x, %d\n", p->uuid, os_err);
            free(buffer);
        }
    }

    return true;
}

WIRELESS_DATA_CALLBACK_FUNC_REGISTER(wireless_custom_data_trans) = {
    .event_handler  = NULL,
    .tx_events_suss = wireless_custom_data_send_suss,
    .rx_events_suss = wireless_custom_data_receive_suss,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件同步接口，默认在按键消息池处截取按键消息
 *
 * @param key_msg:具体的按键消息
 *
 * @return 按键消息转发长度
 */
/* ----------------------------------------------------------------------------*/
int wireless_key_event_sync(int key_msg)
{
#if (!LEA_CIG_KEY_EVENT_SYNC)
    return 0;
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (!app_cis_is_connected(WIRELESS_MASTER_DEV | WIRELESS_SLAVE_DEV1 | WIRELESS_SLAVE_DEV2)) {
        return 0;
    }
#endif

    int slen = 0;
    u8 len = sizeof(int) + 1;
    u8 *buffer = zalloc(len);
    u8 device_channel = WIRELESS_MASTER_DEV | WIRELESS_SLAVE_DEV1 | WIRELESS_SLAVE_DEV2;
    ASSERT(buffer);
    buffer[0] = WIRELESS_KEY_SYNC_TYPE;
    memcpy(buffer + 1, &key_msg, sizeof(int));

    if (buffer) {
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
        slen = app_connected_send_custom_data(device_channel, (void *)buffer, len);
#endif
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        slen = app_broadcast_send_custom_data(device_channel, (void *)buffer, len);
#endif
        if (!slen) {
            free(buffer);
            app_send_message(key_msg, 0);
        }
    }

    return slen;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG状态回调函数，用于处理部分状态下按键同步执行的流程
 *
 * @param event:CIG状态事件
 * @param arg:状态事件附带的参数
 */
/* ----------------------------------------------------------------------------*/
static void wireless_sync_key_events_handler(const CIG_EVENT event, void *arg)
{
    switch (event) {
    case CIG_EVENT_CENTRAL_CONNECT:
    case CIG_EVENT_PERIP_CONNECT:
        break;
    case CIG_EVENT_CENTRAL_DISCONNECT:
    case CIG_EVENT_PERIP_DISCONNECT:
        break;
    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件发送成功接口
 *
 * @param data:实际发送出去的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int wireless_tx_key_events_suss(void *data, int len)
{
    u8 *buffer = (u8 *)data;
    int key_msg;
    if (buffer[0] != WIRELESS_KEY_SYNC_TYPE) {
        return false;
    }

    memcpy(&key_msg, &buffer[1], sizeof(key_msg));
    app_send_message(key_msg, 0);
    free(data);

    return true;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件接收成功接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:实际接收到的数据
 * @param len:数据长度
 *
 * @return 0:不拦截后续外部的tx_events_suss函数的遍历 1:拦截后续外部的遍历
 */
/* ----------------------------------------------------------------------------*/
static int wireless_rx_key_events_suss(u16 acl_hdl, const void *const data, int len)
{
    u8 *buffer = (u8 *)data;
    int key_msg;
    if (buffer[0] != WIRELESS_KEY_SYNC_TYPE) {
        return false;
    }

    memcpy(&key_msg, &buffer[1], sizeof(key_msg));
    app_send_message(key_msg, APP_KEY_MSG_FROM_CIS);

    return true;
}

WIRELESS_DATA_CALLBACK_FUNC_REGISTER(wireless_key_sync) = {
    .event_handler  = wireless_sync_key_events_handler,
    .tx_events_suss = wireless_tx_key_events_suss,
    .rx_events_suss = wireless_rx_key_events_suss,
};

/*==========================================================================
 *                               demo
 *=========================================================================*/
#if 0
static void common_state_sync_handler(int state, int sync_call_role)
{
    switch (state) {
    default:
        g_printf("%s %d, state:0x%x, sync_call_role:%d", __FUNCTION__, __LINE__, state, sync_call_role);
        break;
    }
}

WIRELESS_SYNC_CALL_REGISTER(common_state_sync) = {
    .uuid = 'C',
    .task_name = "app_core",
    .func = common_state_sync_handler,
};

static void data_transmit_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];
    g_printf("%s %d, length:%d, trans_role:%d", __FUNCTION__, __LINE__, len, trans_role);
    put_buf(data, len);
    //由于转发流程中申请了内存，因此执行完毕后必须释放
    if (data) {
        free(data);
    }
}

WIRELESS_CUSTOM_DATA_STUB_REGISTER(data_transmit) = {
    .uuid = 'C',
    /* 没有给task_name赋值时，func默认在收数中断执行。当func在收数中断执行时，func内的代码尽量简短，避免对后续收数产生影响 */
    .task_name = "app_core",
    .func = data_transmit_handler,
};
#endif

#endif

