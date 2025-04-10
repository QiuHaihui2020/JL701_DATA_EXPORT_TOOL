#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".soundbox.data.bss")
#pragma data_seg(".soundbox.data")
#pragma const_seg(".soundbox.text.const")
#pragma code_seg(".soundbox.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "app_tone.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "soundbox.h"
#include "idle.h"
#include "bt_slience_detect.h"

#include "ui/ui_api.h"
#include "ui_manage.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/a2dp_media_codec.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "user_cfg.h"
#include "audio_cvp.h"
#include "bt_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "btstack/bluetooth.h"
#include "spp_online_db.h"

#include "bt_event_func.h"

#include "dtemp_pll_trim.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#if TCFG_ANC_BOX_ENABLE
#include "app_ancbox.h"
#endif

#include "bt_tws.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "clock_manager/clock_manager.h"
#include "bt_background.h"
#include "app_version.h"
#include "sdfile.h"
#include "dual_conn.h"
#if RCSP_MODE
#include "rcsp.h"
#endif
#include "local_tws.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"
#include "btstack_rcsp_user.h"

#define LOG_TAG             "[SOUNDBOX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

struct bt_mode_var g_bt_hdl = {.work_mode = BT_MODE_SIGLE_BOX};

#if TCFG_APP_BT_EN

#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN))||(TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)

#include "multi_protocol_main.h"
#endif

#define AVC_VOLUME_UP			0x41
#define AVC_VOLUME_DOWN			0x42
#define AVC_PLAY			    0x44
#define AVC_PAUSE			    0x46

BT_USER_COMM_VAR bt_user_comm_var;
static u16 power_mode_timer = 0;
static u8 sniff_out = 0;

u8 a2dp_support_delay_report = 1;  ///蓝牙 delay report使能，蓝牙库使用，请勿删除

static int app_bt_init();
static int app_bt_exit();
extern void bt_tws_sync_volume();

static spinlock_t bt_code_ram;
static u8 *bt_code_run_addr = NULL;
extern u32 __bt_movable_slot_start[];
extern u32 __bt_movable_slot_end[];
extern u8 __bt_movable_region_start[];
extern u8 __bt_movable_region_end[];


u8 get_sniff_out_status()
{
    return sniff_out;
}
void clear_sniff_out_status()
{
    sniff_out = 0;
}

/**********进入蓝牙dut模式
*  mode=0:使能可以进入dut，原本流程不变。
*  mode=1:删除一些其它切换状态，产线中通过工具调用此接口进入dut模式，提高测试效率
 *********************/
void bt_bredr_enter_dut_mode(u8 mode, u8 inquiry_scan_en)
{
    puts("<<<<<<<<<<<<<bt_bredr_enter_dut_mode>>>>>>>>>>>>>>\n");
    clock_alloc("DUT", SYS_48M);
    bredr_set_dut_enble(1, 1);
    if (mode) {
        g_bt_hdl.auto_connection_counter = 0;
#if TCFG_USER_TWS_ENABLE
        bt_page_scan_for_test(inquiry_scan_en);
#endif
    }
}

static u8 *bt_get_sdk_ver_info(u8 *len)
{
    const char *p = sdk_version_info_get();
    if (len) {
        *len = strlen(p);
    }

    log_info("sdk_ver:%s %x\n", p, *len);
    return (u8 *)p;
}

void bredr_handle_register()
{
#if (TCFG_BT_SUPPORT_SPP==1)
#if APP_ONLINE_DEBUG
    online_spp_init();
#endif
    bt_spp_data_deal_handle_register(spp_data_handler);
#endif
    bt_fast_test_handle_register(bt_fast_test_api);//测试盒快速测试接口
#if TCFG_BT_VOL_SYNC_ENABLE
    bt_music_vol_change_handle_register(set_music_device_volume, phone_get_device_vol);
#endif
#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_get_battery_value_handle_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#endif

    //样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    //获取远端设备蓝牙名字回调
    bt_read_remote_name_handle_register(bt_read_remote_name);

#if TCFG_BT_MUSIC_INFO_ENABLE
    //获取歌曲信息回调
    bt_music_info_handle_register(user_get_bt_music_info);
#endif

}

