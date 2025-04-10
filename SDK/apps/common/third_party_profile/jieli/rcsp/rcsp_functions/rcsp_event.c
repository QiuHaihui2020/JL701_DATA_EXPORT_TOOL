#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_event.data.bss")
#pragma data_seg(".rcsp_event.data")
#pragma const_seg(".rcsp_event.text.const")
#pragma code_seg(".rcsp_event.text")
#endif
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "rcsp.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"
#include "rcsp_device_status.h"
#include "ble_rcsp_server.h"
#include "file_trans_back.h"
#include "file_transfer.h"

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif
#include "app_task.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "rcsp_misc_setting.h"
#include "rcsp_device_info_func_common.h"

#if (RCSP_MODE)

/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

extern void bt_search_device(void);

extern void emitter_search_stop(u8 result);

#include "dev_manager.h"
// 如果可视化分支已实现以下虚函数，请移除
extern int app_task_switch_to(u8 app_task, int priv);
extern u8 app_get_curr_task(void);

static void file_browser_stop_resp(u8 reason)
{
    JL_CMD_send(JL_OPCODE_FILE_BROWSE_REQUEST_STOP, &reason, 1, JL_NEED_RESPOND, 0, NULL);
}

static void file_browser_stop(u8 reason)
{
    int argv[3];
    argv[0] = (int)file_browser_stop_resp;
    argv[1] = 1;
    argv[2] = reason;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
}

