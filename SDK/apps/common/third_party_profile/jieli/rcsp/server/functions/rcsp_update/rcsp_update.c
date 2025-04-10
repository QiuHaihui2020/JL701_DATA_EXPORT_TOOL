#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_update.data.bss")
#pragma data_seg(".rcsp_update.data")
#pragma const_seg(".rcsp_update.text.const")
#pragma code_seg(".rcsp_update.text")
#endif
#include "app_config.h"
#include "rcsp_update.h"
#if (RCSP_MODE && RCSP_UPDATE_EN && !RCSP_BLE_MASTER)
#include "uart.h"
#include "system/timer.h"
#include "update.h"
#include "custom_cfg.h"
#include "btstack/avctp_user.h"
#include "ble_rcsp_server.h"
#include "rcsp_manage.h"
#include "rcsp_bt_manage.h"
#include "update_loader_download.h"
#include "ble_rcsp_server.h"
#include "classic/tws_api.h"
#include "rcsp_task.h"
#include "rcsp_config.h"
#include "syscfg_id.h"
#include "rcsp_device_status.h"
#include "JL_rcsp_protocol.h"
#include "rcsp_config.h"
#include "rcsp_ch_loader_download.h"
#include "btstack_rcsp_user.h"
#if RCSP_ADV_EN
#include "rcsp_setting_opt.h"
#endif

#if	TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if (RCSP_MODE == RCSP_MODE_WATCH)
#include "rcsp_extra_flash_opt.h"
#endif

#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)
#include "rcsp_misc_setting.h"
#endif


#if (TCFG_MIC_EFFECT_ENABLE)
#include "mic_effect.h"
#endif

#define RCSP_DEBUG_EN
#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

#define DEV_UPDATE_FILE_INFO_OFFEST  0x00//0x40
#define DEV_UPDATE_FILE_INFO_LEN     0x00//(0x10 + VER_INFO_EXT_COUNT * (VER_INFO_EXT_MAX_LEN + 1))

typedef enum {
    UPDATA_START = 0x00,
    UPDATA_REV_DATA,
    UPDATA_STOP,
} UPDATA_BIT_FLAG;

static update_file_ext_id_t update_file_id_info = {
    .update_file_id_info.ver[0] = 0xff,
    .update_file_id_info.ver[1] = 0xff,
};

extern const int support_dual_bank_update_en;
extern void ble_app_disconnect(void);
extern void updata_parm_set(UPDATA_TYPE up_type, void *priv, u32 len);
extern u8 check_le_pakcet_sent_finish_flag(void);
extern void rcsp_update_ancs_disconn_handler(void);

static u8 update_flag = 0;
static u8 tws_need_update = 0;             //标志耳机是否需要强制升级

#if RCSP_MODE == RCSP_MODE_EARPHONE
static u8 tws_need_role_switch = 0;	 	   //用于tws连接之后版本号不匹配进行role_switch，低版本号的作为主机进行强制升级
#endif

enum {
    UPDATE_FLAG_OK,
    UPDATE_FLAG_LOW_POWER,
    UPDATE_FLAG_FW_INFO_ERR,
    UPDATE_FLAG_FW_INFO_CONSISTENT,
    UPDATE_FLAG_TWS_DISCONNECT,
    UPDATE_FLAG_TWS_NOT_IN_CHARGESTORE,
    UPDATE_FLAG_ALREADY,						//已有升级任务
    UPDATE_FLAG_MULTI_CONN_DEVICE,				//多个蓝牙连接设备不允许升级
};

static void rcsp_update_prepare()
{
    ///升级前可以选择关闭需要关闭的模块
#if RCSP_DEVICE_STATUS_ENABLE
    rcsp_device_status_setting_stop();
#endif

#if (RCSP_MODE != RCSP_MODE_SOUNDBOX)
    // 关闭混响
    extern void rcsp_close_reverbrateion_state_and_update(void);
    rcsp_close_reverbrateion_state_and_update();
#endif

#if (SOUNDCARD_ENABLE)
    extern void soundcard_close(void);
    soundcard_close();
    return ;
#endif

#if (TCFG_MIC_EFFECT_ENABLE && (0 == RCSP_REVERBERATION_SETTING))
    if (mic_effect_player_runing()) {
        mic_effect_player_close();
    }
#endif

}

