#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".update_tws_new.data.bss")
#pragma data_seg(".update_tws_new.data")
#pragma const_seg(".update_tws_new.text.const")
#pragma code_seg(".update_tws_new.text")
#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "generic/lbuf.h"
#include "init.h"
#include "update_tws_new.h"
#include "dual_bank_updata_api.h"
#include "bt_tws.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "update.h"
#include "app_main.h"
#include "timer.h"
#include "clock_manager/clock_manager.h"

#if (OTA_TWS_SAME_TIME_ENABLE && OTA_TWS_SAME_TIME_NEW)

//#define LOG_TAG_CONST       EARPHONE
#define LOG_TAG             "[UPDATE_TWS]"
#define log_errorOR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define THIS_TASK_NAME	            "tws_ota"

#define TWS_FUNC_ID_OTA_TRANS	(('O' << (3 * 8)) | ('T' << (2 * 8)) | ('A' << (1 * 8)) | ('\0'))

OS_SEM tws_ota_sem;
static int old_sys_clk = 0;;
int tws_ota_timeout_hdl = 0;
static u8 sync_update_sn = 1;

static void (*user_chip_tws_update_handle)(void *data, u32 len) = NULL;
static void (*sync_update_crc_init_hdl)(void) = NULL;
static u32(*sync_update_crc_calc_hdl)(u32 init_crc, const void *data, u32 len) = NULL;
extern const int support_dual_bank_update_no_erase;
extern const int support_dual_bank_update_breakpoint;

struct _bt_event {
    u8 event;
    u8 args[7];
    u32 value;
};

static void bt_ota_event_update_to_user(u8 event)
{
    int from = 0;
    int msg[4];
    struct _bt_event *evt = (struct _bt_event *)msg;

    evt->event = event;

    from = SYS_BT_OTA_EVENT_TYPE_STATUS;

    os_taskq_post_type("app_core", from, sizeof(*evt) / 4, msg);

    /* 防止短时间内太多事件,app_core处理不过来导致qfull */
    os_time_dly(1);
}

void ota_event_update_to_user(u8 event)
{
    bt_ota_event_update_to_user(event);
}

__attribute__((weak))
void db_update_notify_fail_to_phone()
{

}

void tws_sync_update_crc_handler_register(void (*crc_init_hdl)(void), u32(*crc_calc_hdl)(u32 init_crc, const void *data, u32 len))
{
    sync_update_crc_init_hdl = crc_init_hdl;
    sync_update_crc_calc_hdl = crc_calc_hdl;
    printf("sync_update_crc_init_hdl:%x\n  sync_update_crc_calc_hdl:%x\n", (int)sync_update_crc_init_hdl, (int)sync_update_crc_calc_hdl);
}

int tws_ota_close(void);

int tws_ota_get_data_from_sibling(u8 opcode, const void *data, u8 len)
{
    return 0;
}

void tws_ota_timeout(void *priv) 		//从机没收到命令超时就退出升级
{
    tws_ota_close();
    dual_bank_passive_update_exit(NULL);
}

void tws_ota_timeout_reset(void)
{
    if (tws_ota_timeout_hdl) {
        sys_timeout_del(tws_ota_timeout_hdl);
    }
    tws_ota_timeout_hdl = sys_timeout_add(NULL, tws_ota_timeout, 8000);
}

void tws_ota_timeout_del(void)
{
    if (tws_ota_timeout_hdl) {
        sys_timeout_del(tws_ota_timeout_hdl);
    }
}


int tws_ota_trans_to_sibling(u8 *data, u16 len)
{
    if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        return -1;
    }
    tws_api_tx_unsniff_req();
    tws_sniff_controle_check_disable();
    sys_auto_shut_down_disable();
    return tws_api_send_data_to_sibling(data, len, TWS_FUNC_ID_OTA_TRANS);
}

int dual_bank_update_burn_boot_info_callback(int ret)
{
    u8 rsp_data[3] = {0};
    rsp_data[0] = OTA_TWS_WRITE_BOOT_INFO_RSP;
    rsp_data[1] = ++ sync_update_sn;
    printf("sync_sn:%d\n", sync_update_sn);
    if (ret) {
        log_info("update_burn_boot_info err\n");
        rsp_data[2] = OTA_TWS_CMD_ERR;
        return -1;
    } else {
        log_info("slave update_burn_boot_info succ\n");
        rsp_data[2] = OTA_TWS_CMD_SUCC;
        //确保从机的回复命令送达到主机
        /* os_time_dly(50); */
        /* ota_event_update_to_user(OTA_UPDATE_SUCC); */
    }
    if (tws_ota_trans_to_sibling(rsp_data, 3)) {
        return -1;
    }

    return 0;
}

