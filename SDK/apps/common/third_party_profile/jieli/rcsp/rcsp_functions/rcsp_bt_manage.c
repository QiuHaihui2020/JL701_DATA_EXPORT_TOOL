#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_bt_manage.data.bss")
#pragma data_seg(".rcsp_bt_manage.data")
#pragma const_seg(".rcsp_bt_manage.text.const")
#pragma code_seg(".rcsp_bt_manage.text")
#endif
#include "system/event.h"
#include "app_config.h"
#include "rcsp_bt_manage.h"
#include "rcsp_config.h"
#include "rcsp.h"
#include "rcsp_update.h"
#include "rcsp_device_status.h"
#include "rcsp_setting_sync.h"

#include "app_main.h"
#include "adv_mic_setting.h"
#include "rcsp_update_tws.h"
#include "adv_time_stamp_setting.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_tws.h"
#include "ble_rcsp_adv.h"
#include "rcsp_adv_bluetooth.h"
#include "ble_rcsp_server.h"
#include "rcsp_music_info_setting.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_protocol.h"
#include "rcsp_command.h"
#include "app_ble_spp_api.h"
#include "asm/charge.h"
#include "btstack_rcsp_user.h"
#include "rcsp_ch_loader_download.h"
#include "clock_manager/clock_manager.h"

#if RCSP_MODE == RCSP_MODE_EARPHONE
#include "earphone.h"
#endif

#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
#include "adv_anc_voice.h"
#endif

#if (OTA_TWS_SAME_TIME_ENABLE && OTA_TWS_SAME_TIME_NEW)
#include "update_tws_new.h"
#endif

#if TCFG_RCSP_DUAL_CONN_ENABLE
#include "adv_1t2_setting.h"
#endif

#if RCSP_MODE

#define LOG_TAG             "[RCSP-ADV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static u8 ble_adv_miss_flag = 0;
static u8 ble_adv_poweron_flag = 0;

#if (0 == BT_CONNECTION_VERIFY)
void JL_rcsp_auth_flag_tws_sync(void);
#endif
void rcsp_protocol_bound_tws_sync(void);
void rcsp_interface_bt_handle_tws_sync(void);

int rcsp_bt_state_set_page_scan_enable()
{
#if (TCFG_USER_TWS_ENABLE == 0)
    bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
#elif (CONFIG_NO_DISPLAY_BUTTON_ICON || !TCFG_CHARGESTORE_ENABLE)
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        printf("switch_icon_ctl11...\n");
        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
    }
#endif
    return 0;
}

int rcsp_bt_state_cancel_page_scan()
{
#if (TCFG_USER_TWS_ENABLE == 0)
    bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
#elif (CONFIG_NO_DISPLAY_BUTTON_ICON || !TCFG_CHARGESTORE_ENABLE)
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        if (ble_adv_miss_flag) {
            ble_adv_miss_flag = 0;
            puts("ble_adv_miss_flag...\n");
        } else {
            printf("switch_icon_ctl00...\n");
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
        }
    }
#endif
    return 0;
}

#if TCFG_USER_TWS_ENABLE
int rcsp_bt_state_tws_init(int paired)
{
    ble_adv_poweron_flag = 1;

    if (paired) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_ble_adv_ioctl(BT_ADV_ENABLE, 0, 1);
        } else {
            //slave close
            bt_ble_adv_ioctl(BT_ADV_DISABLE, 0, 1);
        }
    }
    return 0;
}

