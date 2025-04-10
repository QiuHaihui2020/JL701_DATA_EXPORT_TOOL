#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".update.data.bss")
#pragma data_seg(".update.data")
#pragma const_seg(".update.text.const")
#pragma code_seg(".update.text")
#endif
#include "update.h"
#include "update_loader_download.h"
#include "crc.h"
#include "asm/wdt.h"
#include "os/os_api.h"
#include "os/os_cpu.h"
#include "app_config.h"
#include "cpu.h"
#include "syscfg_id.h"
#include "btcontroller_modules.h"
#include "system/includes.h"
/* #include "uart_update.h" */
#include "update_interactive_uart.h"
#include "dual_bank_updata_api.h"
#include "btstack/avctp_user.h"
#include "poweroff.h"
#include "app_main.h"
#include "vm.h"
#include "bt_common.h"
#include "boot.h"
#include "asm/sfc_norflash_api.h"

#if TCFG_MIC_EFFECT_ENABLE
#include "mic_effect.h"
#endif

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#endif

#ifdef UPDATE_VOICE_REMIND
#include "app_tone.h"
#include "audio_config.h"
#endif

#include "custom_cfg.h"

#define LOG_TAG "[APP-UPDATE]"
#define LOG_INFO_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

#if (JL_RCSP_EXTRA_FLASH_OPT)
#include "rcsp_extra_flash_opt.h"
#endif

#define LOADER_NAME		"LOADER.BIN"
#define DEVICE_UPDATE_KEY_ERR  BIT(30)
#define DEVICE_FIRST_START     BIT(31)

extern void update_module_init(void (*cbk)(update_mode_info_t *, u32, void *));
extern void testbox_update_init(void);
extern void ll_hci_destory(void);
extern void hci_controller_destory(void);
extern const int support_norflash_update_en;
extern const int support_dual_bank_update_no_erase;
extern void ram_protect_close(void);
extern void hwi_all_close(void);
extern void wifi_det_close();

__attribute__((weak))
void wifi_det_close()
{
    printf("tmp weak func wifi_det_close\n");
}
extern void *dev_update_get_parm(int type);
extern u8 get_ota_status();
extern int get_nor_update_param(void *buf);
extern bool get_tws_phone_connect_state(void);
extern void tws_sniff_controle_check_disable(void);
extern void sys_auto_sniff_controle(u8 enable, u8 *addr);
extern void app_audio_set_wt_volume(s16 volume);
extern u8 get_max_sys_vol(void);
extern u8 get_bt_trim_info_for_update(u8 *res);


extern const int support_norflash_update_en;
const u8 loader_file_path[] = "mnt/norflash/C/"LOADER_NAME"";
//升级文件路径必须是短文件名（8+3）结构，仅支持２层目录
/* const char updata_file_name[] = "/UPDATA/JL_692X.BFU"; */
const char updata_file_name[] = "/*.UFW";
static u32 g_updata_flag = 0;
static volatile u8 ota_status = 0;
static succ_report_t succ_report;

bool vm_need_recover(void)
{
    printf(">>>[test]:g_updata_flag = 0x%x\n", g_updata_flag);
    return ((g_updata_flag & 0xffff) == UPDATA_SUCC) ? true : false;
}


u16 update_result_get(void)
{
    u16 ret = UPDATA_NON;

    if (CONFIG_UPDATE_ENABLE) {
        UPDATA_PARM *p = UPDATA_FLAG_ADDR;
        u16 crc_cal;
        crc_cal = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);	//2 : crc_val
        if (crc_cal && crc_cal == p->parm_crc) {
            ret =  p->parm_result;
        }
        g_updata_flag = ret;
        g_updata_flag |= ((u32)(p->magic)) << 16;

        memset(p, 0x00, sizeof(UPDATA_PARM));
    }

    return ret;
}

void update_result_set(u16 result)
{
    if (CONFIG_UPDATE_ENABLE) {
        UPDATA_PARM *p = UPDATA_FLAG_ADDR;

        memset(p, 0x00, sizeof(UPDATA_PARM));
        p->parm_result = result;
        p->parm_crc = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);
    }