int bt_ota_event_handler(int *msg)
{
    struct _bt_event *bt = (struct _bt_event *)msg;
    int ret = 0;
    switch (bt->event) {
    case OTA_START_UPDATE:
        log_info("OTA_START_UPDATE\n");
        break;
    case OTA_START_UPDATE_READY:
        break;
    case OTA_START_VERIFY:
        log_info("OTA_START_VERIFY\n");
        break;
    case OTA_UPDATE_OVER:
        log_info("OTA_UPDATE_OVER\n");
        update_result_set(UPDATA_SUCC);
        dual_bank_passive_update_exit(NULL);
        cpu_reset();
        break;
    case OTA_UPDATE_ERR:
        log_info("OTA_UPDATE_ERR\n");
        dual_bank_passive_update_exit(NULL);
        //cpu_reset();
        break;
    case OTA_UPDATE_SUCC:
        log_info("OTA_UPDATE_SUCC\n");
        update_result_set(UPDATA_SUCC);
        dual_bank_passive_update_exit(NULL);
        sys_enter_soft_poweroff(POWEROFF_RESET);
        break;
    default:
        break;
    }

    return 0;
}

int tws_ota_data_send_m_to_s(u8 *buf, u16 len)
{
    int ret = 0;
    u8 retry = 5;
    os_sem_set(&tws_ota_sem, 0);
    u8 *data = malloc(len + 2);
    data[0] = OTA_TWS_TRANS_UPDATE_DATA;
    data[1] = ++ sync_update_sn;
    memcpy(&data[2], buf, len);
    while (retry --) {
        ret = tws_ota_trans_to_sibling(data, len + 2);
        if (ret < 0 && ret != -1) {
            os_time_dly(5);
        } else {
            break;
        }
    }
    free(data);
    return 0;
}

static u32 g_tws_ota_addr_recore = 0;
static int tws_ota_data_send_with_crc_m_to_s(u8 *buf, u16 len, u16 pack_crc, void *priv)
{
    int ret = 0;
    u8 retry = 5;
    if (g_tws_ota_addr_recore != ((u32 *)priv)[0]) {
        os_sem_set(&tws_ota_sem, 0);
        g_tws_ota_addr_recore = ((u32 *)priv)[0];
    }
    u8 *data = malloc(len + 4);
    data[0] = OTA_TWS_TRANS_UPDATE_DATA_VERIFY_CRC;
    data[1] = ++ sync_update_sn;
    memcpy(&data[2], (u8 *)&pack_crc, sizeof(pack_crc));
    memcpy(&data[4], buf, len);
    while (retry --) {
        ret = tws_ota_trans_to_sibling(data, len + 4);
        if (ret < 0 && ret != -1) {
            os_time_dly(5);
        } else {
            break;
        }
    }
    free(data);
    return 0;
}

extern int norflash_origin_read(u8 *buf, u32 offset, u32 len);
static int tws_trans_data_send_data_again(void *recv_data, u16 len)
{
    u8 retry = 2;
    u16 pack_crc = 0;
    u16 data_len = 0;
    memcpy(&pack_crc, &recv_data[0], sizeof(pack_crc));
    memcpy(&data_len, &recv_data[2], sizeof(data_len));
    // 通过之前记录的地址，从flash中获取对应的数据
    u8 *retry_data = zalloc(data_len);
    if (retry_data) {
        // 防止还没写入数据就把数据读出来，重试2次
        while (retry) {
            norflash_origin_read(retry_data, g_tws_ota_addr_recore, data_len);
            if (CRC16(retry_data, data_len) == pack_crc) {
                tws_ota_data_send_with_crc_m_to_s(retry_data, data_len, pack_crc, (void *)&g_tws_ota_addr_recore);
                break;
            }
            retry --;
            // 写数据大概需要20~30ms
            os_time_dly(3);
        }
        free(retry_data);
    }
    // 如果错误如何告诉升级库?
    return 0;
}

