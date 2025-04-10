#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_main.data.bss")
#pragma data_seg(".app_main.data")
#pragma const_seg(".app_main.text.const")
#pragma code_seg(".app_main.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "gpadc.h"
#include "app_tone.h"
#include "gpio_config.h"
#include "app_main.h"
#include "asm/charge.h"
#include "update.h"
#include "app_power_manage.h"
#include "audio_config.h"
#include "app_charge.h"
#include "bt_profile_cfg.h"
#include "update_loader_download.h"
#include "idle.h"
#include "bt_tws.h"
#include "key_driver.h"
#include "user_cfg.h"
#include "app_default_msg_handler.h"
#include "app_music.h"
#include "fm.h"
#include "pc.h"
#include "linein.h"
#include "linein_dev.h"
#include "soundbox.h"
#include "rtc.h"
#include "record.h"
#include "usb/usb_task.h"
#include "usb/device/usb_stack.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "alarm.h"
#include "spdif.h"
#include "power_on.h"
#include "key/adkey.h"
#include "key/iokey.h"
#include "trim.h"
#include "iis.h"
#include "dev_manager.h"
#include "app_mode_update.h"
#include "sdfile.h"
#include "mix_record_api.h"
#include "app_version.h"
#include "app_mode_sink.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "rcsp_device_status.h"

#if TCFG_LP_TOUCH_KEY_ENABLE
#include "asm/lp_touch_key_api.h"
#endif

#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            1,     0,   1024,   768 },
    {"btctrler",            4,     0,   512,   512 },
    {"btencry",             1,     0,   512,   128 },
#if (BT_FOR_APP_EN)
    {"btstack",             3,     0,   1024,  256 },
#else
    {"btstack",             3,     0,   768,   256 },
#endif
    {"jlstream",            3,     0,  768,   128 },
    {"jlstream_0",          5,     0,  768,   0 },
    {"jlstream_1",          5,     0,  768,   0 },
    {"jlstream_2",          5,     0,  768,   0 },
    {"jlstream_3",          5,     0,  768,   0 },
    {"jlstream_4",          5,     0,  768,   0 },
    {"jlstream_5",          5,     0,  768,   0 },
    {"jlstream_6",          5,     0,  768,   0 },
    {"jlstream_7",          5,     0,  768,   0 },
    {"jlstream_8",          5,     0,  768,   0 },
    {"jlstream_9",          5,     0,  768,   0 },
    {"jlstream_a",          5,     0,  768,   0 },
    {"jlstream_b",          5,     0,  768,   0 },

#if TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
    /*virtual surround pro*/
    {"media0",          5,     0,  768,   0 },
    {"media1",          5,     0,  768,   0 },
    {"media2",          5,     0,  768,   0 },
#endif

#if (TCFG_BT_SUPPORT_LHDC)
    {"a2dp_dec",            4,     0,  512 + 256,   0 },
#else
    {"a2dp_dec",            4,     0,  512,   0 },
#endif
    /*
    *file dec任务不打断jlstream任务运行,故优先级低于jlstream
    */
    {"file_dec",            4,     0,  640,   0 },
    {"file_cache",          6,     0,  512,   0 },
    {"write_file",		    5,	   0,  512,   0 },


    /* 麦克风音效任务优先级要高 */
    {"mic_effect1",         6,     1,  768,   0 },
    {"mic_effect2",         6,     1,  768,   0 },
    {"mic_effect3",         6,     1,  768,   0 },
    {"mic_effect4",         6,     1,  768,   0 },
    {"mic_effect5",         6,     1,  768,   0 },
    {"mic_effect6",         6,     1,  768,   0 },
    {"mic_effect7",         6,     1,  768,   0 },
    {"mic_effect8",         6,     1,  768,   0 },
    {"mic_effect9",         6,     1,  768,   0 },
    {"mic_effecta",         6,     1,  768,   0 },

    /*
     *为了防止dac buf太大，通话一开始一直解码，
     *导致编码输入数据需要很大的缓存，这里提高编码的优先级
     */
    {"audio_enc",           6,     0,   768,   128 },
    {"aec",					2,	   1,   768,   128 },

    {"aec_dbg",				3,	   0,   512,   128 },
    {"update",				1,	   0,   512,   0   },
    {"tws_ota",				2,	   0,   256,   0   },
    {"tws_ota_msg",			2,	   0,   256,   128 },
    {"dw_update",		 	2,	   0,   256,   128 },
    {"aud_capture",         4,     0,   512,   256 },
    {"data_export",         5,     0,   512,   256 },
    {"anc",                 3,     0,   512,   128 },
    {"pmu_task",            6,      0,  256,   128 },
    {"dac",                 2,     0,   256,   128 },
