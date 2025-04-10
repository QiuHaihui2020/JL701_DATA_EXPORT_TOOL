#include "app_msg.h"
#include "app_main.h"
#include "bt_tws.h"
#include "asm/charge.h"
#include "battery_manager.h"
#include "led_config.h"
#include "pwm_led/led_ui_api.h"


#if (TCFG_PWMLED_ENABLE)

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_app_msg_pwmled.data.bss")
#pragma data_seg(".ui_app_msg_pwmled.data")
#pragma const_seg(".ui_app_msg_pwmled.text.const")
#pragma code_seg(".ui_app_msg_pwmled.text")
#endif

typedef struct _led_app_ui_var {
    u8 other_status;
    u8 power_status;
    u8 current_status;
} led_app_ui_var;

led_app_ui_var led_app_ui = {
    .other_status = LED_STA_NONE,
    .power_status = LED_STA_NONE,
    .current_status  = LED_STA_NONE,
};


static int led_ui_charge_status_deal(u8 *status)
{
    int ret = 0;

    if (*status == LED_STA_NONE) {
        //无优先级高ui，直接返回 -1
        ret = -1;
        return ret;
    }

    printf("pwm led charge status:%d\n", *status);
    switch (*status) {
    case LED_STA_CHARGE_START:
        led_ui_set_state(LED_STA_RED_ON, DISP_CLEAR_OTHERS);
        break;
    case LED_STA_CHARGE_FULL:
        led_ui_set_state(LED_STA_BLUE_ON, DISP_CLEAR_OTHERS);
        break;
    case LED_STA_CHARGE_CLOSE:
        led_ui_set_state(LED_STA_ALL_OFF, DISP_CLEAR_OTHERS);
        break;
    case LED_STA_CHARGE_ERR:
        led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, DISP_CLEAR_OTHERS);
        break;
    case LED_STA_LOWPOWER:
        led_ui_set_state(LED_STA_RED_SLOW_FLASH, 0);
        break;
    case LED_STA_EXIT_LOWPOWER:
    case LED_STA_CHARGE_LDO5V_OFF:
    case LED_STA_NORMAL_POWER:
        *status = LED_STA_NONE; //退出高优先态
        ret = -1;
        break;
    }

    return ret;
}

static int led_ui_normal_status_deal(u8 *status)
{
    printf("pwm led normal status:%d\n", *status);
    switch (*status) {
    case LED_STA_POWERON:
        led_ui_set_state(LED_STA_BLUE_ON, 0);
        break;
    case LED_STA_POWEROFF:
        led_ui_set_state(LED_STA_ALL_OFF, 0);
        break;
    case LED_STA_POWERON_LOWPOWER:
        led_ui_set_state(LED_STA_BLUE_ON, 0);
        break;
    case LED_STA_BT_INIT_OK:
        led_ui_set_state(LED_STA_RED_BLUE_SLOW_FLASH_ALTERNATELY, 0);
        break;
    case LED_STA_BT_CONN:
        led_ui_set_state(LED_STA_BLUE_FLASH_1TIMES_PER_5S, 0);
        break;
    case LED_STA_BT_DISCONN:
        led_ui_set_state(LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, 0);
        break;
    case LED_STA_MUSIC_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_MUSIC_PLAY:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_MUSIC_PAUSE:
        led_ui_set_state(LED_STA_BLUE_ON, 0);
        break;
    case LED_STA_LINEIN_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_LINEIN_PLAY:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_LINEIN_PAUSE:
        led_ui_set_state(LED_STA_BLUE_ON, 0);
        break;
    case LED_STA_PC_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_PC_PLAY:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_PC_PAUSE:
        led_ui_set_state(LED_STA_BLUE_ON, 0);
        break;
    case LED_STA_FM_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_RECORD_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_SPDIF_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    case LED_STA_RTC_MODE:
        led_ui_set_state(LED_STA_BLUE_SLOW_FLASH, 0);
        break;
    }
    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief   pwm led app层ui模式设置函数
  @param   msg:充电转发过来的msg
  @return  0
  @note	集中处理ui信息
  */
/*----------------------------------------------------------------------------*/
void led_app_ui_set(u8 led_status)
{
    //记录各自状态
    if (led_status >= LED_STA_CHARGE_START && led_status <= LED_STA_NORMAL_POWER) {
        led_app_ui.power_status = led_status;
    } else {
        led_app_ui.other_status = led_status;
    }

    //充电态优先处理，无充电时才处理其他ui
    if (led_ui_charge_status_deal(&led_app_ui.power_status) == (-1)) {
        led_ui_normal_status_deal(&led_app_ui.other_status);
        led_app_ui.current_status = led_app_ui.other_status;
    } else {
        led_app_ui.current_status = led_app_ui.power_status;
    }
}

//*----------------------------------------------------------------------------*/
/**@brief   pwm led ui处理
  @param   msg:充电转发过来的msg
  @return  0
  @note	集中处理ui信息
  */
/*----------------------------------------------------------------------------*/
static int ui_led_charge_msg_entry(int *msg)
{
    int ret = false;
    u8 otg_status = 0;

    /* printf("pwm led charge msg:%d\n", msg[0]); */
    switch (msg[0]) {
    case CHARGE_EVENT_CHARGE_START:
        led_app_ui_set(LED_STA_CHARGE_START);
        break;
    case CHARGE_EVENT_CHARGE_FULL:
        led_app_ui_set(LED_STA_CHARGE_FULL);
        break;
    case CHARGE_EVENT_LDO5V_OFF:
        led_app_ui_set(LED_STA_CHARGE_LDO5V_OFF);
        break;
    case POWER_EVENT_POWER_NORMAL:
        led_app_ui_set(LED_STA_NORMAL_POWER);
        break;
    case POWER_EVENT_POWER_WARNING:
        led_app_ui_set(LED_STA_LOWPOWER);
        break;
    default:
        break;
    }

    return ret;
}

//*----------------------------------------------------------------------------*/
/**@brief   pwm led ui处理
  @param   msg:蓝牙转发过来的msg
  @return  0
  @note	集中处理ui信息
  */
/*----------------------------------------------------------------------------*/
static int ui_led_bt_stack_msg_entry(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }

    /* printf("pwm led bt stack:%d\n", bt->event); */
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        led_app_ui_set(LED_STA_BT_INIT_OK);
        break;
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        led_app_ui_set(LED_STA_BT_CONN);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        led_app_ui_set(LED_STA_BT_DISCONN);
        break;
    }
    return 0;
}

