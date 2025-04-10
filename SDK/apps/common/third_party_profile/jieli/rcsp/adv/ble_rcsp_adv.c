#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_rcsp_adv.data.bss")
#pragma data_seg(".ble_rcsp_adv.data")
#pragma const_seg(".ble_rcsp_adv.text.const")
#pragma code_seg(".ble_rcsp_adv.text")
#endif
/*********************************************************************************************
    *   Filename        : ble_rcsp_adv.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-06-08 11:14

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
// *****************************************************************************
#include "app_config.h"
#include "ble_rcsp_adv.h"
#if RCSP_MODE != RCSP_MODE_OFF
#include "system/includes.h"
#include "rcsp_config.h"
#include "app_action.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "app_chargestore.h"

#include "btcrypt.h"
#include "custom_cfg.h"
#include "rcsp_music_info_setting.h"
#include "ble_rcsp_server.h"
#include "rcsp_manage.h"
#include "rcsp_bt_manage.h"
#include "rcsp_adv_bluetooth.h"
#include "rcsp_update.h"
#include "btstack/avctp_user.h"
#include "multi_protocol_main.h"
#include "JL_rcsp_api.h"
#include "rcsp_config.h"
#include "btstack_rcsp_user.h"

#if	TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#include "classic/tws_api.h"
#endif

#if RCSP_MODE == RCSP_MODE_EARPHONE
#include "earphone.h"
#elif RCSP_MODE == RCSP_MODE_SOUNDBOX
#include "soundbox.h"
#endif

#include "asm/charge.h"

#if TCFG_USER_BLE_CTRL_BREDR_EN
#define VER_FLAG_BLE_CTRL_BREDR					BIT(0) // 先连接ble再连接edr
#else
#define VER_FLAG_BLE_CTRL_BREDR					0
#endif

#define VER_FLAG_IOS_BLE_LINK_BREDR				BIT(1) // ios一次连接

#if 1
#define log_info(x, ...)       printf("[BLE-RCSP-ADV]" x " ", ## __VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

extern void *rcsp_server_ble_hdl;
extern void *rcsp_server_ble_hdl1;

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

static u16 ble_timer_handle = 0;
u16 sibling_ver_info = 0;
u16 cur_ver_info  = 0;

#define BLE_TIMER_SET (500)
/* #define BLE_TIMER_SET (5000) */

struct _bt_adv_handle {

    u8 seq_rand;
    u8 connect_flag;

    u8 bat_charge_L;
    u8 bat_charge_R;
    u8 bat_charge_C;
    u8 bat_percent_L;
    u8 bat_percent_R;
    u8 bat_percent_C;

    u8 modify_flag;
    u8 adv_disable_cnt;

    u8 ble_adv_notify;
} bt_adv_handle = {0};

#define __this (&bt_adv_handle)

extern u8 get_tws_sibling_bat_persent(void);

static void rcsp_adv_fill_mac_addr(u8 *mac_addr_buf)
{
    swapX(bt_get_mac_addr(), mac_addr_buf, 6);
}

#if (CONFIG_CPU_BR27 || CONFIG_CPU_BR28 || CONFIG_CPU_BR50) && ((RCSP_MODE == RCSP_MODE_SOUNDBOX) || (RCSP_MODE == RCSP_MODE_EARPHONE))
extern int JL_AES_BASE_BT;
int JL_AES_BASE_BT = (int)JL_AES;	// add for btcon_hash, by lingxuanfeng, 20220517
#endif
int rcsp_make_set_adv_data(void)
{
    u8 i;
    u8 *buf = adv_data;
    buf[0] = 0x1E;
    buf[1] = 0xFF;

    buf[2] = 0xD6;	// JL ID
    buf[3] = 0x05;

    u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    buf[4] = vid & 0xFF;
    buf[5] = vid >> 8;

    u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    buf[6] = pid & 0xFF;
    buf[7] = pid >> 8;

#if RCSP_MODE == RCSP_MODE_EARPHONE

    buf[8] = 0x20;	//   2:TWS耳机类型   |  protocol verson

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    buf[8] |= 4;
#else
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[8] |= 2;
    }
#endif

#if (RCSP_ADV_HALTER_ENABLE)
    buf[8] = 0x23;
#endif

#endif // RCSP_MODE == RCSP_MODE_EARPHONE

#if RCSP_MODE == RCSP_MODE_SOUNDBOX
#if (SOUNDCARD_ENABLE)
    buf[8] = 0x40;	//   4:声卡类型   |  protocol verson
