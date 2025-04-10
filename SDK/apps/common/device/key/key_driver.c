#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key_driver.data.bss")
#pragma data_seg(".key_driver.data")
#pragma const_seg(".key_driver.text.const")
#pragma code_seg(".key_driver.text")
#endif
#include "system/timer.h"
#include "asm/power_interface.h"
#include "key_driver.h"

#define LOG_TAG             "[KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

static volatile u8 g_is_key_active = 0;             //按键是否活跃标志，判断是否可进低功耗
static volatile bool g_key_idle_query_en = false;   //是否需要判断g_is_key_active的使能标志

__attribute__((weak))
void key_down_event_handler(u8 key_value)
{

}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键扫描函数，扫描所有注册的按键驱动
 *
 * @param key_ops:按键操作句柄
 */
/* ----------------------------------------------------------------------------*/
static void key_driver_scan(void *key_ops)
{
    struct key_driver_ops *key_handler = (struct key_driver_ops *)key_ops;
    struct key_driver_para *scan_para = (struct key_driver_para *)key_handler->param;

    u8 key_event = 0;
    u8 cur_key_value = NO_KEY;
    u8 key_value = 0;
    struct key_event key = {0};

    key.init = 1;
    //区分按键类型
    key.type = scan_para->key_type;

    cur_key_value = key_handler->get_value();

    if (cur_key_value != NO_KEY) {
        //35*10Ms
        g_is_key_active = 35;
    } else if (g_is_key_active) {
        g_is_key_active --;
    }
    //===== 按键消抖处理
    //当前按键值与上一次按键值如果不相等, 重新消抖处理, 注意filter_time != 0;
    if (cur_key_value != scan_para->filter_value && scan_para->filter_time) {
        //消抖次数清0, 重新开始消抖
        scan_para->filter_cnt = 0;
        //记录上一次的按键值
        scan_para->filter_value = cur_key_value;
        //第一次检测, 返回不做处理
        return;
    }
    //当前按键值与上一次按键值相等, filter_cnt开始累加
    if (scan_para->filter_cnt < scan_para->filter_time) {
        scan_para->filter_cnt++;
        return;
    }
    //===== 按键消抖结束, 开始判断按键类型(单击, 双击, 长按, 多击, HOLD, (长按/HOLD)抬起)

    if (cur_key_value != scan_para->last_key) {
        if (cur_key_value == NO_KEY) {
            //cur_key = NO_KEY; last_key = valid_key -> 按键被抬起
            if (scan_para->press_cnt >= scan_para->long_time) {
                //长按/HOLD状态之后被按键抬起;
                key_event = KEY_ACTION_UP;
                key_value = scan_para->last_key;
                goto __notify;
            } else {
                key_event = KEY_ACTION_CLICK;
                key_value = scan_para->last_key;
                scan_para->click_delay_cnt = 1;
                goto __notify;
            }
        } else {
            //cur_key = valid_key, last_key = NO_KEY -> 按键被按下
            scan_para->press_cnt = 1;   //用于判断long和hold事件的计数器重新开始计时;
            scan_para->press_sum_cnt = 1;
            scan_para->click_delay_cnt = 0;

            key_down_event_handler(cur_key_value);
        }
        //返回, 等待延时时间到
        goto __scan_end;
    } else {
        //cur_key = last_key -> 没有按键按下/按键长按(HOLD)
        if (cur_key_value == NO_KEY) {
            //last_key = NO_KEY; cur_key = NO_KEY -> 没有按键按下
            if (scan_para->click_delay_cnt > 0) {
                scan_para->click_delay_cnt++;
                if (scan_para->click_delay_cnt > scan_para->click_delay_time) {
                    key_event = KEY_ACTION_NO_KEY;
                    scan_para->click_delay_cnt = 0;
                    goto __notify;   //有按键需要消息需要处理
                }
            }
            goto __scan_end;     //没有按键需要处理
        } else {
            //last_key = valid_key; cur_key = valid_key, press_cnt累加用于判断long和hold
            scan_para->press_cnt++;
            if (scan_para->press_sum_cnt) {
                scan_para->press_sum_cnt++;
            }
            if (scan_para->press_cnt == scan_para->long_time) {
                key_event = KEY_ACTION_LONG;
            } else if (scan_para->press_cnt >= scan_para->hold_time) {
                key_event = KEY_ACTION_HOLD;
                scan_para->press_cnt = scan_para->long_time;
            } else {
                goto __scan_end; //press_cnt没到长按和HOLD次数, 返回
            }
            key_value = cur_key_value;
            goto __notify;
        }
    }

__notify:
    key.event = key_event;
    key.value = key_value;
    key.tmr = sys_timer_get_ms();
    /* printf("key_value: 0x%x, event: %d\n", key_value, key_event); */
    key_event_handler(&key);
__scan_end:
    scan_para->last_key = cur_key_value;
    return;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief wakeup回调函数
 *
 * @param port:唤醒口
 */
/* ----------------------------------------------------------------------------*/
void key_active_set(u8 port)
{
    g_is_key_active = 35;      //35*10Ms
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键初始化函数，初始化所有注册的按键驱动
 */
/* ----------------------------------------------------------------------------*/
void key_driver_init(void)
{
    const struct key_driver_ops *key;
    list_for_each_key(key) {
        if (!key->key_init) {
            continue;
        }
        if ((!key->key_init()) && key->get_value) {
            sys_s_hi_timer_add((void *)key, key_driver_scan, key->param->scan_time); //注册按键扫描定时器
            if (key->idle_query_en) {
                g_key_idle_query_en = true;
            }
        }
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键模块是否可进低功耗查询函数
 *
 * @return 0：不可进
 *         1：可进
 */
/* ----------------------------------------------------------------------------*/
static u8 key_idle_query(void)
{
    if (g_key_idle_query_en) {
        return !g_is_key_active;
    } else {
        return 1;
    }
}
REGISTER_LP_TARGET(key_driver_target) = {
    .name = "key",
    .is_idle = key_idle_query,
};