#if TCFG_USER_TWS_ENABLE
static void rx_dual_conn_info(u8 *data, int len)
{
    r_printf("tws_sync_dual_conn_info_func: %d, %d\n", data[0], data[1]);
    if (data[0]) {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;
    } else {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_ONE;
    }
    syscfg_write(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);

}
static void tws_sync_dual_conn_info_func(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = malloc(len);
        memcpy(data, _data, len);
        int msg[4] = { (int)rx_dual_conn_info, 2, (int)data, len};
        os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
    }
}
REGISTER_TWS_FUNC_STUB(app_vol_sync_stub) = {
    .func_id = 0x1A782C1B,
    .func    = tws_sync_dual_conn_info_func,
};
void tws_sync_dual_conn_info()
{
    u8 data[2];
    data[0] = g_bt_hdl.bt_dual_conn_config;
    tws_api_send_data_to_slave(data, 2, 0x1A782C1B);

}
#endif

u8 get_bt_dual_config()
{
    return g_bt_hdl.bt_dual_conn_config;
}
void set_dual_conn_config(u8 *addr, u8 dual_conn_en)
{
#if TCFG_BT_DUAL_CONN_ENABLE
    if (dual_conn_en) {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;
    } else {
        g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_ONE;
        u8 *other_conn_addr;
        other_conn_addr = btstack_get_other_dev_addr(addr);
        if (other_conn_addr) {
            btstack_device_detach(btstack_get_conn_device(other_conn_addr));
        }
        if (addr) {
            updata_last_link_key(addr, get_remote_dev_info_index());
        }
    }
#if TCFG_USER_TWS_ENABLE
    tws_sync_dual_conn_info();
#endif
    bt_set_user_ctrl_conn_num((get_bt_dual_config() == DUAL_CONN_CLOSE) ? 1 : 2);
    bt_set_auto_conn_device_num((get_bt_dual_config() == DUAL_CONN_SET_TWO) ? 2 : 1);
    syscfg_write(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);
    r_printf("set_dual_conn_config=%d\n", g_bt_hdl.bt_dual_conn_config);
#endif

}
void test_set_dual_config()
{
    u8 *addr = bt_get_current_remote_addr();
    set_dual_conn_config(addr, (get_bt_dual_config() == DUAL_CONN_SET_TWO ? 0 : 1));
}

