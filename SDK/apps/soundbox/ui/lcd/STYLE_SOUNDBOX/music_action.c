#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".music_action.data.bss")
#pragma data_seg(".music_action.data")
#pragma const_seg(".music_action.text.const")
#pragma code_seg(".music_action.text")
#endif
#include "ui/ui.h"
#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "audio_config.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#include "../lyrics_api.h"
#include "music/music_player.h"
#include "app_msg.h"
#include "file_player.h"
#include "app_music.h"
#include "music/music_player.h"
#include "effects/eq_config.h"
#include "idle.h"



#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL

/* extern const char *music_file_get_cur_name(int *len, int *is_unicode); */

typedef struct _MUSIC_UI_VAR {
    u8 start: 1;
    u8 menu: 1;
    u8 eq_mode: 3;
    u8 cycle_mode: 3;
    u8 disp_lrc;
    int timer;
    int total_time;
    int file_total;
    int cur_num;
} MUSIC_UI_VAR;


static MUSIC_UI_VAR *__this = NULL;
static u8 enter_music_mode = 0;
static u16 disp_time = 0;

//*----------------------------------------------------------------------------*/
/**@brief    music 获取当前播放的文件名
  @param    len:返回的文件名长度， is_unicode:文件名是否为unicode
   @return  文件名
   @note    用于获取播放成功的文件名信息
*/
/*----------------------------------------------------------------------------*/
u8 ui_music_file_name[128] = {0};
const char *ui_get_music_file_cur_name(struct music_player *player_hd, int *len, int *is_unicode)
{
    int music_file_name_len =  fget_name(player_hd->file, ui_music_file_name, sizeof(ui_music_file_name));
    if (ui_music_file_name[0] == '\\' && ui_music_file_name[1] == 'U') {
        *is_unicode = 1 ;
        *len = music_file_name_len - 2;

        /*for printf test*/
        char *utf8 = zalloc(2 * sizeof(ui_music_file_name));
        int file_len = Unicode2UTF8(utf8, (u16 *)ui_music_file_name + 1, (music_file_name_len - 2) / 2);
        printf(">>>[ui test]:filename = %s, len = %d\n", utf8, file_len);
        if (utf8) {
            free(utf8);
            utf8 = NULL;
        }
        /*for printf test*/

        return (const char *)(ui_music_file_name + 2);
    }

    //不是unicode
    *is_unicode = 0 ;
    *len = music_file_name_len;
    return (const char *)ui_music_file_name;
}