int rcsp_bt_state_tws_connected(int first_pair, u8 *comm_addr)
{
#if RCSP_ADV_MUSIC_INFO_ENABLE
    extern void stop_get_music_timer(u8 en) ;
    stop_get_music_timer(0);
#endif

    if (first_pair) {
        u8 tmp_ble_addr[6] = {0};
#if DOUBLE_BT_SAME_MAC || TCFG_BT_BLE_BREDR_SAME_ADDR
        memcpy(tmp_ble_addr, comm_addr, 6);
#else
        bt_make_ble_address(tmp_ble_addr, comm_addr);
#endif
        /* printf("first_pait tmp_ble_addr:\n"); */
        /* put_buf(tmp_ble_addr, 6); */
        rcsp_app_ble_set_mac_addr(tmp_ble_addr);//将ble广播地址改成公共地址

        /*新的连接，公共地址改变了，要重新将新的地址广播出去*/
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("\nNew Connect Master!!!\n\n");
            ble_app_disconnect();
            ble_module_enable(0);
            ble_module_enable(1);
        } else {
            printf("\nConnect Slave!!!\n\n");
            /*从机ble关掉*/
            ble_app_disconnect();
            rcsp_bt_ble_adv_enable(0);
        }
    }

    return 0;
}
#endif

int rcsp_bt_state_enter_soft_poweroff()
{
#if (TCFG_USER_TWS_ENABLE && OTA_TWS_SAME_TIME_ENABLE && OTA_TWS_SAME_TIME_NEW)
    tws_ota_stop(0);
#endif
    bt_ble_exit();
    return 0;
}

static int rcsp_bt_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
#if TCFG_USER_TWS_ENABLE
        if ((tws_api_get_role() == TWS_ROLE_MASTER) || (bt_rcsp_spp_conn_num() > 0)) {
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
            rcsp_bt_ble_adv_enable(0);
            rcsp_bt_ble_adv_enable(1);
#endif
        } else {
            //maybe slave already open
            bt_ble_adv_ioctl(BT_ADV_DISABLE, 0, 1);
        }
#else
        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
        rcsp_bt_ble_adv_enable(0);
        rcsp_bt_ble_adv_enable(1);
#endif
#endif
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
#if RCSP_ADV_EN
        bt_adv_seq_change();
        if (!app_var.goto_poweroff_flag) {
#if TCFG_USER_TWS_ENABLE
            if ((tws_api_get_role() == TWS_ROLE_MASTER) || (bt_rcsp_spp_conn_num() > 0)) {
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
            } else {
                //maybe slave already open
                bt_ble_adv_ioctl(BT_ADV_DISABLE, 0, 1);
            }
#else
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
#endif
        }
#endif
#if TCFG_RCSP_DUAL_CONN_ENABLE
        rcsp_1t2_reset_edr_info_for_edr_disconn(bt->args);
#endif
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
#if TCFG_USER_TWS_ENABLE && RCSP_ADV_MIC_SET_ENABLE
        if (bt->value != 0xff) {
            rcsp_user_mic_fixed_deal();
        }
#endif
        break;

    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        printf("BT_STATUS_PHONE_ACTIVE\n");
#if RCSP_ADV_FIND_DEVICE_ENABLE
        extern void rcsp_send_find_device_stop(void);
        rcsp_send_find_device_stop();
#endif
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_PHONE_SCO_STATE_INFO));
        break;
    case BT_STATUS_PHONE_HANGUP:
        printf("BT_STATUS_PHONE_HANGUP\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_PHONE_SCO_STATE_INFO));
        break;
    }
    return 0;
}


static int rcsp_hci_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case HCI_EVENT_CONNECTION_COMPLETE:
        switch (bt->value) {
        case ERROR_CODE_PIN_OR_KEY_MISSING:
#if (CONFIG_NO_DISPLAY_BUTTON_ICON && TCFG_CHARGESTORE_ENABLE)
            //已取消配对了, 切换广播
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
#endif
            break;
        }
        break;
    }

    return 0;
}

