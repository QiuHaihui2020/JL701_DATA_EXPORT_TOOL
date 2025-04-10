#include "cpu/includes.h"
#include "two_io_led.h"
#include "gpio.h"
#include "asm/power_interface.h"
#include "system/timer.h"
#include "app_config.h"
#include "asm/sdmmc.h"
#include "resource_multiplex.h"


#if 1

#define LOG_TAG_CONST       TWO_IO_LED
#define LOG_TAG             "[TWO_IO_LED]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else

#define log_debug   printf

#endif


#if LED_IO_SUPPORT_MUX

#ifdef CONFIG_CPU_BR29
#define TWO_IO_MUX_SDIO_USE_BRK     0
#else
#define TWO_IO_MUX_SDIO_USE_BRK     0
#endif

#endif


typedef struct two_io_ctl_platform_data {
    u8 ctl_time_idx;
    u8 ctl_time_idx_max;
    u8 ctl_cycle_cnt;
    u8 iox_state;
    u8 io0_pwm_h_time;
    u8 io1_pwm_h_time;
    u8 io0_cur_pwm_duty;
    u8 io1_cur_pwm_duty;
    u8 io0_pwm_duty_change_dir;
    u8 io1_pwm_duty_change_dir;
    u16 io0_h_timeout;
    u16 io1_h_timeout;
    u16 pole_io_timeout;
    u16 ctl_timer_add;
    u16 pwm_timer_add;
    u16 duty_step_time_add;
    two_io_pdata_t two_io;
} two_io_ctl_pdata_t;

static two_io_ctl_pdata_t *__this;



static void two_io_mux_free(void *priv)
{
    u8 io = *((u8 *)priv);
    if (io == __this->two_io.com_pole_io) {
        gpio_set_mode(IO_PORT_SPILT(io), PORT_HIGHZ);
        gpio_disable_function(IO_PORT_SPILT(io), PORT_FUNC_NULL);
    } else {
        gpio_set_mode(IO_PORT_SPILT(io), PORT_OUTPUT_LOW);
        gpio_disable_function(IO_PORT_SPILT(io), PORT_FUNC_NULL);
    }
}

#if TWO_IO_MUX_SDIO_USE_BRK
static void two_io_mux_sdc_brk(void *priv)
{
    __this->pole_io_timeout = 0;
    u32 *io0_outreg = gpio2crossbar_outreg(__this->two_io.io0);
    u32 *io1_outreg = gpio2crossbar_outreg(__this->two_io.io1);
    gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
    gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
    (*io0_outreg) |= (0b11);
    if (((*io1_outreg) & (0b11)) == 0) {
        (*io1_outreg) |= (0b11);
    }
    sdc_brk_exit();
}
#endif

static void two_io_set_idle(void *priv)
{
    u8 io = *((u8 *)priv);
    if (io == __this->two_io.io0) {
        __this->io0_h_timeout = 0;
        __this->iox_state &= ~BIT(0);
    }
    if (io == __this->two_io.io1) {
        __this->io1_h_timeout = 0;
        __this->iox_state &= ~BIT(1);
    }

    if (__this->two_io.com_pole_is_io) {
#if LED_IO_SUPPORT_MUX
        u32 io0 = __this->two_io.io0;
        u32 io1 = __this->two_io.io1;
        u32 pole_io = __this->two_io.com_pole_io;
        u32 requester = (u32)__this;
        if (resource_multiplex_is_registered(pole_io)) {
            if (__this->iox_state) {
                return;
            }
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);//CMD
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_LOW);//CLK
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_LOW);//CMD
            if ((resource_multiplex_has_requester_waiting(io1, requester)) || \
                (resource_multiplex_has_requester_waiting(io0, requester))) {
                resource_multiplex_free(io1, requester);
                resource_multiplex_free(io0, requester);
            }
            resource_multiplex_free(pole_io, requester);
        } else