#else
    buf[8] = 0x0;	//   0:音箱类型   |  protocol verson
#endif
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[8] |= 2;
    }
#endif


#if RCSP_MODE == RCSP_MODE_WATCH
    buf[8] = 0x50 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// 手表类型
#endif

    rcsp_adv_fill_mac_addr(&buf[9]);
#if RCSP_MODE == RCSP_MODE_WATCH
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[15] = 1;
    }
#else
    /* printf("connect_flag %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, __this->connect_flag); */
    buf[15] = __this->connect_flag;

    buf[16] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[17] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[18] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;

    memset(&buf[19], 0x00, 4);							// reserve

    buf[19] = __this->seq_rand;

    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[20] = 1;
    }

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    buf[20] |= BIT(2);  // 是否支持Le Audio功能
    if (is_cig_phone_conn()) {
        buf[20] |= BIT(3);  // Le Audio是否已连接
    }
#endif

    u8 t_buf[16];
    btcon_hash(&buf[2], 16, &buf[15], 4, t_buf);		// hash
    for (i = 0; i < 8; i++) {								// single byte
        buf[23 + i] = t_buf[2 * i + 1];
    }
#endif // RCSP_MODE == RCSP_MODE_WATCH

    __this->modify_flag = 0;
    adv_data_len = 31;
    /* ble_op_set_adv_data(31, buf); */
    app_ble_adv_data_set(rcsp_server_ble_hdl, buf, 31);
    app_ble_adv_data_set(rcsp_server_ble_hdl1, buf, 31);

    log_info("ADV data():");
    log_info_hexdump(buf, 31);
    return 0;
}

int rcsp_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;
    const char *edr_name = bt_get_local_name();

#if DOUBLE_BT_SAME_MAC || TCFG_BT_BLE_BREDR_SAME_ADDR
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x0A, 1);
#else
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
#endif

    u8 name_len = strlen(edr_name) + 1;
    if (offset + 2 + name_len > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, name_len);
    scan_rsp_data_len = offset;
    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    /* ble_op_set_rsp_data(offset, buf); */
    app_ble_rsp_data_set(rcsp_server_ble_hdl, buf, 31);
    app_ble_rsp_data_set(rcsp_server_ble_hdl1, buf, 31);
    return 0;
}

/**
 *	@brief 连接后，通过这个通知手机设备状态
 */
static int update_adv_data(u8 *buf)
{
    u8 i;
    buf[1] = 0xD6;	// JL ID
    buf[0] = 0x05;

    u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    buf[3] = vid & 0xFF;
    buf[2] = vid >> 8;

    u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    buf[5] = pid & 0xFF;
    buf[4] = pid >> 8;

#if RCSP_MODE == RCSP_MODE_EARPHONE

    buf[6] = 0x20;	//   2:TWS耳机类型   |  protocol verson
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[6] |= 2;
    }
#if (RCSP_ADV_HALTER_ENABLE)
    buf[6] = 0x23;
#endif

#endif // RCSP_MODE == RCSP_MODE_EARPHONE

#if RCSP_MODE == RCSP_MODE_SOUNDBOX
#if (SOUNDCARD_ENABLE)
    buf[6] = 0x40;	//   4:声卡类型   |  protocol verson
#else
    buf[6] = 0x0;	//   0:音箱类型   |  protocol verson
#endif
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[6] |= 2;
    }
#endif

#if RCSP_MODE == RCSP_MODE_WATCH
    buf[6] = 0x50 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// 手表类型
#endif

    swapX(bt_get_mac_addr(), &buf[7], 6);

#if RCSP_MODE == RCSP_MODE_WATCH
    if (RCSP_USE_SPP == get_defalut_bt_channel_sel()) {
        buf[13] = 1;
    }
#else

#if TCFG_RCSP_DUAL_CONN_ENABLE // 一拖二
    buf[13] = SECNE_DISMISS;
#else
    buf[13] = __this->connect_flag;
#endif

    buf[14] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[15] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[16] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;

    buf[17] = __this->seq_rand;
#endif

    return 0;
}

