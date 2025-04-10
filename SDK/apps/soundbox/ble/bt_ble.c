#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_ble.data.bss")
#pragma data_seg(".bt_ble.data")
#pragma const_seg(".bt_ble.text.const")
#pragma code_seg(".bt_ble.text")
#endif
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_power_manage.h"
#include "btcontroller_modules.h"
#include "app_chargestore.h"
#include "btstack/avctp_user.h"
#include "bt_common.h"
#include "le_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "bt_tws.h"
#include "btstack/le/le_common_define.h"
#include "asm/charge.h"
#include "sdk_config.h"

#if TCFG_BT_BLE_ADV_ENABLE

#define LOG_TAG_CONST       BT_BLE
#define LOG_TAG             "[BT_BLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if 1
#undef log_info
#undef log_debug_hexdump

#define log_info(x, ...)           y_printf("[BT-BLE]" x " ", ## __VA_ARGS__)
#define log_debug_hexdump          put_buf
#endif


//unit: (1ms)
#define  BLE_TIMER_SET                   (150)   /*未连接 或者 连接后未入sniff的定时器检测时间*/
#define  BLE_SNIFF_TIMER_SET             (500*2) /*连接后入sniff的定时器检测时间*/

//unit: (0.625us)
#define  ADV_FAST_INTERVAL               (48)     /*未连接广播周期*/
#define  ADV_CONN_FAST_INTERVAL          (64)     /*连接后快速广播周期*/
#define  ADV_CONN_NORMAL_INTERVAL        (64 * 2)   /*未连接广播周期*/
#define  ADV_SNIFF_INTERVAL              (800*1)  /*edr连接进入sniff后的广播周期*/
#define  ADV_SLOW_INTERVAL               (800*1)  /*连接后慢速广播周期*/

//连上后广播持续密集的时间,默认15秒: ADV_CONN_NORMAL_INTERVAL-->ADV_SLOW_INTERVAL
#define ADV_TIME_TO_SLOW_MS              (15000/BLE_TIMER_SET + 1)

//停留时间 50秒,可以自定义修改
#define ADV_ST_CONN_HOLD_TIME            (50000/BLE_TIMER_SET)//phone connect device,配对连上停留时间
#define ADV_ST_RECONN_HOLD_TIME          (50000/BLE_TIMER_SET)//device connect phone,回连连上停留时间

//以下时间不建议修改
#define ADV_ST_DISMISS_HOLD_TIME         (2000/BLE_TIMER_SET +1)
#define ADV_ST_PRE1_HOLD_TIME            (200/BLE_TIMER_SET  +1)
#define ADV_ST_FAST_DISMISS_HOLD_TIME    (1500/BLE_TIMER_SET +1)//to fast display reconnect

//0---tws应用，2设备配对后才开广播
//1---单设备应用，开广播
#define PHONE_ICON_ONE_BETA             0 //beta case,one ear case

//0---default,inquiry,connect,reconnect,支持搜索、配对连接、回连的显示
//1---only inquiry and first connect   ,只支持搜索、配对连接的显示
#define PHONE_ICON_CTRL_MODE            0 //

//1--连上后,广播一段时间,超时关闭广播
//0--连上后,一直持续广播,显示电量
#define PHONE_ICON_AUTO_DISMISS         1 //auto dismiss icon

#if TCFG_CHARGESTORE_ENABLE
#define REFLASH_ICON_STATE              1 //auto reflash,定时自动更新电量和耳机状态
#else
#define REFLASH_ICON_STATE              0 //not auto reflash,fixed,固定电量,不需要定时更新耳机状态
#endif

static u32 ble_timer_handle;//定时器id
static u16 adv_st_count;//定时器的计数器
static u8  dev_battery_level[3];//master,slave,box,存储电量
static volatile u8 adv_control_enable;//广播控制，默认只有matster开广播
static volatile u8 adv_st_mode;//当前广播模式
static volatile u8 adv_st_next_mode;//准备切换的广播模式
static volatile u8 adv_enable;//广播开关
static u8 adv_data[ADV_RSP_PACKET_MAX];
static u8 adv_data_len;
static u8 rsp_data[ADV_RSP_PACKET_MAX];
static u8 rsp_data_len;
static u8 adv_rand;//广播随机数
static u8 adv_open_lock;//lock open 标记
static volatile u8 send_sync_pos_flag;/*耳机状态有变化，就需要发送形象同步*/