#if TCFG_USER_TWS_ENABLE
static void rcsp_app_opt_tws_event_handler(struct bt_event *bt)
{
    int reason = bt->args[2];
    switch (bt->event) {
    case APP_OPT_TWS_EVENT_SYNC_FUN_CMD:
        if (reason == APP_OPT_SYNC_CMD_APP_RESET_LED_UI) {
#if RCSP_ADV_LED_SET_ENABLE
            extern void update_led_setting_state(void);
            update_led_setting_state();
#endif
        } else if (reason == APP_OPT_SYNC_CMD_MUSIC_INFO) {
#if RCSP_ADV_MUSIC_INFO_ENABLE
            extern void get_music_info(void);
            get_music_info();
#endif
        } else if (reason == APP_OPT_SYNC_CMD_MUSIC_PLAYER_STATE) {
#if RCSP_ADV_MUSIC_INFO_ENABLE
            void rcsp_update_player_state(void);
            rcsp_update_player_state();
#endif
        } else if (reason ==  APP_OPT_SYNC_CMD_MUSIC_PLAYER_TIEM_EN) {
#if RCSP_ADV_MUSIC_INFO_ENABLE
            extern void reset_player_time_en(void);
            reset_player_time_en();
#endif
        }
        break;
    }
}
#endif

/* extern u8 check_le_pakcet_sent_finish_flag(void); */
/* extern bool rcsp_send_list_is_empty(void); */
/* static u8 g_tws_disconn_try_cnt = 0; */
/* static void tws_disconn_ble(void *priv) */
/* { */
/* 	if (!rcsp_handle_get()) { */
/* 		printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
/* 		return; */
/* 	} */
/* 	printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
/* 	if ((rcsp_send_list_is_empty() && check_le_pakcet_sent_finish_flag()) || (g_tws_disconn_try_cnt >= 10)) { */
/* 		g_tws_disconn_try_cnt = 0; */
/* 		ble_module_enable(0); */
/* 		ble_module_enable(1); */
/* 	} else {	 */
/* 		g_tws_disconn_try_cnt++; */
/* 		sys_timeout_add(NULL, tws_disconn_ble, 50); */
/* 	} */
/* } */

static void rcsp_bt_tws_event_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 phone_link_connection = evt->args[1];
    u8 reason = evt->args[2];

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        printf("rcsp_bt_tws_event_handler rcsp role change:%d, %d, %d\n", role, tws_api_get_role(), bt_rcsp_device_conn_num());
        if ((role == TWS_ROLE_MASTER) && (bt_rcsp_device_conn_num() > 0)) {
            // 新耳机出仓的时候，原耳机如果已经与手机建立连接，则tws连接后，
            // 需要同步rcsp相关信息给刚出仓的新耳机
            // 不存在左右耳未配对时分别连接手机app后，tws还会配对的情况
#if (0 == BT_CONNECTION_VERIFY)
            JL_rcsp_auth_flag_tws_sync();
#endif
            rcsp_interface_bt_handle_tws_sync();
            rcsp_protocol_bound_tws_sync();
            bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1);
#if TCFG_RCSP_DUAL_CONN_ENABLE
            rcsp_1t2_setting_tws_sync();
#endif
        }
        if (role == TWS_ROLE_MASTER) {
            //master enable
            log_info("master do icon_open\n");
            if (phone_link_connection) {
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
            } else {
#if (TCFG_CHARGESTORE_ENABLE && !CONFIG_NO_DISPLAY_BUTTON_ICON)
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#else
                if (ble_adv_poweron_flag) { //上电标记

                    if (g_bt_hdl.auto_connection_counter > 0) {
                        //有回连手机动作
                        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTING, 1);
                        //没按键配对的话，等回连成功的时候才显示电量。如果在这里显示，手机取消配对后耳机开机，会显示出按键的界面
                    } else {
                        //没有回连，设可连接
                        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
                    }
                }
#endif
            }
        } else {
            //slave disable
            printf("\nConnect Slave!!!222\n\n");
            /*从机ble关掉*/
            ble_app_disconnect();
            rcsp_bt_ble_adv_enable(0);
        }
        send_version_to_sibling();
        ble_adv_poweron_flag = 0;

#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
        anc_voice_setting_sync();
#endif

#if (RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_ADV_EN
        sync_setting_by_time_stamp();
#endif

#if (RCSP_ADV_FIND_DEVICE_ENABLE && TCFG_USER_TWS_ENABLE)
        find_decice_tws_connect_handle(0, NULL);