#if defined(TCFG_SPECTRUM_NODE_ENABLE) && TCFG_SPECTRUM_NODE_ENABLE
    {"spec0",               3,     1,   512,   128  },
    {"spec1",               3,     1,   512,   128  },
    {"spec2",               3,     1,   512,   128  },
    {"spec3",               3,     1,   512,   128  },
#endif
#if defined(TCFG_SPECTRUM_ADVANCE_NODE_ENABLE) && TCFG_SPECTRUM_ADVANCE_NODE_ENABLE
    {"spec_adv0",           3,     1,   512,   128  },
    {"spec_adv1",           3,     1,   512,   128  },
    {"spec_adv2",           3,     1,   512,   128  },
    {"spec_adv3",           3,     1,   512,   128  },
#endif

#if RCSP_MODE
    {"rcsp",		    	4,	   0,   768,   128 },
#if RCSP_FILE_OPT
    {"rcsp_file_bs",		    	1,	   0,   768,   128 },
#endif
#endif
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    {"kws",                 2,     0,   256,   64  },
#endif
    {"usb_stack",          	1,     0,   512,   128 },
#if (THIRD_PARTY_PROTOCOLS_SEL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN))
    {"app_proto",           2,     0,   768,   64  },
#endif
    {"ui",                  3,     0,   1024,  1024  },
#if (TCFG_DEV_MANAGER_ENABLE)
    {"dev_mg",           	3,     0,   512,   512 },
#endif

#if (TCFG_PWMLED_ENABLE)
    {"led_driver",         	6,     0,   256,   128 },
#endif

    {"audio_vad",           1,     1,   512,   128 },
#if TCFG_KEY_TONE_EN
    {"key_tone",            5,     0,   256,   32  },
#endif
#if (THIRD_PARTY_PROTOCOLS_SEL & TUYA_DEMO_EN)
    {"user_deal",           7,     0,   512,   512 },//定义线程 tuya任务调度
    {"dw_update",           2,     0,   256,   128 },
#endif
#if (TCFG_UPDATE_UART_IO_EN)
    {"uart_update",	        1,	   0,   768,   512	},
#endif
#if (CONFIG_UPDATE_MUTIL_CPU_UART)
    {"update_interactive_uart",   1,  0,   768,   512	},
#endif
    {"periph_demo",       3,     0,   512,   0 },
    {"CVP_RefTask",	        4,	   0,   256,   128	},
    {"trim_task",	        4,	   0,   256,   128	},
    {0, 0},
};


APP_VAR app_var;
static u16 mode_retry_timer = 0xff;
static bool curr_mode_exiting = false;

__attribute__((weak))
int eSystemConfirmStopStatus(void)
{
    /* 系统进入在未来时间里，无任务超时唤醒，可根据用户选择系统停止，
     * 或者系统定时唤醒(100ms)，或自己指定唤醒时间
     * return:
     *   1:Endless Sleep
     *   0:100 ms wakeup
     *   other: x ms wakeup
     */
    if (get_charge_full_flag()) {
        power_set_soft_poweroff();
        return 1;
    } else {
        return 0;
    }
}

__attribute__((used))
int *__errno()
{
    static int err;
    return &err;
}

void app_var_init(void)
{
    app_var.play_poweron_tone = 1;
}


u8 get_power_on_status(void)
{
#if TCFG_ADKEY_ENABLE
    return is_adkey_press_down();
#endif

#if TCFG_IOKEY_ENABLE
    return is_iokey_press_down();
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    return lp_touch_key_power_on_status();
#endif

    return 0;
}