#if TCFG_PAY_ALIOS_ENABLE
int upay_ble_adv_data_set(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;
    u8 *edr_name = bt_get_local_name();
    u8 service_data[8];
    u8 tmp_data[6];

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    service_data[0] = 0x02;
    service_data[1] = 0x38;
    le_controller_get_mac(tmp_data);
    swapX(tmp_data, &service_data[2], 6);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_SERVICE_DATA, service_data, 8);
    adv_data_len = offset;
    ble_op_set_adv_data(offset, buf);

    log_info("upay ADV data(%d):", adv_data_len);
    log_info_hexdump(buf, offset);

    buf = scan_rsp_data;
    offset = 0;
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name));

    scan_rsp_data_len = offset;
    ble_op_set_rsp_data(offset, buf);

    log_info("upay RSP data(%d):", scan_rsp_data_len);
    log_info_hexdump(buf, offset);

    return 0;
}
#endif

static u8 update_dev_battery_level(void)
{

    u8 master_bat = 0;
    u8 master_charge = 0;
    u8 slave_bat = 0;
    u8 slave_charge = 0;
    u8 box_bat = 0;
    u8 box_charge = 0;

//	Master bat
#if CONFIG_DISPLAY_DETAIL_BAT
    master_bat = get_vbat_percent();
#else
    master_bat = get_self_battery_level() * 10 + 10;
#endif
    if (master_bat > 100) {
        master_bat = 100;
    }
    master_charge = get_charge_online_flag();


// Slave bat

#if	TCFG_USER_TWS_ENABLE
    slave_bat = get_tws_sibling_bat_persent();
#if TCFG_CHARGESTORE_ENABLE
    if (slave_bat == 0xff) {
        /* log_info("--update_bat_01\n"); */
        if (get_bt_tws_connect_status()) {
            slave_bat = chargestore_get_sibling_power_level();
        }
    }
#endif

    if (slave_bat == 0xff) {
        /*  log_info("--update_bat_02\n"); */
        slave_bat = 0x00;
    }

    slave_charge = !!(slave_bat & 0x80);
    slave_bat &= 0x7F;
#else
    slave_charge = 0;
    slave_bat = 0;
#endif

// Box bat
    if ((master_charge && (master_bat != 0))
        || (slave_charge && (slave_bat != 0))) {
        //earphone in charge
#if TCFG_CHARGESTORE_ENABLE
        box_bat = chargestore_get_power_level();
        if (box_bat == 0xFF) {
            box_bat = 0;
        }
#else
        box_bat = 0;
#endif
    } else {
        //all not in charge
        box_bat = 0;
    }
    box_charge = !!(box_bat & 0x80);
    box_bat &= 0x7F;
// check if master is L
    u8 tbat_charge_L = 0;
    u8 tbat_charge_R = 0;
    u8 tbat_percent_L = 0;
    u8 tbat_percent_R = 0;
    u8 tbat_percent_C = box_bat;
    u8 tbat_charge_C = box_charge;
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        if ('L' == tws_api_get_local_channel()) {
            tbat_charge_L = master_charge;
            tbat_charge_R = slave_charge;
            tbat_percent_L = master_bat;
            tbat_percent_R = slave_bat;
        } else if ('R' == tws_api_get_local_channel()) {
            tbat_charge_L = slave_charge;
            tbat_charge_R = master_charge;
            tbat_percent_L = slave_bat;
            tbat_percent_R = master_bat;
        }
    } else {
        switch (tws_api_get_local_channel()) {
        case 'R':
            tbat_charge_R = master_charge;
            tbat_percent_R = master_bat;
            break;
        case 'L':
        default:
            tbat_charge_L = master_charge;
            tbat_percent_L = master_bat;
            break;
        }
    }
#else
    tbat_charge_L = master_charge;
    tbat_percent_L = master_bat;
#endif

#if 0//TCFG_CHARGESTORE_ENABLE
    u8 tpercent = 0;
    u8 tcharge = 0;
    if (chargestore_get_earphone_pos() == 'L') {
        //switch position
        log_info("is L pos\n");
        tpercent = tbat_percent_R;
        tcharge = tbat_charge_R;
        tbat_percent_R = tbat_percent_L;
        tbat_charge_R = tbat_charge_L;
        tbat_percent_L = tpercent;
        tbat_charge_L = tcharge;
    } else {
        log_info("is R pos\n");
    }
