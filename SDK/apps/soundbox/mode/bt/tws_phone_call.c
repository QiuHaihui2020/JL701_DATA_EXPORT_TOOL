#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tws_phone_call.data.bss")
#pragma data_seg(".tws_phone_call.data")
#pragma const_seg(".tws_phone_call.text.const")
#pragma code_seg(".tws_phone_call.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/a2dp_media_codec.h"
#include "btstack/bluetooth.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "classic/hci_lmp.h"
#include "bt_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "user_cfg.h"
#include "audio_cvp.h"
#include "bt_tws.h"
#include "app_tone.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "soundbox.h"
#include "app_config.h"
#include "app_testbox.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "bt_slience_detect.h"
#include "mic_effect.h"
#include "mix_record_api.h"
#if TCFG_SMART_VOICE_ENABLE
#include "asr/jl_kws.h"
#include "smart_voice/smart_voice.h"
#endif
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
#include "jl_kws/jl_kws_api.h"
#endif
#include "app_le_broadcast.h"
#include "app_le_connected.h"


#if(TCFG_USER_TWS_ENABLE && TCFG_APP_BT_EN)

#define LOG_TAG             "[PHONECALL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"



/*配置通话时前面丢掉的数据包包数*/
#define ESCO_DUMP_PACKET_ADJUST		1	/*配置使能*/
#define ESCO_DUMP_PACKET_DEFAULT	0
#define ESCO_DUMP_PACKET_CALL		60 /*0~0xFF*/

static u8 esco_dump_packet = ESCO_DUMP_PACKET_CALL;

#define  SYNC_TONE_PHONE_RING_TIME    300
void phone_income_num_check(void *priv);
void tws_phone_call_send_cmd(u8 cmd, u8 *btaddr, u8 bt_value, u8 tx_do_action);

enum {
    CMD_OPEN_ESCO_PLAYER = 1,
    CMD_CLOSE_ESCO_PLAYER,
    CMD_PHONE_INCOME,
    CMD_PHONE_OUT,
    CMD_PHONE_HANGUP,
    CMD_PHONE_ACTIVE,
    CMD_PHONE_LASET_CALL_TYPE,
    CMD_PHONE_SIRI,
};

static u8 g_play_addr[6];

void tone_ring_player_stop()
{
    tone_player_stop();
    ring_player_stop();
}

u8 phone_ring_play_start(void)
{
    log_info("%s,%d\n", __FUNCTION__, g_bt_hdl.inband_ringtone);

    /* if (bt_get_call_status() == BT_CALL_HANGUP) { */
    /* log_info("hangup,--phone ring play return\n"); */
    /* return 0; */
    /* } */
    /* check if support inband ringtone */
    if (BT_CALL_INCOMING != bt_get_call_status_for_addr(g_bt_hdl.phone_ring_addr)) {
        return 0;
    }
    if (!g_bt_hdl.inband_ringtone) {
        tws_play_ring_file_alone(get_tone_files()->phone_in, SYNC_TONE_PHONE_RING_TIME);
        return 1;
    }
    return 0;
}
void phone_check_inband_ring_play_timer(void *priv)
{
#if TCFG_BT_INBAND_RING
    void *device = btstack_get_conn_device(g_bt_hdl.phone_ring_addr);
    if (g_bt_hdl.inband_ringtone && g_bt_hdl.phone_ring_flag && device && BT_CALL_INCOMING == btstack_bt_get_call_status(device)) {
        if (bt_check_esco_state_via_addr(g_bt_hdl.phone_ring_addr) == BT_ESCO_STATUS_CLOSE) {
            r_printf("phone_check_inband_ring_play_timer play");
            g_bt_hdl.inband_ringtone = 0;
            phone_ring_play_start();
        }
    }
#endif
}
extern u8 bt_stack_get_incoming_call_num();

void phone_second_call_ring_play_start(void *priv)
{
//音频设计只有在ESCO工作的时候，播嘟嘟声才能叠加
//两种情况，一是先有嘟嘟声，然后来了ESCO铃声，要先关嘟嘟再开。
//二是两个都是ESCO铃声，第二个打断了第一个之后，第一个的铃声变成嘟嘟叠加。指示有两个来电
    log_info("%s\n", __FUNCTION__);

    u8 *other_addr = NULL;
    if (priv) {
        other_addr = btstack_get_other_dev_addr(priv) ;
        put_buf(priv, 6);
        printf("bt_get_call_status_for_addr=%d,%d\n", bt_get_call_status_for_addr(priv), bt_get_call_status_for_addr(other_addr));
    }
    if (bt_stack_get_incoming_call_num() > 1 || (priv && bt_get_call_status_for_addr(priv) == BT_CALL_INCOMING && bt_get_call_status_for_addr(other_addr) != BT_CALL_OUTGOING)) {
        tws_play_ring_file_alone(get_tone_files()->phone_in, SYNC_TONE_PHONE_RING_TIME);
    }
}