static void rcsp_common_event_deal(int msg, int argc, int *argv)
{
    int err = 0;
    struct RcspModel *rcspModel = (struct RcspModel *)argv[0];
    if (rcspModel == NULL) {
        rcsp_printf("rcspModelhdl NULL err!!\n");
        return ;
    }

    switch (msg) {
    case USER_MSG_RCSP_DISCONNECT_EDR:
        // 这里不会被调用了
        rcsp_printf("USER_MSG_RCSP_DISCONNECT_EDR\n");
        if (bt_get_curr_channel_state() != 0) {
            bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
            bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
        break;
#if TCFG_USER_EMITTER_ENABLE
    case USER_MSG_RCSP_BT_SCAN_OPEN:
        rcsp_printf("USER_MSG_RCSP_BT_SCAN_OPEN\n");
        bt_search_device();
        break;
    case USER_MSG_RCSP_BT_SCAN_STOP:
        rcsp_printf("USER_MSG_RCSP_BT_SCAN_STOP\n");
        emitter_search_stop(0);
        break;

    case USER_MSG_RCSP_BT_CONNECT_SPEC_ADDR:
        rcsp_printf("USER_MSG_RCSP_BT_CONNECT_SPEC_ADDR\n");
        rcsp_put_buf(rcspModel->emitter_con_addr, 6);
        bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, rcspModel->emitter_con_addr);
        break;
#endif//TCFG_USER_EMITTER_ENABLE

    case USER_MSG_RCSP_SET_VOL:
        rcsp_printf("USER_MSG_RCSP_SET_VOL\n");
        rcsp_device_status_update(COMMON_FUNCTION, (u32)argv[1]);
        break;
    case USER_MSG_RCSP_SET_EQ_PARAM:
        rcsp_printf("USER_MSG_RCSP_SET_EQ_PARAM\n");
        rcsp_device_status_update(COMMON_FUNCTION, (u32)argv[1]);
        break;
    case USER_MSG_RCSP_SET_HIGH_LOW_VOL_PARAM:
        rcsp_printf("USER_MSG_RCSP_SET_HIGH_LOW_VOL_PARAM\n");
        rcsp_device_status_update(COMMON_FUNCTION, (u32)argv[1]);
        break;
    case USER_MSG_RCSP_SET_FMTX_FREQ:
        rcsp_printf("USER_MSG_RCSP_SET_FMTX_FREQ\n");
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
        u16 freq = (u16)argv[1];
        rcsp_printf("freq %d\n", freq);
        fm_emitter_manage_set_fre(freq);
#endif
        /* #if TCFG_UI_ENABLE */
        /* 			ui_set_menu_ioctl(MENU_FM_DISP_FRE, freq); */
        /* #endif */
        break;
#if TCFG_USER_EMITTER_ENABLE
    case USER_MSG_RCSP_SET_BT_EMITTER_SW:
        rcsp_printf("USER_MSG_RCSP_SET_BT_EMITTER_SW\n");
        break;
    case USER_MSG_RCSP_SET_BT_EMITTER_CONNECT_STATES:
        // 这里不会被调用了
        rcsp_printf("USER_MSG_RCSP_SET_BT_EMITTER_CONNECT_STATES, state = %d\n", argv[1]);
        if (argv[1]) {
            emitter_search_stop(0);
            rcsp_put_buf(rcspModel->emitter_con_addr, 6);
            bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, rcspModel->emitter_con_addr);
        } else {
            if (bt_get_curr_channel_state() != 0) {
                bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
                bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            }
        }
        break;
#endif//TCFG_USER_EMITTER_ENABLE

#if RCSP_FILE_OPT
    case USER_MSG_RCSP_BS_END:
        rcsp_printf("USER_MSG_RCSP_BS_END\n");
        u8 reason = (u8)argv[1];
        rcsp_browser_stop();
        rcsp_printf("reason = %d\n", reason);
        if (2 == reason) {
#if TCFG_APP_MUSIC_EN
            char *dev_logo = (char *)argv[2];
            char *cur_dev_logo = dev_manager_get_logo(dev_manager_find_active(1));
            u32 sclust = (u32)argv[3];
            u8 app = app_get_curr_task();
            rcsp_printf("app= %d, dev_loop = %s, sclust = %x\n", app, dev_logo, sclust);
            if (app == APP_MUSIC_TASK) {
                rcsp_printf("is music mode \n");
                extern struct __music music_hdl;
                struct vfs_attr tmp_attr = {0};
                fget_attrs(music_hdl.player_hd->file, &tmp_attr);
                // RCSP TODO:
                rcsp_printf("RCSP TODO!!!");
                /* int file_dec_status = file_dec_get_status(); */
                if ((tmp_attr.sclust == sclust) //簇号相同
                    && (cur_dev_logo && (strcmp(cur_dev_logo, dev_logo) == 0)) //设备相同
                    /* && (file_dec_status == FILE_DEC_STATUS_WAIT_PLAY || file_dec_status == FILE_DEC_STATUS_PLAY) //正在播放 */
                   ) {
                    //同一个设备的同一首歌曲，在播放的情况，浏览选中不重新播放
                    rcsp_printf("the same music file!!\n");
                } else {
                    // RCSP TODO:
                    rcsp_printf("rcsp play dev by sclust!!!");
                    dev_manager_set_active_by_logo(dev_logo);
                    app_send_message(APP_MSG_MUSIC_PLAY_START_BY_SCLUST, sclust);
                }
            } else {
                rcsp_printf("is not music mode\n");
                ///设定音乐模式初次播放参数为按照簇号播放
                music_task_set_parm(MUSIC_TASK_START_BY_SCLUST, sclust);
                ///将选定的设备设置为活动设备
                dev_manager_set_active_by_logo(dev_logo);
#if (RCSP_MODE == RCSP_MODE_WATCH)
                //进入music后自动播放
                music_set_start_auto_play(1);
#endif
                ///切换模式
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
                app_task_switch_to(APP_MODE_MUSIC, NULL_VALUE);
#else
                app_task_switch_to(APP_MUSIC_TASK, NULL_VALUE);
#endif
            }
#endif /* #if TCFG_APP_MUSIC_EN */
        }
        file_browser_stop(reason);
        break;
#endif
    case USER_MSG_RCSP_MODE_SWITCH:
        bool ret = true;
        u8 mode = (u8)argv[1];
        switch (mode) {
        case FM_FUNCTION:
#if TCFG_APP_FM_EN
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_FM, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_FM_TASK, NULL_VALUE);
#endif
#endif
            break;
        case BT_FUNCTION:
#if (TCFG_APP_BT_EN)
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_BT, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_BT_TASK, NULL_VALUE);
#endif
#endif
            break;
        case MUSIC_FUNCTION:
#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_MUSIC, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_MUSIC_TASK, NULL_VALUE);
#endif
#endif
            break;
        case RTC_FUNCTION:
#if (TCFG_APP_RTC_EN && RCSP_APP_RTC_EN)
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_RTC, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_RTC_TASK, NULL_VALUE);
#endif
#endif
            break;
        case LINEIN_FUNCTION:
#if TCFG_APP_LINEIN_EN
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_LINEIN, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_LINEIN_TASK, NULL_VALUE);
#endif
#endif
            break;
        /* case FMTX_FUNCTION_MASK: */
        /*     break; */

        case SPDIF_FUNCTION:
