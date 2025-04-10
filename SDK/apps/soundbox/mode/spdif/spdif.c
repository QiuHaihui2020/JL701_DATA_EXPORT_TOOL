#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".spdif.data.bss")
#pragma data_seg(".spdif.data")
#pragma const_seg(".spdif.text.const")
#pragma code_seg(".spdif.text")
#endif
#include "app_main.h"
#if TCFG_APP_SPDIF_EN
#include "system/includes.h"
#include "app_action.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "spdif.h"
#include "app_tone.h"
#include "spdif_file.h"
#include "spdif_player.h"
#include "audio_dai/audio_spdif_slave.h"
#include "hdmi_cec_api.h"
#include "gpio_config.h"
#include "audio_config.h"
#include "media/includes.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "local_tws.h"
#include "rcsp_spdif_func.h"
#include "bt_key_func.h"
#include "btstack_rcsp_user.h"

struct spdif_ctl {
    struct spdif_file_cfg *p_spdif_cfg;	//spdif的配置参数信息
    void *spdif_hdl;
    u8 mute_mark;
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
#if (LEA_BIG_FIX_ROLE==0)
    //下面两个变量配合着使用, 解决问题：当关闭广播，打开本地音频时，是否需要mute（固定广播发送端, mute的状态下打开广播，关闭广播时恢复mute状态）
    u8 close_broadcast_open_lacal_audio_need_mute;
    u8 spdif_broadcast_pp_sw_flag;	//spdif broadcast 按下pp 切换发送端和接收端的标志
#endif
#if (LEA_BIG_FIX_ROLE==1)
    //当关闭广播，打开本地音频时，是否需要mute（固定广播发送端, mute的状态下打开广播，关闭广播时恢复mute状态）
    u8 close_broadcast_open_lacal_audio_need_mute;
    //当关闭广播，打开本地音频时，是否需要解mute（固定广播发送端, unmute的状态下打开广播，关闭广播时需要解mute）
    u8 close_broadcast_open_lacal_audio_need_unmute;
#endif
#if (LEA_BIG_FIX_ROLE==2)
    //当关闭广播，打开本地音频时，是否需要mute（固定广播为接收端, mute的状态下打开广播，关闭广播时恢复mute状态）
    u8 close_broadcast_open_lacal_audio_need_mute;
    //当关闭广播，打开本地音频时，是否需要解mute（固定广播为接收端, unmute的状态下打开广播，关闭广播时需要解mute）
    u8 close_broadcast_open_lacal_audio_need_unmute;
#endif
    struct le_audio_stream_params params;
    void *le_audio;
#endif
};
struct spdif_ctl app_spdif_hd;
static u8 spdif_idle_flag = 1;

/* 模式提示音播放回调 */
static int spdif_tone_play_end_callback(void *priv, enum stream_event event)
{
    if (false == app_in_mode(APP_MODE_SPDIF)) {
        return 0;
    }
    switch (event) {
    /* case STREAM_EVENT_NONE: */
    case STREAM_EVENT_STOP:
        app_send_message(APP_MSG_SPDIF_START, 0);
        break;
    default:
        break;
    }
    return 0;
}
int spdif_app_msg_handler(int *msg)
{
    if (false == app_in_mode(APP_MODE_SPDIF)) {
        return 0;
    }
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_MUSIC_PP:
        puts("APP_MSG_MUSIC_PP\n");
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        app_spdif_hd.mute_mark ^= 1;
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==0)
        if (!get_broadcast_role()) {
            audio_app_mute_en(app_spdif_hd.mute_mark);
        } else {
            if (get_broadcast_role() == 2) {
                //如果此时是接收端, 则需要转为发送端
                app_spdif_hd.mute_mark = 0;
                app_spdif_hd.spdif_broadcast_pp_sw_flag = 1;
            } else {
                //如果此时是发送端, 则需要转为接收端
                app_spdif_hd.mute_mark = 1;
                app_spdif_hd.spdif_broadcast_pp_sw_flag = 1;
            }
        }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==0)
        if (!get_auracast_role()) {
            audio_app_mute_en(app_spdif_hd.mute_mark);
        } else {
            if (get_auracast_role() == 2) {
                //如果此时是接收端, 则需要转为发送端
                app_spdif_hd.mute_mark = 0;
                app_spdif_hd.spdif_broadcast_pp_sw_flag = 1;
            } else {
                //如果此时是发送端, 则需要转为接收端
                app_spdif_hd.mute_mark = 1;
                app_spdif_hd.spdif_broadcast_pp_sw_flag = 1;
            }
        }