//-------------------------------------------------------------------
//持续广播控制操作：开机快广播一段时间，后面根据状态调整广播

enum { //广播步进状态
    ADV_STEP_FAST = 0, //未连接，快速广播
    ADV_STEP_NORMAL,   //连上正常广播
    ADV_STEP_SNIFF,    //EDR进入sleep后，广播
    ADV_STEP_SLOW,     //慢广播状态
};

//0-normal,1-timeout,2-sniff,3-slow adv
static u8 adv_state_step = ADV_STEP_FAST; /*广播的频率状态*/
static u32 adv_check_timer_ms = BLE_TIMER_SET;/*定时器检测的频率*/
static u32 adv_high_timer_count;//广播快变慢计数器

//-------------------user_tag data start----------------------------------------------------------
static u8 user_tag_data[19] = {
    0xd6, 0x05,
    0x01, 0x00,
    0x01, 0x00,
    0x00, //6
};

#define ADV_LONG_PACKET_SIZE   23//

static const u8 user_pre1_tag_data[ADV_LONG_PACKET_SIZE] = {
    0xd6, 0x05,
    0x02, 0x00,
    0x02, 0x00,
    0x00, //6
};

static const u8 user_pre2_tag_data[ADV_LONG_PACKET_SIZE] = {
    0xd6, 0x05,
    0x02, 0x00,
    0x02, 0x00,
    0x00, //6
};

static const u8 user_conn_tag_data[ADV_LONG_PACKET_SIZE] = {
    0xd6, 0x05,
    0x02, 0x00,
    0x02, 0x00,
    0x00, //6
};

static const u8 user_reconn_tag_data[ADV_LONG_PACKET_SIZE] = {
    0xd6, 0x05,
    0x02, 0x00,
    0x02, 0x00,
    0x00, //6
};

//-----------------------data end------------------------------------------------------

static void adv_rand_init(void);
static u8 update_dev_battery_level(void);
static void bt_ble_timer_init(void);
static void bt_ble_timer_exit(void);

//-------------------------------------------------------
/*填入adv包数据*/
const char ble_tag_string[] = "<ble>";
static int make_set_adv_data(u8 *info, u8 len)
{
    u8 offset = 0;
    u8 *buf = (void *)adv_data;
    u8 *tag_buf = (void *)info;
    u8 tag_len = len;

#if 0
    //BLE广播使用EDR的蓝牙名字，防止连接时蓝牙名字变成693_ble蓝牙名字最多只能31-21=10字节。
    const char *edr_name = bt_get_local_name();
    u8 ext_name_len = strlen(ble_tag_string);
    if ((strlen(edr_name) + ext_name_len) > BT_NAME_LEN_MAX) {
        ext_name_len = 0;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)edr_name, strlen(edr_name) + ext_name_len);
    if (ext_name_len) {
        memcpy(&buf[offset - ext_name_len], ble_tag_string, ext_name_len);
    }
    /* offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)ble_name_string, strlen(ble_name_string)); */
#else
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)tag_buf, tag_len);
#endif

    if (offset > ADV_RSP_PACKET_MAX) {
        log_info("adv_data overflow!!!\n");
        return -1;
    }
    log_info("ADV data(%d):", offset);
    log_debug_hexdump(buf, offset);
    adv_data_len = offset;
    return 0;
}

/*填入rsp包数据,只广播默认不需要*/
static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = (void *)rsp_data;

    if (offset > ADV_RSP_PACKET_MAX) {
        log_info("rsp_data overflow!!!\n");
        return -1;
    }

    log_info("RSP data(%d):,offset");
    log_debug_hexdump(buf, offset);
    rsp_data_len = offset;
    return 0;
}

