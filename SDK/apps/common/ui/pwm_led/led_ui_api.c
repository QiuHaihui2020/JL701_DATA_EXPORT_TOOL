#include "app_msg.h"
#include "app_main.h"
#include "bt_tws.h"
#include "battery_manager.h"
#include "asm/charge.h"
#include "led_ui_api.h"

#define LOG_TAG             "[PWM_LED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if (TCFG_PWMLED_ENABLE)

extern const struct led_state_map g_led_state_table[];

struct pwm_led_handler {
    u8 id;
    u8 curr_state_name;     //这个变量用的地方很少，如果要优化，可以把这个变量变成链表运算的，算力换空间
    u8 current_state_loop;
    struct list_head sta_list;
    struct led_state_obj *loop_sta;
};


static DEFINE_SPINLOCK(g_led_lock);
static struct pwm_led_handler *__this;


#ifdef PWM_LED_DEBUG_ENABLE
#include "led_ui_manager_test_demo.c"
#endif


struct led_state_obj *led_ui_get_first_state()
{
    struct led_state_obj *obj;
    obj = list_first_entry(&__this->sta_list, struct led_state_obj, entry);
    if (&obj->entry != &__this->sta_list) {
        return obj;
    }
    return __this->loop_sta;
}

struct led_state_obj *led_ui_get_state(u8 uuid)
{
    struct led_state_obj *obj, *n;
    if (__this->loop_sta && __this->loop_sta->uuid == uuid) {
        return __this->loop_sta;
    }
    list_for_each_entry_safe(obj, n, &__this->sta_list, entry) {
        if (obj->uuid == uuid) {
            return obj;
        }

    }
    return NULL;
}

//软件控制层与硬件层的交互接口
void pwm_led_status_display(const struct led_state_item *action)
{
    led_pdata_t hw_data = {0};

    switch (action->led_name) {
    case LED_RED:
        /* board_cfg_hw_data.led0.brightness = action->brightiness; */
        hw_data.ctl_option = CTL_LED0_ONLY;
        break;
    case LED_BLUE:
        /* board_cfg_hw_data.led1.brightness = action->brightiness; */
        hw_data.ctl_option = CTL_LED1_ONLY;
        break;
    }

    hw_data.ctl_cycle_num = 0;

    //初始化为呼吸灯效
    if (action->breath_time) {
        hw_data.ctl_mode = CYCLE_BREATHE_BRIGHT;
        hw_data.breathe_bright.bright_time = action->breath_time;
        hw_data.ctl_cycle = action->breath_time + 1;
        hw_data.breathe_bright.brightest_keep_time = action->breath_time / 4;
    } else {
        if (action->brightiness) {
            hw_data.ctl_mode = ALWAYS_BRIGHT;
        } else {
            hw_data.ctl_mode = ALWAYS_EXTINGUISH;
        }
    }
    //操作硬件层
    led_effect_output(&hw_data, 0);
}

static void led_do_next_action(void *uuid)
{
    struct led_state_obj *obj;
    spin_lock(&g_led_lock);
    obj = led_ui_get_first_state();
    if (obj && obj->uuid == (u8)uuid) {
        obj->timer = 0;
        led_ui_state_machine_run();
    }
    spin_unlock(&g_led_lock);
}

