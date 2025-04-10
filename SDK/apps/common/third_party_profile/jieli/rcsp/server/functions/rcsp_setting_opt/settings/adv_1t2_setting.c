#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_1t2_setting.data.bss")
#pragma data_seg(".adv_1t2_setting.data")
#pragma const_seg(".adv_1t2_setting.text.const")
#pragma code_seg(".adv_1t2_setting.text")
#endif
#include "adv_1t2_setting.h"
#include "app_config.h"
#include "rcsp_cfg.h"
#include "rcsp_manage.h"
#include "rcsp_bt_manage.h"
#include "rcsp_define.h"
#include "JL_rcsp_protocol.h"
#include "ble_rcsp_server.h"
#include "app_ble_spp_api.h"
#include "btstack_rcsp_user.h"

#if RCSP_MODE && TCFG_RCSP_DUAL_CONN_ENABLE

#if RCSP_MODE == RCSP_MODE_EARPHONE
#include "earphone.h"
#endif

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#include "classic/tws_api.h"
#endif

#define DEVICE_NAME_LEN 32		// 缓存设备名长度为32，不可更改该值

#pragma pack(1)
typedef struct _rcsp_device_edr_info {
    u8 is_bind;
    u8 addr[6];
    u8 name_len;
    u8 name[DEVICE_NAME_LEN];
    u16 ble_con_handle;			// 通过协议绑定ble_con_handle
} JL_rcsp_device_edr_info;
#pragma pack()

static JL_rcsp_device_edr_info device_edr_info = {0};
static JL_rcsp_device_edr_info device_edr_info1 = {0};

void rcsp_device_edr_info_dump()
{
    return;
    printf("%s, %d, a isbind:%d, name_len:%d, ble_con_handle:%d\n", __FUNCTION__, __LINE__, device_edr_info.is_bind, device_edr_info.name_len, device_edr_info.ble_con_handle);
    put_buf(device_edr_info.addr, 6);
    put_buf(device_edr_info.name, device_edr_info.name_len);
    printf("%s, %d, b isbind:%d, name_len:%d, ble_con_handle:%d\n", __FUNCTION__, __LINE__, device_edr_info1.is_bind, device_edr_info1.name_len, device_edr_info1.ble_con_handle);
    put_buf(device_edr_info1.addr, 6);
    put_buf(device_edr_info1.name, device_edr_info1.name_len);
}

/**
 * @brief 获取rcsp_1t2开关
 *
 * @result bool
 */
bool rcsp_get_1t2_switch()
{
#if RCSP_MODE == RCSP_MODE_EARPHONE
    if (get_bt_dual_config() == DUAL_CONN_SET_TWO) {
        return true;
    }
#endif
    return false;
}

/**
 * @brief 设置rcsp_1t2开关
 *
 * @param mode_switch 开关
 */
void rcsp_set_1t2_switch(bool switch_1t2, u16 ble_con_handle, u8 *spp_remote_addr)
{
    printf("%s, switch:%d, ble_con_handle:%d, spp_remote_addr:\n", __FUNCTION__, switch_1t2, ble_con_handle);
    put_buf(spp_remote_addr, 6);
    rcsp_device_edr_info_dump();
    u8 *local_addr = spp_remote_addr;
    u8 _addr_temp[6] = {0};
    if (ble_con_handle && !memcmp(spp_remote_addr, _addr_temp, 6)) {
        // 如果手机没有绑定自己的mac地址，则通过下述api去获取ble的地址，
        // 此时ble与edr的地址可能相同则该逻辑正确，如果不相同的话则有无法处理的bug
        local_addr = rcsp_get_ble_hdl_remote_mac_addr(ble_con_handle);
    }
    printf("%s, local_addr:", __FUNCTION__);
    put_buf(local_addr, 6);
#if RCSP_MODE == RCSP_MODE_EARPHONE
    // 断开edr
    set_dual_conn_config(local_addr, switch_1t2 ? 1 : 0);
#else
    ASSERT(0, "current sdk type is not support 1t2setting!");
#endif
    // 断开ble
    rcsp_disconn_other_ble(ble_con_handle);
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_1T2_SETTING, NULL, 0);
    // 根据设置重新判断是否开启蓝牙广播
#if TCFG_USER_TWS_ENABLE
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            rcsp_ble_adv_enable_with_con_dev();
        }
    }
#else
    rcsp_ble_adv_enable_with_con_dev();
#endif
}

