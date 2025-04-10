#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".led_config.data.bss")
#pragma data_seg(".led_config.data")
#pragma const_seg(".led_config.text.const")
#pragma code_seg(".led_config.text")
#endif
#include "sdk_config.h"
#include "app_config.h"
#include "gpio.h"
#include "system/init.h"
#include "pwm_led/led_ui_api.h"

#if (TCFG_PWMLED_ENABLE)

#define LED_RED_BRIGHTNESS      100
#define LED_BLUE_BRIGHTNESS     100


const led_board_cfg_t board_cfg_hw_data = {
    .layout = TCFG_LED_LAYOUT,
    .com_pole_port = -1,
    .led0.port = TCFG_LED_RED_GPIO,
    .led1.port = TCFG_LED_BLUE_GPIO,
    .led0.logic = TCFG_LED_RED_LOGIC,
    .led1.logic = TCFG_LED_BLUE_LOGIC,
    .led0.brightness = LED_RED_BRIGHTNESS,
    .led1.brightness = LED_BLUE_BRIGHTNESS,
};

// *INDENT-OFF*

//ALL ON
const struct led_platform_data  led_all_on_config = {
    .ctl_option = CTL_LED01_SYNC,
    .ctl_mode = ALWAYS_BRIGHT,
};

//ALL OFF
const struct led_platform_data  led_all_off_config = {
    .ctl_option = CTL_LED01_SYNC,
    .ctl_mode = ALWAYS_EXTINGUISH,
};


#if TCFG_LED_RED_ENABLE

const struct led_platform_data  led_red_on_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = ALWAYS_BRIGHT,
};

const struct led_platform_data  led_red_on_1s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 21,//*50 =1050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 20,//*50=1000
    .cbfunc = pwm_led_hw_cbfunc,
};

const struct led_platform_data  led_red_on_2s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 41,//*50 =2050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 40,//*50=2000
    .cbfunc = pwm_led_hw_cbfunc,
};

const struct led_platform_data  led_red_on_3s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 61,//*50 =3050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 60,//*50=3000
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED0 2s闪1次0.1s
const struct led_platform_data led_red_slow_flash_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 40,//*50 =2000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 0.5s闪1次0.1s
const struct led_platform_data led_red_fast_flash_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 10,//*50 =500
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 1s闪1次0.1s
const struct led_platform_data led_red_flash_1times_per_1s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 20,//*50 =5000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 5s闪1次0.1s
const struct led_platform_data led_red_flash_1times_per_5s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 100,//*50 =5000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 5s闪2次0.1s
const struct led_platform_data led_red_flash_2times_per_5s_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_TWICE_BRIGHT,
    .ctl_cycle = 100,//*50 =5000
    .ctl_cycle_num = 0,
    .twice_bright.first_bright_time = 2,//*50=100
    .twice_bright.bright_gap_time = 2,//*50=100
    .twice_bright.second_bright_time = 2,//*50=100
};

//LED0 0.35s闪1次0.3s
const struct led_platform_data led_red_flash_1times_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 7,//*50 =350
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED0 0.65s闪2次0.3s
const struct led_platform_data led_red_flash_2times_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 13,//*50 =650
    .ctl_cycle_num = 2,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED0 0.95s闪1次0.3s
const struct led_platform_data led_red_flash_3times_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 19,//*50 =950
    .ctl_cycle_num = 3,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED0 3s闪呼吸1次2s
const struct led_platform_data led_red_breathe_config = {
    .ctl_option = CTL_LED0_ONLY,
    .ctl_mode = CYCLE_BREATHE_BRIGHT,
    .ctl_cycle = 60,//*50 =3000
    .ctl_cycle_num = 0,
    .breathe_bright.bright_time = 40,//*50=2000
    .breathe_bright.brightest_keep_time = 20,//*50=1000
};

#endif

#if TCFG_LED_BLUE_ENABLE

const struct led_platform_data led_blue_on_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = ALWAYS_BRIGHT,
};

const struct led_platform_data led_blue_on_1s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 21,//*50 =1050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 20,//*50=1000
    .cbfunc = pwm_led_hw_cbfunc,
};

const struct led_platform_data led_blue_on_2s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 41,//*50 =2050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 40,//*50=2000
    .cbfunc = pwm_led_hw_cbfunc,
};

const struct led_platform_data led_blue_on_3s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 61,//*50 =3050
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 60,//*50=3000
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED1 2s闪1次0.1s
const struct led_platform_data led_blue_slow_flash_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 40,//*50 =2000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED1 0.5s闪1次0.1s
const struct led_platform_data led_blue_fast_flash_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 10,//*50 =500
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED1 1s闪1次0.1s
const struct led_platform_data led_blue_flash_1times_per_1s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 20,//*50 =5000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED1 5s闪1次0.1s
const struct led_platform_data led_blue_flash_1times_per_5s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 100,//*50 =5000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED1 5s闪2次0.1s
const struct led_platform_data led_blue_flash_2times_per_5s_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_TWICE_BRIGHT,
    .ctl_cycle = 100,//*50 =5000
    .ctl_cycle_num = 0,
    .twice_bright.first_bright_time = 2,//*50=100
    .twice_bright.bright_gap_time = 2,//*50=100
    .twice_bright.second_bright_time = 2,//*50=100
};

