#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".kws_voice_event_deal.data.bss")
#pragma data_seg(".kws_voice_event_deal.data")
#pragma const_seg(".kws_voice_event_deal.text.const")
#pragma code_seg(".kws_voice_event_deal.text")
#endif
#include "event.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "asr/kws_event.h"
#include "key_driver.h"
#include "app_msg.h"
#include "bt_key_func.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#define LOG_TAG             "[KWS_VOICE_EVENT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if ((defined TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE) && (TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE))

struct jl_kws_event_hdl {
    u32 last_event;
    u32 last_event_jiffies;
};

static struct jl_kws_event_hdl kws_hdl = {
    .last_event = 0,
    .last_event_jiffies = 0,
};

#define __this 		(&kws_hdl)


/* ---------------------------------------------------------------------------- */
/**
 * @brief: 关键词唤醒语音事件处理流程
 *
 * @param event: 系统事件
 *
 * @return : true: 处理该事件; false: 不处理该事件, 由
 */
/* ---------------------------------------------------------------------------- */
int jl_kws_voice_event_handle(int event)
{
    u32 cur_jiffies = jiffies;
    u8 a2dp_state;
    u8 call_state;
    u32 voice_event = event;

    log_info("%s: event: %d", __func__, voice_event);

    if (voice_event == __this->last_event) {
        if (jiffies_to_msecs(cur_jiffies - __this->last_event_jiffies) < 1000) {
            log_info("voice event %d same, ignore", voice_event);
            __this->last_event_jiffies = cur_jiffies;
            return true;
        }
    }
    __this->last_event_jiffies = cur_jiffies;
    __this->last_event = voice_event;

    switch (voice_event) {
    case KWS_EVENT_HEY_KEYWORD:
    case KWS_EVENT_XIAOJIE:
        //主唤醒词:
        log_info("send SIRI cmd");
        bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
        break;

    case KWS_EVENT_XIAODU:
        //主唤醒词:
        log_info("send SIRI cmd");
        bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
        break;

    case KWS_EVENT_PLAY_MUSIC:
    case KWS_EVENT_STOP_MUSIC:
    case KWS_EVENT_PAUSE_MUSIC:
        call_state = bt_get_call_status();
        if ((call_state == BT_CALL_OUTGOING) ||
            (call_state == BT_CALL_ALERT)) {
            //bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        } else if (call_state == BT_CALL_INCOMING) {
            //bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        } else if (call_state == BT_CALL_ACTIVE) {
            //bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        } else {
            a2dp_state = bt_a2dp_get_status();
            if (a2dp_state == BT_MUSIC_STATUS_STARTING) {
                if (voice_event == KWS_EVENT_PAUSE_MUSIC) {
                    log_info("send PAUSE cmd");
                    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
                } else if (voice_event == KWS_EVENT_STOP_MUSIC) {
                    log_info("send STOP cmd");
                    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_STOP, 0, NULL);
                }
            } else {
                if (voice_event == KWS_EVENT_PLAY_MUSIC) {
                    log_info("send PLAY cmd");
                    bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
                }
            }
        }
        break;

    case KWS_EVENT_VOLUME_UP:
        log_info("volume up");
        bt_key_vol_up();
        break;

    case KWS_EVENT_VOLUME_DOWN:
        log_info("volume down");
        bt_key_vol_down();
        break;

    case KWS_EVENT_PREV_SONG:
        log_info("Send PREV cmd");
        bt_key_music_prev();
        break;

    case KWS_EVENT_NEXT_SONG:
        log_info("Send NEXT cmd");
        bt_key_music_next();
        break;

    case KWS_EVENT_CALL_ACTIVE:
        if (bt_get_call_status() == BT_CALL_INCOMING) {
            log_info("Send ANSWER cmd");
            bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        }
        break;

    case KWS_EVENT_CALL_HANGUP:
        log_info("Send HANG UP cmd");
        if ((bt_get_call_status() >= BT_CALL_INCOMING) && (bt_get_call_status() <= BT_CALL_ALERT)) {
            bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        break;
#if TCFG_AUDIO_ANC_ENABLE
    case KWS_EVENT_ANC_ON:
        anc_mode_switch(ANC_ON, 1);
        break;
    case KWS_EVENT_TRANSARENT_ON:
        anc_mode_switch(ANC_TRANSPARENCY, 1);
        break;
    case KWS_EVENT_ANC_OFF:
        anc_mode_switch(ANC_OFF, 1);
        break;
#endif

    case KWS_EVENT_NULL:
        log_info("KWS_EVENT_NULL");
        break;

    default:
        break;
    }

    return true;
}

static int kws_voice_event_msg_handler(int *msg)
{
    int type = msg[0];

    switch (type) {
    case APP_MSG_SMART_VOICE_EVENT:
        int event = msg[1];
        jl_kws_voice_event_handle(event);
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(kws_voice_event_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = kws_voice_event_msg_handler,
};

#endif