int bt_phone_income(u8 after_conn, u8 *bt_addr)
{

    le_audio_scene_deal(LE_AUDIO_PHONE_START);

#if TCFG_BT_INBAND_RING
#else
    lmp_private_esco_suspend_resume(3);
#endif
    tone_ring_player_stop();

    g_bt_hdl.phone_ring_sync_tws = 1;
    memcpy(g_bt_hdl.phone_ring_addr, bt_addr, 6);
    put_buf(g_bt_hdl.phone_ring_addr, 6);

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        g_bt_hdl.phone_ring_flag = 1;
        g_bt_hdl.phone_income_flag = 1;
        puts("bt_phone_income sync call\n");
    } else {
        void *device = btstack_get_conn_device(bt_addr);
        if (!device || BT_CALL_HANGUP == btstack_bt_get_call_status(device)) {
            puts("bt_phone_income err 1\n");
            return 1;
        }
#if TCFG_BT_INBAND_RING
        g_bt_hdl.inband_ringtone = btstack_get_inband_ringtone_flag_for_addr(bt_addr);
#else
        g_bt_hdl.inband_ringtone = 0 ;
#endif

        printf("inband_ringtone=0x%x\n", g_bt_hdl.inband_ringtone);
        g_bt_hdl.phone_ring_flag = 1;
        g_bt_hdl.phone_income_flag = 1;

        if (bt_stack_get_incoming_call_num() > 1) {
            //第一次来电被置上一次。第二次再来就有值了
            sys_timeout_add(g_bt_hdl.phone_ring_addr, phone_second_call_ring_play_start, 2000);
        } else {
            if (g_bt_hdl.inband_ringtone) {
                if (after_conn == 2) { //强制播本地铃声
                    sys_timeout_add(g_bt_hdl.phone_ring_addr, phone_second_call_ring_play_start, 2000);
                }
            } else {
#if TCFG_BT_PHONE_NUMBER_ENABLE
                if (after_conn) {
                    phone_ring_play_start();
                } else {
                    phone_income_num_check(NULL);
                }
#else
                phone_ring_play_start();
#endif
            }
        }


    }

    return 0;
}

int bt_phone_hangup(u8 *bt_addr)
{
    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    log_info("phone_handup\n");
    if (g_bt_hdl.phone_ring_flag) {
        g_bt_hdl.phone_ring_flag = 0;
        tone_ring_player_stop();
        if (g_bt_hdl.phone_timer_id) {
            sys_timeout_del(g_bt_hdl.phone_timer_id);
            g_bt_hdl.phone_timer_id = 0;
        }
    }
    g_bt_hdl.phone_income_flag = 0;
    g_bt_hdl.phone_num_flag = 0;
    g_bt_hdl.phone_ring_sync_tws = 0;
    lmp_private_esco_suspend_resume(4);

    /*
     * 挂断的时候会清除了一些标识并且会stop了提示音
     * 判断如果另一个手机还在来电并且不支持inband ring，那就恢复一个嘟嘟声提示音
     */
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        u8 temp_btaddr[6];
        u8 *addr = btstack_get_other_dev_addr(bt_addr);
        if (addr && bt_get_call_status_for_addr(bt_addr) != BT_CALL_OUTGOING) {
            //有另一个连接存在
            u8 call_status = bt_get_call_status_for_addr(addr);
            u8 inband_ring_flag = btstack_get_inband_ringtone_flag_for_addr(addr);
            printf("other connect call sta:%d-%d-%d\n", call_status,
                   inband_ring_flag, bt_check_esco_state_via_addr(addr));
            if (call_status == BT_CALL_INCOMING) {
                memcpy(g_bt_hdl.phone_ring_addr, addr, 6);
                g_bt_hdl.phone_ring_sync_tws = 1;
                if ((inband_ring_flag == 0) || (inband_ring_flag && bt_check_esco_state_via_addr(g_bt_hdl.phone_ring_addr) == BT_ESCO_STATUS_CLOSE)) {
                    tws_phone_call_send_cmd(CMD_PHONE_INCOME, g_bt_hdl.phone_ring_addr, 2, 1);

                }
            }
        }
    }
    le_audio_scene_deal(LE_AUDIO_PHONE_STOP);
    return 0;

}
int bt_phone_out(u8 *bt_addr)
{
    le_audio_scene_deal(LE_AUDIO_PHONE_START);

    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    g_bt_hdl.phone_income_flag = 0;
    return 0;

}
static int esco_audio_open(u8 *bt_addr)
{
    esco_player_open(bt_addr);
#if TCFG_TWS_POWER_BALANCE_ENABLE && TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        log_info("tws_master open esco recoder\n");
        esco_recoder_open(COMMON_SCO, bt_addr);
    } else {
        log_info("tws_slave don't open esco recoder\n");
    }