#if TCFG_APP_SPDIF_EN
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_SPDIF, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_SPDIF_TASK, NULL_VALUE);
#endif
#endif
            break;
        case PC_FUNCTION:
#if TCFG_APP_PC_EN
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
            ret = app_task_switch_to(APP_MODE_PC, NULL_VALUE);
#else
            ret = app_task_switch_to(APP_PC_TASK, NULL_VALUE);
#endif
#endif
            break;
        }
#if RCSP_DEVICE_STATUS_ENABLE
        /* if (false == ret) { */
        /*     function_change_inform(app_get_curr_task(), ret); */
        /* } */
#endif
        printf("USER_MSG_RCSP_MODE_SWITCH:%d %d\n", ret, mode);
        break;
    case USER_MSG_RCSP_FM_UPDATE_STATE:
        rcsp_printf("USER_MSG_RCSP_FM_UPDATE_STATE\n");
        extern void rcsp_fm_msg_deal(int msg);
#if (TCFG_APP_FM_EN)
        rcsp_fm_msg_deal(-1);
#endif
        break;
    case USER_MSG_RCSP_RTC_UPDATE_STATE:
        /* rcsp_printf("USER_MSG_RCSP_RTC_UPDATE_STATE\n"); */
        /* extern void rcsp_rtc_msg_deal(int msg); */
        /* rcsp_rtc_msg_deal(-1); */
        break;
#if (TCFG_DEV_MANAGER_ENABLE && RCSP_FILE_OPT)
    case USER_MSG_RCSP_CONNECT_RESET:
        rcsp_printf("USER_MSG_RCSP_CONNECT_RESET\n");
        rcsp_file_transfer_close();
        break;

    case USER_MSG_RCSP_FILE_TRANS_BACK:
        rcsp_printf("USER_MSG_RCSP_FILE_TRANS_BACK\n");
        rcsp_file_trans_back_close();
        break;
#endif

    case USER_MSG_RCSP_NFC_FILE_TRANS_BACK:
        rcsp_printf("USER_MSG_RCSP_NFC_FILE_TRANS_BACK\n");
        extern void nfc_file_trans_back_end(void *priv, u32 handler, u8 op, int result, int param);
        nfc_file_trans_back_end((void *)rcspModel, (u32)argv[1], (u8)argv[2], argv[3], argv[4]);
        break;
    case USER_MSG_RCSP_SPORT_DATA_EVENT:
        rcsp_printf("USER_MSG_RCSP_SPORT_DATA_EVENT\n");
        extern void sport_data_func_event(void *priv, u8 flag);
        sport_data_func_event((void *)rcspModel, (u8)argv[1]);
        break;
    default:
        break;
    }
}

#define APP_RCSP_MSG_VAL_MAX	8
bool rcsp_msg_post(int msg, int argc, ...)
{
    int argv[APP_RCSP_MSG_VAL_MAX] = {0};
    bool ret = true;
    va_list argptr;
    va_start(argptr, argc);

    if (argc > APP_RCSP_MSG_VAL_MAX) {
        rcsp_printf("%s, msg argc err\n", __FUNCTION__);
        ret = false;
    } else {
        argv[0] = (int)	rcsp_common_event_deal;
        argv[2] = msg;
        for (int i = 0; i < argc; i++) {
            argv[i + 3] = va_arg(argptr, int);
        }

        if (argc >= 2) {
            argv[1] = argc + 1;
        } else {
            argv[1] = 3;
            argc = 3;
        }
        int r = os_taskq_post_type("app_core", Q_CALLBACK, argc + 3, argv);
        if (r) {
            rcsp_printf("app_next post msg err %x\n", r);
            ret = false;
        }
    }
    va_end(argptr);
    return ret;
}

int rcsp_bt_key_event_deal(u8 key_event, int ret)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return ret;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        return ret;
    }
    return ret;
}

int rcsp_common_key_event_deal(u8 key_event, int ret)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return ret;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        return ret;
    }
    return ret;
}

bool rcsp_key_event_filter_before(int key_event)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return false;
    }
    if (0 == rcspModel->dev_vol_sync) {
        return false;
    }
    bool ret = false;
    if (ret) {
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_VOL));
    }
    return ret;
}

#endif

