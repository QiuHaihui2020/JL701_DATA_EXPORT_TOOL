
#include "app_config.h"

/*
** 驱动代码的宏开关
*/
#if TCFG_LCD_SPI_GC9B71_ENABLE

/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
** 包含imb头文件，imb输出格式定义在imb.h
*/
#include "asm/imb.h"
#include "asm/imd.h"
#include "includes.h"
#include "ui/ui_api.h"

#define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE0_1T8B
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE1_1T2B */
/* #define LCD_DRIVE_CONFIG                    QSPI_RGB565_SUBMODE2_1T2B */

#define SCR_X 0
#define SCR_Y 0
#define SCR_W 320
#define SCR_H 386
#define LCD_W 320
#define LCD_H 386
#define LCD_BLOCK_W 320
#define LCD_BLOCK_H 40
#define BUF_NUM 2


/*
** 初始化代码
*/
static const u8 lcd_cmd_t[] ALIGNED(4) = {
    _BEGIN_, 0xfe, _END_,
    _BEGIN_, 0xef, _END_,
    _BEGIN_, 0x80, 0x11, _END_,
    _BEGIN_, 0x81, 0x70, _END_,
    _BEGIN_, 0x82, 0x09, _END_,
    _BEGIN_, 0x83, 0x03, _END_,
    _BEGIN_, 0x84, 0x62, _END_,
    _BEGIN_, 0x89, 0x18, _END_,
    _BEGIN_, 0x8a, 0x40, _END_,
    _BEGIN_, 0x8b, 0x0a, _END_,

    _BEGIN_, 0x3a, 0x55, _END_,
    _BEGIN_, 0x36, 0x40, _END_,
    _BEGIN_, 0xec, 0x07, _END_,
    _BEGIN_, 0x74, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, _END_,

    _BEGIN_, 0x98, 0x3e, _END_,
    _BEGIN_, 0x99, 0x3e, _END_,
    _BEGIN_, 0xa1, 0x01, 0x04, _END_,
    _BEGIN_, 0xa2, 0x01, 0x04, _END_,

    _BEGIN_, 0xcb, 0x02, _END_,
    _BEGIN_, 0x7c, 0xb6, 0x24, _END_,
    _BEGIN_, 0xac, 0x74, _END_,
    _BEGIN_, 0xf6, 0x80, _END_,
    _BEGIN_, 0xb5, 0x09, 0x09, _END_,
    _BEGIN_, 0xeb, 0x01, 0x81, _END_,

    _BEGIN_, 0x60, 0x38, 0x06, 0x13, 0x56, _END_,
    _BEGIN_, 0x63, 0x38, 0x08, 0x13, 0x56, _END_,
    _BEGIN_, 0x61, 0x3b, 0x1b, 0x58, 0x38, _END_,
    _BEGIN_, 0x62, 0x3b, 0x1b, 0x58, 0x38, _END_,
    _BEGIN_, 0x64, 0x38, 0x0a, 0x73, 0x16, 0x13, 0x56, _END_,
    _BEGIN_, 0x66, 0x38, 0x0b, 0x73, 0x17, 0x13, 0x56, _END_,
    _BEGIN_, 0x68, 0x00, 0x0b, 0x22, 0x0b, 0x22, 0x1c, 0x1c, _END_,
    _BEGIN_, 0x69, 0x00, 0x0b, 0x26, 0x0b, 0x26, 0x1c, 0x1c, _END_,

    _BEGIN_, 0x6a, 0x15, 0x00, _END_,

    _BEGIN_, 0x6e, 0x08, 0x02, 0x1a, 0x00, 0x12, 0x12, 0x11, 0x11, 0x14, 0x14, 0x13, 0x13, 0x04, 0x19, 0x1e, 0x1d, 0x1d, 0x1e, 0x19, 0x04, 0x0b, 0x0b, 0x0c, 0x0c, 0x09, 0x09, 0x0a, 0x0a, 0x00, 0x1a, 0x01, 0x07, _END_,

    _BEGIN_, 0x6c, 0xcc, 0x0c, 0xcc, 0x84, 0xcc, 0x04, 0x50, _END_,
    _BEGIN_, 0x7d, 0x72, _END_,
    _BEGIN_, 0x70, 0x02, 0x03, 0x09, 0x07, 0x09, 0x03, 0x09, 0x07, 0x09, 0x03, _END_,
    _BEGIN_, 0x90, 0x06, 0x06, 0x05, 0x06, _END_,
    _BEGIN_, 0x93, 0x45, 0xff, 0x00, _END_,

    _BEGIN_, 0xc3, 0x15, _END_,
    _BEGIN_, 0xc4, 0x36, _END_,
    _BEGIN_, 0xc9, 0x3d, _END_,

    _BEGIN_, 0xf0, 0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29, _END_,
    _BEGIN_, 0xf2, 0x47, 0x07, 0x0a, 0x0a, 0x00, 0x29, _END_,
    _BEGIN_, 0xf1, 0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f, _END_,
    _BEGIN_, 0xf3, 0x42, 0x91, 0x10, 0x2d, 0x2f, 0x6f, _END_,

    _BEGIN_, 0xf9, 0x30, _END_,
    _BEGIN_, 0xbe, 0x11, _END_,
    _BEGIN_, 0xfb, 0x00, 0x00, _END_,

    _BEGIN_, 0x84, 0x32, _END_,// B4 EN
    _BEGIN_, 0xB4, 0x0A, _END_,// TE Width
    _BEGIN_, 0x35, 0x00, _END_,		// 开启TE
    _BEGIN_, 0x44, 0x00, 0xa0, _END_,		// TE configure


    _BEGIN_, 0x11, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x29, _END_,
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
};



// 配置参数
struct imd_param param_t = {
    .scr_x    = SCR_X,
    .scr_y	  = SCR_Y,
    .scr_w	  = SCR_W,
    .scr_h	  = SCR_H,

    // imb配置相关
    .in_width  = SCR_W,
    .in_height = SCR_H,
    .in_format = OUTPUT_FORMAT_RGB565,//-1,

    // 屏幕相关
    .lcd_width  = LCD_W,
    .lcd_height = LCD_H,
    .lcd_type   = LCD_TYPE_SPI,

    .buffer_num  = BUF_NUM,
    .buffer_size = LCD_BLOCK_W * LCD_BLOCK_H * 2,
    .fps = 60,

    .spi = {
        .spi_mode = SPI_IF_MODE(LCD_DRIVE_CONFIG),
        .pixel_type = PIXEL_TYPE(LCD_DRIVE_CONFIG),
        .out_format = OUT_FORMAT(LCD_DRIVE_CONFIG),
        .port		= SPI_PORTA,
        .spi_dat_mode = SPI_MODE_UNIDIR,
    },

    .debug_mode_en = false,
    .debug_mode_color = 0x0000ff,

    .te_en = false,
};


/*
** 设置lcd进入睡眠
*/
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

REGISTER_LCD_DEVICE_NEW(gc9b71) = {
    .logo = "gc9b71",
    .row_addr_align		= 2,
    .column_addr_align	= 2,

    .lcd_cmd = (void *) &lcd_cmd_t,
    .cmd_cnt = sizeof(lcd_cmd_t) / sizeof(lcd_cmd_t[0]),
    .param   = (void *) &param_t,

    .reset			= NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL, //lcd_backlight_ctrl,
    .entersleep     = lcd_entersleep,
    .exitsleep      = lcd_exitsleep,
};


#endif


