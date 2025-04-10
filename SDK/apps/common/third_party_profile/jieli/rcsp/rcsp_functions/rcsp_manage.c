#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_manage.data.bss")
#pragma data_seg(".rcsp_manage.data")
#pragma const_seg(".rcsp_manage.text.const")
#pragma code_seg(".rcsp_manage.text")
#endif
#include "rcsp_config.h"

#include "app_main.h"
#include "rcsp_update_tws.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_tws.h"
#include "rcsp_manage.h"
#include "ble_rcsp_server.h"
#include "rcsp_setting_opt.h"
#include "rcsp_adv_bluetooth.h"
#include "rcsp.h"
#include "rcsp_device_status.h"
#include "rcsp_music_info_setting.h"
#include "rcsp_bt_manage.h"
#include "rcsp_update.h"
#include "JL_rcsp_protocol.h"
#include "rcsp_extra_flash_opt.h"
#include "file_transfer.h"
#include "file_bluk_trans_prepare.h"
#include "custom_cfg.h"
#include "system/event.h"
#include "syscfg_id.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_packet.h"
#include "spp_user.h"
#include "spp_config.h"
#include "rcsp_command.h"
#include "ble_rcsp_adv.h"
#include "app_msg.h"
#include "btstack_rcsp_user.h"
#include "rcsp_ch_loader_download.h"

#if RCSP_MODE

#define LOG_TAG             "[RCSP-ADV]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern void rcsp_find_device_reset(void);
extern void sport_data_func_init(void);
void sport_data_func_release(void);
static void rcsp_user_state_handler(u8 *param, u8 param_len);
extern const int support_dual_bank_update_en;

// 获取当前ble/spp的连接状态
u8 get_rcsp_connect_status(void)
{
    if (bt_rcsp_device_conn_num() > 0) {
        return 1;
    }
    return 0;
}

void JL_rcsp_event_to_user(u32 type, u8 event, u8 *msg, u8 size)
{
    struct rcsp_event rcsp_e;
    memset(&rcsp_e, 0, sizeof(struct rcsp_event));
    if (size > sizeof(rcsp_e.args)) {
        log_info("rcsp event size overflow:%x %lx\n", size, sizeof(rcsp_e.args));
    }
    rcsp_e.event = event;
    if (size) {
        memcpy(rcsp_e.args, msg, size);
    }
    rcsp_e.size = size;
    struct rcsp_event *rcsp_e_p = &rcsp_e;
    app_send_message_from(MSG_FROM_RCSP, sizeof(*rcsp_e_p), (int *)rcsp_e_p);
}

/**
 * @abstract 处理rcsp事件
 */
int JL_rcsp_event_handler(struct rcsp_event *rcsp)
{
    int ret = 0;
    switch (rcsp->event) {
    case MSG_JL_ADV_SETTING_SYNC:
        log_info("MSG_JL_ADV_SETTING_SYNC\n");
        update_rcsp_setting(-1);
        break;
    case MSG_JL_ADV_SETTING_UPDATE:
        log_info("MSG_JL_ADV_SETTING_UPDATE\n");
        update_info_from_vm_info();
        break;
    case MSG_JL_UPDATE_EQ:
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_EQ_INFO));
        break;
    case MSG_JL_USER_SPP_BLE_STATE:
        log_info("MSG_JL_USER_SPP_BLE_STATE\n");
        rcsp_user_state_handler(rcsp->args, rcsp->size);
        break;
#if RCSP_ADV_MUSIC_INFO_ENABLE
    case MSG_JL_UPDATE_PLAYER_TIME:
        log_info("MSG_JL_UPDATE_PLAYER_TIME\n");
        if (JL_rcsp_get_auth_flag()) {
            rcsp_device_status_update(BT_FUNCTION_MASK, 0x100);
        }
        break;
    case MSG_JL_UPDATE_PLAYER_STATE:
        log_info("MSG_JL_UPDATE_PLAYER_STATE\n");
        if (JL_rcsp_get_auth_flag()) {
            rcsp_device_status_update(BT_FUNCTION_MASK, 0x80);
        }
        break;
    case MSG_JL_UPDATE_MUSIC_INFO:
        /* log_info("MSG_JL_UPDATE_MUSIC_INFO\n"); */
        if (JL_rcsp_get_auth_flag()) {
            /* log_info("rcsp type %x\n",rcsp->args[0]); */
            rcsp_device_status_update(BT_FUNCTION_MASK, BIT(rcsp->args[0] - 1));
        }
        break;
    case  MSG_JL_UPDATE_MUSIC_PLAYER_TIME_TEMER:
        log_info("MSG_JL_UPDATE_MUSIC_PLAYER_TIME_TEMER\n");
        if (JL_rcsp_get_auth_flag()) {
            music_player_time_timer_deal(rcsp->args[0]);
        }
        break;
#endif
#if (RCSP_ADV_ANC_VOICE)
    case MSG_JL_UPDATE_ANC_VOICE:
        log_info("MSG_JL_UPDATE_ANC_VOICE\n");