#define TCFG_BT_BLE_ADV_TYPE         3
#define TCFG_BT_BLE_ADV_CHANNEL     0x07
static void adv_setup_init(void)
{
    u8 adv_type = TCFG_BT_BLE_ADV_TYPE;
    u8 adv_channel  = TCFG_BT_BLE_ADV_CHANNEL;
    u16 adv_interval;

    switch (adv_st_mode) {
    case ADV_ST_INQUIRY:
    case ADV_ST_DISMISS:
    case ADV_ST_FAST_DISMISS:
        //特殊状态,广播周期快
        adv_interval = ADV_FAST_INTERVAL;
        break;

    //已连接及其他状态
    case ADV_ST_CONN:
    case ADV_ST_RECONN:
    default:
        if (adv_state_step == ADV_STEP_NORMAL) {
            adv_interval = ADV_CONN_NORMAL_INTERVAL;
            adv_high_timer_count = ADV_TIME_TO_SLOW_MS;
        } else if (adv_state_step == ADV_STEP_SNIFF) {
            adv_interval = ADV_SNIFF_INTERVAL;
        } else if (adv_state_step == ADV_STEP_SLOW) {
            adv_interval = ADV_SLOW_INTERVAL;
        } else {
            adv_interval = ADV_CONN_FAST_INTERVAL;
        }
        break;
    }

    log_info("adv_interval= %d\n", adv_interval);

    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, adv_interval, adv_type, adv_channel);
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, adv_data_len, adv_data);
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, rsp_data_len, rsp_data);
}

u8 bt_ble_get_adv_enable(void)
{
    return adv_enable;
}

__attribute__((weak))
void bt_ble_adv_enable(u8 enable)
{
    if (adv_enable == enable) {
        return;
    }
    adv_enable = enable;

    log_info("***ble_adv_en: %d\n", enable);
    if (enable) {
        bt_ble_timer_init();
        adv_setup_init();
    } else {
        bt_ble_timer_exit();
        adv_high_timer_count = 0;
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, enable);
}

//-----------------------
static u8 get_device_bat_percent(void)
{
#if CONFIG_DISPLAY_DETAIL_BAT
    u8 val = get_vbat_percent();
#else
    u8 val = get_self_battery_level() * 10 + 10;
#endif

    if (val > 100) {
        val = 100;
    }
    val = (val | (get_charge_online_flag() << 7));
    /* printf("[%d]",val); */
    return val;
}