void check_power_on_key(void)
{
    u32 delay_10ms_cnt = 0;

    while (1) {
        wdt_clear();
        os_time_dly(1);

        if (get_power_on_status()) {
            putchar('+');
            delay_10ms_cnt++;
            if (delay_10ms_cnt > 70) {
                app_var.poweron_reason = SYS_POWERON_BY_KEY;
                return;
            }
        } else {
            log_info("enter softpoweroff\n");
            delay_10ms_cnt = 0;
            app_var.poweroff_reason = SYS_POWEROFF_BY_KEY;
            power_set_soft_poweroff();
        }
    }
}


__attribute__((weak))
u8 get_charge_online_flag(void)
{
    return 0;
}

/*充电拔出,CPU软件复位, 不检测按键，直接开机*/
static void app_poweron_check(int update)
{
#if (CONFIG_BT_MODE == BT_NORMAL)
    if (!update && cpu_reset_by_soft()) {
        app_var.play_poweron_tone = 0;
        return;
    }

#if TCFG_CHARGE_ENABLE
    if (is_ldo5v_wakeup()) {
#if TCFG_CHARGE_OFF_POWERON_EN
        app_var.play_poweron_tone = 0;
        app_var.poweron_reason = SYS_POWERON_BY_OUT_BOX;
        return;
#else
        //拔出关机
        power_set_soft_poweroff();
#endif

    }
#endif

#if TCFG_AUTO_POWERON_ENABLE
    return;
#endif

    check_power_on_key();

#endif
}



__attribute__((weak))
void board_init()
{

}

__attribute__((weak))
void arch_trim()
{

}