int tws_ota_data_send_pend(void)
{
    if (os_sem_pend(&tws_ota_sem, 300) ==  OS_TIMEOUT) {       //等待收到从机的RSP
        return -1;
    }
    return 0;
}


int tws_ota_user_chip_update_send_m_to_s(u8 cmd, u8 *buf, u16 len)
{
    int ret = 0;
    u8 retry = 5;
    u8 *data = malloc(len + 3);
    data[0] = OTA_TWS_USER_CHIP_UPDATE;
    data[1] = ++ sync_update_sn;
    data[2] = cmd;
    memcpy(&data[3], buf, len);
    while (retry --) {
        ret = tws_ota_trans_to_sibling(data, len + 3);
        if (ret < 0 && ret != -1) {
            os_time_dly(5);
        } else {
            break;
        }
    }
    free(data);

    return 0;
}

u16 tws_ota_enter_verify(void *priv)
{
    u8 rsp_data[2];
    rsp_data[0] = OTA_TWS_VERIFY;
    rsp_data[1] = ++ sync_update_sn;
    printf("tws_ota_enter_verify111\n");
    os_sem_set(&tws_ota_sem, 0);
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        log_info("tws_disconn in verify\n");
        db_update_notify_fail_to_phone();
        return -1;
    }
    if (tws_ota_trans_to_sibling((u8 *)&rsp_data, 2)) {
        return -1;
    }
    if (os_sem_pend(&tws_ota_sem, 800) ==  OS_TIMEOUT) {
        return -1;
    }
    printf("tws_ota_enter_verify222\n");
    return 0;

}

u16 tws_ota_enter_verify_without_crc(void *priv)
{
    u8 rsp_data = OTA_TWS_VERIFY_WITHOUT_CRC;
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        log_info("tws_disconn in verify\n");
        db_update_notify_fail_to_phone();
        return -1;
    }
    if (tws_ota_trans_to_sibling(&rsp_data, 1)) {
        return -1;
    }
    if (os_sem_pend(&tws_ota_sem, 800) ==  OS_TIMEOUT) {
        return -1;
    }
    return 0;

}

u16 tws_ota_exit_verify(u8 *res, u8 *up_flg)
{
    u8 rsp_data[2];
    rsp_data[0] = OTA_TWS_WRITE_BOOT_INFO;
    rsp_data[1] = ++ sync_update_sn;

    *up_flg = 1;
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        log_info("tws_disconn exit verify\n");
        db_update_notify_fail_to_phone();
        return 0;
    }
    if (tws_ota_trans_to_sibling((u8 *)&rsp_data, 2)) {
        return 0;
    }
    if (os_sem_pend(&tws_ota_sem, 300) ==  OS_TIMEOUT) {
        return 0;
    }
    return 1;
}

int tws_ota_result(u8 result)
{
    if (result == 0) {
        tws_api_sync_call_by_uuid(0xA2E22223, SYNC_CMD_UPDATE_OVER, 400);
    } else {
        tws_api_sync_call_by_uuid(0xA2E22223, SYNC_CMD_UPDATE_ERR, 400);
    }
    return 0;
}

void tws_ota_stop(u8 reason)
{
    log_info("%s", __func__);

    tws_ota_close();
    dual_bank_passive_update_exit(NULL);
#if RCSP_UPDATE_EN
    extern void rcsp_db_update_fail_deal(); //双备份升级失败处理
    rcsp_db_update_fail_deal();
#endif
}

void tws_ota_app_event_deal(u8 evevt)
{
    switch (evevt) {
    case TWS_EVENT_CONNECTION_DETACH:
    /* case TWS_EVENT_PHONE_LINK_DETACH: */
    case TWS_EVENT_REMOVE_PAIRS:
        log_info("stop ota : %d --1\n", evevt);
        tws_ota_stop(OTA_STOP_LINK_DISCONNECT);
        break;
    default:
        break;
    }
}

int tws_ota_init(void)
{
    u8 data;
    log_info("tws_ota_init\n");
    tws_api_auto_role_switch_disable();
    os_sem_create(&tws_ota_sem, 0);
    return 0;
}

