#include "system/includes.h"
#include "app_config.h"
#include "app_task.h"
#include "media/includes.h"
#include "tone_player.h"
#include "app_tone.h"
#include "app_main.h"
#include "ui_manage.h"
#include "vm.h"
#include "ui/ui_api.h"
#include "scene_switch.h"
#include "audio_config_def.h"
#include "audio_config.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"


#if TCFG_APP_IIS_EN
#include "iis_player.h"

#define LOG_TAG             "[APP_IIS]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct iis_opr {
    s16 volume ;
    u8 onoff;
    u8 audio_state; /*判断iis模式使用模拟音量还是数字音量*/
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2))
    //如果固定为接收端，则会存在的问题：打开广播下从其它模式切进mic模式，关闭广播mic不会自动打开的问题
    u8 iis_broadcast_local_need_open_flag;	//当此flag置1时，需要打开mic
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1))
    //固定为发送端
    //暂停中开广播再关闭：暂停。暂停中开广播点击pp后关闭：播放。播歌开广播点击pp后关闭广播：暂停. 该变量为1时表示关闭广播时需要本地音频需要是播放状态
    u8 iis_local_audio_resume_onoff;
#endif
};
static struct iis_opr iis_hdl = {0};
#define __this 	(&iis_hdl)

static int le_audio_iis_volume_pp(void);

// 打开iis数据流操作
int iis_start(void)
{
    if (__this->onoff == 1) {
        printf("iis is aleady start\n");
        return true;
    }
    iis_player_open();
    __this->audio_state = APP_AUDIO_STATE_MUSIC;
    __this->volume = app_audio_get_volume(__this->audio_state);
    __this->onoff = 1;
    return true;
}


// stop iis 数据流
void iis_stop(void)
{
    if (__this->onoff == 0) {
        printf("iis is aleady stop\n");
        return;
    }
    iis_player_close();

    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
    __this->onoff = 0;
}


/*-------------------------------------------------------------------*/
/**@brief    iis 数据流 start stop 播放暂停切换
  @param    无
  @return   无
  @note
 */
/*-------------------------------------------------------------------*/
int iis_volume_pp(void)
{
    int ret = 0;

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#if (LEA_BIG_FIX_ROLE==2)
    //固定为接收端, pp按键mute操作
    u8 iis_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
    if (get_broadcast_role() == 2) {
        //接收端已连上
        iis_volume_mute_mark ^= 1;
        audio_app_mute_en(iis_volume_mute_mark);
        return 0;
    } else {
        if (iis_volume_mute_mark == 1) {
            //没有连接情况下，如果之前是mute住了，那么先解mute
            iis_volume_mute_mark ^= 1;
            audio_app_mute_en(iis_volume_mute_mark);
            return 0;
        }
        if (__this->onoff) {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#else
    if (get_broadcast_role()) {
        ret = le_audio_iis_volume_pp();
    } else {
        if (__this->onoff) {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
        } else {
            update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
        }
    }
#endif
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if ((app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) ||
        (app_get_connected_role() == APP_CONNECTED_ROLE_DUPLEX)) {
        ret = le_audio_iis_volume_pp();
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
            iis_stop();
        } else {
            iis_start();
        }
    }

    printf("pp:%d \n", __this->onoff);
    /* UI_REFLASH_WINDOW(true); */
    return  __this->onoff;
}

/*-------------------------------------------------------------------*/
/**@brief   获取 iis  播放状态
  @param    无
  @return   1:当前正在打开 0：当前正在关闭
  @note
 */
/*-------------------------------------------------------------------*/
u8 iis_get_status(void)
{
    return __this->onoff;
}

/*-------------------------------------------------------------------*/
/*@brief   iis 音量设置函数
  @param    需要设置的音量
  @return
  @note    在iis 情况下针对了某些情景进行了处理，设置音量需要使用独立接口
*/
/*-------------------------------------------------------------------*/
int iis_volume_set(s16 vol)
{
    app_audio_set_volume(__this->audio_state, vol, 1);
    printf("iis vol: %d", __this->volume);
    __this->volume = vol;

#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif
    return true;
}

void iis_key_vol_up(void)
{
    s16 vol;
    if (__this->volume < app_audio_volume_max_query(AppVol_IIS)) {
        __this->volume ++;
        iis_volume_set(__this->volume);
    } else {
        iis_volume_set(__this->volume);
        if (tone_player_runing() == 0) {
            /* tone_play(TONE_MAX_VOL); */
#if TCFG_MAX_VOL_PROMPT
            play_tone_file(get_tone_files()->max_vol);
#endif
        }
    }
    vol = __this->volume;
    app_send_message(APP_MSG_VOL_CHANGED, vol);
    printf("vol+:%d\n", __this->volume);
}


void iis_key_vol_down(void)
{
    s16 vol;
    if (__this->volume) {
        __this->volume --;
        iis_volume_set(__this->volume);
    }
    vol = __this->volume;
    app_send_message(APP_MSG_VOL_CHANGED, vol);
    printf("vol-:%d\n", __this->volume);
}

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)