#else
    esco_recoder_open(COMMON_SCO, bt_addr);
#endif
    return 0;
}

int bt_phone_active(u8 *bt_addr)
{
    if (g_bt_hdl.phone_call_dec_begin) {
        log_info("call_active,dump_packet clear\n");
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    }
    if (g_bt_hdl.phone_ring_flag) {
        g_bt_hdl.phone_ring_flag = 0;
        tone_ring_player_stop();
        if (g_bt_hdl.phone_timer_id) {
            sys_timeout_del(g_bt_hdl.phone_timer_id);
            g_bt_hdl.phone_timer_id = 0;
        }
    }
    lmp_private_esco_suspend_resume(4);
    g_bt_hdl.phone_income_flag = 0;
    g_bt_hdl.phone_num_flag = 0;
    g_bt_hdl.phone_ring_sync_tws = 0;
    /* g_bt_hdl.phone_con_sync_ring = 0; */
    g_bt_hdl.phone_vol = 15;
    bt_tws_sync_volume();
    r_printf("phone_active:%d\n", app_var.call_volume);
    app_audio_set_volume(APP_AUDIO_STATE_CALL, app_var.call_volume, 1);
    return 0;

}

#define ESCO_SIRI_WAKEUP()      (app_var.siri_stu == 1 || app_var.siri_stu == 2)
static void esco_smart_voice_detect_handler(void)
{
#if TCFG_SMART_VOICE_ENABLE
#if TCFG_CALL_KWS_SWITCH_ENABLE
    if (ESCO_SIRI_WAKEUP() || (bt_get_call_status() != BT_CALL_INCOMING)) {
        audio_smart_voice_detect_close();
    }
#else
    audio_smart_voice_detect_close();
#endif
#endif
}

int bt_phone_esco_play(u8 *bt_addr)
{
    le_audio_scene_deal(LE_AUDIO_PHONE_START);

    puts("<<<<<<<<<<<esco_dec_stat\n");
    mem_stats();
    u8 temp_btaddr[6];
    if (esco_player_get_btaddr(temp_btaddr)) {
        //已经有设备在用了，不能重复初始化
        printf("CMD_OPEN_ESCO_PLAYER error\n");
        return 1;
    }
#if TCFG_SMART_VOICE_ENABLE
    esco_smart_voice_detect_handler();
#endif

    a2dp_player_close(bt_addr);
    bt_stop_a2dp_slience_detect(bt_addr);
    a2dp_media_close(bt_addr);
#if 0   //debug
    void mic_capless_feedback_toggle(u8 toggle);
    mic_capless_feedback_toggle(0);
#endif
#if TCFG_BT_INBAND_RING
    tone_ring_player_stop();
#else
    if (BT_CALL_INCOMING == bt_get_call_status_for_addr(bt_addr)) {
        lmp_private_esco_suspend_resume(3);
    } else {
        tone_ring_player_stop();
    }
#endif
    bt_api_esco_status(bt_addr, BT_ESCO_STATUS_OPEN);
    esco_audio_open(bt_addr);

    g_bt_hdl.phone_call_dec_begin = 1;
    if (bt_get_call_status() == BT_CALL_ACTIVE) {
        log_info("dec_begin,dump_packet clear\n");
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    }

    tws_page_scan_deal_by_esco(1);
    pbg_user_mic_fixed_deal(1);
    return 0;

}

