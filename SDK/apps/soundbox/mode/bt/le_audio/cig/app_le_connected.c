/*********************************************************************************************
    *   Filename        : app_connected.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-15 14:30

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "cig.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "app_config.h"
#include "app_tone.h"
#include "btstack/avctp_user.h"
#include "audio_config.h"
#include "tone_player.h"
#include "app_main.h"
#include "ble_rcsp_server.h"
#include "wireless_trans.h"
#include "mic_effect.h"
#include "bt_common.h"
#include "user_cfg.h"
#include "btstack/bluetooth.h"
/* #include "earphone.h" */
#include "btstack/le/ble_api.h"
#if TCFG_USER_TWS_ENABLE
#include "classic/tws_api.h"
extern void tws_dual_conn_state_handler();
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
#include "ble_rcsp_server.h"
#include "btstack_rcsp_user.h"
#endif

#if LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG             "[APP_LE_CONNECTED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define CIG_CONNECTED_TIMEOUT   0//(10 * 1000L)  //单位:ms

/**************************************************************************************************
  Data Types
**************************************************************************************************/
enum {
    ///0x1000起始为了不要跟提示音的IDEX_TONE_重叠了
    TONE_INDEX_CONNECTED_OPEN = 0x1002,
    TONE_INDEX_CONNECTED_CLOSE,
};

struct app_cis_conn_info {
    u8 cis_status;
    u8 remote_dev_identification;
    u16 cis_hdl;
    u16 acl_hdl;
    u16 Max_PDU_C_To_P;
    u16 Max_PDU_P_To_C;
};

struct app_cig_conn_info {
    u8 used;
    u8 cig_hdl;
    u8 cig_status;
    u8 local_dev_identification;
    struct app_cis_conn_info cis_conn_info[CIS_MAX_CONNECTABLE_NUMS];
};

/**************************************************************************************************
  Static Prototypes
**************************************************************************************************/
static bool is_connected_as_central();
static void app_cig_connection_timeout(void *priv);
static void remote_dev_msg_status_sync_info_send(u16 acl_hdl);

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
static OS_MUTEX mutex;
static u8 connected_last_role = 0; /*!< 挂起前CIG角色 */
static u8 connected_app_mode_exit = 0;  /*!< 音源模式退出标志 */
static u8 config_connected_as_master = 0;   /*!< 配置强制做发送设备 */
static u8 acl_connected_nums = 0;
static u8 cis_connected_nums = 0;
static u8 app_connected_init_flag = 0;
static struct app_cig_conn_info app_cig_conn_info[CIG_MAX_NUMS];
static int cig_connection_timeout = 0;
static int cur_deal_scene = -1; /*< 当前系统处于的运行场景 */

#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
static u8 save_sync_status_table[5][2] = {0};
#endif

static u8 cis_switch_onoff = 0;

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
static int rcsp_connect_dev_detect_timer = 0;
#endif
/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/* --------------------------------------------------------------------------*/
/**
 * @brief 申请互斥量，用于保护临界区代码，与app_connected_mutex_post成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_connected_mutex_pend(OS_MUTEX *mutex, u32 line)
{
    if (!app_connected_init_flag) {
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
 * @brief 释放互斥量，用于保护临界区代码，与app_connected_mutex_pend成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_connected_mutex_post(OS_MUTEX *mutex, u32 line)
{
    if (!app_connected_init_flag) {
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
 * @brief ACL数据接口回调，用于记录远端设备标识，与hdl形成映射关系
 *
 * @param acl_hdl:对应特定远端设备的句柄
 * @param arg_num:参数个数
 * @param argv:首个参数地址
 */