#endif

        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        /*
         * 跟手机的链路LMP层已完全断开, 只有tws在连接状态才会收到此事件
         */
        if (reason == 0x0b) {
            //CONNECTION ALREADY EXISTS
            ble_adv_miss_flag = 1;
        } else {
            ble_adv_miss_flag = 0;
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        /*
         * TWS连接断开
         */
        // 通知手机ble回连
        /* printf("adv_cmd03 %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* u8 adv_cmd = 0x03; */
        /* adv_info_device_request(&adv_cmd, sizeof(adv_cmd)); */
        if (app_var.goto_poweroff_flag) {
            break;
        }
#if (RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_ADV_EN
        if (role == TWS_ROLE_MASTER) {
            deal_adv_setting_gain_time_stamp();
        }
#endif
        /* ble_module_enable(0); */
        /* ble_module_enable(1); */
        /* bt_rcsp_reset_ble_info_for_tws_sw(); */
        /* rcsp_ble_adv_enable_with_con_dev(); */
        /* if (bt_rcsp_device_conn_num() > 0) { */
        /*     bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1); */
        /* } */
        break;
    case TWS_EVENT_ROLE_SWITCH:
        if (role == TWS_ROLE_MASTER) {	// 切换后触发
#if RCSP_ADV_LED_SET_ENABLE
            extern void update_led_setting_state(void);
            update_led_setting_state();
#endif
        }
        {
            adv_role_switch_handle(role);
        }
        break;
    }

#if OTA_TWS_SAME_TIME_ENABLE
    tws_ota_app_event_deal(evt->event);
#endif

}

#if TCFG_USER_TWS_ENABLE
#define TWS_FUNC_ID_RCSP_SPP_STATE \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('A' + 'P' + 'I') << (2 * 8)) | \
	 ((u8)('S' + 'P' + 'P') << (1 * 8)) | \
	 ((u8)('S' + 'T' + 'E') << (0 * 8)))

int rcsp_user_spp_state_specific(u8 packet_type, u8 *spp_remote_addr);
static void rcsp_spp_state_in_task(u8 *data, int len)
{
    u8 spp_state = *data;
    /* printf("rcsp_spp_state_in_task:%d\n", spp_state); */
    /* put_buf(data, len); */
    rcsp_user_spp_state_specific(spp_state, data + 1);
    free(data);
}

static void rcsp_spp_state_in_irq(void *_data, u16 len, bool rx)
{
    u8 *rx_data = NULL;
    if (rx) {
        /* u8 *data = (u8 *)_data; */
        /* u8 spp_state = *data; */
        /* printf("spp_state-----------------:%d\n", spp_state); */
        /* put_buf(_data, len); */

        rx_data = malloc(len);
        if (rx_data == NULL) {
            return;
        }
        memcpy(rx_data, _data, len);

        int argv[4];
        argv[0] = (int)rcsp_spp_state_in_task;
        argv[1] = 2;
        argv[2] = (int)rx_data;
        argv[3] = (int)len;
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, 4, argv);
        if (ret) {
            printf("%s taskq post err \n", __func__);
        }
    }
    //cppcheck-suppress memleak
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_spp_state) = {
    .func_id = TWS_FUNC_ID_RCSP_SPP_STATE,
    .func = rcsp_spp_state_in_irq,
};
#endif

int rcsp_user_spp_state_specific(u8 packet_type, u8 *spp_remote_addr)
{
#if TCFG_USER_TWS_ENABLE
    // 通知从机同步状态
    if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) {
        u8 tws_buf[7] = {0};;	// type + addr
        tws_buf[0] = packet_type;
        if (spp_remote_addr) {
            memcpy(tws_buf + 1, spp_remote_addr, 6);
        }
        /* printf("tws_buf-----------------:\n"); */
        /* put_buf(tws_buf, 7); */
        tws_api_send_data_to_sibling((void *)&tws_buf, sizeof(tws_buf), TWS_FUNC_ID_RCSP_SPP_STATE);
    }
