#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sbar_bt_action.data.bss")
#pragma data_seg(".sbar_bt_action.data")
#pragma const_seg(".sbar_bt_action.text.const")
#pragma code_seg(".sbar_bt_action.text")
#endif
#include "app_config.h"
#include "ui/ui.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "app_msg.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "app_default_msg_handler.h"


#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBAR))
#if (TCFG_APP_BT_EN)

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)

static int ui_music_vol_handler(const char *type, u32 arg);
static int ui_mute_sta_handler(const char *type, u32 arg);
static int ui_eq_sta_handler(const char *type, u32 arg);
/************************************************************
    窗口app与ui的消息交互注册 app可以发生消息到ui进行显示
 ************************************************************/

static const struct uimsg_handl ui_msg_handler[] = {
    { "music_vol",        ui_music_vol_handler     }, /* 显示音量 */
    { "mute_sta",         ui_mute_sta_handler      }, /* 显示正常或mute状态 */
    { "eq_sta",           ui_eq_sta_handler        }, /* 显示eq状态*/
    { NULL, NULL},      /* 必须以此结尾！ */
};

//先把其他layout hide了，再show传入的layout
static void ui_hide_other_and_show_one_layout(int id)
{
    ui_hide(BT_VOL_LAYOUT);
    ui_hide(BT_EQ_LAYOUT);
    ui_hide(BT_LAYOUT);
    ui_hide(BT_MUTE_LAYOUT);
    ui_show(id);
}


/************************************************************
                         蓝牙主页窗口控件
              可以在这个窗口注册各个布局需要用的资源
 ************************************************************/
static int bt_layout_check(int p)
{
    if (get_sys_aduio_mute_statu()) {
        ui_hide_other_and_show_one_layout(BT_MUTE_LAYOUT);
    } else {
        ui_hide_other_and_show_one_layout(BT_LAYOUT);
    }
    return 0;
}


static int bt_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***>>>>>>>>>>>>>>>>>>bt_mode_onchange***\n");
        ui_register_msg_handler(ID_WINDOW_BT, ui_msg_handler);//注册消息交互的回调
        /*
         * 注册APP消息响应
         */
        bt_layout_check(0);//判断返回哪个布局
        break;
    case ON_CHANGE_RELEASE:
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_BT)
.onchange = bt_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         蓝牙设置布局控件
 ************************************************************/

static int bt_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        break;

    case ON_CHANGE_FIRST_SHOW:
        /* ui_set_call(bt_status_check, 0); */
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(BT_LAYOUT)
.onchange = bt_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


/************************************************************
                         音量设置布局控件
 ************************************************************/
static u16 bt_timer = 0;
static void bt_vol_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        bt_layout_check(0);//判断返回哪个布局
    }
    bt_timer = 0;
}

//音量显示处理
static void ui_show_vol_deal(u32 cur_vol)
{
    struct ui_number *num;
    if (cur_vol > 0 && cur_vol < app_audio_get_max_volume()) {

        ui_hide(BT_VOL_M_PIC);//隐藏最大最小控件

        //需要重新显示音量图标及大小
        ui_pic_show_image_by_id(BT_VOL_PIC, 0);
        num = (struct ui_number *)ui_core_get_element_by_id(BT_VOL_NUM);
        if (num) {
            num->text.elm.css.invisible  = 0;//恢复数字
        }
        struct unumber num;
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = cur_vol;
        ui_number_update_by_id(BT_VOL_NUM, &num);
    } else {
        ui_hide(BT_VOL_PIC);
        ui_hide(BT_VOL_NUM);

        if (cur_vol) {
            ui_pic_show_image_by_id(BT_VOL_M_PIC, 0);
        } else {
            ui_pic_show_image_by_id(BT_VOL_M_PIC, 1);
        }
    }
}