//*----------------------------------------------------------------------------*/
/**@brief    music 获取当前播放的文件名
  @param    len:返回的文件名长度， is_unicode:文件名是否为unicode
   @return  文件名
   @note    用于获取播放成功的文件名信息
*/
/*----------------------------------------------------------------------------*/
u32 ui_get_music_player_file_total(struct music_player *player_hd)
{
    if (player_hd && player_hd->fsn) {
        return player_hd->fsn->file_number;
    }
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放文件序号
   @param
   @return   当前文件序号
   @note
*/
/*----------------------------------------------------------------------------*/
u32 ui_get_music_player_file_cur(struct music_player *player_hd)
{
    if (player_hd && player_hd->fsn) {
        return player_hd->fsn->file_counter;
    }
    return 0;
}



static int music_start(const char *type, u32 arg)
{
    const char *file_name;
    printf("__FUNCTION__ =%s type =%s %d\n", __FUNCTION__, type, arg);
    struct unumber num;
    struct utime time_r;

    if (!__this) {
        return 0;
    }

    if (type && !strcmp(type, "show_lyric")) {
        if (arg) {
            /* ui_hide(MUSIC_FILE);//不显示歌名 */
            /* ui_show(MUSIC_LYRICS);//显示歌词 */
        } else {
            /* ui_hide(MUSIC_LYRICS);//不显示歌词 */
            /* ui_show(MUSIC_FILE);//显示歌名 */
            int len = 0;
            int is_unicode = 0;
            file_name = ui_get_music_file_cur_name(music_app_get_cur_hdl(), &len, &is_unicode);
            if (len && file_name) {
                printf("music start is_unicode = %d\n", is_unicode);
                put_buf((const u8 *)file_name, len);
                if (is_unicode) {
                    ui_text_set_text_by_id(LRC_TEXT_ID_SEC, "", 16, FONT_DEFAULT);
                    ui_text_set_textw_by_id(MUSIC_FILE, file_name, len, FONT_ENDIAN_SMALL, FONT_DEFAULT | FONT_SHOW_SCROLL);
                } else {
                    ui_text_set_text_by_id(LRC_TEXT_ID_SEC, "", 16, FONT_DEFAULT);
                    ui_text_set_text_by_id(MUSIC_FILE, file_name, len, FONT_DEFAULT);
                }

            }
        }
        return 0;
    }


    if (type && !strcmp(type, "dev")) {
        if (arg && (!strncmp((char *)arg, "udisk", 5))) {
            ui_pic_show_image_by_id(MUSIC_DEV, 1); //显示udik
        } else {
            ui_pic_show_image_by_id(MUSIC_DEV, 2); //显示sd
        }
        return 0;
    }


    if (type && !strcmp(type, "filenum")) {
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = arg;
        ui_number_update_by_id(MUSIC_CUR_NUM, &num);
        return 0;
    }


    if (type && !strcmp(type, "total_filenum")) {
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = arg;
        ui_number_update_by_id(MUSIC_TOTAL_NUM, &num);
        return 0;
    }

    return 0;
}

static const struct uimsg_handl ui_msg_handler[] = {
    { "music_start",        music_start     }, /* */
    { NULL, NULL},      /* 必须以此结尾！ */
};




static void music_check_add(void *ptr);

/************************************************************
                         音乐模式主页窗口控件
 ************************************************************/


static int music_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    static int bt_status_timer = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***music_mode_onchange***\n");
        enter_music_mode = 1;
        /* key_ui_takeover(1); */
        if (!__this) {
            __this = zalloc(sizeof(MUSIC_UI_VAR));
        }
        ui_register_msg_handler(ID_WINDOW_MUSIC, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        if (!__this->timer) {
            __this->timer = sys_timer_add(NULL, music_check_add, 500);
        }
        disp_time = 0;
        break;
    case ON_CHANGE_RELEASE:

        if (__this->timer) {
            sys_timer_del(__this->timer);
        }

        if (__this) {
            free(__this);
            __this = NULL;
        }
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




/* extern void test_browser(); */
/************************************************************
                         音乐模式布局控件按键事件
 ************************************************************/

static int music_layout_init(int id)
{
#if (defined(TCFG_LRC_LYRICS_ENABLE) && (TCFG_LRC_LYRICS_ENABLE))
#else
#endif
    return 0;
}

static int music_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_text *text = (struct ui_text *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        /* ui_text_set_text_attrs(text, NULL, 0, FONT_ENCODE_UTF8, 0, FONT_DEFAULT); */
        break;
    case ON_CHANGE_SHOW_PROBE:
        break;
    case ON_CHANGE_FIRST_SHOW:
        ui_set_call(music_layout_init, 0);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

static int music_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("music_layout_onkey %d\n", e->value);
    char *logo = music_app_get_dev_cur();
    switch (e->value) {
    case UI_KEY_MENU:
        ui_show(MUSIC_MENU_LAYOUT);
        break;
    case UI_KEY_OK:
        app_send_message(APP_MSG_MUSIC_PP, 0);
        break;
    case UI_KEY_UP:
        app_send_message(APP_MSG_MUSIC_PREV, 0);
        break;
    case UI_KEY_DOWN:
        app_send_message(APP_MSG_MUSIC_NEXT, 0);
        break;
    case UI_KEY_VOLUME_INC:
    case UI_KEY_VOLUME_DEC:
        ui_show(MUSIC_VOL_LAYOUT);
        break;
    case UI_KEY_MODE:
        UI_HIDE_CURR_WINDOW();
        ui_show_main(ID_WINDOW_SYS);
        break;
    case UI_KEY_POWER_START:
    case UI_KEY_POWER:
        power_off_deal(APP_MSG_KEY_POWER_OFF);
        break;
    default:
        return false;
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_LAYOUT)
.onchange = music_layout_onchange,
 .onkey = music_layout_onkey,
  .ontouch = NULL,
};

void music_player_disp_status()
{
    if (__this) {
        __this->disp_lrc = 1;
        ui_hide(MUSIC_START);
// ui_show(MUSIC_FILE);
        //  ui_show(MUSIC_LYRICS);
    }
}
/************************************************************
                         音乐模式的定时器
 ************************************************************/
static void music_check_add(void *ptr)
{
    int sencond;
    struct utime time_r;
    char *logo = music_app_get_dev_cur();

    if (!__this) {
        return;
    }

#if TCFG_EQ_ENABLE
    if (eq_mode_get_cur() != __this->eq_mode) {
        __this->eq_mode = eq_mode_get_cur();
        ui_pic_show_image_by_id(MUSIC_EQ, __this->eq_mode);
    }
#endif

    if (music_player_get_repeat_mode() != __this->cycle_mode) {
        __this->cycle_mode = music_player_get_repeat_mode();
        ui_pic_show_image_by_id(MUSIC_MLOOP, __this->cycle_mode);
    }


    if (true == music_file_get_player_status(get_music_file_player())) {
        if (!__this->start) {
            ui_pic_show_image_by_id(MUSIC_START, 0);
            if (!disp_time) {
                disp_time = sys_timeout_add(NULL, music_player_disp_status, 200);
            }
            sencond = music_file_get_total_time(get_music_file_player());
            if (-1 == sencond) {
                sencond = 0;
            }
            time_r.hour = sencond / 60 / 60;
            time_r.min = sencond / 60 % 60;
            time_r.sec = sencond % 60;
            ui_time_update_by_id(MUSIC_TOTAL_TIME, &time_r);
            __this->total_time = sencond;
            __this->start = 1;
        }
        sencond = music_file_get_total_time(get_music_file_player());
        if (-1 == sencond) {
            sencond = 0;
        }
        printf("total time:%d\n", sencond);
        if (sencond != __this->total_time) {
            time_r.hour = sencond / 60 / 60;
            time_r.min = sencond / 60 % 60;
            time_r.sec = sencond % 60;
            ui_time_update_by_id(MUSIC_TOTAL_TIME, &time_r);
            __this->total_time = sencond;
        }
        sencond = music_file_get_cur_time(get_music_file_player());
        if (-1 == sencond) {
            sencond = 0;
        }
        printf("sec = %d \n", sencond);
        time_r.hour = sencond / 60 / 60;
        time_r.min = sencond / 60 % 60;
        time_r.sec = sencond % 60;
        ui_time_update_by_id(MUSIC_CUR_TIME, &time_r);
        if (__this->disp_lrc == 1) {
#if (defined(TCFG_LRC_LYRICS_ENABLE) && (TCFG_LRC_LYRICS_ENABLE))
            sencond = music_file_get_cur_time(get_music_file_player());
            if (-1 == sencond) {
                sencond = 0;
            }
            if (lrc_show_api(MUSIC_FILE, sencond, 0)) {
                if (enter_music_mode == 1) {
                    UI_MSG_POST("music_start:dev=%4:filenum=%4:total_filenum=%4", \
                                logo, \
                                ui_get_music_player_file_cur(music_app_get_cur_hdl()), \
                                ui_get_music_player_file_total(music_app_get_cur_hdl()));
                    enter_music_mode = 0;
                }
            } else {
                if (enter_music_mode == 1) {
                    UI_MSG_POST("music_start:show_lyric=%4:dev=%4:filenum=%4:total_filenum=%4", \
                                0,   \
                                logo, \
                                ui_get_music_player_file_cur(music_app_get_cur_hdl()), \
                                ui_get_music_player_file_total(music_app_get_cur_hdl()));
                    enter_music_mode = 0;
                }
            }
#endif
        }
    } else if (FILE_PLAYER_PAUSE == music_file_get_player_status(get_music_file_player())) {
        if (__this->start) {
            if (disp_time) {
                sys_timeout_del(disp_time);
                disp_time = 0;
                // ui_hide(MUSIC_FILE);
                // ui_hide(MUSIC_LYRICS);
                ui_pic_show_image_by_id(MUSIC_START, 1);
            }
            __this->start = 0;
            __this->disp_lrc = 0;
        }
    } else {
        if (__this->start) {
            if (disp_time) {
                sys_timeout_del(disp_time);
                disp_time = 0;
                // ui_hide(MUSIC_FILE);
                // ui_hide(MUSIC_LYRICS);
                ui_pic_show_image_by_id(MUSIC_START, 1);
            }
            __this->start = 0;
            __this->disp_lrc = 0;
        }
    }
}

/************************************************************
                         音乐模式的一些控件初始化事件
 ************************************************************/

static int play_timer_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_time *timer = (struct ui_time *)ctr;
    int sencond = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        if (timer->text.elm.id == MUSIC_CUR_TIME) {
            sencond = music_file_get_cur_time(get_music_file_player());
            if (-1 == sencond) {
                sencond = 0;
            }
        } else if (timer->text.elm.id == MUSIC_TOTAL_TIME) {
            sencond = music_file_get_total_time(get_music_file_player());
            if (-1 == sencond) {
                sencond = 0;
            }
        }

        timer->hour = sencond / 60 / 60;
        if (true == music_file_get_player_status(get_music_file_player())) {
            timer->min = sencond / 60 % 60;
            timer->sec = sencond % 60;
        } else {
            timer->min = sencond / 60 % 60;
            timer->sec = sencond % 60;
        }
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_TOTAL_TIME)
.onchange = play_timer_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

REGISTER_UI_EVENT_HANDLER(MUSIC_CUR_TIME)
.onchange = play_timer_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};