/* ----------------------------------------------------------------------------*/
static void identification_record_handler(u16 acl_hdl, u8 arg_num, int *argv)
{
    u8 i, j;
    int trans_role = argv[0];
    u8 *data = (u8 *)argv[1];
    int len = argv[2];
    if (trans_role == WIRELESS_SYNC_CALL_RX) {
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                if (app_cig_conn_info[i].cis_conn_info[j].acl_hdl == acl_hdl) {
                    memcpy(&app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification, data, len);
                    /* g_printf("remote_dev_identification:0x%x", app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification); */
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

/* WIRELESS_CUSTOM_DATA_STUB_REGISTER(identification_record) = { */
/*     .uuid = WIRELESS_IDTF_SYNC_FUNC_ID, */
/*     .task_name = "app_core", */
/*     .func = identification_record_handler, */
/* }; */

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG开关提示音结束回调接口
 *
 * @param priv:传递的参数
 * @param event:提示音回调事件
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int connected_tone_play_end_callback(void *priv, enum stream_event event)
{
    u32 index = (u32)priv;

    switch (event) {
    case STREAM_EVENT_NONE:
    case STREAM_EVENT_STOP:
        switch (index) {
        case TONE_INDEX_CONNECTED_OPEN:
            g_printf("TONE_INDEX_CONNECTED_OPEN");
            break;
        case TONE_INDEX_CONNECTED_CLOSE:
            g_printf("TONE_INDEX_CONNECTED_CLOSE");
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
 * @brief CIG连接状态事件处理函数
 *
 * @param msg:状态事件附带的返回参数
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int app_connected_conn_status_event_handler(int *msg)
{
    u8 i, j;
    u8 find = 0;
    static u8 local_dev_identification = 0;
    static u8 remote_dev_identification = 0;
    int ret = 0;
    u64 pair_addr0 = 0;
    u64 pair_addr1 = 0;
    cig_hdl_t *hdl;
    cis_acl_info_t *acl_info;
    struct wireless_data_callback_func *p;
    int *event = msg;
    int result = 0;

    if (!app_get_connected_role()) {
        return -EPERM;
    }

    switch (event[0]) {
    case CIG_EVENT_CENTRAL_CONNECT:
        g_printf("CIG_EVENT_CENTRAL_CONNECT");
        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);


        //vm写ram使能
        /* vm_ram_direct_2_flash_switch(FALSE); */

        hdl = (cig_hdl_t *)&event[1];

        //记录设备的cig_hdl等信息
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (app_cig_conn_info[i].used && (app_cig_conn_info[i].cig_hdl == hdl->cig_hdl)) {
                app_cig_conn_info[i].local_dev_identification = WIRELESS_MASTER_DEV;
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    if (!app_cig_conn_info[i].cis_conn_info[j].cis_hdl) {
                        app_cig_conn_info[i].cis_conn_info[j].cis_hdl = hdl->cis_hdl;
                        app_cig_conn_info[i].cis_conn_info[j].acl_hdl = hdl->acl_hdl;
                        app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification = WIRELESS_SLAVE_DEV1 << j;
                        g_printf("remote_dev_identification:0x%x", app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification);
                        app_cig_conn_info[i].cis_conn_info[j].cis_status = APP_CONNECTED_STATUS_CONNECT;
                        /* remote_dev_msg_status_sync_info_send(app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification); */
                        find = 1;
                        break;
                    }
                }
            }
        }

        if (!find) {
            //释放互斥量
            app_connected_mutex_post(&mutex, __LINE__);
            break;
        }

        cis_connected_nums++;
        ASSERT(cis_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && cis_connected_nums >= 0, "cis_connected_nums:%d", cis_connected_nums);

        //有设备连接上时删掉连接超时处理定时
        if (cig_connection_timeout) {
            sys_timeout_del(cig_connection_timeout);
            cig_connection_timeout = 0;
        }

        list_for_each_wireless_data_callback(p) {
            if (p->event_handler) {
                p->event_handler(event[0], (void *)hdl);
            }
        }

        ret = connected_central_connect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("connected_central_connect_deal fail");
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);
        break;

    case CIG_EVENT_CENTRAL_DISCONNECT:
        g_printf("CIG_EVENT_CENTRAL_DISCONNECT");
        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);

        hdl = (cig_hdl_t *)&event[1];

        list_for_each_wireless_data_callback(p) {
            if (p->event_handler) {
                p->event_handler(event[0], (void *)hdl);
            }
        }

        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (app_cig_conn_info[i].used && (app_cig_conn_info[i].cig_hdl == hdl->cig_hdl)) {
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    if (app_cig_conn_info[i].cis_conn_info[j].cis_hdl == hdl->cis_hdl) {
                        app_cig_conn_info[i].cis_conn_info[j].cis_hdl = 0;
                        app_cig_conn_info[i].cis_conn_info[j].acl_hdl = 0;
                        app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification = 0;
                        app_cig_conn_info[i].cis_conn_info[j].cis_status = APP_CONNECTED_STATUS_DISCONNECT;
                        break;
                    }
                }
                find = 1;
            }
        }

        if (!find) {
            //释放互斥量
            app_connected_mutex_post(&mutex, __LINE__);
            break;
        }

        cis_connected_nums--;
        ASSERT(cis_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && cis_connected_nums >= 0, "cis_connected_nums:%d", cis_connected_nums);

#if CIG_CONNECTED_TIMEOUT
        if (!cis_connected_nums && !cig_connection_timeout) {
            y_printf("app_cig_connection_timeout add:%d", __LINE__);
            cig_connection_timeout = sys_timeout_add(NULL, app_cig_connection_timeout, CIG_CONNECTED_TIMEOUT);
        }
#endif

        ret = connected_central_disconnect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("connected_central_disconnect_deal fail");
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        if (!cis_connected_nums) {
            sys_auto_shut_down_enable();   // 恢复自动关机
        }
#endif
        /* if (!cis_connected_nums) { */
        /*     //关闭VM写RAM */
        /*     vm_ram_direct_2_flash_switch(TRUE); */
        /*  */
        /*     //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash */
        /*     if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) { */
        /*         vm_flush2flash(0); */
        /*     } */
        /* } */

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);
        break;

    case CIG_EVENT_PERIP_CONNECT:
        g_printf("CIG_EVENT_PERIP_CONNECT");
        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);

        hdl = (cig_hdl_t *)&event[1];

        //记录设备的cig_hdl等信息
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (app_cig_conn_info[i].used && (app_cig_conn_info[i].cig_hdl == 0xFF)) {
                app_cig_conn_info[i].cig_hdl = hdl->cig_hdl;
                app_cig_conn_info[i].local_dev_identification = WIRELESS_SLAVE_DEV1;
                local_dev_identification = app_cig_conn_info[i].local_dev_identification;
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    if (!app_cig_conn_info[i].cis_conn_info[j].cis_hdl) {
                        app_cig_conn_info[i].cis_conn_info[j].cis_hdl = hdl->cis_hdl;
                        app_cig_conn_info[i].cis_conn_info[j].acl_hdl = hdl->acl_hdl;
                        app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification = WIRELESS_MASTER_DEV;
                        app_cig_conn_info[i].cis_conn_info[j].cis_status = APP_CONNECTED_STATUS_CONNECT;
                        remote_dev_identification = app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification;
                        app_cig_conn_info[i].cis_conn_info[j].Max_PDU_C_To_P = hdl->Max_PDU_C_To_P;
                        app_cig_conn_info[i].cis_conn_info[j].Max_PDU_P_To_C = hdl->Max_PDU_P_To_C;
                        find = 1;
                        break;
                    }
                }
            }
        }

        if (!find) {
            //释放互斥量
            app_connected_mutex_post(&mutex, __LINE__);
            break;
        }

        cis_connected_nums++;
        ASSERT(cis_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && cis_connected_nums >= 0, "cis_connected_nums:%d", cis_connected_nums);

        //有设备连接上时删掉连接超时处理定时
        if (cig_connection_timeout) {
            sys_timeout_del(cig_connection_timeout);
            cig_connection_timeout = 0;
        }

        list_for_each_wireless_data_callback(p) {
            if (p->event_handler) {
                p->event_handler(event[0], (void *)hdl);
            }
        }
        ret = connected_perip_connect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("connected_perip_connect_deal fail");
        } else {
            //告知主机本机的设备标识
            wireless_custom_data_send_to_sibling('I', &local_dev_identification, sizeof(local_dev_identification), remote_dev_identification);
        }