#endif
        {
            gpio_set_mode(IO_PORT_SPILT(io), PORT_HIGHZ);
            if (__this->iox_state) {
                return;
            }
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
        }
    } else {
        gpio_set_mode(IO_PORT_SPILT(io), PORT_HIGHZ);
    }

}

static void two_io_set_output(void *priv)
{
    if ((__this->io0_pwm_h_time == 0) && \
        (__this->io1_pwm_h_time == 0)) {
        return;
    }
    if ((__this->io0_h_timeout) || \
        (__this->io1_h_timeout) || \
        (__this->pole_io_timeout)) {
        return;
    }

#if LED_IO_SUPPORT_MUX

    u32 io0 = __this->two_io.io0;
    u32 io1 = __this->two_io.io1;
    u32 pole_io = __this->two_io.com_pole_io;
    u32 requester = (u32)__this;

    if ((__this->two_io.com_pole_is_io) && \
        (resource_multiplex_is_registered(pole_io))) {
        int err0 = resource_multiplex_check_request(io0, requester, FORGO_WHEN_RESOURCE_BUSY, NULL);
        int err1 = resource_multiplex_check_request(io1, requester, FORGO_WHEN_RESOURCE_BUSY, NULL);
        if ((err1 == 0) && (err0 == 0)) {
        } else if ((err1) && (err0 == 0)) {
            resource_multiplex_free(io0, requester);
            return;
        } else if ((err1 == 0) && (err0)) {
            resource_multiplex_free(io1, requester);
            return;
        } else {
#if TWO_IO_MUX_SDIO_USE_BRK
            u32 *io0_outreg = gpio2crossbar_outreg(__this->two_io.io0);
            u32 *io1_outreg = gpio2crossbar_outreg(__this->two_io.io1);
            if (__this->io1_pwm_h_time) {
                sdc_brk_set_clk_polarity(1);
            } else {
                sdc_brk_set_clk_polarity(0);
            }
            int err, cnt = 0;
__brk_enter:
            err = sdc_brk_enter();
            if (err == 0) {
                err = sdc_brk_wait_pend();
                if (err) {
                    cnt ++;
                    if (cnt > 3) {
                        return;
                    }
                    goto __brk_enter;
                }
                gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
                if (__this->io0_pwm_h_time) {
                    gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_INPUT_PULLUP_10K);
                    (*io0_outreg) &= ~(0b11);
                    udelay(1);
                    if (gpio_read(__this->two_io.io0)) {
                        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
                    } else {
                        (*io0_outreg) |= (0b11);
                        sdc_brk_exit();
                        udelay(10);
                        goto __brk_enter;
                    }
                } else {
                    gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_INPUT_PULLDOWN_10K);
                    (*io0_outreg) &= ~(0b11);
                    udelay(1);
                    if (gpio_read(__this->two_io.io0)) {
                        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_INPUT_PULLUP_10K);
                        (*io0_outreg) |= (0b11);
                        sdc_brk_exit();
                        udelay(10);
                        goto __brk_enter;
                    } else {
                        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_INPUT_PULLUP_10K);
                        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_LOW);
                    }
                }
            } else {
                gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
                (*io0_outreg) &= ~(0b11);
                gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_HIGH);
                (*io1_outreg) &= ~(0b11);
                if (__this->io1_pwm_h_time == 0) {
                    gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_LOW);
                }
                if (__this->io0_pwm_h_time == 0) {
                    gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_LOW);
                }
            }
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_OUTPUT_LOW);
            if (__this->io1_pwm_h_time) {
                __this->pole_io_timeout = sys_hi_timeout_add(NULL, two_io_mux_sdc_brk, __this->io1_pwm_h_time);
            } else {
                __this->pole_io_timeout = sys_hi_timeout_add(NULL, two_io_mux_sdc_brk, __this->io0_pwm_h_time);
            }