#if (RCSP_UPDATE_EN && RCSP_MODE && JL_RCSP_EXTRA_FLASH_OPT)
    if (UPDATA_SUCC == result) {
        rcsp_eflash_update_flag_set(0);
        rcsp_eflash_flag_set(0);
        extern void set_update_ex_flash_flag(u8 update_flag);
        set_update_ex_flash_flag(0);
    }
#endif
}
void update_clear_result()
{
    g_updata_flag = 0;
}

bool update_success_boot_check(void)
{
    if (CONFIG_UPDATE_ENABLE) {
        u16 result = g_updata_flag & 0xffff;
        u16 up_tpye = g_updata_flag >> 16;
        if ((UPDATA_SUCC == result) && ((SD0_UPDATA == up_tpye) || (SD1_UPDATA == up_tpye) || (USB_UPDATA == up_tpye))) {
            return true;
        }
    }
    return false;
}

bool device_is_first_start()
{
    log_info("g_updata_flag=0x%x\n", g_updata_flag);
    if ((g_updata_flag & DEVICE_FIRST_START) || (g_updata_flag & DEVICE_UPDATE_KEY_ERR) || (g_updata_flag == UPDATA_SUCC)) {
        puts("\n=================device_is_first_start=========================\n");
        return true;
    }
    return false;
}

void led_update_start(void)
{
#ifdef UPDATE_LED_REMIND
    puts("led_update_start\n");
    /* pwm_led_mode_set(PWM_LED_ALL_OFF); */
#endif
}

void led_update_finish(void)
{
#ifdef UPDATE_LED_REMIND
    puts("led_update_finish\n");
    /* pwm_led_mode_set(PWM_LED0_LED1_FAST_FLASH); */
#endif
}

static inline void dev_update_close_ui()
{

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
    u8 count = 0;
    UI_SHOW_WINDOW(ID_WINDOW_POWER_OFF);
__retry:
    if (UI_GET_WINDOW_ID() != ID_WINDOW_POWER_OFF) {
        os_time_dly(10);//增加延时防止没有关显示
        count++;
        if (count < 3) {
            goto __retry;
        }
    }
#endif
}

extern void update_tone_event_clear();

int update_result_deal()
{
#ifdef CONFIG_FPGA_ENABLE
    return 0;
#endif

    u8 key_voice_cnt = 0;
    u16 result = 0;
    result = (g_updata_flag & 0xffff);
    log_info("<--------update_result_deal=0x%x %x--------->\n", result, g_updata_flag >> 16);
#if CONFIG_DEBUG_ENABLE
#if TCFG_APP_BT_EN
    u8 check_update_param_len(void);
    ASSERT(check_update_param_len(), "UPDATE_PARAM_LEN ERROR");
#endif
#endif
    if (result == UPDATA_NON || 0 == result) {
        return 0;
    }
#ifdef UPDATE_VOICE_REMIND
#endif
    if (result == UPDATA_SUCC) {
#if((RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_UPDATE_EN)
        u8 clear_update_flag = 0;
        syscfg_write(VM_UPDATE_FLAG, &clear_update_flag, 1);
#endif
#ifdef UPDATE_LED_REMIND
        led_update_finish();
#endif
    }

    int voice_max_cnt = 5;

    while (1) {
        wdt_clear();

        key_voice_cnt++;
#ifdef UPDATE_VOICE_REMIND
        int msg[8];
        if (result == UPDATA_SUCC) {
            puts("<<<<<<UPDATA_SUCC");
            /* app_audio_set_volume(APP_AUDIO_STATE_WTONE, get_max_sys_vol() / 2, 1); */
            play_tone_file(get_tone_files()->normal);
            for (int i = 0; i < 5; i++) {
                os_time_dly(5);
                os_taskq_accept(8, msg);
            }
            tone_player_stop();
            puts(">>>>>>>>>>>\n");
            update_tone_event_clear();
        } else {
            voice_max_cnt = 20; //区分下升级失败提示音
            log_info("!!!!!!!!!!!!!!!updata waring !!!!!!!!!!!=0x%x\n", result);
            /* app_audio_set_volume(APP_AUDIO_STATE_WTONE, get_max_sys_vol(), 1); */
            play_tone_file(get_tone_files()->normal);
            for (int i = 0; i < 5; i++) {
                os_time_dly(2);
                os_taskq_accept(8, msg);
            }
            tone_player_stop();
            update_tone_event_clear();
        }
#endif
        if (key_voice_cnt > voice_max_cnt) {
            puts("enter_sys_soft_poweroff\n");
            break;
            //注:关机要慎重,要设置开机键
            //enter_sys_soft_poweroff();
        }
    }

    return 1;
}