int bt_phone_esco_stop(u8 *bt_addr)
{
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    /* 处理来电时挂断电话，先跑释放资源再收到handup命令的情况
     * 避免先开smart voice，再关闭"yes/no"，导致出错*/
    jl_kws_speech_recognition_close();
#endif /*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/


    if (!esco_player_is_playing(bt_addr)) {
        puts("esco_player_is_close\n");
        return 0;
    }
    puts("<<<<<<<<<<<esco_dec_stop\n");
    g_bt_hdl.phone_call_dec_begin = 0;
    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    bt_api_esco_status(bt_addr, BT_ESCO_STATUS_CLOSE);
    esco_recoder_close();
    esco_player_close();

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_open(JL_KWS_COMMAND_KEYWORD);
#endif

    if (app_var.goto_poweroff_flag) {
        return 0;
    }
    tws_page_scan_deal_by_esco(0);
    pbg_user_mic_fixed_deal(0);
    le_audio_scene_deal(LE_AUDIO_PHONE_STOP);

    return 0;

}
int bt_phone_last_call_type(u8 *bt_addr, u8 bt_value)
{
    g_bt_hdl.last_call_type = bt_value;
    return 0;
}
int bt_phone_siri(u8 *bt_addr, u8 bt_value)
{
    if (bt_value) {
        le_audio_scene_deal(LE_AUDIO_PHONE_START);
    }

    app_var.siri_stu = bt_value;
    esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;

    if (!bt_value) {
        le_audio_scene_deal(LE_AUDIO_PHONE_STOP);
    }

    return 0;
}

static void tws_esco_play_in_task(u8 *data)
{
    u8 *bt_addr = data + 2;
    u8 bt_value = data[8] ;


    r_printf("tws_esco_play_in_task=%d\n", data[0]);
    switch (data[0]) {
    case CMD_OPEN_ESCO_PLAYER:
#if (TCFG_BT_ESCO_PLAYER_ENABLE == 0)
        lmp_private_esco_suspend_resume(1);
        break;
#endif
#if TCFG_MIC_EFFECT_ENABLE
        mic_effect_player_pause(1);
#endif

        /*tws通话，仅主机出声*/
#if ((defined TCFG_TWS_ESCO_MODE) && (TCFG_TWS_ESCO_MODE == TWS_ESCO_ONLY_MASTER))
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            printf("tws role slave,esco player disable\n");
            lmp_private_esco_suspend_resume(1);
            break;
        }
#endif
        bt_phone_esco_play(bt_addr);
        break;
    case CMD_CLOSE_ESCO_PLAYER:
        bt_phone_esco_stop(bt_addr);
#if TCFG_MIC_EFFECT_ENABLE
        mic_effect_player_pause(0);
#endif
        break;
    case CMD_PHONE_HANGUP:
        bt_phone_hangup(bt_addr);
        break;
    case CMD_PHONE_ACTIVE:
        bt_phone_active(bt_addr);
        break;
    case CMD_PHONE_INCOME:
        bt_phone_income(bt_value, bt_addr);
        break;
    case CMD_PHONE_OUT:
        bt_phone_out(bt_addr);
        break;

    case CMD_PHONE_LASET_CALL_TYPE:
        bt_phone_last_call_type(bt_addr, bt_value);
        break;
    case CMD_PHONE_SIRI:
        bt_phone_siri(bt_addr, bt_value);
        break;
    }
    if (data[1] != 2) {
        free(data);
    }
}

static void tws_esco_play_callback(u8 *data)
{
    int msg[4];

    u8 *buf = malloc(9);
    if (!buf) {
        return;
    }
    memcpy(buf, data, 9);

    msg[0] = (int)tws_esco_play_in_task;
    msg[1] = 1;
    msg[2] = (int)buf;

    os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}

static void tws_esco_player_data_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    if (data[1] == 0 && !rx) {
        return;
    }
    tws_esco_play_callback(data);
}
REGISTER_TWS_FUNC_STUB(tws_esco_player_stub) = {
    .func_id = 0x183C0528,
    .func = tws_esco_player_data_in_irq,
};

void tws_phone_call_send_cmd(u8 cmd, u8 *btaddr, u8 bt_value, u8 tx_do_action)
{
    u8 data[9];
    data[0] = cmd;
    data[1] = tx_do_action;
    memcpy(data + 2, btaddr, 6);
    data[8] = bt_value;
    int err = tws_api_send_data_to_sibling(data, 9, 0x183C0528);
    if (err) {
        data[1] = 2;
        tws_esco_play_in_task(data);
    }
}