static void reflash_adv_tag_data(const u8 *tag_data, u8 len, u8 spec_val)
{
    u8 tmp_tag_data[32];
    u8 tmp8;

    memcpy(tmp_tag_data, tag_data, len);
    memcpy(&tmp_tag_data[14], dev_battery_level, 3);

    u8 cur_ear_pos = 0;
#if TCFG_CHARGESTORE_ENABLE
    log_info("get pos form chargesstore\n");
    cur_ear_pos = chargestore_get_earphone_pos();
#elif TCFG_USER_TWS_ENABLE
    cur_ear_pos = bt_tws_get_local_channel();
#endif

    //位置14,15,16 对应 L,R,BOX
    if (cur_ear_pos != 'R') {
        //switch position
        log_info("is R pos\n");
        tmp8 = tmp_tag_data[14];
        tmp_tag_data[14] = tmp_tag_data[15];
        tmp_tag_data[15] = tmp8;
    } else {
        log_info("is L pos\n");
    }

    if (spec_val != 0xff) {
        tmp_tag_data[17] = spec_val;
        if (spec_val != 9) {
        } else {
            memset(&tmp_tag_data[14], 0xFF, 3);//关闭广播的时候不显示电量
        }
    }

    //---
#if (!REFLASH_ICON_STATE) //
    if (len == ADV_LONG_PACKET_SIZE) {
        //fixed do,start,固定分开显示电量,如不分开，可以屏蔽这段代码
        log_info("fixed two do\n");
        /* log_debug_hexdump(&tmp_tag_data[14],3);  */
        if ((tmp_tag_data[14] != 0xff)
            && (tmp_tag_data[15] != 0xff)) {
            if ((tmp_tag_data[14]&BIT(7))
                && (tmp_tag_data[15]&BIT(7))) {
                tmp_tag_data[15] &= 0x7f;
                log_info("fixed c1\n");
            } else {
                if (((tmp_tag_data[14]&BIT(7)) == 0)
                    && ((tmp_tag_data[15]&BIT(7)) == 0)) {
                    tmp_tag_data[14] |= BIT(7);
                    log_info("fixed c2\n");
                }

            }

        }
        //fixed do,end
    }
#endif
    //---

    bt_ble_adv_enable(0);

    if (send_sync_pos_flag) {
        u8 pos_state[2];
        //L---offset 14
        if (tmp_tag_data[14] == 0xff) {
            pos_state[0] = PBG_POS_NOT_EXIST;
        } else if (tmp_tag_data[14] & BIT(7)) {
            pos_state[0] = PBG_POS_IN_BOX;
        } else {
            pos_state[0] = PBG_POS_OUT_BOX;
        }

        //R---offset 15
        if (tmp_tag_data[15] == 0xff) {
            pos_state[1] = PBG_POS_NOT_EXIST;
        } else if (tmp_tag_data[15] & BIT(7)) {
            pos_state[1] = PBG_POS_IN_BOX;
        } else {
            pos_state[1] = PBG_POS_OUT_BOX;
        }
        send_sync_pos_flag  = 0;
        pbg_user_battery_level_sync(&tmp_tag_data[14]);
        pbg_user_ear_pos_sync(pos_state[0], pos_state[1]);
    }

    //fix adv_data state
#if 0//(CONFIG_NO_DISPLAY_BUTTON_ICON == 0)
    //if(ADV_ST_CONN == adv_st_mode) //
    if (0) {
        ;
    } else {
        //pbg_adv_data_change_parse(tmp_tag_data, len, adv_rand);
    }
#endif
    //end

    make_set_adv_data(tmp_tag_data, len);
    bt_ble_adv_enable(1);
}

static bool check_adv_open_lock(void)
{
    if (bt_get_total_connect_dev()) {
        //已连上手机
        adv_open_lock = 1;
    }
    return adv_open_lock;
}

static void change_adv_st_mode(u8 mode)
{
    if (mode == adv_st_mode) {
        return;
    }

    log_info("ble_adv_st:old= %d,new= %d \n", adv_st_mode, mode);

    adv_st_mode = mode;
    adv_st_count = 0;

    switch (mode) {
    case ADV_ST_CONN:
        check_adv_open_lock();
        reflash_adv_tag_data(user_conn_tag_data, sizeof(user_conn_tag_data), 0xff);
        break;

    case ADV_ST_RECONN:
        check_adv_open_lock();
        reflash_adv_tag_data(user_reconn_tag_data, sizeof(user_reconn_tag_data), 0xff);
        break;

    case ADV_ST_DISMISS:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), 0x9);
        break;

    case ADV_ST_INQUIRY:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), adv_rand);
        break;

    /* case ADV_ST_PRE1: */
    /* reflash_adv_tag_data(user_pre1_tag_data, sizeof(user_pre1_tag_data), 0xff); */
    /* break; */

    /* case ADV_ST_PRE2: */
    /* reflash_adv_tag_data(user_pre2_tag_data, sizeof(user_pre2_tag_data), 0xff); */
    /* break; */

    case ADV_ST_FAST_DISMISS:
        reflash_adv_tag_data(user_tag_data, sizeof(user_tag_data), 0x9);
        break;

    case ADV_ST_END:
        bt_ble_adv_enable(0);
        break;

    default:
        break;
    }

}

static void check_dev_pos_changed(u8 cur_adv_mode)
{
#if REFLASH_ICON_STATE
    if (update_dev_battery_level()) {
        log_info("---reflash_dev_state\n");
        /* adv_st_next_mode = cur_adv_mode; */
        /* change_adv_st_mode(ADV_ST_FAST_DISMISS); */
        if (adv_state_step == ADV_STEP_SLOW) {
            adv_state_step = ADV_STEP_NORMAL;//slow to high
        }
        send_sync_pos_flag = 1;
        change_adv_st_mode(ADV_ST_END);
        change_adv_st_mode(cur_adv_mode);
    }
#endif
}

