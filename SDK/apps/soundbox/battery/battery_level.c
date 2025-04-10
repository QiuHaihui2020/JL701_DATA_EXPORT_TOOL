#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".battery_level.data.bss")
#pragma data_seg(".battery_level.data")
#pragma const_seg(".battery_level.text.const")
#pragma code_seg(".battery_level.text")
#endif
#include "system/includes.h"
#include "battery_manager.h"
#include "app_power_manage.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "asm/charge.h"
#include "app_tone.h"
#include "gpadc.h"
#include "btstack/avctp_user.h"
#include "user_cfg.h"
#include "asm/charge.h"
#include "bt_tws.h"
#include "idle.h"

#if RCSP_ADV_EN
#include "ble_rcsp_adv.h"
#endif

#define LOG_TAG             "[BATTERY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

enum {
    VBAT_NORMAL = 0,
    VBAT_WARNING,
    VBAT_LOWPOWER,
} VBAT_STATUS;

#define VBAT_DETECT_CNT         6 //每次更新电池电量采集的次数
#define VBAT_DETECT_TIME        10L //每次采集的时间间隔
#define VBAT_UPDATE_SLOW_TIME   60000L //慢周期更新电池电量
#define VBAT_UPDATE_FAST_TIME   10000L //快周期更新电池电量

union battery_data {
    u32 raw_data;
    struct {
        u8 reserved;
        u16 voltage;
        u8 percent;
    } __attribute__((packed)) data;
};

static u16 vbat_slow_timer = 0;
static u16 vbat_fast_timer = 0;
static u16 lowpower_timer = 0;
static u8 old_battery_level = 9;
static u16 cur_battery_voltage = 0;
static u8 cur_battery_level = 0;
static u8 cur_battery_percent = 0;
static u8 tws_sibling_bat_level = 0xff;
static u8 tws_sibling_bat_percent_level = 0xff;
static u8 cur_bat_st = VBAT_NORMAL;
static u8 battery_curve_max;
static const struct battery_curve *battery_curve_p = NULL;

void vbat_check(void *priv);
void clr_wdt(void);

#if TCFG_USER_TWS_ENABLE
u8 get_tws_sibling_bat_level(void)
{
    return tws_sibling_bat_level & 0x7f;
}

u8 get_tws_sibling_bat_persent(void)
{
    return tws_sibling_bat_percent_level;
}

void app_power_set_tws_sibling_bat_level(u8 vbat, u8 percent)
{
    tws_sibling_bat_level = vbat;
    tws_sibling_bat_percent_level = percent;
    /*
     ** 发出电量同步事件进行进一步处理
     **/
    batmgr_send_msg(POWER_EVENT_SYNC_TWS_VBAT_LEVEL, 0);

    log_info("set_sibling_bat_level: %d, %d\n", vbat, percent);
}


static void set_tws_sibling_bat_level(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (rx) {
        app_power_set_tws_sibling_bat_level(data[0], data[1]);
    }
}

REGISTER_TWS_FUNC_STUB(vbat_sync_stub) = {
    .func_id = TWS_FUNC_ID_VBAT_SYNC,
    .func    = set_tws_sibling_bat_level,
};

void tws_sync_bat_level(void)
{
#if TCFG_BT_DISPLAY_BAT_ENABLE
    u8 battery_level = cur_battery_level;
#if CONFIG_DISPLAY_DETAIL_BAT
    u8 percent_level = get_vbat_percent();
#else
    u8 percent_level = get_self_battery_level() * 10 + 10;
#endif
    if (get_charge_online_flag()) {
        percent_level |= BIT(7);
    }

    u8 data[2];
    data[0] = battery_level;
    data[1] = percent_level;
    tws_api_send_data_to_sibling(data, 2, TWS_FUNC_ID_VBAT_SYNC);

    log_info("tws_sync_bat_level: %d,%d\n", battery_level, percent_level);
#endif
}
#endif


static void power_warning_timer(void *p)
{
    batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
}

