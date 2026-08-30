#include "oled.h"
#if (OLED_INTERFACE == OLED_INTERFACE_SPI)

/* 硬件 SPI 接口 OLED 驱动 (SSD1306 128x64, 4线SPI)
 * 参考: apps/common/ui/lcd_drive/spi_oled_drive.c
 * 引脚配置复用板级 LCD_SPI_PLATFORM_DATA:
 *   TCFG_LCD_PIN_RESET / CS / DC / BL / EN, TCFG_TFT_LCD_DEV_SPI_HW_NUM
 * 与 oled.c(IIC) 二选一编译, 对外提供相同的 OLED_xxx 接口
 */

#include "oledfont.h"
#include "app_config.h"
#include "system/includes.h"
#include "spi.h"
#include "clock.h"

/* ---------- 引脚/SPI 配置回退 (与 board/br28/sdk_config.h 一致) ---------- */
/* 板级已定义时使用板级配置, 否则使用以下默认值, 保证可独立编译 */
#ifndef TCFG_LCD_PIN_RESET
#define TCFG_LCD_PIN_RESET              IO_PORTC_03
#endif
#ifndef TCFG_LCD_PIN_CS
#define TCFG_LCD_PIN_CS                 IO_PORTC_05
#endif
#ifndef TCFG_LCD_PIN_DC
#define TCFG_LCD_PIN_DC                 IO_PORTC_04
#endif
#ifndef TCFG_LCD_PIN_BL
#define TCFG_LCD_PIN_BL                 IO_PORTC_06
#endif
#ifndef TCFG_LCD_PIN_EN
#define TCFG_LCD_PIN_EN                 NO_CONFIG_PORT
#endif
#ifndef TCFG_TFT_LCD_DEV_SPI_HW_NUM
#define TCFG_TFT_LCD_DEV_SPI_HW_NUM     HW_SPI1
#endif

#define OLED_SPI_DEV                    TCFG_TFT_LCD_DEV_SPI_HW_NUM

/* ---------- GPIO 控制辅助 ---------- */
static void oled_spi_pin_output(u32 pin)
{
    if (pin == (u32)NO_CONFIG_PORT) {
        return;
    }
    gpio_set_mode(IO_PORT_SPILT(pin), PORT_OUTPUT_LOW);
}

static void oled_spi_pin_write(u32 pin, u8 val)
{
    if (pin == (u32)NO_CONFIG_PORT) {
        return;
    }
    gpio_write(pin, val);
}

#define OLED_SPI_CS_L()    oled_spi_pin_write(TCFG_LCD_PIN_CS, 0)
#define OLED_SPI_CS_H()    oled_spi_pin_write(TCFG_LCD_PIN_CS, 1)
#define OLED_SPI_DC_L()    oled_spi_pin_write(TCFG_LCD_PIN_DC, 0)   //命令
#define OLED_SPI_DC_H()    oled_spi_pin_write(TCFG_LCD_PIN_DC, 1)   //数据
#define OLED_SPI_RES_L()   oled_spi_pin_write(TCFG_LCD_PIN_RESET, 0)
#define OLED_SPI_RES_H()   oled_spi_pin_write(TCFG_LCD_PIN_RESET, 1)
#define OLED_SPI_BL_H()    oled_spi_pin_write(TCFG_LCD_PIN_BL, 1)
#define OLED_SPI_EN_H()    oled_spi_pin_write(TCFG_LCD_PIN_EN, 1)

/* ---------- SPI 读写 ---------- */
/* CS拉低, DC选择命令/数据, 发送一字节, CS拉高 */
static void oled_spi_wr_byte(u8 dat, u8 cmd)
{
    OLED_SPI_CS_L();
    if (cmd) {
        OLED_SPI_DC_H();   //数据
    } else {
        OLED_SPI_DC_L();   //命令
    }
    spi_send_byte(OLED_SPI_DEV, dat);
    OLED_SPI_CS_H();
}

void OLED_WrCmd(u8 ucCmd)
{
    oled_spi_wr_byte(ucCmd, OLED_CMD);
}

void OLED_WrDat(u8 ucData)
{
    oled_spi_wr_byte(ucData, OLED_DATA);
}

/*********************OLED 设置坐标************************************/
void OLED_Set_Pos(unsigned char x, unsigned char y)
{
	OLED_WrCmd(0xb0+y);
	OLED_WrCmd(((x&0xf0)>>4)|0x10);
	OLED_WrCmd((x&0x0f)|0x01);
}
/*********************OLED全屏************************************/
void OLED_Fill(unsigned char bmp_dat)
{
	unsigned char y,x;
	for(y=0;y<8;y++)
	{
		OLED_WrCmd(0xb0+y);
		OLED_WrCmd(0x01);
		OLED_WrCmd(0x10);
		for(x=0;x<X_WIDTH;x++)
		OLED_WrDat(bmp_dat);
	}
}
/*********************OLED复位(清屏)************************************/
void OLED_CLS(void)
{
	unsigned char y,x;
	for(y=0;y<8;y++)
	{
		OLED_WrCmd(0xb0+y);
		OLED_WrCmd(0x01);
		OLED_WrCmd(0x10);
		for(x=0;x<X_WIDTH;x++)
		OLED_WrDat(0);
	}
}
void Set_Address(u8 page,u8 column)
{
	OLED_WrCmd(0xb0 + column);
	OLED_WrCmd(((page & 0xf0) >> 4) | 0x10);
	OLED_WrCmd((page & 0x0f) | 0x00);
}
void OLED_32x32_Chinese(u8 page,u8 column, const u8 *dp)
{
    u8 i,j;
    for(j=4;j>0;j--)
	{
		Set_Address(column,page);
		for (i=0;i<32;i++)
		{
			OLED_WrDat(*dp);
			dp++;
		}
		page++;
	}
}