int tws_ota_open(void *priv)
{
    struct __tws_ota_para *para = (struct __tws_ota_para *)priv;
    int ret = 0;
    g_tws_ota_addr_recore = 0;
    tws_api_auto_role_switch_disable();
    os_sem_create(&tws_ota_sem, 0);
    u8 *data = malloc(sizeof(struct __tws_ota_para) + 2 + para->param_len);
    if (!data) {
        ret = -1;
        goto _ERR_RET;
    }
    log_info("tws_ota_open\n");

    if (tws_api_get_tws_state() & TWS_STA_SIBLING_DISCONNECTED) {
        log_info("tws_disconn, ota open fail");
        db_update_notify_fail_to_phone();
        ret = -1;
        goto _ERR_RET;
    }

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        extern void bt_check_exit_sniff();
        bt_check_exit_sniff();
        //master 发命令给slave启动升级
    }

    log_info("tws_ota_open1\n");
    data[0] = OTA_TWS_START_UPDATE;
    data[1] = ++ sync_update_sn;
    memcpy(data + 2, para, sizeof(struct __tws_ota_para));
    if (para->param_len) {
        memcpy(data + 2 + sizeof(struct __tws_ota_para), para->param, para->param_len);
    }
    log_info("tws_ota_open2\n");
    if (tws_ota_trans_to_sibling(data, sizeof(struct __tws_ota_para) + 2 + para->param_len)) {
        ret = -1;
        goto _ERR_RET;
    }
    if (os_sem_pend(&tws_ota_sem, 500) == OS_TIMEOUT) {
        log_info("tws_ota_open pend timeout\n");
        ret = -1;
        goto _ERR_RET;
    }
    printf("tws_ota_open succ...\n");
_ERR_RET:
    if (data) {
        free(data);
    }
    return ret;
}

int tws_ota_close(void)
{
    int ret = 0;
    log_info("%s", __func__);
    task_kill(THIS_TASK_NAME);
    return ret;
}


int tws_ota_err_callback(u8 reason)
{
    ota_event_update_to_user(OTA_UPDATE_ERR);
    return 0;
}

u16 tws_ota_updata_boot_info_over(void *priv)
{
    return 0;
}

int tws_verify_result_hdl(int calc_crc)
{
    u8 rsp_data[3];
    printf("tws_verify_result_hdl:%d\n", calc_crc);
    rsp_data[0] = OTA_TWS_VERIFY_RSP;
    rsp_data[1] = ++ sync_update_sn;
    if (calc_crc == 1) {
        rsp_data[2] = OTA_TWS_CMD_SUCC;
    } else {
        rsp_data[2] = OTA_TWS_CMD_ERR;
    }
    if (tws_ota_trans_to_sibling(rsp_data, 3)) {
        return -1;
    }
    if (calc_crc != 1) {
        if (support_dual_bank_update_no_erase) {
            dual_bank_check_flash_update_area(1);
        }
        // 添加写保护
        norflash_set_write_protect_en();
    }
    return 0;
}

int tws_verify_without_crc_result_hdl(int calc_crc)
{
    printf("tws_verify_without_crc_result_hdl, crc:%d", calc_crc);
    u8 rsp_data[2];
    rsp_data[0] = OTA_TWS_VERIFY_WITHOUT_CRC_RSP;
    if (calc_crc == 0) {
        rsp_data[1] = OTA_TWS_CMD_SUCC;
    } else {
        rsp_data[1] = OTA_TWS_CMD_ERR;
    }
    if (tws_ota_trans_to_sibling(rsp_data, 2)) {
        return -1;
    }
    return 0;
}

int dual_bank_curr_write_offset_callback(u32 curr_write_offset, u32 remain_len, void *priv);
static int tws_trans_data_write_callback(void *priv)
{
    u8 rsp_data[3];
    rsp_data[0] = OTA_TWS_TRANS_UPDATE_DATA_RSP;
    rsp_data[1] = ++ sync_update_sn;
    rsp_data[2] = OTA_TWS_CMD_SUCC;
    tws_ota_trans_to_sibling(rsp_data, 3);
    if (support_dual_bank_update_breakpoint) {
        dual_bank_curr_write_offset_callback(dual_bank_curr_write_offset_get(), 1, NULL);
    }
    return 0;
}

static int tws_trans_data_recv_data_again_callback(u16 pack_crc, u16 data_len, void *priv)
{
    u8 rsp_data[6];
    rsp_data[0] = OTA_TWS_TRANS_UPDATE_DATA_RECV_AGAIN;
    rsp_data[1] = ++ sync_update_sn;
    memcpy(&rsp_data[2], &pack_crc, sizeof(pack_crc));
    memcpy(&rsp_data[4], &data_len, sizeof(data_len));
    tws_ota_trans_to_sibling(rsp_data, 6);
    return 0;
}

