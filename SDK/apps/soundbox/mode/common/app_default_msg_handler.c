#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_default_msg_handler.data.bss")
#pragma data_seg(".app_default_msg_handler.data")
#pragma const_seg(".app_default_msg_handler.text.const")
#pragma code_seg(".app_default_msg_handler.text")
#endif
#include "app_main.h"
#include "init.h"
#include "app_config.h"
#include "app_default_msg_handler.h"
#include "dev_status.h"
#include "alarm.h"
#include "idle.h"
#include "audio_config.h"
#include "app_tone.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "idle.h"
#include "soundbox.h"
#include "node_param_update.h"
#include "bass_treble.h"
#include "usb/device/usb_stack.h"
#include "asm/charge.h"
#include "record.h"
#include "mix_record_api.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"
#include "le_audio_player.h"
#include "rcsp_device_status.h"
#include "btstack_rcsp_user.h"
#include "bt_key_func.h"

static u32 input_number = 0;
static u16 input_number_timer = 0;
static void input_number_timeout(void *p)
{
#if TCFG_APP_MUSIC_EN
    input_number_timer = 0;
    printf("input_number = %d\n", input_number);
    if (app_in_mode(APP_MODE_MUSIC)) {
        app_send_message(APP_MSG_MUSIC_PLAY_BY_NUM, (int)input_number);
    }
    input_number = 0;
#endif
}

static void input_number_deal(u32 num)
{
#if TCFG_APP_MUSIC_EN
    input_number = input_number * 10 + num;
    if (input_number > 9999) {
        input_number = num;
    }
    printf("num = %d, input_number = %d, input_number_timer = %d\n", num, input_number, input_number_timer);
    if (input_number_timer == 0) {
        input_number_timer = sys_timeout_add(NULL, input_number_timeout, 1000);
    } else {
        sys_timer_modify(input_number_timer, 1000);
    }
    app_send_message(APP_MSG_INPUT_FILE_NUM, input_number);
#endif
}
static u8 sys_audio_mute_statu = 0;//记录 audio dac mute