static void rcsp_update_fail_and_resume(void)
{
#if (TCFG_MIC_EFFECT_ENABLE)
    mic_effect_player_open();
#endif

#if (SOUNDCARD_ENABLE)
    extern void soundcard_start(void);
    soundcard_start();
#endif
}

u8 get_jl_update_flag(void)
{
    printf("get_update_flag:%x\n", update_flag);
    return update_flag;
}

void set_jl_update_flag(u8 flag)
{
    update_flag = flag;
    printf("update_flag:%x\n", update_flag);
}

/**
 * @abstract 获取当前连接设备类型
 *
 * @return 0:ble,1:spp
 */
static u8 device_type = 0;
static void set_curr_update_type(u8 type)
{
    device_type = type;
}

/**
 * @abstract 获取当前连接设备类型
 *
 * @return 0:ble,1:spp
 */
u8 get_curr_device_type(void)
{
    return device_type;
}

#if (RCSP_MODE == RCSP_MODE_WATCH)

void set_update_ex_flash_flag(u8 update_flag)
{
    // 读取vm，如果相同不设置，如果不同设置vm
    u8 tmp_flag = 0;
    syscfg_read(VM_RESET_EX_FLASH_FLAG, &tmp_flag, sizeof(tmp_flag));
    if (tmp_flag != update_flag) {
        syscfg_write(VM_RESET_EX_FLASH_FLAG, &update_flag, sizeof(update_flag));
    }
}

u8 get_update_ex_flash_flag(void)
{
    // 从vm中获取标志位
    u8 update_flag = 0;
    u8 tmp_flag = -1;
    if (sizeof(tmp_flag) ==
        syscfg_read(VM_RESET_EX_FLASH_FLAG, &tmp_flag, sizeof(tmp_flag))) {
        update_flag = tmp_flag;
    }
    return update_flag;
}

#endif // (RCSP_MODE == RCSP_MODE_WATCH)

typedef struct _update_mode_t {
    u8 opcode;
    u8 opcode_sn;
    u16 ble_con_handle;
    u8 spp_remote_addr[6];

} update_mode_t;

static update_mode_t update_record_info;

#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)

#define RCSP_UPDATE_WAIT_SPP_DISCONN_TIME		(1000)
static bool check_edr_is_disconnct(void);
static void rcsp_update_private_param_fill(UPDATA_PARM *p);
static void rcsp_update_before_jump_handle(int type);
static void wait_response_and_disconn_spp(void *priv)
{
    static u32 wait_time = 0;
    if (check_edr_is_disconnct()) {
        rcsp_printf("b");
        if (wait_time > RCSP_UPDATE_WAIT_SPP_DISCONN_TIME) {
            wait_time = 0;
            bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
        } else {
            wait_time += 100;
        }
        sys_timeout_add(NULL, wait_response_and_disconn_spp, 100);
        return;
    }

    wait_time = 0;
    rcsp_printf("BLE_APP_UPDATE\n");
    update_mode_api_v2(BLE_APP_UPDATA,
                       rcsp_update_private_param_fill,
                       rcsp_update_before_jump_handle);
}
#endif // (RCSP_MODE == RCSP_MODE_SOUNDBOX)

static void JL_controller_save_curr_cmd_para(u8 OpCode, u8 OpCode_SN)
{
    update_record_info.opcode = OpCode;
    update_record_info.opcode_sn = OpCode_SN;
}

static void JL_controller_get_curr_cmd_para(u8 *OpCode, u8 *OpCode_SN)
{
    *OpCode = update_record_info.opcode;
    *OpCode_SN = update_record_info.opcode_sn;
}

static void (*fw_update_block_handle)(u8 state, void *buf, int len) = NULL;
static void register_receive_fw_update_block_handle(void (*handle)(u8 state, void *buf, int len))
{
    fw_update_block_handle = handle;
}

static u16  ble_discon_timeout;
static void ble_discon_timeout_handle(void *priv)
{
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
}

static u16 wait_response_timeout;
static void wait_response_and_disconn_ble(void *priv)
{
    rcsp_printf("W");
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_DEV_DISCONNECT, NULL, 0);
}

