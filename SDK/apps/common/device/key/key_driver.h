#ifndef SYS_KEY_DRIVER_H
#define SYS_KEY_DRIVER_H

#include "typedef.h"

#define NO_KEY 		     0xff
#define KEY_NOT_SUPPORT  0x01


#define     KEY_POWER           0
#define     KEY_PREV            1
#define     KEY_NEXT            2
#define     KEY_SLIDER          3
#define     KEY_MODE            4
#define     KEY_PLAY            5

#define     KEY_AD_NUM0           10
#define     KEY_AD_NUM1           11
#define     KEY_AD_NUM2           12
#define     KEY_AD_NUM3           13
#define     KEY_AD_NUM4           14
#define     KEY_AD_NUM5           15
#define     KEY_AD_NUM6           16
#define     KEY_AD_NUM7           17
#define     KEY_AD_NUM8           18
#define     KEY_AD_NUM9           19
#define     KEY_AD_NUM10          20
#define     KEY_AD_NUM11          21
#define     KEY_AD_NUM12          22
#define     KEY_AD_NUM13          23
#define     KEY_AD_NUM14          24
#define     KEY_AD_NUM15          25
#define     KEY_AD_NUM16          26
#define     KEY_AD_NUM17          27
#define     KEY_AD_NUM18          28
#define     KEY_AD_NUM19          29

#define     KEY_IR_NUM0           30
#define     KEY_IR_NUM1           31
#define     KEY_IR_NUM2           32
#define     KEY_IR_NUM3           33
#define     KEY_IR_NUM4           34
#define     KEY_IR_NUM5           35
#define     KEY_IR_NUM6           36
#define     KEY_IR_NUM7           37
#define     KEY_IR_NUM8           38
#define     KEY_IR_NUM9           39
#define     KEY_IR_NUM10          40
#define     KEY_IR_NUM11          41
#define     KEY_IR_NUM12          42
#define     KEY_IR_NUM13          43
#define     KEY_IR_NUM14          44
#define     KEY_IR_NUM15          45
#define     KEY_IR_NUM16          46
#define     KEY_IR_NUM17          47
#define     KEY_IR_NUM18          48
#define     KEY_IR_NUM19          49
#define     KEY_IR_NUM20          50
#define     KEY_IR_NUM21          51
#define     KEY_IR_NUM22          52
#define     KEY_IR_NUM23          53
#define     KEY_IR_NUM24          54
#define     KEY_IR_NUM25          55
#define     KEY_IR_NUM26          56
#define     KEY_IR_NUM27          57
#define     KEY_IR_NUM28          58
#define     KEY_IR_NUM29          59
#define     KEY_IR_NUM30          60
#define     KEY_IR_NUM31          61
#define     KEY_IR_NUM32          62
#define     KEY_IR_NUM33          63
#define     KEY_IR_NUM34          64

#define     KEY_IO_NUM0           65
#define     KEY_IO_NUM1           66
#define     KEY_IO_NUM2           67
#define     KEY_IO_NUM3           68
#define     KEY_IO_NUM4           69
#define     KEY_IO_NUM5           70
#define     KEY_IO_NUM6           71
#define     KEY_IO_NUM7           72
#define     KEY_IO_NUM8           73
#define     KEY_IO_NUM9           74
#define     KEY_IO_NUM10          75
#define     KEY_IO_NUM11          76
#define     KEY_IO_NUM12          77
#define     KEY_IO_NUM13          78
#define     KEY_IO_NUM14          79

typedef enum __KEY_DRIVER_TYPE {
    KEY_DRIVER_TYPE_IO = 0x0,
    KEY_DRIVER_TYPE_AD,
    KEY_DRIVER_TYPE_RTCVDD_AD,
    KEY_DRIVER_TYPE_IR,
    KEY_DRIVER_TYPE_TOUCH,
    KEY_DRIVER_TYPE_CTMU_TOUCH,
    KEY_DRIVER_TYPE_RDEC,
    KEY_DRIVER_TYPE_SLIDEKEY,
    KEY_DRIVER_TYPE_SOFTKEY,
    KEY_DRIVER_TYPE_BRIGHTNESS,
    KEY_DRIVER_TYPE_VOICE,

    KEY_DRIVER_TYPE_MAX,
} KEY_DRIVER_TYPE;


#ifdef CONFIG_EARPHONE_CASE_ENABLE
enum key_action {
    KEY_ACTION_CLICK,
    KEY_ACTION_LONG,
    KEY_ACTION_HOLD,
    KEY_ACTION_UP,
    KEY_ACTION_DOUBLE_CLICK,
    KEY_ACTION_TRIPLE_CLICK,
    KEY_ACTION_FOURTH_CLICK,
    KEY_ACTION_FIRTH_CLICK,
    KEY_ACTION_SEXTUPLE_CLICK,
    KEY_ACTION_SEPTUPLE_CLICK,
    KEY_ACTION_HOLD_1SEC,           //10
    KEY_ACTION_HOLD_3SEC,
    KEY_ACTION_HOLD_5SEC,
    KEY_ACTION_HOLD_8SEC,
    KEY_ACTION_HOLD_10SEC,