static int tws_ota_recv_data_again_handle(void *recv_data, u16 len)
{
    if (len > 4) {
        u16 pack_crc = 0;
        u16 data_crc = CRC16(recv_data + 4, len - 4);
        memcpy(&pack_crc, &recv_data[2], sizeof(pack_crc));
        if (pack_crc != data_crc) {
            g_printf("err:tws ota recv data crc verify fail, %x, %x\n", pack_crc, data_crc);
            // 告诉主机需要重传，把数据长度和crc再次传给主机
            tws_trans_data_recv_data_again_callback(pack_crc, len - 4, NULL);
            return -1;
        }
    }
    return 0;
}

extern int dual_bank_curr_write_offset_callback(u32 curr_write_offset, u32 remain_len, void *priv);
extern int dual_bank_bp_fw_code_verify(u8 *remote_fw_code, u32 code_start_of_fw_file, u32 file_code_len);
static void deal_sibling_tws_ota_trans(void *data, u16 len)
{
    static u8 last_sync_update_sn = 0;
    int ret = 0;
    u8 rsp_data[3];
    u8 *recv_data = (u8 *)data;

    if (recv_data[1] == last_sync_update_sn) {        //处理同一包数据会出现两次回调的情况
        g_printf("recv_data == last_sync_update_sn:%d %d %d\n", recv_data[0], recv_data[1], last_sync_update_sn);
        free(data);
        return;
    }

    last_sync_update_sn = recv_data[1];
    /* log_info(">>>%s\n len:%d", __func__, len); */
    /* put_buf(data, len); */
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_ota_timeout_reset();                //从机接收命令超时
    }
    switch (recv_data[0]) {
    case OTA_TWS_START_UPDATE:
        g_printf("MSG_OTA_TWS_START_UPDATE\n");
        struct __tws_ota_para para;
        u32 write_offset = 0;
        memcpy(&para, recv_data + 2, sizeof(struct __tws_ota_para));
        printf("crc:%d fm_size:%d max_pkt_len:%d\n", para.fm_crc, para.fm_size, para.max_pkt_len);
        if (support_dual_bank_update_no_erase) {
            extern void verify_os_time_dly_set(u32 time);
            verify_os_time_dly_set(1); //校验每包延时10ms
        }
        if (support_dual_bank_update_breakpoint) {
            u8 *ptr = recv_data + 2 + sizeof(struct __tws_ota_para);
            memcpy(&write_offset, ptr, sizeof(write_offset));
            ptr += sizeof(write_offset);
            // 对fw校验码进行处理
            dual_bank_bp_fw_code_verify(ptr, 0, 0); // 如果只指被动升级，做不到根据fw校验码去区分升级文件
            if ((u32) - 1 == write_offset) {
                printf("err:tws ota breakpoint is enable, the last ufw is not match current ufw\n");
                g_tws_ota_addr_recore = -1;
                dual_bank_curr_write_offset_callback(0, 0, NULL);
                rsp_data[0] = OTA_TWS_START_UPDATE_RSP;
                rsp_data[1] = ++ sync_update_sn;
                rsp_data[2] = OTA_TWS_CMD_ERR;
                tws_ota_trans_to_sibling(rsp_data, 3);
                ota_event_update_to_user(OTA_UPDATE_ERR);
                break;
            }
        }
        // 如果没有打开被动升级断点续传功能，初始化是需要把记录清除
        dual_bank_curr_write_offset_set(write_offset);
        dual_bank_passive_update_init(para.fm_crc, para.fm_size, para.max_pkt_len, NULL);
        // 解除保护
        norflash_set_write_protect_remove();
        ret = dual_bank_update_allow_check(para.fm_size);
        if (ret) {
            rsp_data[0] = OTA_TWS_START_UPDATE_RSP;
            rsp_data[1] = ++ sync_update_sn;
            rsp_data[2] = OTA_TWS_CMD_ERR;
            ota_event_update_to_user(OTA_UPDATE_ERR);
        } else {
            rsp_data[0] = OTA_TWS_START_UPDATE_RSP;
            rsp_data[1] = ++ sync_update_sn;
            rsp_data[2] = OTA_TWS_CMD_SUCC;
        }
        tws_ota_trans_to_sibling(rsp_data, 3);
        break;
    case OTA_TWS_START_UPDATE_RSP:
        g_printf("MSG_OTA_TWS_START_UPDATE_RSP\n");
        if (recv_data[2] == OTA_TWS_CMD_SUCC) {
            os_sem_post(&tws_ota_sem);
        }
        break;
    case OTA_TWS_TRANS_UPDATE_DATA_VERIFY_CRC:
        printf("OTA_TWS_TRANS_UPDATE_DATA_VERIFY_CRC %d, %d\n", len, recv_data[1]);
        if (tws_ota_recv_data_again_handle(recv_data, len)) {
            break;
        }
        if (support_dual_bank_update_no_erase) {
            dual_bank_update_write_only(recv_data + 4, len - 4, tws_trans_data_write_callback);
        } else {
            dual_bank_update_write(recv_data + 4, len - 4, tws_trans_data_write_callback);
        }
        break;
    case OTA_TWS_TRANS_UPDATE_DATA_RECV_AGAIN:
        printf("OTA_TWS_TRANS_UPDATE_DATA_RECV_AGAIN:%d\n", recv_data[1]);
        tws_trans_data_send_data_again(recv_data + 2, len - 2);
        break;
    case OTA_TWS_TRANS_UPDATE_DATA:
        printf("MSG_OTA_TWS_TRANS_UPDATE_DATA %d, %d\n", len, recv_data[1]);
        if (support_dual_bank_update_no_erase) {
            dual_bank_update_write_only(recv_data + 2, len - 2, tws_trans_data_write_callback);
        } else {
            dual_bank_update_write(recv_data + 2, len - 2, tws_trans_data_write_callback);
        }
        break;
    case OTA_TWS_TRANS_UPDATE_DATA_RSP:
        printf("MSG_OTA_TWS_TRANS_UPDATE_DATA_RSP:%d\n", recv_data[1]);
        if (recv_data[2] == OTA_TWS_CMD_SUCC) {
            os_sem_post(&tws_ota_sem);
        }
        break;
    case OTA_TWS_VERIFY:
        g_printf("MSG_OTA_TWS_VERIFY\n");
        clock_alloc("sys", 120 * 1000000L);   //提升主频加快CRC校验速度
        if (support_dual_bank_update_breakpoint) {
            dual_bank_curr_write_offset_callback(0, 0, NULL);
            dual_bank_bp_fw_code_verify(NULL, 0, 0);
        }
        ret =  dual_bank_update_verify(sync_update_crc_init_hdl, sync_update_crc_calc_hdl, tws_verify_result_hdl);
        if (ret) {
            rsp_data[0] = OTA_TWS_VERIFY_RSP;
            rsp_data[1] = ++ sync_update_sn;
            rsp_data[2] = OTA_TWS_CMD_ERR;
            tws_ota_trans_to_sibling(rsp_data, 2);
        }
        break;
    case OTA_TWS_VERIFY_RSP:
        g_printf("MSG_OTA_TWS_VERIFY_RSP %d\n", recv_data[1]);
        if (recv_data[2] == OTA_TWS_CMD_SUCC) {
            os_sem_post(&tws_ota_sem);
        }
        break;
    case OTA_TWS_VERIFY_WITHOUT_CRC:
        g_printf("MSG_OTA_TWS_VERIFY_WITHOUT_CRC\n");
        clock_alloc("sys", 160 * 1000000L);   //提升主频加快CRC校验速度
        ret = dual_bank_update_verify_without_crc_new(tws_verify_without_crc_result_hdl);
        if (ret) {
            rsp_data[0] = OTA_TWS_VERIFY_WITHOUT_CRC_RSP;
            rsp_data[1] = OTA_TWS_CMD_ERR;
            tws_ota_trans_to_sibling(rsp_data, 2);
        }
        break;
    case OTA_TWS_VERIFY_WITHOUT_CRC_RSP:
        g_printf("MSG_OTA_TWS_VERIFY_WITHOUT_CRC_RSP\n");
        if (recv_data[1] == OTA_TWS_CMD_SUCC) {
            os_sem_post(&tws_ota_sem);
        }
        break;
    case OTA_TWS_WRITE_BOOT_INFO:
        g_printf("MSG_OTA_TWS_WRITE_BOOT_INFO\n");
        dual_bank_update_burn_boot_info(dual_bank_update_burn_boot_info_callback);
        break;
    case OTA_TWS_WRITE_BOOT_INFO_RSP:
        g_printf("MSG_OTA_TWS_WRITE_BOOT_INFO_RSP\n");
        if (recv_data[2] == OTA_TWS_CMD_SUCC) {
            os_sem_post(&tws_ota_sem);
        }
        break;
    case OTA_TWS_UPDATE_RESULT:
        g_printf("MSG_OTA_TWS_UPDATE_RESULT\n");
        if (recv_data[2] == OTA_TWS_CMD_SUCC) {
            ota_event_update_to_user(OTA_UPDATE_OVER);
        } else {
            ota_event_update_to_user(OTA_UPDATE_ERR);
        }
        break;

    case OTA_TWS_USER_CHIP_UPDATE:
        if (user_chip_tws_update_handle) {
            user_chip_tws_update_handle(recv_data + 2, len - 2);
        }
        break;
    }
    free(data);
}