#endif
            return;
        }
        resource_multiplex_check_request(pole_io, requester, WAIT_RESOURCE_FREE, NULL);
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_OUTPUT_HIGH);
        gpio_disable_function(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_FUNC_NULL);
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
        gpio_disable_function(IO_PORT_SPILT(__this->two_io.io0), PORT_FUNC_NULL);
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_HIGH);
        gpio_disable_function(IO_PORT_SPILT(__this->two_io.io1), PORT_FUNC_NULL);
        if (__this->io1_pwm_h_time == 0) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_LOW);
        }
        if (__this->io0_pwm_h_time == 0) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_LOW);
        }
    }
#endif

    if ((__this->io0_pwm_h_time) && (__this->io0_pwm_h_time < __this->two_io.soft_pwm_cycle)) {
        __this->io0_h_timeout = sys_hi_timeout_add(&(__this->two_io.io0), two_io_set_idle, __this->io0_pwm_h_time);
    }
    if ((__this->io1_pwm_h_time) && (__this->io1_pwm_h_time < __this->two_io.soft_pwm_cycle)) {
        __this->io1_h_timeout = sys_hi_timeout_add(&(__this->two_io.io1), two_io_set_idle, __this->io1_pwm_h_time);
    }

    if (__this->io0_pwm_h_time) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
        __this->iox_state |=  BIT(0);
    }
    if (__this->io1_pwm_h_time) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_HIGH);
        __this->iox_state |=  BIT(1);
    }
    if (__this->two_io.com_pole_is_io) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_OUTPUT_LOW);
    }
}


static void two_io_soft_pwm_init(void)
{
#if LED_IO_SUPPORT_MUX
    src_mux_cbfunc_info cbfunc_info;
    u32 io0 = __this->two_io.io0;
    u32 io1 = __this->two_io.io1;
    u32 pole_io = __this->two_io.com_pole_io;
    u32 requester = (u32)__this;
    if ((__this->two_io.com_pole_is_io) && \
        (resource_multiplex_is_registered(pole_io))) {
        resource_multiplex_check_request(io0, requester, WAIT_RESOURCE_FREE, NULL);
        resource_multiplex_check_request(io1, requester, WAIT_RESOURCE_FREE, NULL);
        resource_multiplex_check_request(pole_io, requester, WAIT_RESOURCE_FREE, NULL);

        cbfunc_info.cbfunc_mode = 0;
        cbfunc_info.cbfunc_owner = (u32)__this;
        cbfunc_info.cbfunc = (void *)two_io_mux_free;
        cbfunc_info.cbfunc_arg = (void *) & (__this->two_io.io0);
        resource_multiplex_register_free_cbfunc(io0, &cbfunc_info);
        cbfunc_info.cbfunc_arg = (void *) & (__this->two_io.io1);
        resource_multiplex_register_free_cbfunc(io1, &cbfunc_info);
        cbfunc_info.cbfunc_arg = (void *) & (__this->two_io.com_pole_io);
        resource_multiplex_register_free_cbfunc(pole_io, &cbfunc_info);

#if TWO_IO_MUX_SDIO_USE_BRK

        sdc_brk_t brk;
        brk.check_mode = 1;
        sdc_brk_init(&brk);
#endif
    }

#endif

    two_io_set_output(NULL);
    __this->pwm_timer_add = sys_timer_add(NULL, two_io_set_output, __this->two_io.soft_pwm_cycle);
}

