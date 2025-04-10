#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein_api.data.bss")
#pragma data_seg(".linein_api.data")
#pragma const_seg(".linein_api.text.const")
#pragma code_seg(".linein_api.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "media/includes.h"
#include "tone_player.h"
#include "app_tone.h"
/* #include "audio_dec_linein.h" */
#include "asm/audio_linein.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "linein_dev.h"
#include "linein.h"
#include "user_cfg.h"
#include "audio_config.h"
#include "ui/ui_api.h"
/* #include "fm_emitter/fm_emitter_manage.h" */
/* #include "bt.h" */
#include "bt_tws.h"
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "application/audio_output_dac.h"
#endif
#include "linein_player.h"
#include "effect/effects_default_param.h"
#include "scene_switch.h"
#include "local_tws.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "app_le_auracast.h"
#include "bt_key_func.h"
#include "btstack_rcsp_user.h"
/*************************************************************
  此文件函数主要是linein实现api
 **************************************************************/

#if TCFG_APP_LINEIN_EN

//开关linein后，是否保持变调状态
#define LINEIN_PLAYBACK_PITCH_KEEP          0

#define LOG_TAG             "[APP_LINEIN]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct linein_opr {
    s16 volume ;
    u8 onoff;
    u8 audio_state; /*判断linein模式使用模拟音量还是数字音量*/
    //以下两个变量都是为了解决问题：固定为接收端时，关闭广播恢复播放状态的问题
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    u8 onoff_as_broadcast_receive;	//在接收模式下记录的linein状态
    u8 last_run_local_audio_close;	//上一次是否有跑 local_audio_close 函数
#if (LEA_BIG_FIX_ROLE==1)
    //固定为发送端
    //暂停中开广播再关闭：暂停。暂停中开广播点击pp后关闭：播放。播歌开广播点击pp后关闭广播：暂停. 该变量为1时表示关闭广播时需要本地音频需要是播放状态
    u8 linein_local_audio_resume_onoff;
#endif
#endif
};
static struct linein_opr linein_hdl = {0};
#define __this 	(&linein_hdl)

static int le_audio_linein_volume_pp(void);

/*----------------------------------------------------------------------------*/
/*@brief   linein 音量设置函数
  @param    需要设置的音量
  @return
  @note    在linein 情况下针对了某些情景进行了处理，设置音量需要使用独立接口
*/
/*----------------------------------------------------------------------------*/
int linein_volume_set(s16 vol)
{
    app_audio_set_volume(__this->audio_state, vol, 1);
    log_info("linein vol: %d", __this->volume);
    __this->volume = vol;

    return true;
}

///*----------------------------------------------------------------------------*/
/**@brief    linein 设置硬件
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
int linein_start(void)
{
    if (__this->onoff == 1) {
        log_info("linein is aleady start\n");
        return true;
    }

    /* set_dac_start_delay_time(1, 1); //设置dac通道的启动延时 */
    linein_player_open();
#if (TCFG_PITCH_SPEED_NODE_ENABLE && LINEIN_PLAYBACK_PITCH_KEEP)
    audio_pitch_default_parm_set(app_var.pitch_mode);
    linein_file_pitch_mode_init(app_var.pitch_mode);
