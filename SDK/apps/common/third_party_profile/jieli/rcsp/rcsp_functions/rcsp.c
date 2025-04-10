#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp.data.bss")
#pragma data_seg(".rcsp.data")
#pragma const_seg(".rcsp.text.const")
#pragma code_seg(".rcsp.text")
#endif
#include "rcsp.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "string.h"
#include "system/timer.h"
#include "spp_user.h"
#include "app_task.h"
#include "system/task.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "rcsp_manage.h"
#include "rcsp_setting_opt.h"
#include "adv_1t2_setting.h"
#include "rcsp_ch_loader_download.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if (RCSP_MODE)
#define RCSP_TASK_NAME   "rcsp"

#define RCSP_SPP_INTERACTIVE_SUPPORT	1
#define RCSP_BLE_INTERACTIVE_SUPPORT	1

#define RCSP_DEBUG_EN
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

static struct RcspModel *__this = NULL;

#if RCSP_BLE_MASTER

extern void rcsp_c_cmd_recieve(void *priv, u8 opCode, u8 opCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_c_cmd_recieve_no_response(void *priv, u8 opCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_c_cmd_response(void *priv, u8 opCode, u8 status, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

#else // RCSP_BLE_MASTER

extern void rcsp_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_cmd_recieve_no_respone(void *priv, u8 OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_cmd_respone(void *priv, u8 OpCode, u8 status, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_data_recieve(void *priv, u8 OpCode_SN, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_data_recieve_no_respone(void *priv, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_data_respone(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

#endif // RCSP_BLE_MASTER

extern void JL_recieve_packet_parse_process(void);
int JL_rcsp_recieve_deal(int param)
{
    if ((!__this) || (!__this->rcsp_buf)) {
        return 0;
    }
    JL_recieve_packet_parse_process();
    return 0;
}

void JL_rcsp_recieve_resume(void)
{
    int argv[3];
    argv[0] = (int)JL_rcsp_recieve_deal;
    argv[1] = 1;
    argv[2] = 0;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
}

void JL_rcsp_resume_do(void)
{
    os_sem_post(&__this->sem);
    JL_rcsp_recieve_resume();
}

bool JL_rcsp_protocol_can_send(void)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status() && (tws_api_get_role() == TWS_ROLE_SLAVE)) {
        // 从机不允许发rcsp数据
        /* printf("rcsp slave cant send data!\n"); */
        return false;
    } else {
        return true;
    }
#endif
    return true;
}


extern void rcsp_clean_update_hdl_for_end_update(u16 ble_con_handle, u8 *spp_remote_addr);
static void rcsp_process(void *p)
{
    ///从vm获取相关配置
    rcsp_setting_init();
#if TCFG_RCSP_DUAL_CONN_ENABLE
    rcsp_1t2_setting_reset();
#endif
    rcsp_clean_update_hdl_for_end_update(0, NULL);

    while (1) {
        os_sem_pend(&__this->sem, 0);
        JL_send_packet_process();
        /* JL_protocol_process(); */
    }
}


struct RcspModel *rcsp_handle_get(void)
{
    return __this;
}

static u16 rcsp_timer = 0;

static void rcsp_process_timer()
{
    os_sem_post(&__this->sem);
}

void rcsp_timer_contrl(u8 status)
{
    if (status) {
        if (rcsp_timer == 0) {
            rcsp_timer = sys_s_hi_timer_add(NULL, rcsp_process_timer, 2000);
        }
    } else {
        if (rcsp_timer) {
            sys_s_hi_timer_del(rcsp_timer);
            rcsp_timer = 0;
        }
    }
}

static JL_PRO_CB bt_rcsp_callback = {
#if RCSP_BLE_MASTER
    .CMD_resp          = rcsp_c_cmd_recieve,
    .CMD_no_resp       = rcsp_c_cmd_recieve_no_response,
    .CMD_recieve_resp  = rcsp_c_cmd_response,
    .DATA_resp         = NULL,
    .DATA_no_resp      = NULL,
    .DATA_recieve_resp = NULL,

#else // RCSP_BLE_MASTER

    .CMD_resp          = rcsp_cmd_recieve,
    .CMD_no_resp       = rcsp_cmd_recieve_no_respone,
    .CMD_recieve_resp  = rcsp_cmd_respone,
    .DATA_resp         = rcsp_data_recieve,
    .DATA_no_resp      = rcsp_data_recieve_no_respone,
    .DATA_recieve_resp = rcsp_data_respone,
#endif // RCSP_BLE_MASTER

    .wait_resp_timeout = NULL,
};

void rcsp_init(void)
{
    if (__this) {
        return;
    }
    rcsp_printf("%s", __FUNCTION__);

    //set_jl_mtu_resv();
    ///设置rcsp最大发送buf， 即MTU
#if RCSP_BLE_MASTER
    /* set_jl_mtu_send(540); */
    set_jl_mtu_send(4130);
#else
    set_jl_mtu_send(272);
#endif
#if (RCSP_MODE == RCSP_MODE_EARPHONE)
    set_jl_rcsp_recieve_buf_size(768);
#else
    //如果支持大文件传输可以通过修改接收buf大小优化传输速度
    set_jl_rcsp_recieve_buf_size(4 * 1024 + 272);
#endif

    u32 size = rcsp_protocol_need_buf_size();
#if TCFG_RCSP_DUAL_CONN_ENABLE
    // 一拖二要求底层buf乘以2
    size *= 2;
#endif
    printf("rcsp alloc buf size:%d\n", size);
    u8 *ptr = zalloc(size);
    ASSERT(ptr, "no ram for rcsp !!\n");

    struct RcspModel *rcspModel = (struct RcspModel *)zalloc(sizeof(struct RcspModel));
    ASSERT(rcspModel, "no ram for rcsp !!\n");
    rcsp_config(rcspModel);
    __this = rcspModel;
    __this->rcsp_buf = ptr;

    JL_protocol_init(ptr, size);
    bt_rcsp_callback.priv = rcspModel;
    JL_protocol_dev_switch(&bt_rcsp_callback);

    os_sem_create(&__this->sem, 0);

    int err = task_create(rcsp_process, (void *)rcspModel, RCSP_TASK_NAME);
    if (err) {
        rcsp_printf("rcsp create fail %x\n", err);
    }
}

static void rcsp_exit_in_app_core_task(void)
{
    if (rcsp_timer) {
        sys_s_hi_timer_del(rcsp_timer);
        rcsp_timer = 0;
    }
    JL_protocol_exit();
    task_kill(RCSP_TASK_NAME);
    if (__this->rcsp_buf) {
        free(__this->rcsp_buf);
        __this->rcsp_buf = NULL;
    }
    if (__this) {
        free(__this);
        __this = NULL;
    }
    rcsp_opt_release();
    rcsp_update_resume();
}

void rcsp_exit(void)
{
    int argv[3];
    argv[0] = (int)rcsp_exit_in_app_core_task;
    argv[1] = 1;
    argv[2] = 0;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
}

#endif//RCSP_MODE