/************************************************************
                         音乐模式主菜单列表按键事件
 ************************************************************/

static int music_enter_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case UI_KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
            ui_show(MUSIC_EQ_LAYOUT);
            break;
        case 1:
            ui_show(MUSIC_CYCLE_LAYOUT);
            break;
        case 2:
            ui_show(MUSIC_FILE_LAYOUT);
            break;
        case 3:
            ui_hide(MUSIC_MENU_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_MENU_LIST)
.onchange = NULL,
 .onkey = music_enter_callback,
  .ontouch = NULL,
};





/************************************************************
                         音乐模式eq菜单列表按键事件
 ************************************************************/

static const int eq_mode_pic[] = {
    MUSIC_2,
    MUSIC_8,
    MUSIC_12,
    MUSIC_14,
    MUSIC_16,
    MUSIC_18,
};



#if TCFG_EQ_ENABLE
static int eq_pic_common_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    switch (event) {
    case ON_CHANGE_INIT:
        if (pic->elm.id == eq_mode_pic[eq_mode_get_cur() % 6]) {
            ui_pic_set_image_index(pic, 1);
        }
        break;
    case ON_CHANGE_SHOW_POST:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}

REGISTER_UI_EVENT_HANDLER(MUSIC_2)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MUSIC_8)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MUSIC_12)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MUSIC_14)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MUSIC_16)
.onchange = eq_pic_common_onchange,
};
REGISTER_UI_EVENT_HANDLER(MUSIC_18)
.onchange = eq_pic_common_onchange,
};
#endif