static void rcsp_update_before_jump_handle(int type)
{
#if CONFIG_UPDATE_JUMP_TO_MASK
    void latch_reset();
    latch_reset();
#else
    cpu_reset();
#endif
}

static void rcsp_wait_reboot_dev(void *priv)
{
    if (NULL == priv && !rcsp_send_list_is_empty()) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    extern void ble_module_enable(u8 en);
    ble_module_enable(0);
#if CONFIG_UPDATE_JUMP_TO_MASK
    void latch_reset();
    latch_reset();
#else
    cpu_reset();
#endif
}

static void rcsp_rcsp_reboot_dev(void)
{
    extern void adv_edr_name_change_now(void);
    adv_edr_name_change_now();
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
#if RCSP_ADV_EN
        modify_bt_name_and_reset(500);
#endif
    } else {
        if (sys_timer_add(NULL, rcsp_wait_reboot_dev, 500) == 0) {
            rcsp_wait_reboot_dev((void *)1);
        }
    }
#else
    if (sys_timer_add(NULL, rcsp_wait_reboot_dev, 500) == 0) {
        rcsp_wait_reboot_dev((void *)1);
    }
#endif

}

extern void rcsp_set_update_hdl(u16 ble_con_handle, u8 *spp_remote_addr);
extern void rcsp_get_update_hdl(u16 *ble_con_handle, u8 *spp_remote_addr);
void rcsp_loader_download_result_handle(void *priv, u8 type, u8 cmd);
u32 rcsp_update_status_response(void *priv, u8 status);
u32 rcsp_update_data_read(void *priv, u32 offset_addr, u16 len);
extern void rcsp_update_handle(u8 state, void *buf, int len);
extern void rcsp_update_data_api_register(u32(*data_send_hdl)(void *priv, u32 offset, u16 len), u32(*send_update_handl)(void *priv, u8 state));
extern void bt_ble_rcsp_adv_disable(void);

#if RCSP_MODE == RCSP_MODE_EARPHONE
extern void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms);
#elif RCSP_MODE == RCSP_MODE_SOUNDBOX
extern void bt_set_low_latency_mode(int enable);
#endif

void JL_resp_inquire_device_if_can_update(u8 OpCode, u8 OpCode_SN, u8 update_sta, u16 ble_con_handle, u8 *spp_remote_addr);
void JL_rcsp_resp_dev_update_file_info_offest(u8 OpCode, u8 OpCode_SN, u16 ble_con_handle, u8 *spp_remote_addr);
int JL_rcsp_update_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    if ((OpCode >= JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET) && \
        (OpCode <= JL_OPCODE_SET_DEVICE_REBOOT)) {
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            if (!(support_dual_bank_update_en && (OTA_TWS_SAME_TIME_ENABLE == 0))) {   //如果双备份且没有打开TWS_SAME_TIME_ENABLE还是需要监听,应该是需要打开TWS_SAME_TIME_ENABEL,不然BLE传输只能升级一边
                putchar('&');
                return 0;
            }
        }
