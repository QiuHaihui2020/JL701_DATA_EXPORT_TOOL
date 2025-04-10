#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_adv.data.bss")
#pragma data_seg(".ble_adv.data")
#pragma const_seg(".ble_adv.text.const")
#pragma code_seg(".ble_adv.text")
#endif
#include "app_config.h"


#if TCFG_BT_BLE_ADV_ENABLE

#include "soundbox.h"
#include "app_main.h"
#include "bt_ble.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "system/includes.h"
#include "bt_tws.h"

#define LOG_TAG             "[BLE-ADV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

typedef struct {
    u8 miss_flag: 1;
    u8 exchange_bat: 2;
    u8 poweron_flag: 1;
    u8 reserver: 4;
} icon_ctl_t;

static icon_ctl_t ble_icon_contrl;


int adv_earphone_state_set_page_scan_enable()
{
#if (TCFG_USER_TWS_ENABLE == 0)
    bt_ble_icon_open(ICON_TYPE_INQUIRY);
#elif (CONFIG_NO_DISPLAY_BUTTON_ICON || !TCFG_CHARGESTORE_ENABLE)
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        printf("switch_icon_ctl11...\n");
        bt_ble_icon_open(ICON_TYPE_INQUIRY);
    }
#endif
    return 0;
}


int adv_earphone_state_cancel_page_scan()
{
#if (TCFG_USER_TWS_ENABLE == 1)
#if (CONFIG_NO_DISPLAY_BUTTON_ICON || !TCFG_CHARGESTORE_ENABLE)
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (ble_icon_contrl.miss_flag) {
            ble_icon_contrl.miss_flag = 0;
            puts("ble_icon_contrl.miss_flag...\n");
        } else {
            printf("switch_icon_ctl00...\n");
            bt_ble_icon_open(ICON_TYPE_INQUIRY);
        }
    }
#endif
#endif
    return 0;
}

int adv_earphone_state_tws_init(int paired)
{
    memset(&ble_icon_contrl, 0, sizeof(icon_ctl_t));
    ble_icon_contrl.poweron_flag = 1;

    if (paired) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_ble_set_control_en(1);
        } else {
            //slave close
            bt_ble_set_control_en(0);
        }
    } else {

    }

    return 0;
}


int adv_earphone_state_enter_soft_poweroff()
{
#if (!TCFG_CHARGESTORE_ENABLE)
    //非智能充电仓时，做停止广播操作
    if (bt_ble_icon_get_adv_state() != ADV_ST_NULL &&
        bt_ble_icon_get_adv_state() != ADV_ST_END) {
        bt_ble_icon_close(1);
        os_time_dly(50);//盒盖时间，根据效果调整时间
    }
#endif
    bt_ble_exit();
    return 0;
}

static int adv_bt_status_event_handler(int *msg)
{
    u8 connet_type;
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        if (bt_get_auto_connect_state(bt->args)) {
            connet_type = ICON_TYPE_RECONNECT;
        } else {
            connet_type = ICON_TYPE_CONNECTED;
        }
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_ble_icon_open(connet_type);
        } else {
            //maybe slave already open
            bt_ble_icon_close(0);
        }
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        break;

    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
        break;
    }

    return 0;
}
APP_MSG_HANDLER(adv_btstack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = adv_bt_status_event_handler,
};


int ble_adv_hci_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case HCI_EVENT_CONNECTION_COMPLETE:
        switch (bt->value) {
        case ERROR_CODE_PIN_OR_KEY_MISSING:
#if (CONFIG_NO_DISPLAY_BUTTON_ICON && TCFG_CHARGESTORE_ENABLE)
            //已取消配对了
            if (bt_ble_icon_get_adv_state() == ADV_ST_RECONN) {
                //切换广播
                bt_ble_icon_open(ICON_TYPE_INQUIRY);
            }
#endif
            break;
        }
        break;
    }

    return 0;
}
APP_MSG_HANDLER(adv_bthci_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = ble_adv_hci_event_handler,
};

int ble_adv_bt_tws_event_handler(int *msg)
{
    struct tws_event *bt = (struct tws_event *)msg;
    int role = bt->args[0];
    int phone_link_connection = bt->args[1];
    int reason = bt->args[2];

    switch (bt->event) {
    case TWS_EVENT_CONNECTED:
        bt_ble_icon_slave_en(1);
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            //master enable
            log_info("master do icon_open\n");
            bt_ble_set_control_en(1);

            if (phone_link_connection) {
                bt_ble_icon_open(ICON_TYPE_RECONNECT);
            } else {
#if (TCFG_CHARGESTORE_ENABLE && !CONFIG_NO_DISPLAY_BUTTON_ICON)
                bt_ble_icon_open(ICON_TYPE_RECONNECT);
#else
                if (ble_icon_contrl.poweron_flag) { //上电标记
                    if (g_bt_hdl.auto_connection_counter > 0) {
                        //有回连手机动作
                        /* g_printf("ICON_TYPE_RECONNECT"); */
                        /* bt_ble_icon_open(ICON_TYPE_RECONNECT); //没按键配对的话，等回连成功的时候才显示电量。如果在这里显示，手机取消配对后耳机开机，会显示出按键的界面*/
                    } else {
                        //没有回连，设可连接
                        /* g_printf("ICON_TYPE_INQUIRY"); */
                        bt_ble_icon_open(ICON_TYPE_INQUIRY);
                    }

                }
#endif
            }
        } else {
            //slave disable
            bt_ble_set_control_en(0);
        }
        ble_icon_contrl.poweron_flag = 0;
        break;
    case TWS_EVENT_CONNECTION_TIMEOUT:
        /*
        * TWS连接超时
        */
        bt_ble_icon_slave_en(0);
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        /*
         * 跟手机的链路LMP层已完全断开, 只有tws在连接状态才会收到此事件
         */
        if (reason == 0x0b) {
            //CONNECTION ALREADY EXISTS
            ble_icon_contrl.miss_flag = 1;
        } else {
            ble_icon_contrl.miss_flag = 0;
        }
        break;
    case TWS_EVENT_ROLE_SWITCH:
        bt_ble_icon_role_switch(role);
        break;
    }

    return 0;
}
APP_MSG_HANDLER(adv_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = ble_adv_bt_tws_event_handler,
};


static int adv_app_msg_handler(int *msg)
{
    u8 comm_addr[6];

    switch (msg[0]) {
    case APP_MSG_BT_OPEN_PAGE_SCAN:
        adv_earphone_state_set_page_scan_enable();
        break;
    case APP_MSG_BT_CLOSE_PAGE_SCAN:
        adv_earphone_state_cancel_page_scan();
        break;
    case APP_MSG_BT_ENTER_SNIFF:
        bt_ble_icon_state_sniff(1);
        break;
    case APP_MSG_BT_EXIT_SNIFF:
        bt_ble_icon_state_sniff(0);
        break;
    case APP_MSG_TWS_PAIRED:
        adv_earphone_state_tws_init(1);
        break;
    case APP_MSG_TWS_UNPAIRED:
        adv_earphone_state_tws_init(0);
        break;
    case APP_MSG_TWS_PAIR_SUSS:
        syscfg_read(CFG_TWS_COMMON_ADDR, comm_addr, 6);
        bt_ble_icon_set_comm_address(comm_addr);
        break;
    case APP_MSG_POWER_OFF:
        adv_earphone_state_enter_soft_poweroff();
        break;
    }
    return 0;
}
APP_MSG_HANDLER(adv_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = adv_app_msg_handler,
};


#endif