void clr_update_ram_info(void)
{
    UPDATA_PARM *p = UPDATA_FLAG_ADDR;
    memset(p, 0x00, sizeof(UPDATA_PARM));
}

void update_close_hw(void *filter_name)
{
    const struct update_target *p;
    list_for_each_update_target(p) {
        if (memcmp(filter_name, p->name, strlen(filter_name)) != 0) {
            printf("close Hw Name : %s\n", p->name);
            p->driver_close();
        }
    }
}

static void update_before_jump_common_handle(UPDATA_TYPE up_type)
{

#if CPU_CORE_NUM > 1            //双核需要把CPU1关掉
    printf("Before Suspend Current Cpu ID:%d Cpu In Irq?:%d\n", current_cpu_id(),  cpu_in_irq());
    if (current_cpu_id() == 1) {
        os_suspend_other_core();
    }
    ASSERT(current_cpu_id() == 0);          //确保跳转前CPU1已经停止运行
    cpu_suspend_other_core(CPU_SUSPEND_TYPE_UPDATE);
    printf("After Suspend Current Cpu ID:%d\n", current_cpu_id());
#else
    local_irq_disable();   //双核在跳转前关中断lock_set后会和maskrom 初始化中断冲突导致ilock err
#endif

    hwi_all_close();

    /*跳转的时候遇到死掉的情况很可能是硬件模块没关导致，加上保护可以判断哪个异常，保护的地址根据不同SDK而定*/
    /* u8 inv = 0; */
    /* mpu_set(1, (u32)&test_pro_addr, (u32)test_pro_addr, inv, "0r", DBG_FM); */

}

//ota.bin 放到exflash升级的方式，parm_priv存放了norflash的参数，对应实际升级方式的参数需要放在norflash参数之后
void update_param_priv_fill(UPDATA_PARM *p, void *priv, u16 priv_len)
{
    int parm_offset = 0;
    if (support_norflash_update_en) {
        parm_offset = get_nor_update_param(p->parm_priv);          //如果loader放在外挂norflash,parm_priv前面放norflash参数，后面才是升级类型本身的参数
    }
    memcpy(p->parm_priv + parm_offset, priv, priv_len);
}