#endif
    }
    int ret = 0;
    u8 msg[5];
    rcsp_printf("%s\n", __FUNCTION__);
    switch (OpCode) {
    case JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET:
        if (0 == len) {
            rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET\n");
            JL_rcsp_resp_dev_update_file_info_offest(OpCode, OpCode_SN, ble_con_handle, spp_remote_addr);

        } else {
            rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET ERR\n");
        }
        break;
    case JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE:
        rcsp_printf("JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE:%x %x\n", len, data[0]);
        if (len) {
            u8 can_update_flag = UPDATE_FLAG_FW_INFO_ERR;
            set_curr_update_type(data[0]);
#if (TCFG_USER_TWS_ENABLE && OTA_TWS_SAME_TIME_ENABLE)
            int get_bt_tws_connect_status();
            if (get_bt_tws_connect_status() || (!support_dual_bank_update_en)) {
                can_update_flag = UPDATE_FLAG_OK;
            } else {
                can_update_flag = UPDATE_FLAG_TWS_DISCONNECT;
            }
#else
            can_update_flag = UPDATE_FLAG_OK;
#endif // (TCFG_USER_TWS_ENABLE && OTA_TWS_SAME_TIME_ENABLE)

            // 1t2判断是否已有升级任务
            u8 cur_con_dev = bt_rcsp_device_conn_num();
            if (cur_con_dev > 1) {
                can_update_flag = UPDATE_FLAG_MULTI_CONN_DEVICE;
            }
            if (UPDATE_FLAG_OK == can_update_flag) {
                u8 max_con_dev = rcsp_max_support_con_dev_num();
                if (max_con_dev > 1) {
                    u16 current_update_ble_con_hdl = 0;
                    u8 current_update_spp_addr[6] = {0};
                    u8 _addr_temp[6] = {0};
                    rcsp_get_update_hdl(&current_update_ble_con_hdl, current_update_spp_addr);
                    if (current_update_ble_con_hdl || memcmp(current_update_spp_addr, _addr_temp, 6)) {
                        can_update_flag = UPDATE_FLAG_ALREADY;
                    }
                }
            }

            //todo;judge voltage
            JL_resp_inquire_device_if_can_update(OpCode, OpCode_SN, can_update_flag, ble_con_handle, spp_remote_addr);

            if (UPDATE_FLAG_OK != can_update_flag) {
                break;
            }
            rcsp_set_update_hdl(ble_con_handle, spp_remote_addr);
            rcsp_update_prepare();

            if (0 == support_dual_bank_update_en) {
#if RCSP_MODE == RCSP_MODE_EARPHONE
                if (tws_api_get_role() == TWS_ROLE_MASTER) {
                    g_printf("tws master start update...\n");
                    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_LOADER_DOWNLOAD_START, NULL, 0);
                    //需要通知从机进入了升级
                    u8 data = TWS_UPDATE_INFO;
                    tws_api_send_data_to_sibling(&data, sizeof(data), TWS_FUNC_ID_SEQ_RAND_SYNC);
                } else {
                    bt_ble_rcsp_adv_disable();
                    ble_module_enable(0);               //关闭广播防止从机被手机误回连
                    r_printf("slave close adv...\n");
                    sys_timeout_add(NULL,  update_slave_adv_reopen, 1000 * 60);     //延迟一分钟再开广播
                }
                if (RCSP_USE_SPP == get_curr_device_type()) {
#if TCFG_USER_TWS_ENABLE
                    tws_api_detach(TWS_DETACH_BY_LOCAL, 5000); //单备份升级断开tws
                    tws_cancle_all_noconn();
#endif
                }

#else

#if (TCFG_USER_TWS_ENABLE == 1)
                if (!tws_api_get_role()) {
#else
                if (1) {
#endif
                    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_LOADER_DOWNLOAD_START, NULL, 0);
                } else {
                    tws_need_update = 1;                //置上该标志位APP重新连接的时候强制进入升级
                }

#endif
            }

        }
        break;
    case JL_OPCODE_EXIT_UPDATE_MODE:
        rcsp_printf("JL_OPCODE_EXIT_UPDATE_MODE\n");
        break;
    case JL_OPCODE_ENTER_UPDATE_MODE:
        rcsp_printf("JL_OPCODE_ENTER_UPDATE_MODE\n");
#if RCSP_MODE == RCSP_MODE_EARPHONE
        bt_set_low_latency_mode(0, 0, 0);
#elif RCSP_MODE == RCSP_MODE_SOUNDBOX
        bt_set_low_latency_mode(0);
#endif