void bt_function_select_init()
{
#if TCFG_BT_DUAL_CONN_ENABLE
    g_bt_hdl.bt_dual_conn_config = DUAL_CONN_SET_TWO;//DUAL_CONN_SET_TWO:默认可以连接1t2  DUAL_CONN_SET_ONE:默认只支持一个连接
    syscfg_read(CFG_TWS_DUAL_CONFIG, &(g_bt_hdl.bt_dual_conn_config), 1);
#else
    g_bt_hdl.bt_dual_conn_config = DUAL_CONN_CLOSE;
#endif

    g_printf("<<<<<<<<<<<<<<bt_dual_conn_config=%d>>>>>>>>>>\n", g_bt_hdl.bt_dual_conn_config);
    if (g_bt_hdl.bt_dual_conn_config != DUAL_CONN_SET_TWO) {
        set_tws_task_interval(120);
    }

    bt_set_user_ctrl_conn_num((get_bt_dual_config() == DUAL_CONN_CLOSE) ? 1 : 2);
    set_lmp_support_dual_con((get_bt_dual_config() == DUAL_CONN_CLOSE) ? 1 : 2);
    bt_set_auto_conn_device_num((get_bt_dual_config() == DUAL_CONN_SET_TWO) ? 2 : 1);

    /* set_bt_data_rate_acl_3mbs_mode(1); */
    bt_set_support_msbc_flag(TCFG_BT_MSBC_EN);

#if (!CONFIG_A2DP_GAME_MODE_ENABLE)
    bt_set_support_aac_flag(TCFG_BT_SUPPORT_AAC);
    bt_set_aac_bitrate(TCFG_AAC_BITRATE);
#endif

#if (defined(TCFG_BT_SUPPORT_LHDC) && TCFG_BT_SUPPORT_LHDC)
    bt_set_support_lhdc_flag(TCFG_BT_SUPPORT_LHDC);
#endif

#if (defined(TCFG_BT_SUPPORT_LHDC_V5) && TCFG_BT_SUPPORT_LHDC_V5)
    bt_set_support_lhdc_v5_flag(TCFG_BT_SUPPORT_LHDC_V5);
#endif

#if (defined(TCFG_BT_SUPPORT_LDAC) && TCFG_BT_SUPPORT_LDAC)
    bt_set_support_ldac_flag(TCFG_BT_SUPPORT_LDAC);
#endif

#if TCFG_BT_DISPLAY_BAT_ENABLE
    bt_set_update_battery_time(60);
#else
    bt_set_update_battery_time(0);
#endif

#if TCFG_BT_HFP_ONLY_DISPLAY_BAT_ENABLE
    bt_set_disable_sco_flag(1);
#endif

    /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    bt_set_page_timeout_value(0);

    /*回连时超时参数设置。ms单位。做主机有效*/
    bt_set_super_timeout_value(8000);

#if TCFG_BT_DUAL_CONN_ENABLE
    bt_set_auto_conn_device_num(2);
#endif

#if TCFG_BT_VOL_SYNC_ENABLE
    vol_sys_tab_init();
#endif
    /* io_capabilities
     * 0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput
     *  authentication_requirements: 0:not protect  1 :protect
    */
    bt_set_simple_pair_param(3, 0, 2);

    /*测试盒连接获取参数需要的一些接口注册*/
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_VALUE, get_vbat_value);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_VBAT_PERCENT, get_vbat_percent);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_BURN_CODE, sdfile_get_burn_code);
    bt_testbox_ex_info_get_handle_register(TESTBOX_INFO_SDK_VERSION, bt_get_sdk_ver_info);

    bt_set_sbc_cap_bitpool(TCFG_BT_SBC_BITPOOL);
#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
#if TCFG_BT_BLE_BREDR_SAME_ADDR
    memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);
#else
    bt_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
#endif
    le_controller_set_mac((void *)tmp_ble_addr);
    puts("-----edr + ble 's address-----\n");
    printf_buf((void *)bt_get_mac_addr(), 6);
    printf_buf((void *)tmp_ble_addr, 6);
#endif

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif

#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif
}

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_connction_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        bt_status_init_ok();

#if TCFG_USER_BLE_ENABLE
#if RCSP_MODE
        rcsp_init();
#endif
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN))||(TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
        multi_protocol_bt_init();
#endif
        break;

    case BT_STATUS_SECOND_CONNECTED:
        bt_clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");

#if((RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_UPDATE_EN)
        if (rcsp_update_get_role_switch()) {
            tws_api_role_switch();
            tws_api_auto_role_switch_disable();
        }
#endif

#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_connect();
#endif
#if TCFG_USER_TWS_ENABLE
        bt_tws_phone_connected();
#endif
#if TCFG_BT_SUPPORT_MAP
        bt_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
#endif
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        if (app_var.goto_poweroff_flag) {
            break;
        }
#if TCFG_CHARGESTORE_ENABLE
        chargestore_set_phone_disconnect();
#endif
        break;

    case BT_STATUS_CONN_A2DP_CH:
        log_info("++++++++ BT_STATUS_CONN_A2DP_CH +++++++++  \n");
        u8 a2dp_vol_mac[6];
        memcpy(a2dp_vol_mac, bt->args, 6);
        app_audio_bt_volume_save_mac(a2dp_vol_mac);
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        log_info("++++++++ BT_STATUS_DISCON_A2DP_CH +++++++++  \n");
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        log_info("BT_STATUS_AVRCP_INCOME_OPID:%d\n", bt->value);
        if (bt->value == AVC_VOLUME_UP) {

        } else if (bt->value == AVC_VOLUME_DOWN) {
        } else if (bt->value == AVC_PLAY) {
            bt_music_player_time_timer_deal(1);
        } else if (bt->value == AVC_PAUSE) {
            bt_music_player_time_timer_deal(0);
        }
        break;

    case BT_STATUS_AVRCP_VOL_CHANGE:
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
        if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
            /* log_info("BT_STATUS_AVRCP_VOL_CHANGE %d,%d\n", bt->value, bt->value * 16 / 127); */
            u8 max_vol = app_audio_get_max_volume();
            u8 sync_volume = (int)(bt->value * 16 / 127);
            u8 cur_music_vol = sync_volume * max_vol / 16;
            /* log_info("cur_vol is:%d,sync_vol %d, max:%d\n", cur_music_vol, sync_volume, app_audio_get_max_volume()); */
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, cur_music_vol, 1);
        }