#endif
    switch (packet_type) {
    case SPP_USER_ST_CONNECT:
        // spp 连接后会走这里
        clock_refurbish();

#if TCFG_USER_TWS_ENABLE
        if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
            rcsp_ble_adv_enable_with_con_dev();
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                rcsp_ble_adv_enable_with_con_dev();
            } else {
                rcsp_bt_ble_adv_enable(0);
            }
        }
#else
        rcsp_ble_adv_enable_with_con_dev();
#endif

#if RCSP_ADV_EN
        set_connect_flag(SECNE_CONNECTED);
        bt_ble_adv_ioctl(BT_ADV_SET_NOTIFY_EN, 1, 1);
#endif

        break;
    case SPP_USER_ST_DISCONN:
        rcsp_clean_update_hdl_for_end_update(0, spp_remote_addr);
#if RCSP_ADV_EN
        set_connect_flag(SECNE_UNCONNECTED);
#endif
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {         //进入升级前不再开广播了
            return 0;
        } else {
#if TCFG_RCSP_DUAL_CONN_ENABLE
            extern u32 classic_update_task_exist_flag_get(void);
            if (classic_update_task_exist_flag_get()) {
                return 0;
            }
#endif
        }
#endif

#if TCFG_USER_TWS_ENABLE
        if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
            rcsp_ble_adv_enable_with_con_dev();
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                rcsp_ble_adv_enable_with_con_dev();
            }
        }
#else
        rcsp_ble_adv_enable_with_con_dev();
#endif

        break;
    }
    return 0;
}

int jl_phone_app_init()
{
    printf("jl_phone_app_init\n");
#if RCSP_ADV_MUSIC_INFO_ENABLE
    bt_music_info_handle_register(rcsp_adv_music_info_deal);
#endif
    return 0;
}


int rcsp_bt_state_init()
{
    jl_phone_app_init();
    return 0;
}

#if TCFG_USER_TWS_ENABLE
static int rcsp_tws_msg_handler(int *msg)
{
    rcsp_bt_tws_event_handler(msg);
    return 0;
}

APP_MSG_HANDLER(rcsp_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = rcsp_tws_msg_handler,
};
#endif

static int rcsp_bt_stack_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    rcsp_bt_status_event_handler(event);
    return 0;
}
APP_MSG_HANDLER(rcsp_bt_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = rcsp_bt_stack_msg_handler,
};

static int rcsp_bt_hci_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    rcsp_hci_event_handler(event);
    return 0;
}
APP_MSG_HANDLER(rcsp_bt_hci_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = rcsp_bt_hci_msg_handler,
};

static int rcsp_app_msg_handler(int *msg)
{
    u8 comm_addr[6];

    switch (msg[0]) {
    case APP_MSG_ENTER_MODE://1
        log_info("APP_MSG_ENTER_MODE");
        if ((msg[1] & 0xff) == APP_MODE_BT) {
            rcsp_bt_state_init();
        }
        break;
    case APP_MSG_BT_GET_CONNECT_ADDR://1
        log_info("APP_MSG_BT_GET_CONNECT_ADDR");
        /* bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTING, 1); */
        break;
    case APP_MSG_BT_OPEN_PAGE_SCAN://1
        log_info("APP_MSG_BT_OPEN_PAGE_SCAN");
        rcsp_bt_state_set_page_scan_enable();
        break;
    case APP_MSG_BT_CLOSE_PAGE_SCAN://1
        log_info("APP_MSG_BT_CLOSE_PAGE_SCAN");
        rcsp_bt_state_cancel_page_scan();
        break;
    case APP_MSG_BT_ENTER_SNIFF:
        break;
    case APP_MSG_BT_EXIT_SNIFF:
        break;
#if TCFG_USER_TWS_ENABLE
    case APP_MSG_TWS_PAIRED://1
        log_info("APP_MSG_TWS_PAIRED");
        rcsp_bt_state_tws_init(1);
        break;
    case APP_MSG_TWS_UNPAIRED://1
        log_info("APP_MSG_TWS_UNPAIRED");
        rcsp_bt_state_tws_init(0);
        break;
    case APP_MSG_TWS_PAIR_SUSS://1
        log_info("APP_MSG_TWS_PAIR_SUSS");
        syscfg_read(CFG_TWS_COMMON_ADDR, comm_addr, 6);
        rcsp_bt_state_tws_connected(1, comm_addr);
        break;
    case APP_MSG_TWS_CONNECTED://1
        log_info("APP_MSG_TWS_CONNECTED");
        rcsp_bt_state_tws_connected(0, NULL);
        break;
#endif
    case APP_MSG_POWER_OFF://1
        log_info("APP_MSG_POWER_OFF");
        rcsp_bt_state_enter_soft_poweroff();
        break;
    default:
        break;
    }
    return 0;
}
APP_MSG_HANDLER(rcsp_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = rcsp_app_msg_handler,
};

