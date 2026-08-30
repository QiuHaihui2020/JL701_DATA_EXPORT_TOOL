#ifndef _OLED_H_
#define _OLED_H_

#include "typedef.h"

/* ============================================================= */
/* OLED 通信接口选择 (2选1): IIC 或 SPI                            */
/*   OLED_INTERFACE_IIC : 软件/硬件 IIC 接口 OLED (oled.c)        */
/*   OLED_INTERFACE_SPI : 硬件 SPI 接口 OLED (oled_spi.c)         */
/* 在 app_config.h 或编译选项中定义 OLED_INTERFACE 选择其一       */
/* ============================================================= */
#define OLED_INTERFACE_IIC  0
#define OLED_INTERFACE_SPI  1
#ifndef OLED_INTERFACE
#define OLED_INTERFACE      OLED_INTERFACE_SPI
#endif

/* ===================== 公共定义 (IIC/SPI 通用) ===================== */
#define	Brightness	0xFF
#define X_WIDTH 	128
#define Y_WIDTH 	64

void OLED_WrCmd(u8 ucCmd);
void OLED_WrDat(u8 ucData);
//OLED 设置坐标
void OLED_Set_Pos(unsigned char x, unsigned char y);
//OLED全屏
void OLED_Fill(unsigned char bmp_dat);
//OLED复位
void OLED_CLS(void);
void Set_Address(u8 page, u8 column);
void OLED_32x32_Chinese(u8 page, u8 column, const u8 *dp);
//OLED初始化
void OLED_Init(void);
//显示6*8一组标准ASCII字符串    显示的坐标（x,y），y为页范围0～7
void OLED_P6x8Str(unsigned char x, unsigned char y, char ch[]);
//显示8*16一组标准ASCII字符串    显示的坐标（x,y），y为页范围0～7
void OLED_P8x16Str(unsigned char x, unsigned char y, char ch[]);
//显示16*16点阵  显示的坐标（x,y），y为页范围0～7
void OLED_P16x16Ch(unsigned char x, unsigned char y, unsigned char N);
//显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, char BMP[]);


/* ===================== IIC 接口相关定义 ===================== */
#if (OLED_INTERFACE == OLED_INTERFACE_IIC)

#define OLED_PORT      JL_PORTA
#define RES_PIN        1
// #define SDIN_PIN       3
// #define SCLK_PIN       2

#define GPIO_SetBits(port, pin)     (port->OUT |=  BIT(pin))
#define GPIO_ResetBits(port, pin)   (port->OUT &= ~BIT(pin))

#define OLED_RES_Clr() GPIO_ResetBits(OLED_PORT, RES_PIN)//JL_PORTB->OUT &= ~BIT(8)	//RES
#define OLED_RES_Set() GPIO_SetBits(OLED_PORT, RES_PIN) //JL_PORTB->OUT |=  BIT(8)

#define OLED_RES_INIT()  {OLED_PORT->DIR &= ~BIT(RES_PIN); \
                         OLED_PORT->DIE |=  BIT(RES_PIN); \
                         OLED_PORT->PU  &= ~BIT(RES_PIN); \
                         OLED_PORT->PD  &= ~BIT(RES_PIN);}

/* ===================== SPI 接口相关定义 ===================== */
/* 引脚/平台数据配置在 oled_spi.c 中定义, 使用板级宏:
 *   TCFG_LCD_PIN_RESET / TCFG_LCD_PIN_CS / TCFG_LCD_PIN_DC
 *   TCFG_LCD_PIN_BL / TCFG_LCD_PIN_EN / TCFG_TFT_LCD_DEV_SPI_HW_NUM
 * (与 sdk_config.h 中 LCD_SPI_PLATFORM_DATA 配置一致)            */
#elif (OLED_INTERFACE == OLED_INTERFACE_SPI)

#define OLED_CMD   0
#define OLED_DATA 1

#endif /* OLED_INTERFACE */

#endif /*_OLED_H_*/
