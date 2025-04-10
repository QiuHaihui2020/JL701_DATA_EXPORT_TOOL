#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "system/init.h"
#include "system/spinlock.h"
#include "pwm_led/led_ui_tws_sync.h"
#include "pwm_led/led_ui_api.h"
#include "app_msg.h"
#include "app_config.h"
#include "asm/power_interface.h"
#include "bt_tws.h"


/* #ifdef PWM_LED_DEBUG_ENABLE */
#if 0 //LED UI Test Code
/* #if (PWM_LED_TEST_MODE != 0) */

struct led_mode led_mode_table[] = {
//
#if 1
    /* {LED_STA_NONE, "LED_STA_NONE"}, */

    {LED_STA_RED_BLUE_SLOW_FLASH_ALTERNATELY, "LED_STA_RED_BLUE_SLOW_FLASH_ALTERNATELY"},
    {LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, "LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY"},
    {LED_STA_RED_BLUE_BREATHE_ALTERNATELY, "LED_STA_RED_BLUE_BREATHE_ALTERNATELY"},

    {LED_STA_ALL_ON, "LED_STA_ALL_ON"},
    {LED_STA_ALL_OFF, "LED_STA_ALL_OFF"},
    {LED_STA_BLUE_OFF_RED_ON, "LED_STA_BLUE_OFF_RED_ON"},
    {LED_STA_BLUE_OFF_RED_ON_1S, "LED_STA_BLUE_OFF_RED_ON_1S"},
    {LED_STA_BLUE_OFF_RED_ON_2S, "LED_STA_BLUE_OFF_RED_ON_2S"},
    {LED_STA_BLUE_OFF_RED_ON_3S, "LED_STA_BLUE_OFF_RED_ON_3S"},
    {LED_STA_BLUE_OFF_RED_FLASH_1TIMES, "LED_STA_BLUE_OFF_RED_FLASH_1TIMES"},
    {LED_STA_BLUE_OFF_RED_FLASH_2TIMES, "LED_STA_BLUE_OFF_RED_FLASH_2TIMES"},
    {LED_STA_BLUE_OFF_RED_FLASH_3TIMES, "LED_STA_BLUE_OFF_RED_FLASH_3TIMES"},

    {LED_STA_BLUE_OFF_RED_SLOW_FLASH, "LED_STA_BLUE_OFF_RED_SLOW_FLASH"},
    {LED_STA_BLUE_OFF_RED_FAST_FLASH, "LED_STA_BLUE_OFF_RED_FAST_FLASH"},
    {LED_STA_BLUE_OFF_RED_BREATHE, "LED_STA_BLUE_OFF_RED_BREATHE"},
    {LED_STA_BLUE_OFF_RED_FLASH_1TIMES_PER_5S, "LED_STA_BLUE_OFF_RED_FLASH_1TIMES_PER_5S"},
    {LED_STA_BLUE_OFF_RED_FLASH_2TIMES_PER_5S, "LED_STA_BLUE_OFF_RED_FLASH_2TIMES_PER_5S"},

    {LED_STA_RED_OFF_BLUE_ON, "LED_STA_RED_OFF_BLUE_ON"},
    {LED_STA_RED_OFF_BLUE_ON_1S, "LED_STA_RED_OFF_BLUE_ON_1S"},
    {LED_STA_RED_OFF_BLUE_ON_2S, "LED_STA_RED_OFF_BLUE_ON_2S"},
    {LED_STA_RED_OFF_BLUE_ON_3S, "LED_STA_RED_OFF_BLUE_ON_3S"},
    {LED_STA_RED_OFF_BLUE_FLASH_1TIMES, "LED_STA_RED_OFF_BLUE_FLASH_1TIMES"},
    {LED_STA_RED_OFF_BLUE_FLASH_2TIMES, "LED_STA_RED_OFF_BLUE_FLASH_2TIMES"},
    {LED_STA_RED_OFF_BLUE_FLASH_3TIMES, "LED_STA_RED_OFF_BLUE_FLASH_3TIMES"},