static void two_io_soft_pwm_close(void)
{
    if (__this->io0_pwm_h_time) {
        if (__this->io0_h_timeout) {
            sys_hi_timer_del(__this->io0_h_timeout);
            two_io_set_idle(&(__this->two_io.io0));
        } else if (__this->io0_pwm_h_time >= __this->two_io.soft_pwm_cycle) {
            two_io_set_idle(&(__this->two_io.io0));
        }
    }
    if (__this->io1_pwm_h_time) {
        if (__this->io1_h_timeout) {
            sys_hi_timer_del(__this->io1_h_timeout);
            two_io_set_idle(&(__this->two_io.io1));
        } else if (__this->io1_pwm_h_time >= __this->two_io.soft_pwm_cycle) {
            two_io_set_idle(&(__this->two_io.io1));
        }
    }
    if (__this->pwm_timer_add) {
        sys_timer_del(__this->pwm_timer_add);
        __this->pwm_timer_add = 0;
    }

#if LED_IO_SUPPORT_MUX
    u32 io0 = __this->two_io.io0;
    u32 io1 = __this->two_io.io1;
    u32 pole_io = __this->two_io.com_pole_io;
    u32 requester = (u32)__this;
    if ((__this->two_io.com_pole_is_io) && \
        (resource_multiplex_is_registered(pole_io))) {
        resource_multiplex_remove_free_cbfunc(pole_io, requester);
        resource_multiplex_remove_free_cbfunc(io0, requester);
        resource_multiplex_remove_free_cbfunc(io1, requester);
        resource_multiplex_free(pole_io, requester);
        resource_multiplex_free(io0, requester);
        resource_multiplex_free(io1, requester);
    }
#endif
}

static void two_io_change_breathe_duty(void *priv)
{
    if (__this->io0_pwm_duty_change_dir == 1) {
        __this->io0_cur_pwm_duty += __this->two_io.cycle_breathe.duty_step;
        if (__this->io0_cur_pwm_duty >= __this->two_io.io0_pwm_duty) {
            __this->io0_pwm_duty_change_dir = 0;
            __this->io0_pwm_h_time = __this->two_io.io0_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        } else {
            __this->io0_pwm_h_time = __this->io0_cur_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        }
    } else if (__this->io0_pwm_duty_change_dir == 2) {
        __this->io0_cur_pwm_duty -= __this->two_io.cycle_breathe.duty_step;
        if (__this->io0_cur_pwm_duty == 0) {
            __this->io0_pwm_duty_change_dir = 0;
        }
        __this->io0_pwm_h_time = __this->io0_cur_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
    }
    if (__this->io1_pwm_duty_change_dir == 1) {
        __this->io1_cur_pwm_duty += __this->two_io.cycle_breathe.duty_step;
        if (__this->io1_cur_pwm_duty >= __this->two_io.io1_pwm_duty) {
            __this->io1_pwm_duty_change_dir = 0;
            __this->io1_pwm_h_time = __this->two_io.io1_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        } else {
            __this->io1_pwm_h_time = __this->io1_cur_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        }
    } else if (__this->io1_pwm_duty_change_dir == 2) {
        __this->io1_cur_pwm_duty -= __this->two_io.cycle_breathe.duty_step;
        if (__this->io1_cur_pwm_duty == 0) {
            __this->io1_pwm_duty_change_dir = 0;
        }
        __this->io1_pwm_h_time = __this->io1_cur_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
    }
}