#if RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
        rcsp_device_status_update(COMMON_FUNCTION, (BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_ANC_VOICE) | BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION)));
#else
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_ANC_VOICE));
#endif
        break;
    case MSG_JL_UPDATE_ANC_VOICE_MAX_SYNC:
        log_info("MSG_JL_UPDATE_ANC_VOICE_MAX_SYNC\n");
#if TCFG_USER_TWS_ENABLE && RCSP_ADV_ANC_VOICE
        extern void anc_voice_max_val_swap_sync(void);
        anc_voice_max_val_swap_sync();
#endif
        break;
#endif
#if RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
    case MSG_JL_UPDATE_ADAPTIVE_NOISE_REDUCTION:
        log_info("MSG_JL_UPDATE_ADAPTIVE_NOISE_REDUCTION\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION));
        break;
#endif
#if RCSP_ADV_AI_NO_PICK
    case MSG_JL_UPDATE_AI_NO_PICK:
        log_info("MSG_JL_UPDATE_AI_NO_PICK\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_AI_NO_PICK));
        break;
#endif
#if RCSP_ADV_SCENE_NOISE_REDUCTION
    case MSG_JL_UPDATE_SCENE_NOISE_REDUCTION:
        log_info("MSG_JL_UPDATE_SCENE_NOISE_REDUCTION\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_SCENE_NOISE_REDUCTION));
        break;
#endif
#if RCSP_ADV_WIND_NOISE_DETECTION
    case MSG_JL_UPDATE_WIND_NOISE_DETECTION:
        log_info("MSG_JL_UPDATE_WIND_NOISE_DETECTION\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_WIND_NOISE_DETECTION));
        break;
#endif
#if RCSP_ADV_VOICE_ENHANCEMENT_MODE
    case MSG_JL_UPDATE_VOICE_ENHANCEMENT_MODE:
        log_info("MSG_JL_UPDATE_VOICE_ENHANCEMENT_MODE\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_VOICE_ENHANCEMENT_MODE));
        break;
#endif
#if TCFG_RCSP_DUAL_CONN_ENABLE
    case MSG_JL_1T2_SETTING:
        log_info("MSG_JL_UPDATE_VOICE_ENHANCEMENT_MODE\n");
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_1T2));
        break;
#endif
#if RCSP_ADV_FIND_DEVICE_ENABLE
    case MSG_JL_FIND_DEVICE_RESUME:
        log_info("MSG_JL_FIND_DEVICE_RESUME\n");
        /* printf("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        /* put_buf(rcsp->args, 3); */
#if TCFG_USER_TWS_ENABLE
        find_decice_tws_connect_handle(2, rcsp->args);
#endif
#if RCSP_MODE == RCSP_MODE_EARPHONE
        extern void earphone_mute_handler(u8 * other_opt, u32 msec);
        earphone_mute_handler(rcsp->args, 300);
#endif
        break;
    case MSG_JL_FIND_DEVICE_STOP:
        log_info("MSG_JL_FIND_DEVICE_STOP\n");
        u16 sec = *((u16 *)rcsp->args);
        /* printf("rcsp_find %s, %s, %d, sec:%x\n", __FILE__, __FUNCTION__, __LINE__, sec); */
        /* put_buf(rcsp->args, 3); */
#if TCFG_USER_TWS_ENABLE
        find_decice_tws_connect_handle(1, rcsp->args);
#endif
        extern void find_device_timeout_handle(u32 sec);
        find_device_timeout_handle(sec);
        break;
#endif
    case MSG_JL_TWS_NEED_UPDATE:
        log_info("MSG_JL_TWS_NEED_UPDATE\n");
        syscfg_write(VM_UPDATE_FLAG, (const void *)rcsp->args, 1);
        break;

    default:
        log_info("default\n");
#if RCSP_ADV_EN
        if (0 == JL_rcsp_adv_event_handler(rcsp)) {
            break;
        }
#endif
#if RCSP_UPDATE_EN
        if (0 == JL_rcsp_update_msg_deal(NULL, rcsp->event, rcsp->args)) {
            break;
        }
#endif
        break;
    }

    return ret;
}

static void rcsp_ble_disconnect(void)
{
    app_rcsp_task_switch_stop();
#if (TCFG_DEV_MANAGER_ENABLE && RCSP_FILE_OPT)
    rcsp_file_transfer_close();
    rcsp_file_bluk_trans_close(1);
#endif
#if JL_RCSP_EXTRA_FLASH_OPT
    rcsp_extra_flash_disconnect_tips(10);
#endif
#if RCSP_UPDATE_EN && !RCSP_BLE_MASTER
    rcsp_update_resume();
#endif
    sport_data_func_release();
    rcsp_timer_contrl(0);

    // 防止上一次接收长度太长且未接收完成就中断，影响到下一次连接后的交互
    JL_packet_clear();
}

static void rcsp_ble_connect(void)
{
#if JL_RCSP_EXTRA_FLASH_OPT
    rcsp_extra_flash_disconnect_tips(0);
#endif
    sport_data_func_init();
    rcsp_timer_contrl(1);
    set_ble_adv_notify(1);
}

static void rcsp_user_state_event(u8 opcode, u8 state)
{
    u8 data[2] = {0};
    data[0] = opcode;
    data[1] = state;
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_USER_SPP_BLE_STATE, data, sizeof(data));
}

