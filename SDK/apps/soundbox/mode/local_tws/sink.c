
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "app_tone.h"
#include "local_tws.h"
#include "local_tws_player.h"
#include "app_mode_sink.h"

static enum app_mode_t sink_mode_rsp_arg = 0;

void app_sink_set_sink_mode_rsp_arg(enum app_mode_t arg)
{
    sink_mode_rsp_arg = arg;
}

static int app_sink_init(void)
{
    y_printf("%s\n", __func__);
    local_tws_enter_sink_mode_rsp(sink_mode_rsp_arg);
    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_SINK);
    return 0;
}

void app_sink_exit()
{
    y_printf("%s\n", __func__);
    local_tws_player_close();
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_SINK);
}

int sink_msg_handler(int *msg)
{
    const struct key_remap_table *table;

    if (false == app_in_mode(APP_MODE_SINK)) {
        return 0;
    }
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
#if TCFG_LOCAL_TWS_SYNC_VOL
    case APP_MSG_VOL_UP:
        local_tws_vol_operate(CMD_TWS_VOL_UP);
        break;
    case APP_MSG_VOL_DOWN:
        local_tws_vol_operate(CMD_TWS_VOL_DOWN);
        break;
#endif
    case APP_MSG_MUSIC_PP:
        local_tws_music_operate(CMD_TWS_MUSIC_PP, &sink_mode_rsp_arg);
        break;
    case APP_MSG_MUSIC_PREV:
        local_tws_music_operate(CMD_TWS_MUSIC_PREV, NULL);
        break;
    case APP_MSG_MUSIC_NEXT:
        local_tws_music_operate(CMD_TWS_MUSIC_NEXT, NULL);
        break;

    case APP_MSG_BT_WORK_MODE_CHANGE:
        if (msg[1] == APP_KEY_MSG_FROM_TWS) { //非后台不响应来自tws的切换模式消息
            return 0;
        }
        bt_work_mode_switch_to_next();
        break;

    default:
        app_common_key_msg_handler(msg);
        break;
    }

    return 0;
}

struct app_mode *app_enter_sink_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_sink_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), sink_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            sink_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_sink_exit();

    return next_mode;
}

static int sink_mode_try_enter(int arg)
{
    return 0;
}

static int sink_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops sink_mode_ops = {
    .try_enter      = sink_mode_try_enter,
    .try_exit       = sink_mode_try_exit,
};

/*
 * 注册linein模式
 */
REGISTER_APP_MODE(sink_mode) = {
    .name 	= APP_MODE_SINK,
    .index  = 0xff,
    .ops 	= &sink_mode_ops,
};