static void two_io_cycle_change_breathe(void *priv)
{
    u32 max_duty = __this->two_io.io0_pwm_duty > __this->two_io.io1_pwm_duty ? __this->two_io.io0_pwm_duty : __this->two_io.io1_pwm_duty;
    u32 step = max_duty / __this->two_io.cycle_breathe.duty_step;
    u32 step_time = __this->two_io.cycle_breathe.duty_step_time / step;

    if (__this->ctl_time_idx == 0) {
        __this->ctl_time_idx ++;
        if (__this->two_io.io0_pwm_duty) {
            __this->io0_pwm_duty_change_dir = 1;
        }
        if (__this->two_io.io1_pwm_duty) {
            __this->io1_pwm_duty_change_dir = 1;
        }
        if (__this->two_io.ctl_option == 2) {
            if (__this->ctl_cycle_cnt % 2) {
                __this->io0_pwm_duty_change_dir = 0;
            } else {
                __this->io1_pwm_duty_change_dir = 0;
            }
        }
        two_io_soft_pwm_init();
        __this->duty_step_time_add = sys_timer_add(NULL, two_io_change_breathe_duty, step_time);
        sys_timer_modify(__this->ctl_timer_add, __this->two_io.cycle_breathe.duty_step_time);
    } else if (__this->ctl_time_idx == 1) {
        __this->ctl_time_idx ++;
        if (__this->two_io.cycle_breathe.duty_keep_time) {
            sys_timer_modify(__this->ctl_timer_add, __this->two_io.cycle_breathe.duty_keep_time);
            return;
        }
    }
    if (__this->ctl_time_idx == 2) {
        __this->ctl_time_idx ++;
        if (__this->two_io.io0_pwm_duty) {
            __this->io0_pwm_duty_change_dir = 2;
        }
        if (__this->two_io.io1_pwm_duty) {
            __this->io1_pwm_duty_change_dir = 2;
        }
        if (__this->two_io.ctl_option == 2) {
            if (__this->ctl_cycle_cnt % 2) {
                __this->io0_pwm_duty_change_dir = 0;
            } else {
                __this->io1_pwm_duty_change_dir = 0;
            }
        }
        sys_timer_modify(__this->ctl_timer_add, __this->two_io.cycle_breathe.duty_step_time);
    } else if (__this->ctl_time_idx == 3) {
        if (__this->duty_step_time_add) {
            sys_timer_del(__this->duty_step_time_add);
            __this->duty_step_time_add = 0;
        }
        two_io_soft_pwm_close();
        __this->ctl_time_idx = 0;
        __this->ctl_cycle_cnt ++;
        if ((__this->two_io.ctl_cycle_num) && \
            (__this->two_io.ctl_cycle_num <= __this->ctl_cycle_cnt) && \
            (__this->two_io.ctl_cycle_cbfunc)) {
            __this->two_io.ctl_cycle_cbfunc(__this->two_io.ctl_cycle_cbpriv);
            if (__this->ctl_timer_add) {
                sys_timer_del(__this->ctl_timer_add);
                __this->ctl_timer_add = 0;
            }
            free(__this);
            __this = NULL;
            return;
        } else {
            sys_timer_modify(__this->ctl_timer_add, __this->two_io.cycle_breathe.idle_time);
        }
    }
}

static void two_io_cycle_change_state(void *priv)
{
    if ((__this->ctl_time_idx % 2) == 0) {
        __this->ctl_time_idx ++;
        if (__this->two_io.ctl_option == 2) {
            if (__this->ctl_cycle_cnt % 2) {
                __this->io0_pwm_h_time = 0;
                __this->io1_pwm_h_time = __this->two_io.io1_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
            } else {
                __this->io0_pwm_h_time = __this->two_io.io0_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
                __this->io1_pwm_h_time = 0;
            }
        }
        if (__this->io0_pwm_h_time == __this->two_io.soft_pwm_cycle) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
            __this->io0_pwm_h_time = 0;
            __this->iox_state |=  BIT(0);
        }
        if (__this->io1_pwm_h_time == __this->two_io.soft_pwm_cycle) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_HIGH);
            __this->io1_pwm_h_time = 0;
            __this->iox_state |=  BIT(1);
        }
        if ((__this->two_io.com_pole_is_io)  && (__this->iox_state)) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_OUTPUT_LOW);
        }
        if (__this->io0_pwm_h_time || __this->io1_pwm_h_time) {
            two_io_soft_pwm_init();
        }
    } else {
        __this->ctl_time_idx ++;
        if (__this->io0_pwm_h_time || __this->io1_pwm_h_time) {
            two_io_soft_pwm_close();
        }
        if ((__this->two_io.com_pole_is_io)  && (__this->iox_state)) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
        }
        if ((__this->two_io.io0_pwm_duty == 100) && (__this->io0_pwm_h_time == 0)) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_HIGHZ);
            __this->io0_pwm_h_time =  __this->two_io.soft_pwm_cycle;
            __this->iox_state &= ~BIT(0);
        }
        if ((__this->two_io.io1_pwm_duty == 100) && (__this->io1_pwm_h_time == 0)) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_HIGHZ);
            __this->io1_pwm_h_time = __this->two_io.soft_pwm_cycle;
            __this->iox_state &= ~BIT(1);
        }
        if (__this->ctl_time_idx == __this->ctl_time_idx_max) {
            __this->ctl_time_idx = 0;
            __this->ctl_cycle_cnt ++;
            if ((__this->two_io.ctl_cycle_num) && \
                (__this->two_io.ctl_cycle_num <= __this->ctl_cycle_cnt) && \
                (__this->two_io.ctl_cycle_cbfunc)) {
                __this->two_io.ctl_cycle_cbfunc(__this->two_io.ctl_cycle_cbpriv);
                if (__this->ctl_timer_add) {
                    sys_timer_del(__this->ctl_timer_add);
                    __this->ctl_timer_add = 0;
                }
                free(__this);
                __this = NULL;
                return;
            }
        }
    }
    u16 next_time = ((u16 *)(&(__this->two_io.cycle_once.idle_time)))[__this->ctl_time_idx];
    sys_timer_modify(__this->ctl_timer_add, next_time);
}

