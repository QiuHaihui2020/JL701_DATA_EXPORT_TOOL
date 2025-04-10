#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_music.data.bss")
#pragma data_seg(".ui_music.data")
#pragma const_seg(".ui_music.text.const")
#pragma code_seg(".ui_music.text")
#endif
#include "ui/ui_api.h"
#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif
#include "file_player.h"
#include "music/music_player.h"
#include "app_music.h"
#include "le_broadcast.h"
#include "wireless_trans.h"

#if TCFG_APP_MUSIC_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
#include "music_ui.h"


void ui_music_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void ui_led7_show_music_time(void *hd, int sencond)
{
    LCD_API *dis = (LCD_API *)hd;
    u8 tmp_buf[5] = {0};
    u8 min = 0;
    min = sencond / 60 % 60;
    sencond = sencond % 60;

    itoa2(min, (u8 *)&tmp_buf[0]);
    itoa2(sencond, (u8 *)&tmp_buf[2]);


    dis->lock(1);
    /* dis->clear(); */
    dis->setXY(0, 0);
    dis->clear_icon(0xffff);
    dis->show_string(tmp_buf);
    dis->flash_icon(LED7_2POINT);
    dis->show_icon(LED7_PLAY);
    dis->show_icon(LED7_MP3);
    dis->lock(0);

}


static void led7_show_filenumber(void *hd, u16 file_num)
{
    LCD_API *dis = (LCD_API *)hd;
    u8 bcd_number[5] = {0};	    ///<换算结果显示缓存
    itoa4(file_num, (u8 *)bcd_number);

    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
#if TCFG_UI_LED1888_ENABLE //UI使用LED1888
    if (file_num > 999 && file_num <= 1999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
#endif
    dis->show_string(bcd_number);
    dis->lock(0);
}

static void led7_show_le(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" LE");
    dis->lock(0);
}

static void led7_show_pause(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" PAU");
    dis->show_icon(LED7_PAUSE);
    dis->lock(0);
}

static void led7_show_repeat_mode(void *hd, u32 val)
{
    if (val >= FCYCLE_MAX) { //大于循环类型
        return ;
    }
    u8 mode = (u8)val;

    const u8 playmodestr[][5] = {
        "LIST",
        " ALL",
        " ONE",
        "Fold",
        " rAn",
    };

    if (mode >= sizeof(playmodestr) / sizeof(playmodestr[0])) {
        printf("rpt mode display err !!\n");
        return ;
    }

    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)playmodestr[mode]);
    dis->lock(0);
}


static void led7_fm_show_freq(void *hd, u32 arg)
{
    u8 bcd_number[5] = {0};
    LCD_API *dis = (LCD_API *)hd;
    u16 freq = 0;
    freq = arg;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    itoa4(freq, (u8 *)bcd_number);
    if (freq > 999 && freq <= 1999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
    dis->show_string(bcd_number);
    dis->show_icon(LED7_DOT);
    dis->lock(0);
}

static void *ui_open_music(void *hd)
{
    MUSIC_DIS_VAR *ui_music = NULL;
    /* ui_music = (MUSIC_DIS_VAR *)malloc(sizeof(MUSIC_DIS_VAR)); */
    /* if (ui_music == NULL) { */
    /* return NULL; */
    /* } */
    ui_set_auto_reflash(500);//设置主页500ms自动刷新
    return ui_music;
}

static void ui_close_music(void *hd, void *private)
{
    MUSIC_DIS_VAR *ui_music = (MUSIC_DIS_VAR *)private;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }

    if (ui_music) {
        free(ui_music);
    }
}

static void led7_show_music_dev(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    char *dev = NULL;
    dev = music_app_get_dev_cur();
    if (dev) {
        if (!memcmp(dev, "udisk", 5)) {
            dis->show_icon(LED7_USB);
        } else {
            dis->show_icon(LED7_SD);
        }
    }
}


static void ui_music_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }

#if TCFG_APP_FM_EMITTER_EN
    if (true == file_dec_is_pause()) {
        led7_show_pause(hd);
    } else {
        u16 fre = fm_emitter_manage_get_fre();
        if (fre != 0) {
            led7_fm_show_freq(hd, fre);
        }
    }
#else
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
    if (get_le_audio_curr_role() == 2) {
        led7_show_le(hd);
    } else
#endif
        if (music_player_runing()) {
            u8 paly_status = music_file_get_player_status(get_music_file_player());
            if (paly_status == FILE_PLAYER_PAUSE) {
                led7_show_pause(hd);
            } else if (paly_status == FILE_PLAYER_START) {
                int play_second = music_file_get_cur_time(get_music_file_player());
                if (-1 == play_second) {
                    play_second = 0;
                }
                ui_led7_show_music_time(hd, (u16)play_second);
                led7_show_music_dev(hd);
                /* printf("sec = %d \n", play_second); */
            }
        } else {
            if (!get_music_le_audio_flag()) {
                led7_show_pause(hd);
            } else {
                printf("!!! %s %d\n", __FUNCTION__, __LINE__);
            }
        }
#endif
}


static int ui_music_user(void *hd, void *private, int menu, int arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {
    case MENU_FILENUM:
        led7_show_filenumber(hd, arg);
        break;
    case MENU_MUSIC_REPEATMODE:
        led7_show_repeat_mode(hd, arg);
        break;
    default:
        ret = false;
        break;
    }

    return ret;

}



const struct ui_dis_api music_main = {
    .ui      = UI_MUSIC_MENU_MAIN,
    .open    = ui_open_music,
    .ui_main = ui_music_main,
    .ui_user = ui_music_user,
    .close   = ui_close_music,
};

#endif
#endif