#else
        audio_app_mute_en(app_spdif_hd.mute_mark);
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        if (get_broadcast_role()) {
            le_audio_spdif_volume_pp();
        }
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        if (get_auracast_role()) {
            le_audio_spdif_volume_pp();
        }
#endif
        break;
    case APP_MSG_MUSIC_NEXT:
        break;
    case APP_MSG_MUSIC_PREV:
        break;
    case APP_MSG_SPDIF_START:
        if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }

        if ((uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff) && app_spdif_hd.p_spdif_cfg->hdmi_det_mode == HDMI_DET_UNUSED) {
            hdmi_cec_init(uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port), 0);
        }
        app_spdif_hd.spdif_hdl = spdif_init();
        spdif_start();
        break;
    case APP_MSG_SPDIF_SWITCH_SOURCE:
        spdif_io_loop_switch();
        printf("spdif switch source");
        break;
    case APP_MSG_SPDIF_SET_SOURCE:
        spdif_set_port_by_index(msg[1]);
        app_send_message(APP_MSG_SPDIF_SOURCE_UPDATE, 0);
        //printf("spdif set source %d\n",msg[1]);
        break;
    case APP_MSG_CEC_VOL_UP:
        if (uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[0]) != get_spdif_source_io() &&
            uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[1]) != get_spdif_source_io()) {
            //当前播放源不是HDMI ARC
            break;
        }
    /* fall-through */
    case APP_MSG_VOL_UP:
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
        if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
            bt_key_rcsp_vol_up();
        } else {
            app_audio_volume_up(1);
        }
#else
        app_audio_volume_up(1);
#endif
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume()) {
            if (tone_player_runing() == 0) {
#if TCFG_MAX_VOL_PROMPT
                play_tone_file(get_tone_files()->max_vol);
#endif
            }
        }
        if (uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff) {
            if (uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[0]) == get_spdif_source_io() ||
                uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[1]) == get_spdif_source_io()) {
                //当前播放源为HDMI ARC
                hdmi_cec_send_volume(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
            }
        }
        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
    case APP_MSG_CEC_VOL_DOWN:
        if (uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[0]) != get_spdif_source_io() &&
            uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[1]) != get_spdif_source_io()) {
            //当前播放源不是HDMI ARC
            break;
        }
    /* fall-through */
    case APP_MSG_VOL_DOWN:
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
        if (bt_rcsp_device_conn_num() && JL_rcsp_get_auth_flag() && (app_get_current_mode()->name != APP_MODE_BT)) {
            bt_key_rcsp_vol_down();
        } else {
            app_audio_volume_down(1);
        }
#else
        app_audio_volume_down(1);
#endif
        if (uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff) {
            if (uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[0]) == get_spdif_source_io() ||
                uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[1]) == get_spdif_source_io()) {
                //当前播放源为HDMI ARC

                hdmi_cec_send_volume(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
            }
        }
        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
    case APP_MSG_PITCH_UP:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg spdif pitch up\n");
        if (spdif_player_runing()) {
            app_var.pitch_mode = spdif_file_pitch_up(); //返回当前变调模式
        }
#endif
        break;
    case APP_MSG_PITCH_DOWN:
#if TCFG_PITCH_SPEED_NODE_ENABLE
        printf("app msg spdif pitch down\n");
        if (spdif_player_runing()) {
            app_var.pitch_mode = spdif_file_pitch_down();
        }