static int app_power_event_handler(int *msg)
{
    int ret = false;

#if(TCFG_SYS_LVD_EN == 1)
    switch (msg[0]) {
    case POWER_EVENT_POWER_NORMAL:
        break;
    case POWER_EVENT_POWER_WARNING:
        play_tone_file(get_tone_files()->low_power);
        if (lowpower_timer == 0) {
            lowpower_timer = sys_timer_add(NULL, power_warning_timer, LOW_POWER_WARN_TIME);
        }
        break;
    case POWER_EVENT_POWER_LOW:
        r_printf(" POWER_EVENT_POWER_LOW");
        vbat_timer_delete();
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
#if TCFG_APP_BT_EN
#if RCSP_ADV_EN
        adv_tws_both_in_charge_box(1);
#endif
        if (!app_in_mode(APP_MODE_IDLE)) {
            sys_enter_soft_poweroff(POWEROFF_NORMAL);
        } else {
            power_set_soft_poweroff();
        }
#else
        app_goto_mode(APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
#endif
        break;
#if TCFG_APP_BT_EN
    case POWER_EVENT_SYNC_TWS_VBAT_LEVEL:
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
        }
        break;
    case POWER_EVENT_POWER_CHANGE:
        /* log_info("POWER_EVENT_POWER_CHANGE\n"); */

        if (!app_in_mode(APP_MODE_BT)) {
            break;
        }
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_tws_state()&TWS_STA_ESCO_OPEN) {
                break;
            }
            tws_sync_bat_level();
        }
#endif
        bt_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
#endif
        break;
    case POWER_EVENT_POWER_CHARGE:
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
        break;
#if TCFG_CHARGE_ENABLE
    case CHARGE_EVENT_LDO5V_OFF:
        //充电拔出时重新初始化检测定时器
        vbat_check_init();
        break;
#endif
    default:
        break;
    }
#endif

    return ret;
}
APP_MSG_HANDLER(bat_level_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = app_power_event_handler,
};

static u16 get_vbat_voltage(void)
{
    return (adc_get_voltage(AD_CH_PMU_VBAT) * 4);
}

static u16 battery_calc_percent(u16 bat_val)
{
    u8 i, tmp_percent;
    u16 max, min, div_percent;
    if (battery_curve_p == NULL) {
        log_error("battery_curve not init!!!\n");
        return 0;
    }

    for (i = 0; i < (battery_curve_max - 1); i++) {
        if (bat_val <= battery_curve_p[i].voltage) {
            return battery_curve_p[i].percent;
        }
        if (bat_val >= battery_curve_p[i + 1].voltage) {
            continue;
        }
        div_percent = battery_curve_p[i + 1].percent - battery_curve_p[i].percent;
        min = battery_curve_p[i].voltage;
        max = battery_curve_p[i + 1].voltage;
        tmp_percent = battery_curve_p[i].percent;
        tmp_percent += (bat_val - min) * div_percent / (max - min);
        return tmp_percent;
    }
    return battery_curve_p[battery_curve_max - 1].percent;
}

u16 get_vbat_value(void)
{
    return cur_battery_voltage;
}

u8 get_vbat_percent(void)
{
    return cur_battery_percent;
}

bool get_vbat_need_shutdown(void)
{
    if ((cur_battery_voltage <= app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
        return TRUE;
    }
    return FALSE;
}

//将当前电量转换为1~9级发送给手机同步电量
u8  battery_value_to_phone_level(void)
{
    u8  battery_level = 0;
    u8 vbat_percent = get_vbat_percent();
    if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
        return 0;
    }
    battery_level = (vbat_percent - 5) / 10;
    return battery_level;
}

//获取自身的电量
u8  get_self_battery_level(void)
{
    return cur_battery_level;
}

#if TCFG_USER_TWS_ENABLE
u8 get_cur_battery_level(void)
{
    u8 bat_lev = tws_sibling_bat_level & (~BIT(7));
    if (bat_lev == 0x7f) {
        return cur_battery_level;
    }

#if (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LOWER)
    return cur_battery_level < bat_lev ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_HIGHER)
    return cur_battery_level < bat_lev ? bat_lev : cur_battery_level;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_LEFT)
    return tws_api_get_local_channel() == 'L' ? cur_battery_level : bat_lev;
