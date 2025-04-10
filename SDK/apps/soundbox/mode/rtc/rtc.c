#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rtc.data.bss")
#pragma data_seg(".rtc.data")
#pragma const_seg(".rtc.text.const")
#pragma code_seg(".rtc.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "app_tone.h"
#include "rtc_ui.h"
#include "rtc.h"
#include "alarm.h"
#include "ui/ui_api.h"
#include "ui/ui_style.h"
#include "ui_manage.h"
#include "rcsp_rtc_func.h"
#include "local_tws.h"

#if TCFG_APP_RTC_EN

#define LOG_TAG             "[APP_RTC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define RTC_SET_MODE  0x55
#define ALM_SET_MODE  0xAA

#define RTC_POS_DEFAULT      RTC_POS_YEAR
#define RTC_ALM_POS_DEFAULT  ALM_POS_HOUR
#define RTC_MODE_DEFAULT     RTC_SET_MODE

#define MAX_YEAR          2099
#define MIN_YEAR          2000

enum {
    RTC_POS_NULL = 0,
    RTC_POS_YEAR,
    RTC_POS_MONTH,
    RTC_POS_DAY,
    RTC_POS_HOUR,
    RTC_POS_MIN,
    /* RTC_POS_SEC, */
    RTC_POS_MAX,
    ALM_POS_HOUR,
    ALM_POS_MIN,
    ALM_POS_ENABLE,
    ALM_POS_MAX,
};

struct rtc_opr {
    void *dev_handle;
    u8  rtc_set_mode;
    u8  rtc_pos;
    u8  alm_enable;
    u8  alm_num;
    struct sys_time set_time;
};

static struct rtc_opr *__this = NULL;

const char *alm_string[] =  {" AL ", " ON ", " OFF"};
const char *alm_select[] =  {"AL-1", "AL-2", "AL-3", "AL-4", "AL-5"};