static void app_version_check()
{
    puts("=================Version===============\n");
    for (char *version = __VERSION_BEGIN; version < __VERSION_END;) {
        version += 4;
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");
}

static struct app_mode *app_task_init()
{
    app_var_init();
    app_version_check();

    sdfile_init();
    syscfg_tools_init();
#if (defined(TCFG_DEBUG_DLOG_ENABLE) && TCFG_DEBUG_DLOG_ENABLE)
    dlog_init();
#endif

    do_early_initcall();
    board_init();
    do_platform_initcall();


    cfg_file_parse(0);
    key_driver_init();

    key_wakeup_init();

    do_initcall();
    do_module_initcall();
    do_late_initcall();

    dev_manager_init();

#if TCFG_UI_ENABLE
    UI_INIT((void *)&ui_cfg_data);
#endif /* #if TCFG_UI_ENABLE */

#if TCFG_APP_RTC_EN
    alarm_init();
#endif

    int update = 0;
    if (CONFIG_UPDATE_ENABLE) {
        update = update_result_deal();
    }

#if TCFG_MC_BIAS_AUTO_ADJUST
    mic_capless_trim_init(update);
#endif

    app_var.start_time = jiffies_msec();

    int msg[4] = { MSG_FROM_APP, APP_MSG_GOTO_MODE, 0, 0 };

    if (get_charge_online_flag()) {
#if(TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
        msg[2] = APP_MODE_IDLE;
        msg[3] = IDLE_MODE_CHARGE;
    } else {
        msg[2] = APP_MODE_POWERON;
        check_power_on_voltage();
        app_poweron_check(update);
        app_send_message(APP_MSG_POWER_ON, 0);
    }

#if TCFG_CHARGE_ENABLE
    set_charge_event_flag(1);
#endif

#if TCFG_DYNAMIC_SWITCHING_IOVDDM_ENABLE
    miovdd_adaptive_adjustment();
#endif

    arch_trim();

    struct app_mode *mode;
    mode = app_mode_switch_handler(msg);
    ASSERT(mode != NULL);
    return mode;
}

static int app_core_get_message(int *msg, int max_num)
{
    while (1) {
        int res = os_taskq_pend(NULL, msg, max_num);
        if (res != OS_TASKQ) {
            continue;
        }
        if (msg[0] & Q_MSG) {
            return 1;
        }
    }
    return 0;
}

static int g_mode_switch_arg;
static int g_mode_switch_msg[2];

static void retry_goto_mode_in_irq(void *_arg)
{
    mode_retry_timer = 0xff;
    if (g_mode_switch_msg[0] == APP_MSG_GOTO_MODE) {
        app_send_message2(g_mode_switch_msg[0], g_mode_switch_msg[1], g_mode_switch_arg);
    } else {
        app_send_message(g_mode_switch_msg[0], g_mode_switch_arg);
    }
}

struct app_mode *app_mode_switch_handler(int *msg)
{
    int arg;
    struct app_mode *next_mode, *traverse_first;

    if (msg[0] != MSG_FROM_APP) {
        return NULL;
    }

    switch (msg[1]) {
    case APP_MSG_GOTO_MODE:
        arg = msg[3];
        /*判断当前是否已经处于GOTO MODE希望切换的任务，如果是则不反复切换*/
        if (app_get_current_mode() && app_get_current_mode()->name == msg[2]) {
            if (curr_mode_exiting == false) {       //这里为了处理正在退出当前模式，但是流程上又post了消息想重新进入的情况
                return NULL;
            }
        }
        next_mode = app_get_mode_by_name(msg[2]);
        break;
    case APP_MSG_GOTO_NEXT_MODE:
        arg = msg[2];
        next_mode = app_get_next_mode();
        if (app_get_current_mode() == next_mode) {
            return NULL;
        }
        break;
    default:
        return NULL;
    }

    g_mode_switch_arg = arg;
    g_mode_switch_msg[0] = msg[1];
    g_mode_switch_msg[1] = msg[2];

#if TCFG_APP_BT_EN && TCFG_BT_BACKGROUND_ENABLE
    if (!bt_check_already_initializes()) {          //如果是后台使能蓝牙还没初始化，需要先记录切换的模式，等到蓝牙初始化完成之后再切回去
        if (next_mode->name != APP_MODE_BT && next_mode->name != APP_MODE_POWERON && next_mode->name != APP_MODE_IDLE) {
            bt_background_set_switch_mode(next_mode->name);
            if (app_get_current_mode()->name == APP_MODE_POWERON) {
                next_mode =  app_get_mode_by_name(APP_MODE_BT);
            } else {
                return NULL;
            }
        }
    }
#endif

    /*
     * 循环检查下一个模式是否可以进入
     */
    traverse_first = next_mode;
    do {
        if (app_try_enter_mode(next_mode, arg)) {
            break;
        }
        next_mode = app_next_mode(next_mode);
        if (traverse_first == next_mode) {      //已经遍历了一轮没有可以进入的模式，则跳转到IDLE
            next_mode = app_get_mode_by_name(APP_MODE_IDLE);
            g_mode_switch_arg = IDLE_MODE_WAIT_DEVONLINE;
            break;
        }
    } while (next_mode);

    //处理开了多个模式，但是除了当前模式其他模式都无法进入的情况，不要重复切换当前任务
    if (app_get_current_mode()->name == next_mode->name && curr_mode_exiting == false) {
        return NULL;
    }

#if TCFG_MIX_RECORD_ENABLE
    //切换模式前关闭混合录音
    if (get_mix_recorder_status()) {
        mix_recorder_stop();
    }
#endif // TCFG_MIX_RECORD_ENABLE

    /*
     * 等待当前模式退出
     */
    if (!app_try_exit_curr_mode()) {
        putchar('T');
        if (mode_retry_timer != 0xff) {
            sys_hi_timeout_del(mode_retry_timer);
        }
        mode_retry_timer = sys_hi_timeout_add(NULL, retry_goto_mode_in_irq, 100);
        curr_mode_exiting = true;
        return NULL;
    }

    curr_mode_exiting = false;

    return next_mode;
}

int app_get_message(int *msg, int max_num, const struct key_remap_table *key_table)
{
    const struct app_msg_handler *handler;
    uint32_t rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));

    app_core_get_message(msg, max_num);

    if (msg[1] == APP_MSG_MUSIC_PLAY_SUCCESS) {
        printf("app_get_message  APP_MSG_MUSIC_PLAY_SUCCESS:0x%x\n", rets_addr);
    }

    if (msg[0] == MSG_FROM_KEY && key_table) {
        /*
         * 按键消息映射成当前模式的消息
         */
        struct app_mode *mode = app_get_current_mode();
        if (mode) {
            int key_msg = app_key_event_remap(key_table, msg + 1);
            if (key_msg == APP_MSG_NULL) {
                return 1;
            }
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#if (LEA_BIG_CUSTOM_DATA_EN && LEA_BIG_VOL_SYNC_EN && (!TCFG_KBOX_1T3_MODE_EN))
            if ((get_broadcast_role() == BROADCAST_ROLE_RECEIVER) && get_receiver_connected_status()) {
                if ((key_msg == APP_MSG_VOL_UP) || (key_msg == APP_MSG_VOL_DOWN)) {
                    return 1;
                }
            }
#endif
#endif
            if (mode->name == APP_MODE_BT) { //蓝牙模式 判断是否需要tws同步按键消息
#if (TCFG_USER_TWS_ENABLE && TCFG_APP_BT_EN)
                if (key_msg != APP_MSG_CHANGE_MODE) {
                    bt_tws_key_msg_sync(key_msg);
                } else {
                    msg[0] = MSG_FROM_APP;
                    msg[1] = key_msg;
                }
#else
                msg[0] = MSG_FROM_APP;
                msg[1] = key_msg;
#endif
            } else {
                if (msg[0] == MSG_FROM_RTC) {
                    msg[0] = MSG_FROM_RTC;
                    msg[1] = key_msg;
                } else {
                    msg[0] = MSG_FROM_APP;
                    msg[1] = key_msg;
                }
            }
        }
    }

    //消息截获,返回1表示中断消息分发
    int abandon = 0;
    for_each_app_msg_prob_handler(handler) {
        if (handler->from == msg[0]) {
            abandon = handler->handler(msg + 1);
            if (abandon) {
                break;
            }
        }
    }
    if (abandon) {
        return 0;
    }
    return 1;
}