int tws_ota_sync_cmd(int reason)
{
    switch (reason) {
    case SYNC_CMD_UPDATE_OVER:
        g_printf("SYNC_CMD_UPDATE_OVER\n");
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            update_result_set(UPDATA_SUCC);
        }
        cpu_reset();
        /* ota_event_update_to_user(OTA_UPDATE_OVER); */
        break;
    case SYNC_CMD_UPDATE_ERR:
        g_printf("SYNC_CMD_UPDATE_ERR\n");
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            update_result_set(UPDATA_DEV_ERR);
        } else {
            /* // 如果这里cpu_reset被注释掉，就打开这个，fw验证码校验不通过，从机就会触发擦除 */
            /* if (support_dual_bank_update_no_erase) { */
            /* 	if ((u8)-1 == g_tws_ota_addr_recore) { */
            /* 		dual_bank_check_flash_update_area(1); */
            /* 	} */
            /* } */
            /* // 添加写保护 */
            /* norflash_set_write_protect_en(); */
        }
        cpu_reset();
        /* ota_event_update_to_user(OTA_UPDATE_ERR); */
        break;
    }
    return 0;
}

static void rcsp_adv_tws_ota_sync_handler(int reason, int err)
{
    tws_ota_sync_cmd(reason);
}

update_op_tws_api_t update_tws_api = {
    .tws_ota_start = tws_ota_open,
    .tws_ota_data_send = tws_ota_data_send_m_to_s,
    .tws_ota_err = tws_ota_err_callback,
    .enter_verfiy_hdl = tws_ota_enter_verify,
    .exit_verify_hdl = tws_ota_exit_verify,
    .update_boot_info_hdl =  tws_ota_updata_boot_info_over,
    .tws_ota_result_hdl = tws_ota_result,
    .tws_ota_data_send_pend = tws_ota_data_send_pend,
    //for user_chip
    .tws_ota_user_chip_update_send = tws_ota_user_chip_update_send_m_to_s,
    .tws_ota_user_chip_update_send_data = tws_ota_data_send_with_crc_m_to_s,
};

