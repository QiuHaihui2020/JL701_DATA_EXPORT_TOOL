#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_key_func.data.bss")
#pragma data_seg(".bt_key_func.data")
#pragma const_seg(".bt_key_func.text.const")
#pragma code_seg(".bt_key_func.text")
#endif
#include "btstack/avctp_user.h"
#include "key_driver.h"
#include "audio_manager.h"
#include "vol_sync.h"
#include "app_main.h"
#include "audio_config.h"
#include "bt_key_func.h"
#include "ui/ui_api.h"
#include "ui_manage.h"

#include "a2dp_player.h"
#include "bt_event_func.h"
#include "app_tone.h"
#include "a2dp_player.h"


/*************************************************************************************************/
/*!
 *  \brief      音量加
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		加音量
 */
/*************************************************************************************************/
static void volume_up(void)
{
    u8 test_box_vol_up = 0x41;
    s8 cur_vol = 0;
    u8 call_status = bt_get_call_status();
    u8 data[6];
    a2dp_player_get_btaddr(data);

    if ((tone_player_runing() || ring_player_runing())) {
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            volume_up_down_direct(1);
        }
        return;
    }

    /*打电话出去彩铃要可以调音量大小*/
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        cur_vol = app_audio_get_volume(APP_AUDIO_STATE_CALL);
    } else {
        cur_vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    }
    if (bt_get_remote_test_flag()) {
        bt_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_up); //音量加
    }

    if (cur_vol >= app_audio_get_max_volume()) {
        audio_event_to_user(AUDIO_EVENT_VOL_MAX);	//触发vol max事件

#if TCFG_MAX_VOL_PROMPT
        play_tone_file(get_tone_files()->max_vol);
#endif
        if (bt_get_call_status() != BT_CALL_HANGUP) {
            /*本地音量最大，如果手机音量还没最大，继续加，以防显示不同步*/
            if (g_bt_hdl.phone_vol < 15) {
                if (bt_get_curr_channel_state() & HID_CH) {
                    bt_cmd_prepare(USER_CTRL_HID_VOL_UP, 0, NULL);
                } else {
                    bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
                }
            }
            return;
        }
#if TCFG_BT_VOL_SYNC_ENABLE
        if (bt_get_call_status() == BT_CALL_HANGUP) {
            opid_play_vol_sync_fun(&app_var.music_volume, 1);
            bt_cmd_prepare_for_addr(data, USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL);
        }
#endif/* TCFG_BT_VOL_SYNC_ENABLE */
        return;
    }

#if TCFG_BT_VOL_SYNC_ENABLE
    if (bt_get_call_status() == BT_CALL_HANGUP) {
        opid_play_vol_sync_fun(&app_var.music_volume, 1);
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
    }
#else
    if (app_audio_get_state() == APP_AUDIO_STATE_IDLE) {
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
    }
    app_audio_volume_up(1);
#endif/*TCFG_BT_VOL_SYNC_ENABLE*/
    printf("vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_UP, 0, NULL);
    } else {
#if TCFG_BT_VOL_SYNC_ENABLE
        bt_cmd_prepare_for_addr(data, USER_CTRL_CMD_SYNC_VOL_INC, 0, NULL); //使用HID调音量
#endif
    }
}

/*************************************************************************************************/
/*!
 *  \brief      音量减
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		减音量
 */
/*************************************************************************************************/
static void volume_down(void)
{
    u8 test_box_vol_down = 0x42;
    u8 data[6];
    a2dp_player_get_btaddr(data);

    if ((tone_player_runing() || ring_player_runing())) {
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            volume_up_down_direct(-1);
        }
        return;
    }
    if (bt_get_remote_test_flag()) {
        bt_cmd_prepare(USER_CTRL_TEST_KEY, 1, &test_box_vol_down); //音量减
    }

    if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) <= 0) {
        audio_event_to_user(AUDIO_EVENT_VOL_MIN);	//触发vol mix事件
        if (bt_get_call_status() != BT_CALL_HANGUP) {
            /*
             *本地音量最小，如果手机音量还没最小，继续减
             *注意：有些手机通话最小音量是1(GREE G0245D)
             */
            if (g_bt_hdl.phone_vol > 1) {
                if (bt_get_curr_channel_state() & HID_CH) {
                    bt_cmd_prepare(USER_CTRL_HID_VOL_DOWN, 0, NULL);
                } else {
                    bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);

                }
            }
            return;
        }
#if TCFG_BT_VOL_SYNC_ENABLE
        if (bt_get_call_status() == BT_CALL_HANGUP) {
            opid_play_vol_sync_fun(&app_var.music_volume, 0);
            bt_cmd_prepare_for_addr(data, USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
        }
#endif
        return;
    }

#if TCFG_BT_VOL_SYNC_ENABLE
    if (bt_get_call_status() == BT_CALL_HANGUP) {
        opid_play_vol_sync_fun(&app_var.music_volume, 0);
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_var.music_volume, 1);
    }