#endif
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

enum {
    TEST_STATE_INIT = 1,
    TEST_STATE_EXIT = 3,
};

enum {
    ITEM_KEY_STATE_DETECT = 0,
    ITEM_IN_EAR_DETECT,
};

static u8 in_ear_detect_test_flag = 0;
static void testbox_in_ear_detect_test_flag_set(u8 flag)
{
    in_ear_detect_test_flag = flag;
}

u8 testbox_in_ear_detect_test_flag_get(void)
{
    return in_ear_detect_test_flag;
}

static void bt_in_ear_detection_test_state_handle(u8 state, u8 *value)
{
    switch (state) {
    case TEST_STATE_INIT:
        testbox_in_ear_detect_test_flag_set(1);
        //start trim
        break;
    case TEST_STATE_EXIT:
        testbox_in_ear_detect_test_flag_set(0);
        break;
    }
}

static void bt_vendor_meta_event_handle(u8 sub_evt, u8 *arg, u8 len)
{
    log_info("vendor event:%x\n", sub_evt);
    log_info_hexdump(arg, 6);

    if (sub_evt != HCI_SUBEVENT_VENDOR_TEST_MODE_CFG) {
        log_info("unknow_sub_evt:%x\n", sub_evt);
        return;
    }

    u8 test_item = arg[0];
    u8 state = arg[1];

    if (ITEM_IN_EAR_DETECT == test_item) {
        bt_in_ear_detection_test_state_handle(state, NULL);
    }
}