update_op_tws_api_t *get_tws_update_api(void)
{
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        return &update_tws_api;
    } else {
        return NULL;
    }
}

static void deal_sibling_tws_ota_trans_in_irq(void *data, u16 len, bool rx)
{
    if (rx) {
        void *buf = malloc(len);
        if (buf == NULL) {
            printf("%s, malloc fail len:%d\n", __FUNCTION__, __LINE__);
            return;
        }
        memcpy(buf, data, len);
        int msg[] = {(int)deal_sibling_tws_ota_trans, 2, (int)buf, len};
        os_taskq_post_type("app_core", Q_CALLBACK, ARRAY_SIZE(msg), msg);
    }
}

REGISTER_TWS_FUNC_STUB(tws_ota_trans) = {
    .func_id = TWS_FUNC_ID_OTA_TRANS,
    .func    = deal_sibling_tws_ota_trans_in_irq,
};

TWS_SYNC_CALL_REGISTER(rcsp_adv_tws_ota_sync) = {
    .uuid = 0xA2E22223,
    .task_name = "app_core",
    .func = rcsp_adv_tws_ota_sync_handler,
};

void tws_update_register_user_chip_update_handle(void (*update_handle)(void *data, u32 len))
{
    user_chip_tws_update_handle = update_handle;
}

#endif