#endif

    if ((!!__this->bat_charge_L) || (!!__this->bat_charge_R) || (!!__this->bat_charge_C)) {
        if (((__this->bat_percent_C + 1) & 0x7F) <= 0x2) {
            __this->bat_percent_C = 0x2;
        }
    }

    // 防止充电仓电量跳变
    if (__this->bat_percent_C && ((!!tbat_charge_L) || (!!tbat_charge_R))) {
        // 左耳或右耳进仓
        if ((!!tbat_charge_C)) {
            // 充电仓充电, 新值比旧值大
            if ((__this->bat_percent_C > tbat_percent_C) && (__this->bat_percent_C == (tbat_percent_C + 1))) {
                tbat_percent_C = __this->bat_percent_C;
            }
        } else {
            // 充电仓没有充电, 新值比旧值小
            if ((__this->bat_percent_C < tbat_percent_C) && ((__this->bat_percent_C + 1) == tbat_percent_C)) {
                tbat_percent_C = __this->bat_percent_C;
            }
        }
    }
// check if change
    u8 tchange = 0;
    if ((tbat_charge_L  != __this->bat_charge_L)
        || (tbat_charge_R  != __this->bat_charge_R)
        || (tbat_charge_C  != __this->bat_charge_C)
        || (tbat_percent_L != __this->bat_percent_L)
        || (tbat_percent_R != __this->bat_percent_R)
        || (tbat_percent_C != __this->bat_percent_C)) {
        tchange = 1;
    }

    __this->bat_charge_L  = tbat_charge_L;
    __this->bat_charge_R  = tbat_charge_R;
    __this->bat_charge_C  = tbat_charge_C;
    __this->bat_percent_L = tbat_percent_L;
    __this->bat_percent_R = tbat_percent_R;
    __this->bat_percent_C = tbat_percent_C;

    return tchange;
}

void bt_ble_init_do(void)
{
    log_info("bt_ble_test_tag_do\n");
    //初始化三个电量值
    /* swapX(bt_get_mac_addr(), &user_tag_data[7], 6); */

    __this->connect_flag = SECNE_DISMISS;
    __this->bat_charge_L = 1;
    __this->bat_charge_R = 1;
    __this->bat_charge_C = 1;
    __this->bat_percent_L = 100;
    __this->bat_percent_R = 100;
    __this->bat_percent_C = 100;
    __this->modify_flag = 0;
    __this->ble_adv_notify = 0;
    update_dev_battery_level();
    bt_adv_seq_change();
}

// 一般用于设置推送内容
int bt_ble_adv_ioctl(u32 cmd, u32 priv, u8 mode)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, rets=0x%x\n", __FUNCTION__, rets_addr);

#if RCSP_MODE == RCSP_MODE_WATCH
    return 0;
#endif
    log_info(" bt_ble_adv_ioctl %d %d %d\n", cmd, priv, mode);
    if (mode) {			// set
        switch (cmd) {
        case BT_ADV_ENABLE:
            break;
        case BT_ADV_DISABLE:
            log_info("ADV DISABLE !!!\n");
            rcsp_bt_ble_adv_enable(0);
            __this->connect_flag = SECNE_DISMISS;
            break;
        case BT_ADV_SET_EDR_CON_FLAG:
            __this->connect_flag = priv;
            if (priv == SECNE_UNCONNECTED) {
                bt_adv_seq_change();
            }
#if (RCSP_ADV_MUSIC_INFO_ENABLE)
            extern void rcsp_adv_music_info_set_state(u8 state, u32 time);
            if (priv == SECNE_UNCONNECTED) {
                rcsp_adv_music_info_set_state(0, 1);
            } else if (priv == SECNE_CONNECTED) {
                rcsp_adv_music_info_set_state(1, 1000);
            }
#endif
            break;
        case BT_ADV_SET_BAT_CHARGE_L:
            __this->bat_charge_L = priv;
            break;
        case BT_ADV_SET_BAT_CHARGE_R:
            __this->bat_charge_R = priv;
            break;
        case BT_ADV_SET_BAT_CHARGE_C:
            __this->bat_charge_C = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_L:
            __this->bat_percent_L = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_R:
            __this->bat_percent_R = priv;
            break;
        case BT_ADV_SET_BAT_PERCENT_C:
            __this->bat_percent_C = priv;
            break;
        case BT_ADV_SET_NOTIFY_EN:
            set_ble_adv_notify(priv);
            break;
        default:
            return -1;
        }

        if (ble_timer_handle) {
            __this->modify_flag = 1;
        }
    } else {			// get
        // do nothing
    }
    return 0;
}