static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("-----------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (bt->value == VENDOR_TEST_DISCONNECTED) {
#if TCFG_TEST_BOX_ENABLE
            if (testbox_get_status()) {
                if (bt_get_remote_test_flag()) {
                    testbox_clear_connect_status();
                }
            }
#endif
            bt_set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            cpu_reset();
            return 0;
        } else {

#if TCFG_BT_BLE_ADV_ENABLE
#if (CONFIG_BT_MODE == BT_NORMAL)
            //1:edr con;2:ble con;
            if (VENDOR_TEST_LEGACY_CONNECTED_BY_BT_CLASSIC == bt->value) {
                bt_ble_adv_enable(0);
            }
#endif
#endif
#if TCFG_USER_TWS_ENABLE
            if (VENDOR_TEST_CONNECTED_WITH_TWS != bt->value) {
                bt_tws_poweroff();
            }
#endif
        }
    }


    switch (bt->event) {
    case HCI_EVENT_VENDOR_META:
        bt_vendor_meta_event_handle(bt->value, bt->args, 6);
        break;
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");

        if (bt->value ==  ERROR_CODE_CONNECTION_TIMEOUT) {
            bt_hci_event_connection_timeout(bt);
        }
        bt_hci_event_disconnect(bt) ;
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            testbox_in_ear_detect_test_flag_set(0);
            break;

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            //case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}

u8 bt_app_exit_check(void)
{
    int esco_state;
    if (app_var.siri_stu && app_var.siri_stu != 3 && bt_get_esco_coder_busy_flag()) {
        // siri不退出
        return 0;
    }
    esco_state = bt_get_call_status();
    if (esco_state == BT_CALL_OUTGOING  ||
        esco_state == BT_CALL_ALERT     ||
        esco_state == BT_CALL_INCOMING  ||
        esco_state == BT_CALL_ACTIVE) {
        // 通话不退出
        return 0;
    }

    return 1;
}

bool bt_check_already_initializes(void)
{
    return g_bt_hdl.init_start;
}

struct app_mode *app_enter_bt_mode(int arg)
{
    int msg[16];
    struct bt_event *event;
    struct app_mode *next_mode;

#if 0
    //Note:需要在蓝牙协议栈初始化之前调用
    bt_set_rxtx_status_enable(1);
    //Note:tx_io,rx_io可以只初始一个，0xffff表示不是用该io;
    bt_rf_PA_control_io_remap(IO_PORTC_00, IO_PORTC_01);
#endif
    app_bt_init();

    while (1) {

        if (!app_get_message(msg, ARRAY_SIZE(msg), bt_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        event = (struct bt_event *)(msg + 1);

        switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
        case MSG_FROM_TWS:
            bt_tws_connction_status_event_handler(msg + 1);
            break;
#endif
        case MSG_FROM_BT_STACK:
            bt_connction_status_event_handler(event);
            break;
        case MSG_FROM_BT_HCI:
            bt_hci_event_handler(event);
            break;
        case MSG_FROM_APP:
            bt_app_msg_handler(msg + 1);
            break;
        }

        app_default_msg_handler(msg);
    }

    app_bt_exit();

    return next_mode;
}

static int bt_tone_play_end_callback(void *priv, enum stream_event event)
{
#if TCFG_USER_TWS_ENABLE && TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
    if (event == STREAM_EVENT_STOP) {
        if (g_bt_hdl.work_mode == BT_MODE_TWS) {
            bt_tws_poweron();
        }
    }
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出蓝牙等待蓝牙状态可以退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_no_background_exit_check(void *priv)
{
    if (g_bt_hdl.init_ok == 0) {
        return;
    }
    if (esco_player_runing() || a2dp_player_runing()) {
        return ;
    }
#if TCFG_USER_BLE_ENABLE && (TCFG_NORMAL_SET_DUT_MODE == 0)
    if (BT_MODE_IS(BT_NORMAL)) {
        bt_ble_exit();
    }
#endif

#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN | GFPS_EN | MMA_EN | FMNA_EN | REALME_EN | SWIFT_PAIR_EN | DMA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN))
    multi_protocol_bt_exit();
#endif

    sys_timer_del(g_bt_hdl.exit_check_timer);

#if (TCFG_KBOX_1T3_MODE_EN == 0)       //如果需要维持LE_AUDIO,则不能退出协议栈
    trim_timer_del();
    btstack_exit();
    g_bt_hdl.init_ok = 0;
#else
    btstack_exit_edr();
#endif

    g_bt_hdl.init_ok = 0;
    g_bt_hdl.init_start = 0;
    g_bt_hdl.exit_check_timer = 0;
    bt_set_stack_exiting(0);
    g_bt_hdl.exiting = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙非后台模式退出模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 bt_nobackground_exit()
{
    if (!g_bt_hdl.init_start) {
        g_bt_hdl.exiting = 0;
        return 0;
    }
#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
    bt_tws_poweroff();
#else
    dual_conn_close();
#endif
    bt_set_stack_exiting(1);
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);

#if (TCFG_KBOX_1T3_MODE_EN == 0)        //三合一退出模式不关闭le audio
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    app_broadcast_close(APP_BROADCAST_STATUS_STOP);
    app_broadcast_uninit();
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    app_connected_uninit();
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)||(TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SOURCE_EN)
    app_auracast_close_in_other_mode();
#endif


    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

    if (g_bt_hdl.exit_check_timer == 0) {
        g_bt_hdl.exit_check_timer = sys_timer_add(NULL, bt_no_background_exit_check, 10);
        printf("set exit timer\n");
    }

    return 0;
}

static int app_bt_init()
{
    int ret = -1;
#if TCFG_CODE_RUN_RAM_BT_CODE
    int bt_code_size = __bt_movable_region_end - __bt_movable_region_start;
    printf("bt_code_size:%d\n", bt_code_size);
    mem_stats();

    if (bt_code_size && bt_code_run_addr == NULL) {
        bt_code_run_addr = phy_malloc(bt_code_size);

    }
    spin_lock(&bt_code_ram);
    if (bt_code_run_addr) {
        printf("bt_code_run_addr:0x%x", (unsigned int)bt_code_run_addr);
        code_movable_load(__bt_movable_region_start, bt_code_size, bt_code_run_addr, __bt_movable_slot_start, __bt_movable_slot_end);
    }
    spin_unlock(&bt_code_ram);

    mem_stats();

#endif

    g_bt_hdl.init_start = 1;//蓝牙协议栈已经开始初始化标志位
    g_bt_hdl.init_ok = 0;
    g_bt_hdl.exiting = 0;
    g_bt_hdl.wait_exit = 0;
    g_bt_hdl.ignore_discon_tone = 0;
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if (TCFG_BT_BACKGROUND_ENABLE)      //后台返回到蓝牙模式如果是通过模式切换返回的还是要播放提示音
    if (g_bt_hdl.background.backmode == BACKGROUND_GOBACK_WITH_MODE_SWITCH && !bt_background_switch_mode_check())
#endif  //endif TCFG_BLUETOOTH_BACK_MODE
    {
        tone_player_stop();
#if TCFG_USER_TWS_ENABLE && TCFG_LOCAL_TWS_ENABLE
        ret = local_tws_enter_mode(get_tone_files()->bt_mode, NULL);
        if (ret != 0)
#endif //TCFG_LOCAL_TWS_ENABLE

        {
#if TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
            play_tone_file_callback(get_tone_files()->bt_mode, NULL, bt_tone_play_end_callback);
#else
            play_tone_file(get_tone_files()->bt_mode);
#endif
        }
    }
#if (TCFG_BT_BACKGROUND_ENABLE)
    if (g_bt_hdl.background.background_working) {
        g_bt_hdl.init_ok = 1;
        bt_background_resume();
        le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER);
        app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT);
        return 0;
    }

    bt_background_init(bt_hci_event_handler, bt_connction_status_event_handler);
#endif  //endif TCFG_BLUETOOTH_BACK_MODE

    if (g_bt_hdl.init_ok == 0) {
        bt_function_select_init();
        bredr_handle_register();
        EARPHONE_STATE_INIT();
        if (!g_bt_hdl.initializing) {
            btstack_init();
        }
#if TCFG_USER_TWS_ENABLE

#if TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
        tws_api_esco_rssi_role_switch(1);//通话根据信号强度主从切换使能
#endif
        tws_profile_init();
#if TCFG_KBOX_1T3_MODE_EN
        set_tws_task_add_run_slot(6);
        tws_api_pure_monitor_enable(1);
#endif

#endif
        void bt_sniff_feature_init();
        bt_sniff_feature_init();
        app_var.dev_volume = -1;
    } else {
        //协议栈已初始化
#if TCFG_KBOX_1T3_MODE_EN
        btstack_int_edr();
#endif
    }

#if TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;    //设置变调初始模式
#endif
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_BT);

    return 0;
}

