#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gfps_protocol.data.bss")
#pragma data_seg(".gfps_protocol.data")
#pragma const_seg(".gfps_protocol.text.const")
#pragma code_seg(".gfps_protocol.text")
#endif
#include "sdk_config.h"
#include "app_msg.h"
#include "bt_tws.h"
#include "app_main.h"
#include "battery_manager.h"
#include "app_power_manage.h"
#include "btstack/avctp_user.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "audio_config.h"
#include "app_protocol_api.h"
#include "clock_manager/clock_manager.h"
#include "app_ble_spp_api.h"
#include "volume_node.h"
#include "asm/charge.h"
#include "gfps_platform_api.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & GFPS_EN)

extern u32 unactice_device_cmd_prepare(USER_CMD_TYPE cmd, u16 param_len, u8 *param);
extern void tws_dual_conn_close();
extern void bt_set_need_keep_scan(u8 en);
extern int audio_digital_vol_update_parm(u8 dvol_idx, s32 param);
extern void set_wait_poweroff_time(u16 time);

/**
 弹窗的逻辑：
 1、没有任何手机配对记录情况，广播初始配对广播。
 2、开机有配对记录广播后续配对广播，回连超时进入配对状态时，广播初始配对广播。
 3、回连手机发现linkkey_missing，且没有任何手机配对记录了，广播初始配对广播。
 4、连上任意一台手机后， 广播后续配对广播，断开进入配对状态也是广播后续配对广播。（客户要求断开后也广播初始配对广播）
 5、恢复出厂设置时，需要调用一下gfps_all_info_recover(1)清除一下GFP的账号信息和修改的蓝牙名字
 6、认证的时候，需要有一个UI来触发初始配对广播（调用gfps_set_pair_mode(NULL)），可以为长按左右耳5秒之类的
 7、耳机关机要关电量显示
 */

#define TONE_GOOGLE         "tone_en/google.*"
#define GFPS_SIBLING_SYNC_TYPE_ADV_MAC_STATE    0x04

#define ADV_INIT_PAIR_IN_PAIR_MODE      0 //进入配对状态广播初始配对广播

struct gfps_protocol_t {
    u8 poweron_reconn;
    u8 close_icon;
};
static struct gfps_protocol_t gfps_protocol = {0};

#define __this          (&gfps_protocol)

#define INIT_PAIR_ADV_INTERVAL      150
#define SUBS_PAIR_ADV_INTERVAL      352

void gfps_set_pair_mode_by_user(u8 disconn_ble)
{
    if (disconn_ble) {
        gfps_disconnect(NULL);
    }
    gfps_adv_interval_set(INIT_PAIR_ADV_INTERVAL);
    set_gfps_pair_state(0);
    gfps_bt_ble_adv_enable(0);
    gfps_bt_ble_adv_enable(1);
}

#if 0
u8 gfps_bat_test_value = 100;
void gfps_battery_test_timeout(void *pirv)
{
    gfps_bat_test_value--;
    printf(">>> gfps_bat_test_value = %d\n", gfps_bat_test_value);
    gfps_battery_update();
    sys_timeout_add(NULL, gfps_battery_test_timeout, 30000);
}
#endif

//重写弱函数，获取电量
void generate_rfcomm_battery_data(u8 *data)
{
#if 0
    if (gfps_bat_test_value == 100) {
        gfps_bat_test_value = 99;
        sys_timeout_add(NULL, gfps_battery_test_timeout, 30000);
    }
    data[0] = gfps_bat_test_value;
    data[1] = gfps_bat_test_value;
    data[2] = gfps_bat_test_value;
    return;
#endif

#if TCFG_USER_TWS_ENABLE
    u8 sibling_val = get_tws_sibling_bat_persent();
#else
    u8 sibling_val = 0xFF;
#endif
    u8 self_val = get_charge_online_flag() << 7 | get_vbat_percent();

    data[2] = 0xFF; //充电仓电量

#if TCFG_CHARGESTORE_ENABLE
    if (self_val >> 7 || sibling_val >> 7) { //有任意一直耳机充电时才显示充电仓电量
        data[2] = chargestore_get_power_level(); //充电仓电量
    }
#endif

    data[0] = (tws_api_get_local_channel() == 'L') ? self_val : sibling_val; //左耳电量
    data[1] = (tws_api_get_local_channel() == 'R') ? self_val : sibling_val; //右耳电量
    printf("%s, l:%x, r:%x, bat:%x", __func__, data[0], data[1], data[2]);
    if (data[0] == 0xFF) {
        data[0] = 0;
    }
    if (data[1] == 0xFF) {
        data[1] = 0;
    }
    if (data[2] == 0xFF) {
        data[2] = 0;
    }
}