#endif
        break;
    case APP_MSG_CEC_MUTE:
        if (uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[0]) == get_spdif_source_io() ||
            uuid2gpio(app_spdif_hd.p_spdif_cfg->hdmi_port[1]) == get_spdif_source_io()) {
            //当前播放源为HDMI ARC
            u8 mute_ctl = msg[1];
            printf("\n CEC_MUTE CTL %d \n", mute_ctl);
            if (mute_ctl > 1) {
                if (get_sys_aduio_mute_statu()) {
                    app_audio_mute(AUDIO_UNMUTE_DEFAULT);
                } else {
                    app_audio_mute(AUDIO_MUTE_DEFAULT);
                }
                hdmi_cec_send_volume(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
            } else {
                if (mute_ctl) {
                    app_audio_mute(AUDIO_MUTE_DEFAULT);
                } else {
                    app_audio_mute(AUDIO_UNMUTE_DEFAULT);
                }
            }

            app_send_message(APP_MSG_MUTE_CHANGED, get_sys_aduio_mute_statu());
        }
        break;
    case APP_MSG_SYS_MUTE:
        u8 sys_audio_mute_statu = app_audio_get_dac_digital_mute() ^ 1;
        if (sys_audio_mute_statu) {
            app_audio_mute(AUDIO_MUTE_DEFAULT);
        } else {
            app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        }

        update_spdif_player_mute_state();
        app_send_message(APP_MSG_MUTE_CHANGED, sys_audio_mute_statu);
        break;
    default:
        app_common_key_msg_handler(msg);
        break;
    }

#if RCSP_MODE
    rcsp_spdif_msg_deal(msg[0]);
#endif

    return 0;
}
static int app_spdif_init()
{
    puts("\nspdif start\n");
    app_spdif_hd.mute_mark = 0;
    app_spdif_hd.spdif_hdl = NULL;
    spdif_idle_flag = 0;
    //开启ui
    /* UI_SHOW_WINDOW(ID_WINDOW_TV);//打开ui主页 */
    /* UI_SHOW_MENU(MENU_TV, 0, 0, NULL); */
    tone_player_stop();
    int ret = play_tone_file_callback(get_tone_files()->spdif_mode, NULL, spdif_tone_play_end_callback);
    if (ret) {
        spdif_tone_play_end_callback(NULL, STREAM_EVENT_NONE); // 提示音播放失败就直接调用 linein start
    }
#if TCFG_PITCH_SPEED_NODE_ENABLE
    app_var.pitch_mode = PITCH_0;
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    btstack_init_in_other_mode();
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_SPDIF);
    app_spdif_hd.p_spdif_cfg = audio_spdif_file_get_cfg();
    app_send_message(APP_MSG_SPDIF_SOURCE_UPDATE, 0);
    app_send_message(APP_MSG_SPDIF_STATUS_UPDATE, 0);
    return 0;
}

void app_spdif_exit()
{
    if (app_spdif_hd.spdif_hdl && app_spdif_hd.p_spdif_cfg->hdmi_det_mode == HDMI_DET_UNUSED \
        && (uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff)) {
        hdmi_cec_close();
    }

    if (spdif_player_runing()) {
        spdif_player_close();
    } else {
        spdif_stop();
        spdif_release(app_spdif_hd.spdif_hdl);
    }
    if (app_spdif_hd.mute_mark) {
        audio_app_mute_en(0);
    }
    app_spdif_hd.spdif_hdl = NULL;
    spdif_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_SPDIF);
}

struct app_mode *app_enter_spdif_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_spdif_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), spdif_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            spdif_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_spdif_exit();

    return next_mode;
}

static int spdif_mode_try_enter()
{
    /* return app_spdif_init(); */
    return 0;
}

static int spdif_mode_try_exit()
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

    /* app_spdif_exit(); */
    return 0;
}

static const struct app_mode_ops spdif_mode_ops = {
    .try_enter          = spdif_mode_try_enter,
    .try_exit           = spdif_mode_try_exit,
};

/*
 * 注册spdif模式
 */
REGISTER_APP_MODE(spdif_mode) = {
    .name 	= APP_MODE_SPDIF,
    .index  = APP_MODE_SPDIF_INDEX,
    .ops 	= &spdif_mode_ops,
};


static u8 spdif_idle_query(void)
{
    return spdif_idle_flag;
}

REGISTER_LP_TARGET(spdif_lp_target) = {
    .name = "spdif",
    .is_idle = spdif_idle_query,
};

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
static int get_spdif_play_status(void)
{
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
    return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
}