void rcsp_1t2_set_edr_info(u8 *addr, u8 *device_name)
{
    if (!addr) {
        return;
    }
    printf("%s, %s, %d, device_name:%s\n", __FILE__, __FUNCTION__, __LINE__, device_name);
    put_buf(addr, 6);
    if (memcmp(device_edr_info.addr, addr, 6) && memcmp(device_edr_info1.addr, addr, 6)) {
        u8 _addr_temp[6] = {0};
        if (!memcmp(device_edr_info.addr, _addr_temp, 6)) {
            printf("%s, %d\n", __FUNCTION__, __LINE__);
            memcpy(device_edr_info.addr, addr, 6);
            put_buf(device_edr_info.addr, 6);
            device_edr_info.name_len = strlen((const char *)device_name);
            if (device_edr_info.name_len) {
                memcpy(device_edr_info.name, device_name, device_edr_info.name_len);
            }
        } else if (!memcmp(device_edr_info1.addr, _addr_temp, 6)) {
            printf("%s, %d\n", __FUNCTION__, __LINE__);
            memcpy(device_edr_info1.addr, addr, 6);
            device_edr_info1.name_len = strlen((const char *)device_name);
            if (device_edr_info1.name_len) {
                memcpy(device_edr_info1.name, device_name, device_edr_info1.name_len);
            }
        }
    }
    if (bt_rcsp_device_conn_num() > 0) {
#if TCFG_USER_TWS_ENABLE
        if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) {
            rcsp_notify_1t2_bt_device_name_list();
        }
#else
        rcsp_notify_1t2_bt_device_name_list();
#endif
    }
    rcsp_device_edr_info_dump();
    rcsp_1t2_setting_tws_sync();
}

void rcsp_1t2_reset_edr_info_for_ble_disconn(u16 ble_con_handle)
{
    if (device_edr_info.ble_con_handle == ble_con_handle) {
        device_edr_info.is_bind = 0;
        device_edr_info.ble_con_handle = 0;
    }
    if (device_edr_info1.ble_con_handle == ble_con_handle) {
        device_edr_info1.is_bind = 0;
        device_edr_info1.ble_con_handle = 0;
    }
    rcsp_1t2_setting_tws_sync();
    rcsp_notify_1t2_bt_device_name_list();
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    rcsp_device_edr_info_dump();
}

void rcsp_1t2_reset_edr_info_for_edr_disconn(u8 *addr)
{
    if (!addr) {
        return;
    }
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(addr, 6); */
    u8 _addr_temp[6] = {0};
    if (memcmp(device_edr_info.addr, _addr_temp, 6)) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(device_edr_info.addr, 6); */
        if (!memcmp(device_edr_info.addr, addr, 6)) {
            /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
            rcsp_disconn_designated_ble(device_edr_info.ble_con_handle);
            memset(&device_edr_info, 0, sizeof(JL_rcsp_device_edr_info));
        }
    }
    if (memcmp(device_edr_info1.addr, _addr_temp, 6)) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(device_edr_info1.addr, 6); */
        if (!memcmp(device_edr_info1.addr, addr, 6)) {
            /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
            rcsp_disconn_designated_ble(device_edr_info1.ble_con_handle);
            memset(&device_edr_info1, 0, sizeof(JL_rcsp_device_edr_info));
        }
    }
    rcsp_1t2_setting_tws_sync();
    rcsp_notify_1t2_bt_device_name_list();
    rcsp_device_edr_info_dump();
}

void rcsp_1t2_setting_reset()
{
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    memset(&device_edr_info, 0, sizeof(JL_rcsp_device_edr_info));
    memset(&device_edr_info1, 0, sizeof(JL_rcsp_device_edr_info));
}

