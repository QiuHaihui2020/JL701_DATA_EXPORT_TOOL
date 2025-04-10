#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein.data.bss")
#pragma data_seg(".linein.data")
#pragma const_seg(".linein.text.const")
#pragma code_seg(".linein.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "app_tone.h"
#include "linein.h"
#include "linein_dev.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "linein_player.h"
#include "local_tws.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"

#if TCFG_APP_LINEIN_EN

static u8 linein_idle_flag = 1;
//*----------------------------------------------------------------------------*/
/**@brief    linein 入口
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static int linein_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_LINEIN)) {
        return 0;
    }
    switch (event) {
    /* case STREAM_EVENT_NONE: */
    case STREAM_EVENT_STOP:
        app_send_message(APP_MSG_LINEIN_START, 0);
        break;
    default:
        break;
    }
    return 0;
}

static int app_linein_init()
{
    int ret = -1;
    linein_idle_flag = 0;
    //开启ui

    /* ui_update_status(STATUS_LINEIN_MODE); */

#if TCFG_LOCAL_TWS_ENABLE
    ret = local_tws_enter_mode(get_tone_files()->linein_mode, NULL);
#endif //TCFG_LOCAL_TWS_ENABLE

    if (ret != 0) {
        tone_player_stop();
        int ret = play_tone_file_callback(get_tone_files()->linein_mode, NULL, linein_tone_play_end_callback);
        if (ret) {
            linein_tone_play_end_callback(NULL, STREAM_EVENT_NONE); // 提示音播放失败就直接调用 linein start
        }
    }
#if TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;    //设置变调初始模式
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    btstack_init_in_other_mode();
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_LINEIN);
    return 0;
}

void app_linein_exit()
{
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
    linein_stop();
    linein_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_LINEIN);
    /* tone_player_stop(); */

}


struct app_mode *app_enter_linein_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_linein_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), linein_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            linein_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_linein_exit();

    return next_mode;
}

static int linein_mode_try_enter(int arg)
{
    /* return 0;//for test 暂不检查插入 */
    if (linein_is_online()) {
        return 0;
    }
    return -1;
}

static int linein_mode_try_exit()
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE && !TCFG_KBOX_1T3_MODE_EN)
    app_broadcast_close_in_other_mode();
#endif
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE && !TCFG_KBOX_1T3_MODE_EN)
    app_connected_close_in_other_mode();
#endif
#endif

#if (!TCFG_KBOX_1T3_MODE_EN)
    btstack_exit_in_other_mode();
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE)
    app_auracast_close_in_other_mode();
    btstack_exit_in_other_mode();
#endif
#endif

    return 0;
}

static const struct app_mode_ops linein_mode_ops = {
    .try_enter      = linein_mode_try_enter,
    .try_exit       = linein_mode_try_exit,
};

/*
 * 注册linein模式
 */
REGISTER_APP_MODE(linein_mode) = {
    .name 	= APP_MODE_LINEIN,
    .index  = APP_MODE_LINEIN_INDEX,
    .ops 	= &linein_mode_ops,
};

static u8 linein_idle_query(void)
{
    return linein_idle_flag;
}
REGISTER_LP_TARGET(linein_lp_target) = {
    .name = "linein",
    .is_idle = linein_idle_query,
};

#endif