static void dev_adv_reset_do(u8 cur_adv_mode)
{
    if (bt_ble_get_adv_enable()) {
        log_info("dev_adv_reset_do:%d\n", cur_adv_mode);
        change_adv_st_mode(ADV_ST_END);
        change_adv_st_mode(cur_adv_mode);
    }
}


static void bt_ble_timer_handler(void *priv)
{
    putchar('~');

    if (!adv_enable) {
        return;
    }

    adv_st_count++;
    switch (adv_st_mode) {
    /* case ADV_ST_PRE1: */
    /* if (adv_st_count > ADV_ST_PRE1_HOLD_TIME) { */
    /* change_adv_st_mode(ADV_ST_PRE2); */
    /* } */
    /* break; */

    case ADV_ST_CONN:
        if (adv_st_count > ADV_ST_CONN_HOLD_TIME) {
            if (!pbg_user_is_connected()) {
                change_adv_st_mode(ADV_ST_DISMISS);
                break;
            }
#if PHONE_ICON_AUTO_DISMISS
            change_adv_st_mode(ADV_ST_DISMISS);
#else
            if (adv_state_step == ADV_STEP_FAST) {
                adv_state_step = ADV_STEP_NORMAL;
                dev_adv_reset_do(adv_st_mode);
            } else {
                //keep adv
            }
            //change_adv_st_mode(ADV_ST_END);
#endif
        } else {
            check_dev_pos_changed(adv_st_mode);
        }
        break;

    case ADV_ST_RECONN:
        if (adv_st_count > ADV_ST_RECONN_HOLD_TIME) {
            if (!pbg_user_is_connected()) {
                change_adv_st_mode(ADV_ST_DISMISS);
                break;
            }
#if PHONE_ICON_AUTO_DISMISS
            change_adv_st_mode(ADV_ST_DISMISS);
#else
            if (adv_state_step == ADV_STEP_FAST) {
                adv_state_step = ADV_STEP_NORMAL;
                dev_adv_reset_do(adv_st_mode);
            } else {
                //keep adv
            }
            //change_adv_st_mode(ADV_ST_END);
#endif
        } else {
            check_dev_pos_changed(adv_st_mode);
        }
        break;

    case ADV_ST_DISMISS:
        if (adv_st_count > ADV_ST_DISMISS_HOLD_TIME) {
            change_adv_st_mode(ADV_ST_END);
        }
        break;

    case ADV_ST_FAST_DISMISS:
        if (adv_st_count > ADV_ST_FAST_DISMISS_HOLD_TIME) {
            change_adv_st_mode(adv_st_next_mode);
        }
        break;

    case ADV_ST_INQUIRY:
        check_dev_pos_changed(adv_st_mode);
        break;

    case ADV_ST_NULL:
        /* case ADV_ST_PRE2: */
        break;

    default:
        break;
    }

#if(!PHONE_ICON_AUTO_DISMISS)
    if (adv_high_timer_count && (adv_state_step != ADV_STEP_FAST)) {
        if (adv_high_timer_count == 1) {
            log_info("adv high to slow\n");
            adv_state_step = ADV_STEP_SLOW;
            dev_adv_reset_do(adv_st_mode);
        }
        adv_high_timer_count--;
    }
#endif
}

/*创建定时器*/
static void bt_ble_timer_init(void)
{
    if (!ble_timer_handle) {
        ble_timer_handle = sys_timer_add(NULL, bt_ble_timer_handler, adv_check_timer_ms);
    }
}

/*删除定时器*/
static void bt_ble_timer_exit(void)
{
    if (ble_timer_handle) {
        sys_timer_del(ble_timer_handle);
        ble_timer_handle = 0;
    }
}

static void bt_ble_init_tag_do(void)
{
    log_info("bt_ble_init_tag_do\n");
    //初始化三个电量值
    memset(dev_battery_level, 0xff, sizeof(dev_battery_level));
    dev_battery_level[0] = get_device_bat_percent();
    swapX(bt_get_mac_addr(), &user_tag_data[7], 6);
    log_info("---master address:\n");
    log_debug_hexdump(&user_tag_data[7], 6);
}