#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

#if TCFG_COMMON_CASE_EN && (TCFG_COMMON_CASE_NAME == 0x1)
        if (cis_connected_nums > 1) {
            for (i = 0; i < CIG_MAX_NUMS; i++) {
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    remote_dev_msg_status_sync_info_send(app_cig_conn_info[i].cis_conn_info[j].acl_hdl);
                }
            }
        }
#endif

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);
        break;

    case CIG_EVENT_PERIP_DISCONNECT:
        g_printf("CIG_EVENT_PERIP_DISCONNECT");
        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);

        hdl = (cig_hdl_t *)&event[1];

        list_for_each_wireless_data_callback(p) {
            if (p->event_handler) {
                p->event_handler(event[0], (void *)hdl);
            }
        }

        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (app_cig_conn_info[i].used && (app_cig_conn_info[i].cig_hdl == hdl->cig_hdl)) {
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    if (app_cig_conn_info[i].cis_conn_info[j].cis_hdl == hdl->cis_hdl) {
                        app_cig_conn_info[i].cis_conn_info[j].cis_hdl = 0;
                        app_cig_conn_info[i].cis_conn_info[j].acl_hdl = 0;
                        app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification = 0;
                        app_cig_conn_info[i].cis_conn_info[j].cis_status = APP_CONNECTED_STATUS_DISCONNECT;
                        app_cig_conn_info[i].cig_hdl = 0xFF;
                        break;
                    }
                }
                find = 1;
            }
        }

        if (!find) {
            //释放互斥量
            app_connected_mutex_post(&mutex, __LINE__);
            break;
        }

        cis_connected_nums--;
        ASSERT(cis_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && cis_connected_nums >= 0, "cis_connected_nums:%d", cis_connected_nums);

        ret = connected_perip_disconnect_deal((void *)hdl);
        if (ret < 0) {
            r_printf("connected_perip_disconnect_deal fail");
        }

#if CIG_CONNECTED_TIMEOUT
        if (!cis_connected_nums && !cig_connection_timeout) {
            y_printf("app_cig_connection_timeout add:%d", __LINE__);
            cig_connection_timeout = sys_timeout_add(NULL, app_cig_connection_timeout, CIG_CONNECTED_TIMEOUT);
        }
#endif
#if TCFG_AUTO_SHUT_DOWN_TIME
        if (!cis_connected_nums) {
            sys_auto_shut_down_enable();   // 恢复自动关机
        }