static void ui_set_rtc_timeout(int menu)
{
    if (!__this) {
        return ;
    }
    __this->rtc_set_mode =  RTC_SET_MODE;
    __this->rtc_pos = RTC_POS_NULL;
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 闹钟设置 \ 时间设置转换
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
void set_rtc_sw()
{
#if TCFG_UI_ENABLE
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }

    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }
    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        __this->rtc_set_mode = ALM_SET_MODE;
        __this->rtc_pos = RTC_POS_NULL;
        __this->alm_num = 0;
        rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
        rtc->str = alm_select[__this->alm_num];

        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        __this->alm_num++;
        __this->rtc_pos = RTC_POS_NULL;
        if (__this->alm_num >= sizeof(alm_select) / sizeof(alm_select[0])) {
            __this->rtc_set_mode = RTC_SET_MODE;
            __this->alm_num = 0;
            UI_REFLASH_WINDOW(true);
            break;
        }
        rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
        rtc->str = alm_select[__this->alm_num];
        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    }
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 设置调整时钟的位置
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
void set_rtc_pos()
{
#if TCFG_UI_ENABLE
    T_ALARM alarm = {0};
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }

    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:

        if (__this->rtc_pos == RTC_POS_NULL) {
            __this->rtc_pos = RTC_POS_DEFAULT;
            dev_ioctl(__this->dev_handle, IOCTL_GET_SYS_TIME, (u32)&__this->set_time);
        } else {
            __this->rtc_pos++;
            if (__this->rtc_pos == RTC_POS_MAX) {
                __this->rtc_pos = RTC_POS_NULL;
                rtc_update_time_api(&__this->set_time);
                UI_REFLASH_WINDOW(true);
                break;
            }
        }

        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);

        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;

        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        if (__this->rtc_pos == RTC_POS_NULL) {
            __this->rtc_pos = RTC_ALM_POS_DEFAULT;
            if (alarm_get_info(&alarm, __this->alm_num) != 0) {
                log_error("alarm_get_info \n");
            }

            __this->set_time.hour = alarm.time.hour;
            __this->set_time.min = alarm.time.min;
            __this->alm_enable = alarm.sw;

        } else {
            __this->rtc_pos++;
            if (__this->rtc_pos == ALM_POS_MAX) {
                __this->rtc_pos = RTC_POS_NULL;
                alarm.time.hour = __this->set_time.hour;
                alarm.time.min  = __this->set_time.min;
                alarm.time.sec  = 0;
                alarm.sw = __this->alm_enable;
                alarm.index = __this->alm_num;
                alarm.mode  = 0;
                alarm_add(&alarm, __this->alm_num);
                __this->alm_num++;
                if (__this->alm_num >= sizeof(alm_select) / sizeof(alm_select[0])) {
                    __this->rtc_set_mode = RTC_SET_MODE;
                    __this->alm_num = 0;
                    UI_REFLASH_WINDOW(true);
                } else {
                    rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
                    rtc->str = alm_select[__this->alm_num];
                    app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
                }

                break;
            }
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }

        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    }
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 调整时钟 加时间
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
void set_rtc_up()
{

#if TCFG_UI_ENABLE
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }

    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    if (__this->rtc_pos == RTC_POS_NULL) {
        return ;
    }


    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        switch (__this->rtc_pos) {
        case RTC_POS_YEAR:
            __this->set_time.year++;
            if (__this->set_time.year > MAX_YEAR) {
                __this->set_time.year = MIN_YEAR;
            }
            break;
        case RTC_POS_MONTH:
            if (++__this->set_time.month > 12) {
                __this->set_time.month = 1;
            }
            break;
        case RTC_POS_DAY:
            if (++__this->set_time.day > month_for_day(__this->set_time.month, __this->set_time.year)) {
                __this->set_time.day = 1;
            }
            break;
        case RTC_POS_HOUR:
            if (++__this->set_time.hour >= 24) {
                __this->set_time.hour = 0;
            }
            break;

        case RTC_POS_MIN:
            if (++__this->set_time.min >= 60) {
                __this->set_time.min = 0;
            }
            break;
        }

        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);
        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;
        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;
    case ALM_SET_MODE:

        switch (__this->rtc_pos) {
        case ALM_POS_HOUR:
            if (++__this->set_time.hour >= 24) {
                __this->set_time.hour = 0;
            }
            break;

        case ALM_POS_MIN:
            if (++__this->set_time.min >= 60) {
                __this->set_time.min = 0;
            }
            break;
        case ALM_POS_ENABLE:
            __this->alm_enable = !__this->alm_enable;
            break;
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }

        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    default:
        break;
    }
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 调整时钟 减时间
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
void set_rtc_down()
{
#if TCFG_UI_ENABLE
    if ((!__this) || (!__this->dev_handle)) {
        return;
    }
    struct ui_rtc_display *rtc = rtc_ui_get_display_buf();

    if (!rtc) {
        return;
    }

    if (__this->rtc_pos == RTC_POS_NULL) {
        return ;
    }

    switch (__this->rtc_set_mode) {
    case RTC_SET_MODE:
        switch (__this->rtc_pos) {
        case RTC_POS_YEAR:
            __this->set_time.year--;
            if (__this->set_time.year < MIN_YEAR) {
                __this->set_time.year = MAX_YEAR;
            }
            break;
        case RTC_POS_MONTH:

            if (__this->set_time.month == 1) {
                __this->set_time.month = 12;
            } else {
                __this->set_time.month--;
            }

            break;
        case RTC_POS_DAY:

            if (__this->set_time.day == 1) {
                __this->set_time.day = month_for_day(__this->set_time.month, __this->set_time.year);
            } else {
                __this->set_time.day --;
            }

            break;
        case RTC_POS_HOUR:
            if (__this->set_time.hour == 0) {
                __this->set_time.hour = 23;
            } else {
                __this->set_time.hour--;
            }
            break;
        case RTC_POS_MIN:
            if (__this->set_time.min == 0) {
                __this->set_time.min = 59;
            } else {
                __this->set_time.min--;
            }
            break;
        }

        rtc->rtc_menu = UI_RTC_ACTION_YEAR_SET + (__this->rtc_pos - RTC_POS_YEAR);
        rtc->time.Year = __this->set_time.year;
        rtc->time.Month = __this->set_time.month;
        rtc->time.Day = __this->set_time.day;
        rtc->time.Hour = __this->set_time.hour;
        rtc->time.Min = __this->set_time.min;
        rtc->time.Sec = __this->set_time.sec;
        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    case ALM_SET_MODE:
        switch (__this->rtc_pos) {
        case ALM_POS_HOUR:
            if (__this->set_time.hour == 0) {
                __this->set_time.hour = 23;
            } else {
                __this->set_time.hour--;
            }
            break;

        case ALM_POS_MIN:
            if (__this->set_time.min == 0) {
                __this->set_time.min = 59;
            } else {
                __this->set_time.min--;
            }
            break;

        case ALM_POS_ENABLE:
            __this->alm_enable = !__this->alm_enable;
            break;
        }

        if (ALM_POS_ENABLE == __this->rtc_pos) {
            rtc->rtc_menu = UI_RTC_ACTION_STRING_SET;
            if (__this->alm_enable) {
                rtc->str = " ON ";
            } else {
                rtc->str = " OFF";
            }
        } else {
            rtc->rtc_menu = UI_RTC_ACTION_HOUR_SET + (__this->rtc_pos - ALM_POS_HOUR);
            rtc->time.Year = __this->set_time.year;
            rtc->time.Month = __this->set_time.month;
            rtc->time.Day = __this->set_time.day;
            rtc->time.Hour = __this->set_time.hour;
            rtc->time.Min = __this->set_time.min;
            rtc->time.Sec = __this->set_time.sec;
        }
        app_send_message(APP_MSG_RTC_SET, (int)ui_set_rtc_timeout);
        break;

    default:
        break;
    }
#endif
}