static void bt_ble_rcsp_adv_enable_do(void *priv)
{
#if TCFG_USER_TWS_ENABLE

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        /* printf("%s, %s, it's slave\n", __FILE__, __FUNCTION__); */
        return;
        /* if (bt_rcsp_spp_conn_num() == 0) { */
        /* 	// 如果从机不是spp连接，则返回 */
        /*     return; */
        /* } */
    }
#endif

#if RCSP_UPDATE_EN
    extern u32 classic_update_task_exist_flag_get(void);
    if (classic_update_task_exist_flag_get()) {
        return;
    }
#endif

    /* printf("modify_flag:%d, ble_adv_notify:%d\n", __this->modify_flag, __this->ble_adv_notify); */

    // battery
    if (update_dev_battery_level()) {
        __this->modify_flag = 1;
    }

    extern u8 adv_info_notify(u8 * buf, u8 len);
#if (RCSP_ADV_EN) && (TCFG_USER_TWS_ENABLE)
    if (get_rcsp_connect_status() && JL_rcsp_get_auth_flag()) {
#if TCFG_USER_TWS_ENABLE
        extern void deal_adv_setting_gain_time_stamp(void);
        deal_adv_setting_gain_time_stamp();
#endif // TCFG_USER_TWS_ENABLE
    }
#endif
    u8 max_con_dev = rcsp_max_support_con_dev_num();
    u8 rcsp_conn_num = bt_rcsp_device_conn_num();
    // 判断是否已连接并进入RCSP通讯
    if ((rcsp_conn_num > 0) && JL_rcsp_get_auth_flag() && (__this->ble_adv_notify || __this->modify_flag)) {
        u8 data[18];
        update_adv_data(data);
        adv_info_notify(data, 18);
        if (rcsp_conn_num >= max_con_dev) {
            __this->modify_flag = 0;
        }
        set_ble_adv_notify(0);
    }

    // 如果设备连接数最大，则不开启广播
    if (rcsp_conn_num >= max_con_dev) {
        return;
    }

    if (__this->modify_flag == 0) {
        return;
    }

    log_info("adv modify!!!!!!\n");
    __this->modify_flag = 0;
    rcsp_bt_ble_adv_enable(0);
    rcsp_make_set_adv_data();
    rcsp_make_set_rsp_data();
    rcsp_bt_ble_adv_enable(1);
}

void bt_ble_rcsp_adv_enable(void)
{
    if (ble_timer_handle == 0) {
        ble_timer_handle = sys_timer_add(NULL, bt_ble_rcsp_adv_enable_do, BLE_TIMER_SET);
    }
}

u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (len) {
        *len = scan_rsp_data_len;
    }
    return scan_rsp_data;
}

u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (len) {
        *len = adv_data_len;
    }
#if RCSP_UPDATE_EN
    adv_data[15] = 1;
    adv_data[20] = 0;
    u8 t_buf[16];
    btcon_hash(&adv_data[2], 16, &adv_data[15], 4, t_buf);
    for (u8 i = 0; i < 8; i++) {
        adv_data[23 + i] = t_buf[2 * i + 1];
    }
#endif
    return adv_data;
}

u8 get_connect_flag(void)
{
    return __this->connect_flag;
}

void set_connect_flag(u8 value)
{
    __this->connect_flag = value;
    /* printf("connect_flag %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, __this->connect_flag); */
}

void bt_ble_rcsp_adv_disable_timer(void)
{
    if (ble_timer_handle) {
        sys_timer_del(ble_timer_handle);
        ble_timer_handle = 0;
        __this->modify_flag = 0;
        set_ble_adv_notify(0);
    }
}

void bt_ble_rcsp_adv_disable(void)
{
    bt_ble_rcsp_adv_disable_timer();
    ble_app_disconnect();
    rcsp_bt_ble_adv_enable(0);
    bt_adv_seq_change();
}