#endif

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);
        break;

    case CIG_EVENT_ACL_CONNECT:
        g_printf("CIG_EVENT_ACL_CONNECT");
        acl_info = (cis_acl_info_t *)&event[1];
        if (acl_info->conn_type) {
            log_info("connect test box ble");
            return -EPERM;
        }

        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);
        g_printf("remote device addr:");
        put_buf((u8 *)&acl_info->pri_ch, sizeof(acl_info->pri_ch));
        acl_connected_nums++;
        ASSERT(acl_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && acl_connected_nums >= 0, "acl_connected_nums:%d", acl_connected_nums);

        syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_addr0, sizeof(pair_addr0));
        syscfg_read(VM_WIRELESS_PAIR_CODE1, &pair_addr1, sizeof(pair_addr1));
        if ((acl_info->pri_ch != pair_addr0) && (acl_info->pri_ch != pair_addr1)) {
            //记录远端设备地址，用于下次连接时过滤设备
            if (acl_connected_nums == 1) {
                ret = syscfg_write(VM_WIRELESS_PAIR_CODE0, &acl_info->pri_ch, sizeof(acl_info->pri_ch));
            } else if (acl_connected_nums == 2) {
                ret = syscfg_write(VM_WIRELESS_PAIR_CODE1, &acl_info->pri_ch, sizeof(acl_info->pri_ch));
            }
            if (ret <= 0) {
                r_printf(">>>>>>wireless pair code save err:%d, ret:%d", acl_connected_nums, ret);
            }
        }

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);

        break;

    case CIG_EVENT_ACL_DISCONNECT:
        g_printf("CIG_EVENT_ACL_DISCONNECT");
        acl_info = (cis_acl_info_t *)&event[1];
        if (acl_info->conn_type) {
            log_info("disconnect test box ble");
            return -EPERM;
        }

        //由于是异步操作需要加互斥量保护，避免connected_close的代码与其同时运行,添加的流程请放在互斥量保护区里面
        app_connected_mutex_pend(&mutex, __LINE__);

        acl_connected_nums--;
        ASSERT(acl_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && acl_connected_nums >= 0, "acl_connected_nums:%d", acl_connected_nums);

        //释放互斥量
        app_connected_mutex_post(&mutex, __LINE__);
        break;

    default:
        result = -ESRCH;
        break;
    }

    return result;
}
APP_MSG_PROB_HANDLER(app_le_connected_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_CIG,
    .handler = app_connected_conn_status_event_handler,
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前是否在退出模式的状态
 *
 * @return 1；是，0：否
 */
/* ----------------------------------------------------------------------------*/
u8 get_connected_app_mode_exit_flag(void)
{
    return connected_app_mode_exit;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 判断当前设备作为CIG主机还是从机
 *
 * @return true:主机，false:从机
 */
/* ----------------------------------------------------------------------------*/
static bool is_connected_as_central()
{
#if (LEA_CIG_FIX_ROLE == 1)
    return true;
#elif (LEA_CIG_FIX_ROLE == 2)
    return false;
#endif

    gpio_set_mode(PORTC, PORT_PIN_8, PORT_INPUT_PULLUP_10K);
    os_time_dly(2);
    if (gpio_read_port(PORTC, PORT_PIN_8) == 0) {
        return true;
    } else {
        return false;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 检测当前是否处于挂起状态
 *
 * @return true:处于挂起状态，false:处于非挂起状态
 */
/* ----------------------------------------------------------------------------*/
static bool is_need_resume_connected()
{
    u8 i;
    u8 find = 0;
    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].cig_status == APP_CONNECTED_STATUS_SUSPEND) {
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
 * @brief CIG从挂起状态恢复
 */
/* ----------------------------------------------------------------------------*/
static void app_connected_resume()
{
    cig_hdl_t temp_connected_hdl;

    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!is_need_resume_connected()) {
        return;
    }

    app_connected_open(0);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG进入挂起状态
 */
/* ----------------------------------------------------------------------------*/
static void app_connected_suspend()
{
    u8 i;
    u8 find = 0;
    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            find = 1;
            break;
        }
    }
    if (find) {
        connected_last_role = app_get_connected_role();
        app_connected_close_all(APP_CONNECTED_STATUS_SUSPEND);
    }
}

static void app_connected_retry_open(void *priv)
{
    u8 pair_without_addr = (u8)priv;

    app_connected_open(pair_without_addr);


}
/* --------------------------------------------------------------------------*/
/**
 * @brief 开启CIG
 *
 * @param pair_without_addr:不匹配配对地址，重新自由配对
 */
/* ----------------------------------------------------------------------------*/
void app_connected_open(u8 pair_without_addr)
{
    u8 i;
    u8 role = 0;
    u8 cig_available_num = 0;
    cig_hdl_t temp_connected_hdl;
    cig_parameter_t *params;
    int ret;
    u64 pair_addr;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!app_connected_init_flag) {
        return;
    }

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (!app_cig_conn_info[i].used) {
            cig_available_num++;
        }
    }

    if (!cig_available_num) {
        return;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return;
    }

    if ((mode->name == APP_MODE_FM) ||
        (mode->name == APP_MODE_PC)) {
        return;
    }


    cis_switch_onoff = 1;

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    ble_module_enable(0);
    if (bt_rcsp_ble_conn_num() > 0) {
        u32 addr = pair_without_addr;
        rcsp_connect_dev_detect_timer = sys_timeout_add((void *)addr, app_connected_retry_open, 250);//由于非标准广播使用私有hci事件回调所以需要等RCSP断连事件处理完后才能开广播
        return;
    } else {
        rcsp_connect_dev_detect_timer = 0;
    }
#endif

    log_info("connected_open");

    app_connected_mutex_pend(&mutex, __LINE__);
#if TCFG_KBOX_1T3_MODE_EN
    le_audio_ops_register(APP_MODE_NULL);
#endif

    if (is_connected_as_central()) {
        //初始化cig发送端参数
        params = set_cig_params(mode->name, CONNECTED_ROLE_CENTRAL, pair_without_addr);
        //打开big，打开成功后会在函数app_connected_conn_status_event_handler做后续处理
        temp_connected_hdl.cig_hdl = connected_central_open(params);
        //记录角色
        role = CONNECTED_ROLE_CENTRAL;

    } else {
        //初始化cig接收端参数
        params = set_cig_params(mode->name, CONNECTED_ROLE_PERIP, pair_without_addr);
        //打开big，打开成功后会在函数app_connected_conn_status_event_handler做后续处理
        temp_connected_hdl.cig_hdl = connected_perip_open(params);

    }

    if (temp_connected_hdl.cig_hdl >= 0) {
        //只有按地址配对时才需要注册超时任务
        if (!pair_without_addr) {
            //读取配对地址，只要有一个配对地址存在就注册连接超时处理函数
            ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_addr, sizeof(pair_addr));
            if (ret <= 0) {
                if (role == CONNECTED_ROLE_CENTRAL) {
                    ret = syscfg_read(VM_WIRELESS_PAIR_CODE1, &pair_addr, sizeof(pair_addr));
                }
            }
            if (ret > 0) {
#if CIG_CONNECTED_TIMEOUT
                if (!cig_connection_timeout) {
                    y_printf("app_cig_connection_timeout add:%d", __LINE__);
                    cig_connection_timeout = sys_timeout_add(NULL, app_cig_connection_timeout, CIG_CONNECTED_TIMEOUT);
                }
#endif
            }
        }
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (!app_cig_conn_info[i].used) {
                app_cig_conn_info[i].cig_hdl = temp_connected_hdl.cig_hdl;
                app_cig_conn_info[i].cig_status = APP_CONNECTED_STATUS_START;
                app_cig_conn_info[i].used = 1;
                break;
            }
        }
    }
    app_connected_mutex_post(&mutex, __LINE__);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭对应cig_hdl的CIG连接
 *
 * @param cig_hdl:需要关闭的CIG连接对应的hdl
 * @param status:关闭后CIG进入的suspend还是stop状态
 */