#elif (CONFIG_DISPLAY_TWS_BAT_TYPE == CONFIG_DISPLAY_TWS_BAT_RIGHT)
    return tws_api_get_local_channel() == 'R' ? cur_battery_level : bat_lev;
#else
    return cur_battery_level;
#endif
}

#else

u8 get_cur_battery_level(void)
{
    return cur_battery_level;
}
#endif

void vbat_check_slow(void *priv)
{
    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_TIME, 1);
    }
    if (get_charge_online_flag()) {
        sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_SLOW_TIME);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
    }
}

void vbat_curve_init(const struct battery_curve *curve_table, int table_size)
{
    battery_curve_p = curve_table;
    battery_curve_max = table_size;

    //初始化相关变量
    cur_battery_voltage = get_vbat_voltage();
    cur_battery_percent = battery_calc_percent(cur_battery_voltage);
    cur_battery_level = battery_value_to_phone_level();
}

void vbat_check_init(void)
{
#if TCFG_BATTERY_CURVE_ENABLE == 0
    u8 tmp[128] = {0};
    int i;
    u16 battery_0, battery_100;
    union battery_data battery_data_t;

    //初始化电池曲线
    if (battery_curve_p == NULL) {
        memset(tmp, 0x00, sizeof(tmp));
        int ret = syscfg_read(CFG_BATTERY_CURVE_ID, tmp, sizeof(tmp));
        if (ret > 0) {
            battery_curve_max = ret / sizeof(battery_data_t);
        } else {
            battery_curve_max = 2;
        }
        battery_curve_p = malloc(battery_curve_max * sizeof(struct battery_curve));
        ASSERT(battery_curve_p, "malloc battery_curve err!");
        if (ret < 0) {
            log_error("battery curve id, ret: %d\n", ret);
            battery_0 = app_var.poweroff_tone_v;
#if TCFG_CHARGE_ENABLE
            //防止部分电池充不了这么高电量，充满显示未满的情况
            battery_100 = (get_charge_full_value() - 100);
#else
            battery_100 = 4100;
#endif
            battery_curve_p[0].percent = 0;
            battery_curve_p[0].voltage = battery_0;
            battery_curve_p[1].percent = 100;
            battery_curve_p[1].voltage = battery_100;
            log_info("percent: %d, voltage: %d mV", 0, battery_curve_p[0].voltage);
            log_info("percent: %d, voltage: %d mV", 100, battery_curve_p[1].voltage);
        } else {
            for (i = 0; i < battery_curve_max; i++) {
                memcpy(&battery_data_t.raw_data,
                       &tmp[i * sizeof(battery_data_t)], sizeof(battery_data_t));
                battery_curve_p[i].percent = battery_data_t.data.percent;
                battery_curve_p[i].voltage = battery_data_t.data.voltage;
                log_info("percent: %d, voltage: %d mV\n",
                         battery_curve_p[i].percent, battery_curve_p[i].voltage);
            }
        }
        //初始化相关变量
        cur_battery_voltage = get_vbat_voltage();
        cur_battery_percent = battery_calc_percent(cur_battery_voltage);
        cur_battery_level = battery_value_to_phone_level();
    }
#endif

    if (vbat_slow_timer == 0) {
        vbat_slow_timer = sys_timer_add(NULL, vbat_check_slow, VBAT_UPDATE_FAST_TIME);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_UPDATE_FAST_TIME);
    }

    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_TIME, 1);
    }
}

void vbat_timer_delete(void)
{
    if (vbat_slow_timer) {
        sys_timer_del(vbat_slow_timer);
        vbat_slow_timer = 0;
    }
    if (vbat_fast_timer) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }
}

