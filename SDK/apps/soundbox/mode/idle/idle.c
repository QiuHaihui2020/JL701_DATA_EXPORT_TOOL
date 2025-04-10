#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".idle.data.bss")
#pragma data_seg(".idle.data")
#pragma const_seg(".idle.text.const")
#pragma code_seg(".idle.text")
#endif
#include "idle.h"
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_tone.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "user_cfg.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "avctp_user.h"
#include "mic_effect.h"
#include "linein_dev.h"
#include "hdmi_cec_api.h"
#include "asm/wdt.h"
#include "audio_config.h"
#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif
#include "dev_manager.h"

#if TCFG_ANC_BOX_ENABLE
#include "app_ancbox.h"
#endif

#define LOG_TAG             "[APP_IDLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#include "bt_tws.h"

static int app_idle_init(int param);
void app_idle_exit();

// idle 是否关闭不用的模块，减少功耗
#define LOW_POWER_IN_IDLE    0

#define POWER_ON_CNT         10
static u8 goto_poweron_cnt = 0;
static u8 goto_poweron_flag = 0;

#define POWER_OFF_CNT       10
static u8 goto_poweroff_cnt = 0;
unsigned char goto_poweroff_first_flag = 0;
static u8 goto_poweroff_flag = 0;
static u8 power_off_tone_play_flag = 0;
static u16 wait_device_online_timer = 0;
static u8 idle_enter_param = 0;

#if LOW_POWER_IN_IDLE
/*----------------------------------------------------------------------------*/
/**@brief  下面处理是为了关闭不需要用的模块，减少系统功耗
  @param
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
#include "usb/otg.h"
#include "usb/host/usb_host.h"
#include "usb/device/usb_stack.h"
#include "usb_task.h"
#include "asm/power/p33.h"
#include "asm/sdmmc.h"
#include "dev_manager/dev_manager.h"

static u8 suspend_flag = 0;
static u8 is_idle_flag = 0;

u32 regs_buf[11] = {0};
void resume_some_peripheral()
{
    u32 *regs_ptr = regs_buf;

    if (!suspend_flag) {
        return;
    }

    /*
       here need to add
    */

    suspend_flag = 0;
}

void suspend_some_peripheral()
{
    u32 *regs_ptr = regs_buf;

    if (suspend_flag) {
        return;
    }

    /*
       here need to add
    */

    suspend_flag = 1;
}

#if 0
static u8 is_idle_query(void)
{
    return is_idle_flag;
}
REGISTER_LP_TARGET(idle_lp_target) = {
    .name = "not_idle",
    .is_idle = is_idle_query,
};
#endif

#if (TCFG_PC_ENABLE || TCFG_USB_HOST_ENABLE)
static u8 usb_wait_schedule_done;
static void idle_close_usb_module(void *arg)
{
    /* extern int usb_mount_offline(usb_dev usb_id); */
#if TCFG_OTG_MODE
    usb_detect_timer_del();
#endif
    /* os_time_dly((TCFG_OTG_DET_INTERVAL + 9) / 10); */
    if (usb_otg_online(0) == HOST_MODE) {
#if TCFG_UDISK_ENABLE
        dev_manager_del("udisk0");
        usb_host_unmount(0);
        usb_h_sie_close(0);

        /*
           经过测试发现，有相当一部分在DP/DM设成高阻状态下U盘的电流仍维持在
           20 ~ 30mA，需要把DP设成上拉，DM设成下拉，这些U盘的电流才能降到2mA
           以下。即有部分U盘需要主机维持在空闲时J状态才能进入suspend。
         */
        gpio_set_mode(PORTUSB, PORT_PIN_0, PORT_INPUT_PULLUP_10K);//usb dp
        gpio_set_mode(PORTUSB, PORT_PIN_1, PORT_INPUT_PULLDOWN_10K);//usb dm


        /* usb_mount_offline(0); */
#endif
    } else if (usb_otg_online(0) == SLAVE_MODE) {
#if TCFG_PC_ENABLE
        usb_pause(0);

        gpio_set_mode(PORTUSB, PORT_PIN_0 | PORT_PIN_1, PORT_HIGHZ);//usb dp |dm
#endif
    }
    usb_wait_schedule_done = 1;
}