void tws_conn_switch_role();
static void deal_sibling_seq_rand_sync_in_task(void *data, u16 len)
{
    u8 tws_need_update = 1;
    log_info_hexdump(data, len);
    switch (((u8 *)data)[0]) {
    case TWS_ADV_SEQ_CHANGE:
        __this->seq_rand = ((u8 *) data)[1];
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_SEQ, NULL, 0);
        break;
    case TWS_VERSON_INFO:
        sibling_ver_info  = ((u8 *)data)[1] | ((u8 *)data)[2] << 8;
        cur_ver_info  = get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
        log_info("get sibling version:0x%x cur_ver_info:0x%x\n", ((u8 *)data)[1] | ((u8 *)data)[2] << 8, cur_ver_info);
        if ((cur_ver_info > sibling_ver_info) && (tws_api_get_role() == TWS_ROLE_MASTER)) { //如果主机版本号比从机高需要进行role_switch
            /* if(bt_tws_phone_connected()) { */
            /*     rcsp_update_set_role_switch(1); */
            /* } else {             */
            /*     g_printf("rcsp_need role switch\n");                    //已经连接上手机在此处role_switch */
            /*     tws_conn_switch_role(); */
            /*     tws_api_auto_role_switch_disable(); */
            /* } */
        } else if (cur_ver_info == sibling_ver_info) {                  //如果tws连接同步版本号一致需要把标志清楚防止重复升级
            tws_need_update = 0;
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_TWS_NEED_UPDATE, &tws_need_update, 1);
        }

        break;
#if RCSP_MODE == RCSP_MODE_EARPHONE
    case TWS_UPDATE_INFO:                                               //单备份升级走BLE主机需要要通过该消息来通知从机进入升级
        bt_ble_rcsp_adv_disable();
        ble_module_enable(0);                                           //关闭广播防止从机被手机误回连
        g_printf("slave close adv...\n");
        sys_timeout_add(NULL,  update_slave_adv_reopen, 1000 * 60);     //延迟一分钟再开广播
        sibling_ver_info  = ((u8 *)data)[1] | ((u8 *)data)[2] << 8;
        cur_ver_info  = get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
        if (cur_ver_info <= sibling_ver_info) {
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_TWS_NEED_UPDATE, &tws_need_update, 1);
            extern void sys_auto_shut_down_disable(void);
            sys_auto_shut_down_disable();
        }
        break;
#endif
    default:
        break;
    }
    free(data);
}
static void deal_sibling_seq_rand_sync(void *data, u16 len, bool rx)
{
    if (rx) {
        u8 *buf = malloc(len);
        int msg[] = {(int)deal_sibling_seq_rand_sync_in_task, 2, (int)buf, len};

        memcpy(buf, data, len);
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    }
}
REGISTER_TWS_FUNC_STUB(adv_seq_rand_sync) = {
    .func_id = TWS_FUNC_ID_SEQ_RAND_SYNC,
    .func    = deal_sibling_seq_rand_sync,
};
void adv_seq_vaule_sync(void)
{
    syscfg_write(ADV_SEQ_RAND, &__this->seq_rand, sizeof(__this->seq_rand));
}
void bt_adv_seq_change(void)
{
    u8 trand;
    u8 data[2];
    syscfg_read(ADV_SEQ_RAND, &trand, 1);
    log_info("adv seq read: %x\n", trand);
    trand++;
    syscfg_write(ADV_SEQ_RAND, &trand, 1);
    log_info("adv seq write: %x\n", trand);
    __this->seq_rand = trand;
    data[0] = TWS_ADV_SEQ_CHANGE;
    data[1] = __this->seq_rand;
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status() && tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_api_send_data_to_sibling(data, sizeof(data), TWS_FUNC_ID_SEQ_RAND_SYNC);
    }
#endif
}

u8 get_ble_adv_modify(void)
{
    return __this->modify_flag;
}

u8 get_ble_adv_notify(void)
{
    return __this->ble_adv_notify;
}

#if TCFG_USER_TWS_ENABLE
#define TWS_FUNC_ID_RCSP_ADV_NOTIFY_TWS_SYNC \
	(((u8)('R' + 'C' + 'S' + 'P') << (3 * 8)) | \
	 ((u8)('A' + 'D' + 'V') << (2 * 8)) | \
	 ((u8)('N' + 'T' + 'F') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))
static void rcsp_adv_notify_tws_sync_in_irq(void *_data, u16 len, bool rx)
{
    if (rx) {
        __this->ble_adv_notify = *(u8 *)_data;
        printf("%s, %d, adv_notify:%d\n", __FUNCTION__, __LINE__, __this->ble_adv_notify);
    }
}
REGISTER_TWS_FUNC_STUB(tws_rcsp_adv_notify_sync) = {
    .func_id = TWS_FUNC_ID_RCSP_ADV_NOTIFY_TWS_SYNC,
    .func = rcsp_adv_notify_tws_sync_in_irq,
};
#endif
void set_ble_adv_notify(u8 en)
{
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));
    printf("%s, %d, rets=0x%x\n", __FUNCTION__, en, rets_addr);
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status() && TWS_ROLE_MASTER == tws_api_get_role()) {
        u8 *buf = &en;
        tws_api_send_data_to_sibling(buf, 1, TWS_FUNC_ID_RCSP_ADV_NOTIFY_TWS_SYNC);
    }