/* ----------------------------------------------------------------------------*/
void app_connected_close(u8 cig_hdl, u8 status)
{
    u8 i;
    u8 find = 0;

    if (!app_connected_init_flag) {
        return;
    }

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if (rcsp_connect_dev_detect_timer) {
        sys_timeout_del(rcsp_connect_dev_detect_timer);
    }
#endif
    log_info("connected_close");
    //由于是异步操作需要加互斥量保护，避免和开启的流程同时运行,添加的流程请放在互斥量保护区里面
    app_connected_mutex_pend(&mutex, __LINE__);

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used && (app_cig_conn_info[i].cig_hdl == cig_hdl)) {
            memset(&app_cig_conn_info[i], 0, sizeof(struct app_cig_conn_info));
            app_cig_conn_info[i].cig_status = status;
            find = 1;
            break;
        }
    }

    if (find) {
        connected_close(cig_hdl);
        acl_connected_nums -= CIS_MAX_CONNECTABLE_NUMS;
        ASSERT(acl_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && acl_connected_nums >= 0, "acl_connected_nums:%d", acl_connected_nums);
        cis_connected_nums -= CIS_MAX_CONNECTABLE_NUMS;
        ASSERT(cis_connected_nums <= CIS_MAX_CONNECTABLE_NUMS && cis_connected_nums >= 0, "cis_connected_nums:%d", cis_connected_nums);
        if (cig_connection_timeout) {
            sys_timeout_del(cig_connection_timeout);
            cig_connection_timeout = 0;
        }
    }

#if TCFG_AUTO_SHUT_DOWN_TIME
    sys_auto_shut_down_enable();   // 恢复自动关机
#endif

    //关闭VM写RAM
    /* vm_ram_direct_2_flash_switch(TRUE); */

    //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
    /* if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) { */
    /*     vm_flush2flash(0); */
    /* } */

    //释放互斥量
    app_connected_mutex_post(&mutex, __LINE__);
    cis_switch_onoff = 0;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if ((status != APP_CONNECTED_STATUS_SUSPEND)) {
        ll_set_private_access_addr_pair_channel(0);
        ble_module_enable(1);
    }
#endif
}

bool get_connected_on_off(void)
{
    bool find = false;
    for (int i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            find = true;
            break;
        }
    }
    return find;

}