void phone_sync_vol(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_CALL_SET_VOLUME, 1, &g_bt_hdl.phone_vol);
}

#if ESCO_DUMP_PACKET_ADJUST
u8 get_esco_packet_dump(void)
{
    //log_info("esco_dump_packet:%d\n", esco_dump_packet);
    return esco_dump_packet;
}
#endif


static u8 check_phone_income_idle(void)
{
    if (g_bt_hdl.phone_ring_flag) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(phone_incom_lp_target) = {
    .name       = "phone_check",
    .is_idle    = check_phone_income_idle,
};


static void bt_tws_phone_num_callback(int priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        if (tws_api_get_role() == TWS_ROLE_MASTER &&
            g_bt_hdl.phone_ring_flag && g_bt_hdl.inband_ringtone == 0) {
            tws_play_ring_file_alone(get_tone_files()->phone_in, SYNC_TONE_PHONE_RING_TIME);
        }
    }
}
REGISTER_TWS_TONE_CALLBACK(phone_num_stub) = {
    .func_uuid  = 0x763A8D10,
    .callback   = bt_tws_phone_num_callback,
};

static int bt_phone_num_to_file_list(const char *file_list[], int max_file_num)
{
    int i;
    char *num = (char *)(g_bt_hdl.income_phone_num);

    printf("phone_num: %s\n", num);

    for (i = 0; i < max_file_num && num[i] != '\0'; i++) {
        file_list[i] = get_tone_files()->num[num[i] - '0'];
    }
    return i;
}

void phone_income_num_check(void *priv)
{
    const char *file_list[20];
    g_bt_hdl.phone_timer_id = 0;

    if (g_bt_hdl.phone_num_flag) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (g_bt_hdl.phone_ring_flag) {
                tone_ring_player_stop();
                int file_num = bt_phone_num_to_file_list(file_list, ARRAY_SIZE(file_list));
                tws_play_tone_files_alone_callback(file_list, file_num, 300, 0x763A8D10);
            }
        }
    } else {
        /*电话号码还没有获取到，定时查询*/
        g_bt_hdl.phone_timer_id = sys_timeout_add(NULL, phone_income_num_check, 200);
    }
}

