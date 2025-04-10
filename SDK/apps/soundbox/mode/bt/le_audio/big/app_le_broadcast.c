/*********************************************************************************************
    *   Filename        : app_le_broadcast.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-15 14:18

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "app_tone.h"
#include "app_main.h"
#include "wireless_trans.h"
#include "audio_config.h"
#include "a2dp_player.h"
#include "vol_sync.h"
#include "ble_rcsp_server.h"
#include "spdif_player.h"
#include "fm_api.h"
#include "linein.h"
#include "spdif_file.h"
#include "spdif.h"
#include "soundbox.h"
/* #include "mic.h" */
#include "iis.h"
#include "pc_spk_player.h"
#include "bt_slience_detect.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
#include "ble_rcsp_server.h"
#include "btstack_rcsp_user.h"
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG             "[APP_LE_BROADCAST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define TRANSMITTER_AUTO_TEST_EN    0
#define RECEIVER_AUTO_TEST_EN       0

#define APP_MSG_KEY_SWITCH 3

/**************************************************************************************************
  Data Types
**************************************************************************************************/
enum {
    ///0x1000起始为了不要跟提示音的IDEX_TONE_重叠了
    TONE_INDEX_BROADCAST_OPEN = 0x1000,
    TONE_INDEX_BROADCAST_CLOSE,
};

struct app_bis_hdl_t {
    u8 bis_status;
    u8 remote_dev;
    u16 bis_hdl;
    u16 aux_hdl;
    audio_param_t enc;
};

struct app_big_hdl_t {
    u8 used;
    u8 big_status;
    u8 big_hdl;
    u8 local_dev;
    struct app_bis_hdl_t bis_hdl_info[BIG_MAX_BIS_NUMS];
};

/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/
static bool is_broadcast_as_transmitter();
static void broadcast_pair_tx_event_callback(const PAIR_EVENT event, void *priv);
static void broadcast_pair_rx_event_callback(const PAIR_EVENT event, void *priv);
static int broadcast_padv_data_deal(void *priv);
static void remote_dev_msg_status_sync_info_send(u16 acl_hdl);

/**************************************************************************************************
  Local Global Variables
**************************************************************************************************/
static OS_MUTEX mutex;
static u8 broadcast_last_role = 0; /*!< 挂起前广播角色 */
static u8 broadcast_app_mode_exit = 0;  /*!< 音源模式退出标志 */
static u8 bis_connected_nums = 0;
static struct broadcast_sync_info app_broadcast_data_sync;  /*!< 用于记录广播同步状态 */
static struct app_big_hdl_t app_big_hdl_info[BIG_MAX_NUMS];
static u8 config_broadcast_as_master = 0;   /*!< 配置广播强制做主机 */
static u8 receiver_connected_status = 0; /*< 广播接收端连接状态 */
static u8 app_broadcast_init_flag = 0;
static int cur_deal_scene = -1; /*< 当前系统处于的运行场景 */
static const pair_callback_t pair_tx_cb = {
    .pair_event_cb = broadcast_pair_tx_event_callback,
};
static const pair_callback_t pair_rx_cb = {
    .pair_event_cb = broadcast_pair_rx_event_callback,
};

#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
static u8 save_sync_status_table[5][2] = {0};
#endif

static u8 bis_switch_onoff = 0;

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
static int rcsp_connect_dev_detect_timer = 0;
#endif
/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* --------------------------------------------------------------------------*/
/**
 * @brief 申请互斥量，用于保护临界区代码，与app_broadcast_mutex_post成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_broadcast_mutex_pend(OS_MUTEX *mutex, u32 line)
{
    if (!app_broadcast_init_flag) {
        log_error("%s err, mutex uninit", __FUNCTION__);
        return;
    }

    int os_ret;
    os_ret = os_mutex_pend(mutex, 0);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 用于外部重新打开发送端的数据流
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_reset_transmitter(void)
{
    int i = 0;
    if (get_broadcast_role() == BROADCAST_ROLE_TRANSMITTER) {
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            //固定收发角色重启广播数据流
            broadcast_audio_recorder_reset(app_big_hdl_info[i].big_hdl);
        }
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 释放互斥量，用于保护临界区代码，与app_broadcast_mutex_pend成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_broadcast_mutex_post(OS_MUTEX *mutex, u32 line)
{
    if (!app_broadcast_init_flag) {
        log_error("%s err, mutex uninit", __FUNCTION__);
        return;
    }

    int os_ret;
    os_ret = os_mutex_post(mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief BIG开关提示音结束回调接口
 *
 * @param priv:传递的参数
 * @param event:提示音回调事件
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int broadcast_tone_play_end_callback(void *priv, enum stream_event event)
{
    u32 index = (u32)priv;

    g_printf("%s, event:0x%x", __FUNCTION__, event);

    switch (event) {
    case STREAM_EVENT_NONE:
    case STREAM_EVENT_STOP:
        switch (index) {
        case TONE_INDEX_BROADCAST_OPEN:
            g_printf("TONE_INDEX_BROADCAST_OPEN");
            break;
        case TONE_INDEX_BROADCAST_CLOSE:
            g_printf("TONE_INDEX_BROADCAST_CLOSE");
            break;
        default:
            break;
        }
    default:
        break;
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取接收方连接状态
 *
 * @return 接收方连接状态
 */