static void led_ui_state_do_action(struct led_state_obj *obj)
{
    int msg[2];
    int uuid = obj->uuid;
    const struct led_state_item *status = &obj->table[obj->action_index];

    pwm_led_status_display(status);
    //******上面操作了一次硬件*************************************************************
    obj->action_index++;

    //Next Action
    switch (status->action) {
    case LED_ACTION_WAIT:
        PWMLED_LOG_DEBUG(">>>>ACTION_WAIT:0");
        if (status->time_msec) {
            obj->timer = sys_hi_timeout_add((void *)uuid, led_do_next_action,
                                            status->time_msec * LED_UI_FOLLOW_HW_TIME_MS);
        } else {
            led_ui_state_do_action(obj);
        }
        break;
    case LED_ACTION_CONTINUE:
        PWMLED_LOG_DEBUG(">>>>ACTION_CONTINUE:1");
        led_ui_state_do_action(obj);
        break;
    case LED_ACTION_END:
        PWMLED_LOG_DEBUG(">>>>ACTION_END:2");
        obj->action_index = 0xff;
        msg[0] = LED_MSG_STATE_END;
        msg[1] = obj->uuid;
        app_send_message_from(MSG_FROM_PWM_LED, 8, msg);
        break;
    case LED_ACTION_LOOP:
        PWMLED_LOG_DEBUG(">>>>>>>>ACTION_LOOP:3");
        obj->action_index = 0;
        if (status->time_msec) {
            obj->timer = sys_hi_timeout_add((void *)uuid, led_do_next_action,
                                            status->time_msec * LED_UI_FOLLOW_HW_TIME_MS);
        } else {
            led_ui_state_do_action(obj);
        }
        break;
    default:
        break;
    }
}


void led_ui_state_machine_run()
{
    struct led_state_obj *obj;

    spin_lock(&g_led_lock);

    while (1) {
        obj = led_ui_get_first_state();
        if (!obj) {
            __this->curr_state_name = 0xff;
            goto __exit;
        }
        if (obj->action_index != 0xff) {
            break;
        }
        if (__this->loop_sta == obj) {
            __this->loop_sta = NULL;
        } else {
            __list_del_entry(&obj->entry);
        }
        free(obj);
    }
    __this->curr_state_name = obj->name;

    /*log_info("machine_run_name: %d\n", obj->name);*/

#if TCFG_USER_TWS_ENABLE
    //是需要tws同步的灯效
    if (obj->disp_mode & DISP_TWS_SYNC) {
        if (!(obj->disp_mode & DISP_TWS_SYNC_TX)) {
            if (obj->time) {
                obj->disp_mode |= DISP_TWS_SYNC_TX;
                int time = 1;
                if (obj->time->ctl_mode == ALWAYS_BRIGHT ||
                    obj->time->ctl_mode == ALWAYS_EXTINGUISH) {
                    time = 0;
                }
                led_tws_state_sync_start(obj->name, time, 300);
                goto __exit;
            }
            if (obj->action_index == 0) {
                obj->disp_mode |= DISP_TWS_SYNC_TX;
                led_tws_state_sync_start(obj->name, 0, 300);
                goto __exit;
            }
        }
        obj->disp_mode &= ~DISP_TWS_SYNC_TX;
    } else if (!(obj->disp_mode & DISP_TWS_SYNC_RX)) {
        led_tws_state_sync_stop();
    }
#endif

    if (obj->time) {
        __this->current_state_loop = 0;
        PWMLED_LOG_SUCC("USE Hard PWM_LED Module to run effect");

        const led_pdata_t *time = obj->time;
        //直接使用硬件驱动来推灯
        led_effect_output((void *)time, obj->uuid);

    } else if (obj->table) {

        if (obj->table[obj->table_size - 1].action == LED_ACTION_LOOP) {
            __this->current_state_loop = 1;
        } else {

            __this->current_state_loop = 0;
        }

        //先使用软件定时器操作一下，再经过中间层策略，然后再控制硬件层驱动
        PWMLED_LOG_SUCC("USE software timer && PWM_LED Module to run effect");

        led_ui_state_do_action(obj);
    }

__exit:
    spin_unlock(&g_led_lock);
}

void led_ui_clr_loop_state()
{
    struct led_state_obj *p = __this->loop_sta;
    if (!p) {
        return;
    }
    if (p->timer) {
        sys_hi_timer_del(p->timer);
        p->timer = 0;
    }
    free(p);
    __this->loop_sta = NULL;
}