//*----------------------------------------------------------------------------*/
/**@brief    rtc 退出
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void rtc_task_close()
{
#if RCSP_MODE
    extern void rcsp_rtc_mode_exit(void);
    rcsp_rtc_mode_exit();
#endif
    if (__this) {
        if (__this->dev_handle) {
            dev_close(__this->dev_handle);
            __this->dev_handle = NULL;
        }
        free(__this);
        __this = NULL;
    }
}

static void rtc_app_init()
{
    if (!__this) {
        __this = zalloc(sizeof(struct rtc_opr));
        ASSERT(__this, "%s %di \n", __func__, __LINE__);
        __this->dev_handle = dev_open("rtc", NULL);
        if (!__this->dev_handle) {
            ASSERT(0, "%s %d \n", __func__, __LINE__);
        }
    }
    __this->rtc_set_mode =  RTC_SET_MODE;
    __this->rtc_pos = RTC_POS_NULL;

    //ui_update_status(STATUS_RTC_MODE);
}

//*----------------------------------------------------------------------------*/
/**@brief   rtc 启动
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void rtc_task_start()
{
    rtc_app_init();
    if (alarm_active_flag_get()) {
        alarm_ring_start();
    }
}

static int rtc_tone_play_end_callback(void *priv, enum stream_event event)
{
    rtc_task_start();
    return 0;
}

static int app_rtc_init()
{
    int ret = -1;
    log_info("rtc start");

#if TCFG_LOCAL_TWS_ENABLE
    ret = local_tws_enter_mode(NULL, NULL);
#endif //TCFG_LOCAL_TWS_ENABLE

    if (ret == -1) {
#if (RCSP_MODE)
        extern u8 rcsp_rtc_ring_tone(void);
        if (rcsp_rtc_ring_tone()) {
            play_tone_file_callback(get_tone_files()->rtc_mode, NULL, rtc_tone_play_end_callback);
        } else {
            rtc_task_start();
        }
#else
        tone_player_stop();
        int ret = play_tone_file_callback(get_tone_files()->rtc_mode, NULL, rtc_tone_play_end_callback);
        if (ret) {
            log_error("rtc tone play err!!!");
            rtc_task_start();
        }
#endif
    }
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_RTC);
    return 0;
}

static void app_rtc_exit()
{
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
    rtc_task_close();
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_RTC);
}

struct app_mode *app_enter_rtc_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_rtc_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), rtc_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            rtc_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_rtc_exit();

    return next_mode;
}

static int rtc_mode_try_enter(int arg)
{
    return 0;
}

static int rtc_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops rtc_mode_ops = {
    .try_enter          = rtc_mode_try_enter,
    .try_exit           = rtc_mode_try_exit,
};

/*
 * 注册rtc模式
 */
REGISTER_APP_MODE(rtc_mode) = {
    .name 	= APP_MODE_RTC,
    .index  = APP_MODE_RTC_INDEX,
    .ops 	= &rtc_mode_ops,
};

#endif

