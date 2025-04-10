#include "cpu/includes.h"
#include "app_msg.h"
#include "led_api.h"
#include "pwm_led.h"
#include "two_io_led.h"
#include "gpio.h"
#include "system/timer.h"
#include "app_config.h"
#include "resource_multiplex.h"


#if 1

#define LOG_TAG_CONST       LED
#define LOG_TAG             "[LED]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else

#define log_debug   printf

#endif


const led_board_cfg_t *led_board_cfg = NULL;
static OS_SEM *led_driver_ready_sem;
static led_pdata_t led_effect_cfg;



static void led_effect_output_by_hardware(led_pdata_t *led_effect, int cbpriv)
{
    u32 time_unit = 50;
    pwm_led_pdata_t pled;
    memset(&pled, 0, sizeof(pwm_led_pdata_t));
    if (led_board_cfg->layout == ONE_IO_ONE_LED) {
        pled.port0 = led_board_cfg->led0.port;
        pled.port1 = -1;
        if (led_board_cfg->led0.logic == BRIGHT_BY_HIGH) {
            pled.first_logic = 0;
            pled.h_pwm_duty = led_board_cfg->led0.brightness;
        } else {
            pled.first_logic = 1;
            pled.l_pwm_duty = led_board_cfg->led0.brightness;
        }
        pled.alternate_out = 0;
    } else {
        pled.port0 = led_board_cfg->led0.port;
        pled.port1 = led_board_cfg->led1.port;
        pled.alternate_out = 0;
        if (led_effect->ctl_option == CTL_LED0_ONLY) {
            pled.port1 = -1;
            if (led_board_cfg->led0.logic == BRIGHT_BY_HIGH) {
                pled.first_logic = 0;
                pled.h_pwm_duty = led_board_cfg->led0.brightness;
            } else {
                pled.first_logic = 1;
                pled.l_pwm_duty = led_board_cfg->led0.brightness;
            }
        } else if (led_effect->ctl_option == CTL_LED1_ONLY) {
            pled.port0 = -1;
            if (led_board_cfg->led1.logic == BRIGHT_BY_HIGH) {
                pled.first_logic = 0;
                pled.h_pwm_duty = led_board_cfg->led1.brightness;
            } else {
                pled.first_logic = 1;
                pled.l_pwm_duty = led_board_cfg->led1.brightness;
            }
        } else if (led_effect->ctl_option == CTL_LED01_ASYNC) {
            pled.first_logic = 0;
            pled.alternate_out = 1;
            if (led_board_cfg->led0.logic == BRIGHT_BY_HIGH) {
                pled.h_pwm_duty = led_board_cfg->led0.brightness;
                pled.l_pwm_duty = led_board_cfg->led1.brightness;
            } else {
                pled.h_pwm_duty = led_board_cfg->led1.brightness;
                pled.l_pwm_duty = led_board_cfg->led0.brightness;
            }
        } else {
            pled.first_logic = 0;
            if (led_board_cfg->led0.logic != led_board_cfg->led1.logic) {
                pled.alternate_out = 1;
            }
            if (led_board_cfg->led0.logic == BRIGHT_BY_HIGH) {
                pled.h_pwm_duty = led_board_cfg->led0.brightness;
                pled.l_pwm_duty = led_board_cfg->led1.brightness;
            } else {
                pled.h_pwm_duty = led_board_cfg->led1.brightness;
                pled.l_pwm_duty = led_board_cfg->led0.brightness;
            }
        }
    }
    pled.out_mode = led_effect->ctl_mode;
    pled.ctl_cycle = led_effect->ctl_cycle * time_unit;
    pled.ctl_cycle_num = led_effect->ctl_cycle_num;
    pled.cbfunc = led_effect->cbfunc;
    pled.cbpriv = cbpriv;

    u32 cycle_base = 50;
    u32 cycle_pnum = 2;
    if (__get_lrc_hz() / 100000) {
        cycle_base = 320;
        cycle_pnum = 4;
    }

    switch (led_effect->ctl_mode) {
    case CYCLE_ONCE_BRIGHT:
        pled.out_once.pwm_out_time = led_effect->once_bright.bright_time * time_unit;
        pled.pwm_cycle = pled.out_once.pwm_out_time * cycle_pnum;
        pled.pwm_cycle = pled.pwm_cycle > cycle_base ? cycle_base : pled.pwm_cycle;
        break;
    case CYCLE_TWICE_BRIGHT:
        pled.out_twice.first_pwm_out_time  = led_effect->twice_bright.first_bright_time * time_unit;
        pled.out_twice.second_pwm_out_time = led_effect->twice_bright.second_bright_time * time_unit;
        pled.out_twice.pwm_gap_time        = led_effect->twice_bright.bright_gap_time * time_unit;
        pled.pwm_cycle = pled.out_twice.first_pwm_out_time * cycle_pnum;
        pled.pwm_cycle = pled.pwm_cycle > cycle_base ? cycle_base : pled.pwm_cycle;
        break;
    case CYCLE_BREATHE_BRIGHT:
        pled.out_breathe.pwm_out_time = led_effect->breathe_bright.bright_time * time_unit;
        pled.out_breathe.pwm_duty_max_keep_time = led_effect->breathe_bright.brightest_keep_time * time_unit;
        pled.pwm_cycle = pled.out_breathe.pwm_out_time * cycle_pnum;
        pled.pwm_cycle = pled.pwm_cycle > cycle_base ? cycle_base : pled.pwm_cycle;
        break;
    case ALWAYS_BRIGHT:
        pled.ctl_cycle = 5;
        pled.pwm_cycle = 32;
        pled.out_mode = CYCLE_ONCE_BRIGHT;
        pled.out_once.pwm_out_time = pled.ctl_cycle;
        pled.cbfunc = NULL;
        break;
    case ALWAYS_EXTINGUISH:
        pwm_led_hw_close();
        return;
    }
    pwm_led_hw_init(&pled);
}