/*重设ble的mac地址*/
void bt_ble_icon_set_comm_address(u8 *addr)
{
    u8 tmp_addr[6];

    if (TCFG_BT_BLE_BREDR_SAME_ADDR) {
        memcpy(tmp_addr, addr, 6);
    } else {
        bt_make_ble_address(tmp_addr, addr);
    }

    swapX(tmp_addr, &user_tag_data[7], 6);
    le_controller_set_mac(tmp_addr);//将ble广播地址改成公共地址
    log_info("---reset master address:\n");
    log_debug_hexdump(&user_tag_data[7], 6);
}

/*重设icon*/
void bt_ble_icon_reset(void)
{
    log_info("bt_ble_icon_reset\n");
    if (!adv_control_enable) {
        return;
    }
    //重置状态，允许再广播
    log_info("reset_do\n");
    /* adv_rand_init(); */
    adv_st_mode = ADV_ST_NULL;
    adv_open_lock = 0;
    bt_ble_adv_enable(0);
}

u8 bt_ble_icon_get_adv_state(void)
{
    return adv_st_mode;
}

//return change_flag
static u8 update_dev_battery_level(void)
{

    u8 old_dev_battery[3];

    memcpy(old_dev_battery, dev_battery_level, 3);

    //read bat_level
    dev_battery_level[0] = get_device_bat_percent();

    if (!get_tws_sibling_connect_state()) {
        dev_battery_level[1] = 0xff;
    }

    u8 sibling_bat_level = dev_battery_level[1];
    if (sibling_bat_level == 0xff) {
        /* log_info("--update_bat_00\n"); */
        goto update_dev2;
    }


    sibling_bat_level = get_tws_sibling_bat_persent();

#if TCFG_CHARGESTORE_ENABLE
    if (sibling_bat_level == 0xff) {
        /* log_info("--update_bat_01\n"); */
        sibling_bat_level  = chargestore_get_sibling_power_level();
    }
#endif

    if (sibling_bat_level == 0xff) {
        /* log_info("--update_bat_02\n"); */
        dev_battery_level[1] = dev_battery_level[0];
    } else {
        dev_battery_level[1] = sibling_bat_level;
    }

update_dev2:
    if (((dev_battery_level[0] & BIT(7)) && (dev_battery_level[0] != 0xff))
        || ((dev_battery_level[1] & BIT(7)) && (dev_battery_level[1] != 0xff))) {
        //earphone in charge
#if TCFG_CHARGESTORE_ENABLE
        dev_battery_level[2] = chargestore_get_power_level();
#else
        dev_battery_level[2] = 0xff;
#endif
    } else {
        //all not in charge
        dev_battery_level[2] = 0xff;
    }

    u8 i;
    u8 change_flag = 0;

    for (i = 0; i < 3; i++) {
        if ((dev_battery_level[i]&BIT(7)) != (old_dev_battery[i]&BIT(7))) {
            break;
        }

        if ((dev_battery_level[i] == 0xff) && (old_dev_battery[i] != 0xff)) {
            break;
        }

        if ((dev_battery_level[i] != 0xff) && (old_dev_battery[i] == 0xff)) {
            break;
        }

    }

    if (i < 3) {
        log_info("dev_pos_state have changed:");
        log_debug_hexdump(old_dev_battery, 3);
        log_debug_hexdump(dev_battery_level, 3);
        change_flag = 1;
    }
    return change_flag;
}



//open 不同类型广播
void bt_ble_icon_open(u8 type)
{
    log_info("bt_ble_icon_open: %d\n", type);

    if (!adv_control_enable) {
        return;
    }

    if (adv_open_lock) {
        log_info("open_lock\n");
        return;
    }

    log_info("open_do\n");

    adv_state_step = ADV_STEP_FAST;
    send_sync_pos_flag = 1;
    update_dev_battery_level();

    switch (adv_st_mode) {
    case ADV_ST_NULL:

#if PHONE_ICON_ONE_BETA
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_INQUIRY);
        } else if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            //record,and not display
            change_adv_st_mode(ADV_ST_END);
        }
        break;