void vbat_check(void *priv)
{
    static u8 unit_cnt = 0;
    static u8 low_voice_cnt = 0;
    static u8 low_power_cnt = 0;
    static u8 power_normal_cnt = 0;
    static u8 charge_online_flag = 0;
    static u8 low_voice_first_flag = 1;//进入低电后先提醒一次
    static u32 bat_voltage = 0;
    u16 tmp_percent;

    bat_voltage += get_vbat_voltage();
    unit_cnt++;
    if (unit_cnt < VBAT_DETECT_CNT) {
        return;
    }
    unit_cnt = 0;

    //更新电池电压,以及电池百分比,还有电池等级
    cur_battery_voltage = bat_voltage / VBAT_DETECT_CNT;
    bat_voltage = 0;
    tmp_percent = battery_calc_percent(cur_battery_voltage);
    if (get_charge_online_flag()) {
        if (tmp_percent > cur_battery_percent) {
            cur_battery_percent++;
        }
    } else {
        if (tmp_percent < cur_battery_percent) {
            cur_battery_percent--;
        }
    }
    cur_battery_level = battery_value_to_phone_level();

    g_printf("cur_voltage: %d mV, tmp_percent: %d, cur_percent: %d, cur_level: %d\n",
             cur_battery_voltage, tmp_percent, cur_battery_percent, cur_battery_level);

    if (get_charge_online_flag() == 0) {
        if (adc_check_vbat_lowpower() ||
            (cur_battery_voltage <= app_var.poweroff_tone_v)) { //低电关机
            low_power_cnt++;
            low_voice_cnt = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_LOWPOWER;
            if (low_power_cnt > 6) {
                log_info("\n*******Low Power,enter softpoweroff********\n");
                low_power_cnt = 0;
                batmgr_send_msg(POWER_EVENT_POWER_LOW, 0);
                usr_timer_del(vbat_fast_timer);
                vbat_fast_timer = 0;
            }
        } else if (cur_battery_voltage <= app_var.warning_tone_v) { //低电提醒
            low_voice_cnt ++;
            low_power_cnt = 0;
            power_normal_cnt = 0;
            cur_bat_st = VBAT_WARNING;
            if ((low_voice_first_flag && low_voice_cnt > 1) || //第一次进低电10s后报一次
                (!low_voice_first_flag && low_voice_cnt >= 5)) {
                low_voice_first_flag = 0;
                low_voice_cnt = 0;
#if(TCFG_SYS_LVD_EN == 1)
                if (!lowpower_timer) {
                    log_info("\n**Low Power,Please Charge Soon!!!**\n");
                    batmgr_send_msg(POWER_EVENT_POWER_WARNING, 0);
                }
#endif
            }
        } else {
            power_normal_cnt++;
            low_voice_cnt = 0;
            low_power_cnt = 0;
            if (power_normal_cnt > 2) {
                if (cur_bat_st != VBAT_NORMAL) {
                    log_info("[Noraml power]\n");
                    cur_bat_st = VBAT_NORMAL;
                    batmgr_send_msg(POWER_EVENT_POWER_NORMAL, 0);
                }
            }
        }
    } else {
        batmgr_send_msg(POWER_EVENT_POWER_CHARGE, 0);
    }

    if (cur_bat_st != VBAT_LOWPOWER) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
        //电量等级变化,或者在仓状态变化,交换电量
        if ((cur_battery_level != old_battery_level) ||
            (charge_online_flag != get_charge_online_flag())) {
            batmgr_send_msg(POWER_EVENT_POWER_CHANGE, 0);
        }
        charge_online_flag =  get_charge_online_flag();
        old_battery_level = cur_battery_level;
    }
}

bool vbat_is_low_power(void)
{
    return (cur_bat_st != VBAT_NORMAL);
}

void check_power_on_voltage(void)
{
#if(TCFG_SYS_LVD_EN == 1)

    u16 val = 0;
    u8 normal_power_cnt = 0;
    u8 low_power_cnt = 0;

    while (1) {
        clr_wdt();
        val = get_vbat_voltage();
        printf("vbat: %d\n", val);
        if ((val < app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
            low_power_cnt++;
            normal_power_cnt = 0;
            if (low_power_cnt > 10) {
                /* ui_update_status(STATUS_POWERON_LOWPOWER); */
                os_time_dly(100);
                log_info("power on low power , enter softpoweroff!\n");
                power_set_soft_poweroff();
            }
        } else {
            normal_power_cnt++;
            low_power_cnt = 0;
            if (normal_power_cnt > 10) {
                vbat_check_init();
                return;
            }
        }
    }
#endif
}