static int rcsp_msg_handler(int *msg)
{
    struct rcsp_event *rcsp_e = (struct rcsp_event *)msg;
    int JL_rcsp_event_handler(struct rcsp_event * rcsp);
    JL_rcsp_event_handler(rcsp_e);

    return 0;
}
APP_MSG_HANDLER(rcsp_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_RCSP,
    .handler    = rcsp_msg_handler,
};

#if TCFG_USER_TWS_ENABLE
static int rcsp_bt_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    rcsp_app_opt_tws_event_handler(event);

    return 0;
}
APP_MSG_HANDLER(rcsp_bt_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_RCSP_BT,
    .handler    = rcsp_bt_msg_handler,
};


#if (0 == BT_CONNECTION_VERIFY)
#define TWS_FUNC_ID_RCSP_AUTH \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('A' + 'U' + 'T' + 'H') << (2 * 8)) | \
	 ((u8)('T' + 'W' + 'S') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))

static void rcsp_auth_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    if (rx && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
        /* printf("auth %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(_data, len); */
        u16 auth_hdl_size = sizeof(JL_rcsp_auth_hdl);
        u8 *data = (u8 *)_data;
        JL_rcsp_auth_hdl h1, h2;
        memcpy((void *)&h1, data, auth_hdl_size);
        memcpy((void *)&h2, data + auth_hdl_size, auth_hdl_size);
        /* printf("hdl1:%d, auth1:%d addr1:\n", (u32)h1.ble_con_handle, h1.rcsp_auth_flag); */
        /* put_buf(h1.spp_remote_addr, 6); */
        /* printf("hdl2:%d, auth2:%d addr2:\n", (u32)h2.ble_con_handle, h2.rcsp_auth_flag); */
        /* put_buf(h2.spp_remote_addr, 6); */
        JL_rcsp_set_auth_hdl(&h1, &h2);
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_auth_sync) = {
    .func_id = TWS_FUNC_ID_RCSP_AUTH,
    .func = rcsp_auth_tws_sync_in_irq,
};

// rcsp认证库调用，用于tws同步设备的认证信息，勿删
void JL_rcsp_auth_flag_tws_sync(void)
{
    if (IS_CHARGE_EN()) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        return;
    }
    /* if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) { */
    // 设备连接认证/断开连接后lib会调用这个函数
    u16 auth_hdl_size = sizeof(JL_rcsp_auth_hdl);
    u8 buf[sizeof(JL_rcsp_auth_hdl) * 2] = {0};
    JL_rcsp_auth_hdl h1, h2;
    JL_rcsp_get_auth_hdl(&h1, &h2);

    memcpy(buf, (void *)&h1, auth_hdl_size);
    memcpy(buf + auth_hdl_size, (void *)&h2, auth_hdl_size);
    /* printf("auth %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(buf, sizeof(buf)); */

    // 通知从机同步状态
    tws_api_send_data_to_sibling(buf, sizeof(buf), TWS_FUNC_ID_RCSP_AUTH);
    /* } */
}
#endif // (0 == BT_CONNECTION_VERIFY)

#define TWS_FUNC_ID_RCSP_BOUND \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('B' + 'N' + 'D') << (2 * 8)) | \
	 ((u8)('T' + 'W' + 'S') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))