static void led_effect_output_by_software(led_pdata_t *led_effect, int cbpriv)
{
    u32 time_unit = 50;
    two_io_pdata_t tled;
    memset(&tled, 0, sizeof(two_io_pdata_t));
    tled.io0 = led_board_cfg->led0.port;
    tled.io1 = led_board_cfg->led1.port;
    if (led_effect->ctl_option == CTL_LED0_ONLY) {
        tled.io0_pwm_duty = led_board_cfg->led0.brightness;
    } else if (led_effect->ctl_option == CTL_LED1_ONLY) {
        tled.io1_pwm_duty = led_board_cfg->led1.brightness;
    } else if (led_effect->ctl_option >= CTL_LED01_ASYNC) {
        tled.io0_pwm_duty = led_board_cfg->led0.brightness;
        tled.io1_pwm_duty = led_board_cfg->led1.brightness;
    }
    u32 ctl_cycle = led_effect->ctl_cycle * time_unit;
    tled.soft_pwm_cycle = 10;
    tled.ctl_option = led_effect->ctl_option;
    tled.ctl_mode = led_effect->ctl_mode;
    tled.ctl_cycle_num = led_effect->ctl_cycle_num;
    tled.ctl_cycle_cbfunc = led_effect->cbfunc;
    tled.ctl_cycle_cbpriv = cbpriv;
    if (led_board_cfg->layout == THREE_IO_TWO_LED) {
        tled.com_pole_is_io = 1;
        tled.com_pole_io = led_board_cfg->com_pole_port;
#if LED_IO_SUPPORT_MUX
        if (resource_multiplex_is_registered(led_board_cfg->com_pole_port)) {
            tled.io0_pwm_duty = tled.io0_pwm_duty > 60 ? 60 : tled.io0_pwm_duty;
            tled.io1_pwm_duty = tled.io1_pwm_duty > 60 ? 60 : tled.io1_pwm_duty;
        }
#endif
    } else {
        tled.com_pole_is_io = 0;
        tled.com_pole_io = -1;
    }
    switch (led_effect->ctl_mode) {
    case CYCLE_ONCE_BRIGHT:
        tled.cycle_once.out_time = led_effect->once_bright.bright_time * time_unit;
        tled.cycle_once.idle_time = ctl_cycle - tled.cycle_once.out_time;
        break;
    case CYCLE_TWICE_BRIGHT:
        tled.cycle_twice.out_time = led_effect->twice_bright.first_bright_time * time_unit;
        tled.cycle_twice.gap_time = led_effect->twice_bright.bright_gap_time * time_unit;
        tled.cycle_twice.out_time1 = led_effect->twice_bright.second_bright_time * time_unit;
        tled.cycle_twice.idle_time = ctl_cycle - (tled.cycle_twice.out_time + tled.cycle_twice.gap_time + tled.cycle_twice.out_time1);
        break;
    case CYCLE_BREATHE_BRIGHT:
        tled.cycle_breathe.duty_step = 10;
        tled.cycle_breathe.duty_keep_time = led_effect->breathe_bright.brightest_keep_time * time_unit;
        tled.cycle_breathe.duty_step_time = led_effect->breathe_bright.bright_time * time_unit;
        tled.cycle_breathe.duty_step_time -= tled.cycle_breathe.duty_keep_time;
        tled.cycle_breathe.duty_step_time /= 2;
        tled.cycle_breathe.idle_time = ctl_cycle - (led_effect->breathe_bright.bright_time * time_unit);
        tled.cycle_breathe.idle_time = tled.cycle_breathe.idle_time ? tled.cycle_breathe.idle_time : time_unit;
        break;
    case ALWAYS_BRIGHT:
        tled.ctl_cycle_num = 0;
        tled.ctl_cycle_cbfunc = NULL;
        break;
    case ALWAYS_EXTINGUISH:
        two_io_ctl_close();
        return;
    }
    two_io_ctl_init(&tled);
}