int bt_mode_try_exit()
{
    putchar('k');

    if (g_bt_hdl.wait_exit) {
        //等待蓝牙断开或者音频资源释放或者电话资源释放
        if (!g_bt_hdl.exiting) {
            g_bt_hdl.wait_exit++;
            if (g_bt_hdl.wait_exit > 3) {
                //wait two round to do some hci event or other stack event
                return 0;
            }
        }
        return -EBUSY;
    }
    g_bt_hdl.wait_exit = 1;
    g_bt_hdl.exiting = 1;
#if(TCFG_BT_BACKGROUND_ENABLE == 0 || TCFG_LOCAL_TWS_ENABLE == 0) //非后台退出不播放提示音
    g_bt_hdl.ignore_discon_tone = bt_get_total_connect_dev();
#endif

    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);

    //only need to do once
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif

#if (TCFG_BT_BACKGROUND_ENABLE)
    bt_background_suspend();
#else
    bt_nobackground_exit();
#endif
    return -EBUSY;
}

static int app_bt_exit()
{
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_BT);

#if TCFG_CODE_RUN_RAM_BT_CODE
    if (bt_code_run_addr) {
        mem_stats();

        spin_lock(&bt_code_ram);
        code_movable_unload(__bt_movable_region_start, __bt_movable_slot_start, __bt_movable_slot_end);
        spin_unlock(&bt_code_ram);

        phy_free(bt_code_run_addr);
        bt_code_run_addr = NULL;
        mem_stats();
        printf("\n-------------bt_exit ok-------------\n");
    }