    {LED_STA_RED_OFF_BLUE_SLOW_FLASH, "LED_STA_RED_OFF_BLUE_SLOW_FLASH"},
    {LED_STA_RED_OFF_BLUE_FAST_FLASH, "LED_STA_RED_OFF_BLUE_FAST_FLASH"},
    {LED_STA_RED_OFF_BLUE_BREATHE, "LED_STA_RED_OFF_BLUE_BREATHE"},
    {LED_STA_RED_OFF_BLUE_FLASH_1TIMES_PER_5S, "LED_STA_RED_OFF_BLUE_FLASH_1TIMES_PER_5S"},
    {LED_STA_RED_OFF_BLUE_FLASH_2TIMES_PER_5S, "LED_STA_RED_OFF_BLUE_FLASH_2TIMES_PER_5S"},

#endif //#if 0
    //

    {LED_STA_RED_BLUE_5S_FLASHS_3_TIMES, "LED_STA_RED_BLUE_5S_FLASHS_3_TIMES"},
    {LED_STA_BLUE_BREATH_1_5_AND_ON_3S, "LED_STA_BLUE_BREATH_1_5_AND_ON_3S"}
};

//=================================================================//
//                      LED UI Test Code                           //
//=================================================================//

#include "system/timer.h"

void led_ui_manager_mode_test(void *priv);

#include "clock.h"
void led_ui_test_index_sum();
char led_ui_test_index = 0;
char led_ui_test_index_next2 = 0;

void led_ui_manager_INTR_test(void *priv)
{
    line_inf
    led_ui_set_state(LED_STA_BLUE_OFF_RED_FLASH_3TIMES, DISP_NON_INTR | DISP_INTR_CURRENT | DISP_RECOVER_CURRENT, 0);
}


void led_ui_manager_test_code_timeout(void *priv)
{
#if (PWM_LED_TEST_MODE != 0)
    PWMLED_LOG_INFO("led_ui_manager_test_code>>>");

#if 0
    /* #ifdef PWM_LED_IO_TEST */
    //设置为输出
    LED_TIMER_TEST_PORT->DIR &= ~BIT(LED_TIMER_TEST_PIN);
    //低
    LED_TIMER_TEST_PORT->OUT &= ~BIT(LED_TIMER_TEST_PIN);
#endif

#define PWM_LED_DELAY_POWERDOWN 0

    int tws_role = tws_api_get_role_async();

    if (tws_role == TWS_ROLE_MASTER) {
#if (PWM_LED_TEST_MODE == 3)

        sys_timer_add(NULL, led_tws_attr_test, 8000);
#elif (PWM_LED_TEST_MODE == 4)
        /* sys_timer_add(NULL, led_ui_manager_mode_test, 10000); */
        sys_timer_add(NULL, led_tws_attr_test, 8000);
        //测试打断灯效
        /* sys_timeout_add(NULL, led_ui_manager_INTR_test, 9000); */
#else
        //30s的不进入低功耗时间+20s的进入低功耗时间
        sys_timer_add(NULL, led_ui_manager_mode_test, 20000);
#endif
        /* sys_hi_timer_add(NULL, led_ui_manager_mode_test, 40000); */

        /* sys_timer_add(NULL, led_ui_manager_mode_by_name_test, 20000); */

#endif //#if 0

    }
}


int led_ui_manager_test_code(void)
{
#if (PWM_LED_TEST_MODE == 0)
    return 0;
#endif

    //等待开机配对ok后再进行test
    //跳过开机灯效后的复杂灯效切换环境再进行测试
#if (PWM_LED_TEST_MODE == 4) || (PWM_LED_TEST_MODE == 3)
    sys_timeout_add(NULL, led_ui_manager_test_code_timeout, 10000);
#else
    led_ui_manager_test_code_timeout(NULL);
#endif
    return 0;
}

late_initcall(led_ui_manager_test_code);



u32 usr_led_timer_lock_id = 0;
void usr_led_timer_lock(void *priv)
{
    PWMLED_LOG_INFO("led timer done allow to enter powerdown");
    sys_hi_timer_del(usr_led_timer_lock_id);
    usr_led_timer_lock_id = 0;
    return ;
}