#if TCFG_USER_BLE_CTRL_BREDR_EN
extern void bt_close_bredr();
extern void bt_init_bredr();
extern void bredr_conn_last_dev();

#endif

// 需要放到app_core处理ble状态的函数
void rcsp_user_event_ble_handler(ble_state_e ble_status, u8 flag)
{
    /* if (cpu_in_irq()) { */
    /*     printf("%s, %s, %d, cpu_in_irq\n", __FILE__, __FUNCTION__, __LINE__); */
    /* } else { */
    /*     printf("%s, %s, %d, task:%s\n", __FILE__, __FUNCTION__, __LINE__, os_current_task()); */
    /* } */
    switch (ble_status) {
    case BLE_ST_INIT_OK:

#if TCFG_USER_BLE_CTRL_BREDR_EN
        bt_close_bredr();
#endif
        break;
    case BLE_ST_IDLE:
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            if (0 == support_dual_bank_update_en) {
                JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
            }
        }
#endif
        break;
    case BLE_ST_CONNECT:
        if (flag) {
            rcsp_user_state_event(RCSP_BT_EVENT_BLE_STATUS, BLE_ST_CONNECT);
            break;
        }
        rcsp_ble_connect();
#if (RCSP_ADV_FIND_DEVICE_ENABLE)
        log_info("rcsp_find_device_reset\n");
        rcsp_find_device_reset();
#endif
#if (TCFG_USER_BLE_CTRL_BREDR_EN)
        //bt_init_bredr();
        bredr_conn_last_dev();
#endif
        break;
    case BLE_ST_DISCONN:
        if (flag) {
            rcsp_user_state_event(RCSP_BT_EVENT_BLE_STATUS, BLE_ST_DISCONN);
            break;
        }
        rcsp_ble_disconnect();
#if RCSP_ADV_MUSIC_INFO_ENABLE
        stop_get_music_timer(1);
#endif
#if JL_RCSP_EXTRA_FLASH_OPT
        if (!get_defalut_bt_channel_sel() || get_curr_platform()) {
            // 当选择spp是不执行这句话
            rcsp_extra_flash_opt_stop();
        }
#endif
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            rcsp_bt_ble_adv_enable(0);
        }
#endif
        break;
    default:
        break;
    }
}

// 需要app_core线程处理的操作放到以下函数
void rcsp_user_event_spp_handler(u8 spp_status, u8 flag)
{
    if (cpu_in_irq()) {
        printf("%s, %s, %d, cpu_in_irq\n", __FILE__, __FUNCTION__, __LINE__);
    } else {
        printf("%s, %s, %d, task:%s\n", __FILE__, __FUNCTION__, __LINE__, os_current_task());
    }
    switch (spp_status) {
    case SPP_USER_ST_NULL:
        break;
    case SPP_USER_ST_CONNECT:
        if (flag) {
            rcsp_user_state_event(RCSP_BT_EVENT_SPP_STATUS, spp_status);
            break;
        }
        rcsp_ble_connect();
#if (RCSP_ADV_FIND_DEVICE_ENABLE)
        log_info("rcsp_find_device_reset\n");
        rcsp_find_device_reset();
#endif
        break;
    default:
        if (flag) {
            rcsp_user_state_event(RCSP_BT_EVENT_SPP_STATUS, spp_status);
            break;
        }

        if (bt_rcsp_device_conn_num() == 0) {
            rcsp_ble_disconnect();
#if RCSP_ADV_MUSIC_INFO_ENABLE
            stop_get_music_timer(1);
#endif
#if JL_RCSP_EXTRA_FLASH_OPT
            rcsp_extra_flash_opt_stop();
#endif
        }
#if RCSP_UPDATE_EN
        if (get_jl_update_flag()) {
            if (0 == support_dual_bank_update_en) {
                JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
            }
        }
#endif
        break;
    }
}

static void rcsp_user_event_select_handler(void)
{
#if RCSP_UPDATE_EN
    set_jl_update_flag(0);
#endif
}

static void rcsp_user_state_handler(u8 *param, u8 param_len)
{
    u8 opcode = param[0];
    u8 state = param[1];
    switch (opcode) {
    case RCSP_BT_EVENT_BLE_STATUS:
        rcsp_user_event_ble_handler((ble_state_e)state, 0);
        break;
    case RCSP_BT_EVENT_SPP_STATUS:
        rcsp_user_event_spp_handler(state, 0);
        break;
    default:
        break;
    }
}

#endif