void rcsp_get_1t2_bt_device_name_list(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
    /* put_buf(data, len); */
    /* printf("%s, %d, ble_con_handle:%d\n", __FUNCTION__, __LINE__, ble_con_handle); */
    /* put_buf(spp_remote_addr, 6); */
    rcsp_device_edr_info_dump();

    u8 is_bind = 0;
    if (len > 0) {
        is_bind = data[0];
    }
    /* printf("%s, %d, is_bind:%d\n", __FUNCTION__, __LINE__, is_bind); */
    u8 _addr_temp[6] = {0};
    bool need_notify = false;
    if (is_bind) {
        need_notify = true;
        // 已绑定信息，更新本地缓存edr信息
        u8 *phone_addr = data + 1;
        u8 phone_name_len = data[7];
        if (!memcmp(device_edr_info.addr, phone_addr, 6)) {
            /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
            device_edr_info.is_bind = 1;
            if (phone_name_len) {
                memset(device_edr_info.name, 0, DEVICE_NAME_LEN);
                memcpy(device_edr_info.name, data + 8, phone_name_len);
            }
            // iOS绑定ble_con_handle
            if (ble_con_handle) {
                device_edr_info.ble_con_handle = ble_con_handle;
            } else {
                device_edr_info.ble_con_handle = 0;
            }
        }
        if (!memcmp(device_edr_info1.addr, phone_addr, 6)) {
            /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
            device_edr_info1.is_bind = 1;
            if (phone_name_len) {
                memset(device_edr_info1.name, 0, DEVICE_NAME_LEN);
                memcpy(device_edr_info1.name, data + 8, phone_name_len);
            }
            // iOS绑定ble_con_handle
            if (ble_con_handle) {
                device_edr_info1.ble_con_handle = ble_con_handle;
            } else {
                device_edr_info1.ble_con_handle = 0;
            }
        }
    } else {
        // do nothing
        // 未绑定信息，直接发送本地缓存edr信息
    }

    // 返回已连接手机列表
    // 			device_count + (is_bind + phone_addr + phone_name_len + phone_name) + (...)
    u8 total_buf_len = 1;
    u8 device_count = 0;
    if (memcmp(device_edr_info.addr, _addr_temp, 6)) {
        device_count++;
        // 详细见rcsp协议文档C031相关命令
        total_buf_len += (1 + 6 + 1 + device_edr_info.name_len);
    }
    if (memcmp(device_edr_info1.addr, _addr_temp, 6)) {
        device_count++;
        total_buf_len += (1 + 6 + 1 + device_edr_info1.name_len);
    }

    if (device_count == 0) {
        return;
    }

    u8 *resp = zalloc(total_buf_len);
    ASSERT(resp, "func = %s, line = %d, no ram!!\n", __FUNCTION__, __LINE__);
    memcpy(resp, (u8 *)&device_count, 1);
    u8 buf_index = 1;
    if (memcmp(device_edr_info.addr, _addr_temp, 6)) {
        /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
        memcpy(resp + buf_index, (u8 *)&device_edr_info.is_bind, 1);
        buf_index += 1;
        memcpy(resp + buf_index, device_edr_info.addr, 6);
        buf_index += 6;
        memcpy(resp + buf_index, (u8 *)&device_edr_info.name_len, 1);
        buf_index += 1;
        memcpy(resp + buf_index, device_edr_info.name, device_edr_info.name_len);
        buf_index += device_edr_info.name_len;
    }
    if (memcmp(device_edr_info1.addr, _addr_temp, 6)) {
        /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
        memcpy(resp + buf_index, (u8 *)&device_edr_info1.is_bind, 1);
        buf_index += 1;
        memcpy(resp + buf_index, device_edr_info1.addr, 6);
        buf_index += 6;
        memcpy(resp + buf_index, (u8 *)&device_edr_info1.name_len, 1);
        buf_index += 1;
        memcpy(resp + buf_index, device_edr_info1.name, device_edr_info1.name_len);
    }
    /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
    /* put_buf(resp, total_buf_len); */

    if (OpCode == JL_OPCODE_1T2_DEVICE_EDR_INFO_LIST) {
        /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
        // 回复
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp, total_buf_len, ble_con_handle, spp_remote_addr);
    } else {
        /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
        // 通知
        JL_CMD_send(JL_OPCODE_1T2_DEVICE_EDR_INFO_LIST, resp, total_buf_len, JL_NOT_NEED_RESPOND, 0, NULL);
    }
    if (need_notify) {
        /* printf("%s, %d\n", __FUNCTION__, __LINE__); */
        JL_CMD_send(JL_OPCODE_1T2_DEVICE_EDR_INFO_LIST, resp, total_buf_len, JL_NOT_NEED_RESPOND, 0, NULL);
    }
    free(resp);

    rcsp_1t2_setting_tws_sync();
}

void rcsp_notify_1t2_bt_device_name_list()
{
    rcsp_get_1t2_bt_device_name_list(NULL, 0, 0, NULL, 0, 0, NULL);
}

#if TCFG_USER_TWS_ENABLE
#define TWS_FUNC_ID_RCSP_1T2_SETTING \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('1' + 'T' + '2') << (2 * 8)) | \
	 ((u8)('T' + 'W' + 'S') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))

static void rcsp_1t2_setting_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    if (rx) {
        u16 info_size = sizeof(JL_rcsp_device_edr_info);
        memcpy((void *)&device_edr_info, _data, info_size);
        memcpy((void *)&device_edr_info1, _data + info_size, info_size);
        /* printf("1t2setting %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf((void *)&device_edr_info, info_size); */
        /* printf("1t2setting %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf((void *)&device_edr_info1, info_size); */
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_1t2_setting_sync) = {
    .func_id = TWS_FUNC_ID_RCSP_1T2_SETTING,
    .func = rcsp_1t2_setting_tws_sync_in_irq,
};

// tws同步1t2缓存信息
void rcsp_1t2_setting_tws_sync(void)
{
    if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) {
        u16 info_size = sizeof(JL_rcsp_device_edr_info);
        u8 buf[sizeof(JL_rcsp_device_edr_info) * 2] = {0};
        memcpy(buf, (void *)&device_edr_info, info_size);
        memcpy(buf + info_size, (void *)&device_edr_info1, info_size);
        /* printf("1t2setting %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf((void *)&device_edr_info, info_size); */
        /* printf("1t2setting %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf((void *)&device_edr_info1, info_size); */
        /* printf("1t2setting %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(buf, sizeof(buf)); */

        tws_api_send_data_to_sibling(buf, sizeof(buf), TWS_FUNC_ID_RCSP_1T2_SETTING);
    }
}

#else

// tws同步1t2缓存信息
void rcsp_1t2_setting_tws_sync(void)
{

}

#endif

#endif // TCFG_RCSP_DUAL_CONN_ENABLE