static int spdif_local_audio_open(void)
{
    if (1) {//(get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
        if ((uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff) && app_spdif_hd.p_spdif_cfg->hdmi_det_mode == HDMI_DET_UNUSED) {
            hdmi_cec_init(uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port), 0);
        }
        app_spdif_hd.spdif_hdl = spdif_init();
        int err = spdif_start();
        if (err) {
            log_e("spdif_start fail!!!");
        }
    }
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==0)
    if (get_broadcast_role() == 0) {
        if (app_spdif_hd.spdif_broadcast_pp_sw_flag) {
            app_spdif_hd.spdif_broadcast_pp_sw_flag = 0;
        } else {
            if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute == 1) {
                //需要恢复mute状态
                app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
                app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
                if (app_spdif_hd.mute_mark == 0) {
                    app_spdif_hd.mute_mark ^= 1;
                    audio_app_mute_en(app_spdif_hd.mute_mark);
                }
            }
        }
    }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==0)
    if (get_auracast_role() == 0) {
        if (app_spdif_hd.spdif_broadcast_pp_sw_flag) {
            app_spdif_hd.spdif_broadcast_pp_sw_flag = 0;
        } else {
            if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute == 1) {
                //需要恢复mute状态
                app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
                app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
                if (app_spdif_hd.mute_mark == 0) {
                    app_spdif_hd.mute_mark ^= 1;
                    audio_app_mute_en(app_spdif_hd.mute_mark);
                }
            }
        }
    }
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==1)
    if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute == 1) {
        //需要恢复mute状态
        app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark == 0) {
            app_spdif_hd.mute_mark ^= 1;
            audio_app_mute_en(app_spdif_hd.mute_mark);
        }
    }
    if (app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute == 1) {
        //打开广播前是unmute状态，关闭广播时需要解mute
        app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 0;
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark == 1) {
            app_spdif_hd.mute_mark ^= 1;
            audio_app_mute_en(app_spdif_hd.mute_mark);
        }
    }
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==2)
    if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute == 1) {
        //需要恢复mute状态
        app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark == 0) {
            app_spdif_hd.mute_mark ^= 1;
            audio_app_mute_en(app_spdif_hd.mute_mark);
        }
    }
    if (app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute == 1) {
        //打开广播前是unmute状态，关闭广播时需要解mute
        app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 0;
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark == 1) {
            app_spdif_hd.mute_mark ^= 1;
            audio_app_mute_en(app_spdif_hd.mute_mark);
        }
    }
#endif
    return 0;
}


static int spdif_local_audio_close(void)
{
    if (get_spdif_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==0)
        //没有固定发送端或者是固定接收端
        if (get_broadcast_role()) {
            app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
            if (app_spdif_hd.mute_mark) {
                //此时是mute状态，需要解mute
                app_spdif_hd.mute_mark ^= 1;
                app_audio_set_mute_state(APP_AUDIO_STATE_MUSIC, app_spdif_hd.mute_mark);
                app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 1;	//关闭广播时需要恢复mute状态
            }
        }
#endif
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==0)
        //没有固定发送端或者是固定接收端
        if (get_auracast_role()) {
            app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
            if (app_spdif_hd.mute_mark) {
                //此时是mute状态，需要解mute
                app_spdif_hd.mute_mark ^= 1;
                app_audio_set_mute_state(APP_AUDIO_STATE_MUSIC, app_spdif_hd.mute_mark);
                app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 1;	//关闭广播时需要恢复mute状态
            }
        }
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==1)
        //如果固定为发送端，则可能会存在两种情况：
        //1、mute住的情况下打开广播,此时发送端mute住，但接收端有声音, 发送端需要解mute. 关闭广播时需要重新给mute住
        //2、unmute的情况下打开广播，在关闭广播时需要解mute
        //打开广播(发送端)会先关闭本地音频，再打开发送，可以在关闭本地音频时先解mute，记录下此时音频状态，在广播结束打开本地音频时恢复状态
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark) {
            //此时是mute状态，需要解mute
            app_spdif_hd.mute_mark ^= 1;
            audio_app_mute_en(app_spdif_hd.mute_mark);
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 1;	//关闭广播时需要恢复mute状态
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 0;
        } else {
            //此时是unmute的状态，需要记录下来，等关闭广播的时候恢复unmute状态
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 1;	//关闭广播时需要恢复mute状态
        }