bool is_siri_open()
{
    if (app_var.siri_stu == 1 || app_var.siri_stu == 2) {
        return 1;
    }
    return 0;
}

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_phone_status_event_handler(int *msg)
{
    u8 *phone_number = NULL;
    struct bt_event *bt = (struct bt_event *)msg;

    if (tws_api_get_role_async() == TWS_ROLE_SLAVE) {
        return 0;
    }

    switch (bt->event) {
    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
#if TCFG_MIX_RECORD_ENABLE
        // 来电，结束录音
        if (get_mix_recorder_status()) {
            r_printf(">>> BT_STATUS_PHONE_INCOME, Stop recoder!\n");
            mix_recorder_stop();
        }
#endif
        put_buf(bt->args, 6);
        esco_dump_packet = ESCO_DUMP_PACKET_CALL;
        u8 tmp_bd_addr[6];
        memcpy(tmp_bd_addr, bt->args, 6);
        /*
         *(1)1t2有一台通话的时候，另一台如果来电会叠加
         */
        g_bt_hdl.phone_ring_sync_tws = 1;
        tws_phone_call_send_cmd(CMD_PHONE_INCOME, bt->args, 0, 1);
        /* bt_phone_income(0, 0, tmp_bd_addr); */
#if TCFG_BT_PHONE_NUMBER_ENABLE
        bt_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
#endif
        break;
    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        put_buf(bt->args, 6);
        /* bt_phone_out(bt->args); */
        tws_phone_call_send_cmd(CMD_PHONE_OUT, bt->args, 0, 0);
#if TCFG_BT_PHONE_NUMBER_ENABLE
        bt_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
#endif
        break;
    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        tws_phone_call_send_cmd(CMD_PHONE_ACTIVE, bt->args, 0, 1);

        break;
    case BT_STATUS_PHONE_HANGUP:
        log_info("BT_STATUS_PHONE_HANGUP\n");
#if TCFG_MIX_RECORD_ENABLE
        // 挂断电话, 结束录音
        if (get_mix_recorder_status()) {
            printf(">>> BT_STATUS_PHONE_HANGUP, Stop recoder!\n");
            mix_recorder_stop();
        }
#endif
        put_buf(bt->args, 6);
        tws_phone_call_send_cmd(CMD_PHONE_HANGUP, bt->args, 0, 1);

        break;
    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        phone_number = (u8 *)bt->value;
        if (g_bt_hdl.phone_num_flag == 1) {
            break;
        }
        g_bt_hdl.income_phone_len = 0;
        memset(g_bt_hdl.income_phone_num, '\0',
               sizeof(g_bt_hdl.income_phone_num));
        for (int i = 0; i < strlen((const char *)phone_number); i++) {
            if (phone_number[i] >= '0' && phone_number[i] <= '9') {
                //过滤，只有数字才能报号
                g_bt_hdl.income_phone_num[g_bt_hdl.income_phone_len++] = phone_number[i];
                if (g_bt_hdl.income_phone_len >= sizeof(g_bt_hdl.income_phone_num)) {
                    break;    /*buffer 空间不够，后面不要了*/
                }
            }
        }
        if (g_bt_hdl.income_phone_len > 0) {
            g_bt_hdl.phone_num_flag = 1;
        } else {
            log_info("PHONE_NUMBER len err\n");
        }
        break;
    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
#if TCFG_BT_INBAND_RING
        g_bt_hdl.inband_ringtone = bt->value;
#else
        g_bt_hdl.inband_ringtone = 0;
#endif
        printf("inband_ringtone=0x%x\n", g_bt_hdl.inband_ringtone);
        break;

    case BT_STATUS_SCO_STATUS_CHANGE:
        printf("BT_STATUS_SCO_STATUS_CHANGE len:%d, type:%d\n",
               (bt->value >> 16), (bt->value & 0x0000ffff));
        if (bt->value != 0xff) {
            u8 call_vol = 15;
            //为了解决两个手机都在通话，在手机上轮流切声卡的音量问题
            call_vol = bt_get_call_vol_for_addr(bt->args);
            //r_printf("---bt_get_call_vol_for_addr--%d\n", call_vol);
            app_audio_state_switch(APP_AUDIO_STATE_CALL, app_var.aec_dac_gain, NULL);
            app_audio_set_volume(APP_AUDIO_STATE_CALL, call_vol, 1);
#if TCFG_MIC_EFFECT_ENABLE
            mic_effect_player_pause(1);
#endif
            bt_tws_sync_volume();
            tws_phone_call_send_cmd(CMD_OPEN_ESCO_PLAYER, bt->args, 0, 1);

        } else {
            u8 bt_esco_play[6];
            int ret = esco_player_get_btaddr(bt_esco_play);
            if (ret && memcmp(bt_esco_play, bt->args, 6) != 0) {
                //如果有地址在是用esco音频，但跟传出来的值地址不一致，就不更新了。
                puts("<<<<<<<<<<<bt_esco_stop err,check addr\n");
                break;
            }

            bt_phone_esco_stop(bt->args);
            tws_phone_call_send_cmd(CMD_CLOSE_ESCO_PLAYER, bt->args, 0, 1);
#if TCFG_MIC_EFFECT_ENABLE
            mic_effect_player_pause(0);
#endif

        }
        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        //判断是当前地址的音量值才更新
        u8 bt_esco_play[6];
        int ret = esco_player_get_btaddr(bt_esco_play);
        if (ret && memcmp(bt_esco_play, bt->args, 6) != 0) {
            //如果有地址在是用esco音频，但跟传出来的值地址不一致，就不更新了。
            break;
        }
        printf("call_vol:%d", bt->value);
        u8 volume = bt->value;
        u8 call_status = bt_get_call_status_for_addr(bt->args);

#if TCFG_BT_INBAND_RING
        if ((call_status != BT_CALL_ACTIVE) && (bt->value == 0)) {
            printf("not set vol\n");
            break;
        }
#endif

        g_bt_hdl.phone_vol = bt->value;
        if ((call_status == BT_CALL_ACTIVE) ||
            (call_status == BT_CALL_OUTGOING) || app_var.siri_stu) {
            app_audio_set_volume(APP_AUDIO_STATE_CALL, volume, 1);
            bt_tws_sync_volume();
        } else {
            /*
             *只保存，不设置到dac
             *微信语音通话的时候，会更新音量到app，但是此时的call status可能是hangup
             */
            app_var.call_volume = volume;
        }
        break;

    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
        log_info("BT_STATUS_LAST_CALL_TYPE_CHANGE:%d\n", bt->value);
        g_bt_hdl.last_call_type = bt->value;
        tws_phone_call_send_cmd(CMD_PHONE_LASET_CALL_TYPE, bt->args, bt->value, 0);
        break;

    case BT_STATUS_CONN_HFP_CH:
        r_printf("++++++++ BT_STATUS_CONN_HFP_CH +++++++++  \n");
        break;
    case BT_STATUS_DISCON_HFP_CH:
        r_printf("++++++++ BT_STATUS_DISCON_HFP_CH +++++++++  \n");
        break;
    case BT_STATUS_PHONE_MANUFACTURER:
        log_info("BT_STATUS_PHONE_MANUFACTURER:%d\n", bt->value);
        app_var.remote_dev_company = bt->value;
        break;
    case BT_STATUS_SIRI_OPEN:
    case BT_STATUS_SIRI_CLOSE:
    case BT_STATUS_SIRI_GET_STATE:
        /* case BT_STATUS_VOICE_RECOGNITION: */
        log_info(" BT_STATUS_SIRI_RECOGNITION:%d \n", bt->value);
        /* put_buf(bt, sizeof(struct bt_event)); */
        app_var.siri_stu = bt->value;
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
        tws_phone_call_send_cmd(CMD_PHONE_SIRI, bt->args, bt->value, 0);
        break;
    case BT_STATUS_SCO_CONNECTION_REQ :
        log_info(" BT_STATUS_SCO_CONNECTION_REQ");
        put_buf(bt->args, 6);
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }

    return 0;
}
APP_MSG_HANDLER(bt_phone_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_phone_status_event_handler,
};