void update_param_ext_fill(UPDATA_PARM *p, u8 ext_type, u8 *ext_data, u8 ext_len)
{
    struct ext_arg_t ext_arg;

    ext_arg.type = ext_type;
    ext_arg.len  = ext_len;
    ext_arg.data = ext_data;

    ASSERT(((u32)p + sizeof(UPDATA_PARM) + p->ext_arg_len + ext_len + 2 - (u32)p) <= UPDATA_PARM_SIZE, "update param overflow!\n");
    memcpy((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len, &ext_arg, 2);          //2byte:type + len
    memcpy((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len + 2, ext_arg.data, ext_arg.len);
    log_info("ext_fill :");
    log_info_hexdump((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len, ext_arg.len + 2);
    p->ext_arg_len += (2 + ext_arg.len);
    p->ext_arg_crc = CRC16((u8 *)p + sizeof(UPDATA_PARM), p->ext_arg_len);
}

//fill common content \ private content \ crc16
static void update_param_content_fill(int type, UPDATA_PARM *p, void (*priv_param_fill_hdl)(UPDATA_PARM *P))
{
    u8 ext_len = 0;
    u8 *ext_data = NULL;

    memset((u8 *)p, 0x00, sizeof(UPDATA_PARM));

    if (support_norflash_update_en) {
        p->parm_type = NORFLASH_UPDATA;                                //uboot通过该标识从外挂flash读取ota.bin
        *((u16 *)((u8 *)p + sizeof(UPDATA_PARM) + 32)) = (u16)type;    //将实际的升级类型保存到UPDATA_PARM后
    } else {
        p->parm_type = (u16)type;
    }

    p->parm_result = (u16)UPDATA_READY;
    p->magic = UPDATE_PARAM_MAGIC;
    p->ota_addr = succ_report.loader_saddr;

    //支持loader放到外挂flash里ota_addr为0
    if (0 == p->ota_addr && !support_norflash_update_en) {
        log_error("ota addr err\n");
        return;
    }

    if (priv_param_fill_hdl) {
        priv_param_fill_hdl(p);
    }

#ifndef CONFIG_FPGA_ENABLE
#ifndef CONFIG_CPU_BR28
    if (type == BT_UPDATA || type == BLE_APP_UPDATA || type == SPP_APP_UPDATA || type == BLE_TEST_UPDATA) {     //D版芯片蓝牙相关的升级需要保存LDO_TRIM_RES
        ext_data = malloc(256);
        if (ext_data != NULL) {
            ext_len = get_bt_trim_info_for_update(ext_data);
            printf("ext_len:%d\n", ext_len);
            update_param_ext_fill(p, EXT_LDO_TRIM_RES, ext_data, ext_len);
            free(ext_data);
        }
        update_param_ext_fill(p, EXT_BT_MAC_ADDR, (u8 *)bt_get_mac_addr(), 6);
    }
#endif
#endif

    u8 ext_flag = 0;
    ext_len = 1;
#if CONFIG_UPDATE_JUMP_TO_MASK
    ext_flag = 1;
#endif
    update_param_ext_fill(p, EXT_JUMP_FLAG, &ext_flag, ext_len);

    p->parm_crc = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);	//2 : crc_val
}

static void update_param_ram_set(u8 *buf, u16 len)
{
    u8 *update_ram = UPDATA_FLAG_ADDR;
    if (len > (u32)(&UPDATA_SIZE)) {
        len = (u32)(&UPDATA_SIZE);
    }
    memcpy(update_ram, (u8 *)buf, len);
}

void update_mode_api_v2(UPDATA_TYPE type, void (*priv_param_fill_hdl)(UPDATA_PARM *p), void (*priv_update_jump_handle)(int type))
{
    u16 update_param_len = UPDATA_PARM_SIZE;//sizeof(UPDATA_PARM) + UPDATE_PRIV_PARAM_LEN;

    UPDATA_PARM *p = malloc(update_param_len);

    if (p) {
        update_param_content_fill(type, p, priv_param_fill_hdl);

        if (succ_report.update_param_write_hdl) {
            succ_report.update_param_write_hdl(succ_report.priv_param, (u8 *)p, update_param_len);
        }

#ifdef UPDATE_LED_REMIND
        led_update_start();
#endif

        update_param_ram_set((u8 *)p, sizeof(UPDATA_PARM) + p->ext_arg_len);

#if TCFG_MIC_EFFECT_ENABLE
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        }
#endif

#ifdef CONFIG_SUPPORT_WIFI_DETECT
        wifi_det_close();
#endif

#if TCFG_AUDIO_ANC_ENABLE
        extern void audio_anc_hw_close();
        audio_anc_hw_close();
#endif

        if (type == BT_UPDATA) {
#if TCFG_BT_BACKGROUND_ENABLE
            if (!app_in_mode(APP_MODE_BT)) {
                //这里需要切到蓝牙模式再切时钟，如果再Music等模式需要较高主频才能跑得过来的任务，切换时钟会导致系统跑不过来导致异常
                g_bt_hdl.background.backmode = BACKGROUND_GOBACK_WITH_OTHER;
                app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
                while (!app_in_mode(APP_MODE_BT)) {
                    os_time_dly(1);
                }
            }
#endif
            clk_set_api("sys", 24000000);       //测试盒跳转升级需要强制设置时钟为24M
        }

        update_before_jump_common_handle(type);
        if (priv_update_jump_handle) {
            priv_update_jump_handle(type);
        }
        free(p);
    } else {
        ASSERT(p, "malloc update param err \n");
    }
}

int update_check_sniff_en(void)

{
    if (CONFIG_UPDATE_ENABLE) {
        if (get_ota_status()) {
            log_info("ota ing...");
            return 0;
        } else {
            return 1;
        }
    }
    return 1;
}


void set_ota_status(u8 stu)
{
    ota_status = stu;
}

u8 get_ota_status()
{
    return ota_status;
}

static u8 ota_idle_query(void)
{
    return !ota_status;
}

//防止升级过程进入powerdown
REGISTER_LP_TARGET(ota_lp_target) = {
    .name = "ota",
    .is_idle = ota_idle_query,
};

extern void tws_sync_update_api_register(const update_op_tws_api_t *op);
extern update_op_tws_api_t *get_tws_update_api(void);

extern const int support_dual_bank_update_breakpoint;
extern const int support_dual_bank_update_en;
extern int tws_ota_init(void);
extern void tws_api_auto_role_switch_disable();
extern void tws_api_auto_role_switch_enable();
extern void tws_dual_conn_state_handler();
extern void bt_sniff_enable();
extern int  norflash_set_write_protect(u32 enable, u32 wp_addr);

#define DUAL_BANK_BREAKPOIONT_STEP		(64 * 1024)
// 断点设置
// 当前函数时双备份升级时会触发，仅双备份升级且开启了断点续传功能有效
int dual_bank_curr_write_offset_callback(u32 curr_write_offset, u32 remain_len, void *priv)
{
    static u32 write_offset_recored = 0;
    u32 offset_step = curr_write_offset / DUAL_BANK_BREAKPOIONT_STEP * DUAL_BANK_BREAKPOIONT_STEP;
    if (remain_len) {
        if (write_offset_recored != offset_step) {
            write_offset_recored = offset_step;
            // 64k记录一次，放入vm中
            syscfg_write(DUAL_BANK_BP_STEP, &curr_write_offset, sizeof(curr_write_offset));
        }
    } else {
        write_offset_recored = 0;
        // 清除vm中的断点信息
        syscfg_write(DUAL_BANK_BP_STEP, &write_offset_recored, sizeof(write_offset_recored));
    }
    return 0;
}

// 断点获取
// 当前函数用于升级前或升级失败后全擦的情况，如果是使用断点续传，中途失败不擦，其他情况擦除
// 如果有开断点续传功能，该函数返回0，则表示没有升级或升级完成，忽略校验是否成功
u32 dual_bank_update_bp_info_get(void)
{
    if (support_dual_bank_update_breakpoint) {
        u32 write_offset = 0;
        // 从vm中获取断点信息
        if (sizeof(write_offset) != syscfg_read(DUAL_BANK_BP_STEP, &write_offset, sizeof(write_offset))) {
            return 0;
        }
        return write_offset;
    }
    return 0;
}

void norflash_set_write_protect_en(void)
{
    struct vm_info vm = {0};
    norflash_ioctl(NULL, IOCTL_GET_VM_INFO, (u32)&vm);
    norflash_set_write_protect(1, vm.vm_saddr);
}

void norflash_set_write_protect_remove(void)
{
    norflash_set_write_protect(0, 0);
}

static void update_init_common_handle(int type)
{
    ota_status = 1;
    if (UPDATE_DUAL_BANK_IS_SUPPORT()) {
#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

#if OTA_TWS_SAME_TIME_ENABLE
        // 关闭pack_scan
        write_scan_conn_enable(0, 0);
        // 退出sniff并关闭sniff
        update_start_exit_sniff();
        // 关闭主从切换
        tws_api_auto_role_switch_disable();
        tws_sync_update_api_register(get_tws_update_api());
        tws_ota_init();
#endif
        if (support_dual_bank_update_no_erase) {
            extern void verify_os_time_dly_set(u32 time);
            verify_os_time_dly_set(1); // 校验每包延时10ms
        }

        if (support_dual_bank_update_en && support_dual_bank_update_breakpoint) {
            dual_bank_curr_write_offset_set(dual_bank_update_bp_info_get());
        }
    }
    // 解除保护
    norflash_set_write_protect_remove();
}

static void update_exit_common_handle(int type, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;
    printf("%s, %x\n", __func__, ret_code->err_code);
    if (UPDATE_RESULT_ERR_NONE != ret_code->err_code && (UPDATE_RESULT_FLAG_BITMAP | UPDATE_RESULT_ERR_NONE) != ret_code->err_code && UPDATE_RESULT_BT_UPDATE_OVER != ret_code->err_code) {
        if (support_dual_bank_update_no_erase) {
            if (0 == dual_bank_update_bp_info_get()) {
                dual_bank_check_flash_update_area(1);
            }
        }
        // 升级失败，添加写保护
        norflash_set_write_protect_en();
    }

#if TCFG_AUTO_SHUT_DOWN_TIME
    sys_auto_shut_down_enable();
#endif

#if OTA_TWS_SAME_TIME_ENABLE
    if (UPDATE_DUAL_BANK_IS_SUPPORT()) {
        // 打开主从切换
        tws_api_auto_role_switch_enable();
        // 打开pack_scan
        tws_dual_conn_state_handler();
        // 允许进入sniff
        bt_sniff_enable();
    }
#endif

    ota_status = 0;
}

static void update_common_state_cbk(update_mode_info_t *info, u32 state, void *priv)
{
    int type = info->type;

    log_info("type:%x state:%x code:%x\n", type, state, priv);

    switch (state) {
    case UPDATE_CH_INIT:
        //如果开启了VM配置项暂存RAM功能则在每次触发升级前保存数据到vm_flash
        if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
            vm_flush2flash(1);
        }
        dlog_flush2flash();

        memset((u8 *)&succ_report, 0x00, sizeof(succ_report_t));
        update_init_common_handle(info->type);
        dev_update_close_ui();
        break;

    case UPDATE_CH_SUCESS_REPORT:
        log_info("succ report stored\n");
        memcpy((u8 *)&succ_report, (u8 *)priv, sizeof(succ_report_t));
        break;
    }

    if (info->state_cbk) {
        info->state_cbk(type, state, priv);
    }

    switch (state) {
    case UPDATE_CH_EXIT:
        update_exit_common_handle(info->type, priv);
        break;
    }
}


static int app_update_init(void)
{
    update_module_init(update_common_state_cbk);
    testbox_update_init();
    return 0;
}

__initcall(app_update_init);


void update_start_exit_sniff(void)
{
#if TCFG_APP_BT_EN
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
        g_printf("exit sniff mode...\n");
        bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    } else {
        tws_api_tx_unsniff_req();
    }
    tws_sniff_controle_check_disable();
#else
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
#endif
    sys_auto_sniff_controle(0, NULL);
#endif
}

u32 update_get_machine_num(u8 *buf, u32 len)
{
    u32 name_len = strlen(STRCHANGE(CONFIG_UPDATE_MACHINE_NUM));
    printf(">>>[test]:name_len = %d, len = %d\n", name_len, len);
    if (len < name_len) {
        return 0;
    }
    memcpy(buf, STRCHANGE(CONFIG_UPDATE_MACHINE_NUM), name_len);
    buf[name_len] = '\0';
    y_printf(">>>[test]: machine num : %s\n", buf);
    return name_len;
}

