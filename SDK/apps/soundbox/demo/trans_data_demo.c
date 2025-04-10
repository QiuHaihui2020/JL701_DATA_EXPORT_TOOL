#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".trans_data_demo.data.bss")
#pragma data_seg(".trans_data_demo.data")
#pragma const_seg(".trans_data_demo.text.const")
#pragma code_seg(".trans_data_demo.text")
#endif
#include "app_config.h"


#include "soundbox.h"
#include "app_main.h"
#include "update_tws.h"
#include "3th_profile_api.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_tws.h"
#include "spp_trans_data.h"
#include "spp_user.h"


#if (THIRD_PARTY_PROTOCOLS_SEL & TRANS_DATA_EN)

#define LOG_TAG             "[TRANS_DATA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


u8 ble_adv_miss_flag = 0;
u8 ble_adv_poweron_flag = 0;

int trans_data_earphone_state_tws_connected(int first_pair, u8 *comm_addr)
{
#if 0
    if (first_pair) {
        /* bt_ble_adv_enable(0); */
        u8 tmp_ble_addr[6] = {0};
        bt_make_ble_address(tmp_ble_addr, comm_addr);
        le_controller_set_mac(tmp_ble_addr);//将ble广播地址改成公共地址
        bt_update_mac_addr(comm_addr);
        /* bt_ble_adv_enable(1); */


        /*新的连接，公共地址改变了，要重新将新的地址广播出去*/
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("\nNew Connect Master!!!\n\n");
            ble_app_disconnect();
            bt_ble_adv_enable(0);
            bt_ble_adv_enable(1);
        } else {
            printf("\nConnect Slave!!!\n\n");
            /*从机ble关掉*/
            ble_app_disconnect();
            bt_ble_adv_enable(0);
        }
    }
#endif
    return 0;
}

int trans_data_earphone_state_enter_soft_poweroff()
{
    bt_ble_exit();
    return 0;
}

void trans_data_bt_tws_event_handler(struct bt_event *bt)
{
    int role = bt->args[0];
    int phone_link_connection = bt->args[1];
    int reason = bt->args[2];

    switch (bt->event) {
    case TWS_EVENT_CONNECTED:
        //bt_ble_adv_enable(1);
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            //master enable
            printf("\nConnect Slave!!!\n\n");
            /*从机ble关掉*/
            ble_app_disconnect();
            bt_ble_adv_enable(0);
        }

        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        /*
         * 跟手机的链路LMP层已完全断开, 只有tws在连接状态才会收到此事件
         */
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        /*
         * TWS连接断开
         */
        if (app_var.goto_poweroff_flag) {
            break;
        }
        if (get_app_connect_type() == 0) {
            printf("\ntws detach to open ble~~~\n\n");
            bt_ble_adv_enable(1);
        }
        set_ble_connect_type(TYPE_NULL);

        break;
    case TWS_EVENT_SYNC_FUN_CMD:
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }

#if OTA_TWS_SAME_TIME_ENABLE
    tws_ota_app_event_deal(bt->event);
#endif
}

int trans_data_sys_event_handler_specific(struct sys_event *event)
{
    switch (event->type) {
    case SYS_BT_EVENT:
#if 0
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {

        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {

        }
#if TCFG_USER_TWS_ENABLE
        else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
            trans_data_bt_tws_event_handler(&event->u.bt);
        }
#endif
#if OTA_TWS_SAME_TIME_ENABLE
        else if (((u32)event->arg == SYS_BT_OTA_EVENT_TYPE_STATUS)) {
            bt_ota_event_handler(&event->u.bt);
        }
#endif
#endif
        break;
    case SYS_DEVICE_EVENT:
        break;
    }

    return 0;
}

int user_spp_state_specific(u8 packet_type)
{
#if 0
    switch (packet_type) {
    case 1:
        bt_ble_adv_enable(0);
        set_app_connect_type(TYPE_SPP);
        break;
    case 2:

        set_app_connect_type(TYPE_NULL);

#if TCFG_USER_TWS_ENABLE
        if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
            ble_module_enable(1);
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                ble_module_enable(1);
            }
        }
#else
        ble_module_enable(1);
#endif

        break;
    }
#endif
    return 0;
}


static int trans_data_earphone_state_init()
{
    transport_spp_init();
    bt_ble_init();

    return 0;
}

static int trans_data_btstack_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;

    switch (event->event) {
    case BT_STATUS_INIT_OK:
        printf("trans_data_earphone_state_init\n");
        trans_data_earphone_state_init();
        return 0;
    case BT_STATUS_FIRST_CONNECTED:
        break;
    case BT_STATUS_SECOND_CONNECTED:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(trans_data_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = trans_data_btstack_event_handler,
};

static int trans_data_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int reason = event->args[2];

    switch (event->event) {
    case TWS_EVENT_CONNECTED:
        if (role == TWS_ROLE_MASTER) {
        } else {
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        if (app_var.goto_poweroff_flag) {
            break;
        }
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }

    return 0;
}

APP_MSG_HANDLER(trans_data_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = trans_data_tws_event_handler,
};



#endif
