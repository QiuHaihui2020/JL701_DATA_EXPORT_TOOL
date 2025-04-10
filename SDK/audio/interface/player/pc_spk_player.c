#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".pc_spk_player.data.bss")
#pragma data_seg(".pc_spk_player.data")
#pragma const_seg(".pc_spk_player.text.const")
#pragma code_seg(".pc_spk_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "system/timer.h"
#include "system/includes.h"
#include "pc_spk_player.h"
#include "pc_spk_file.h"
#include "app_config.h"
#include "app_main.h"
#include "effects/audio_pitchspeed.h"
#include "effect/effects_default_param.h"
#include "audio_config.h"
#include "scene_switch.h"
#include "uac_stream.h"
#include "audio_cvp.h"

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
#include "app_le_auracast.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[pcspk]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE

typedef enum {
    PC_SPK_STA_CLOSE,
    PC_SPK_STA_WAIT_CLOSE,
    PC_SPK_STA_OPEN,
    PC_SPK_STA_WAIT_OPEN,
} pc_spk_state_t;
pc_spk_state_t g_pc_spk_state;

struct pc_spk_player {
    struct jlstream *stream;
    s8 pc_spk_pitch_mode;
};
static struct pc_spk_player *g_pc_spk_player = NULL;

extern void dac_try_power_on_task_delete();

static u8 pc_player_status = 0;

/*
 * @description: 判断usb检测是否有数
 * @return：1 有数 0 无数
 * @node:
 */
bool pc_get_status()
{
    /* g_printf("--pc_get_status is%d-", pc_player_status); */
    return pc_player_status;
}

static void pc_spk_player_callback(void *private_data, int event)
{
    struct pc_spk_player *player = g_pc_spk_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:

#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
        //先开pc mic，后开spk，需要取消忽略外部数据，重启aec
        if (audio_aec_status()) {
            audio_aec_reboot(0);
            audio_cvp_ref_data_align_reset();
        }
#endif

        if (app_get_current_mode()->name == APP_MODE_PC) {
            s16 cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
            u16 l_vol = 0, r_vol = 0;
            uac_speaker_stream_get_volume(&l_vol, &r_vol);
            if (cur_vol != (r_vol + l_vol) / 2) {
                app_audio_set_volume(APP_AUDIO_STATE_MUSIC, (r_vol + l_vol) / 2, 1);
            }
        }
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
        musci_vocal_remover_update_parm();
#endif
        break;
    }
}

/*
 * @description: 打开 pc spk 数据流
 * @return：0 - 成功。其它值失败
 * @node:
 */
int pc_spk_player_open(void)
{
    int err = 0;
    struct pc_spk_player *player = NULL;;

    if (g_pc_spk_player || !app_in_mode(APP_MODE_PC)) {
        return 0;
    }

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"pc_spk");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }
    player->pc_spk_pitch_mode = PITCH_0;
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_PC_SPK);

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }
    u16 l_vol = 0, r_vol = 0;
    uac_speaker_stream_get_volume(&l_vol, &r_vol);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, (r_vol + l_vol) / 2, 1);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 192);
    jlstream_set_callback(player->stream, player->stream, pc_spk_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_PC_SPK);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    } else {
        dac_try_power_on_task_delete();
    }

    g_pc_spk_state = PC_SPK_STA_OPEN;
    g_pc_spk_player = player;
    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

// 返回1说明player 在运行
bool pc_spk_player_runing()
{
    return g_pc_spk_player != NULL;
}

/*
 * @description: 关闭spk 数据流
 * @return：
 * @node:
 */
void pc_spk_player_close(void)
{
    struct pc_spk_player *player = g_pc_spk_player;

    if (!player) {
        if (g_pc_spk_state == PC_SPK_STA_WAIT_OPEN || g_pc_spk_state == PC_SPK_STA_OPEN) {
            //可能在切到其它模式瞬间有spk音频起来，打开spk采用的是信号量方式打开，导致关闭spk的动作比打开的动作更提前
            log_debug("err, [%s], player_wait_open_flag:1!\n", __func__);
            pcspk_close_player_by_taskq();
        }
        return;
    }

#if 0//pc + bt 通过mixer叠加的环境， 因usbrx已经停止，无法驱动数据流。需手动提前将当前的mixer ch关闭
    struct mixer_ch_pause pause = {0};
    pause.ch_idx = 7;
    jlstream_set_node_param(NODE_UUID_MIXER, "MIXER27", &pause, sizeof(pause));
#endif
    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);
    free(player);
    player = NULL;
    g_pc_spk_player = NULL;
#if TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE && (defined TCFG_IIS_NODE_ENABLE)
    if (audio_aec_status()) {
        //忽略参考数据
        audio_cvp_ioctl(CVP_OUTWAY_REF_IGNORE, 1, NULL);
        audio_cvp_ref_data_align_reset();
    }
#endif
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"pc_spk");

    g_pc_spk_state = PC_SPK_STA_CLOSE;
}