static void two_io_ctl_cycle_fixed_times(void)
{
    __this->io0_pwm_h_time = 0;
    __this->io1_pwm_h_time = 0;
    if (__this->two_io.ctl_option != 1) {
        __this->io0_pwm_h_time = __this->two_io.io0_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
    }
    if (__this->two_io.ctl_option != 0) {
        __this->io1_pwm_h_time = __this->two_io.io1_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
    }
    if (__this->io0_pwm_h_time || __this->io1_pwm_h_time) {
        __this->ctl_timer_add = sys_timer_add(NULL, two_io_cycle_change_state, __this->two_io.cycle_once.idle_time);
    }
}

static void two_io_ctl_cycle_once(void)
{
    __this->ctl_time_idx = 0;
    __this->ctl_time_idx_max = 2;
    two_io_ctl_cycle_fixed_times();
}

static void two_io_ctl_cycle_twice(void)
{
    __this->ctl_time_idx = 0;
    __this->ctl_time_idx_max = 4;
    two_io_ctl_cycle_fixed_times();
}

static void two_io_ctl_cycle_breathe(void)
{
    __this->io0_pwm_h_time = 0;
    __this->io1_pwm_h_time = 0;
    __this->io0_cur_pwm_duty = 0;
    __this->io1_cur_pwm_duty = 0;
    __this->io0_pwm_duty_change_dir = 0;
    __this->io1_pwm_duty_change_dir = 0;
    if ((__this->two_io.io0_pwm_duty) || (__this->two_io.io1_pwm_duty)) {
        __this->ctl_timer_add = sys_timer_add(NULL, two_io_cycle_change_breathe, __this->two_io.cycle_breathe.idle_time);
    }
}

static void two_io_ctl_always_output(void)
{
    __this->io0_pwm_h_time = 0;
    __this->io1_pwm_h_time = 0;
    if (__this->two_io.ctl_option != 1) {
        if (__this->two_io.io0_pwm_duty == 100) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_OUTPUT_HIGH);
            __this->iox_state |=  BIT(0);
        } else {
            __this->io0_pwm_h_time = __this->two_io.io0_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        }
    }
    if (__this->two_io.ctl_option != 0) {
        if (__this->two_io.io1_pwm_duty == 100) {
            gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_OUTPUT_HIGH);
            __this->iox_state |=  BIT(1);
        } else {
            __this->io1_pwm_h_time = __this->two_io.io1_pwm_duty * __this->two_io.soft_pwm_cycle / 100;
        }
    }
    if ((__this->two_io.com_pole_is_io)  && (__this->iox_state)) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_OUTPUT_LOW);
    }
    if (__this->io0_pwm_h_time || __this->io1_pwm_h_time) {
        two_io_soft_pwm_init();
    }
}

