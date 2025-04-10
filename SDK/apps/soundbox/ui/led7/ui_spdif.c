#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ui_spdif.data.bss")
#pragma data_seg(".ui_spdif.data")
#pragma const_seg(".ui_spdif.text.const")
#pragma code_seg(".ui_spdif.text")
#endif
#include "ui/ui_api.h"
#include "system/includes.h"

#if TCFG_APP_SPDIF_EN
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
void ui_spdif_temp_finsh(u8 menu)//子菜单被打断或者显示超时
{
    switch (menu) {
    default:
        break;
    }
}

static void led7_show_spdif(void *hd)
{
    LCD_API *dis = (LCD_API *)hd;
    dis->lock(1);
    dis->clear();
    dis->setXY(0, 0);
    dis->show_string((u8 *)" SP");
    /* dis->show_icon(LED7_USB); */
    dis->lock(0);
}




static void *ui_open_spdif(void *hd)
{
    /* ui_set_auto_reflash(500);//设置主页500ms自动刷新 */
    return NULL;
}

static void ui_close_spdif(void *hd, void *private)
{
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return ;
    }

    if (private) {
        free(private);
    }
}

static void ui_spdif_main(void *hd, void *private) //主界面显示
{
    if (!hd) {
        return;
    }
    led7_show_spdif(hd);
}


static int ui_spdif_user(void *hd, void *private, int menu, int arg)//子界面显示 //返回true不继续传递 ，返回false由common统一处理
{
    int ret = true;
    LCD_API *dis = (LCD_API *)hd;
    if (!dis) {
        return false;
    }
    switch (menu) {
    case MENU_SPDIF:
        led7_show_spdif(hd);
        break;
    default:
        ret = false;
        break;
    }

    return ret;

}



const struct ui_dis_api spdif_main = {
    .ui      = UI_SPDIF_MENU_MAIN,
    .open    = ui_open_spdif,
    .ui_main = ui_spdif_main,
    .ui_user = ui_spdif_user,
    .close   = ui_close_spdif,
};


#endif
#endif