static void idle_resume_usb_module(void *arg)
{
#if TCFG_OTG_MODE
    usb_detect_timer_add();
#endif
    usb_wait_schedule_done = 1;
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    idle 重新打开需要的模块
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void idle_app_open_module()
{
#if LOW_POWER_IN_IDLE
    is_idle_flag = 0;
#if (TCFG_LOWPOWER_LOWPOWER_SEL == 0)
    resume_some_peripheral();
#endif

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    sdx_dev_detect_timer_add();
#endif

#if (TCFG_PC_ENABLE || TCFG_USB_HOST_ENABLE)
    int umsg[4];
    umsg[0] = (int)idle_resume_usb_module;
    umsg[1] = 1;
    umsg[2] = 0;
    usb_wait_schedule_done = 0;
    if (OS_NO_ERR == os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, umsg)) {
        while (!usb_wait_schedule_done) {
            os_time_dly(1);
        }
    }
#endif

#if TCFG_APP_LINEIN_EN
    linein_detect_timer_add();
#endif
#endif //LOW_POWER_IN_IDLE
}

//*----------------------------------------------------------------------------*/
/**@brief    idle 关闭不需要的模块
  @param    无
  @return
  @note
 */
/*----------------------------------------------------------------------------*/
static void idle_app_close_module()
{
#if LOW_POWER_IN_IDLE
    int ret = false;
    is_idle_flag = 1;

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
    sdx_dev_detect_timer_del();
#endif

    /* #if (((TCFG_PC_ENABLE) && (!TCFG_USB_PORT_CHARGE)) || ((TCFG_USB_HOST_ENABLE) && (!TCFG_PC_ENABLE))) */
#if (TCFG_PC_ENABLE || TCFG_USB_HOST_ENABLE)
    int umsg[4];
    umsg[0] = (int)idle_close_usb_module;
    umsg[1] = 1;
    umsg[2] = 0;
    usb_wait_schedule_done = 0;
    if (OS_NO_ERR == os_taskq_post_type(USB_TASK_NAME, Q_CALLBACK, 3, umsg)) {
        while (!usb_wait_schedule_done) {
            os_time_dly(1);
        }
    }
#endif

#if TCFG_APP_LINEIN_EN
    linein_detect_timer_del();
#endif
#if TCFG_APP_SPDIF_EN
    hdmi_detect_timer_del();
#endif

#if (TCFG_LOWPOWER_LOWPOWER_SEL == 0)
    suspend_some_peripheral();
#endif
#endif
}

#endif /*LOW_POWER_IN_IDLE*/


//power off前的ui处理
#if  (TCFG_UI_ENABLE)
void power_off_wait_ui(void)
{
#if(CONFIG_UI_STYLE== STYLE_JL_SOUNDBAR)
    ui_soundbar_style_power_off_wait();
#endif
}
#else
void power_off_wait_ui()
{
}
#endif


void idle_key_poweron_deal(int msg)
{
    if (idle_enter_param == IDLE_MODE_WAIT_DEVONLINE) { //如果是等待设备上线过程不响应按键消息
        return;
    }
    switch (msg) {
    case APP_MSG_KEY_POWER_ON:
        goto_poweron_cnt = 0;
        goto_poweron_flag = 1;
        break;
    case APP_MSG_KEY_POWER_ON_HOLD:
        printf("poweron flag:%d cnt:%d\n", goto_poweron_flag, goto_poweron_cnt);
        if (goto_poweron_flag) {
            goto_poweron_cnt++;
            if (goto_poweron_cnt >= POWER_ON_CNT) {
                goto_poweron_cnt = 0;
                goto_poweron_flag = 0;
                app_var.goto_poweroff_flag = 0;
#if LOW_POWER_IN_IDLE
                idle_app_open_module();
#endif
                app_var.play_poweron_tone = 0;
                app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
            }
        }
        break;
    }
}

#if TCFG_USER_TWS_ENABLE
enum {
    API_POWER_OFF,
    API_TWS_POWER_OFF,
};

