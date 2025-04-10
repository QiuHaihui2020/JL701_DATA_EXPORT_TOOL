#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sbar_music_action.data.bss")
#pragma data_seg(".sbar_music_action.data")
#pragma const_seg(".sbar_music_action.text.const")
#pragma code_seg(".sbar_music_action.text")
#endif
#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "audio_config.h"
#include "app_music.h"
#include "app_default_msg_handler.h"


#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBAR))
#if (TCFG_APP_MUSIC_EN)

#define STYLE_NAME  JL

static int music_start(const char *type, u32 arg);
static int ui_music_vol_handler(const char *type, u32 arg);
static int ui_mute_sta_handler(const char *type, u32 arg);
static int ui_eq_sta_handler(const char *type, u32 arg);


static void ui_hide_other_and_show_one_layout(int id)
{
    ui_hide(MUSIC_VOL_LAYOUT);
    ui_hide(MUSIC_EQ_LAYOUT);
    ui_hide(MUSIC_LAYOUT);
    ui_hide(MUSIC_MUTE_LAYOUT);
    ui_show(id);
}

static const struct uimsg_handl ui_msg_handler[] = {
    { "music_start",      music_start     }, /* */
    { "music_vol",        ui_music_vol_handler     }, /* 显示音量 */
    { "mute_sta",         ui_mute_sta_handler      }, /* 显示正常或mute状态 */
    { "eq_sta",           ui_eq_sta_handler        }, /* 显示eq状态*/
    { NULL, NULL},      /* 必须以此结尾！ */
};


/************************************************************
                         音乐模式主页窗口控件
 ************************************************************/
static int music_layout_check(int p)
{
    if (get_sys_aduio_mute_statu()) {
        ui_hide_other_and_show_one_layout(MUSIC_MUTE_LAYOUT);
    } else {
        ui_hide_other_and_show_one_layout(MUSIC_LAYOUT);
    }
    return 0;
}

static int music_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***music_mode_onchange***\n");
        ui_register_msg_handler(ID_WINDOW_MUSIC, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        music_layout_check(0); //判断返回哪个布局
        break;
    case ON_CHANGE_RELEASE:
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_MUSIC)
.onchange = music_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};




/************************************************************
                         音乐模式布局控件事件
 ************************************************************/
//响应图层消息
static int music_start(const char *type, u32 arg)
{
    const char *file_name;
    printf("__FUNCTION__ =%s type =%s %d\n", __FUNCTION__, type, arg);

    if (get_sys_aduio_mute_statu()) { //mute的时候只显示mute
        return 0;
    }

    if (type && !strcmp(type, "dev")) {
        if (ui_get_disp_status_by_id(MUSIC_LAYOUT) <= 0) {
            ui_hide_other_and_show_one_layout(MUSIC_LAYOUT);
        }

        if (arg && (!strncmp((char *)arg, "udisk", 5))) {
            ui_pic_show_image_by_id(MUSIC_STATUS, 1); //显示udik
        } else if (arg && (!strncmp((char *)arg, "sd", 2))) {
            ui_pic_show_image_by_id(MUSIC_STATUS, 2); //显示sd
        } else if (arg && (!strncmp((char *)arg, "pause", 5))) {
            ui_pic_show_image_by_id(MUSIC_STATUS, 3); //显示pause
        }
        return 0;
    }

    return 0;
}

static int music_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        //显示layout前，把图片选好
        struct ui_pic *pic = (struct ui_pic *)ui_core_get_element_by_id(MUSIC_STATUS);
        char *logo = music_app_get_dev_cur();
        if (!pic) {
            puts("pic NULL\n");
            break;
        }
        if (logo && (!strncmp((char *)logo, "udisk", 5))) {
            ui_pic_set_image_index(pic, 1);
        } else if (logo && (!strncmp((char *)logo, "sd", 2))) {
            ui_pic_set_image_index(pic, 2);
        } else if (logo && (!strncmp((char *)logo, "pause", 5))) {
            ui_pic_set_image_index(pic, 3);
        }
        if (!logo) {
            ui_pic_set_image_index(pic, 0);
        }
        break;
    case ON_CHANGE_SHOW_PROBE:
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    case ON_CHANGE_INIT_PROBE:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_LAYOUT)
.onchange = music_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         音乐模式音量布局事件
 ************************************************************/

static u16 music_timer = 0;
static void music_vol_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        music_layout_check(0); //判断返回哪个布局
    }
    music_timer = 0;
}


static void ui_show_vol_init(void)
{
    u8 ui_get_cur_vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);

    /* printf("ui_get_cur_vol:%d\n",ui_get_cur_vol); */
    /* printf("cur max vol:%d\n",app_audio_get_max_volume()); */
    if (ui_get_cur_vol > 0 && ui_get_cur_vol < app_audio_get_max_volume()) {
        ui_hide(MUSIC_VOL_M_PIC); //隐藏最大最小图标

        //初始化不用再单独show另外两个控件
        struct unumber num;
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = ui_get_cur_vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);
    } else {
        ui_hide(MUSIC_VOL_PIC);
        ui_hide(MUSIC_VOL_NUM);

        //初始化不用再单独showXX_VOL_M_PIC
        struct ui_pic *pic = (struct ui_pic *)ui_core_get_element_by_id(MUSIC_VOL_M_PIC);
        if (ui_get_cur_vol) {
            ui_pic_set_image_index(pic, 0); //显示max
        } else {
            ui_pic_set_image_index(pic, 1);//显示min
        }
    }
}