static void pc_spk_player_restert(void)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (!get_broadcast_role()) {
        if (g_pc_spk_state == PC_SPK_STA_OPEN) {
            pc_spk_player_close();
            pc_spk_player_open();
        }
    } else { //广播模式则重启广播数据流
        le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
        le_audio_scene_deal(LE_AUDIO_MUSIC_START);
    }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (!get_auracast_role()) {
        if (g_pc_spk_state == PC_SPK_STA_OPEN) {
            pc_spk_player_close();
            pc_spk_player_open();
        }
    } else { //广播模式则重启广播数据流
        le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
        le_audio_scene_deal(LE_AUDIO_MUSIC_START);
    }
#else
    if (g_pc_spk_state == PC_SPK_STA_OPEN) {
        pc_spk_player_close();
        pc_spk_player_open();
    }
#endif
}


int pcspk_close_player_by_taskq(void)
{
    int msg[2];
    int ret = 0;
    if (g_pc_spk_state == PC_SPK_STA_OPEN ||
        g_pc_spk_state == PC_SPK_STA_WAIT_OPEN) {

        g_pc_spk_state = PC_SPK_STA_WAIT_CLOSE;
        msg[0] = (int)pc_spk_player_close;
        msg[1] = 0;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    }
    return ret;
}


int pcspk_open_player_by_taskq(void)
{
    int msg[2];
    int ret = 0;
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && !TCFG_KBOX_1T3_MODE_EN
    if ((g_pc_spk_state == PC_SPK_STA_CLOSE ||
         g_pc_spk_state == PC_SPK_STA_WAIT_CLOSE) && !get_broadcast_role()) {
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if ((g_pc_spk_state == PC_SPK_STA_CLOSE ||
         g_pc_spk_state == PC_SPK_STA_WAIT_CLOSE) && !get_auracast_role()) {
#else
    if (g_pc_spk_state == PC_SPK_STA_CLOSE ||
        g_pc_spk_state == PC_SPK_STA_WAIT_CLOSE) {
#endif
        g_pc_spk_state = PC_SPK_STA_WAIT_OPEN;
        msg[0] = (int)pc_spk_player_open;
        msg[1] = 0;
        ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    }
    return ret;
}

int pcspk_restart_player_by_taskq(void)
{
    int msg[2];
    int ret = 0;
    msg[0] = (int)pc_spk_player_restert;
    msg[1] = 0;
    ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    return ret;
}

static void pc_spk_set_volume(void)
{
    if (app_get_current_mode()->name == APP_MODE_PC) {
        s16 cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        u16 l_vol = 0, r_vol = 0;
        uac_speaker_stream_get_volume(&l_vol, &r_vol);
        if (cur_vol != ((l_vol + r_vol) / 2)) {
            app_audio_set_volume(APP_AUDIO_STATE_MUSIC, ((l_vol + r_vol) / 2), 1);
            printf(">>> pc vol: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
            app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        }
    }
}

int pcspk_set_volume_by_taskq(void)
{
    int msg[2];
    int ret = -1;
#if TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
    msg[0] = (int)pc_spk_set_volume;
    msg[1] = 0;
    ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
#endif
    return ret;
}


int usb_device_event_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_PC_AUDIO_PLAY_OPEN:
        pc_player_status = 1;
        printf("APP_MSG_PC_AUDIO_PLAY_OPEN\n");
#if TCFG_KBOX_1T3_MODE_EN
        if (pc_spk_player_runing() == 0) {
            //打开播放器
            pc_spk_player_open();
        }
#else
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        if (!get_broadcast_role()) {
            if (pc_spk_player_runing() == 0) {
                //打开播放器
                pc_spk_player_open();
            }
        } else {
            le_audio_scene_deal(LE_AUDIO_MUSIC_START);
        }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        if (!get_auracast_role()) {
            if (pc_spk_player_runing() == 0) {
                //打开播放器
                pc_spk_player_open();
            }
        } else {
            le_audio_scene_deal(LE_AUDIO_MUSIC_START);
        }
#else
        if (pc_spk_player_runing() == 0) {
            //打开播放器
            pc_spk_player_open();
        }
#endif
#endif
        break;
    case APP_MSG_PC_AUDIO_PLAY_CLOSE:
        pc_player_status = 0;
        printf("APP_MSG_PC_AUDIO_PLAY_CLOSE\n");
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        if (get_broadcast_role()) {
            //广播（发送端）
            printf(">>[PC] spk lost audio stream, broadcast audio need suspend!\n");
            le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
        } else {
            pc_spk_player_close();
        }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        if (get_auracast_role()) {
            printf(">>[PC] spk lost audio stream, broadcast audio need suspend!\n");
            le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
        } else {
            pc_spk_player_close();
        }
#else
        pc_spk_player_close();
#endif
        break;
    }
    return 0;
}

APP_MSG_HANDLER(usb_device_app_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = usb_device_event_handler,
};

#else

bool pc_spk_player_runing()
{
    return 0;
}


#endif