static int music_eq_menu_list_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int list = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        printf("ON_CHANGE_INIT %d \n", grid->avail_item_num);
        break;
    default:
        break;
    }
    return false;
}

static int music_eq_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case UI_KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
#if TCFG_EQ_ENABLE
            eq_mode_set(sel_item);
#endif
        case 6:
            ui_hide(MUSIC_EQ_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_EQ_LIST)
.onchange = music_eq_menu_list_onchange,
 .onkey = music_eq_callback,
  .ontouch = NULL,
};




/************************************************************
                         音乐模式循环菜单列表按键事件
 ************************************************************/

static const int cycle_mode_pic[] = {
    MUSIC_29,
    MUSIC_32,
    MUSIC_35,
    MUSIC_38,
    MUSIC_41,
};


static int music_cycle_menu_list_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int list = 0;
    switch (e) {
    case ON_CHANGE_INIT:
        printf("ON_CHANGE_INIT %d \n", grid->avail_item_num);
        int id =  cycle_mode_pic[music_player_get_repeat_mode() % 5];
        printf("id = %x \n", id);
        ui_pic_show_image_by_id(id, 1);
        break;
    default:
        break;
    }
    return false;
}

static int music_cycle_callback(void *ctr, struct element_key_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    printf("ui key %s %d\n", __FUNCTION__, e->value);
    int sel_item;
    switch (e->value) {
    case UI_KEY_OK:
        sel_item = ui_grid_cur_item(grid);
        switch (sel_item) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            music_player_set_repeat_mode(music_app_get_cur_hdl(), sel_item);
        case 5:
            ui_hide(MUSIC_CYCLE_LAYOUT);
            break;
        }
        break;
    default:
        return false;
    }
    return TRUE;
}