void two_io_ctl_info_dump(void)
{
    printf("io0 = %d", __this->two_io.io0);
    printf("io1 = %d", __this->two_io.io1);
    printf("soft_pwm_cycle = %d", __this->two_io.soft_pwm_cycle);
    printf("io0_pwm_duty = %d", __this->two_io.io0_pwm_duty);
    printf("io1_pwm_duty = %d", __this->two_io.io1_pwm_duty);
    printf("ctl_option = %d", __this->two_io.ctl_option);
    printf("ctl_mode = %d", __this->two_io.ctl_mode);
    printf("ctl_cycle_num = %d", __this->two_io.ctl_cycle_num);
    printf("ctl_cycle_cbfunc = 0x%x", (u32)__this->two_io.ctl_cycle_cbfunc);
    if (__this->two_io.ctl_mode == 0) {
        printf("cycle_once.idle_time = %d", __this->two_io.cycle_once.idle_time);
        printf("cycle_once.out_time = %d", __this->two_io.cycle_once.out_time);
    } else if (__this->two_io.ctl_mode == 1) {
        printf("cycle_twice.idle_time = %d", __this->two_io.cycle_twice.idle_time);
        printf("cycle_twice.out_time = %d", __this->two_io.cycle_twice.out_time);
        printf("cycle_twice.gap_time = %d", __this->two_io.cycle_twice.gap_time);
        printf("cycle_twice.out_time1 = %d", __this->two_io.cycle_twice.out_time1);
    } else if (__this->two_io.ctl_mode == 2) {
        printf("cycle_breathe.idle_time = %d", __this->two_io.cycle_breathe.idle_time);
        printf("cycle_breathe.duty_step = %d", __this->two_io.cycle_breathe.duty_step);
        printf("cycle_breathe.duty_step_time = %d", __this->two_io.cycle_breathe.duty_step_time);
        printf("cycle_breathe.duty_keep_time = %d", __this->two_io.cycle_breathe.duty_keep_time);
    }
}

void two_io_ctl_close(void)
{
    if (__this == NULL) {
        return;
    }
    if (__this->ctl_timer_add) {
        sys_timer_del(__this->ctl_timer_add);
        __this->ctl_timer_add = 0;
    }
    if (__this->duty_step_time_add) {
        sys_timer_del(__this->duty_step_time_add);
        __this->duty_step_time_add = 0;
    }
    two_io_soft_pwm_close();
    if ((__this->two_io.com_pole_is_io)  && (__this->iox_state)) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.com_pole_io), PORT_HIGHZ);
    }
    if ((__this->two_io.io0_pwm_duty == 100) && (__this->io0_pwm_h_time == 0)) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io0), PORT_HIGHZ);
        __this->iox_state &= ~BIT(0);
    }
    if ((__this->two_io.io1_pwm_duty == 100) && (__this->io1_pwm_h_time == 0)) {
        gpio_set_mode(IO_PORT_SPILT(__this->two_io.io1), PORT_HIGHZ);
        __this->iox_state &= ~BIT(1);
    }
    free(__this);
    __this = NULL;
}

void two_io_ctl_init(two_io_pdata_t *pdata)
{
    two_io_ctl_close();
    if (__this == NULL) {
        __this = (two_io_ctl_pdata_t *)malloc(sizeof(two_io_ctl_pdata_t));
        memset(__this, 0, sizeof(two_io_ctl_pdata_t));
    }
    memcpy((u8 *)&__this->two_io, (u8 *)pdata, sizeof(two_io_pdata_t));
    /* two_io_ctl_info_dump(); */
    switch (__this->two_io.ctl_mode) {
    case 0://周期输出一次
        two_io_ctl_cycle_once();
        break;
    case 1://周期输出两次
        two_io_ctl_cycle_twice();
        break;
    case 2://周期呼吸
        two_io_ctl_cycle_breathe();
        break;
    case 3://常输出高
        two_io_ctl_always_output();
        break;
    default :
        free(__this);
        __this = NULL;
        break;
    }
}

