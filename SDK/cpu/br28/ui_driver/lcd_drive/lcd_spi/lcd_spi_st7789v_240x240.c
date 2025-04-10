#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_spi_st7789v_240x240.data.bss")
#pragma data_seg(".lcd_spi_st7789v_240x240.data")
#pragma const_seg(".lcd_spi_st7789v_240x240.text.const")
#pragma code_seg(".lcd_spi_st7789v_240x240.text")
#endif
/*
** 包含board的头文件，确定baord里面开关的屏驱宏
*/
#include "app_config.h"


/*
** 驱动代码的宏开关
*/
//<<<[4-fire屏 240x240]>>>//
#if TCFG_LCD_SPI_ST7789V_ENABLE



/*
** 包含imd头文件，屏驱相关的变量和结构体都定义在imd.h
** 包含imb头文件，imb输出格式定义在imb.h
*/
#include "asm/imb.h"
#include "asm/imd.h"
#include "includes.h"
#include "ui/ui_api.h"

#define LCD_DRIVE_CONFIG  SPI_4WIRE_RGB565_1T8B

#define SCR_X 0
#define SCR_Y 0
#define SCR_W 240
#define SCR_H 240
#define LCD_W 240
#define LCD_H 240
#define LCD_BLOCK_W 240
#define LCD_BLOCK_H 40
#define BUF_NUM 2

void lcd_spi_st7789v_set_pwr()
{
    /* 作为st7789v屏的电源初始信号,对应屏上LCD_PWR引脚,IO可换 */
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), 0);
    os_time_dly(2);
}

/*
** 初始化代码
*/
static const u8 lcd_cmd_t[] ALIGNED(4) = {
    _BEGIN_, 0x01, _END_,				// soft reset
    _BEGIN_, REGFLAG_DELAY, 120, _END_,	// delay 120ms
    _BEGIN_, 0x11, _END_,				// sleep out
    _BEGIN_, REGFLAG_DELAY, 120, _END_,
    _BEGIN_, 0x36, 0x00, _END_,

    _BEGIN_, 0x44, 0, 0, _END_,
    _BEGIN_, 0x35, 0x00, _END_, //TE V-BlancKing only

    /* #if (OUT_FORMAT == FORMAT_RGB565) */
    _BEGIN_, 0x3A, 0x05, _END_,

    _BEGIN_, 0xB2, 0x1f, 0x1f, 0x00, 0x33, 0x33, _END_,
    _BEGIN_, 0xB7, 0x00, _END_,
    _BEGIN_, 0xBB, 0x3f, _END_,
    _BEGIN_, 0xC0, 0x2c, _END_,
    _BEGIN_, 0xC2, 0x01, _END_,
    _BEGIN_, 0xC3, 0x0D, _END_,
    _BEGIN_, 0xC4, 0x20, _END_,
    _BEGIN_, 0xC6, 0x13, _END_,
    _BEGIN_, 0xD0, 0xA4, 0xA1, _END_,
    _BEGIN_, 0xD6, 0xA1, _END_,
    _BEGIN_, 0xE0, 0xf0, 0x03, 0x09, 0x03, 0x03, 0x10, 0x2d, 0x43, 0x3f, 0x33, 0x0D, 0x0e, 0x29, 0x32, _END_,
    _BEGIN_, 0xE1, 0xf0, 0x0c, 0x10, 0x0e, 0x0e, 0x0a, 0x2d, 0x33, 0x45, 0x3a, 0x14, 0x19, 0x31, 0x37, _END_,
    _BEGIN_, 0X21, _END_,
    _BEGIN_, 0X29, _END_,
    _BEGIN_, 0X2A, 0x00, 0x00, 0x00, 0xEF, _END_,
    _BEGIN_, 0X2B, 0x00, 0x00, 0x00, 0xEF, _END_,
    _BEGIN_, REGFLAG_DELAY, 20, _END_,
    _BEGIN_, 0X2C, _END_,
    _BEGIN_, REGFLAG_DELAY, 20, _END_,
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

REGISTER_LCD_DEVICE_NEW(st7789) = {
    .logo = "st7789v",
    .row_addr_align		= 1,
    .column_addr_align	= 1,

    .lcd_cmd = (void *) &lcd_cmd_t,
    .cmd_cnt = sizeof(lcd_cmd_t) / sizeof(lcd_cmd_t[0]),
    .param   = (void *) &param_t,

    .reset			= NULL,	// 没有特殊的复位操作，用内部普通复位函数即可
    .backlight_ctrl = NULL, //lcd_backlight_ctrl,
    .entersleep     = lcd_entersleep,
    .exitsleep      = lcd_exitsleep,
};


#endif