/* ----------------------------------------------------------------------------*/
u8 get_receiver_connected_status(void)
{
    return receiver_connected_status;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief BIG状态事件处理函数
 *
 * @param msg:状态事件附带的返回参数
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int app_broadcast_conn_status_event_handler(int *msg)
{
    u8 i, j;
    struct app_mode *mode;
    u8 find = 0;
    u8 big_hdl;
    int ret;
    big_hdl_t *hdl;
    int *event = msg;
    int result = 0;
    u32 bitrate = 0;

    /* g_printf("%s, event:0x%x", __FUNCTION__, event[0]); */

    switch (event[0]) {
    case BIG_EVENT_TRANSMITTER_CONNECT:
        g_printf("BIG_EVENT_TRANSMITTER_CONNECT");
        //由于是异步操作需要加互斥量保护，避免broadcast_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_broadcast_mutex_pend(&mutex, __LINE__);

        //关闭VM写RAM
        /* vm_ram_direct_2_flash_switch(FALSE); */

        hdl = (big_hdl_t *)&event[1];

        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (app_big_hdl_info[i].used && (app_big_hdl_info[i].big_hdl == hdl->big_hdl)) {
                app_big_hdl_info[i].local_dev = WIRELESS_TX_DEV1;
                for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                    if (!app_big_hdl_info[i].bis_hdl_info[j].bis_hdl)     {
                        app_big_hdl_info[i].bis_hdl_info[j].bis_hdl = hdl->bis_hdl[0];
                        app_big_hdl_info[i].bis_hdl_info[j].aux_hdl = hdl->aux_hdl;
                        app_big_hdl_info[i].bis_hdl_info[j].remote_dev = WIRELESS_RX_DEV1;
                        memcpy(&app_big_hdl_info[i].bis_hdl_info[j].enc, &hdl->enc, sizeof(audio_param_t));
                        find = 1;
                        break;
                    }
                }
            }
            if (find) {
                break;
            }
        }

        if (!find) {
            //释放互斥量
            app_broadcast_mutex_post(&mutex, __LINE__);
            break;
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

        bitrate = get_big_audio_coding_bit_rate();
#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
        app_broadcast_data_sync.bit_rate = bitrate / 2;
#else
        app_broadcast_data_sync.bit_rate = bitrate;
#endif
        app_broadcast_data_sync.coding_type = LE_AUDIO_CODEC_TYPE;
        app_broadcast_data_sync.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
        broadcast_set_sync_data(hdl->big_hdl, &app_broadcast_data_sync, sizeof(struct broadcast_sync_info));

        mode = app_get_current_mode();
        ASSERT(mode);
        ret = broadcast_transmitter_connect_deal((void *)hdl, mode->name);
        if (ret < 0) {
            r_printf("broadcast_transmitter_connect_deal fail");
        }

        //释放互斥量
        app_broadcast_mutex_post(&mutex, __LINE__);
        break;

    case BIG_EVENT_RECEIVER_CONNECT:
        g_printf("BIG_EVENT_RECEIVER_CONNECT");
        //由于是异步操作需要加互斥量保护，避免broadcast_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_broadcast_mutex_pend(&mutex, __LINE__);

        //关闭VM写RAM
        /* vm_ram_direct_2_flash_switch(FALSE); */

        receiver_connected_status = 1;

        hdl = (big_hdl_t *)&event[1];

#if (!TCFG_KBOX_1T3_MODE_EN)
        //接收端更新同步广播发送端的参数
        update_receiver_big_params(hdl->enc.max_sdu, hdl->enc.sdu_int_us);
#endif

        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (app_big_hdl_info[i].used && (app_big_hdl_info[i].big_hdl == hdl->big_hdl)) {
                app_big_hdl_info[i].local_dev = WIRELESS_RX_DEV1;
                for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                    if (!app_big_hdl_info[i].bis_hdl_info[j].bis_hdl)     {
                        app_big_hdl_info[i].bis_hdl_info[j].bis_hdl = hdl->bis_hdl[0];
                        app_big_hdl_info[i].bis_hdl_info[j].aux_hdl = hdl->aux_hdl;
                        app_big_hdl_info[i].bis_hdl_info[j].remote_dev = WIRELESS_TX_DEV1 << j;
                        memcpy(&app_big_hdl_info[i].bis_hdl_info[j].enc, &hdl->enc, sizeof(audio_param_t));
                        find = 1;
                        break;
                    }
                }
            }
            if (find) {
                break;
            }
        }

        if (!find) {
            //释放互斥量
            app_broadcast_mutex_post(&mutex, __LINE__);
            break;
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

        g_printf("big_hdl:0x%x, bis_hdl:0x%x, aux_hdl:0x%x", hdl->big_hdl, hdl->bis_hdl[0], hdl->aux_hdl);
#if TCFG_KBOX_1T3_MODE_EN
        bis_connected_nums++;
        ASSERT(bis_connected_nums <= BIG_MAX_BIS_NUMS && bis_connected_nums >= 0, "bis_connected_nums:%d", bis_connected_nums);
#if (TCFG_KBOX_1T3_BIND_EN == 2) //0-不配对，1-TX生成配对码，2-RX生成配对码
        ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_code, sizeof(u32));
        if ((ret <= 0) || (pair_code == 0xFFFFFFFF)) {
            wireless_trans_get_pair_code("big_rx", (u8 *)&pair_code, 1);
            ret = syscfg_write(VM_WIRELESS_PAIR_CODE0, &pair_code, sizeof(u32));
            if (ret <= 0) {
                r_printf(">>>>>>wireless pair code save err, %d", __LINE__);
            }
        }
        wireless_custom_data_send_to_sibling('P', &pair_code, sizeof(u32), remote_dev_identification);
        wireless_trans_set_pair_code("big_rx", (u8 *)&pair_code);
#elif (TCFG_KBOX_1T3_BIND_EN == 1)  //由于TX事件比较快，导致TX发送配对码时，RX仍未准备好，此处改为引导TX发配对码
        pair_code = 0x55aa55aa;
        wireless_custom_data_send_to_sibling('P', &pair_code, sizeof(u32), remote_dev_identification);
#endif
#endif

        ret = broadcast_receiver_connect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("broadcast_receiver_connect_deal fail");
        }

#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
        if (bis_connected_nums > 1) {
            for (i = 0; i < BIG_MAX_NUMS; i++) {
                for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                    remote_dev_msg_status_sync_info_send(app_big_hdl_info[i].bis_hdl_info[j].aux_hdl);
                }
            }
        }