#endif

#if (PHONE_ICON_CTRL_MODE == 1)
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_INQUIRY);
        } else if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            //record,and not display
            change_adv_st_mode(ADV_ST_END);
        }
        break;
#endif


        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_RECONN);
        } else if (type == ICON_TYPE_PRE_DEV) {
            /* change_adv_st_mode(ADV_ST_PRE1); */
        } else {
            change_adv_st_mode(ADV_ST_INQUIRY);
        }
        break;

    case ADV_ST_INQUIRY:

#if PHONE_ICON_ONE_BETA
        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_DISMISS);
        }
        break;
#endif

#if (PHONE_ICON_CTRL_MODE == 1)
        if (type == ICON_TYPE_CONNECTED || type == ICON_TYPE_RECONNECT) {
            change_adv_st_mode(ADV_ST_DISMISS);
        }
        break;
#endif


        switch (type) {
        case ICON_TYPE_CONNECTED:
            change_adv_st_mode(ADV_ST_CONN);
            break;

        case ICON_TYPE_RECONNECT:
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_RECONN;
            break;

        default:
            break;
        }
        break;

    /* case ADV_ST_PRE1: */
    /* case ADV_ST_PRE2: */
    /* if (type == ICON_TYPE_INQUIRY) { */
    /* change_adv_st_mode(ADV_ST_INQUIRY); */
    /* } else if (type == ICON_TYPE_CONNECTED) { */
    /* change_adv_st_mode(ADV_ST_CONN); */
    /* } else { */
    /* ; */
    /* } */
    /* break; */


    case ADV_ST_RECONN:
        if (type == ICON_TYPE_INQUIRY) {
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_INQUIRY;
        } else if (type == ICON_TYPE_CONNECTED) {
            change_adv_st_mode(ADV_ST_FAST_DISMISS);
            adv_st_next_mode = ADV_ST_CONN;
        } else {
            ;
        }
        break;

    default:
        break;
    }
}

//dismiss_flag
void bt_ble_icon_close(u8 dismiss_flag)
{
    log_info("bt_ble_close: %d\n", dismiss_flag);

    if ((!dismiss_flag) || (!adv_control_enable)) {
        log_info("close_adv\n");
        change_adv_st_mode(ADV_ST_END);
        return;
    }

#if TCFG_CHARGESTORE_ENABLE
    log_info("###ear_online:%x, conn_state:%x\n", chargestore_get_earphone_online(), get_tws_sibling_connect_state());
    if (chargestore_get_earphone_online() != 2 && get_tws_sibling_connect_state()) {
        log_info("sibling is running\n");
        //另外一个耳机没有关机，不需要去icon
        change_adv_st_mode(ADV_ST_END);
        return;
    }
#endif

    switch (adv_st_mode) {
    case ADV_ST_CONN:
    case ADV_ST_RECONN:
    case ADV_ST_INQUIRY:
    /* case ADV_ST_PRE1: */
    /* case ADV_ST_PRE2: */
    case ADV_ST_FAST_DISMISS:
    case ADV_ST_DISMISS:
    case ADV_ST_END://也要做
        if (get_tws_sibling_connect_state() == FALSE) {
            adv_open_lock = 0;
            adv_state_step = ADV_STEP_NORMAL;
            bt_ble_set_control_en(1);
            bt_ble_icon_reset();
        }
        change_adv_st_mode(ADV_ST_DISMISS);
        break;

    default:
        break;
    }
}

//tws,need set en
void bt_ble_icon_slave_en(u8 en)
{
    log_info("---icon_slave_enable= %d\n", en);
    if (en) {
        dev_battery_level[1] = 0xfc;//set flag
    } else {
        dev_battery_level[1] = 0xff;
    }

    if (get_tws_sibling_connect_state()) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            //已配对连接过的从机，记录以后不允许再广播
            adv_open_lock = 1;//lock open 接口
            bt_ble_icon_close(0);
        }
    }
}