/*********************OLED初始化************************************/
void OLED_Init(void)
{
    /* 1. 控制引脚初始化 (CS/DC/RES/BL/EN 设为输出) */
    oled_spi_pin_output(TCFG_LCD_PIN_CS);
    oled_spi_pin_output(TCFG_LCD_PIN_DC);
    oled_spi_pin_output(TCFG_LCD_PIN_RESET);
    oled_spi_pin_output(TCFG_LCD_PIN_BL);
    oled_spi_pin_output(TCFG_LCD_PIN_EN);
    OLED_SPI_CS_H();
    OLED_SPI_DC_H();

    /* 2. 使能电源(若配置了EN) */
    OLED_SPI_EN_H();

    /* 3. 打开硬件SPI (引脚/波特率取板级 get_hw_spi_config) */
    spi_open(OLED_SPI_DEV, get_hw_spi_config(OLED_SPI_DEV));

    /* 4. 硬件复位 */
    OLED_SPI_RES_H();
    mdelay(10);
    OLED_SPI_RES_L();
    mdelay(10);
    OLED_SPI_RES_H();
    mdelay(50);

    /* 5. 开背光/显示电源 */
    OLED_SPI_BL_H();

    /* 6. SSD1306 初始化命令序列 (与IIC版本一致, 页地址模式) */
	OLED_WrCmd(0xae);//--turn off oled panel
	OLED_WrCmd(0x00);//---set low column address
	OLED_WrCmd(0x10);//---set high column address
	OLED_WrCmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
    OLED_WrCmd(0xB0);
	OLED_WrCmd(0x81);//--set contrast control register
	OLED_WrCmd(Brightness); // Set SEG Output Current Brightness
	OLED_WrCmd(0xa1);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WrCmd(0xa6);//--set normal display
	OLED_WrCmd(0xa8);//--set multiplex ratio(1 to 64)
	OLED_WrCmd(0x3f);//--1/64 duty
    OLED_WrCmd(0xc8);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WrCmd(0xd3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WrCmd(0x00);//-not offset

	OLED_WrCmd(0xd5);//--set display clock divide ratio/oscillator frequency
	OLED_WrCmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec

	OLED_WrCmd(0xD8);//set area color mode off
	OLED_WrCmd(0x05);//

	OLED_WrCmd(0xd9);//--set pre-charge period
	OLED_WrCmd(0xf1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock

	OLED_WrCmd(0xda);//--set com pins hardware configuration
	OLED_WrCmd(0x12);

	OLED_WrCmd(0xdb);//--set vcomh
    OLED_WrCmd(0x30);

	OLED_WrCmd(0x40);//Set VCOM Deselect Level
	OLED_WrCmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WrCmd(0x02);//

	OLED_WrCmd(0x8d);//--set Charge Pump enable/disable
	OLED_WrCmd(0x14);//--set(0x10) disable

	OLED_WrCmd(0xa4);// Disable Entire Display On (0xa4/0xa5)
	OLED_WrCmd(0xa6);// Disable Inverse Display On (0xa6/a7)
	OLED_WrCmd(0xaf);//--turn on oled panel

	OLED_Fill(0x00); //初始清屏
	OLED_Set_Pos(0,0);
}

/***************功能描述：显示6*8一组标准ASCII字符串	显示的坐标（x,y），y为页范围0～7****************/
void OLED_P6x8Str(unsigned char x, unsigned char y, char ch[])
{
	unsigned char c=0,i=0,j=0;
	while (ch[j]!='\0')
	{
		c =ch[j]-32;
		if(x>126){x=0;y++;}
		OLED_Set_Pos(x,y);
		for(i=0;i<6;i++)
		OLED_WrDat(F6x8[c][i]);
		x+=6;
		j++;
	}
}
/*******************功能描述：显示8*16一组标准ASCII字符串	 显示的坐标（x,y），y为页范围0～7****************/
void OLED_P8x16Str(unsigned char x, unsigned char y, char ch[])
{
	unsigned char c=0,i=0,j=0;
	while (ch[j]!='\0')
	{
		c =ch[j]-32;
		if(x>120){x=0;y++;}
		OLED_Set_Pos(x,y);
		for(i=0;i<8;i++)
		OLED_WrDat(F8X16[c*16+i]);
		OLED_Set_Pos(x,y+1);
		for(i=0;i<8;i++)
		OLED_WrDat(F8X16[c*16+i+8]);
		x+=8;
		j++;
	}
}
/*****************功能描述：显示16*16点阵  显示的坐标（x,y），y为页范围0～7****************************/
void OLED_P16x16Ch(unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm=0;
	unsigned int adder=32*N;
	OLED_Set_Pos(x , y);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
	OLED_Set_Pos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
}

// Parameters     : x0,y0 -- 起始点坐标(x0:0~127, y0:0~7); x1,y1 -- 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0) y=y1/8;
  else y=y1/8+1;
	for(y=y0;y<y1;y++)
	{
		OLED_Set_Pos(x0,y);
    for(x=x0;x<x1;x++)
	    {
	    	OLED_WrDat(BMP[j++]);
	    }
	}
}

#endif /* OLED_INTERFACE_SPI */