#if TCFG_USER_TWS_ENABLE
        if (support_dual_bank_update_en && !tws_api_get_role()) {
#else
        if (support_dual_bank_update_en) {
#endif
            u8 status = 0;
#if JL_RCSP_EXTRA_FLASH_OPT
            if (!get_update_ex_flash_flag()) {
                register_user_chip_update_handle(NULL);
                if (rcsp_eflash_update_flag_get()) {
                    extern void app_rcsp_task_disable_opt(void);
                    app_rcsp_task_disable_opt();
                    app_rcsp_task_prepare(0, RCSP_TASK_ACTION_WATCH_TRANSFER, 0);
                }
            } else if (0 == get_curr_platform()) {
#if UPDATE_EX_FALSH_USE_4K_BUF
                /* u32 update_once_req_size = rcsp_packet_write_alloc_len() / 256 * 256; */
                u32 update_once_req_size = 4096;
                extern void ex_flash_update_once_req_size_set(u32 pack_size);
                ex_flash_update_once_req_size_set(update_once_req_size);
                set_jl_mtu_resv(rcsp_packet_write_alloc_len());
                rcsp_update_change_mtu(rcsp_packet_write_alloc_len());
#endif
            }
#endif
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &status, 1, ble_con_handle, spp_remote_addr);
            rcsp_update_data_api_register(rcsp_update_data_read, rcsp_update_status_response);
            register_receive_fw_update_block_handle(rcsp_update_handle);
            printf("rcsp_update_loader_download_init %d\n", __LINE__);
            rcsp_update_loader_download_init(DUAL_BANK_UPDATA, rcsp_loader_download_result_handle);
        }
        break;
    case JL_OPCODE_SEND_FW_UPDATE_BLOCK:
        rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK\n");
        break;
    case JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS:
        rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS\n");
        JL_controller_save_curr_cmd_para(OpCode, OpCode_SN);
        if (fw_update_block_handle) {
            fw_update_block_handle(UPDATA_STOP, NULL, 0);
        }
        break;
    case JL_OPCODE_SET_DEVICE_REBOOT:
        rcsp_printf("JL_OPCODE_SET_DEVICE_REBOOT\n");
        JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);
        rcsp_rcsp_reboot_dev();

        break;
#if 0//TCFG_RCSP_DUAL_CONN_ENABLE
    case JL_OPCODE_CHECK_DEVICE_CONN_NUM:
        rcsp_printf("JL_OPCODE_CHECK_DEVICE_CONN_NUM\n");
        u8 cur_con_dev = bt_rcsp_device_conn_num();
        if (cur_con_dev > 1) {
            // 踢掉另一个连接的设备
            u8 first_flag = data[0];
            if (first_flag) {
                u8 *remote_addr = spp_remote_addr;
                if (ble_con_handle) {
                    remote_addr = rcsp_get_ble_hdl_remote_mac_addr(ble_con_handle);
                }
                u8 *other_conn_addr = btstack_get_other_dev_addr(remote_addr);
                if (other_conn_addr) {
                    btstack_device_detach(btstack_get_conn_device(other_conn_addr));
                }
                rcsp_disconn_other_ble(ble_con_handle);
            }
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &cur_con_dev, 1, ble_con_handle, spp_remote_addr);
        } else {
            // 通知手机可以进入升级了
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, &cur_con_dev, 1, ble_con_handle, spp_remote_addr);
        }
        break;
#endif
    default:
        ret = 1;
        break;
    }
    return ret;
}

static void JL_rcsp_resp_dev_update_file_info_offest(u8 OpCode, u8 OpCode_SN, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 data[4 + 2];
    u16 update_file_info_offset = DEV_UPDATE_FILE_INFO_OFFEST;
    u16 update_file_info_len = DEV_UPDATE_FILE_INFO_LEN;
    WRITE_BIG_U32(data + 0, update_file_info_offset);
    WRITE_BIG_U16(data + 4, update_file_info_len);
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data), ble_con_handle, spp_remote_addr);
}

static void JL_resp_inquire_device_if_can_update(u8 OpCode, u8 OpCode_SN, u8 update_sta, u16 ble_con_handle, u8 *spp_remote_addr)
{
    u8 data[1];
    data[0] = update_sta;
    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data), ble_con_handle, spp_remote_addr);
}

static u8 judge_remote_version_can_update(void)
{
    //extern u16 ex_cfg_get_local_version_info(void);
    u16 remote_file_ver = READ_BIG_U16(update_file_id_info.update_file_id_info.ver);
    //u16 local_ver = ex_cfg_get_local_version_info();
    u16 local_ver = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);

#if (0 == VER_INFO_EXT_COUNT)
    //extern u16 ex_cfg_get_local_pid_info(void);
    //extern u16 ex_cfg_get_local_vid_info(void);

    //u16 local_pid = ex_cfg_get_local_pid_info();
    //u16 local_vid = ex_cfg_get_local_vid_info();
    u16 local_pid = get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    u16 local_vid = get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);

    u16 remote_file_pid = READ_BIG_U16(update_file_id_info.update_file_id_info.pid);
    u16 remote_file_vid = READ_BIG_U16(update_file_id_info.update_file_id_info.vid);

    if (remote_file_ver > local_ver || remote_file_pid != local_pid || remote_file_vid != local_vid) {
        return UPDATE_FLAG_FW_INFO_ERR;
    }