#endif
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==2)
        //如果固定为接收端，则可能会存在两种情况：
        //1、mute住的情况下打开广播，此时进入接收端也是mute的状态，需要解mute；当关闭广播接收的时候，需要mute回去
        //2、unmute的情况下打开广播，在关闭广播时需要解mute
        app_spdif_hd.mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (app_spdif_hd.mute_mark) {
            //此时是mute状态，需要解mute
            app_spdif_hd.mute_mark ^= 1;
            app_audio_set_mute_state(APP_AUDIO_STATE_MUSIC, 0);
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 1;	//关闭广播时需要恢复mute状态
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 0;
        } else {
            //此时是unmute的状态，需要记录下来，等关闭广播的时候恢复unmute状态
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 1;	//关闭广播时需要恢复mute状态
        }
#endif

        if (spdif_player_runing()) {
            //关闭本地播放
            spdif_player_close();
        } else { //没有开数据流的时候也可能开了spdif的硬件
            spdif_stop();
            spdif_release(app_spdif_hd.spdif_hdl);
        }
    }

    return 0;
}

static void *spdif_tx_le_audio_open(void *args)
{
    int err;
    void *le_audio = NULL;
    if (1) {//(get_iis_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        if ((uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port) != 0xff) && app_spdif_hd.p_spdif_cfg->hdmi_det_mode == HDMI_DET_UNUSED) {
            hdmi_cec_init(uuid2gpio(app_spdif_hd.p_spdif_cfg->cec_io_port), 0);
        }
        app_spdif_hd.spdif_hdl = spdif_init();
        spdif_start();
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);
        struct le_audio_stream_format *le_audio_fmt = &params->fmt;
        memcpy(&app_spdif_hd.params, params, sizeof(struct le_audio_stream_params));
        app_spdif_hd.le_audio = le_audio;
    }
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    //修复spdif模式下反复开关广播后，在广播下按下pp键，本地mute住但接收端依旧出声的问题
    update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_START);
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    //修复spdif模式下反复开关广播后，在广播下按下pp键，本地mute住但接收端依旧出声的问题
    update_app_auracast_deal_scene(LE_AUDIO_MUSIC_START);
#endif
    return le_audio;
}

static int spdif_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }
    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_spdif_recorder_close();
    le_audio_stream_free(le_audio);
    app_spdif_hd.le_audio = NULL;
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    update_app_broadcast_deal_scene(LE_AUDIO_MUSIC_STOP);
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    update_app_auracast_deal_scene(LE_AUDIO_MUSIC_STOP);
#endif
    /* app_audio_set_volume(APP_AUDIO_STATE_MUSIC, __this->volume, 1); */
    return 0;
}

static int spdif_rx_le_audio_open(void *rx_audio, void *args)
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

static int spdif_rx_le_audio_close(void *rx_audio)
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

static int le_audio_spdif_volume_pp(void)
{
    int ret = 0;
    if (!app_spdif_hd.mute_mark) {
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_START);
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==1)
        if (app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute == 0) {
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 1;
        }
        if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute) {
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 0;
        }
#endif
    } else {
        ret = le_audio_scene_deal(LE_AUDIO_MUSIC_STOP);
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==1)
        if (app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute) {
            app_spdif_hd.close_broadcast_open_lacal_audio_need_unmute = 0;
        }
        if (app_spdif_hd.close_broadcast_open_lacal_audio_need_mute == 0) {
            app_spdif_hd.close_broadcast_open_lacal_audio_need_mute = 1;
        }
#endif
    }

    return ret;
}

const struct le_audio_mode_ops le_audio_spdif_ops = {
    .local_audio_open = spdif_local_audio_open,
    .local_audio_close = spdif_local_audio_close,
    .tx_le_audio_open = spdif_tx_le_audio_open,
    .tx_le_audio_close = spdif_tx_le_audio_close,
    .rx_le_audio_open = spdif_rx_le_audio_open,
    .rx_le_audio_close = spdif_rx_le_audio_close,
    .play_status = get_spdif_play_status,
};



struct le_audio_stream_params *spdif_get_le_audio_params(void)
{
    return &app_spdif_hd.params;
}


void *spdif_get_le_audio_hdl(void)
{
    return app_spdif_hd.le_audio;
}


#endif
#endif

