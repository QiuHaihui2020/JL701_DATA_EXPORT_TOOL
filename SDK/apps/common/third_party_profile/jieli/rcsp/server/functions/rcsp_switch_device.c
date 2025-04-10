#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_switch_device.data.bss")
#pragma data_seg(".rcsp_switch_device.data")
#pragma const_seg(".rcsp_switch_device.text.const")
#pragma code_seg(".rcsp_switch_device.text")
#endif
#include "rcsp_config.h"
#include "rcsp_device_feature.h"
#include "btstack/avctp_user.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "rcsp_bt_manage.h"
#include "rcsp_update.h"
#include "JL_rcsp_protocol.h"
#include "app_ble_spp_api.h"

#include "app_main.h"

#if (RCSP_MODE)

/* #define RCSP_DEBUG_EN */

#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         printf_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

extern void *rcsp_server_ble_hdl;
extern void *rcsp_server_ble_hdl1;

static u8 g_new_reconn_flag = 0;

static u16 g_ble_con_handle = 0;		// ble手机发切换通讯方式时不为空
static bool g_is_switching = false;
static u8 g_is_switch_to_spp = 0;		// 0:BLE; 1:SPP

extern void ble_app_disconnect(void);
extern u8 check_le_pakcet_sent_finish_flag(void);
static void wait_response_and_disconn_ble(void *priv)
{
    static u16 wait_response_timeout = 0;
    static u8 wait_cnt = 0;
    // ble切换spp：前一个条件开始不满足，之后满足; 后一个条件开始满足，之后不满足; 超时10次后自动断开ble
    if (wait_response_timeout || (g_ble_con_handle && (1 == g_is_switch_to_spp))) {
        if ((10 == wait_cnt) || (rcsp_send_list_is_empty() && check_le_pakcet_sent_finish_flag())) {
            if (wait_response_timeout) {
                sys_timeout_del(wait_response_timeout);
                wait_response_timeout = 0;
            }
            wait_cnt = 0;
            ble_app_disconnect();
            /*             u16 ble_con_handle = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl); */
            /*             if (g_ble_con_handle == ble_con_handle) { */
            /*                 app_ble_disconnect(rcsp_server_ble_hdl); */
            /*             } */
            /* #if TCFG_RCSP_DUAL_CONN_ENABLE */
            /*             u16 ble_con_handle1 = app_ble_get_hdl_con_handle(rcsp_server_ble_hdl1); */
            /*             if (g_ble_con_handle == ble_con_handle1) { */
            /*                 app_ble_disconnect(rcsp_server_ble_hdl1); */
            /*             } */
            /* #endif */
            g_is_switching = false;
        } else {
            wait_cnt++;
            wait_response_timeout = sys_timeout_add(NULL, wait_response_and_disconn_ble, 100);
        }
    } else {
        g_is_switching = false;
    }
}

u8 get_rcsp_support_new_reconn_flag(void)
{
    return g_new_reconn_flag;
}

void switch_device(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ;
    }
    if (g_is_switching) {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_BUSY, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        return;
    }
    rcspModel->trans_chl = data[0];//指spp还是ble
    printf("rcspModel->trans_chl:%x\n", rcspModel->trans_chl);
    if (len > 1) {
        g_new_reconn_flag = data[1];
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &g_new_reconn_flag, 1, ble_con_handle, spp_remote_addr);
    } else {
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
    }
    g_is_switch_to_spp = data[0];
    g_is_switching = true;
    g_ble_con_handle = ble_con_handle;
#if RCSP_UPDATE_EN
    if (get_jl_update_flag()) {
        if (ble_con_handle) {
            rcsp_printf("BLE_ CON START DISCON\n");
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_DEV_DISCONNECT, NULL, 0);
            g_is_switching = false;
        } else {
            rcsp_printf("WAIT_FOR_SPP_DISCON\n");
            g_is_switching = false;
        }
    } else {
        wait_response_and_disconn_ble(NULL);
    }
#else
    wait_response_and_disconn_ble(NULL);
#endif
}

#endif//RCSP_MODE