//当固定为接收端时，其它模式下开广播切进mic模式，关闭广播后music模式不会自动播放
void iis_set_broadcast_local_open_flag(u8 en)
{
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2))
    __this->iis_broadcast_local_need_open_flag = en;
#endif
}

static int get_iis_play_status(void)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (get_broadcast_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1))
    if (get_broadcast_role()) {
        if (__this->iis_local_audio_resume_onoff) {
            return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
        } else {
            return LOCAL_AUDIO_PLAYER_STATUS_STOP;
        }
    }
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2))
    if (__this->onoff || __this->iis_broadcast_local_need_open_flag) {
#else
    if (__this->onoff) {
#endif
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (get_connected_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }

    if (__this->onoff) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#endif
}

static int iis_local_audio_open(void)
{
    if (1) {//(get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2))
        if (__this->iis_broadcast_local_need_open_flag) {
            __this->iis_broadcast_local_need_open_flag = 0;
        }
#endif
        int err = iis_start();
        if (!err) {
            log_error("iis_start fail!!!");
        }
    }
    return 0;
}

static int iis_local_audio_close(void)
{
    /* if (get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) { */
    if (iis_player_runing()) {
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==2))
        __this->iis_broadcast_local_need_open_flag = 1;
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1))
        __this->iis_local_audio_resume_onoff = 1;	//关闭广播需要恢复本地音频播放
#endif
        //关闭本地播放
        iis_stop();
    } else {
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==1))
        __this->iis_local_audio_resume_onoff = 0;	//关闭广播不用恢复本地音频播放
#endif
    }
    return 0;
}

static void *iis_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;

    if (1) {//(get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        err = le_audio_iis_recorder_open((void *)&params->fmt, le_audio, params->latency);
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
    }

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
#endif

    return le_audio;
}

static int iis_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_iis_recorder_close();
    le_audio_stream_free(le_audio);

    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1);
    __this->onoff = 0;

    return 0;
}

static int iis_rx_le_audio_open(void *rx_audio, void *args)
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

static int iis_rx_le_audio_close(void *rx_audio)
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

static int le_audio_iis_volume_pp(void)
{
    int ret = 0;

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (__this->onoff == 0) {
        __this->onoff = 1;
        ret = app_broadcast_deal(LE_AUDIO_MUSIC_START);
#if (LEA_BIG_FIX_ROLE == 1)
        if (__this->iis_local_audio_resume_onoff == 0) {
            __this->iis_local_audio_resume_onoff = 1;
        }
#endif
    } else {
        __this->onoff = 0;
        ret = app_broadcast_deal(LE_AUDIO_MUSIC_STOP);
#if (LEA_BIG_FIX_ROLE == 1)
        if (__this->iis_local_audio_resume_onoff == 1) {
            __this->iis_local_audio_resume_onoff = 0;
        }
#endif
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (__this->onoff == 0) {
        __this->onoff = 1;
        ret = app_connected_deal(LE_AUDIO_MUSIC_START);

    } else {
        __this->onoff = 0;
        ret = app_connected_deal(LE_AUDIO_MUSIC_STOP);
    }
#endif

    return ret;
}

const struct le_audio_mode_ops le_audio_iis_ops = {
    .local_audio_open = iis_local_audio_open,
    .local_audio_close = iis_local_audio_close,
    .tx_le_audio_open = iis_tx_le_audio_open,
    .tx_le_audio_close = iis_tx_le_audio_close,
    .rx_le_audio_open = iis_rx_le_audio_open,
    .rx_le_audio_close = iis_rx_le_audio_close,
    .play_status = get_iis_play_status,
};

#endif
#endif