#endif
        //释放互斥量
        app_broadcast_mutex_post(&mutex, __LINE__);
        break;

    case BIG_EVENT_PERIODIC_DISCONNECT:
    case BIG_EVENT_RECEIVER_DISCONNECT:
        g_printf("BIG_EVENT_RECEIVER_DISCONNECT");
        //由于是异步操作需要加互斥量保护，避免broadcast_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_broadcast_mutex_pend(&mutex, __LINE__);

        receiver_connected_status = 0;

        hdl = (big_hdl_t *)&event[1];

        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (app_big_hdl_info[i].used && (app_big_hdl_info[i].big_hdl == hdl->big_hdl)) {
                for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
#if TCFG_KBOX_1T3_MODE_EN
                    if (app_big_hdl_info[i].bis_hdl_info[j].bis_hdl == hdl->bis_hdl[0]) {
                        app_big_hdl_info[i].bis_hdl_info[j].bis_hdl = 0;
                        app_big_hdl_info[i].bis_hdl_info[j].aux_hdl = 0;
                        app_big_hdl_info[i].bis_hdl_info[j].remote_dev = 0;
                        find = 1;
                        break;
                    }
#else
                    app_big_hdl_info[i].bis_hdl_info[j].bis_hdl = 0;
                    app_big_hdl_info[i].bis_hdl_info[j].aux_hdl = 0;
                    app_big_hdl_info[i].bis_hdl_info[j].remote_dev = 0;
                    find = 1;
                    break;
#endif
                }
            }
            if (find) {
                break;
            }
        }

        if (!find) {
            //释放互斥量
            app_broadcast_mutex_post(&mutex, __LINE__);
            break;
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_enable();   // 恢复自动关机
#endif

        g_printf("big_hdl:0x%x, bis_hdl:0x%x, aux_hdl:0x%x", hdl->big_hdl, hdl->bis_hdl[0], hdl->aux_hdl);

#if TCFG_KBOX_1T3_MODE_EN
        bis_connected_nums--;
        ASSERT(bis_connected_nums <= BIG_MAX_BIS_NUMS && bis_connected_nums >= 0, "bis_connected_nums:%d", bis_connected_nums);
#endif

        ret = broadcast_receiver_disconnect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("broadcast_receiver_disconnect_deal fail");
        }

        if (!bis_connected_nums) {
            //关闭VM写RAM
            /* vm_ram_direct_2_flash_switch(TRUE); */

            //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
            if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
                vm_flush2flash(1);
            }
        }

        //释放互斥量
        app_broadcast_mutex_post(&mutex, __LINE__);
        break;

    case BIG_EVENT_CUSTOM_DATA_SYNC:
        g_printf("BIG_EVENT_PADV_DATA_SYNC");
        broadcast_padv_data_deal((void *)&event[1]);
        break;

    default:
        result = -ESRCH;
        break;
    }

    return result;
}
APP_MSG_PROB_HANDLER(app_le_broadcast_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_BIG,
    .handler = app_broadcast_conn_status_event_handler,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前是否在退出模式的状态
 *
 * @return 1；是，0：否
 */
/* ----------------------------------------------------------------------------*/
u8 get_broadcast_app_mode_exit_flag(void)
{
    return broadcast_app_mode_exit;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 判断当前设备作为广播发送设备还是广播接收设备
 *
 * @return true:发送设备，false:接收设备
 */
/* ----------------------------------------------------------------------------*/
static bool is_broadcast_as_transmitter()
{
    struct app_mode *cur_mode = app_get_current_mode();

#if (TCFG_BT_BACKGROUND_ENABLE)
    //如果能量检测中则等待能量检测完成再触发做发送的流程，避免重复打开数据流
    u8 addr[6];
    if (cur_mode->name == APP_MODE_BT && bt_slience_get_detect_addr(addr)) {
        return false;
    }
#endif

#if (LEA_BIG_FIX_ROLE == 1)
    return true;
#elif (LEA_BIG_FIX_ROLE == 2)
    return false;
#endif

    //当前处于蓝牙模式并且已连接手机设备时，
    //(1)播歌作为广播发送设备；
    //(2)暂停作为广播接收设备。
    if ((cur_mode->name == APP_MODE_BT) &&
        (bt_get_connect_status() != BT_STATUS_WAITINT_CONN)) {
        if ((bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING &&  bt_get_connect_status() == BT_STATUS_PLAYING_MUSIC) ||
            get_a2dp_decoder_status() ||
            a2dp_player_runing()) {
            return true;
        } else {
            return false;
        }
    }

#if TCFG_APP_LINEIN_EN
    if (cur_mode->name == APP_MODE_LINEIN)  {
        if (linein_get_status() || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_IIS_EN
    if (cur_mode->name == APP_MODE_IIS)  {
        if (iis_get_status() || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_MIC_EN
    if (cur_mode->name == APP_MODE_MIC)  {
        if (mic_get_status() || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_MUSIC_EN
    if (cur_mode->name == APP_MODE_MUSIC) {
        if ((music_file_get_player_status(get_music_file_player()) == FILE_PLAYER_START) || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_FM_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_FM)  {
        if (fm_get_fm_dev_mute() == 0 || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_SPDIF_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_SPDIF) {
        //由于spdif 是先打开数据源然后再打开数据流的顺序，具有一定的滞后性，所以不能用 函数 spdif_player_runing 函数作为判断依据
        /* if (spdif_player_runing() ||  config_broadcast_as_master) { */
        if (!get_spdif_mute_state()) {
            y_printf("spdif_player_runing?\n");
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_PC_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_PC) {
#if defined(TCFG_USB_SLAVE_AUDIO_SPK_ENABLE) && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        if (pc_get_status() || config_broadcast_as_master) {
            return true;
        } else {
            return false;
        }
#else
        return false;
#endif
    }
#endif

    return false;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 检测广播当前是否处于挂起状态
 *
 * @return true:处于挂起状态，false:处于非挂起状态
 */
/* ----------------------------------------------------------------------------*/
static bool is_need_resume_broadcast()
{
    u8 i;
    u8 find = 0;
    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (app_big_hdl_info[i].big_status == APP_BROADCAST_STATUS_SUSPEND) {
            find = 1;
            break;
        }
    }
    if (find) {
        return true;
    } else {
        return false;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播从挂起状态恢复
 */
/* ----------------------------------------------------------------------------*/
static void app_broadcast_resume()
{
    big_parameter_t *params;

    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!is_need_resume_broadcast()) {
        return;
    }

    app_broadcast_open();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播进入挂起状态
 */
/* ----------------------------------------------------------------------------*/
static void app_broadcast_suspend()
{
    u8 i;
    u8 find = 0;
    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (app_big_hdl_info[i].used) {
            find = 1;
            break;
        }
    }
    if (find) {
        broadcast_last_role = get_broadcast_role();
        app_broadcast_close(APP_BROADCAST_STATUS_SUSPEND);
    }
}



static void app_broadcast_retry_open(void *priv)
{
    u32 role = (u32)priv;

    if (role) {
        app_broadcast_open_with_role(role - 1);
    } else {
        app_broadcast_open();
    }


}
/* --------------------------------------------------------------------------*/
/**
 * @brief 开启广播
 *
 * @return >=0:success
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_open()
{
    u8 i;
    u8 big_available_num = 0;
    int temp_broadcast_hdl = 0;
    big_parameter_t *params;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_broadcast_init_flag) {
        return -EPERM;
    }

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (!app_big_hdl_info[i].used) {
            big_available_num++;
        }
    }

    if (!big_available_num) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }


    bis_switch_onoff = 1;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    ble_module_enable(0);
    if (bt_rcsp_ble_conn_num() > 0) {
        rcsp_connect_dev_detect_timer = sys_timeout_add((void *)0, app_broadcast_retry_open, 250); //由于非标准广播使用私有hci事件回调所以需要等RCSP断连事件处理完后才能开广播
        return -EPERM;
    } else {
        rcsp_connect_dev_detect_timer = 0;
    }
#endif
    log_info("broadcast_open");

    app_broadcast_mutex_pend(&mutex, __LINE__);
#if TCFG_KBOX_1T3_MODE_EN
    le_audio_ops_register(APP_MODE_NULL);
#endif

    if (is_broadcast_as_transmitter()) {
        //初始化广播发送端参数
        params = set_big_params(mode->name, BROADCAST_ROLE_TRANSMITTER, 0);

        //打开big，打开成功后会在函数app_broadcast_conn_status_event_handler做后续处理
        temp_broadcast_hdl = broadcast_transmitter(params);
#if TRANSMITTER_AUTO_TEST_EN
        //不定时切换模式
        wireless_trans_auto_test3_init();
        //不定时暂停播放
        wireless_trans_auto_test4_init();
#endif
    } else {
        //初始化广播接收端参数
        params = set_big_params(mode->name, BROADCAST_ROLE_RECEIVER, 0);

        //打开big，打开成功后会在函数app_broadcast_conn_status_event_handler做后续处理
        temp_broadcast_hdl = broadcast_receiver(params);
#if RECEIVER_AUTO_TEST_EN
        //不定时切换模式
        wireless_trans_auto_test3_init();
        //不定时暂停播放
        wireless_trans_auto_test4_init();
#endif
    }
    if (temp_broadcast_hdl >= 0) {
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (!app_big_hdl_info[i].used) {
                app_big_hdl_info[i].big_hdl = temp_broadcast_hdl;
                app_big_hdl_info[i].big_status = APP_BROADCAST_STATUS_START;
                app_big_hdl_info[i].used = 1;
                break;
            }
        }
    }

    app_broadcast_mutex_post(&mutex, __LINE__);
    return temp_broadcast_hdl;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 固定角色（0：接收端，1：发射端）开启广播
 *
 * @return >=0:success
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_open_with_role(u8 role)
{
    u8 i;
    u8 big_available_num = 0;
    int temp_broadcast_hdl = 0;
    big_parameter_t *params;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_broadcast_init_flag) {
        return -EPERM;
    }

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (!app_big_hdl_info[i].used) {
            big_available_num++;
        }
    }

    if (!big_available_num) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    bis_switch_onoff = 1;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    ble_module_enable(0);
    if (bt_rcsp_ble_conn_num() > 0) {
        u32 temp_role = role + 1;
        rcsp_connect_dev_detect_timer = sys_timeout_add((void *)temp_role, app_broadcast_retry_open, 250); //由于非标准广播使用私有hci事件回调所以需要等RCSP断连事件处理完后才能开广播
        return -EPERM;
    } else {
        rcsp_connect_dev_detect_timer = 0;
    }
#endif

    log_info("broadcast_open_with_role %d", role);

    app_broadcast_mutex_pend(&mutex, __LINE__);
#if TCFG_KBOX_1T3_MODE_EN
    le_audio_ops_register(APP_MODE_NULL);
#endif

    if (role) {
        //初始化广播发送端参数
        params = set_big_params(mode->name, BROADCAST_ROLE_TRANSMITTER, 0);

        //打开big，打开成功后会在函数app_broadcast_conn_status_event_handler做后续处理
        temp_broadcast_hdl = broadcast_transmitter(params);
#if TRANSMITTER_AUTO_TEST_EN
        //不定时切换模式
        wireless_trans_auto_test3_init();
        //不定时暂停播放
        wireless_trans_auto_test4_init();
#endif
    } else {
        //初始化广播接收端参数
        params = set_big_params(mode->name, BROADCAST_ROLE_RECEIVER, 0);

        //打开big，打开成功后会在函数app_broadcast_conn_status_event_handler做后续处理
        temp_broadcast_hdl = broadcast_receiver(params);
#if RECEIVER_AUTO_TEST_EN
        //不定时切换模式
        wireless_trans_auto_test3_init();
        //不定时暂停播放
        wireless_trans_auto_test4_init();
#endif
    }
    if (temp_broadcast_hdl >= 0) {
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (!app_big_hdl_info[i].used) {
                app_big_hdl_info[i].big_hdl = temp_broadcast_hdl;
                app_big_hdl_info[i].big_status = APP_BROADCAST_STATUS_START;
                app_big_hdl_info[i].used = 1;
                break;
            }
        }
    }

    app_broadcast_mutex_post(&mutex, __LINE__);

    return temp_broadcast_hdl;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭广播
 *
 * @param status:挂起还是停止
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_close(u8 status)
{
    u8 i;
    struct app_mode *mode;
    int ret = 0;

    if (!app_broadcast_init_flag) {
        return -EPERM;
    }

    log_info("broadcast_close");

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if (rcsp_connect_dev_detect_timer) {
        sys_timeout_del(rcsp_connect_dev_detect_timer);
    }
#endif
    //由于是异步操作需要加互斥量保护，避免和开启开广播的流程同时运行,添加的流程请放在互斥量保护区里面
    app_broadcast_mutex_pend(&mutex, __LINE__);

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (app_big_hdl_info[i].used && app_big_hdl_info[i].big_hdl) {
            ret = broadcast_close(app_big_hdl_info[i].big_hdl);
            memset(&app_big_hdl_info[i], 0, sizeof(struct app_big_hdl_t));
            app_big_hdl_info[i].big_status = status;
        }
    }

    receiver_connected_status = 0;
    bis_connected_nums = 0;

#if TCFG_AUTO_SHUT_DOWN_TIME
    sys_auto_shut_down_enable();   // 恢复自动关机
#endif

    //关闭VM写RAM
    /* vm_ram_direct_2_flash_switch(TRUE); */

    //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
    //if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
    //    vm_flush2flash(1);
    //}

    //释放互斥量
    app_broadcast_mutex_post(&mutex, __LINE__);

    bis_switch_onoff = 0;

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if (status != APP_BROADCAST_STATUS_SUSPEND) {
        ll_set_private_access_addr_pair_channel(0);
        ble_module_enable(1);
    }
#endif

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播开关切换
 *
 * @return 0：操作成功
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_switch(void)
{
    u8 i;
    u8 find = 0;
    big_parameter_t *params;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {


        return -EPERM;
    }

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (app_big_hdl_info[i].used) {
            find = 1;
            break;
        }
    }

    if (!tone_player_runing()) {
        if (find) {
            bt_work_mode_select(g_bt_hdl.last_work_mode);
            play_tone_file_alone_callback(get_tone_files()->le_broadcast_close,
                                          (void *)TONE_INDEX_BROADCAST_CLOSE,
                                          broadcast_tone_play_end_callback);
        } else {

            if (g_bt_hdl.work_mode !=  BT_MODE_BROADCAST) {
                bt_work_mode_select(BT_MODE_BROADCAST);
                play_tone_file_alone_callback(get_tone_files()->le_broadcast_open,
                                              (void *)TONE_INDEX_BROADCAST_OPEN,
                                              broadcast_tone_play_end_callback);

            } else if (app_broadcast_open() >= 0) {
                play_tone_file_alone_callback(get_tone_files()->le_broadcast_open,
                                              (void *)TONE_INDEX_BROADCAST_OPEN,
                                              broadcast_tone_play_end_callback);
            }
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 更新系统当前处于的场景
 *
 * @param scene:当前系统状态
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int update_app_broadcast_deal_scene(int scene)
{
    cur_deal_scene = scene;
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播开启情况下，不同场景的处理流程
 *
 * @param scene:当前系统状态
 *
 * @return ret < 0:无需处理，ret == 0:处理事件但不拦截后续流程，ret > 0:处理事件并拦截后续流程
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_deal(int scene)
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));

    if (g_bt_hdl.work_mode != BT_MODE_BROADCAST) {
        return -EPERM;
    }
    u8 i;
    int ret = 0;
    static u8 phone_start_cnt = 0;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    if ((cur_deal_scene == scene) &&
        (scene != LE_AUDIO_PHONE_START) &&
        (scene != LE_AUDIO_PHONE_STOP)) {
        log_error("app_broadcast_deal,scene not be modified:%d", scene);
        return -EPERM;
    }

    g_printf("app_broadcast_deal, rets_addr:0x%x", rets_addr);

#if TCFG_KBOX_1T3_MODE_EN
    if ((scene != LE_AUDIO_PHONE_START) && (scene != LE_AUDIO_PHONE_STOP)) {
        log_error("app_broadcast_deal,cur scene Operation not permitted:%d", scene);
        return -EPERM;
    }
#endif

    cur_deal_scene = scene;

    switch (scene) {
    case LE_AUDIO_APP_MODE_ENTER:
        log_info("LE_AUDIO_APP_MODE_ENTER");
        //进入当前模式
        broadcast_app_mode_exit = 0;
    case LE_AUDIO_APP_OPEN:
        config_broadcast_as_master = 1;
        mode = app_get_current_mode();
        if (mode) {
            le_audio_ops_register(mode->name);
        }
        if (is_need_resume_broadcast()) {
            app_broadcast_resume();
            ret = 1;
        }
        config_broadcast_as_master = 0;
        break;

    case LE_AUDIO_APP_MODE_EXIT:
        log_info("LE_AUDIO_APP_MODE_EXIT");
        //退出当前模式
        broadcast_app_mode_exit = 1;
    case LE_AUDIO_APP_CLOSE:
        phone_start_cnt = 0;
        app_broadcast_suspend();
        le_audio_ops_unregister();
        break;

    case LE_AUDIO_MUSIC_START:
    case LE_AUDIO_A2DP_START:
        log_info("LE_AUDIO_MUSIC_START");
        //启动a2dp播放
        if (broadcast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现LE_AUDIO_APP_MODE_EXIT，再出现LE_AUDIO_A2DP_START，导致广播状态发生改变
            break;
        }
#if (LEA_BIG_FIX_ROLE == 0)
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            if (app_big_hdl_info[i].used && (app_big_hdl_info[i].big_status == APP_BROADCAST_STATUS_START)) {
                //(1)当处于广播开启并且作为接收设备时，挂起广播，播放当前手机音乐；
                //(2)当前广播处于挂起状态时，恢复广播并作为发送设备。
                if (get_broadcast_role() == BROADCAST_ROLE_RECEIVER) {
                    app_broadcast_suspend();
                } else if (get_broadcast_role() == BROADCAST_ROLE_TRANSMITTER) {
                    ret = 1;
                }
                break;
            }
        }
#else
        if (get_broadcast_role() == BROADCAST_ROLE_TRANSMITTER) {
            for (i = 0; i < BIG_MAX_NUMS; i++) {
                //固定收发角色重启广播数据流
                broadcast_audio_all_open(app_big_hdl_info[i].big_hdl);
            }
            ret = 1;
            break;
        }
#endif

#if TCFG_BT_VOL_SYNC_ENABLE
        mode = app_get_current_mode();
        if (mode && (mode->name == APP_MODE_BT)) {
            set_music_device_volume(get_music_sync_volume());
        }
#endif

        if (is_need_resume_broadcast()) {
            /* app_broadcast_resume(); */
            app_broadcast_open_with_role(1);
            ret = 1;
        }
        break;

    case LE_AUDIO_MUSIC_STOP:
    case LE_AUDIO_A2DP_STOP:
        log_info("LE_AUDIO_MUSIC_STOP");
        //停止a2dp播放
        if (broadcast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现LE_AUDIO_APP_MODE_EXIT，再出现LE_AUDIO_A2DP_STOP，导致广播状态发生改变
            break;
        }
#if (LEA_BIG_FIX_ROLE == 0)
        //当前处于广播挂起状态时，停止手机播放，恢复广播并接收其他设备的音频数据
        app_broadcast_suspend();
#else
        if (get_broadcast_role() == BROADCAST_ROLE_TRANSMITTER) {
            for (i = 0; i < BIG_MAX_NUMS; i++) {
                //固定收发角色暂停播放时关闭广播数据流
                broadcast_audio_all_close(app_big_hdl_info[i].big_hdl);
            }
            ret = 1;
            break;
        }
#endif
        if (is_need_resume_broadcast()) {
            /* app_broadcast_resume(); */
            app_broadcast_open_with_role(0);
            ret = 1;
        }
        break;

    case LE_AUDIO_PHONE_START:
        log_info("LE_AUDIO_PHONE_START");
        //通话时，挂起广播
        phone_start_cnt++;
        printf("===phone_start_cnt:%d===\n", phone_start_cnt);
        app_broadcast_suspend();
        break;

    case LE_AUDIO_PHONE_STOP:
        log_info("LE_AUDIO_PHONE_STOP");
        //通话结束恢复广播
        phone_start_cnt--;
        printf("===phone_start_cnt:%d===\n", phone_start_cnt);
        if (phone_start_cnt) {
            log_info("phone_start_cnt:%d", phone_start_cnt);
            break;
        }
        mode = app_get_current_mode();
        //当前处于蓝牙模式并且挂起前广播作为发送设备，恢复广播的操作在播放a2dp处执行
        if (mode && (mode->name == APP_MODE_BT)) {
            if (broadcast_last_role == BROADCAST_ROLE_TRANSMITTER) {
            }
        }
        //当前处于蓝牙模式并且挂起前广播，恢复广播并作为接收设备
        if (is_need_resume_broadcast()) {
            /* app_broadcast_resume(); */
            app_broadcast_open_with_role(0);
            ret = 1;
        }
        break;

    case LE_AUDIO_EDR_DISCONN:
        log_info("LE_AUDIO_EDR_DISCONN");
        if (broadcast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现LE_AUDIO_APP_MODE_EXIT，再出现LE_AUDIO_EDR_DISCONN，导致广播状态发生改变
            break;
        }
        //当经典蓝牙断开后，作为发送端的广播设备挂起广播
        if (get_broadcast_role() == BROADCAST_ROLE_TRANSMITTER) {
            app_broadcast_suspend();
        }
        if (is_need_resume_broadcast()) {
            /* app_broadcast_resume(); */
            app_broadcast_open_with_role(0);
            ret = 1;
        }
        break;

    default:
        log_error("%s invalid operation\n", __FUNCTION__);
        ret = -ESRCH;
        break;
    }

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 初始化同步的状态数据的内容
 */
/* ----------------------------------------------------------------------------*/
static void app_broadcast_sync_data_init(void)
{
    app_broadcast_data_sync.volume = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    broadcast_sync_data_init(&app_broadcast_data_sync);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 更新广播同步状态的数据
 *
 * @param type:更新项
 * @param value:更新值
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int update_broadcast_sync_data(u8 type, int value)
{
    u8 i;
    switch (type) {
    case BROADCAST_SYNC_VOL:
        if (app_broadcast_data_sync.volume == value) {
            return 0;
        }
        app_broadcast_data_sync.volume = value;
        break;
    default:
        return -ESRCH;
    }

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        broadcast_set_sync_data(app_big_hdl_info[i].big_hdl, &app_broadcast_data_sync, sizeof(struct broadcast_sync_info));
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 接收到广播发送端的同步数据，并更新本地配置
 *
 * @param priv:接收到的同步数据
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int broadcast_padv_data_deal(void *priv)
{
    struct broadcast_sync_info *sync_data = (struct broadcast_sync_info *)priv;
    if (app_broadcast_data_sync.volume != sync_data->volume) {
        app_broadcast_data_sync.volume = sync_data->volume;
#if (LEA_BIG_VOL_SYNC_EN && (!TCFG_KBOX_1T3_MODE_EN))
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, app_broadcast_data_sync.volume, 1);
        /* y_printf("----------> vol:%d\n", app_broadcast_data_sync.volume); */
        app_send_message(APP_MSG_VOL_CHANGED, app_broadcast_data_sync.volume);
#endif
    }
    memcpy(&app_broadcast_data_sync, sync_data, sizeof(struct broadcast_sync_info));
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播资源初始化
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_init(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (app_broadcast_init_flag) {
        return;
    }

    int os_ret = os_mutex_create(&mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
        ASSERT(0);
    }

    broadcast_init();
    app_broadcast_sync_data_init();
    app_broadcast_init_flag = 1;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播资源卸载
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_uninit(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!app_broadcast_init_flag) {
        return;
    }

    app_broadcast_init_flag = 0;

    int os_ret = os_mutex_del(&mutex, OS_DEL_NO_PEND);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
    }

    broadcast_uninit();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播接收端配对事件回调
 *
 * @param event：配对事件
 * @param priv：事件附带的参数
 */
/* ----------------------------------------------------------------------------*/
static void broadcast_pair_rx_event_callback(const PAIR_EVENT event, void *priv)
{
    switch (event) {
    case PAIR_EVENT_RX_PRI_CHANNEL_CREATE_SUCCESS:
        u32 *private_connect_access_addr = (u32 *)priv;
        g_printf("PAIR_EVENT_RX_PRI_CHANNEL_CREATE_SUCCESS:0x%x", *private_connect_access_addr);
        int ret = syscfg_write(VM_WIRELESS_PAIR_CODE0, private_connect_access_addr, sizeof(u32));
        if (ret <= 0) {
            r_printf(">>>>>>wireless pair code save err");
        }
        break;

    case PAIR_EVENT_RX_OPEN_PAIR_MODE_SUCCESS:
        g_printf("PAIR_EVENT_RX_OPEN_PAIR_MODE_SUCCESS");
        break;

    case PAIR_EVENT_RX_CLOSE_PAIR_MODE_SUCCESS:
        g_printf("PAIR_EVENT_RX_CLOSE_PAIR_MODE_SUCCESS");
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播发送端配对事件回调
 *
 * @param event：配对事件
 * @param priv：事件附带的参数
 */
/* ----------------------------------------------------------------------------*/
static void broadcast_pair_tx_event_callback(const PAIR_EVENT event, void *priv)
{
    switch (event) {
    case PAIR_EVENT_TX_PRI_CHANNEL_CREATE_SUCCESS:
        u32 *private_connect_access_addr = (u32 *)priv;
        g_printf("PAIR_EVENT_TX_PRI_CHANNEL_CREATE_SUCCESS:0x%x", *private_connect_access_addr);
        int ret = syscfg_write(VM_WIRELESS_PAIR_CODE0, private_connect_access_addr, sizeof(u32));
        if (ret <= 0) {
            r_printf(">>>>>>wireless pair code save err");
        }
        break;

    case PAIR_EVENT_TX_OPEN_PAIR_MODE_SUCCESS:
        g_printf("PAIR_EVENT_TX_OPEN_PAIR_MODE_SUCCESS");
        break;

    case PAIR_EVENT_TX_CLOSE_PAIR_MODE_SUCCESS:
        g_printf("PAIR_EVENT_TX_CLOSE_PAIR_MODE_SUCCESS");
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播进入配对模式
 *
 * @param role：广播角色接收端还是发送端
 * @param mode：0-广播配对，1-连接配对
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_enter_pair(u8 role, u8 mode)
{
    int ret;
    u32 private_connect_access_addr = 0;;

    app_broadcast_close(APP_BROADCAST_STATUS_STOP);

    ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &private_connect_access_addr, sizeof(u32));
    if (role == BROADCAST_ROLE_UNKNOW) {
        if (is_broadcast_as_transmitter()) {
            broadcast_enter_pair(BROADCAST_ROLE_TRANSMITTER, mode, (void *)&pair_tx_cb, private_connect_access_addr);
        } else {
            broadcast_enter_pair(BROADCAST_ROLE_RECEIVER, mode, (void *)&pair_rx_cb, private_connect_access_addr);
        }
    } else if (role == BROADCAST_ROLE_TRANSMITTER) {
        broadcast_enter_pair(role, mode, (void *)&pair_tx_cb, private_connect_access_addr);
    } else if (role == BROADCAST_ROLE_RECEIVER) {
        broadcast_enter_pair(role, mode, (void *)&pair_rx_cb, private_connect_access_addr);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 退出广播配对模式
 *
 * @param role：广播角色接收端还是发送端
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_exit_pair(u8 role)
{
    if (role == BROADCAST_ROLE_UNKNOW) {
        if (is_broadcast_as_transmitter()) {
            broadcast_exit_pair(BROADCAST_ROLE_TRANSMITTER);
        } else {
            broadcast_exit_pair(BROADCAST_ROLE_RECEIVER);
        }
    } else {
        broadcast_exit_pair(role);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 非蓝牙后台情况下，在其他音源模式开启BIG，前提要先开蓝牙协议栈
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_open_in_other_mode()
{
    if (g_bt_hdl.work_mode != BT_MODE_BROADCAST) {
        return;
    }
    app_broadcast_init();
#if TCFG_KBOX_1T3_MODE_EN
    app_broadcast_open();
#endif
    if (is_need_resume_broadcast()) {
        struct app_mode *mode = app_get_current_mode();
        if (mode) {
            le_audio_ops_register(mode->name);
        }
        //下面的代码，会导致在关闭蓝牙后台后，切模式时会提前打开广播
        /* app_broadcast_resume(); */
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 非蓝牙后台情况下，在其他音源模式关闭BIG
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_close_in_other_mode()
{
    if (g_bt_hdl.work_mode != BT_MODE_BROADCAST) {
        return;
    }
#if TCFG_KBOX_1T3_MODE_EN
    app_broadcast_close(APP_BROADCAST_STATUS_STOP);
#else
    app_broadcast_suspend();
#endif
    app_broadcast_uninit();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 自定义数据发送接口
 *
 * @param device:发送给远端设备的标识
 * @param data:需要发送的数据
 * @param length:数据长度
 *
 * @return slen:实际发送数据的总长度
 */
/* ----------------------------------------------------------------------------*/
int app_broadcast_send_custom_data(u8 device, void *data, size_t length)
{
    u8 i, j;
    int slen = 0;
    struct wireless_data_callback_func *p;

    for (i = 0; i < BIG_MAX_NUMS; i++) {
        if (app_big_hdl_info[i].used) {
            for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                if (app_big_hdl_info[i].bis_hdl_info[j].remote_dev & device) {
                    slen += broadcast_send_custom_data(app_big_hdl_info[i].bis_hdl_info[j].aux_hdl, data, length);
                }
            }
        }
    }

    if (slen) {
        list_for_each_wireless_data_callback(p) {
            if (p->tx_events_suss) {
                if (p->tx_events_suss(data, length)) {
                    break;
                }
            }
        }
    }

    return slen;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 将先前记录的信息同步给新加入组队的设备，默认新设备连上后调用
 */
/* ----------------------------------------------------------------------------*/
static void remote_dev_msg_status_sync_info_send(u16 acl_hdl)
{
#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
    if (!save_sync_status_table[0][0]) {
        return;
    }
    int table_size_h = (sizeof(save_sync_status_table) / sizeof(save_sync_status_table[0]));
    int table_size_l = (sizeof(save_sync_status_table[0]) / sizeof(save_sync_status_table[0][0]));
    for (u8 i = 0, j; i < BIG_MAX_NUMS; i++) {
        for (u8 j = 0; j < BIG_MAX_BIS_NUMS; j++) {
            if (app_big_hdl_info[i].bis_hdl_info[j].aux_hdl == acl_hdl) {
                for (u8 k = 0; k < table_size_h; k++) {
                    if (save_sync_status_table[k][0]) {
                        wireless_custom_data_send_to_sibling('M', save_sync_status_table[k], table_size_l, app_big_hdl_info[i].bis_hdl_info[j].remote_dev);
                    }

                }
            }
        }
    }
#endif
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 保存需要同步给新加入组队的远端设备的变量
 */
/* ----------------------------------------------------------------------------*/
static void remote_dev_msg_status_sync_info_save(u8 *data, u32 len)
{
#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
    int table_size_h = (sizeof(save_sync_status_table) / sizeof(save_sync_status_table[0]));
    int k = 0;
    for (int i = 0; i < table_size_h; i++) { //控制行
        if (save_sync_status_table[i][0] == data[0]) {
            save_sync_status_table[i][1]  = data[1];
            break;
        } else if (!save_sync_status_table[i][0]) {
            save_sync_status_table[i][0] = data[0];
            save_sync_status_table[i][1]  = data[1];
            break;
        }
    }
#endif
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 转发某个从机的消息给所有从机，用于同步所有从机消息和状态
 *
 * @param acl_hdl:对应特定远端设备的句柄
 * @param arg_num:参数个数
 * @param argv:首个参数地址
 */
/* ----------------------------------------------------------------------------*/
static void remote_dev_msg_status_sync_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    u8 i, j;
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];

    if (trans_role == WIRELESS_SYNC_CALL_RX) {
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                if (app_big_hdl_info[i].bis_hdl_info[j].aux_hdl != acl_hdl) {
                    wireless_custom_data_send_to_sibling('M', data, len, app_big_hdl_info[i].bis_hdl_info[j].remote_dev);
                }
            }
        }
        //记录信息，便于同步给新加入组队的设备
        remote_dev_msg_status_sync_info_save(data, len);
    }

    //由于转发流程中申请了内存，因此执行完毕后必须释放
    if (data) {
        free(data);
    }
}
WIRELESS_CUSTOM_DATA_STUB_REGISTER(msg_status_sync) = {
    .uuid = 'M',
    .task_name = "app_core",
    .func = remote_dev_msg_status_sync_handler,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 音量同步
 */
/* ----------------------------------------------------------------------------*/
void app_broadcast_sync_volume(u8 remote_dev_identification)
{
    u8 data[2];
    data[0] = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    data[1] = app_audio_get_volume(APP_AUDIO_STATE_CALL);
    /* g_printf("music_vol_sync: %d, call_vol_sync: %d\n", data[0], data[1]); */
    //同步音量
    wireless_custom_data_send_to_sibling('V', data, sizeof(data), remote_dev_identification);
}

static void volume_sync_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    u8 i, j;
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];
    if (trans_role == WIRELESS_SYNC_CALL_RX) {
        for (i = 0; i < BIG_MAX_NUMS; i++) {
            for (j = 0; j < BIG_MAX_BIS_NUMS; j++) {
                if (app_big_hdl_info[i].bis_hdl_info[j].aux_hdl == acl_hdl) {
                    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, data[0], 1);
                    app_audio_set_volume(APP_AUDIO_STATE_CALL, data[1], 1);
                    /* r_printf("music_vol_sync: %d, call_vol_sync: %d\n", data[0], data[1]); */
                    break;
                }
            }
        }
    }
    //由于转发流程中申请了内存，因此执行完毕后必须释放
    if (data) {
        free(data);
    }
}

WIRELESS_CUSTOM_DATA_STUB_REGISTER(volume_sync) = {
    .uuid = 'V',
    .task_name = "app_core",
    .func = volume_sync_handler,
};

static void remote_dev_cmd_status_sync_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    u8 i, j;
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];
    if (trans_role == WIRELESS_SYNC_CALL_RX) {
#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
        if (data[0] == 0xAA) {
            switch (data[1]) {    //共用雅轩话筒 消原音开关 和话筒开关反过来
            case CMD_MUSIC_PP:
                app_send_message(APP_MSG_MUSIC_PP, app_get_current_mode()->name);
                break;
            case CMD_MUSIC_PREV:
                app_send_message(APP_MSG_MUSIC_PREV, app_get_current_mode()->name);
                break;
            case CMD_MUSIC_NEXT:
                app_send_message(APP_MSG_MUSIC_NEXT, app_get_current_mode()->name);
                break;
            case CMD_EFFECT_MODE_CLUB:
                play_tone_file_alone(get_tone_files()->club_mode);
                break;
            case CMD_EFFECT_MODE_COMPERE:
                play_tone_file_alone(get_tone_files()->compere_mode);
                break;
            case CMD_VOCAL_REMOVE:
                app_send_message(APP_MSG_VOCAL_REMOVE, app_get_current_mode()->name);
                break;
            case CMD_EFFECT_MODE_MONSTER:
                play_tone_file_alone(get_tone_files()->monster_mode);
                break;
            case CMD_EFFECT_MODE_ORIGINAL:
                play_tone_file_alone(get_tone_files()->original_mode);
                break;
            case CMD_EFFECT_MODE_KTV:
                play_tone_file_alone(get_tone_files()->ktv_mode);
                break;

#if 0
            case CMD_NOISEGATE_SW:
                extern int jlsp_nsgate_flg;
                jlsp_nsgate_flg ^= 1;
                if (jlsp_nsgate_flg) {
                    play_tone_file_alone(get_tone_files()->num[1]);
                } else {
                    play_tone_file_alone(get_tone_files()->num[0]);
                }
                break;
#endif
            default:
                printf("******command error******\n");
                break;
            }
        }
#endif
    }

    //由于转发流程中申请了内存，因此执行完毕后必须释放
    if (data) {
        free(data);
    }

}

WIRELESS_CUSTOM_DATA_STUB_REGISTER(cmd_status_sync) = {
    .uuid = 'C',
    .task_name = "app_core",
    .func = remote_dev_cmd_status_sync_handler,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取bis连接数量
 *
 * @return bis连接数量
 */
/* ----------------------------------------------------------------------------*/
u8 get_bis_connected_num(void)
{
    return bis_connected_nums;
}


u8 get_bis_switch_onoff(void)
{
    return bis_switch_onoff;
}

#endif

