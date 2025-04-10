#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key.data.bss")
#pragma data_seg(".key.data")
#pragma const_seg(".key.text.const")
#pragma code_seg(".key.text")
#endif
#include "app_msg.h"
#include "key_driver.h"
#include "system/timer.h"

/*最大多击次数限制, 目前SDK默认支持按键三击，如果需要更大的按键连击次数，改动此处需要同步修改*/
/*对应模式的key_msg_table.c的按键消息表,增加对应事件的消息映射*/
#define MULTI_CLICK_MAX_CNT     3

struct key_hold {
    u8 send_3s_msg;
    u8 send_5s_msg;
    u32 start_time;
};

static struct key_hold key_hold_hdl[3];

static struct key_hold *get_key_hold(u8 key_value)
{
    if (key_value < ARRAY_SIZE(key_hold_hdl)) {
        return &key_hold_hdl[key_value];
    }
    return NULL;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 多击按键判断
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 *
 * @return 0：不拦截按键事件
 *         1：拦截按键事件
 */
/* ----------------------------------------------------------------------------*/
static int multi_clicks_translate(struct key_event *key)
{
    static u8 click_cnt;
    static u8 notify_value = 0xff;
    struct key_hold *hold = get_key_hold(key->value);

    if (key->event == KEY_ACTION_LONG) {
        if (hold) {
            hold->start_time = jiffies;
        }
    } else if (key->event == KEY_ACTION_HOLD) {
        if (hold) {
            int time_msec = jiffies_offset_to_msec(hold->start_time, jiffies);
            if (time_msec >= 3000 && hold->send_3s_msg == 0) {
                hold->send_3s_msg = 1;
                key->event = KEY_ACTION_HOLD_3SEC;
            } else if (time_msec >= 5000 && hold->send_5s_msg) {
                hold->send_5s_msg = 1;
                key->event = KEY_ACTION_HOLD_5SEC;
            }
        }
    } else {
        if (hold) {
            hold->send_3s_msg = 0;
            hold->send_5s_msg = 1;
            hold->start_time = 0;
        }
    }

    if (key->event == KEY_ACTION_CLICK) {
        if (key->value != notify_value) {
            click_cnt = 1;
            notify_value = key->value;
        } else {
            click_cnt++;
        }
        return 1;
    }
    if (key->event == KEY_ACTION_NO_KEY) {
        if (click_cnt >= 2) {
            if (click_cnt > MULTI_CLICK_MAX_CNT) {
                click_cnt = MULTI_CLICK_MAX_CNT;        //限制多击按键最大连击次数，避险出现连击次数超过按键表定义的事件导致发到其他按键消息或者引发内存越界访问
            }
            key->event = KEY_ACTION_DOUBLE_CLICK + (click_cnt - 2);
        } else {
            key->event = KEY_ACTION_CLICK;
        }
        key->value = notify_value;
        click_cnt = 0;
        notify_value = NO_KEY;
    } else if (key->event > KEY_ACTION_CLICK) {
        click_cnt = 0;
        notify_value = NO_KEY;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 联合按键判断
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 *
 * @return 0：不拦截按键事件
 *         1：拦截按键事件
 */
/* ----------------------------------------------------------------------------*/
static int combination_key_translate(struct key_event *key)
{
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件过滤、检测和发送
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 */
/* ----------------------------------------------------------------------------*/
void key_event_handler(struct key_event *key)
{
    const struct key_callback *p;

    if (multi_clicks_translate(key)) {
        return;
    }
    if (combination_key_translate(key)) {
        return;
    }
    if (key->event == KEY_ACTION_NO_KEY) {
        return;
    }

    //外部需要格外做的处理流程，请通过注册的形式在此处回调
    list_for_each_key_callback(p) {
        if (p->cb_deal == NULL) {
            continue;
        }
        if (p->cb_deal(p->arg)) {
            return;
        }
    }
    app_send_message_from(MSG_FROM_KEY, sizeof(*key), (int *)key);
}


