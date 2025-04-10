#ifndef LED_UI_API_H
#define LED_UI_API_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "led_config.h"
#include "led_api.h"
#include "pwm_led.h"
#include "pwm_led_debug.h"

#if TCFG_USER_TWS_ENABLE
#include "pwm_led/led_ui_tws_sync.h"
#endif

//***********************************

#define LED_BRIGHT_OFF  0
#define NON_BREATH_MODE 0

#define LED_STATE_TABLE(table)  ARRAY_SIZE(table), table


enum {
    LED_UI_STATUS_IDLE = 0,
    LED_UI_STATUS_ACTIVE,
};


enum {
    LED_MSG_STATE_END = 1,
};

//灯效属性
enum led_disp_mode : u8 {
    DISP_NON_INTR          = 0x01, // 当前设置的灯效不允许被打断
    DISP_CLEAR_OTHERS      = 0x02, // 清除其它灯效, 设置了DISP_NON_INTR标记的不会被清除
    DISP_RECOVERABLE       = 0x04, // 周期灯效和非周期灯效共存标记, 被非周期的灯效打断后不会被删除
    DISP_TWS_SYNC          = 0x08, // 需要tws同步的灯效。默认只允许tws主机设置,
    // 硬件会间隔发起tws通信保持同步，软件只会同步第一次。
    DISP_TWS_SYNC_RX       = 0x10, // tws主机发起的同步灯效，从机收到后自动添加此标记
    DISP_TWS_SYNC_TX       = 0x20, // tws主机发起的同步灯效，主机发送后自动添加此标记
};



enum {
    LED_ACTION_WAIT,
    LED_ACTION_CONTINUE,
    LED_ACTION_LOOP,
    LED_ACTION_END,
};

struct led_hw_config {
    u8 led_name;
    u8 gpio;
    u8 led_logic; //enum led_logic_table
};


#define LED_UI_FOLLOW_HW_TIME_MS 50


struct led_state_item {
    u8 led_name;
    u8 time_msec;//灯亮的时间，单位50ms
    u8 brightiness;
    u8 breath_time;//0为非呼吸灯效。有值为呼吸灯效，值代表的作用是亮度增到最大的时候，如果需要保持的时间，单位50ms，该时间要小于 time_msec
    u8 action;
};



//u8数据类型约支持75行action，满足需求
#define TIME_EFFECT_MODE 0xff

struct led_state_map {
    enum led_state_name name;
    u8 time_flag; //标记是否是硬件灯效，硬件灯效是类型最大值 TIME_EFFECT_MODE  软件灯效是实际结构体数组长度
    const void *arg1;
};

struct led_state_obj {
    struct list_head entry;
    const led_pdata_t *time;
    const struct led_state_item *table;
    enum led_disp_mode disp_mode;
    enum led_state_name name;
    u16 timer;
    u8 table_size;
    u8 action_index;
    u8 uuid;
};


struct led_state_obj *led_ui_get_first_state();

struct led_state_obj *led_ui_get_state(u8 uuid);


void led_ui_state_machine_run();

void led_ui_set_state(enum led_state_name state_name, enum led_disp_mode disp_mode);

enum led_state_name led_ui_get_state_name();

int led_ui_state_is_idle();

void pwm_led_hw_cbfunc(u32 cnt);

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭led_ui_manager
 */
/* --------------------------------------------------------------------------*/
void led_ui_manager_close(void);
void led_index_2_name_dump(enum led_state_name led_index, u8 loop);

extern const led_board_cfg_t board_cfg_hw_data;

#endif