#else
    //extern u32 ex_cfg_get_local_authkey_info(u8 * authkey_data[], u8 * authkey_len);
    //extern u32 ex_cfg_get_local_procode_info(u8 * procode_data[], u8 * procode_len);

    u8 authkey_len = 0;
    u8 *local_authkey_data = NULL;
    get_authkey_procode_from_cfg_file(&local_authkey_data, &authkey_len, GET_AUTH_KEY_FROM_EX_CFG);

    u8 procode_len = 0;
    u8 *local_procode_data = NULL;
    get_authkey_procode_from_cfg_file(&local_procode_data, &procode_len, GET_PRO_CODE_FROM_EX_CFG);

    //ex_cfg_get_local_authkey_info(&local_authkey_data, &authkey_len);
    //ex_cfg_get_local_procode_info(&local_procode_data, &procode_len);

    u8 *remote_authkey_data = update_file_id_info.ext;
    u8 *remote_procode_data = update_file_id_info.ext + authkey_len + 1;

    if (remote_file_ver < local_ver
        || 0 != memcmp(remote_authkey_data, local_authkey_data, authkey_len)
        || 0 != memcmp(remote_procode_data, local_procode_data, procode_len)) {
        return UPDATE_FLAG_FW_INFO_ERR;
    }
#endif

    if (remote_file_ver == local_ver) {
        rcsp_printf("remote_file_ver is %x, local_ver is %x, remote_file_ver is similar to local_ver\n", remote_file_ver, local_ver);
        return UPDATE_FLAG_FW_INFO_CONSISTENT;
    }

    return UPDATE_FLAG_OK;
}

static bool check_edr_is_disconnct(void)
{
    if (bt_get_curr_channel_state()) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static bool check_ble_all_packet_sent(void)
{
    if (check_le_pakcet_sent_finish_flag()) {
        return TRUE;
    } else {
        return FALSE;
    }
}


static u32 rcsp_update_data_read(void *priv, u32 offset_addr, u16 len)
{
    u32 err;
    u8 data[4 + 2];
    WRITE_BIG_U32(data, offset_addr);
    WRITE_BIG_U16(data + 4, len);
    u16 current_update_ble_con_hdl = 0;
    u8 current_update_spp_addr[6] = {0};
    rcsp_get_update_hdl(&current_update_ble_con_hdl, current_update_spp_addr);
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* u8 _addr_temp[6] = {0}; */
    /* if (memcmp(current_update_spp_addr, _addr_temp, 6)) { */
    /* 	put_buf(current_update_spp_addr, 6); */
    /* } */
    err = JL_CMD_send(JL_OPCODE_SEND_FW_UPDATE_BLOCK, data, sizeof(data), JL_NEED_RESPOND, current_update_ble_con_hdl, current_update_spp_addr);

    return err;
}

static JL_ERR JL_controller_resp_get_dev_refresh_fw_status(u8 OpCode, u8 OpCode_SN, u8 result)
{
    JL_ERR send_err = JL_ERR_NONE;
    u8 data[1];

    data[0] = result;	//0:sucess 1:fail;
    u16 current_update_ble_con_hdl = 0;
    u8 current_update_spp_addr[6] = {0};
    rcsp_get_update_hdl(&current_update_ble_con_hdl, current_update_spp_addr);
    send_err = JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, sizeof(data), current_update_ble_con_hdl, current_update_spp_addr);

    return send_err;
}

static u32 rcsp_update_status_response(void *priv, u8 status)
{
    u8 OpCode;
    u8 OpCode_SN;

    JL_ERR send_err = JL_ERR_NONE;

    JL_controller_get_curr_cmd_para(&OpCode, &OpCode_SN);

    //log_info("get cmd para:%x %x\n", OpCode, OpCode_SN);

    if (JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS == OpCode) {
        send_err = JL_controller_resp_get_dev_refresh_fw_status(OpCode, OpCode_SN, status);
    }

    return send_err;
}



