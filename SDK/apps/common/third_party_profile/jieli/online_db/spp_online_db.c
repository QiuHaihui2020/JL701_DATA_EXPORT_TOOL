#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spp_online_db.data.bss")
#pragma data_seg(".spp_online_db.data")
#pragma const_seg(".spp_online_db.text.const")
#pragma code_seg(".spp_online_db.text")
#endif

#include "app_config.h"
#include "app_action.h"

#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "bt_common.h"
#include "online_db_deal.h"
#include "spp_online_db.h"
#include "classic/tws_api.h"
#include "cfg_tool.h"
#include "app_ble_spp_api.h"
#include "app_main.h"

#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
#include "app_anctool.h"
#endif


#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#if APP_ONLINE_DEBUG

static void *online_debug_spp_hdl = NULL;

static struct db_online_api_t *db_api;
static u8 spp_state;

#define ONLINE_SPP_CONNECT      0x0A
#define ONLINE_SPP_DISCONNECT   0x0B
#define ONLINE_SPP_DATA         0x0C
void tws_online_spp_send(u8 cmd, u8 *_data, u16 len, u8 tx_do_action);
int online_spp_send_data(u8 *data, u16 len)
{
#if TCFG_USER_TWS_ENABLE
    // TWS 从机不需要回复
    printf("online_spp_send_data role:%d\n", tws_api_get_role());
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        printf("online_spp slave dont send\n");
        return -1;
    }
#endif
    int ret = app_spp_data_send(online_debug_spp_hdl, data, len);
    return ret;
}

int online_spp_send_data_check(u16 len)
{
    /* if (spp_api) { */
    /* if (spp_api->busy_state()) { */
    /* return 0; */
    /* } */
    /* } */
    return 1;
}

static void online_spp_state_cbk(void *hdl, void *remote_addr, u8 state)
{
    spp_state = state;
    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");
        app_spp_set_filter_remote_addr(online_debug_spp_hdl, remote_addr);
        tws_online_spp_send(ONLINE_SPP_CONNECT, &state, 1, 1);
        break;

    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");
        tws_online_spp_send(ONLINE_SPP_DISCONNECT, &state, 1, 1);
        break;

    default:
        break;
    }

}

static void online_spp_send_wakeup(void *hdl)
{
    /* putchar('W'); */
    db_api->send_wake_up();
}

static void online_spp_recieve_cbk(void *hdl, void *remote_addr, u8 *buf, u16 len)
{
    log_info("online_spp_rx(%d) \n", len);
    /* log_info_hexdump(buf, len); */
    tws_online_spp_send(ONLINE_SPP_DATA, buf, len, 1);
}

#define ONLINE_SPP_HDL_UUID \
	(((u8)('O' + 'N') << (3 * 8)) | \
	 ((u8)('L' + 'I' + 'N' + 'E') << (2 * 8)) | \
	 ((u8)('S' + 'P' + 'P') << (1 * 8)) | \
	 ((u8)('H' + 'D' + 'L') << (0 * 8)))

void online_spp_init(void)
{
    log_info("spp_file: %s", __FILE__);
    spp_state = 0;

    if (online_debug_spp_hdl == NULL) {
        online_debug_spp_hdl = app_spp_hdl_alloc(0x0);
        if (online_debug_spp_hdl == NULL) {
            printf("online_debug_spp_hdl alloc err !!\n");
            return;
        }
    }
    app_spp_hdl_uuid_set(online_debug_spp_hdl, ONLINE_SPP_HDL_UUID);
    app_spp_recieve_callback_register(online_debug_spp_hdl, online_spp_recieve_cbk);
    app_spp_state_callback_register(online_debug_spp_hdl, online_spp_state_cbk);
    app_spp_wakeup_callback_register(online_debug_spp_hdl, online_spp_send_wakeup);
    db_api = app_online_get_api_table();
}

void online_spp_exit(void)
{
    log_info("online_spp_exit: %s", __FILE__);
    if (online_debug_spp_hdl == NULL) {
        log_info("online_debug_spp_hdl == NULL!!\n");
        return;
    } else {
        app_spp_hdl_free(online_debug_spp_hdl);
        online_debug_spp_hdl = NULL;
    }
}

static void tws_online_spp_in_task(u8 *data)
{
    printf("tws_online_spp_in_task");
    u16 data_len = little_endian_read_16(data, 2);
    switch (data[0]) {
    case ONLINE_SPP_CONNECT:
        puts("ONLINE_SPP_CONNECT000\n");
        db_api->init(DB_COM_TYPE_SPP);
        db_api->register_send_data(online_spp_send_data);
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        app_anctool_spp_connect();
#endif
        break;
    case ONLINE_SPP_DISCONNECT:
        puts("ONLINE_SPP_DISCONNECT000\n");
        db_api->exit();
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        app_anctool_spp_disconnect();
#endif
        break;
    case ONLINE_SPP_DATA:
        puts("ONLINE_SPP_DATA0000\n");
        log_info_hexdump(&data[4], data_len);
#if (TCFG_ANC_TOOL_DEBUG_ONLINE && TCFG_AUDIO_ANC_ENABLE)
        if (app_anctool_spp_rx_data(&data[4], data_len)) {
            free(data);
            return;
        }
#endif
#if TCFG_CFG_TOOL_ENABLE
#if (CFG_TOOL_VER == CFG_TOOL_VER_VISUAL)
        cfg_tool_combine_rx_data(&data[4], data_len);
        free(data);
        return;
#else
        if (!online_cfg_tool_data_deal(&data[4], data_len)) {
            free(data);
            return;
        }
#endif
#endif
        db_api->packet_handle(&data[4], data_len);

        //loop send data for test
        /* if (online_spp_send_data_check(data_len)) { */
        /*online_spp_send_data(&data[4], data_len);*/
        /* } */
        break;
    }
    free(data);
}

#if TCFG_USER_TWS_ENABLE
static void tws_online_spp_callback(u8 *data, u16 len)
{
    int msg[4];

    u8 *buf = malloc(len);
    if (!buf) {
        return;
    }
    memcpy(buf, data, len);

    msg[0] = (int)tws_online_spp_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void tws_online_spp_data_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (data[1] == 0 && !rx) {
        return;
    }
    tws_online_spp_callback(data, len);
}
REGISTER_TWS_FUNC_STUB(tws_online_spp_player_stub) = {
    .func_id = 0x096A5E82,
    .func = tws_online_spp_data_in_irq,
};

// TWS 配对成功需要同步spp连接状态
static int online_debug_tws_status_event_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state;
    log_info("online debug tws status callback 0x%x role:%x\n", evt->event, role);
    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (db_get_active_state() == DB_COM_TYPE_SPP) {
                log_info("send spp connection to slave\n");
                state = SPP_USER_ST_CONNECT;
                tws_online_spp_send(ONLINE_SPP_CONNECT, &state, 1, 0);
            }
        }
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(online_debug_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = online_debug_tws_status_event_handler,
};
#endif

void tws_online_spp_send(u8 cmd, u8 *_data, u16 len, u8 tx_do_action)
{
    u8 *data = malloc(len + 4 + 4);
    data[0] = cmd;
    data[1] = tx_do_action;
    little_endian_store_16(data, 2, len);
    memcpy(data + 4, _data, len);
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        // TWS 从机不需要同步给主机
        tws_online_spp_in_task(data);
        return;
    }
    int err = tws_api_send_data_to_sibling(data, len + 4, 0x096A5E82);
    if (err) {
        tws_online_spp_in_task(data);
    } else {
        free(data);
    }
#else
    tws_online_spp_in_task(data);
#endif
}
#endif

