#ifndef BT_BACKGROUND_H
#define BT_BACKGROUND_H


#include "generic/typedef.h"
#include "avctp_user.h"
#include "app_msg.h"

typedef enum {
    BACKGROUND_EVENT_NO_MACTH = 0,          //不需要进行处理
    BACKGROUND_SWITCH_TO_BT,                //需要进行切换到蓝牙模式
    BACKGROUND_PHONE_CALL_SWITCH_TO_BT,     //由于电话引起的任务切换
    BACKGROUND_EVENT_ORIGINAL_DEAL,         //需要再次走原本流程处理
    BACKGROUND_A2DP_SLIENCE_DETECT,         //后台进行能量检测
} BACKGROUND_EVENT_RESULT;

typedef enum {
    BACKGROUND_GOBACK_WITH_MODE_SWITCH,     //由模式切换回到蓝牙模式
    BACKGROUND_GOBACK_WITH_MUSIC,           //由播放歌曲回到蓝牙模式
    BACKGROUND_GOBACK_WITH_PHONE,           //由通话回到蓝牙模式
    BACKGROUND_GOBACK_WITH_OTHER,           //其他情况回到蓝牙模式
} BACKGROUND_GOBACK_MODE;

typedef struct _background_var {
    BACKGROUND_GOBACK_MODE backmode;
    u8 background_working;
    u8 close_bt_hw_in_background;           // 1-处于后台模式时关闭整个蓝牙，重新进入蓝牙再初始化
    u16 goback_timer;                       // 后台由电话切回蓝牙模式之后需要再电话结束之后返回原来模式的定时器
    u8  goback_mode;						// 通话结束之后需要切回哪个模式
    u8  goback_fitler;                      // 后台返回过滤，通话可能会有多个消息触发返回，防止多次发消息切模式
    bool poweron_need_switch_mode;          // 开机如果有需要切换模式需要等待进入蓝牙模式初始化完成之后才切模式
    u8  poweron_mode;                       // 记录poweron任务时需要切换的任务模式，等待蓝牙初始化完成之后才跳转到此模式
    struct list_head forward_msg_head;
    int (*original_hci_handler)(struct bt_event *);             //注册蓝牙模式的消息处理函数给后台调用
    int (*original_status_handler)(struct bt_event *);          //注册蓝牙模式的消息处理函数给后台调用
} background_var;

struct forward_msg {
    int msg_from;       // 后台切到前台之后还需要转发的消息来源
    int msg[4];         // 后台切换到前台之后需要转发的消息
    struct list_head entry;
};

bool bt_background_active(void);

void bt_switch_to_foreground(int action, bool exit_curr_app);

int bt_background_check_if_can_enter();

bool bt_in_background(void);

bool bt_background_msg_forward_filter(int *msg);

void bt_background_init(int (*hci_handler)(struct bt_event *), int (*status_handler)(struct bt_event *));

void bt_background_resume(void);

void bt_background_suspend(void);

void bt_background_set_switch_mode(u8 mode);

bool bt_background_switch_mode_check(void);

void bt_background_switch_mode_after_initializes(void);

#endif

