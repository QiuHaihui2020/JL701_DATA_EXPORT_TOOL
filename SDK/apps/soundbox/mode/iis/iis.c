#include "app_main.h"
#include "system/includes.h"
#include "app_action.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "app_tone.h"
#include "gpio_config.h"
#include "audio_config.h"
#include "media/includes.h"
#include "iis_file.h"
#include "audio_iis.h"
#include "iis_player.h"
#include "iis.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "local_tws.h"

#if TCFG_APP_IIS_EN

extern const struct key_remap_table iis_mode_key_table[];

static u8 iis_idle_flag = 1;

static int iis_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_IIS)) {
        return 0;
    }
    switch (event) {
    case STREAM_EVENT_NONE:
    case STREAM_EVENT_STOP:
        app_send_message(APP_MSG_IIS_START, 0);
        break;
    default:
        break;
    }
    return 0;
}

void app_iis_exit()
{
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
    app_set_current_mode(app_get_mode_by_name(APP_MODE_NULL));        //这里把MODE设置成NULL，防止快速切模式的时候提示音还没播完已经退出当前模式，在提示音回调里还继续开广播

    iis_stop();
    iis_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_IIS);
    /* tone_player_stop(); */

}

static int app_iis_init(void)
{
    int ret = -1;
    iis_idle_flag = 0;

    tone_player_stop();

#if TCFG_LOCAL_TWS_ENABLE
    ret = local_tws_enter_mode(get_tone_files()->iis_mode, NULL);
#endif //TCFG_LOCAL_TWS_ENABLE

    if (ret != 0) {
        ret = play_tone_file_callback(get_tone_files()->iis_mode, NULL, iis_tone_play_end_callback);
        if (ret) {
            //提示音播放失败
            iis_tone_play_end_callback(NULL, STREAM_EVENT_NONE);
        }
    }

#ifdef TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;    //设置变调初始模式
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    btstack_init_in_other_mode();
#if (LEA_BIG_FIX_ROLE==2)
    iis_set_broadcast_local_open_flag(1);
#endif
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_IIS);
    return 0;
}

struct app_mode *app_enter_iis_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_iis_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), iis_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            iis_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_iis_exit();

    return next_mode;
}




static int iis_mode_try_enter(int arg)
{
    return 0;
}

static int iis_mode_try_exit()
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    app_broadcast_deal(LE_AUDIO_APP_MODE_EXIT);

#if (!TCFG_BT_BACKGROUND_ENABLE)
    app_broadcast_close_in_other_mode();
#endif

#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    app_connected_deal(LE_AUDIO_APP_MODE_EXIT);

#if (!TCFG_BT_BACKGROUND_ENABLE && !TCFG_KBOX_1T3_MODE_EN)
    app_connected_close_in_other_mode();
#endif

#endif
    btstack_exit_in_other_mode();
    return 0;
}

static const struct app_mode_ops iis_mode_ops = {
    .try_enter      = iis_mode_try_enter,
    .try_exit       = iis_mode_try_exit,
};

/*
 * 注册 iis 模式
 */
REGISTER_APP_MODE(iis_mode) = {
    .name 	= APP_MODE_IIS,
    .index  = APP_MODE_IIS_INDEX,
    .ops 	= &iis_mode_ops,
};

static u8 iis_idle_query(void)
{
    return iis_idle_flag;
}
REGISTER_LP_TARGET(iis_lp_target) = {
    .name = "iis",
    .is_idle = iis_idle_query,
};

static void iis_local_start(void *priv)
{
    iis_start();
}

REGISTER_LOCAL_TWS_OPS(iis) = {
    .name 	= APP_MODE_IIS,
    .local_audio_open = iis_local_start,
    .get_play_status = iis_player_runing,
};

#endif


