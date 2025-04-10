#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tws_poweroff.data.bss")
#pragma data_seg(".tws_poweroff.data")
#pragma const_seg(".tws_poweroff.text.const")
#pragma code_seg(".tws_poweroff.text")
#endif
#include "classic/hci_lmp.h"
#include "btstack/avctp_user.h"

#include "app_config.h"
#include "app_tone.h"
#include "app_main.h"
#include "soundbox.h"
#include "bt_tws.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "idle.h"
#include "app_charge.h"
#include "bt_slience_detect.h"
#include "poweroff.h"
/* #include "audio_anc.h" */

#if(TCFG_USER_TWS_ENABLE)

#define LOG_TAG             "[POWEROFF]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"




static u16 g_poweroff_timer = 0;
static u16 g_bt_detach_timer = 0;


static void sys_auto_shut_down_deal(void *priv);


void sys_auto_shut_down_disable(void)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    log_info("sys_auto_shut_down_disable\n");
    if (g_poweroff_timer) {
        sys_timeout_del(g_poweroff_timer);
        g_poweroff_timer = 0;
    }
#endif
}

void sys_auto_shut_down_enable(void)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    if (!app_in_mode(APP_MODE_BT)) {
        return;
    }
    log_info("sys_auto_shut_down_enable\n");

    /*ANC打开，不支持自动关机*/
#if TCFG_AUDIO_ANC_ENABLE
    if (anc_status_get()) {
        return;
    }
#endif

    if (g_poweroff_timer == 0) {
        g_poweroff_timer = sys_timeout_add(NULL, sys_auto_shut_down_deal,
                                           app_var.auto_off_time * 1000);
    }
#endif
}

#if TCFG_AUTO_SHUT_DOWN_TIME

static void tws_sync_call_poweroff(int priv, int err)
{
    sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
}
TWS_SYNC_CALL_REGISTER(tws_poweroff_entry) = {
    .uuid = 0x963BE1AC,
    .task_name = "app_core",
    .func = tws_sync_call_poweroff,
};

static void sys_auto_shut_down_deal(void *priv)
{
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        sys_enter_soft_poweroff(POWEROFF_NORMAL);
    } else {
        tws_api_sync_call_by_uuid(0x963BE1AC, 0, 400);
    }
}


static int poweroff_app_event_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_BT_IN_PAIRING_MODE:
        if (msg[1] == 0) {
            sys_auto_shut_down_enable();
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(poweroff_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = poweroff_app_event_handler,
};


static int poweroff_btstack_event_handler(int *_event)
{
    struct bt_event *bt = (struct bt_event *)_event;

    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        sys_auto_shut_down_disable();
        break;
    }
    return 0;
}
APP_MSG_HANDLER(poweroff_btstack_msg_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = poweroff_btstack_event_handler,
};



static int poweroff_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int phone_link_connection = event->args[1];

    switch (event->event) {
    case TWS_EVENT_CONNECTED:
        if (phone_link_connection == 0) {
            sys_auto_shut_down_disable();
            sys_auto_shut_down_enable();
        }
        break;
    case TWS_EVENT_MONITOR_START:
        sys_auto_shut_down_disable();
        break;
    }

    return 0;
}
APP_MSG_HANDLER(poweroff_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = poweroff_tws_event_handler,
};

#endif




static void wait_exit_btstack_flag(void *_reason)
{
    int reason = (int)_reason;

    if (!a2dp_player_runing() && !esco_player_runing()) {
        lmp_hci_reset();
        os_time_dly(2);
        sys_timer_del(g_bt_detach_timer);

        switch (reason) {
        case POWEROFF_NORMAL:
            log_info("task_switch to idle...\n");
            app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
            break;
        case POWEROFF_RESET:
            log_info("cpu_reset!!!\n");
            cpu_reset();
            break;
        case POWEROFF_POWER_KEEP:
#if TCFG_CHARGE_ENABLE
            app_charge_power_off_keep_mode();
#endif
            break;
        }
    } else {
        if (++app_var.goto_poweroff_cnt > 200) {
            log_info("cpu_reset!!!\n");
            cpu_reset();
        }
        printf("wait_poweroff_cnt: %d\n", app_var.goto_poweroff_cnt);
    }
}