//响应图层消息
static int ui_music_vol_handler(const char *type, u32 arg)
{
    struct unumber num;

    if (ui_get_disp_status_by_id(BT_VOL_LAYOUT) <= 0) {
        ui_hide_other_and_show_one_layout(BT_VOL_LAYOUT);
    }

    if (bt_timer) {
        sys_timer_modify(bt_timer, 3000); //音量继续显示
    }

    if (type && (!strcmp(type, "vol"))) {
        if (ui_get_disp_status_by_id(BT_VOL_LAYOUT) == TRUE) {
            ui_show_vol_deal(arg);
        }
    }
    return 0;
}

static void ui_show_vol_init(void)
{
    u8 ui_get_cur_vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);

    /* printf("ui_get_cur_vol:%d\n",ui_get_cur_vol); */
    /* printf("cur max vol:%d\n",app_audio_get_max_volume()); */
    if (ui_get_cur_vol > 0 && ui_get_cur_vol < app_audio_get_max_volume()) {
        ui_hide(BT_VOL_M_PIC);//隐藏最大最小控件

        //初始化不用再单独show另外两个控件
        struct unumber num;
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = ui_get_cur_vol;
        ui_number_update_by_id(BT_VOL_NUM, &num);
    } else {
        ui_hide(BT_VOL_PIC);
        ui_hide(BT_VOL_NUM);

        //初始化不用再单独showXX_VOL_M_PIC
        struct ui_pic *pic = (struct ui_pic *)ui_core_get_element_by_id(BT_VOL_M_PIC);
        if (ui_get_cur_vol) {
            ui_pic_set_image_index(pic, 0); //显示max
        } else {
            ui_pic_set_image_index(pic, 1);//显示min
        }
    }
}

//注意音量里全部控件默认隐藏为true
static int bt_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        ui_show_vol_init();
        if (!bt_timer) {
            bt_timer  = sys_timeout_add((void *)layout->elm.id, bt_vol_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW:
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (bt_timer) {
            sys_timeout_del(bt_timer);
            bt_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(BT_VOL_LAYOUT)
.onchange = bt_vol_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         mute布局控件
 ************************************************************/
//响应图层消息
static int ui_mute_sta_handler(const char *type, u32 arg)
{
    if (ui_get_disp_status_by_id(BT_MUTE_LAYOUT) <= 0 && arg != 0) {
        ui_hide_other_and_show_one_layout(BT_MUTE_LAYOUT);
    } else {
        ui_hide_other_and_show_one_layout(BT_LAYOUT);
    }

    return 0;
}

static int bt_mute_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        break;

    case ON_CHANGE_FIRST_SHOW:
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        break;

    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(BT_MUTE_LAYOUT)
.onchange = bt_mute_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


/************************************************************
                         eq布局控件
 ************************************************************/
static u16 bt_eq_timer = 0;
static void bt_eq_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        bt_layout_check(0);//判断返回哪个布局
    }
    bt_eq_timer = 0;
}

//响应图层消息
static int ui_eq_sta_handler(const char *type, u32 arg)
{
    struct unumber num;

    if (ui_get_disp_status_by_id(BT_EQ_LAYOUT) <= 0) {
        ui_hide_other_and_show_one_layout(BT_EQ_LAYOUT);
    }

    if (bt_eq_timer) {
        sys_timer_modify(bt_eq_timer, 3000); //继续显示,n毫秒后退出
    }

    if (type && (!strcmp(type, "eq"))) {
        if (ui_get_disp_status_by_id(BT_EQ_LAYOUT) == TRUE) {
            num.type = TYPE_NUM;
            num.numbs = 1;
            num.number[0] = arg;
            ui_number_update_by_id(BT_EQ_NUM, &num);
        }
    }
    return 0;
}

static int bt_eq_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!bt_eq_timer) {
            bt_eq_timer  = sys_timeout_add((void *)layout->elm.id, bt_eq_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW:
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (bt_eq_timer) {
            sys_timeout_del(bt_eq_timer);
            bt_eq_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(BT_EQ_LAYOUT)
.onchange = bt_eq_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


#endif
#endif