static void gfps_ctl_bt_enter_pair_mode()
{
    printf("%s", __func__);
    tws_dual_conn_close();
    lmp_hci_write_scan_enable((1 << 1) | 1);
    bt_set_need_keep_scan(1);
}

static void gfps_before_pair_new_device_in_task(u8 *seeker_addr)
{
    u8 remote_edr_addr[6];
    u8 *tmp_addr = NULL;

    for (int i = 0; i < 6; i ++) {
        remote_edr_addr[i] = seeker_addr[5 - i];
    }

    free(seeker_addr);

#if TCFG_BT_DUAL_CONN_ENABLE
    void *devices[2] = {NULL, NULL};
    int num = btstack_get_conn_devices(devices, 2);
    u8 active_addr[6];

    for (int i = 0; i < num; i++) {
        if (!memcmp(btstack_get_device_mac_addr(devices[i]), remote_edr_addr, 6)) { //地址相同
            printf("Retroactive Pair\n");
            return;
        }
    }

    if (num == 2) { //连接了两个手机，断开其中一个
        if (esco_player_get_btaddr(active_addr) || a2dp_player_get_btaddr(active_addr)) { //断开非当前播歌通话的设备
            tmp_addr = btstack_get_other_dev_addr(active_addr);
            bt_cmd_prepare_for_addr(tmp_addr, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        } else { //断开非活跃的设备
            unactice_device_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
    } else { //开可连接
        int msg[] = {(int)gfps_ctl_bt_enter_pair_mode, 0};
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    }

#else
    if (bt_get_curr_channel_state()) {
        tmp_addr = get_cur_connect_phone_mac_addr();
        if (memcmp(tmp_addr, remote_edr_addr, 6) != 0) {
            printf("Subsequent pair\n");
            printf("USER_CTRL_DISCONNECTION_HCI send\n");
            bt_cmd_prepare_for_addr(tmp_addr, USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        } else {
            printf("Retroactive Pair\n");
        }
    }
#endif
    printf("func:%s, line:%d", __FUNCTION__, __LINE__);
}

//重写弱函数，GFP-BLE配对认证成功后，需要先断开一个设备或者开可连接，等手机连接经典蓝牙
void gfps_before_pair_new_device(u8 *seeker_addr)
{
    int msg[3];
    u8 *tmp_addr = malloc(6);
    memcpy(tmp_addr, seeker_addr, 6);
    msg[0] = (int)gfps_before_pair_new_device_in_task;
    msg[1] = 1;
    msg[2] = (int)tmp_addr;
    int err = os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    if (err) {
        printf("%s post fail\n", __func__);
    }
}

struct google_tone_ctl_t {
    u8 l_en;
    u8 r_en;
    u8 l_mute;
    u8 r_mute;
};
static struct google_tone_ctl_t google_tone_ctl = {0};

static void google_tone_set_volume()
{
    printf("func:%s:%d", __FUNCTION__, app_audio_volume_max_query(SysVol_TONE));
    u32 param = VOLUME_NODE_CMD_SET_VOL | app_audio_volume_max_query(SysVol_TONE);
    jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, "Vol_BtcRing", &param, sizeof(param));
}

static void tws_google_tone_callback(int priv, enum stream_event event)
{
    printf("func:%s, event:%d", __FUNCTION__, event);
    if (event == STREAM_EVENT_START) {
        int msg[] = {(int)google_tone_set_volume, 0};
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    }
}
REGISTER_TWS_TONE_CALLBACK(tws_google_tone_stub) = {
    .func_uuid  = 0x2EC850AE,
    .callback   = tws_google_tone_callback,
};

static int google_tone_play_callback(void *priv, enum stream_event event)
{
    printf("func:%s, event:%d", __FUNCTION__, event);
    if (event == STREAM_EVENT_START) {
        int msg[] = {(int)google_tone_set_volume, 0};
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    }
    return 0;
}

void google_tone_play_deal()
{
    if (strcmp(os_current_task(), "app_core")) {
        printf("%s, task:%s", __func__, os_current_task());
        int msg[] = {(int)google_tone_play_deal, 0};
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
        return;
    }

    /* tone_player_stop(); */
    ring_player_stop();

    u8 l_en = google_tone_ctl.l_en && !google_tone_ctl.l_mute;
    u8 r_en = google_tone_ctl.r_en && !google_tone_ctl.r_mute;

    printf("%s, l_en:%d, r_en:%d, l_mute:%d, r_mute:%d", __func__, google_tone_ctl.l_en, google_tone_ctl.r_en, google_tone_ctl.l_mute, google_tone_ctl.r_mute);

    if (l_en && r_en) {
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_SLAVE) { //对耳连上后，从机同步播。主机同步播的话，由于从机收到响铃慢，导致停止了提示音
                tws_play_ring_file_alone_callback(TONE_GOOGLE, 300, 0x2EC850AE);
            }
        } else {
            play_ring_file_alone_with_callback(TONE_GOOGLE, NULL, google_tone_play_callback);
        }
    } else if ((l_en && tws_api_get_local_channel() == 'L') ||
               (r_en && tws_api_get_local_channel() == 'R')) {
        play_ring_file_alone_with_callback(TONE_GOOGLE, NULL, google_tone_play_callback);
    }
}