void led_ui_clr_state_list()
{
    struct led_state_obj *p, *n;
    list_for_each_entry_safe(p, n, &__this->sta_list, entry) {
        if (p->disp_mode & DISP_NON_INTR) {
            continue;
        }
        if (p->timer) {
            sys_hi_timer_del(p->timer);
            p->timer = 0;
        }
        __list_del_entry(&p->entry);
        free(p);
    }
}

void led_ui_add_state(struct led_state_obj *s)
{
    int machine_run = 0;

    spin_lock(&g_led_lock);

    do {
        s->uuid = ++__this->id;
    } while (s->uuid == 0);

    if ((s->time && (s->time->ctl_cycle_num == 0)) ||
        s->table[s->table_size - 1].action == LED_ACTION_LOOP) {
        // s为周期性循环灯效
        led_ui_clr_loop_state();
        if (s->disp_mode & DISP_CLEAR_OTHERS) {
            led_ui_clr_state_list();
        }
        __this->loop_sta = s;
        if (list_empty(&__this->sta_list)) {
            machine_run = 1;
        }
    } else {
        if (__this->loop_sta) {
            if (!(__this->loop_sta->disp_mode & DISP_RECOVERABLE) ||
                s->disp_mode & DISP_CLEAR_OTHERS) {
                // 如果循环灯效没有设置RECOVERABLE标记，或者s指定清除所有灯效，
                // 则清除循环灯效
                led_ui_clr_loop_state();
            }
        }
        if (s->disp_mode & DISP_CLEAR_OTHERS) {
            // 清除掉没有标记NON_INTR的所有灯效
            led_ui_clr_state_list();
        }
        list_add_tail(&s->entry, &__this->sta_list);
        if (s->entry.prev == &__this->sta_list) {
            machine_run = 1;
        }
    }

    spin_unlock(&g_led_lock);

    if (machine_run) {
        led_ui_state_machine_run();
    }
}

//总入口 只有一个对外接口调用
void led_ui_set_state(enum led_state_name name, enum led_disp_mode disp_mode)
{
    struct led_state_obj *obj;
    const struct led_state_map *map;

    log_info("led_name = %d, disp_mode = 0x%x\n", name, disp_mode);

    for (int i = 0; ; i++) {
        map = &g_led_state_table[i];
        if (map->name == 0) {
            PWMLED_LOG_ERR("search END, may no this effect config");
            break;
        }
        if (name != map->name) {
            /* PWMLED_LOG_DEBUG("find next effect to match from g_led_state_table,map->name = %d", map->name); */
            continue;
        }
        PWMLED_LOG_SUCC("Find effect Succ");
        const u8 *arg1 = (const u8 *)map->arg1;

        obj = zalloc(sizeof(*obj));
        if (!obj) {
            PWMLED_LOG_ERR("obj zalloc fail");
            return;
        }
        obj->name           = name;
        obj->disp_mode      = disp_mode;
        if (map->time_flag == TIME_EFFECT_MODE) {
            obj->time       = (const led_pdata_t *)arg1;
        } else {
            obj->table      = (const struct led_state_item *)arg1;
            obj->table_size = map->time_flag;
        }
        led_ui_add_state(obj);
        break;
    }
}


enum led_state_name led_ui_get_state_name()
{
    return __this->curr_state_name;
}


//*******************************************************************


void pwm_led_hw_cbfunc(u32 cbpriv)
{
    int msg[2];
    struct led_state_obj *obj;

    spin_lock(&g_led_lock);
    obj = led_ui_get_state(cbpriv);
    if (obj) {
        obj->action_index = 0xff;
        msg[0] = LED_MSG_STATE_END;
        msg[1] = cbpriv;
    }
    spin_unlock(&g_led_lock);
    if (obj) {
        app_send_message_from(MSG_FROM_PWM_LED, 8, msg);
    }
}