#else
    if (app_audio_get_state() == APP_AUDIO_STATE_IDLE) {
        app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
    }
    app_audio_volume_down(1);
#endif/*TCFG_BT_VOL_SYNC_ENABLE*/
    printf("vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    if (bt_get_call_status() != BT_CALL_HANGUP) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_VOLUME_DOWN, 0, NULL);
    } else {
#if TCFG_BT_VOL_SYNC_ENABLE
        /* opid_play_vol_sync_fun(&app_var.music_volume, 0); */
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == 0) {
            app_audio_volume_down(0);
        }
        bt_cmd_prepare_for_addr(data, USER_CTRL_CMD_SYNC_VOL_DEC, 0, NULL);
#endif
    }
}
/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 pp 按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_music_pp(void)
{
    if ((bt_get_call_status() == BT_CALL_OUTGOING) ||
        (bt_get_call_status() == BT_CALL_ALERT)) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
    } else if (bt_get_call_status() == BT_CALL_INCOMING) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
    } else if (bt_get_call_status() == BT_CALL_ACTIVE) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
    } else {
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 prev 按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		播放音乐上一曲
 */
/*************************************************************************************************/
void bt_key_music_prev(void)
{
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 next 按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		播放音乐下一曲
 */
/*************************************************************************************************/
void bt_key_music_next(void)
{
    if (bt_get_call_status() == BT_CALL_INCOMING) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    }
    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 vol up 按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		加音量
 */
/*************************************************************************************************/
void bt_key_vol_up(void)
{
    u8 vol;
    u8 call_status;
    if (bt_get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_up();
    call_status = bt_get_call_status();
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        vol = app_audio_get_volume(APP_AUDIO_STATE_CALL);
    } else {
        vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    }
    printf("music_vol:vol=%d, state:%d", vol, app_audio_get_state());
    app_send_message(APP_MSG_VOL_CHANGED, vol);

}

void bt_key_rcsp_vol_up(void)
{
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    u8 vol;
    u8 call_status;
    if (bt_get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_up();
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 vol down 按键处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note		减音量
 */
/*************************************************************************************************/
void bt_key_vol_down(void)
{
    u8 vol;
    u8 call_status;
    if (bt_get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_down();
    call_status = bt_get_call_status();
    if ((call_status == BT_CALL_ACTIVE) || (call_status == BT_CALL_OUTGOING)) {
        vol = app_audio_get_volume(APP_AUDIO_STATE_CALL);
    } else {
        vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    }
    printf("music_vol:vol=%d, state:%d", vol, app_audio_get_state());
    app_send_message(APP_MSG_VOL_CHANGED, vol);
}

void bt_key_rcsp_vol_down(void)
{
#if (THIRD_PARTY_PROTOCOLS_SEL & (RCSP_MODE_EN))
    u8 vol;
    u8 call_status;
    if (bt_get_call_status() == BT_CALL_ACTIVE && bt_sco_state() == 0) {
        return;
    }
    volume_down();
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 回拨最后一个号码  来电拒听
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_last_on(void)
{
    if (bt_get_call_status() == BT_CALL_INCOMING) {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        return;
    }

    if ((bt_get_call_status() == BT_CALL_ACTIVE) ||
        (bt_get_call_status() == BT_CALL_OUTGOING) ||
        (bt_get_call_status() == BT_CALL_ALERT) ||
        (bt_get_call_status() == BT_CALL_INCOMING)) {
        return;//通话过程不允许回拨
    }

    if (g_bt_hdl.last_call_type ==  BT_STATUS_PHONE_INCOME) {
        bt_cmd_prepare(USER_CTRL_HFP_DIAL_NUMBER, g_bt_hdl.income_phone_len,
                       g_bt_hdl.income_phone_num);
    } else {
        bt_cmd_prepare(USER_CTRL_HFP_CALL_LAST_NO, 0, NULL);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 通话挂断
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_hang_up(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 siri开启
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_siri(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 hid 发起拍照命令
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_hid_control(void)
{
    /* log_info("bt_get_curr_channel_state:%x\n", bt_get_curr_channel_state()); */
    if (bt_get_curr_channel_state() & HID_CH) {
        printf("KEY_HID_CONTROL\n");
        bt_cmd_prepare(USER_CTRL_HID_IOS, 0, NULL);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 三方通话 挂断当前去听另一个（未接听或者在保留状态都可以）
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_three_way_answer1(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_THREE_WAY_ANSWER1, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 三方通话 保留当前去接听, 或者用于两个通话的切换
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_three_way_answer2(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_THREE_WAY_ANSWER2, 0, NULL);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙模式 通话声卡切换
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_key_call_switch(void)
{
    if (bt_get_call_status() == BT_CALL_ACTIVE) {
        bt_cmd_prepare(USER_CTRL_SCO_LINK, 0, NULL);
    }
}