#endif
    __this->ble_adv_notify = en;
}

void bt_adv_get_bat(u8 *buf)
{
    buf[0] = __this->bat_percent_L ? (((!!__this->bat_charge_L) << 7) | (__this->bat_percent_L & 0x7F)) : 0;
    buf[1] = __this->bat_percent_R ? (((!!__this->bat_charge_R) << 7) | (__this->bat_percent_R & 0x7F)) : 0;
    buf[2] = __this->bat_percent_C ? (((!!__this->bat_charge_C) << 7) | (__this->bat_percent_C & 0x7F)) : 0;
}

extern u8 jl_app_init_setting(void);
u8 btstck_app_init_setting(void)
{
    jl_app_init_setting();
    return 1;
}

/* #if TCFG_USER_TWS_ENABLE */
/* #define TWS_FUNC_ID_ADV_STATE_INFO_SYNC	(('A' << (3 * 8)) | ('S' << (2 * 8)) | ('S' << (1 * 8)) | ('Y')) */
/* static void deal_sibling_adv_state_info_sync(void *data, u16 len, bool rx) */
/* { */
/*     if (rx) { */
/*         JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDAET_ADV_STATE_INFO, data, len); */
/*     } */
/* } */
/* REGISTER_TWS_FUNC_STUB(adv_state_sync) = { */
/*     .func_id = TWS_FUNC_ID_ADV_STATE_INFO_SYNC, */
/*     .func    = deal_sibling_adv_state_info_sync, */
/* }; */
/* static void adv_state_info_sync(void) */
/* { */
/*     if (get_bt_tws_connect_status()) { */
/*         u8 param[2] = {0}; */
/*         param[0] = JL_rcsp_get_auth_flag(); */
/*         param[1] = __this->ble_adv_notify; */
/*         tws_api_send_data_to_sibling(param, sizeof(param), TWS_FUNC_ID_ADV_STATE_INFO_SYNC); */
/*     } */
/* } */
/* #endif */

// type : 0 - 正常关机检查		// 1 - 低电关机检测
u8 adv_tws_both_in_charge_box(u8 type)
{
    if (0 == ((!!__this->bat_percent_L) & (!!__this->bat_percent_R))) {
        return 0;
    }
    static u8 ret = 0;
    if (ret) {
        goto check_changes;
    }

    if (type) {
#if TCFG_USER_TWS_ENABLE
        if (TWS_ROLE_MASTER == tws_api_get_role()) {
            ret = 1;
        }
#endif
        return ret;
    }

    ret = ((!!__this->bat_charge_L) ^ (!!__this->bat_charge_R));
check_changes:
    if (bt_rcsp_ble_conn_num()) {
        return ret;
    }
    /* if (get_rcsp_connect_status() && ret) { */
    /*     adv_state_info_sync(); */
    /* } */
    return 0;
}

// 切换后触发
void adv_role_switch_handle(u8 role)
{
#if TCFG_USER_TWS_ENABLE
    // 当充电入仓的时候，入仓的主机设备role是1但tws_api_get_role()是0；
    printf("adv_role_switch_handle rcsp role change:%d, %d, %d\n", role, tws_api_get_role(), bt_rcsp_device_conn_num());
    if (tws_api_get_tws_state()) {
        // 设备连接后，主从切换需要spp手机app来请求固件信息
        if ((role == TWS_ROLE_MASTER) && \
            ((bt_rcsp_spp_conn_num() > 0) || (bt_rcsp_ble_conn_num() > 0))) {
            u8 adv_cmd = 0x4;
            adv_info_device_request(&adv_cmd, sizeof(adv_cmd));             //让手机来请求固件信息
        }

        // 如果还需要开广播 并且 一拖二的时候ble还没有连接
        if (bt_rcsp_device_conn_num() < rcsp_max_support_con_dev_num()) {
            if (role == TWS_ROLE_MASTER) {
                rcsp_bt_ble_adv_enable(1);
            } else {
                rcsp_bt_ble_adv_enable(0);
            }
        }
    }

#endif
}

