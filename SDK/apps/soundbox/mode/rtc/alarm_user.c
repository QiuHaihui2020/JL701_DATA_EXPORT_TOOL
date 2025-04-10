#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".alarm_user.data.bss")
#pragma data_seg(".alarm_user.data")
#pragma const_seg(".alarm_user.text.const")
#pragma code_seg(".alarm_user.text")
#endif
#include "system/includes.h"
#include "alarm.h"
#include "system/timer.h"
#include "app_main.h"
#include "tone_player.h"
#include "app_task.h"
#include "app_tone.h"
#include "app_config.h"

#if TCFG_APP_RTC_EN

#define ALARM_RING_MAX 50
volatile u16 g_alarm_ring_cnt = 0;
static u16 g_ring_playing_timer = 0;

/*************************************************************
  此文件函数主要是用户主要修改的文件

  void set_rtc_default_time(struct sys_time *t)
  设置默认时间的函数

  void alm_wakeup_isr(void)
  闹钟到达的函数

  void alarm_event_handler(struct sys_event *event, void *priv)
  监听按键消息接口,应用于随意按键停止闹钟

  int alarm_sys_event_handler(struct sys_event *event)
  闹钟到达响应接口

  void alarm_ring_start()
  闹钟铃声播放接口

  void alarm_stop(void)
  闹钟铃声停止播放接口
 **************************************************************/

void set_rtc_default_time(struct sys_time *t)
{
    t->year = 2019;
    t->month = 5;
    t->day = 5;
    t->hour = 18;
    t->min = 18;
    t->sec = 18;
}

__attribute__((weak))
u8 rtc_app_alarm_ring_play(u8 alarm_state)
{
    return 0;
}

void alm_wakeup_isr(void)
{
    int msg[2];

    msg[0] = (u32)DEVICE_EVENT_FROM_ALM;
    if (!is_sys_time_online()) {
        alarm_active_flag_set(true);
    } else {
        alarm_active_flag_set(true);
        msg[1] = DEVICE_EVENT_IN;
#if (TCFG_DEV_MANAGER_ENABLE)
        os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 2, msg);
#else
        os_taskq_post_type("app_core", MSG_FROM_DEVICE, 2, msg);
#endif
    }
}

void alarm_ring_cnt_clear(void)
{
    g_alarm_ring_cnt = 0;
}

void alarm_stop(void)
{
    printf("ALARM_STOP !!!\n");
    alarm_active_flag_set(0);
    alarm_ring_cnt_clear();
    rtc_app_alarm_ring_play(0);
}

void alarm_play_timer_del(void)
{
    if (g_ring_playing_timer) {
        sys_timeout_del(g_ring_playing_timer);
        g_ring_playing_timer = 0;
    }
}

static void  __alarm_ring_play(void *p)
{
    if (g_alarm_ring_cnt > 0) {
        if (false == app_in_mode(APP_MODE_RTC)) {
            alarm_stop();
            printf("...not in rtc task\n");
            return;
        }

        if (!tone_player_runing()) {
            if (!rtc_app_alarm_ring_play(1)) {
                play_tone_file(get_tone_files()->normal);
                g_alarm_ring_cnt--;
            }
        }
        g_ring_playing_timer = sys_timeout_add(NULL, __alarm_ring_play, 500);
    } else {
        alarm_stop();
    }
}

void alarm_ring_start()
{
    if (g_alarm_ring_cnt == 0) {
        g_alarm_ring_cnt = ALARM_RING_MAX;
        sys_timeout_add(NULL, __alarm_ring_play, 500);
    }
}

//监听按键消息,应用于闹钟响铃时，任意按键按下，闹钟停止
static int alarm_event_handler(int *msg)
{
    if (alarm_active_flag_get()) {
        alarm_stop();
        return 1;//截获按键消息,app应用不会收到消息
    } else {
        return 0;//消息继续分发
    }
}

APP_MSG_PROB_HANDLER(alarm_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_RTC,
    .handler    = alarm_event_handler,
};

#endif