void led_ui_manager_mode_test(void *priv)
{

    PWMLED_LOG_INFO("LED UI Test: display %d: %s", led_ui_test_index, led_mode_table[led_ui_test_index].name);

#ifdef PWM_LED_IO_TEST_DEMO
    LED_TIMER_TEST_PORT->OUT ^= BIT(LED_TIMER_TEST_PIN);
#endif

#if (PWM_LED_TEST_MODE == 1)
    //穿插一个all_on 用于区分
    //
#if 1
    led_ui_set_state(LED_STA_ALL_ON, DISP_RECOVERABLE, 0);
    os_time_dly(200);
    led_ui_set_state(LED_STA_ALL_OFF, DISP_RECOVERABLE, 0);
    os_time_dly(200);
#endif //#if 0
    //

    led_ui_set_state(led_mode_table[led_ui_test_index].uuid, DISP_RECOVERABLE | DISP_TWS_SYNC, 0);
    led_ui_test_index_sum();

#else


    //*********************************************************************************
    PWMLED_LOG_DEBUG("bt_tws_remove_pairs>>>>>");

    int err = tws_api_remove_pairs();
    if (err) {
        bt_tws_remove_pairs();
    }

    os_time_dly(300);

    /* APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS,    // 执行配对和连接的默认流程 */
    PWMLED_LOG_INFO("APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS>>>>");
    app_send_message(APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS, 0);

    //*********************************************************************************

#endif //#if 0

//
#if PWM_LED_DELAY_POWERDOWN
    PWMLED_LOG_INFO("20s not in powerdown>>>");

#if (PWM_LED_TEST_MODE == 1)
    usr_led_timer_lock_id = sys_hi_timeout_add(NULL, usr_led_timer_lock, 10000);
#else
    //30s的不进入低功耗时间+20s的进入低功耗时间
    usr_led_timer_lock_id = sys_hi_timeout_add(NULL, usr_led_timer_lock, 30000);
#endif

#endif //#if 0
    //
    //

}

void led_ui_test_index_sum()
{
    led_ui_test_index++;
    led_ui_test_index_next2 = led_ui_test_index + 2;

    if (led_ui_test_index >= ARRAY_SIZE(led_mode_table)) {
        led_ui_test_index = 0;
    }

    if (led_ui_test_index_next2 >= ARRAY_SIZE(led_mode_table)) {
        led_ui_test_index_next2 = 0;
    }
}


void led_local_attr_test(void)
{

}

void led_tws_attr_test(void)
{

    //
#if 0
    /*  1.两个相同灯效切换是否还能够触发同步灯效？ */
    switch (led_ui_test_index) {

    }
    led_ui_set_state(led_mode_table[led_ui_test_index].uuid, DISP_RECOVERABLE | DISP_TWS_SYNC, 0);

    //软件灯效打断硬件灯效

    //硬件灯效打断软件灯效


    /* A周期灯效切换到B非周期灯效，如果不设置打断，则会触发等待B非周期灯效结束后才可以切换到周期灯效。 */
case 1:
    led_ui_set_state(led_mode_table[led_ui_test_index].uuid, DISP_RECOVERABLE | DISP_TWS_SYNC, 0);
    break;


    /* A非周期灯效或A周期灯效切换到B周期灯效，则直接立刻切换到A非周期灯效或A周期灯效。 */

    /* A非周期灯效切换到B非周期灯效，因为不设置打断，所以会触发等待非周期灯效结束后才可以切换到A非周期灯效 */

    /* A周期灯效切换到B非周期灯效，如果不设置打断，则会触发等待B非周期灯效结束后才可以切换到周期灯效。 */

    /* A非周期灯效或A周期灯效切换到B周期灯效，则直接立刻切换到A非周期灯效或A周期灯效。 */

    /* A非周期灯效切换到B非周期灯效，因为不设置打断，所以会触发等待非周期灯效结束后才可以切换到A非周期灯效 */
    break;
}

led_ui_test_index_sum();
#endif //#if 0
//
}

/* --------------------------------------------------------------------------*/
/**
 *
 * @return 0：不可进
 *         1：可进
 */
/* ----------------------------------------------------------------------------*/
static u8 led_test_idle_query(void)
{
    return (usr_led_timer_lock_id == 0);
}

REGISTER_LP_TARGET(led_test_target) = {
    .name = "led_test",
    .is_idle = led_test_idle_query,
};


void led_index_2_name_dump(enum led_state_name led_index, u8 loop)
{
    for (u8 led_ui_show_index = 0; led_ui_show_index < ARRAY_SIZE(led_mode_table); led_ui_show_index++) {
        if (led_mode_table[led_ui_show_index].uuid == led_index) {
            if (loop == 1) {
                printf("  L :display led_name %d: %s", led_index, led_mode_table[led_ui_show_index].name);

            } else {
                printf("display led_name %d: %s", led_index, led_mode_table[led_ui_show_index].name);

            }
        }
    }
}

#endif

