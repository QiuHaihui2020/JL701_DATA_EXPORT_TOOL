#ifndef LED_CONFIG_H
#define LED_CONFIG_H


//顺序要和填入pwm_led_table的顺序一致
#define LED_RED     0
#define LED_BLUE    1
// #define LED_WHITE   2


//***********************************

enum led_state_name : u8 {
    LED_STA_NONE = 0,                        // mode0: 无状态
    LED_STA_ALL_ON,                          // mode1: 所有灯全亮
    LED_STA_ALL_OFF,                         // mode2: 所有灯全灭

    // 非周期灯效
    LED_STA_RED_ON,                 // 红灯亮
    LED_STA_RED_ON_1S,              // 红灯亮1秒
    LED_STA_RED_ON_2S,              // 红灯亮2秒
    LED_STA_RED_ON_3S,              // 红灯亮3秒
    LED_STA_RED_FLASH_1TIMES,       // 红灯闪1次
    LED_STA_RED_FLASH_2TIMES,       // 红灯闪2次
    LED_STA_RED_FLASH_3TIMES,       // 红灯闪3次

    // 周期灯效
    LED_STA_RED_SLOW_FLASH,         // 红灯慢闪
    LED_STA_RED_FAST_FLASH,         // 红灯快闪
    LED_STA_RED_FLASH_1TIMES_PER_1S,// 红灯每5秒闪1次
    LED_STA_RED_FLASH_1TIMES_PER_2S,// 红灯每5秒闪1次
    LED_STA_RED_FLASH_1TIMES_PER_5S,// 红灯每5秒闪1次
    LED_STA_RED_FLASH_2TIMES_PER_5S,// 红灯每5秒闪2次
    LED_STA_RED_BREATHE,            // 红灯呼吸

    // 非周期灯效
    LED_STA_BLUE_ON,                 // 蓝灯亮
    LED_STA_BLUE_ON_1S,              // 蓝灯亮1秒
    LED_STA_BLUE_ON_2S,              // 蓝灯亮2秒
    LED_STA_BLUE_ON_3S,              // 蓝灯亮3秒
    LED_STA_BLUE_FLASH_1TIMES,       // 蓝灯闪1次
    LED_STA_BLUE_FLASH_2TIMES,       // 蓝灯闪2次
    LED_STA_BLUE_FLASH_3TIMES,       // 蓝灯闪3次


    // 周期灯效
    LED_STA_BLUE_SLOW_FLASH,         // 蓝灯慢闪
    LED_STA_BLUE_FAST_FLASH,         // 蓝灯快闪
    LED_STA_BLUE_FLASH_1TIMES_PER_1S,// 蓝灯每1秒闪1次
    LED_STA_BLUE_FLASH_1TIMES_PER_2S,// 蓝灯每2秒闪1次
    LED_STA_BLUE_FLASH_1TIMES_PER_5S,// 蓝灯每5秒闪1次
    LED_STA_BLUE_FLASH_2TIMES_PER_5S,// 蓝灯每5秒闪2次
    LED_STA_BLUE_BREATHE,            // 蓝灯呼吸

    LED_STA_RED_BLUE_SLOW_FLASH_ALTERNATELY, // 红蓝交替慢闪
    LED_STA_RED_BLUE_FAST_FLASH_ALTERNATELY, // 红蓝交替快闪
    LED_STA_RED_BLUE_BREATHE_ALTERNATELY,    // 红蓝交替呼吸


    //软件控制灯效
    LED_STA_RED_BLUE_5S_FLASHS_3_TIMES,
    LED_STA_BLUE_BREATH_AND_RED_FLASH,
    LED_STA_BLUE_1S_FLASHS_3_TIMES,

    LED_STA_POWERON,
    LED_STA_POWEROFF,
    LED_STA_POWERON_LOWPOWER,
    LED_STA_BT_INIT_OK,
    LED_STA_BT_CONN,
    LED_STA_BT_SLAVE_CONN_MASTER,
    LED_STA_BT_MASTER_CONN_ONE,
    LED_STA_BT_DISCONN,
    LED_STA_BT_TWS_CONN,
    LED_STA_BT_TWS_DISCONN,
    LED_STA_PHONE_INCOME,
    LED_STA_PHONE_OUT,
    LED_STA_PHONE_ACTIV,

    //充电态ui中间不能插入其他状态
    LED_STA_CHARGE_START,
    LED_STA_CHARGE_FULL,
    LED_STA_CHARGE_CLOSE,
    LED_STA_CHARGE_ERR,
    LED_STA_LOWPOWER,
    LED_STA_CHARGE_LDO5V_OFF,
    LED_STA_EXIT_LOWPOWER,
    LED_STA_NORMAL_POWER,
    //充电态ui中间不能插入其他状态
    LED_STA_POWER_NULL,

    LED_STA_MUSIC_MODE,
    LED_STA_MUSIC_PLAY,
    LED_STA_MUSIC_PAUSE,
    LED_STA_LINEIN_MODE,
    LED_STA_LINEIN_PLAY,
    LED_STA_LINEIN_PAUSE,
    LED_STA_PC_MODE,
    LED_STA_PC_PLAY,
    LED_STA_PC_PAUSE,
    LED_STA_FM_MODE,
    LED_STA_RECORD_MODE,
    LED_STA_SPDIF_MODE,
    LED_STA_RTC_MODE,
};


#endif