static void call_app_api(int api, int err)
{
    switch (api) {
    case API_POWER_OFF:
        sys_enter_soft_poweroff(POWEROFF_NORMAL);
        break;
    case API_TWS_POWER_OFF:
        sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
        break;
    default:
        break;
    }
}

TWS_SYNC_CALL_REGISTER(app_api_sync_call_entry) = {
    .uuid       = 0x891E7CD3,
    .func       = call_app_api,
    .task_name  = "app_core",
};
#endif
/*----------------------------------------------------------------------------*/
/**@brief   poweroff 长按等待 关闭蓝牙
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void power_off_deal(int msg)
{
    switch (msg) {
    case APP_MSG_KEY_POWER_OFF:
    case APP_MSG_KEY_POWER_OFF_HOLD:
        if (goto_poweroff_first_flag == 0) {
            goto_poweroff_first_flag = 1;
            goto_poweroff_cnt = 0;
            goto_poweroff_flag = 0;
#if TCFG_APP_BT_EN
            if ((BT_STATUS_CONNECTING == bt_get_connect_status()) ||
                (BT_STATUS_TAKEING_PHONE == bt_get_connect_status()) ||
                (BT_STATUS_PLAYING_MUSIC == bt_get_connect_status())) {
                if ((bt_get_call_status() == BT_CALL_INCOMING) ||
                    (bt_get_call_status() == BT_CALL_OUTGOING)) {
                    log_info("key call reject\n");
                    /* bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL); */
                    goto_poweroff_first_flag = 0;
                    goto_poweroff_flag = 0;
                    break;
                } else if (bt_get_call_status() == BT_CALL_ACTIVE) {
                    log_info("key call hangup\n");
                    /* bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL); */
                    goto_poweroff_first_flag = 0;
                    goto_poweroff_flag = 0;
                    break;
                }
            }

            bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
#endif
            goto_poweroff_flag = 1;
            break;
        }

        log_info("poweroff flag:%d cnt:%d\n", goto_poweroff_flag, goto_poweroff_cnt);

        if (goto_poweroff_flag) {
            goto_poweroff_cnt++;

#if CONFIG_TWS_POWEROFF_SAME_TIME
            if (goto_poweroff_cnt == POWER_OFF_CNT) {
#if TCFG_MIC_EFFECT_ENABLE
                if (mic_effect_player_runing()) {
                    mic_effect_player_close();
                }
#endif

#if TCFG_USER_TWS_ENABLE
                if (get_tws_sibling_connect_state()) {
                    tws_api_sync_call_by_uuid(0x891E7CD3, API_TWS_POWER_OFF, 100);
                    //sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
                } else
#endif
                {
                    power_off_tone_play_flag = 1;
                    sys_enter_soft_poweroff(POWEROFF_NORMAL);
                }
            }
#else
            if (goto_poweroff_cnt >= POWER_OFF_CNT) {
                goto_poweroff_cnt = 0;
#if TCFG_MIC_EFFECT_ENABLE
                if (mic_effect_player_runing()) {
                    mic_effect_player_close();
                }
#endif
#if TCFG_APP_BT_EN
                sys_enter_soft_poweroff(POWEROFF_NORMAL);
#else
                app_var.goto_poweroff_flag = 1;
                app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
#endif
            }
#endif /*CONFIG_TWS_POWEROFF_SAME_TIME*/
        }
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   poweroff 立刻关机
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
void power_off_instantly()
{
    if (goto_poweroff_first_flag == 0) {
        goto_poweroff_first_flag = 1;
    } else {
        puts("power_off_instantly had call\n");
        return;
    }
#if TCFG_APP_BT_EN
    if ((BT_STATUS_CONNECTING == bt_get_connect_status()) ||
        (BT_STATUS_TAKEING_PHONE == bt_get_connect_status()) ||
        (BT_STATUS_PLAYING_MUSIC == bt_get_connect_status())) {
        if ((bt_get_call_status() == BT_CALL_INCOMING) ||
            (bt_get_call_status() == BT_CALL_OUTGOING)) {
            log_info("key call reject\n");
            /* bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL); */
            return;
        } else if (bt_get_call_status() == BT_CALL_ACTIVE) {
            log_info("key call hangup\n");
            /* bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL); */
            return;
        }
    }
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
#endif

#if TCFG_USER_TWS_ENABLE && CONFIG_TWS_POWEROFF_SAME_TIME
    if (get_tws_sibling_connect_state()) {
        sys_enter_soft_poweroff(POWEROFF_NORMAL_TWS);
    } else {
        sys_enter_soft_poweroff(POWEROFF_NORMAL);
    }
#else
#if TCFG_APP_BT_EN
    sys_enter_soft_poweroff(POWEROFF_NORMAL);
#else
    app_var.goto_poweroff_flag = 1;
    app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_IDLE, IDLE_MODE_PLAY_POWEROFF);