REGISTER_UI_EVENT_HANDLER(MUSIC_CYCLE_LIST)
.onchange = music_cycle_menu_list_onchange,
 .onkey = music_cycle_callback,
  .ontouch = NULL,
};



/************************************************************
                         电池控件事件
 ************************************************************/

static void battery_timer(void *priv)
{
    int  incharge = 0;//充电标志
    int id = (int)(priv);
    static u8 percent = 0;
    static u8 percent_last = 0;
    if (get_charge_online_flag()) { //充电时候图标动态效果
        if (percent > get_vbat_percent()) {
            percent = 0;
        }
        ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标
        percent += 20;
    } else {

        percent = get_vbat_percent();
        if (percent != percent_last) {
            ui_battery_set_level_by_id(id, percent, incharge); //充电标志,ui可以显示不一样的图标,需要工具设置
            percent_last = percent;
        }

    }
}


static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;
    int  incharge = 0;//充电标志

    switch (e) {
    case ON_CHANGE_INIT:
        ui_battery_set_level(battery, get_vbat_percent(), incharge);
        if (!timer) {
            timer = sys_timer_add((void *)battery->elm.id, battery_timer, 1 * 1000); //传入的id就是BT_BAT
        }
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    case ON_CHANGE_RELEASE:
        if (timer) {
            sys_timer_del(timer);
            timer = 0;
        }
        break;
    default:
        return false;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(MUSIC_BAT)
.onchange = battery_onchange,
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
        ui_hide(id);
    }
    music_timer = 0;
}

static int vol_init(int id)
{
    struct unumber num;
    num.type = TYPE_NUM;
    num.numbs =  1;
    num.number[0] = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
    ui_number_update_by_id(MUSIC_VOL_NUM, &num);
    return 0;
}


static int music_vol_layout_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct layout *layout = (struct layout *)ctr;
    struct unumber num;
    switch (e) {
    case ON_CHANGE_INIT:
        if (!music_timer) {
            music_timer  = sys_timeout_add((void *)layout->elm.id, music_vol_timeout, 3000);
        }
        break;

    case ON_CHANGE_FIRST_SHOW://布局第一次显示，可以对布局内的一些控件进行初始化
        ui_set_call(vol_init, 0);
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


static int music_vol_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("music_vol_layout_onkey %d\n", e->value);
    struct unumber num;
    u8 vol;

    if (music_timer) {
        sys_timer_modify(music_timer, 3000);
    }

    switch (e->value) {
    case UI_KEY_MENU:
        break;
    case UI_KEY_UP:
    case UI_KEY_VOLUME_INC:
        app_audio_volume_up(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);
        break;
    case UI_KEY_DOWN:
    case UI_KEY_VOLUME_DEC:
        app_audio_volume_down(1);
        vol = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        num.type = TYPE_NUM;
        num.numbs =  1;
        num.number[0] = vol;
        ui_number_update_by_id(MUSIC_VOL_NUM, &num);
        break;
    default:
        return false;
        break;
    }
    return true;
}



REGISTER_UI_EVENT_HANDLER(MUSIC_VOL_LAYOUT)
.onchange = music_vol_layout_onchange,
 .onkey = music_vol_layout_onkey,
  .ontouch = NULL,
};



#endif
