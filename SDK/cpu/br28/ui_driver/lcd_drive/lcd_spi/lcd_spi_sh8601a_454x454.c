/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"

/*
** 驱动代码的宏开关
*/
//<<<[qspi屏 454x454]>>>//
#if TCFG_LCD_SPI_SH8601A_ENABLE

/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE0_1T8B */
#define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE1_1T2B
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE2_1T2B */

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
*/
#include "asm/imd.h"
#include "asm/imb.h"
#include "includes.h"
#include "ui/ui_api.h"

#define SCR_X 0
#define SCR_Y 0
#define SCR_W 454
#define SCR_H 454
#define LCD_W 454
#define LCD_H 454
#define LCD_BLOCK_W 454
#define LCD_BLOCK_H 40
#define BUF_NUM 2

/*
** 初始化代码
*/
static const u8 lcd_spi_sh8601a_cmd_list[] ALIGNED(4) = {
    _BEGIN_, 0xc0, 0x5a, 0x5a, _END_,
    _BEGIN_, 0xc1, 0x5a, 0x5a, _END_,
    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_CONFIRM, 0x0a, 0x98, 100, _END_,

    _BEGIN_, 0x3A, 0x55, _END_,

    _BEGIN_, 0x44, 0, 0x10, _END_,
    _BEGIN_, 0x35, 0x00, _END_,
    _BEGIN_, 0xb0, 0x16, _END_,
    _BEGIN_, 0xb1, 0x01, 0x05, 0x00, 0xa2, 0x00, 0xa7, 0x00, 0xa7, 0x00, _END_,
    _BEGIN_, 0x53, 0x28, _END_,
    _BEGIN_, 0xc4, 0x80, _END_,

    _BEGIN_, REGFLAG_DELAY, 25, _END_,
    _BEGIN_, 0x29, _END_,
    /* _BEGIN_, REGFLAG_DELAY, 20, _END_, */
    _BEGIN_, REGFLAG_CONFIRM, 0x0a, 0x9c, 100, _END_,
    _BEGIN_, 0xb1, 0xc0, _END_,

    _BEGIN_, 0xc0, 0xa5, 0xa5, _END_,
    _BEGIN_, 0xc1, 0xa5, 0xa5, _END_,
};


/*
** lcd电源控制
*/

struct imd_param lcd_spi_sh8601a_param = {
    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,

    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = OUTPUT_FORMAT_RGB565,

    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,

    .lcd_type = LCD_TYPE_SPI,

    .buffer_num = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,

    .fps = 60,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .port = SPI_PORTA,
        .spi_dat_mode = SPI_MODE_UNIDIR,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0xff0000,
};


static void lcd_entersleep(void)
{
    lcd_write_cmd(0x28, NULL, 0);
    lcd_write_cmd(0x10, NULL, 0);
    os_time_dly(2);
}


/*
** 设置lcd退出睡眠
*/
static void lcd_exitsleep(void)
{
    lcd_write_cmd(0x11, NULL, 0);
    os_time_dly(2);
    lcd_write_cmd(0x29, NULL, 0);
}


REGISTER_LCD_DEVICE_NEW(sh8601) = {
    .logo = "sh8601",
    .row_addr_align    = 1,
    .column_addr_align = 1,

    .lcd_cmd = (void *) &lcd_spi_sh8601a_cmd_list,
    .cmd_cnt = sizeof(lcd_spi_sh8601a_cmd_list) / sizeof(lcd_spi_sh8601a_cmd_list[0]),
    .param   = (void *) &lcd_spi_sh8601a_param,

    .reset = NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL,
    .entersleep     = lcd_entersleep,
    .exitsleep      = lcd_exitsleep,
};


#endif