u8 get_connected_cis_num(void)
{
    u8 num = 0;
    if (!get_connected_on_off()) {
        return 0;
    } else {
        return cis_connected_nums;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭所有cig_hdl的CIG连接
 *
 * @param status:关闭后CIG进入的suspend还是stop状态
 */
/* ----------------------------------------------------------------------------*/
void app_connected_close_all(u8 status)
{
    u8 i;

    if (!app_connected_init_flag) {
        return;
    }

    log_info("connected_close");
    //由于是异步操作需要加互斥量保护，避免和开启的流程同时运行,添加的流程请放在互斥量保护区里面
    app_connected_mutex_pend(&mutex, __LINE__);

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used && app_cig_conn_info[i].cig_hdl) {
            connected_close(app_cig_conn_info[i].cig_hdl);
            memset(&app_cig_conn_info[i], 0, sizeof(struct app_cig_conn_info));
            app_cig_conn_info[i].cig_status = status;
        }
    }

    acl_connected_nums = 0;
    cis_connected_nums = 0;

    if (cig_connection_timeout) {
        sys_timeout_del(cig_connection_timeout);
        cig_connection_timeout = 0;
    }

    //关闭VM写RAM
    /* vm_ram_direct_2_flash_switch(TRUE); */

    //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
    /* if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) { */
    /*     vm_flush2flash(0); */
    /* } */

    //释放互斥量
    app_connected_mutex_post(&mutex, __LINE__);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG开关切换
 *
 * @return 0：操作成功
 */
/* ----------------------------------------------------------------------------*/
int app_connected_switch(void)
{
    u8 i;
    u8 find = 0;
    cig_hdl_t temp_connected_hdl;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            find = 1;
            break;
        }
    }

    if (find) {
        play_tone_file_alone_callback(get_tone_files()->le_connected_close,
                                      (void *)TONE_INDEX_CONNECTED_CLOSE,
                                      connected_tone_play_end_callback);
        app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    } else {
        play_tone_file_alone_callback(get_tone_files()->le_connected_open,
                                      (void *)TONE_INDEX_CONNECTED_OPEN,
                                      connected_tone_play_end_callback);
        app_connected_open(0);
    }
    return 0;
}


int app_connected_open_with_role(u8 role_in, u8 pair_without_addr)
{
    u8 i;
    u8 role = 0;
    u8 cig_available_num = 0;
    cig_hdl_t temp_connected_hdl;
    cig_parameter_t *params;
    int ret;
    u64 pair_addr;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (!app_cig_conn_info[i].used) {
            cig_available_num++;
        }
    }

    if (!cig_available_num) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    if ((mode->name == APP_MODE_FM) ||
        (mode->name == APP_MODE_PC)) {
        return -EPERM;
    }

    log_info("connected_open");
    if ((role_in & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        //初始化cig发送端参数
        params = set_cig_params(mode->name, CONNECTED_ROLE_CENTRAL, pair_without_addr);
        //打开big，打开成功后会在函数app_connected_conn_status_event_handler做后续处理
        temp_connected_hdl.cig_hdl = connected_central_open(params);
        //记录角色
        role = CONNECTED_ROLE_CENTRAL;

    } else if ((role_in & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        //初始化cig接收端参数
        params = set_cig_params(mode->name, CONNECTED_ROLE_PERIP, pair_without_addr);
        //打开big，打开成功后会在函数app_connected_conn_status_event_handler做后续处理
        temp_connected_hdl.cig_hdl = connected_perip_open(params);

    }
    if (temp_connected_hdl.cig_hdl >= 0) {
        //只有按地址配对时才需要注册超时任务
        if (!pair_without_addr) {
            //读取配对地址，只要有一个配对地址存在就注册连接超时处理函数
            ret = syscfg_read(VM_WIRELESS_PAIR_CODE0, &pair_addr, sizeof(pair_addr));
            if (ret <= 0) {
                if (role == CONNECTED_ROLE_CENTRAL) {
                    ret = syscfg_read(VM_WIRELESS_PAIR_CODE1, &pair_addr, sizeof(pair_addr));
                }
            }
            if (ret > 0) {
#if CIG_CONNECTED_TIMEOUT
                if (!cig_connection_timeout) {
                    y_printf("app_cig_connection_timeout add:%d", __LINE__);
                    cig_connection_timeout = sys_timeout_add(NULL, app_cig_connection_timeout, CIG_CONNECTED_TIMEOUT);
                }
#endif
            }
        }
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            if (!app_cig_conn_info[i].used) {
                app_cig_conn_info[i].cig_hdl = temp_connected_hdl.cig_hdl;
                app_cig_conn_info[i].cig_status = APP_CONNECTED_STATUS_START;
                app_cig_conn_info[i].used = 1;
                break;
            }
        }
    }

    return 0;
}