void send_version_to_sibling(void)
{
    u8 data[3] = {0};
    u16 ver = 0;
    ver =  get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
    log_info("%s---verson:%x\n", __func__, ver);
    data[0] = TWS_VERSON_INFO;
    data[1] = ver;
    data[2] = ver >> 8;
    tws_api_send_data_to_sibling(data, sizeof(data), TWS_FUNC_ID_SEQ_RAND_SYNC);
}

u16 rebuild_adv_rcsp_info(u8 *adv_rsp_buf, u16 buf_size, u8 type, u8 *addr)
{
    struct excfg_rsp_payload rsp_payload;
    u16 item_len = 0;
    u8 *item_data = NULL;
    u8 i, rsp_adv_len = 0;

    switch (type) {
    case CFG_ITEM_ADV_IND:
        log_info("[make new adv data]\n");
        rsp_payload.vid = 0x05D6;
        memcpy(rsp_payload.logo, "JLOTA", sizeof("JLOTA"));
        for (i = 0; i < sizeof(rsp_payload.logo) / 2; i++) {
            rsp_payload.logo[i] ^= rsp_payload.logo[sizeof(rsp_payload.logo) - i - 1];
            rsp_payload.logo[sizeof(rsp_payload.logo) - i - 1] ^= rsp_payload.logo[i];
            rsp_payload.logo[i] ^= rsp_payload.logo[sizeof(rsp_payload.logo) - i - 1];
        }
        rsp_payload.version = 1;

        memcpy(rsp_payload.addr, addr, 6);

        // +2:(length+type) length; + excfg_rsp_payload length; +6:rcsp数据;
        if ((2 + sizeof(struct excfg_rsp_payload) + 6) > buf_size) {
            log_info("adv data overflow!!!\n");
        } else {
            *(adv_rsp_buf + rsp_adv_len) = 1 + sizeof(struct excfg_rsp_payload) + 6;        // rcsp's length
            *(adv_rsp_buf + rsp_adv_len + 1) = 0xff;                                    // HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA
            memcpy(adv_rsp_buf + rsp_adv_len + 2, &rsp_payload, sizeof(struct excfg_rsp_payload));
            rsp_adv_len += (2 + sizeof(struct excfg_rsp_payload));
            addr[0] += 1;                                                        //修改地址，让手机重新发现服务, 这里地址的修改规则可以用户自行设置
            // vid (2byte)
            u16 vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
            adv_rsp_buf[rsp_adv_len++] = vid & 0xFF;
            adv_rsp_buf[rsp_adv_len++] = vid >> 8;
            // pid (2byte)
            u16 pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
            adv_rsp_buf[rsp_adv_len++] = pid & 0xFF;
            adv_rsp_buf[rsp_adv_len++] = pid >> 8;
            // type + version (1byte)
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            adv_rsp_buf[rsp_adv_len++] = 0x00 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// 音箱类型
#elif (RCSP_MODE == RCSP_MODE_EARPHONE)
#if TCFG_USER_TWS_ENABLE
            adv_rsp_buf[rsp_adv_len++] = 0x02 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// tws类型
#else
            adv_rsp_buf[rsp_adv_len++] = 0x03 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// 耳机类型
#endif
#else
            adv_rsp_buf[rsp_adv_len++] = 0x50 | VER_FLAG_BLE_CTRL_BREDR | VER_FLAG_IOS_BLE_LINK_BREDR;	// 手表类型
#endif
            // vbat (1byte)
            adv_rsp_buf[rsp_adv_len++] = 0;

            log_info("new adv_data(%d) = %lu:\n", rsp_adv_len, (2 + sizeof(struct excfg_rsp_payload) + 6));
            log_info_hexdump(adv_rsp_buf, rsp_adv_len);
        }
        break;
    case CFG_ITEM_SCAN_RSP:
        log_info("[make new rsp data]\n");
        rsp_adv_len += make_eir_packet_val(&adv_rsp_buf[rsp_adv_len], rsp_adv_len, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
        const char *edr_name = bt_get_local_name();
        rsp_adv_len += make_eir_packet_data(&adv_rsp_buf[rsp_adv_len], rsp_adv_len, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name) > 26 ? 26 : strlen(edr_name));
        printf("%s, strlen(edr_name):%d", edr_name, (int)strlen(edr_name));
        log_info("new rsp_data(%d):\n", rsp_adv_len);
        log_info_hexdump(adv_rsp_buf, rsp_adv_len);
        break;
    }
    return rsp_adv_len;
}

#endif