//LED1 0.35s闪1次0.3s
const struct led_platform_data led_blue_flash_1times_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 7,//*50 =350
    .ctl_cycle_num = 1,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED1 0.65s闪2次0.3s
const struct led_platform_data led_blue_flash_2times_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 13,//*50 =650
    .ctl_cycle_num = 2,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED1 0.95s闪1次0.3s
const struct led_platform_data led_blue_flash_3times_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 19,//*50 =950
    .ctl_cycle_num = 3,
    .once_bright.bright_time = 6,//*50=300
    .cbfunc = pwm_led_hw_cbfunc,
};

//LED1 3s闪呼吸1次2s
const struct led_platform_data led_blue_breathe_config = {
    .ctl_option = CTL_LED1_ONLY,
    .ctl_mode = CYCLE_BREATHE_BRIGHT,
    .ctl_cycle = 60,//*50 =3000
    .ctl_cycle_num = 0,
    .breathe_bright.bright_time = 40,//*50=2000
    .breathe_bright.brightest_keep_time = 20,//*50=1000
};

#endif

//LED0 LED1  2s交替闪1次0.1s
const struct led_platform_data led_red_blue_slow_flash_alternately_config = {
    .ctl_option = CTL_LED01_ASYNC,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 40,//*50 =2000
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 LED1  0.5s交替闪1次0.1s
const struct led_platform_data led_red_blue_fast_flash_alternately_config = {
    .ctl_option = CTL_LED01_ASYNC,
    .ctl_mode = CYCLE_ONCE_BRIGHT,
    .ctl_cycle = 10,//*50 =500
    .ctl_cycle_num = 0,
    .once_bright.bright_time = 2,//*50=100
};

//LED0 LED1  5s交替呼吸1次3s
const struct led_platform_data led_red_blue_breathe_alternately_config = {
    .ctl_option = CTL_LED01_ASYNC,
    .ctl_mode = CYCLE_BREATHE_BRIGHT,
    .ctl_cycle = 100,//*50 =5000
    .ctl_cycle_num = 0,
    .breathe_bright.bright_time = 40,//*50=3000
    .breathe_bright.brightest_keep_time = 20,//*50=1000
};


/*
 * 最后一个表项要是LED_ACTION_END 或 LED_ACTION_LOOP
 *
 */
const struct led_state_item blue_led_1s_flashs_3_times[] = {
    // led_name  time_msec(50ms)  brightiness         mode            action
#if TCFG_LED_RED_ENABLE
    { LED_RED,   0,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_CONTINUE },
#endif
    { LED_BLUE,  2,               LED_BLUE_BRIGHTNESS,NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BLUE_BRIGHTNESS,NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BLUE_BRIGHTNESS,NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  10,              LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_LOOP     },
};

#if TCFG_LED_RED_ENABLE && TCFG_LED_BLUE_ENABLE
// 红蓝灯不同时间互闪3下
const struct led_state_item red_blue_led_5s_flashs_3_times[] = {
    // led_name  time_msec(50ms)  brightiness         mode            action
    { LED_RED,   0,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_CONTINUE },
    { LED_BLUE,  0,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_CONTINUE },

    { LED_RED,   2,               LED_RED_BRIGHTNESS, NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_RED,   2,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BLUE_BRIGHTNESS,NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  2,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_RED,   6,               LED_RED_BRIGHTNESS, NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_RED,   6,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  6,               LED_BLUE_BRIGHTNESS,NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  6,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_WAIT     },
    { LED_BLUE,  68,              LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_LOOP     },
};
#endif

// 蓝灯呼吸3s后红灯闪2下,1s后循环
const struct led_state_item blue_led_breathing_and_red_flash[] = {
#if TCFG_LED_RED_ENABLE
    { LED_RED,   0,               LED_BRIGHT_OFF,     NON_BREATH_MODE, LED_ACTION_CONTINUE },
#endif
    { LED_BLUE,  60,              LED_BLUE_BRIGHTNESS,  60, LED_ACTION_WAIT     },
    { LED_RED,   2,               LED_BLUE_BRIGHTNESS,  NON_BREATH_MODE, LED_ACTION_WAIT },
    { LED_RED,   2,               LED_BRIGHT_OFF,       NON_BREATH_MODE, LED_ACTION_WAIT },
    { LED_RED,   2,               LED_BLUE_BRIGHTNESS,  NON_BREATH_MODE, LED_ACTION_WAIT },
    { LED_RED,   2,               LED_BRIGHT_OFF,       NON_BREATH_MODE, LED_ACTION_WAIT },
    { LED_RED,   20,              LED_BRIGHT_OFF,       NON_BREATH_MODE, LED_ACTION_LOOP },
};


// 实际没有用到的灯效可以注释掉以节省代码空间
const struct led_state_map g_led_state_table[] = {
    { LED_STA_ALL_ON,                            TIME_EFFECT_MODE, &led_all_on_config },
    { LED_STA_ALL_OFF,                           TIME_EFFECT_MODE, &led_all_off_config },

#if TCFG_LED_RED_ENABLE
    { LED_STA_RED_ON,                   TIME_EFFECT_MODE, &led_red_on_config },
    { LED_STA_RED_ON_1S,                TIME_EFFECT_MODE, &led_red_on_1s_config },
    { LED_STA_RED_ON_2S,                TIME_EFFECT_MODE, &led_red_on_2s_config },
    { LED_STA_RED_ON_3S,                TIME_EFFECT_MODE, &led_red_on_3s_config },
    { LED_STA_RED_FLASH_1TIMES,         TIME_EFFECT_MODE, &led_red_flash_1times_config },
    { LED_STA_RED_FLASH_2TIMES,         TIME_EFFECT_MODE, &led_red_flash_2times_config },
    { LED_STA_RED_FLASH_3TIMES,         TIME_EFFECT_MODE, &led_red_flash_3times_config },

    { LED_STA_RED_SLOW_FLASH,           TIME_EFFECT_MODE, &led_red_slow_flash_config },
    { LED_STA_RED_FAST_FLASH,           TIME_EFFECT_MODE, &led_red_fast_flash_config },
    { LED_STA_RED_FLASH_1TIMES_PER_1S,  TIME_EFFECT_MODE, &led_red_flash_1times_per_1s_config },
    { LED_STA_RED_FLASH_1TIMES_PER_5S,  TIME_EFFECT_MODE, &led_red_flash_1times_per_5s_config },
    //{ LED_STA_RED_FLASH_2TIMES_PER_5S,  TIME_EFFECT_MODE, &led_red_flash_2times_per_5s_config },
    { LED_STA_RED_BREATHE,              TIME_EFFECT_MODE, &led_red_breathe_config },
#endif

#if TCFG_LED_BLUE_ENABLE
    { LED_STA_BLUE_ON,                   TIME_EFFECT_MODE, &led_blue_on_config },
    { LED_STA_BLUE_ON_1S,                TIME_EFFECT_MODE, &led_blue_on_1s_config },
    { LED_STA_BLUE_ON_2S,                TIME_EFFECT_MODE, &led_blue_on_2s_config },
    { LED_STA_BLUE_ON_3S,                TIME_EFFECT_MODE, &led_blue_on_3s_config },
    { LED_STA_BLUE_FLASH_1TIMES,         TIME_EFFECT_MODE, &led_blue_flash_1times_config },
    { LED_STA_BLUE_FLASH_2TIMES,         TIME_EFFECT_MODE, &led_blue_flash_2times_config },
    { LED_STA_BLUE_FLASH_3TIMES,         TIME_EFFECT_MODE, &led_blue_flash_3times_config },

    { LED_STA_BLUE_SLOW_FLASH,           TIME_EFFECT_MODE, &led_blue_slow_flash_config },
    { LED_STA_BLUE_FAST_FLASH,           TIME_EFFECT_MODE, &led_blue_fast_flash_config },
    { LED_STA_BLUE_FLASH_1TIMES_PER_1S,  TIME_EFFECT_MODE, &led_blue_flash_1times_per_1s_config },
    { LED_STA_BLUE_FLASH_1TIMES_PER_5S,  TIME_EFFECT_MODE, &led_blue_flash_1times_per_5s_config },
    //{ LED_STA_BLUE_FLASH_2TIMES_PER_5S,  TIME_EFFECT_MODE, &led_blue_flash_2times_per_5s_config },
    { LED_STA_BLUE_BREATHE,              TIME_EFFECT_MODE, &led_blue_breathe_config },


    { LED_STA_POWERON,                  TIME_EFFECT_MODE, &led_blue_on_1s_config },
#endif

    //{ LED_STA_RED_BLUE_SLOW_FLASH_ALTERNATELY,   TIME_EFFECT_MODE, &led_red_blue_slow_flash_alternately_config },
    //{ LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY,   TIME_EFFECT_MODE, &led_red_blue_fast_flash_alternately_config },
    //{ LED_STA_RED_BLUE_BREATHE_ALTERNATELY,      TIME_EFFECT_MODE, &led_red_blue_breathe_alternately_config },

    // 软件组合灯效
    //{ LED_STA_RED_BLUE_5S_FLASHS_3_TIMES,       LED_STATE_TABLE(red_blue_led_5s_flashs_3_times) },
    //{ LED_STA_BLUE_BREATH_AND_RED_FLASH,        LED_STATE_TABLE(blue_led_breathing_and_red_flash) },
    { LED_STA_BLUE_1S_FLASHS_3_TIMES,           LED_STATE_TABLE(blue_led_1s_flashs_3_times) },

    { 0, 0, 0 } // END must end
};



#endif /* #if TCFG_PWMLED_ENABLE */