void google_tone_mute_ctl(u8 self, u8 charge_flag)
{
    u8 *mute = &google_tone_ctl.l_mute;

    if ((tws_api_get_local_channel() == 'L' && !self) ||
        (tws_api_get_local_channel() == 'R' && self)) {
        mute = &google_tone_ctl.r_mute;
        printf("%s, r_mute:%d, charge_flag:%d", __func__, *mute, charge_flag);
    } else {
        printf("%s, l_mute:%d, charge_flag:%d", __func__, *mute, charge_flag);
    }

    if (*mute != charge_flag) {
        *mute = charge_flag;
        google_tone_play_deal();
    }
}

#if TCFG_USER_TWS_ENABLE
int gfps_protocol_tws_send_to_sibling(u16 opcode, u8 *data, u16 len)
{
    return tws_api_send_data_to_sibling(data, len, 0x23080315);
}

static void __tws_rx_from_sibling(u8 *data, int len)
{
    gfps_tws_data_deal(data, len);

    switch (data[0]) {
    case GFPS_SIBLING_SYNC_TYPE_ADV_MAC_STATE:
        gfps_adv_interval_set(data[1] ? SUBS_PAIR_ADV_INTERVAL : INIT_PAIR_ADV_INTERVAL);
        set_gfps_pair_state(data[1]);
        gfps_rfcomm_connect_state_set(data[2]);
        app_ble_set_mac_addr(gfps_app_ble_hdl, (void *)&data[3]);
        gfps_bt_ble_adv_enable(0);
        gfps_bt_ble_adv_enable(1);
        break;
    }
    free(data);
}

static void gfps_protocol_rx_from_sibling(void *_data, u16 len, bool rx)
{
    int err = 0;
    if (rx) {
        printf(">>>%s \n", __func__);
        printf("len :%d\n", len);
        put_buf(_data, len);

        u8 *rx_data = malloc(len);
        if (!rx_data) {
            return;
        }

        memcpy(rx_data, _data, len);

        int msg[4];
        msg[0] = (int)__tws_rx_from_sibling;
        msg[1] = 2;
        msg[2] = (int)rx_data;
        msg[3] = (int)len;
        err = os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
        if (err) {
            printf("tws rx post fail\n");
        }
    }
}

//发送给对耳
REGISTER_TWS_FUNC_STUB(app_vol_sync_stub) = {
    .func_id = 0x23080315,
    .func    = gfps_protocol_rx_from_sibling,
};
#endif

#if 0  // for anc test