static int music_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    struct unumber num;
    switch (e) {
    case ON_CHANGE_INIT:
        ui_show_vol_init();
        if (!music_timer) {
            music_timer  = sys_timeout_add((void *)layout->elm.id, music_vol_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW://布局第一次显示，可以对布局内的一些控件进行初始化
        break;
    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (music_timer) {
            sys_timeout_del(music_timer);
            music_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

//音量显示处理
static void ui_show_vol_deal(u32 cur_vol)
{
    struct ui_number *num;
    /* printf("cur_vol:%d\n",cur_vol); */
    /* printf("cur max vol:%d\n",app_audio_get_max_volume()); */
    if (cur_vol > 0 && cur_vol < app_audio_get_max_volume()) {
        ui_hide(MUSIC_VOL_M_PIC); //隐藏最大最小控件

        //需要重新显示音量图标及大小
        ui_pic_show_image_by_id(MUSIC_VOL_PIC, 0);
        num = (struct ui_number *)ui_core_get_element_by_id(MUSIC_VOL_NUM);
        if (num) {
            num->text.elm.css.invisible  = 0;//恢复数字
        }
        struct unumber num;
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = cur_vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);
    } else {
        ui_hide(MUSIC_VOL_PIC);
        ui_hide(MUSIC_VOL_NUM);

        if (cur_vol) {
            ui_pic_show_image_by_id(MUSIC_VOL_M_PIC, 0);
        } else {
            ui_pic_show_image_by_id(MUSIC_VOL_M_PIC, 1);
        }
    }

}

//响应图层消息
static int ui_music_vol_handler(const char *type, u32 arg)
{
    struct unumber num;

    if (ui_get_disp_status_by_id(MUSIC_VOL_LAYOUT) <= 0) {
        ui_hide_other_and_show_one_layout(MUSIC_VOL_LAYOUT);
    }

    if (music_timer) {
        sys_timer_modify(music_timer, 3000); //音量继续显示
    }

    if (type && (!strcmp(type, "vol"))) {
        if (ui_get_disp_status_by_id(MUSIC_VOL_LAYOUT) == TRUE) {
            ui_show_vol_deal(arg);
        }
    }
    return 0;
}



REGISTER_UI_EVENT_HANDLER(MUSIC_VOL_LAYOUT)
.onchange = music_vol_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

/************************************************************
                         mute布局控件
 ************************************************************/
//响应图层消息
static int ui_mute_sta_handler(const char *type, u32 arg)
{
    if (ui_get_disp_status_by_id(MUSIC_MUTE_LAYOUT) <= 0 && arg != 0) {
        ui_hide_other_and_show_one_layout(MUSIC_MUTE_LAYOUT);
    } else {
        ui_hide_other_and_show_one_layout(MUSIC_LAYOUT);
    }

    return 0;
}

static int music_mute_layout_onchange(void *ctr, enum element_change_event e, void *arg)
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

REGISTER_UI_EVENT_HANDLER(MUSIC_MUTE_LAYOUT)
.onchange = music_mute_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


/************************************************************
                         eq布局控件
 ************************************************************/
static u16 music_eq_timer = 0;
static void music_eq_timeout(void *p)
{
    int id = (int)(p);
    if (ui_get_disp_status_by_id(id) == TRUE) {
        music_layout_check(0);//判断返回哪个布局
    }
    music_eq_timer = 0;
}

//响应图层消息
static int ui_eq_sta_handler(const char *type, u32 arg)
{
    struct unumber num;

    if (ui_get_disp_status_by_id(MUSIC_EQ_LAYOUT) <= 0) {
        ui_hide_other_and_show_one_layout(MUSIC_EQ_LAYOUT);
    }

    if (music_eq_timer) {
        sys_timer_modify(music_eq_timer, 3000); //继续显示,n毫秒后退出
    }

    if (type && (!strcmp(type, "eq"))) {
        if (ui_get_disp_status_by_id(MUSIC_EQ_LAYOUT) == TRUE) {
            num.type = TYPE_NUM;
            num.numbs = 1;
            num.number[0] = arg;
            ui_number_update_by_id(MUSIC_EQ_NUM, &num);
        }
    }
    return 0;
}

static int music_eq_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!music_eq_timer) {
            music_eq_timer  = sys_timeout_add((void *)layout->elm.id, music_eq_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW:
        break;

    case ON_CHANGE_SHOW_POST:
        //有显示动作
        break;

    case ON_CHANGE_RELEASE:
        if (music_eq_timer) {
            sys_timeout_del(music_eq_timer);
            music_eq_timer = 0;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_EQ_LAYOUT)
.onchange = music_eq_layout_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


#endif
#endif