#endif
#endif
}



static void app_idle_enter_softoff(void)
{
    //power off前的ui处理
    power_off_wait_ui();
    //ui_update_status(STATUS_POWEROFF);

#if TCFG_CHARGE_ENABLE
    if (get_lvcmp_det() && (0 == get_charge_full_flag())) {
        log_info("charge inset, system reset!\n");
        cpu_reset();
    }
#endif

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_close();
#endif

    //关机前先关dac
    dac_power_off();

    power_set_soft_poweroff();
}

struct app_mode *app_enter_idle_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_idle_init(arg);

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), idle_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            idle_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_idle_exit();

    return next_mode;
}

void app_power_off(void *priv)
{
    app_idle_enter_softoff();
}

static int app_power_off_tone_cb(void *priv, enum stream_event event)
{
    if (event == STREAM_EVENT_STOP) {
        app_idle_enter_softoff();
    }
    return 0;
}

static void device_online_timeout(void *priv)
{
    int ret = play_tone_file_callback(get_tone_files()->power_off, NULL,
                                      app_power_off_tone_cb);
    wait_device_online_timer = 0;
    printf("power_off tone play ret:%d", ret);
    if (ret) {
        if (app_var.goto_poweroff_flag) {
            log_info("power_off tone play err,enter soft poweroff");
            app_idle_enter_softoff();
        }
    }
}

static int app_idle_init(int param)
{
    log_info("idle_mode_enter: %d\n", param);

    idle_enter_param = param;

#if LOW_POWER_IN_IDLE
    idle_app_close_module();
#endif


    switch (param) {
    case IDLE_MODE_PLAY_POWEROFF:
        if (app_var.goto_poweroff_flag) {
            syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 2);
            //如果开启了VM配置项暂存RAM功能则在关机前保存数据到vm_flash
            if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
                vm_flush2flash(1);
            }
            os_taskq_flush();
            int ret = play_tone_file_callback(get_tone_files()->power_off, NULL,
                                              app_power_off_tone_cb);
            printf("power_off tone play ret:%d", ret);
            if (ret) {
                if (app_var.goto_poweroff_flag) {
                    log_info("power_off tone play err,enter soft poweroff");
                    app_idle_enter_softoff();
                }
            }
        }
        break;
    case IDLE_MODE_WAIT_POWEROFF:
        os_taskq_flush();
        syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 2);
        break;
    case IDLE_MODE_CHARGE:
        break;
    case IDLE_MODE_WAIT_DEVONLINE:      //等待1s设备没有上线就进入关机
        if (wait_device_online_timer == 0) {
            wait_device_online_timer = sys_timeout_add(NULL, device_online_timeout, 1000);
        }
        break;
    }

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_IDLE);
    return 0;
}

void app_idle_exit()
{
    if (wait_device_online_timer) {
        sys_timeout_del(wait_device_online_timer);
        wait_device_online_timer = 0;
    }
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_IDLE);
}

static int idle_mode_try_enter(int arg)
{
    return 0;
}

static int idle_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops idle_mode_ops = {
    .try_enter          = idle_mode_try_enter,
    .try_exit           = idle_mode_try_exit,
};

REGISTER_APP_MODE(idle_mode) = {
    .name   = APP_MODE_IDLE,
    .index  = 0xff,
    .ops    = &idle_mode_ops,
};