int app_connected_role_switch(void)
{
    u8 i;
    u8 find = 0;
    cig_hdl_t temp_connected_hdl;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            find = 1;
            break;
        }
    }
    u8 role = get_connected_role();
    printf("===============curr role %d, ready switch============", role);
    if (find) {
        play_tone_file_alone_callback(get_tone_files()->le_connected_close,
                                      (void *)TONE_INDEX_CONNECTED_CLOSE,
                                      connected_tone_play_end_callback);
        app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    }
    if (((role & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) || (role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        play_tone_file_alone_callback(get_tone_files()->le_connected_open,
                                      (void *)TONE_INDEX_CONNECTED_CLOSE,
                                      connected_tone_play_end_callback);
        app_connected_open_with_role(role, 0);
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
int update_app_connected_deal_scene(int scene)
{
    cur_deal_scene = scene;
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG开启情况下，不同场景的处理流程
 *
 * @param scene:当前系统状态
 *
 * @return -1:无需处理，0:处理事件但不拦截后续流程，1:处理事件并拦截后续流程
 */
/* ----------------------------------------------------------------------------*/
int app_connected_deal(int scene)
{
    u8 i, j;
    u8 find = 0;
    int ret = 0;
    static u8 phone_start_cnt = 0;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    if ((cur_deal_scene == scene) &&
        (scene != LE_AUDIO_PHONE_START) &&
        (scene != LE_AUDIO_PHONE_STOP)) {
        log_error("app_connected_deal,cur_deal_scene not be modified:%d", scene);
        return -EPERM;
    }

#if TCFG_KBOX_1T3_MODE_EN
    if ((scene != LE_AUDIO_PHONE_START) && (scene != LE_AUDIO_PHONE_STOP)) {
        log_error("app_connected_deal,cur_deal_scene Operation not permitted:%d", scene);
        return -EPERM;
    }
#endif

    cur_deal_scene = scene;

    switch (scene) {
    case LE_AUDIO_APP_MODE_ENTER:
        log_info("LE_AUDIO_APP_MODE_ENTER");
        //进入当前模式
        connected_app_mode_exit = 0;
        config_connected_as_master = 1;
        mode = app_get_current_mode();
        if (mode) {
            le_audio_ops_register(mode->name);
        }
        if (is_need_resume_connected()) {
            app_connected_resume();
            ret = 1;
        }
        break;
    case LE_AUDIO_APP_MODE_EXIT:
        log_info("LE_AUDIO_APP_MODE_EXIT");
        //退出当前模式
        connected_app_mode_exit = 1;
        config_connected_as_master = 0;
        app_connected_suspend();
        break;
    case LE_AUDIO_MUSIC_START:
    case LE_AUDIO_A2DP_START:
        log_info("LE_AUDIO_MUSIC_START");
        connected_app_mode_exit = 0;
        config_connected_as_master = 1;
        if (app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) {
            for (i = 0; i < CIG_MAX_NUMS; i++) {
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    cis_audio_recorder_reset(app_cig_conn_info[i].cis_conn_info[j].cis_hdl);
                }
            }
            ret = 1;
        }
        break;
    case LE_AUDIO_MUSIC_STOP:
    case LE_AUDIO_A2DP_STOP:
        log_info("LE_AUDIO_MUSIC_STOP");
        config_connected_as_master = 0;
        if (app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) {
            for (i = 0; i < CIG_MAX_NUMS; i++) {
                for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                    cis_audio_recorder_close(app_cig_conn_info[i].cis_conn_info[j].cis_hdl);
                }
            }
            ret = 1;
        }
        break;
    case LE_AUDIO_PHONE_START:
        log_info("LE_AUDIO_PHONE_START");
        //通话时，挂起
        phone_start_cnt++;
        app_connected_suspend();
        ret = 0;
        break;
    case LE_AUDIO_PHONE_STOP:
        log_info("LE_AUDIO_PHONE_STOP");
        //通话结束恢复
        phone_start_cnt--;
        printf("===phone_start_cnt:%d===\n", phone_start_cnt);
        if (phone_start_cnt) {
            log_info("phone_start_cnt:%d", phone_start_cnt);
            break;
        }
        mode = app_get_current_mode();
        //当前处于蓝牙模式并且挂起前作为发送设备，恢复的操作在播放a2dp处执行
        if (mode && (mode->name == APP_MODE_BT)) {
            if (connected_last_role == APP_CONNECTED_ROLE_TRANSMITTER) {
            }
        }
        //当前处于蓝牙模式并且挂起前，恢复并作为接收设备
        if (is_need_resume_connected()) {
            app_connected_resume();
        }
        break;
    case LE_AUDIO_EDR_DISCONN:
        log_info("LE_AUDIO_EDR_DISCONN");
        break;
        ret = 0;
        //当经典蓝牙断开后，作为发送端的设备挂起
        if ((app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) ||
            (app_get_connected_role() == APP_CONNECTED_ROLE_DUPLEX)) {
            app_connected_suspend();
        }
        if (is_need_resume_connected()) {
            app_connected_resume();
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
 * @brief ACL数据发送接口
 *
 * @param device_channel:发送给远端设备的标识
 * @param data:需要发送的数据
 * @param length:数据长度
 *
 * @return slen:实际发送数据的总长度
 */
/* ----------------------------------------------------------------------------*/
int app_connected_send_custom_data(u8 device_channel, void *data, size_t length)
{
    u8 i, j;
    int slen = 0;
    struct wireless_data_callback_func *p;

    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                if (app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification & device_channel) {
                    slen += connected_send_custom_data(app_cig_conn_info[i].cis_conn_info[j].acl_hdl, data, length);
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
 * @brief 判断对应设备标识的CIS当前是否处于连接状态
 *
 * @param device_channel:远端设备标识
 *
 * @return connected_status:连接状态，按bit判断
 */
/* ----------------------------------------------------------------------------*/
u8 app_cis_is_connected(u8 device_channel)
{
    u8 i, j;
    u8 connected_status = 0;
    for (i = 0; i < CIG_MAX_NUMS; i++) {
        if (app_cig_conn_info[i].used) {
            for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                if (app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification & device_channel) {
                    if (app_cig_conn_info[i].cis_conn_info[j].cis_status == APP_CONNECTED_STATUS_CONNECT) {
                        connected_status |= app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification;
                    }
                }
            }
        }
    }
    return connected_status;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG资源初始化
 */
/* ----------------------------------------------------------------------------*/
void app_connected_init(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (app_connected_init_flag) {
        return;
    }

    int os_ret = os_mutex_create(&mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
        ASSERT(0);
    }
    app_connected_init_flag = 1;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG资源卸载
 */
/* ----------------------------------------------------------------------------*/
void app_connected_uninit(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!app_connected_init_flag) {
        return;
    }

    app_connected_init_flag = 0;
    int os_ret = os_mutex_del(&mutex, OS_DEL_NO_PEND);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 非蓝牙后台情况下，在其他音源模式开启CIG，前提要先开蓝牙协议栈
 */
/* ----------------------------------------------------------------------------*/
void app_connected_open_in_other_mode()
{
    if (!g_bt_hdl.init_ok || g_bt_hdl.work_mode != BT_MODE_CIG) {
        return;
    }

    app_connected_init();
    app_connected_open(0);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief
 * @brief 非蓝牙后台情况下，在其他音源模式关闭CIG
 */
/* ----------------------------------------------------------------------------*/
void app_connected_close_in_other_mode()
{
    if (!g_bt_hdl.init_ok || g_bt_hdl.work_mode != BT_MODE_CIG) {
        return;
    }

    app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    app_connected_uninit();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG连接角色重定义
 *
 * @return 设备角色为发送设备还是接收设备
 */
/* ----------------------------------------------------------------------------*/
u8 app_get_connected_role(void)
{
    u8 bit7_value = 0;
    u8 role = APP_CONNECTED_ROLE_UNKNOW;
    u8 connected_role = get_connected_role();
    if (connected_role & BIT(7)) {
        bit7_value = 1;
        connected_role &= ~BIT(7);
    }
#if (LEA_CIG_TRANS_MODE == 1)
#if (LEA_CIG_CONNECT_MODE == 2)
    if (connected_role == CONNECTED_ROLE_CENTRAL) {
        role = APP_CONNECTED_ROLE_RECEIVER;
    } else if (connected_role == CONNECTED_ROLE_PERIP) {
        role = APP_CONNECTED_ROLE_TRANSMITTER;
    }
#else
    if (connected_role == CONNECTED_ROLE_CENTRAL) {
        role = APP_CONNECTED_ROLE_TRANSMITTER;
    } else if (connected_role == CONNECTED_ROLE_PERIP) {
        role = APP_CONNECTED_ROLE_RECEIVER;
    }
#endif
#elif (LEA_CIG_TRANS_MODE == 2)
    if ((connected_role == CONNECTED_ROLE_CENTRAL) ||
        (connected_role == CONNECTED_ROLE_PERIP)) {
        role = APP_CONNECTED_ROLE_DUPLEX;
    }
#endif

    if (role && bit7_value) {
        //没有设备连接时，或上BIT(7)，防止外部流程判断错误
        role |= BIT(7);
    }
    return role;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 根据地址连接超时，重新进入自由配对
 *
 * @param priv
 */
/* ----------------------------------------------------------------------------*/
static void app_cig_connection_timeout(void *priv)
{
    app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    app_connected_open(1);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 清除配对信息，并重新配对
 */
/* ----------------------------------------------------------------------------*/
void app_connected_remove_pairs_addr(void)
{
    u64 pair_addr = 0;
    u8 status = app_get_connected_role();
    if (status) {
        app_connected_close_all(APP_CONNECTED_STATUS_STOP);
    }
    syscfg_write(VM_WIRELESS_PAIR_CODE0, &pair_addr, sizeof(pair_addr));
    syscfg_write(VM_WIRELESS_PAIR_CODE1, &pair_addr, sizeof(pair_addr));
    if (status) {
        app_connected_open(1);
    }
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
    for (u8 i = 0, j; i < CIG_MAX_NUMS; i++) {
        for (u8 j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
            if (app_cig_conn_info[i].cis_conn_info[j].acl_hdl == acl_hdl) {
                for (u8 k = 0; k < table_size_h; k++) {
                    if (save_sync_status_table[k][0]) {
                        wireless_custom_data_send_to_sibling('M', save_sync_status_table[k], table_size_l, app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification);
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
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                if (app_cig_conn_info[i].cis_conn_info[j].acl_hdl != acl_hdl) {
                    wireless_custom_data_send_to_sibling('M', data, len, app_cig_conn_info[i].cis_conn_info[j].remote_dev_identification);
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
 * @brief CIG连接成功后central发起音量同步
 */
/* ----------------------------------------------------------------------------*/
void app_connected_sync_volume(u8 remote_dev_identification)
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
        for (i = 0; i < CIG_MAX_NUMS; i++) {
            for (j = 0; j < CIS_MAX_CONNECTABLE_NUMS; j++) {
                if (app_cig_conn_info[i].cis_conn_info[j].acl_hdl == acl_hdl) {
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

// 是否打开cis connect连接, 1T3 项目使用
bool is_open_cis_connet(void)
{
#if TCFG_MIC_EFFECT_ENABLE && TCFG_KBOX_1T3_MODE_EN
    if (!mic_effect_player_runing()) {
        return 1;
    } else {
        return 0;
    }
#else
    return 1;
#endif
    return 0;
}

u8 get_cis_switch_onoff(void)
{
    return cis_switch_onoff;
}


#endif