//tws,master need set en;使能ICON_OPEN 接口控制
void bt_ble_set_control_en(u8 en)
{
    log_info("---adv_control_enable= %d\n", en);
    adv_control_enable = en;//set master flag
}
//-----------------------
static void adv_rand_init(void)
{
    int ret = syscfg_read(CFG_BLE_MODE_INFO, (u8 *)&adv_rand, 1);
    if (!ret) {
        adv_rand = (rand32() % 7) + 1;
    } else {
        adv_rand++;
    }

    if (adv_rand > 7) {
        adv_rand = 1;
    }
    syscfg_write(CFG_BLE_MODE_INFO, (u8 *)&adv_rand, 1);
    log_info("------adv_rand = %d------\n", adv_rand);

}

//新增两个接口如下，用于获取和同步 adv_rand
void adv_rand_tws_sync(u8 sibling_rand)
{
    log_info("---sync_adv_rand = %d,%d---\n", adv_rand, sibling_rand);
    if (sibling_rand != adv_rand) {
        log_info("update sync\n");
        adv_rand = sibling_rand;
        syscfg_write(CFG_BLE_MODE_INFO, (u8 *)&adv_rand, 1);
    }
}

/*获取广播随机数*/
u8 adv_rand_tws_get(void)
{
    return adv_rand;
}

__BANK_INIT_ENTRY
__attribute__((weak))
void bt_ble_init(void)
{
    if (BT_MODE_IS(BT_NORMAL)) {
        log_info("***ble_init***\n");
        adv_rand_init();

        adv_control_enable = 0;//tws,default off
        adv_st_mode = ADV_ST_NULL;
        adv_open_lock = 0;
        bt_ble_init_tag_do();
    }
}

/*
void bt_ble_icon_set(u8 *info, u8 len)
{
    log_info("***** ble_icon_set******\n");

    bt_ble_adv_enable(0);
    update_user_tag_data(info, len);
    bt_ble_adv_enable(1);
}
*/

__attribute__((weak))
void bt_ble_exit(void)
{
    log_info("***ble_exit***\n");

    bt_ble_adv_enable(0);
    adv_st_mode = ADV_ST_NULL;
    adv_control_enable = 0;
    bt_ble_timer_exit();
}

/*模块开关使能*/
__attribute__((weak))
void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    bt_ble_adv_enable(en);
}

/*edr 是否进入sniff状态,state: 1--sniff,0-- no sniff*/
void bt_ble_icon_state_sniff(u8 state)
{
    log_info("icon_state_sniff= %d, adv_state_step= %d\n", state, adv_state_step);

#if (!PHONE_ICON_AUTO_DISMISS)

    if (state) {
        // enter sniff
        adv_state_step = ADV_STEP_SNIFF;
        adv_check_timer_ms = BLE_SNIFF_TIMER_SET;
    } else {
        // exit sniff
        if (adv_state_step != ADV_STEP_FAST) {
            adv_state_step = ADV_STEP_NORMAL;
            adv_check_timer_ms = BLE_TIMER_SET;
        }
    }

    if (ble_timer_handle) {
        sys_timer_modify(ble_timer_handle, adv_check_timer_ms);
    }
    dev_adv_reset_do(adv_st_mode);
    log_info("adv_state_step:%d ,mdy adv timer:%d ms\n", adv_state_step, adv_check_timer_ms);

#endif

}

void bt_ble_icon_role_switch(u8 role)
{
#if(!PHONE_ICON_AUTO_DISMISS)
    log_info("###bt_ble_icon_role_switch: %d\n", role);
    if (role == TWS_ROLE_MASTER  && bt_get_total_connect_dev() && pbg_user_is_connected()) {
        log_info("change to master\n");
        adv_open_lock = 0;
        bt_ble_set_control_en(1);
        adv_state_step = ADV_STEP_NORMAL;
        bt_ble_icon_reset();
        bt_ble_icon_open(ICON_TYPE_RECONNECT);
    } else {
        log_info("change to slave\n");
        change_adv_st_mode(ADV_ST_END);
        bt_ble_set_control_en(0);
    }
#endif

}


#endif