u8 get_sys_aduio_mute_statu(void)
{
    return 	app_audio_get_dac_digital_mute();
}
void app_common_key_msg_handler(int *msg)
{


    int from_tws = msg[1];

    switch (msg[0]) {
    case APP_MSG_BT_WORK_MODE_CHANGE:
        if (msg[1] == APP_KEY_MSG_FROM_TWS) { //非后台不响应来自tws的切换模式消息
            return;
        }
        bt_work_mode_switch_to_next();
        break;
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


        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
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
        app_send_message(APP_MSG_VOL_CHANGED, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        break;
#if TCFG_KBOX_1T3_MODE_EN
#if TCFG_MIC_EFFECT_ENABLE && (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN || LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    case APP_MSG_SW_WIRED_MIC_OR_WIRELESS_MIC:
        //有线Mic和无线Mic切换
        if (app_get_current_mode()->name == APP_MODE_LINEIN) {
            break;
        }
        if (app_get_current_mode()->name == APP_MODE_FM) {
            //如果是FM模式，则只支持开关混响Mic，不支持打开cis
            r_printf(">> FM Mode not support open Cis!\n");
            if (!mic_effect_player_runing()) {
                //开混响，卸载FM第二段代码, 节省ram资源
                mic_effect_player_open();
            } else {
                //关混响，加载FM第二段代码, 提高FM性能
                mic_effect_player_close();
            }
            break;
        }
        if (!mic_effect_player_runing()) {
            y_printf("----- Close Wireless Mic, Open Wired Mic\n");
            //关闭 connect，打开mic_effect

            if (app_get_current_mode()->name != APP_MODE_BT) {
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
                app_connected_close_in_other_mode();
#elif (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
                app_broadcast_close_in_other_mode();
#endif
            } else {
                //蓝牙模式下关闭cig
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
                app_connected_close_all(APP_CONNECTED_STATUS_STOP);
#elif (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
                app_broadcast_close(APP_BROADCAST_STATUS_STOP);
#endif
            }
            os_time_dly(5);
            mic_effect_player_open();
        } else {
            //关闭混响, 打开 connect
            mic_effect_player_close();
            os_time_dly(5);
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
            app_connected_open_in_other_mode();
#elif (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
            app_broadcast_open_in_other_mode();
#endif
            y_printf("+++++ Open Wireless Mic, Close Wired Mic!\n");
        }
        break;
#endif

    case APP_MSG_WIRELESS_MIC_OPEN:
        //r_printf("wir mic open\n");
        le_audio_working_status_switch(1);
        break;

    case APP_MSG_WIRELESS_MIC_CLOSE:
        //r_printf("wir mic close\n");
        le_audio_working_status_switch(0);
        break;
    case APP_MSG_WIRELESS_MIC0_VOL_UP:
        le_audio_dvol_up(0);
        y_printf("[0] Up!");
        break;
    case APP_MSG_WIRELESS_MIC0_VOL_DOWN:
        y_printf("[0] Down!");
        le_audio_dvol_down(0);
        break;
    case APP_MSG_WIRELESS_MIC1_VOL_UP:
        y_printf("[1] Up!");
        le_audio_dvol_up(1);
        break;
    case APP_MSG_WIRELESS_MIC1_VOL_DOWN:
        y_printf("[1] Down!");
        le_audio_dvol_down(1);
        break;
#endif

    case APP_MSG_SWITCH_SOUND_EFFECT:
        effect_scene_switch();
        break;
#if TCFG_MIC_EFFECT_ENABLE
    case APP_MSG_MIC_EFFECT_ON_OFF://混响开关

        if (from_tws == APP_KEY_MSG_FROM_TWS) {
            break;
        }

#if TCFG_APP_BT_EN
        if (bt_get_call_status() != BT_CALL_HANGUP) {//通话中
            break;
        }
#endif
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        } else {
            mic_effect_player_open();
        }
        break;
    case APP_MSG_SWITCH_MIC_EFFECT://混响音效场景切换
        mic_effect_scene_switch();
        break;
    case APP_MSG_MIC_VOL_UP:
        mic_effect_dvol_up();
        break;
    case APP_MSG_MIC_VOL_DOWN:
        mic_effect_dvol_down();
        break;
#if TCFG_BASS_TREBLE_NODE_ENABLE
    case APP_MSG_MIC_BASS_UP:
        puts("\n APP_MSG_MIC_BASS_UP \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_LOW, 20);
        break;
    case APP_MSG_MIC_BASS_DOWN:
        puts("\n APP_MSG_MIC_BASS_DOWN \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_LOW, -20);
        break;
    case APP_MSG_MIC_TREBLE_UP:
        puts("\n APP_MSG_MIC_TREBLE_UP \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_HIGH, 20);
        break;
    case APP_MSG_MIC_TREBLE_DOWN:
        puts("\n APP_MSG_MIC_TREBLE_DOWN \n");
        mic_bass_treble_eq_udpate("BassTreEff", BASS_TREBLE_HIGH, -20);
        break;
#endif // TCFG_BASS_TREBLE_NODE_ENABLE
#endif
    case APP_MSG_VOCAL_REMOVE:
#if TCFG_APP_BT_EN
        if (bt_get_call_status() != BT_CALL_HANGUP) {//通话中
            break;
        }
#endif
        if (from_tws == APP_KEY_MSG_FROM_TWS) {
            break;
        }

        music_vocal_remover_switch();
        break;
    case APP_MSG_MUSIC_CHANGE_EQ:
        //  for test eq切换接口,工具上需要配置多份EQ配置
        u8 music_eq_preset_index = get_music_eq_preset_index();
        music_eq_preset_index ^= 1;
        set_music_eq_preset_index(music_eq_preset_index);
        eq_update_parm(get_current_scene(), "Eq0Media", music_eq_preset_index);
        app_send_message(APP_MSG_EQ_CHANGED, get_music_eq_preset_index() + 1); //显示EQ从1开始
        printf("APP_MSG_MUSIC_CHANGE_EQ:%d\n", music_eq_preset_index);
        break;

    case APP_MSG_SYS_MUTE:
        sys_audio_mute_statu = app_audio_get_dac_digital_mute() ^ 1;
        if (sys_audio_mute_statu) {
            app_audio_mute(AUDIO_MUTE_DEFAULT);
        } else {
            app_audio_mute(AUDIO_UNMUTE_DEFAULT);
        }
        app_send_message(APP_MSG_MUTE_CHANGED, sys_audio_mute_statu);
        break;
    case APP_MSG_REC_PP:
#if TCFG_MIX_RECORD_ENABLE
        if (app_get_current_mode()->name == APP_MODE_MUSIC) {
            r_printf("[Error]Music Mode no support record!\n");
            break;
        } else if (app_get_current_mode()->name == APP_MODE_PC) {
            r_printf("[Error]PC Mode no support record!\n");
            break;
        }

        printf("\n APP_MSG_REC_PP \n");
        if (get_mix_recorder_status()) {
            mix_recorder_stop();
        } else {
            if (dev_manager_get_phy_logo(dev_manager_find_active(0))) {
                mix_recorder_start();
            }
        }
#endif // TCFG_MIX_RECORD_ENABLE
        break;
    case APP_MSG_IR_NUM0:
    case APP_MSG_IR_NUM1:
    case APP_MSG_IR_NUM2:
    case APP_MSG_IR_NUM3:
    case APP_MSG_IR_NUM4:
    case APP_MSG_IR_NUM5:
    case APP_MSG_IR_NUM6:
    case APP_MSG_IR_NUM7:
    case APP_MSG_IR_NUM8:
    case APP_MSG_IR_NUM9:
        input_number_deal(msg[0]  - APP_MSG_IR_NUM0);
        break;

    case APP_MSG_LE_BROADCAST_SW:
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        g_printf("APP_MSG_LE_AURACAST_SW");
        app_auracast_switch();
#elif ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN ) && (!TCFG_KBOX_1T3_MODE_EN))
        g_printf("APP_MSG_LE_BROADCAST_SW");
        app_broadcast_switch();
#endif
        break;

    case APP_MSG_LE_CONNECTED_SW:
#if ((LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) && (!TCFG_KBOX_1T3_MODE_EN))
        app_connected_switch();
#endif
        break;

    case APP_MSG_LE_AUDIO_ENTER_PAIR:
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        app_broadcast_enter_pair(BROADCAST_ROLE_UNKNOW, 0);
#endif
        break;

    case APP_MSG_LE_AUDIO_EXIT_PAIR:
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        app_broadcast_exit_pair(BROADCAST_ROLE_UNKNOW);
#endif
        break;


    case APP_MSG_LE_AURACAST_SW_DEIVCE:
        printf("---------%s %d--------", __FUNCTION__, __LINE__);
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN))
        app_auracast_sink_switch_source_device(0);
#endif
        break;

    }
}

void app_common_device_event_handler(int *msg)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch (msg[0]) {
    case DEVICE_EVENT_FROM_OTG:
        usb_msg = (const char *)msg[2];
        if (usb_msg[0] == 's') {
#if TCFG_USB_SLAVE_ENABLE
            ret = pc_device_event_handler(msg);
            if (ret == 1) {
                if (true != app_in_mode(APP_MODE_PC)) {
                    app = APP_MODE_PC;
                }
            } else if (ret == 2) {
                app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
            }
#endif
            break;
        } else if (usb_msg[0] == 'h') {
            ///是主机, 统一于SD卡等响应主机处理，这里不break
        } else {
            break;
        }
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
    case DEVICE_EVENT_FROM_USB_HOST:
#if TCFG_APP_MUSIC_EN
        if (true == app_in_mode(APP_MODE_MUSIC)) {
            music_device_msg_handler(msg);
        }
        ret = dev_status_event_filter(msg);///解码设备上下线， 设备挂载等处理
        if (ret == true) {
            if (msg[1] == DEVICE_EVENT_IN) {
                ///设备上线， 非解码模式切换到解码模式播放
                if (true != app_in_mode(APP_MODE_MUSIC)) {
                    app = APP_MODE_MUSIC;
                }
            }
        }
#endif
#if TCFG_APP_RECORD_EN
        if (true == app_in_mode(APP_MODE_RECORD)) {
            record_device_msg_handler(msg);
        }
#endif
#if TCFG_MIX_RECORD_ENABLE
        mix_record_device_msg_handler(msg);
#endif // TCFG_MIX_RECORD_ENABLE
        break;
    case DEVICE_EVENT_FROM_LINEIN:
#if TCFG_APP_LINEIN_EN
        if (msg[1] == DEVICE_EVENT_IN) {
            printf("LINEIN ONLINE");
            if (true != app_in_mode(APP_MODE_LINEIN)) {
                app = APP_MODE_LINEIN;
            }
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            printf("LINEIN OFFLINE");
            if (true == app_in_mode(APP_MODE_LINEIN)) {
                app_send_message(APP_MSG_CHANGE_MODE, 0);
            }
        }
#endif
        break;
    case DEVICE_EVENT_FROM_ALM:
#if TCFG_APP_RTC_EN
        if (msg[1] == DEVICE_EVENT_IN) {
            printf("ALM ONLINE");
            alarm_flag = 1;
            alarm_update_info_after_isr();
            if (true != app_in_mode(APP_MODE_RTC)) {
                app = APP_MODE_RTC;
            } else {
                alarm_ring_start();
            }
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            alarm_flag = 0;
            printf("ALM OFFLINE");
        }
#endif
        break;

    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        break;
    }


#if RCSP_MODE && RCSP_DEVICE_STATUS_ENABLE
    rcsp_update_dev_state(msg[0], NULL);
#endif


    if (app != 0xff) {
        /*一些情况不希望退出蓝牙模式*/
#if TCFG_APP_BT_EN
        if (bt_app_exit_check() == 0) {
            puts("bt_background can not enter\n");
            return;
        }
#endif
        //PC 不响应因为设备上线引发的模式切换
        if ((true != app_in_mode(APP_MODE_PC)) || alarm_flag) {
            //闹钟响起直接切到rtc模式
            if (alarm_flag) {
                app_send_message2(APP_MSG_GOTO_MODE, app, 0);
                return;
            }

#if (TCFG_CHARGE_ENABLE && (!TCFG_CHARGE_POWERON_ENABLE))
            if (get_charge_online_flag() && app != APP_MODE_PC) {
                return;
            }
#endif

#if TCFG_APP_BT_EN && TWFG_APP_POWERON_IGNORE_DEV   //蓝牙模式同时打开设备过滤才会生效，否则会导致关闭蓝牙模式有设备在线开机无法切换到对应模式
            int msec = jiffies_msec2offset(app_var.start_time, jiffies_msec());
            if (msec > TWFG_APP_POWERON_IGNORE_DEV || app_in_mode(APP_MODE_IDLE)) //idle模式下不过滤，防止开机设备检测需要时间，又没打开蓝牙模式会跳转到Idle，此时不能过滤设备上线消息
#endif
            {
#if 0//defined(TCFG_BROADCAST_ENABLE) && (TCFG_BROADCAST_ENABLE)
                if (get_broadcast_role() != BROADCAST_ROLE_RECEIVER) {
                    app_send_message2(APP_MSG_GOTO_MODE, app, 0);
                }
#else
                app_send_message2(APP_MSG_GOTO_MODE, app, 0);
#endif
            }
        }
    }
}