static void wait_tws_detach_handler(void *p)
{
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        if (++app_var.goto_poweroff_cnt > 300) {
            log_info("cpu_reset!!!\n");
            cpu_reset();
        }
        return;
    }

    app_send_message(APP_MSG_POWER_OFF, 0);

    int state = tws_api_get_tws_state();
    if (state & TWS_STA_PHONE_CONNECTED) {
        bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    }

    sys_timer_del(g_bt_detach_timer);

    app_var.goto_poweroff_cnt = 0;
    g_bt_detach_timer = sys_timer_add(p, wait_exit_btstack_flag, 50);
}

static void bt_tws_detach(void *p)
{
    bt_tws_poweroff();
}

static void tws_slave_power_off(void *p)
{
    app_goto_mode(APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
}

static void tws_poweroff_tone_callback(int priv, enum stream_event event)
{
    r_printf("poweroff_tone_callback: %d\n",  event);
    if (event == STREAM_EVENT_STOP) {
        //bt_tws_detach(NULL);
        app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_IDLE, IDLE_MODE_WAIT_POWEROFF);
        sys_timeout_add(NULL, app_power_off, 1000);
    }
}
REGISTER_TWS_TONE_CALLBACK(tws_poweroff_stub) = {
    .func_uuid  = 0x2EC3509D,
    .callback   = tws_poweroff_tone_callback,
};



static void power_off_at_same_time(void *p)
{
    if (!a2dp_player_runing() && !esco_player_runing()) {
        int state = tws_api_get_tws_state();
        log_info("wait_phone_link_detach: %x\n", state);
        if (state & TWS_STA_PHONE_DISCONNECTED) {
            if (state & TWS_STA_SIBLING_CONNECTED) {
                if (tws_api_get_role() == TWS_ROLE_MASTER) {
                    tws_play_tone_file_callback(get_tone_files()->power_off, 400,
                                                0x2EC3509D);
                    /* sys_timeout_add(NULL, bt_tws_detach, 400); */
                } else {
                    /* 防止TWS异常断开导致从耳不关机 */
                    sys_timeout_add(NULL, tws_slave_power_off, 3000);
                }
                sys_timer_del(g_bt_detach_timer);
            } else {
                sys_timer_del(g_bt_detach_timer);
                app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
            }
        } else {
            lmp_hci_reset();
            os_time_dly(2);
        }
    } else {
        if (++app_var.goto_poweroff_cnt > 200) {
            log_info("cpu_reset!!!\n");
            cpu_reset();
        }
    }
}

void sys_enter_soft_poweroff(enum poweroff_reason reason)
{
    log_info("sys_enter_soft_poweroff: %d\n", reason);

    if (app_var.goto_poweroff_flag) {
        return;
    }

    app_var.goto_poweroff_flag = 1;
    app_var.goto_poweroff_cnt = 0;
    sys_auto_shut_down_disable();

#if TCFG_APP_BT_EN
    void bt_sniff_disable();
    bt_sniff_disable();

    bt_stop_a2dp_slience_detect(NULL);

    void tws_dual_conn_close();
    tws_dual_conn_close();
#endif

    EARPHONE_STATE_ENTER_SOFT_POWEROFF();

#if TCFG_AUDIO_ANC_ENABLE
    anc_poweroff();
#endif

    /* TWS同时关机,先断开手机  */
    if (reason == POWEROFF_NORMAL_TWS) {
        bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
        g_bt_detach_timer = sys_timer_add(NULL, power_off_at_same_time, 50);
        return;
    }

    int wait_tws_detach = 1;
#if TCFG_CHARGESTORE_ENABLE
    if (chargestore_get_earphone_online() != 2) {
        bt_tws_poweroff();
    } else {
        wait_tws_detach = 0;
    }
#else
    bt_tws_poweroff();
#endif

    if (wait_tws_detach) {
        g_bt_detach_timer = sys_timer_add((void *)reason, wait_tws_detach_handler, 50);
        return;
    }
    app_send_message(APP_MSG_POWER_OFF, 0);

    int state = tws_api_get_tws_state();
    if (state & TWS_STA_PHONE_CONNECTED) {
        bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    }

    g_bt_detach_timer = sys_timer_add((void *)reason, wait_exit_btstack_flag, 50);
}

#endif