static int tws_phone_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (esco_player_is_playing(bt->args)) {
            bt_phone_esco_stop(bt->args);
        }
        break;
    }

    return 0;
}
APP_MSG_HANDLER(tws_phone_hci_msg_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = tws_phone_hci_event_handler,
};
/* void auto_role_switch_test(void *pri) */
/* { */
/* sys_enter_soft_poweroff(POWEROFF_RESET); */

/* } */

static int call_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];
    u8 phone_addr[6];
    u8 btaddr[6];

    switch (evt->event) {
    case TWS_EVENT_MONITOR_START:
        r_printf("TWS_EVENT_MONITOR_START");
        memcpy(phone_addr, &(evt->args[3]), 6);
        put_buf(phone_addr, 6);

        memset(g_play_addr, 0xff, 6);
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            void *device = btstack_get_conn_device(phone_addr);
            if (device) {
                printf("g_bt_hdl.phone_income_flag=%d,%d\n", g_bt_hdl.phone_income_flag, btstack_bt_get_call_status(device));
                put_buf(g_bt_hdl.phone_ring_addr, 6);
                if (BT_CALL_INCOMING == btstack_bt_get_call_status(device) && (!g_bt_hdl.phone_ring_sync_tws)
                    /*&& (memcmp(g_bt_hdl.phone_ring_addr, phone_addr, 6) == 0)*/) { //phone_ring_sync_tws 手机来电过程中连接，已同步过一次，这里不在发起同步播铃声。只有tws后加入才同步播
                    g_bt_hdl.phone_ring_sync_tws = 1;
                    tws_phone_call_send_cmd(CMD_PHONE_INCOME, phone_addr, 1, 1);

                }
            }

        }
        break;
    case TWS_EVENT_MONITOR_M_START_ENTER_ESCO:
        r_printf("TWS_EVENT_MONITOR_M_START_ENTER_ESCO\n");

        /* sys_timeout_add(NULL, auto_role_switch_test, 5 * 1000); */
        break;
    case TWS_EVENT_MONITOR_S_START_ENTER_ESCO:
        r_printf("TWS_EVENT_MONITOR_S_START_ENTER_ESCO\n");
#if ((defined TCFG_TWS_ESCO_MODE) && (TCFG_TWS_ESCO_MODE == TWS_ESCO_ONLY_MASTER))
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            printf("tws role slave,esco player disable\n");
            lmp_private_esco_suspend_resume(1);
            break;
        }
#endif

#if TCFG_MIC_EFFECT_ENABLE
        mic_effect_player_pause(1);
#endif

        bt_phone_esco_play(&(evt->args[3]));
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        if (esco_player_is_playing(evt->args + 3)) {
            bt_phone_esco_stop(evt->args + 3);
        }
        break;
    }
    return 0;
}
APP_MSG_HANDLER(call_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = call_tws_msg_handler,
};
#endif
