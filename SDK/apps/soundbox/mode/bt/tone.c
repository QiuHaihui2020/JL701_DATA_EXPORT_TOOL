#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tone.data.bss")
#pragma data_seg(".tone.data")
#pragma const_seg(".tone.text.const")
#pragma code_seg(".tone.text")
#endif
#include "btstack/avctp_user.h"
#include "classic/tws_api.h"
#include "app_main.h"
#include "soundbox.h"
#include "app_config.h"
#include "bt_tws.h"
#include "app_tone.h"
#include "app_testbox.h"

#if TCFG_APP_BT_EN

#define TWS_DLY_DISCONN_TIME            0//2000    //TWS超时断开，快速连接上不播提示音

static u8 g_tws_connected = 0;
static u16 tws_dly_discon_time = 0;


static int tone_btstack_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;

#if (TCFG_BT_BACKGROUND_ENABLE && TCFG_BACKGROUND_WITHOUT_EDR_CONNECT)
    if (bt_background_active()) {   //后台不打开支持EDR连接不播放提示音
        return 0;
    }
#endif

    switch (event->event) {
    case BT_STATUS_FIRST_CONNECTED:
    case BT_STATUS_SECOND_CONNECTED:
#if TCFG_TEST_BOX_ENABLE
        if (testbox_get_status()) {
            break;
        }
#endif

        /*
         * 获取tws状态，如果正在播歌或打电话则返回1,不播连接成功提示音
         */
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }

        int state = tws_api_get_lmp_state(event->args);
        if (state & TWS_STA_ESCO_OPEN) {
            break;
        }
        tws_play_tone_file(get_tone_files()->bt_connect, 400);
#else
        play_tone_file(get_tone_files()->bt_connect);
#endif
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        /*
         * 关机不播断开提示音
         */
        if (app_var.goto_poweroff_flag) {
            break;
        }
        if (!g_bt_hdl.ignore_discon_tone) {
#if TCFG_USER_TWS_ENABLE
            if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                break;
            }
            tws_play_tone_file(get_tone_files()->bt_disconnect, 400);
#else
            play_tone_file(get_tone_files()->bt_disconnect);

#endif
        }
        if (g_bt_hdl.ignore_discon_tone) {
            g_bt_hdl.ignore_discon_tone --;
        }
        break;
    }

    return 0;
}

APP_MSG_HANDLER(tone_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = tone_btstack_event_handler,
};


#if TCFG_USER_TWS_ENABLE

void tws_disconn_dly_deal(void *priv)
{
    if (tws_dly_discon_time == 0) {
        return;
    }
    tws_dly_discon_time = 0;

    if (app_var.goto_poweroff_flag) {
        return;
    }

    if (!g_bt_hdl.ignore_discon_tone) {
        tone_player_stop();
#if TCFG_KBOX_1T3_MODE_EN
        play_tone_file_alone(get_tone_files()->tws_disconnect);
#else
        play_tone_file(get_tone_files()->tws_disconnect);
#endif
    }
}

static int tone_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int reason = event->args[2];

    switch (event->event) {
    case TWS_EVENT_CONNECTED:
        g_tws_connected = 1;
        if (tws_dly_discon_time) {
            sys_timeout_del(tws_dly_discon_time);
            tws_dly_discon_time = 0;
            break;
        }
        tone_player_stop();
        if (role == TWS_ROLE_MASTER) {
            int state = tws_api_get_tws_state();
            if (state & (TWS_STA_SBC_OPEN | TWS_STA_ESCO_OPEN)) {
                break;
            }
#if TCFG_USER_TWS_ENABLE
            tws_play_tone_file(get_tone_files()->tws_connect, 400);
#else
            play_tone_file(get_tone_files()->tws_connect);
#endif
        }
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        if (app_var.goto_poweroff_flag) {
            break;
        }
        if (!g_tws_connected) {
            break;
        }
        g_tws_connected = 0;

        if (reason == (TWS_DETACH_BY_REMOTE | TWS_DETACH_BY_POWEROFF)) {
            break;
        }

#if TWS_DLY_DISCONN_TIME
        if (reason & TWS_DETACH_BY_SUPER_TIMEOUT) {
            tws_dly_discon_time = sys_timeout_add(NULL, tws_disconn_dly_deal,
                                                  TWS_DLY_DISCONN_TIME);
            break;
        }
#endif
        if (!g_bt_hdl.ignore_discon_tone) {
            tone_player_stop();
#if TCFG_KBOX_1T3_MODE_EN
            play_tone_file_alone(get_tone_files()->tws_disconnect);
#else
            play_tone_file(get_tone_files()->tws_disconnect);
#endif
        } else {
            g_bt_hdl.ignore_discon_tone --;
        }

        break;
    case TWS_EVENT_REMOVE_PAIRS:
        //play_tone_file(get_tone_files()->tws_disconnect);
        break;
    }

    return 0;
}

APP_MSG_HANDLER(tone_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = tone_tws_event_handler,
};
#endif

#endif