void led_effect_info_dump(led_pdata_t *led_effect)
{
    printf("led0.port = %d", led_board_cfg->led0.port);
    printf("led0.logic = %d", led_board_cfg->led0.logic);
    printf("led0.brightness = %d", led_board_cfg->led0.brightness);
    printf("led1.port = %d", led_board_cfg->led1.port);
    printf("led1.logic = %d", led_board_cfg->led1.logic);
    printf("led1.brightness = %d", led_board_cfg->led1.brightness);
    printf("layout = %d", led_board_cfg->layout);
    printf("ctl_option = %d", led_effect->ctl_option);
    printf("ctl_mode = %d", led_effect->ctl_mode);
    printf("ctl_cycle = %d", led_effect->ctl_cycle);
    printf("ctl_cycle_num = %d", led_effect->ctl_cycle_num);
    printf("cbfunc = %d", (u32)led_effect->cbfunc);
    if (led_effect->ctl_mode == 0) {
        printf("once_bright.bright_time = %d", led_effect->once_bright.bright_time);
    } else if (led_effect->ctl_mode == 1) {
        printf("twice_bright.first_bright_time = %d", led_effect->twice_bright.first_bright_time);
        printf("twice_bright.bright_gap_time = %d", led_effect->twice_bright.bright_gap_time);
        printf("twice_bright.second_bright_time = %d", led_effect->twice_bright.second_bright_time);
    } else if (led_effect->ctl_mode == 2) {
        printf("breathe_bright.bright_time = %d", led_effect->breathe_bright.bright_time);
        printf("breathe_bright.brightest_keep_time = %d", led_effect->breathe_bright.brightest_keep_time);
    }
}


void led_effect_init(led_pdata_t *led_effect, int cbpriv)
{
    /* led_effect_info_dump(led_effect); */
    if (led_effect->board_cfg) {
        led_board_cfg = led_effect->board_cfg;
    }

    if (led_board_cfg == NULL) {
        return;
    }

    u32 use_pwm_led;
    if (led_board_cfg->layout <= ONE_IO_TWO_LED) {
        use_pwm_led = 1;
#if LED_IO_SUPPORT_MUX
    } else if ((led_board_cfg->layout == THREE_IO_TWO_LED) && \
               (resource_multiplex_is_registered(led_board_cfg->com_pole_port))) {
        use_pwm_led = 0;
#endif
    } else if (led_effect->ctl_mode == CYCLE_BREATHE_BRIGHT) {
        use_pwm_led = 1;
    } else if ((led_effect->ctl_option == CTL_LED0_ONLY) || \
               (led_effect->ctl_option == CTL_LED1_ONLY)) {
        use_pwm_led = 1;
    } else if ((led_board_cfg->led0.brightness == led_board_cfg->led1.brightness) && \
               (led_board_cfg->led0.brightness != 100) && \
               (led_board_cfg->led0.brightness != 0)) {
        use_pwm_led = 1;
    } else {
        use_pwm_led = 0;
    }

    if (use_pwm_led) {
        led_effect_output_by_hardware(led_effect, cbpriv);
    } else {
#if TCFG_LED_LAYOUT == TWO_IO_TWO_LED
        led_effect_output_by_software(led_effect, cbpriv);
#endif
        return;
    }

    if (led_board_cfg->layout == THREE_IO_TWO_LED) {//第三IO
        if (led_effect->ctl_mode == ALWAYS_EXTINGUISH) {
            gpio_set_mode(IO_PORT_SPILT(led_board_cfg->com_pole_port), PORT_HIGHZ);
        } else {
            gpio_set_mode(IO_PORT_SPILT(led_board_cfg->com_pole_port), PORT_OUTPUT_LOW);
        }
    }
}

#define MSG_LED_EFFECT (Q_MSG + 1)

void led_effect_output(led_pdata_t *led_effect, int cbpriv)
{
    memcpy(&led_effect_cfg, led_effect, sizeof(led_pdata_t));
    int msg[2];
    msg[0] = (int)&led_effect_cfg;
    msg[1] = cbpriv;
    os_taskq_post_type("led_driver", MSG_LED_EFFECT, 2, msg);
}

void led_driver_task(void *priv)
{
    int ret;
    int msg[16] = {0};

    os_sem_post(led_driver_ready_sem);

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        switch (msg[0]) {
        case MSG_LED_EFFECT:
            led_effect_init((led_pdata_t *)msg[1], msg[2]);
            break;
        default:
            break;
        }
    }
}

void led_effect_board_init(const led_board_cfg_t *cfg)
{
    led_board_cfg = cfg;

    led_driver_ready_sem = (OS_SEM *)malloc(sizeof(OS_SEM));
    os_sem_create(led_driver_ready_sem, 0);

    int err = task_create(led_driver_task, NULL, "led_driver");
    if (err != OS_NO_ERR) {
        ASSERT(0, "task_create fail!!! %x\n", err);
    } else {
        os_sem_pend(led_driver_ready_sem, 0);
    }
    free(led_driver_ready_sem);
    led_driver_ready_sem = NULL;
}