int JL_rcsp_update_cmd_receive_resp(void *priv, u8 OpCode, u8 status, u8 *data, u16 len)
{
    int ret = 0;
    switch (OpCode) {
    case JL_OPCODE_SEND_FW_UPDATE_BLOCK:
        if (fw_update_block_handle) {
            fw_update_block_handle(UPDATA_REV_DATA, data, len);
        }
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

static void rcsp_loader_download_result_handle(void *priv, u8 type, u8 cmd)
{
    if (UPDATE_LOADER_OK == cmd) {
        //JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP,MSG_JL_UPDATE_START,NULL,0);
        set_jl_update_flag(1);
        if (support_dual_bank_update_en) {
            rcsp_printf(">>>rcsp update succ\n");
            update_result_set(UPDATA_SUCC);
        }
    } else {
        rcsp_printf(">>>update loader err\n");
#if 1//(RCSP_MODE == RCSP_MODE_SOUNDBOX)
        rcsp_update_fail_and_resume();
#endif
        /* #if OTA_TWS_SAME_TIME_ENABLE */
        /*         if((tws_ota_control(OTA_TYPE_GET) == OTA_TWS)) { */
        /*             tws_ota_stop(OTA_STOP_UPDATE_OVER_ERR); */
        /*         } */
        /* #endif */
        //rcsp_db_update_fail_deal();
    }
}

extern u32 ex_cfg_fill_content_api(void);
static void rcsp_update_private_param_fill(UPDATA_PARM *p)
{
    u32 exif_addr = ex_cfg_fill_content_api();
    memcpy(p->parm_priv, (u8 *)&exif_addr, sizeof(exif_addr));
}

static void rcsp_update_change_mtu(u16 new_mtu)
{
    u8 buf[2] = {0};
    buf[0] = ((u8 *)&new_mtu)[1];
    buf[1] = ((u8 *)&new_mtu)[0];
    u16 current_update_ble_con_hdl = 0;
    u8 current_update_spp_addr[6] = {0};
    rcsp_get_update_hdl(&current_update_ble_con_hdl, current_update_spp_addr);
    JL_CMD_send(JL_OPCODE_NOTIFY_MTU, (u8 *)buf, sizeof(buf), JL_NOT_NEED_RESPOND, current_update_ble_con_hdl, current_update_spp_addr);
}

extern void register_user_chip_update_handle(const user_chip_update_t *user_update_ins);
extern void bt_adv_seq_change(void);
extern int bt_tws_poweroff();
extern void bt_wait_phone_connect_control(u8 enable);
extern int tws_api_get_role(void);
extern void tws_cancle_all_noconn();
extern void bt_ble_rcsp_adv_enable(void);
extern u32 ex_cfg_fill_content_api(void);
extern void update_param_priv_fill(UPDATA_PARM *p, void *priv, u16 priv_len);

#if RCSP_MODE == RCSP_MODE_EARPHONE

u8 rcsp_get_update_flag(void)
{
    return tws_need_update;
}

void rcsp_set_update_flag(u8 flag)
{
    tws_need_update = flag;
}

u8 rcsp_update_get_role_switch(void)
{
    return tws_need_role_switch;
}

void rcsp_update_set_role_switch(u8 sw)
{
    tws_need_role_switch = sw;
}

void update_slave_adv_reopen(void *priv)
{
    r_printf("slave reopen adv...\n");
    ble_module_enable(1);
    bt_ble_rcsp_adv_enable();
}

#endif

int JL_rcsp_update_msg_deal(void *hdl, u8 event, u8 *msg)
{
    int ret = 0;
    u16 remote_file_version;
    u8 can_update_flag = UPDATE_FLAG_FW_INFO_ERR;

    printf("---%s --- %d   %d\n", __func__, __LINE__, event);
    switch (event) {
    case MSG_JL_DEV_DISCONNECT:
        rcsp_printf("MSG_JL_DEV_DISCONNECT\n");
        static u8 wait_cnt = 0;
        if ((10 == wait_cnt) || (rcsp_send_list_is_empty() && check_ble_all_packet_sent())) {
            wait_cnt = 0;
            ble_app_disconnect();
            if (check_edr_is_disconnct()) {
                puts("-need discon edr\n");
                bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
            }
            ble_discon_timeout = sys_timeout_add(NULL, ble_discon_timeout_handle, 1000);
        } else {
            wait_cnt++;
            wait_response_timeout = sys_timeout_add(NULL, wait_response_and_disconn_ble, 100);
        }
        break;

    case MSG_JL_LOADER_DOWNLOAD_START:
        printf("---%s --- %d\n", __func__, __LINE__);
        rcsp_update_data_api_register(rcsp_update_data_read, rcsp_update_status_response);
        register_receive_fw_update_block_handle(rcsp_update_handle);
#if (RCSP_MODE == RCSP_MODE_WATCH)
        u32 update_once_req_size = 4096;
        extern void ex_flash_update_once_req_size_set(u32 pack_size);
        ex_flash_update_once_req_size_set(update_once_req_size);
        set_jl_mtu_resv(rcsp_packet_write_alloc_len());
        rcsp_update_change_mtu(rcsp_packet_write_alloc_len());
#endif
        if (RCSP_USE_BLE == get_curr_device_type()) {
            printf("rcsp_update_loader_download_init %d\n", __LINE__);
            rcsp_update_loader_download_init(BLE_APP_UPDATA, rcsp_loader_download_result_handle);
        } else if (RCSP_USE_SPP == get_curr_device_type()) {
            printf("rcsp_update_loader_download_init %d\n", __LINE__);
            rcsp_update_loader_download_init(SPP_APP_UPDATA, rcsp_loader_download_result_handle);
        }
        break;

    case MSG_JL_UPDATE_START:
        rcsp_printf("MSG_JL_UPDATE_START\n");
        static u8 update_wait_cnt = 0;
        // loader加载完成，开始进入loader升级
        if ((100 < update_wait_cnt) && check_edr_is_disconnct()) {
            rcsp_printf("b");
            update_wait_cnt++;
            JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0);
            break;
        } else {
            update_wait_cnt = 0;
        }

        // 单备份的loader都是使用ble
        rcsp_printf("BLE_APP_UPDATE\n");
        update_mode_api_v2(BLE_APP_UPDATA,
                           rcsp_update_private_param_fill,
                           rcsp_update_before_jump_handle);
        /* sys_timeout_add(NULL, wait_response_and_disconn_spp, 100); */

        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}


static u8 rcsp_update_flag = 0;
void set_rcsp_db_update_status(u8 value)
{
    rcsp_update_flag = value;
}

u8 get_rcsp_db_update_status()
{
    return rcsp_update_flag;
}

void rcsp_before_enter_db_update_mode() //进入双备份升级前
{
    r_printf("%s", __func__);
    rcsp_update_flag = 1;
    void sys_auto_shut_down_disable(void);
    if (bt_get_total_connect_dev() == 0) {
        sys_auto_shut_down_disable();
    }
#if TCFG_USER_TWS_ENABLE
    int tws_api_get_role(void);
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
#endif
#if !TCFG_USER_TWS_ENABLE
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
#endif
}

extern void sys_auto_shut_down_enable(void);
void rcsp_db_update_fail_deal() //双备份升级失败处理
{
    r_printf("%s", __func__);
    if (rcsp_update_flag) {
        rcsp_update_flag = 0;
        if (bt_get_total_connect_dev() == 0) {
            sys_auto_shut_down_enable();
        }
    }
    /* cpu_reset(); //升级失败直接复位 */
}

#else // (RCSP_MODE && RCSP_UPDATE_EN && !RCSP_BLE_MASTER)

//以下函数是为了编译通过
int JL_rcsp_update_msg_deal(void *hdl, u8 event, u8 *msg)
{
    printf("%s:NULL", __func__);
    return 0;
}
void set_jl_update_flag(u8 flag)
{
    printf("%s:NULL", __func__);
}
void update_slave_adv_reopen(void *priv)
{
    printf("%s:NULL", __func__);
}
void rcsp_set_update_flag(u8 flag)
{
    printf("%s:NULL", __func__);
}
void rcsp_update_set_role_switch(u8 sw)
{
    printf("%s:NULL", __func__);
}
u8 get_jl_update_flag(void)
{
    printf("%s:NULL", __func__);
    return 0;
}
u8 rcsp_get_update_flag(void)
{
    printf("%s:NULL", __func__);
    return 0;
}

#endif // (RCSP_MODE && RCSP_UPDATE_EN && !RCSP_BLE_MASTER)