#endif
    musci_vocal_remover_update_parm();
    __this->audio_state = APP_AUDIO_STATE_MUSIC;
    __this->volume = app_audio_get_volume(__this->audio_state);
    __this->onoff = 1;

    /* UI_REFLASH_WINDOW(false);//刷新主页并且支持打断显示 */
    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief    linein stop
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void linein_stop(void)
{
    if (__this->onoff == 0) {
        log_info("linein is aleady stop\n");
        return;
    }
    /* set_dac_start_delay_time(0, 0); //把dac的延时设置回默认的配置 */
    linein_player_close();
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
    __this->onoff = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    linein start stop 播放暂停切换
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/

int linein_volume_pp(void)
{
    int ret = 0;

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (get_broadcast_role()) {
        ret = le_audio_linein_volume_pp();
    } else {
        if (__this->onoff) {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (get_auracast_role()) {
        ret = le_audio_linein_volume_pp();
    } else {
        if (__this->onoff) {
            update_app_auracast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_auracast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if ((app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) ||
        (app_get_connected_role() == APP_CONNECTED_ROLE_DUPLEX)) {
        ret = le_audio_linein_volume_pp();
    } else {
        if (__this->onoff) {
            update_app_connected_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_connected_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif

    if (ret <= 0) {
        if (__this->onoff) {
            linein_stop();
        } else {
            linein_start();
        }
    }

    log_info("pp:%d \n", __this->onoff);
    /* UI_REFLASH_WINDOW(true); */
    return  __this->onoff;
}

/**@brief   获取linein 播放状态
  @param    无
  @return   1:当前正在打开 0：当前正在关闭
  @note
 */
/*----------------------------------------------------------------------------*/

u8 linein_get_status(void)
{
    return __this->onoff;
}

/*
   @note    在linein 情况下针对了某些情景进行了处理，设置音量需要使用独立接口
 */
void linein_tone_play_callback(void *priv, int flag) //模拟linein用数字音量调节播提示音要用这个回调
{
    linein_volume_pp();
}

void linein_tone_play(u8 index, u8 preemption)
{
    if (preemption) { //抢断播放
        linein_volume_pp();
#if 0
        tone_play_index_with_callback(index, preemption, linein_tone_play_callback, NULL);
#endif
    } else {
#if 0
        tone_play_index_with_callback(index, preemption, NULL, NULL);
#endif
    }
}

void linein_key_vol_up()
{
    s16 vol;
    if (__this->volume < app_audio_volume_max_query(AppVol_LINEIN)) {
        __this->volume ++;
        linein_volume_set(__this->volume);
    } else {
        linein_volume_set(__this->volume);
        if (tone_player_runing() == 0) {
            /* tone_play(TONE_MAX_VOL); */
#if TCFG_MAX_VOL_PROMPT
            play_tone_file(get_tone_files()->max_vol);
#endif
        }
    }


#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
        bt_key_rcsp_vol_up();
    }
#endif


    vol = __this->volume;
    app_send_message(APP_MSG_VOL_CHANGED, vol);
    log_info("vol+:%d\n", __this->volume);
}

/*
   @note    在linein 情况下针对了某些情景进行了处理，设置音量需要使用独立接口
 */
void linein_key_vol_down()
{
    s16 vol;
    if (__this->volume) {
        __this->volume --;
        linein_volume_set(__this->volume);
    }


#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
        bt_key_rcsp_vol_down();
    }
#endif


    vol = __this->volume;
    app_send_message(APP_MSG_VOL_CHANGED, vol);
    log_info("vol-:%d\n", __this->volume);
}

void linein_local_start(void *priv)
{
    linein_start();
}

REGISTER_LOCAL_TWS_OPS(linein) = {
    .name 	= APP_MODE_LINEIN,
    .local_audio_open = linein_local_start,
    .get_play_status = linein_get_status,
};

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

static int get_linein_play_status(void)
{
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
#if (LEA_BIG_FIX_ROLE == 1)
    if (get_le_audio_curr_role()) {
        if (__this->last_run_local_audio_close) {
            __this->last_run_local_audio_close = 0;
            if (__this->linein_local_audio_resume_onoff) {
                __this->linein_local_audio_resume_onoff = 0;
                return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
            } else {
                return LOCAL_AUDIO_PLAYER_STATUS_STOP;
            }
        }
    }
#elif (LEA_BIG_FIX_ROLE == 2)
    //固定为接收端时，打开广播接收后，如果连接上了会关闭本地的音频，当关闭广播后，需要恢复本地的音频播放
    if (get_le_audio_curr_role()) {
        if (__this->last_run_local_audio_close) {
            return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
        } else {
            return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
        }
    }
#else
    if (get_le_audio_curr_role() == 2) {
        //如果是作为接收端
        if (__this->last_run_local_audio_close) {
            if (__this->onoff_as_broadcast_receive == 1) {
                return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
            } else {
                return LOCAL_AUDIO_PLAYER_STATUS_STOP;
            }
            __this->last_run_local_audio_close = 0;
        }
    }
#endif
#endif

    if (__this->onoff) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
}

static int linein_local_audio_open(void)
{
    if (1) {//(get_linein_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
        int err = linein_start();
        if (!err) {
            log_error("linein_start fail!!!");
        }
    }
    return 0;
}

static int linein_local_audio_close(void)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (get_broadcast_role() == 2) {
        //作为接收端的情况
        if (linein_player_runing()) {
            linein_stop();
            __this->onoff_as_broadcast_receive = 1;
        } else {
            __this->onoff_as_broadcast_receive = 0;
        }
        __this->last_run_local_audio_close = 1;
        return 0;
    }
#if (LEA_BIG_FIX_ROLE==1)
    //固定为发送端
    if (get_broadcast_role()) {
        if (linein_player_runing()) {
            linein_stop();
            __this->linein_local_audio_resume_onoff = 1;
        } else {
            __this->linein_local_audio_resume_onoff = 0;
        }
        __this->last_run_local_audio_close = 1;
        return 0;
    }
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (get_auracast_role() == 2) {
        //作为接收端的情况
        if (linein_player_runing()) {
            linein_stop();
            __this->onoff_as_broadcast_receive = 1;
        } else {
            __this->onoff_as_broadcast_receive = 0;
        }
        __this->last_run_local_audio_close = 1;
        return 0;
    }
#if (LEA_BIG_FIX_ROLE==1)
    //固定为发送端
    if (get_auracast_role()) {
        if (linein_player_runing()) {
            linein_stop();
            __this->linein_local_audio_resume_onoff = 1;
        } else {
            __this->linein_local_audio_resume_onoff = 0;
        }
        __this->last_run_local_audio_close = 1;
        return 0;
    }
#endif
#endif

    //作为非接收端的情况
    if (get_linein_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //关闭本地播放
        linein_stop();
    }
    return 0;
}

static void *linein_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;

    if (1) {//(get_linein_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        err = le_audio_linein_recorder_open((void *)&params->fmt, le_audio, params->latency);
        if (err != 0) {
            ASSERT(0, "recorder open fail");
        }
#if LEA_LOCAL_SYNC_PLAY_EN
        err = le_audio_player_open(le_audio, params);
        if (err != 0) {
            ASSERT(0, "player open fail");
        }
#endif
        __this->audio_state = APP_AUDIO_STATE_MUSIC;
        __this->volume = app_audio_get_volume(__this->audio_state);
        __this->onoff = 1;

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        update_app_auracast_deal_scene(LE_AUDIO_MUSIC_START);
#endif
    }

    return le_audio;
}

static int linein_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_linein_recorder_close();
    le_audio_stream_free(le_audio);

    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
    __this->onoff = 0;

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    update_app_auracast_deal_scene(LE_AUDIO_MUSIC_STOP);
#endif

    return 0;
}

static int linein_rx_le_audio_open(void *rx_audio, void *args)
{
    int err;
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //打开广播音频播放
    struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
    rx_audio_hdl->le_audio = le_audio_stream_create(params->conn, &params->fmt);
    rx_audio_hdl->rx_stream = le_audio_stream_rx_open(rx_audio_hdl->le_audio, params->fmt.coding_type);
    err = le_audio_player_open(rx_audio_hdl->le_audio, params);
    if (err != 0) {
        ASSERT(0, "player open fail");
    }

    return 0;
}

static int linein_rx_le_audio_close(void *rx_audio)
{
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
    le_audio_player_close(rx_audio_hdl->le_audio);
    le_audio_stream_rx_close(rx_audio_hdl->rx_stream);
    le_audio_stream_free(rx_audio_hdl->le_audio);

    return 0;
}

static int le_audio_linein_volume_pp(void)
{
    int ret = 0;

    if (__this->onoff == 0) {
        __this->onoff = 1;
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_START);
#if (LEA_BIG_FIX_ROLE==1)
        //固定为发送端，播歌中开广播点击pp键关广播(广播下最终的状态为暂停状态)，本地音频为暂停状态
        if (__this->linein_local_audio_resume_onoff == 0) {
            __this->linein_local_audio_resume_onoff = 1;
        }
#endif
    } else {
        __this->onoff = 0;
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
#if (LEA_BIG_FIX_ROLE==1)
        //固定为发送端，播歌中开广播点击pp键关广播(广播下最终的状态为暂停状态)，本地音频为暂停状态
        if (__this->linein_local_audio_resume_onoff) {
            __this->linein_local_audio_resume_onoff = 0;
        }
#endif
    }

    return ret;
}

const struct le_audio_mode_ops le_audio_linein_ops = {
    .local_audio_open = linein_local_audio_open,
    .local_audio_close = linein_local_audio_close,
    .tx_le_audio_open = linein_tx_le_audio_open,
    .tx_le_audio_close = linein_tx_le_audio_close,
    .rx_le_audio_open = linein_rx_le_audio_open,
    .rx_le_audio_close = linein_rx_le_audio_close,
    .play_status = get_linein_play_status,
};

#endif
#endif