static void app_task_loop(void *p)
{
    struct app_mode *mode;

    mode = app_task_init();

    while (1) {
        app_set_current_mode(mode);
        /*需要每个模式单独音量可以打开该接口，注意:应用流程不完善，需要完善流程每个模式单独记录音量，不然会导致音量混乱*/
        /* audio_digital_vol_default_init(); */

//需要根据具体开发需求选择是否开启切mode也需要缓存flash
//1.需要注意的缓存到flash需要较长一段时间（1~3s），特别是从蓝牙后台模式切换到蓝牙前台模式，会出现用户明显感知卡顿现象，因此需要具体需求选择性开启。
#if 0
        //如果开启了VM配置项暂存RAM功能则在每次切模式的时候保存数据到vm_flash,避免丢失数据
        if (get_vm_ram_storage_enable()) {
            vm_flush2flash();
        }
#endif //#if 0
//


#if (RCSP_MODE && RCSP_DEVICE_STATUS_ENABLE)
        function_change_inform(mode->name, 0);
#endif


        switch (mode->name) {
        case APP_MODE_IDLE:
            mode = app_enter_idle_mode(g_mode_switch_arg);
            break;
        case APP_MODE_POWERON:
            mode = app_enter_poweron_mode(g_mode_switch_arg);
            break;
#if TCFG_APP_BT_EN
        case APP_MODE_BT:
            mode = app_enter_bt_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_MUSIC_EN
        case APP_MODE_MUSIC:
            mode = app_enter_music_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_FM_EN
        case APP_MODE_FM:
            mode = app_enter_fm_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_RECORD_EN
        case APP_MODE_RECORD:
            mode = app_enter_record_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_LINEIN_EN
        case APP_MODE_LINEIN:
            mode = app_enter_linein_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_RTC_EN
        case APP_MODE_RTC:
            mode = app_enter_rtc_mode(g_mode_switch_arg);
            break;
#endif
#if TCFG_APP_PC_EN
        case APP_MODE_PC:
            mode = app_enter_pc_mode(g_mode_switch_arg);
            break;
#endif

#if TCFG_APP_SPDIF_EN
        case APP_MODE_SPDIF:
            mode = app_enter_spdif_mode(g_mode_switch_arg);
            break;
#endif
        case APP_MODE_UPDATE:
            mode = app_enter_update_mode(g_mode_switch_arg);
            break;
#if TCFG_APP_IIS_EN
        case APP_MODE_IIS:
            mode = app_enter_iis_mode(g_mode_switch_arg);
            break;
#endif
        case APP_MODE_SINK:
#if TCFG_LOCAL_TWS_ENABLE
            mode = app_enter_sink_mode(g_mode_switch_arg);
#endif
            break;
        default:
            break;
        }
    }

}

void app_main()
{
    task_create(app_task_loop, NULL, "app_core");

    os_start(); //no return
    while (1) {
        asm("idle");
    }
}

