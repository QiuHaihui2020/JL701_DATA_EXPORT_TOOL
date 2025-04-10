#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc.data.bss")
#pragma data_seg(".pc.data")
#pragma const_seg(".pc.text.const")
#pragma code_seg(".pc.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "audio_config.h"
#include "usb/device/usb_stack.h"
#include "usb/usb_task.h"
#include "usb/device/hid.h"
#include "usb/device/msd.h"
#include "uac_stream.h"
#include "pc.h"
#include "tone_player.h"
#include "app_tone.h"
#include "user_cfg.h"
#include "app_task.h"
#include "app_main.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "local_tws.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"
#include "soundbox.h"
#include "pc_spk_player.h"

#define LOG_TAG_CONST       PC
#define LOG_TAG             "[PC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_APP_PC_EN

struct pc_opr {
    s16 volume;
    u8 onoff : 1;
};

static struct pc_opr pc_hdl = {0};
#define __this 	(&pc_hdl)
static u8 pc_idle_flag = 1;



//*----------------------------------------------------------------------------*/
/**@brief    pc 在线检测  切换模式判断使用
  @param    无
  @return   1 linein设备在线 0 设备不在线
  @note
 */
/*----------------------------------------------------------------------------*/
static int app_pc_check(void)
{
#if ((defined TCFG_PC_BACKMODE_ENABLE) && (TCFG_PC_BACKMODE_ENABLE))
    return false;
#endif//TCFG_PC_BACKMODE_ENABLE

    u32 r = usb_otg_online(0);
    log_info("pc_app_check %d", r);
    if ((r == SLAVE_MODE) ||
        (r == SLAVE_MODE_WAIT_CONFIRMATION)) {
        return true;
    }
    return false;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 打开
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void pc_task_start(void)
{
    if (__this->onoff) {
        log_info("PC is start ");
        return ;
    }
    log_info("App Start - PC");

#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    pc_dm_multiplex_init();
#endif
    /* app_status_handler(APP_STATUS_PC); */
#if TCFG_PC_ENABLE
    usb_message_to_stack(USBSTACK_START, 0, 1);
#endif

#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    usb_otg_resume(0);
#endif
    __this->onoff = 1;
}

//*----------------------------------------------------------------------------*/
/**@brief    pc 关闭
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void pc_task_stop(void)
{
    if (!__this->onoff) {
        log_info("PC is stop ");
        return ;
    }
    __this->onoff = 0;
    u32 state = usb_otg_online(0);
    if (state != SLAVE_MODE && state != SLAVE_MODE_WAIT_CONFIRMATION) {
        log_info("App Stop - PC");
#if TCFG_PC_ENABLE
        usb_message_to_stack(USBSTACK_STOP, 0, 1);
#endif
    } else {
        log_info("App Hold- PC");
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
        usb_otg_suspend(0, 0);
#endif
#if TCFG_PC_ENABLE
        usb_message_to_stack(USBSTACK_PAUSE, 0, 1);
#endif
    }

    /* tone_play_stop(); */
    /* tone_play_stop_by_path(tone_table[IDEX_TONE_PC]);//停止播放提示音 */
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
    pc_dm_multiplex_exit();
#endif

#if (TCFG_DEV_MANAGER_ENABLE)
    dev_manager_list_check_mount();
#endif/*TCFG_DEV_MANAGER_ENABLE*/

}

static int pc_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_PC)) {
        return 0;
    }
    switch (event) {
    /* case STREAM_EVENT_NONE: */
    case STREAM_EVENT_STOP:
        pc_task_start();
        app_send_message(APP_MSG_PC_START, 0);
        break;
    default:
        break;
    }
    return 0;
}

void pc_local_start(void *priv)
{
    pc_task_start();
}

static void app_pc_init()
{
    int ret = -1;
    pc_idle_flag = 0;
    /* ui_update_status(STATUS_PC_MODE); */
    __this->volume =  app_audio_get_volume(APP_AUDIO_STATE_MUSIC);//记录下当前音量
    tone_player_stop();

#if TCFG_LOCAL_TWS_ENABLE
    ret = local_tws_enter_mode(get_tone_files()->pc_mode, NULL);
#endif //TCFG_LOCAL_TWS_ENABLE

    if (ret != 0) {
        ret = play_tone_file_callback(get_tone_files()->pc_mode, NULL, pc_tone_play_end_callback);
        if (ret) {
            log_error("pc tone play err!!!");
            pc_task_start();
            app_send_message(APP_MSG_PC_START, 0);
        }
    }

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    btstack_init_in_other_mode();
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_PC);
}

//*----------------------------------------------------------------------------*/
/**@brief    pc模式 退出
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void app_pc_exit()
{
    pc_task_stop();
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
    pc_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_PC);
}

struct app_mode *app_enter_pc_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_pc_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), pc_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            pc_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_pc_exit();

    return next_mode;
}

static int pc_mode_try_enter(int arg)
{
    if (true == app_pc_check()) {
        return 0;
    }
    return 1;
}

static int pc_mode_try_exit()
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

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE)
    app_auracast_close_in_other_mode();
    btstack_exit_in_other_mode();
#endif
#endif

    return 0;
}

static const struct app_mode_ops pc_mode_ops = {
    .try_enter = pc_mode_try_enter,
    .try_exit  = pc_mode_try_exit,
};

/* 注册pc模式 */
REGISTER_APP_MODE(pc_mode) = {
    .name 	= APP_MODE_PC,
    .index  = APP_MODE_PC_INDEX,
    .ops 	= &pc_mode_ops,
};

static u8 pc_idle_query(void)
{
    return pc_idle_flag;
}

REGISTER_LP_TARGET(pc_lp_target) = {
    .name = "pc",
    .is_idle = pc_idle_query,
};

REGISTER_LOCAL_TWS_OPS(pc) = {
    .name 	= APP_MODE_PC,
    .local_audio_open = pc_local_start,
    .get_play_status =  pc_spk_player_runing,
};
#endif