void log_pwm_led_info()
{
#ifdef PWM_LED_DEBUG_ENABLE
#if 0
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));

    printf("rets_addr :0x%x", rets_addr);

#define led_debug   printf
    led_debug("======== PWM LED CONFIG ======");
    led_debug("PWM_CON0 	= 0x%x", JL_PLED->CON0);
    led_debug("PWM_CON1 	= 0x%x", JL_PLED->CON1);
    led_debug("PWM_CON2 	= 0x%x", JL_PLED->CON2);
    led_debug("PWM_CON3 	= 0x%x", JL_PLED->CON3);
    led_debug("PRD_DIVL     = 0x%x", JL_PLED->PRD_DIVL);

    led_debug("BRI_PRDL      = 0x%x", JL_PLED->BRI_PRDL);
    led_debug("BRI_PRDH      = 0x%x", JL_PLED->BRI_PRDH);

    led_debug("BRI_DUTY0L    = 0x%x", JL_PLED->BRI_DUTY0L);
    led_debug("BRI_DUTY0H    = 0x%x", JL_PLED->BRI_DUTY0H);

    led_debug("BRI_DUTY1L    = 0x%x", JL_PLED->BRI_DUTY1L);
    led_debug("BRI_DUTY1H    = 0x%x", JL_PLED->BRI_DUTY1H);

    led_debug("PWM1_DUTY0    = 0x%x", JL_PLED->DUTY0);
    led_debug("PWM1_DUTY1    = 0x%x", JL_PLED->DUTY1);
    led_debug("PWM1_DUTY2    = 0x%x", JL_PLED->DUTY2);
    led_debug("PWM1_DUTY3    = 0x%x", JL_PLED->DUTY3);

    led_debug("JL_PORTB->DIR = 0x%x", JL_PORTB->DIR);
    led_debug("JL_PORTB->OUT = 0x%x", JL_PORTB->OUT);
    led_debug("JL_PORTB->PU = 0x%x", JL_PORTB->PU);
    led_debug("JL_PORTB->PD = 0x%x", JL_PORTB->PD);
    led_debug("JL_PORTB->DIE = 0x%x", JL_PORTB->DIE);
#endif
#endif
}
//*******************************************************************


int pwm_led_early_init()
{
    __this = zalloc(sizeof(struct pwm_led_handler));
    INIT_LIST_HEAD(&__this->sta_list);

    led_effect_board_init(&board_cfg_hw_data);


    return 0;
}
platform_initcall(pwm_led_early_init);


void led_ui_manager_close(void)
{
    led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS);
    __this->current_state_loop = 0;
}

u8 led_ui_manager_status_busy_query(void)
{
    struct led_state_obj *obj;
    obj = led_ui_get_first_state();
    if (!obj) {
        PWMLED_LOG_ERR("obj get fail");
        return LED_UI_STATUS_IDLE;
    }

    if (obj->time) {
        if (pwm_led_is_working() == 1) {
            return LED_UI_STATUS_ACTIVE;
        } else {
            return LED_UI_STATUS_IDLE;
        }


    } else if (obj->table) {
        if (__this->current_state_loop) {
            return LED_UI_STATUS_ACTIVE;
        } else {
            return LED_UI_STATUS_IDLE;
        }
    }

    return LED_UI_STATUS_IDLE;
}

static void wait_led_ui_stop(void *p)
{
}


int led_ui_state_is_idle()
{
    // 等待LED状态结束, 超时10s关机
    static u32 start_jiffies = 0;
    if (led_ui_manager_status_busy_query()) {
        if (start_jiffies == 0) {
            start_jiffies = jiffies;
        }
        int msec = jiffies_offset_to_msec(start_jiffies, jiffies);
        if (msec < 10 * 1000) {
            sys_timeout_add_2_task(NULL, wait_led_ui_stop, 50, "app_core");
            return 0;
        }
    }
    start_jiffies = 0;
    return 1;
}


#endif