    /* TWS两边同时按下消息 */
    KEY_ACTION_TWS_CLICK,
    KEY_ACTION_TWS_DOUBLE_CLICK,
    KEY_ACTION_TWS_TRIPLE_CLICK,
    KEY_ACTION_TWS_FOURTH_CLICK,
    KEY_ACTION_TWS_FIRTH_CLICK,
    KEY_ACTION_TWS_SEXTUPLE_CLICK,
    KEY_ACTION_TWS_SEPTUPLE_CLICK,
    KEY_ACTION_TWS_HOLD_1SEC,
    KEY_ACTION_TWS_HOLD_3SEC,
    KEY_ACTION_TWS_HOLD_5SEC,
    KEY_ACTION_TWS_HOLD_8SEC,
    KEY_ACTION_TWS_HOLD_10SEC,

    /*=======新增按键动作请在此处之上增加=======*/
    KEY_ACTION_NO_KEY,
    KEY_ACTION_MAX,
};

#else

enum key_action {
    KEY_ACTION_CLICK,
    KEY_ACTION_LONG,
    KEY_ACTION_HOLD,
    KEY_ACTION_UP,
    KEY_ACTION_DOUBLE_CLICK,
    KEY_ACTION_TRIPLE_CLICK,
    KEY_ACTION_FOURTH_CLICK,
    KEY_ACTION_FIRTH_CLICK,
    KEY_ACTION_HOLD_3SEC,
    KEY_ACTION_HOLD_5SEC,
    /*=======新增按键动作请在此处之上增加=======*/
    KEY_ACTION_NO_KEY,
    KEY_ACTION_MAX,
};

#endif

enum slider_key_action {
    KEY_SLIDER_UP,
    KEY_SLIDER_DOWN,
    KEY_SLIDER_LEFT,
    KEY_SLIDER_RIGHT,
};

enum key_event_type {
    KEY_CLICK_EVENT,
    KEY_COMB_EVENT,
};

struct key_event {
    u8 init;
    u8 type;
    u8 event;
    u8 value;
    u32 tmr;
};

struct key_driver_para {
    u8 last_key;  			//上一次get_value按键值
//== 用于消抖类参数
    u8 filter_value; 		//用于按键消抖
    u8 filter_cnt;  		//用于按键消抖时的累加值
//== 用于判定连击事件参数
    u8 click_delay_cnt;  	//按键被抬起后等待连击事件延时计数
    u8 press_cnt;  		 	//与long_time和hold_time对比, 判断long_event和hold_event
    u16 press_sum_cnt;
    const u8 key_type;
    const u8 filter_time;	//当filter_cnt累加到base_cnt值时, 消抖有效
    const u8 long_time;  	//按键判定长按数量
    const u8 hold_time;  	//按键判定HOLD数量
    const u8 click_delay_time;	//按键被抬起后等待连击事件延时数量
    const u32 scan_time;	//按键扫描频率, 单位ms
};

struct key_driver_ops {
    const u8 idle_query_en;
    const struct key_driver_para *param;
    u8(*get_value)(void);
    int (*key_init)(void);
};

#define REGISTER_KEY_OPS(key_ops) \
        const struct key_driver_ops key_ops sec(.key_operation)

extern const struct key_driver_ops key_ops_begin[];
extern const struct key_driver_ops key_ops_end[];

#define list_for_each_key(p) \
    for (p = key_ops_begin; p < key_ops_end; p++)

struct key_callback {
    char *name;
    void *arg;
    int(*cb_deal)(void *arg);
};

#define REGISTER_KEY_DET_CALLBACK(cb) \
        const struct key_callback cb sec(.key_cb)

extern const struct key_callback key_callback_begin[];
extern const struct key_callback key_callback_end[];

#define list_for_each_key_callback(p) \
    for (p = key_callback_begin; p < key_callback_end; p++)

//组合按键映射按键值
struct key_remap {
    u8 bit_value;
    u8 remap_value;
};

struct key_remap_data {
    u8 remap_num;
    const struct key_remap *table;
};

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键初始化函数，初始化所有注册的按键驱动
 */
/* ----------------------------------------------------------------------------*/
void key_driver_init(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件过滤、检测和发送
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 */
/* ----------------------------------------------------------------------------*/
void key_event_handler(struct key_event *key);


void key_active_set(u8 port);

void key_wakeup_init(void);
#endif


