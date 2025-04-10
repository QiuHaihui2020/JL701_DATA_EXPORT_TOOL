#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".fm.data.bss")
#pragma data_seg(".fm.data")
#pragma const_seg(".fm.text.const")
#pragma code_seg(".fm.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "app_tone.h"
#include "fm.h"
#include "fm_manage.h"
#include "fm_api.h"
#include "fm_player.h"
#include "effect/effects_default_param.h"
#include "scene_switch.h"
#include "local_tws.h"
#include "wireless_trans.h"
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#include "app_le_broadcast.h"
#endif

#if TCFG_APP_FM_EN

//开关fm后，是否保持变调状态
#define FM_PLAYBACK_PITCH_KEEP          0

#define LOG_TAG             "[APP_FM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

static u8 fm_idle_flag = 1;
static u8 *fm_code_run_addr = NULL;

static spinlock_t fm_code_ram;
extern u32 __fm_movable_slot_start[];
extern u32 __fm_movable_slot_end[];
extern u8 __fm_movable_region_start[];
extern u8 __fm_movable_region_end[];


static void fm_app_start(void)
{
    if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
        return;
    }

    fm_player_open();
    /* fm_manage_start(); */

#if (TCFG_PITCH_SPEED_NODE_ENABLE && FM_PLAYBACK_PITCH_KEEP)
    audio_pitch_default_parm_set(app_var.pitch_mode);
    fm_file_pitch_mode_init(app_var.pitch_mode);
#endif
}

void fm_local_start(void *priv)
{
    fm_app_start();
}

static int fm_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_FM)) {
        return 0;
    }
    switch (event) {
    case STREAM_EVENT_STOP:
        ///提示音播放结束，启动播放器播放
        fm_app_start();

        break;
    default:
        break;
    }
    return 0;
}

static void app_fm_init()
{
    log_info("\n --------fm start-----------\n");
#ifdef CONFIG_CPU_BR29

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    app_broadcast_close_in_other_mode();
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    app_connected_close_in_other_mode();
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)||(TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SOURCE_EN)
    app_auracast_close_in_other_mode();
#endif

    btstack_exit_in_other_mode();

#if TCFG_BT_BACKGROUND_ENABLE
    btstack_exit_for_app();        //br29蓝牙和FM共RF，进入FM需要关闭蓝牙
#endif

#endif

#if TCFG_CODE_RUN_RAM_FM_CODE
    int fm_code_size = __fm_movable_region_end - __fm_movable_region_start;
    printf("fm_code_size:%d\n", fm_code_size);
    mem_stats();

    if (fm_code_size && fm_code_run_addr == NULL) {
        fm_code_run_addr = phy_malloc(fm_code_size);
    }

    spin_lock(&fm_code_ram);
    if (fm_code_run_addr) {
        printf("fm_code_run_addr:0x%x", (unsigned int)fm_code_run_addr);
        code_movable_load(__fm_movable_region_start, fm_code_size, fm_code_run_addr, __fm_movable_slot_start, __fm_movable_slot_end);
    }
    spin_unlock(&fm_code_ram);
    mem_stats();
#endif


    fm_idle_flag = 0;
    int ret = fm_manage_init();
    if (ret == -1) {
        // 找不到FM设备, 切到下一个模式
        r_printf(">>>>>>>>>>>>>>> Not Found FM Dev, Exit FM Mode!\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        return;
    }
    fm_api_init();//设置频率信息(from vm)

#if TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;    //设置变调初始模式
#endif

#ifndef CONFIG_CPU_BR29
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    btstack_init_in_other_mode();
#endif
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_FM);

    ret = -1;
#ifndef CONFIG_CPU_BR29
#if TCFG_LOCAL_TWS_ENABLE
    ret = local_tws_enter_mode(get_tone_files()->fm_mode, NULL);
#endif //TCFG_LOCAL_TWS_ENABLE
#endif

    if (ret != 0) {
        tone_player_stop();
        ret = play_tone_file_callback(get_tone_files()->fm_mode, NULL, fm_tone_play_end_callback);
        if (ret) {
            log_error("fm tone play err!!!");
        }
    }
}

void app_fm_exit(struct app_mode *next_mode)
{
#ifndef CONFIG_CPU_BR29
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
#endif
    tone_player_stop(); //避免fm模式提示音在fm_manage_close调用tone_player_stop后创建fm数据流
    fm_player_close();
    fm_api_release();
    fm_manage_close();
    //tone_play_stop_by_path(tone_table[IDEX_TONE_FM]);
    fm_idle_flag = 1;

#if TCFG_CODE_RUN_RAM_FM_CODE
    if (fm_code_run_addr) {
        mem_stats();

        spin_lock(&fm_code_ram);
        code_movable_unload(__fm_movable_region_start, __fm_movable_slot_start, __fm_movable_slot_end);
        spin_unlock(&fm_code_ram);

        phy_free(fm_code_run_addr);
        fm_code_run_addr = NULL;

        mem_stats();
        printf("\n-------------fm_exit ok-------------\n");
    }
#endif
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_FM);

#ifdef CONFIG_CPU_BR29
#if TCFG_KBOX_1T3_MODE_EN
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    if (next_mode && (next_mode->name != APP_MODE_BT && next_mode->name != APP_MODE_RTC)) {
        btstack_init_in_other_mode();
    }
#endif
#endif

#if TCFG_BT_BACKGROUND_ENABLE
    btstack_init_for_app();
#endif

#endif
}

struct app_mode *app_enter_fm_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_fm_init();
    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), fm_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            fm_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_fm_exit(next_mode);

    return next_mode;
}

static int fm_mode_try_enter(int arg)
{
    if (fm_manage_check_online() == 0) {
        return 0;
    }
    return -1;
}

static int fm_mode_try_exit()
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

static const struct app_mode_ops fm_mode_ops = {
    .try_enter          = fm_mode_try_enter,
    .try_exit           = fm_mode_try_exit,
};

/*
 * 注册fm模式
 */
REGISTER_APP_MODE(fm_mode) = {
    .name 	= APP_MODE_FM,
    .index  = APP_MODE_FM_INDEX,
    .ops 	= &fm_mode_ops,
};

REGISTER_LOCAL_TWS_OPS(fm) = {
    .name 	= APP_MODE_FM,
    .local_audio_open = fm_local_start,
    .get_play_status = fm_player_runing,
};

static u8 fm_idle_query(void)
{
    return fm_idle_flag;
}
REGISTER_LP_TARGET(fm_lp_target) = {
    .name = "fm",
    .is_idle = fm_idle_query,
};

#endif