static void app_common_app_event_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_KEY_POWER_OFF:
    case APP_MSG_KEY_POWER_OFF_HOLD:
        if (msg[1] == APP_KEY_MSG_FROM_TWS) { //来自tws的按键关机消息不响应
            break;
        }
        power_off_deal(APP_MSG_KEY_POWER_OFF);
        break;
    case APP_MSG_KEY_POWER_OFF_RELEASE:
        goto_poweroff_first_flag = 0;
        break;
    case APP_MSG_KEY_POWER_OFF_INSTANTLY:
        power_off_instantly();
        break;
    default:
        break;
    }
}

void app_default_msg_handler(int *msg)
{
    const struct app_msg_handler *handler;
    struct app_mode *mode;

    mode = app_get_current_mode();
    //消息继续分发
#if (TCFG_BT_BACKGROUND_ENABLE)
    if (!bt_background_msg_forward_filter(msg))
#endif
    {
        for_each_app_msg_handler(handler) {
            if (handler->from != msg[0]) {
                continue;
            }
            if (mode && mode->name == handler->owner) {
                continue;
            }

            /*蓝牙后台情况下，消息仅转发给后台处理*/
            handler->handler(msg + 1);
        }
    }

    switch (msg[0]) {
    case MSG_FROM_DEVICE:
        app_common_device_event_handler(msg + 1);
        break;
    case MSG_FROM_APP:
        app_common_app_event_handler(msg + 1);
    default:
        break;
    }
}