static void rcsp_bound_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    if (rx && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
        /* printf("bound %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(_data, len); */
        u16 auth_hdl_size = sizeof(JL_rcsp_bound_hdl);
        u8 *data = (u8 *)_data;
        JL_rcsp_bound_hdl h1, h2;
        memcpy((void *)&h1, data, auth_hdl_size);
        /* printf("hdl1:%d, addr1:\n", (u32)h1.ble_con_handle); */
        /* put_buf(h1.spp_remote_addr, 6); */
#if TCFG_RCSP_DUAL_CONN_ENABLE
        memcpy((void *)&h2, data + auth_hdl_size, auth_hdl_size);
        /* printf("hdl2:%d, addr2:\n", (u32)h2.ble_con_handle); */
        /* put_buf(h2.spp_remote_addr, 6); */
#endif
        rcsp_protocol_set_bound_hdl(&h1, &h2);
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_bnd_sync) = {
    .func_id = TWS_FUNC_ID_RCSP_BOUND,
    .func = rcsp_bound_tws_sync_in_irq,
};

// rcsp协议库调用，用于tws同步设备的协议绑定信息，勿删
void rcsp_protocol_bound_tws_sync(void)
{
    if (IS_CHARGE_EN()) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        return;
    }
    /* if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) { */
    // 设备连接认证/断开连接后lib会调用这个函数
    u16 hdl_size = sizeof(JL_rcsp_bound_hdl);
    u8 buf[sizeof(JL_rcsp_bound_hdl) * 2] = {0};
    JL_rcsp_bound_hdl h1, h2;
    rcsp_protocol_get_bound_hdl(&h1, &h2);

    memcpy(buf, (void *)&h1, hdl_size);
#if TCFG_RCSP_DUAL_CONN_ENABLE
    memcpy(buf + hdl_size, (void *)&h2, hdl_size);
#endif
    /* printf("pro %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(buf, sizeof(buf)); */

    // 通知从机同步状态
    tws_api_send_data_to_sibling(buf, sizeof(buf), TWS_FUNC_ID_RCSP_BOUND);
    /* } */
}

#define TWS_FUNC_ID_RCSP_INTERFACE_HDL_TWS_SYNC \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('I' + 'N' + 'T') << (2 * 8)) | \
	 ((u8)('H' + 'D' + 'L') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))

static void rcsp_interface_bt_handle_tws_sync_in_task(u8 *data, int len)
{
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(data, len); */
    rcsp_interface_set_bt_hdl_with_tws_data(data, len);
    free(data);
}

static void rcsp_interface_bt_handle_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    if (rx && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(_data, len); */
        u8 *rx_data = malloc(len);
        if (rx_data == NULL) {
            return;
        }
        memcpy(rx_data, _data, len);

        int argv[4];
        argv[0] = (int)rcsp_interface_bt_handle_tws_sync_in_task;
        argv[1] = 2;
        argv[2] = (int)rx_data;
        argv[3] = (int)len;
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, 4, argv);
        if (ret) {
            log_e("taskq post err \n");
        }
    }
}

REGISTER_TWS_FUNC_STUB(tws_rcsp_bt_hdl_sync) = {
    .func_id = TWS_FUNC_ID_RCSP_INTERFACE_HDL_TWS_SYNC,
    .func = rcsp_interface_bt_handle_tws_sync_in_irq,
};

void rcsp_interface_bt_handle_tws_sync(void)
{
    if (IS_CHARGE_EN()) {
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        return;
    }
    /* if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) { */
    u16 buf_size = rcsp_interface_tws_sync_buf_size();
    u8 *buf = malloc(buf_size);
    ASSERT(buf, "rcsp_interface_bt_handle_tws_sync buf malloc fail!");
    rcsp_interface_tws_sync_buf_content(buf);
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(buf, buf_size); */

    tws_api_send_data_to_sibling(buf, buf_size, TWS_FUNC_ID_RCSP_INTERFACE_HDL_TWS_SYNC);
    free(buf);
    /* } */
}

#endif

#endif