#endif

    sys_auto_shut_down_disable();

    return 0;
}

#if TCFG_BT_SUPPORT_MAP
#define    PROFILE_CMD_TRY_AGAIN_LATER      -1004
void bt_get_time_date()
{
    int error = bt_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME, 0, NULL);
    printf(">>>>>error = %d\n", error);
    if (error == PROFILE_CMD_TRY_AGAIN_LATER) {
        sys_timeout_add(NULL, bt_get_time_date, 100);
    }
}
void phone_date_and_time_feedback(u8 *data,  u16 len)
{
    printf("time：%s ", data);
}
void map_get_time_data(char *time, int status)
{
    if (status  ==  0) {
        printf("time：%s ", time);
    } else  {
        printf(">>>map get fail\n");
        sys_timeout_add(NULL, bt_get_time_date, 100);
    }
}
#endif

static int bt_mode_try_enter(int arg)
{
    return 0;
}

static const struct app_mode_ops bt_mode_ops = {
    .try_enter      = bt_mode_try_enter,
    .try_exit       = bt_mode_try_exit,
};

REGISTER_APP_MODE(bt_mode) = {
    .name   = APP_MODE_BT,
    .index  = APP_MODE_BT_INDEX,
    .ops    = &bt_mode_ops,
};
#endif

int bt_nobackground_status_event_handler(int *msg)
{
#if (!TCFG_BT_BACKGROUND_ENABLE)

    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        //非后台模式不开edr
        btstack_exit_edr();
        g_bt_hdl.init_ok = 1;
        g_bt_hdl.initializing = 0;

        trim_timer_add();


#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#if TCFG_KBOX_1T3_MODE_EN && TCFG_MIC_EFFECT_ENABLE
        if (!mic_effect_player_runing()) {
            app_broadcast_open_in_other_mode();
        }
#else
        app_broadcast_open_in_other_mode();
#endif
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
#if TCFG_KBOX_1T3_MODE_EN && TCFG_MIC_EFFECT_ENABLE
        if (is_open_cis_connet() && !mic_effect_player_runing()) {
            app_connected_open_in_other_mode();
        }
#else
        if (is_open_cis_connet()) {
            app_connected_open_in_other_mode();
        }
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        app_auracast_open_in_other_mode();
#endif
        break;

    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
#endif

    return 0;
}
APP_MSG_HANDLER(bt_nobackground_msg_entry) = {
    .owner = APP_MODE_BT,
    .from = MSG_FROM_BT_STACK,
    .handler = bt_nobackground_status_event_handler,
};

void btstack_init_for_app(void)
{
    if (g_bt_hdl.initializing) {
        return;
    }

    if (g_bt_hdl.init_ok == 0) {
        g_bt_hdl.initializing = 1;
#if TCFG_APP_BT_EN
        bt_function_select_init();
        bredr_handle_register();
#endif
        btstack_init();
#if TCFG_USER_TWS_ENABLE
        tws_profile_init();
#if TCFG_KBOX_1T3_MODE_EN
        set_tws_task_add_run_slot(6);
        tws_api_pure_monitor_enable(1);
#endif
#endif
    }
}

void btstack_exit_for_app(void)
{
    if (g_bt_hdl.init_ok) {
#if TCFG_USER_TWS_ENABLE
        tws_dual_conn_close();
        bt_tws_poweroff();
#else
        dual_conn_close();
#endif
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        btstack_exit();
        g_bt_hdl.init_ok = 0;
    }
}

void btstack_init_in_other_mode(void)
{
#if (TCFG_BT_BACKGROUND_ENABLE == 0)
    btstack_init_for_app();
#endif
}

void btstack_exit_in_other_mode(void)
{
#if (TCFG_BT_BACKGROUND_ENABLE == 0)
    btstack_exit_for_app();
#endif
}