#define GFPS_ANC_TRANSPARENT_MODE   0x80
#define GFPS_OFF_MODE               0x20
#define GFPS_ANC_MODE               0x08
#define GFPS_ANC_ALL_MODE           (GFPS_ANC_TRANSPARENT_MODE | GFPS_OFF_MODE | GFPS_ANC_MODE)
extern void gfps_hearable_controls_update(u8 display_flag, u8 settable_flag, u8 current_state);
u8 cur_anc_state = GFPS_OFF_MODE;

void test_gfps_anc_state_switch(void)
{
    if (cur_anc_state == GFPS_OFF_MODE) {
        cur_anc_state = GFPS_ANC_MODE;
        printf("test ANC : OFF -> ANC !\n");
    } else if (cur_anc_state == GFPS_ANC_MODE) {
        cur_anc_state = GFPS_ANC_TRANSPARENT_MODE;
        printf("test ANC : ANC -> TRANS !\n");
    } else if (cur_anc_state == GFPS_ANC_TRANSPARENT_MODE) {
        cur_anc_state = GFPS_OFF_MODE;
        printf("test ANC : TRANS -> OFF !\n");
    }
    gfps_hearable_controls_update(GFPS_ANC_ALL_MODE, GFPS_ANC_ALL_MODE, cur_anc_state);
}

#endif // for anc test

int gfps_message_deal_handler(int id, int opcode, u8 *data, u32 len)
{
    switch (opcode) {
    case APP_PROTOCOL_LIB_TWS_DATA_SYNC:
#if TCFG_USER_TWS_ENABLE
        gfps_protocol_tws_send_to_sibling(opcode, data, len);
#endif
        break;
    case APP_PROTOCOL_GFPS_RING_STOP_ALL:
        printf("GFPS_RING_STOP_ALL");
        google_tone_ctl.l_en = 0;
        google_tone_ctl.r_en = 0;
        google_tone_play_deal();
        break;
    case APP_PROTOCOL_GFPS_RING_RIGHT:
        printf("GFPS_RING_RIGHT");
        google_tone_ctl.l_en = 0;
        google_tone_ctl.r_en = 1;
        google_tone_play_deal();
        break;
    case APP_PROTOCOL_GFPS_RING_LEFT:
        printf("GFPS_RING_LEFT");
        google_tone_ctl.l_en = 1;
        google_tone_ctl.r_en = 0;
        google_tone_play_deal();
        break;
    case APP_PROTOCOL_GFPS_RING_ALL:
        printf("GFPS_RING_ALL");
        google_tone_ctl.l_en = 1;
        google_tone_ctl.r_en = 1;
        google_tone_play_deal();
        break;
    case APP_PROTOCOL_GFPS_HEARABLE_CONTROLS:
        printf("APP_PROTOCOL_GFPS_HEARABLE_CONTROLS display:%x settable:%x current:%x\n", data[0], data[1], data[2]);
#if 0  // for anc test
        cur_anc_state = data[2];
#endif // for anc test

        break;
    }
    return 0;
}

//关机广播收仓
void gfps_need_adv_close_icon_set(u8 en)
{
    printf("func:%s, en:%d", __FUNCTION__, en);
    __this->close_icon = en;
}

void gfps_poweroff_adv_close_icon()
{
    printf("func:%s, line:%d", __FUNCTION__, __LINE__);
    if (__this->close_icon) {
        void set_wait_poweroff_time(u16 time);
        set_wait_poweroff_time(2000); //广播800ms
        gfps_adv_interval_set(48); //连接上手机间隔改为30ms
        gfps_set_battery_ui_enable(0); //关闭电量显示，会重新开关广播
    }
}