//*----------------------------------------------------------------------------*/
/**@brief   pwm led ui处理
  @param   msg:主线程转发过来的msg
  @return  0
  @note	集中处理ui信息
  */
/*----------------------------------------------------------------------------*/
static void led_enter_mode(u8 mode)
{
    /* printf("pwm led enter mode:%d\n", mode); */
    switch (mode) {
    case APP_MODE_IDLE:
        led_app_ui_set(LED_STA_POWEROFF);
        break;
    case APP_MODE_POWERON:
        led_app_ui_set(LED_STA_POWERON);
        break;
    case APP_MODE_BT:
        break;
    case APP_MODE_MUSIC:
        led_app_ui_set(LED_STA_MUSIC_MODE);
        break;
    case APP_MODE_FM:
        led_app_ui_set(LED_STA_FM_MODE);
        break;
    case APP_MODE_RECORD:
        led_app_ui_set(LED_STA_RECORD_MODE);
        break;
    case APP_MODE_LINEIN:
        led_app_ui_set(LED_STA_LINEIN_MODE);
        break;
    case APP_MODE_RTC:
        led_app_ui_set(LED_STA_RTC_MODE);
        break;
    case APP_MODE_PC:
        led_app_ui_set(LED_STA_PC_MODE);
        break;
    case APP_MODE_SPDIF:
        led_app_ui_set(LED_STA_SPDIF_MODE);
        break;
    }
}

static int ui_led_app_msg_entry(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_ENTER_MODE:
        led_enter_mode(msg[1] & 0xff);
        break;
    case APP_MSG_MUSIC_PLAY_STATUS:
        if (FILE_PLAYER_START == msg[1]) {
            led_app_ui_set(LED_STA_MUSIC_PLAY);
        } else if (FILE_PLAYER_PAUSE == msg[1]) {
            led_app_ui_set(LED_STA_MUSIC_PAUSE);
        }
        break;
    case APP_MSG_LINEIN_PLAY_STATUS:
        if (msg[1]) {
            led_app_ui_set(LED_STA_LINEIN_PLAY);
        } else {
            led_app_ui_set(LED_STA_LINEIN_PAUSE);
        }
        break;
    default:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(led_charge_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = ui_led_charge_msg_entry,
};

APP_MSG_HANDLER(led_bt_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = ui_led_bt_stack_msg_entry,
};

APP_MSG_HANDLER(led_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = ui_led_app_msg_entry,
};

#endif