static int gfps_app_msg_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_BT_IN_PAIRING_MODE:
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#endif
        if (bt_get_total_connect_dev() == 0) {
            gfps_adv_interval_set(INIT_PAIR_ADV_INTERVAL); //连接上手机间隔改为93.75ms
            if (__this->poweron_reconn || ADV_INIT_PAIR_IN_PAIR_MODE) { //开机回连超时后，广播初始配对广播
                __this->poweron_reconn = 0;
                gfps_set_pair_mode_by_user(0);
            }
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(gfps_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = gfps_app_msg_handler,
};

static int gfps_bt_status_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        if (btstack_get_num_of_remote_device_recorded()) {
            __this->poweron_reconn = 1;
        }
        break;
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        __this->poweron_reconn = 0;
        gfps_adv_interval_set(SUBS_PAIR_ADV_INTERVAL); //连接上手机间隔改为220ms，防止fast pair validator测试fail
        set_gfps_pair_state(1);
        gfps_bt_ble_adv_enable(0);
        gfps_bt_ble_adv_enable(1);
        break;
    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        gfps_ble_adv_enable(0);
        break;
    case BT_STATUS_PHONE_HANGUP:
        gfps_ble_adv_enable(1);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(gfps_stack_msg_handler) = {
    .owner      = 0xFF,
    .from       = MSG_FROM_BT_STACK,
    .handler    = gfps_bt_status_event_handler,
};

int gfps_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        switch (bt->value) {
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            printf("GFP linkkey missing");
            if (!btstack_get_num_of_remote_device_recorded()) { //没有配对记录的时候，广播初始配对广播
                gfps_all_info_recover(0);
                gfps_adv_interval_set(INIT_PAIR_ADV_INTERVAL); //连接上手机间隔改为93.75ms
                gfps_set_pair_mode_by_user(0);
            }
            break;
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(gfps_hci_msg_handler) = {
    .owner      = 0xFF,
    .from       = MSG_FROM_BT_HCI,
    .handler    = gfps_hci_event_handler,
};

void gfps_sync_info_to_new_master(void)
{
    u8 buf[8];

#if TCFG_USER_TWS_ENABLE
    if (!get_bt_tws_connect_status()) {
        printf("gfps_sync_info tws not connected\n");
        return;
    }
#endif

    u8 *adv_addr = app_ble_adv_addr_get(gfps_app_ble_hdl);
    if (adv_addr == NULL) {
        printf("gfps_sync_info adv addr NULL\n");
        return;
    }
    buf[0] = get_gfps_pair_state();
    buf[1] = gfps_rfcomm_connect_state_get();
    memcpy(buf + 2, adv_addr, 6);
    gfps_sibling_data_send(GFPS_SIBLING_SYNC_TYPE_ADV_MAC_STATE, buf, sizeof(buf));
    printf("gfps_send_pair_state:%d, spp_state:%d, ble_addr:", buf[0], buf[1]);
    put_buf(buf + 2, 6);
}

static int gfps_tws_msg_handler(int *_msg)
{
    struct tws_event *evt = (struct tws_event *)_msg;
    int role = evt->args[0];
    int reason = evt->args[2];

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        if (role == TWS_ROLE_MASTER) {
            gfps_sibling_sync_info();
        } else {
            gfps_disconnect(NULL);
            gfps_ble_adv_enable(0);
        }
        break;
    case TWS_EVENT_ROLE_SWITCH:
        //对耳主从切换时，将旧主机的蓝牙地址和pair_state同步给新主机
        gfps_disconnect(NULL);
        if (role == TWS_ROLE_SLAVE) {
            gfps_ble_adv_enable(0);
            gfps_sync_info_to_new_master();
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        google_tone_ctl.l_en = 0;
        google_tone_ctl.r_en = 0;
        google_tone_play_deal();
        if (reason & TWS_DETACH_BY_POWEROFF) {
            printf("%s, role:%d, poweroff:%d", __func__, role, app_var.goto_poweroff_flag);
            if (role == TWS_ROLE_MASTER && app_var.goto_poweroff_flag) {
                gfps_poweroff_adv_close_icon();
            }
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(gfps_tws_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_TWS,
    .handler = gfps_tws_msg_handler,
};

static int gfps_app_power_event_handler(int *msg)
{
    switch (msg[0]) {
    case POWER_EVENT_SYNC_TWS_VBAT_LEVEL:
        printf("update gfps bat");
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_bat_persent() != 0xFF) {
            google_tone_mute_ctl(0, get_tws_sibling_bat_persent() >> 7);
        }
#endif
        gfps_battery_update();
        break;
    case POWER_EVENT_POWER_CHANGE:
        printf("update gfps bat");
        google_tone_mute_ctl(1, get_charge_online_flag());
        gfps_battery_update();
        break;
    }
    return 0;
}
APP_MSG_HANDLER(gfps_bat_level_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = gfps_app_power_event_handler,
};

#endif

