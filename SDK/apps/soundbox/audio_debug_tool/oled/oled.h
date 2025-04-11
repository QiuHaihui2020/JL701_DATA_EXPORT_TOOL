#ifndef _OLED_H_
#define _OLED_H_

#if 1

#include "typedef.h"


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

// #define OLED_SCLK_INIT()  {OLED_PORT->DIR &= ~BIT(SCLK_PIN); \
//                          OLED_PORT->DIE |=  BIT(SCLK_PIN); \
//                          OLED_PORT->PU  &= ~BIT(SCLK_PIN); \
//                          OLED_PORT->PD  &= ~BIT(SCLK_PIN);}
//
// #define OLED_SDIN_INIT()  {OLED_PORT->DIR &= ~BIT(SDIN_PIN); \
//                          OLED_PORT->DIE |=  BIT(SDIN_PIN); \
//                          OLED_PORT->PU  &= ~BIT(SDIN_PIN); \
//                          OLED_PORT->PD  &= ~BIT(SDIN_PIN);}
//

/*
#define iic_dev 0
#define iic_init(iic) soft_iic_init(iic)
#define iic_uninit(iic) soft_iic_uninit(iic)
#define iic_start(iic) soft_iic_start(iic)
#define iic_stop(iic) soft_iic_stop(iic)
#define iic_tx_byte(iic, byte) soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack) soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len) soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len) soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic) soft_iic_suspend(iic)
#define iic_resume(iic) soft_iic_resume(iic)
#define DELAY_CNT 0
*/


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
void Set_Address(u8 page,u8 column);
void OLED_32x32_Chinese(u8 page,u8 column, const u8 *dp);
//OLED初始化
void OLED_Init(void);
//显示6*8一组标准ASCII字符串    显示的坐标（x,y），y为页范围0～7
void OLED_P6x8Str(unsigned char x, unsigned char y, unsigned char ch[]);
//显示8*16一组标准ASCII字符串    显示的坐标（x,y），y为页范围0～7
void OLED_P8x16Str(unsigned char x, unsigned char y, unsigned char ch[]);
//显示16*16点阵  显示的坐标（x,y），y为页范围0～7
void OLED_P16x16Ch(unsigned char x, unsigned char y, unsigned char N);
//显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char BMP[]);


//-----------------------------------------------//
#else

#include "includes.h"	    

typedef struct{
    u8 x;
    u8 y;
}img_pos;

typedef struct{
    u8 width;
    u8 height;
}img_size;


#define OLED_PORT      JL_PORTB
#define CS_PIN         9
#define DC_PIN         -1
#define RES_PIN        8
#define SDIN_PIN       7
#define SCLK_PIN       6

#define GPIO_SetBits(port, pin)     (port->OUT |=  BIT(pin))
#define GPIO_ResetBits(port, pin)   (port->OUT &= ~BIT(pin))

//-----------------OLED端口定义----------------  					   

#define OLED_CS_Clr()  GPIO_ResetBits(OLED_PORT, CS_PIN)//JL_PORTB->OUT &= ~BIT(2)
#define OLED_CS_Set()  GPIO_SetBits(OLED_PORT, CS_PIN)//JL_PORTB->OUT |=  BIT(2)

#define OLED_RES_Clr() GPIO_ResetBits(OLED_PORT, RES_PIN)//JL_PORTB->OUT &= ~BIT(3)	//RES
#define OLED_RES_Set() GPIO_SetBits(OLED_PORT, RES_PIN) //JL_PORTB->OUT |=  BIT(3)

#define OLED_DC_Clr()  GPIO_ResetBits(OLED_PORT, DC_PIN)//JL_PORTB->OUT &= ~BIT(4)	//DC
#define OLED_DC_Set()  GPIO_SetBits(OLED_PORT, DC_PIN)  //_JL_PORTB->OUT |=  BIT(4)

#define OLED_SCLK_Clr() GPIO_ResetBits(OLED_PORT, SCLK_PIN)//JL_PORTB->OUT &= ~BIT(5)//CLK
#define OLED_SCLK_Set() GPIO_SetBits(OLED_PORT, SCLK_PIN)  //JL_PORTB->OUT |= BIT(5)

#define OLED_SDIN_Clr() GPIO_ResetBits(OLED_PORT, SDIN_PIN) // JL_PORTB->OUT &= ~BIT(6)	//DIN
#define OLED_SDIN_Set() GPIO_SetBits(OLED_PORT, SDIN_PIN)   //JL_PORTB->OUT |= BIT(6)

#define OLED_CS_INIT()  {OLED_PORT->DIR &= ~BIT(CS_PIN); \
                         OLED_PORT->DIE |=  BIT(CS_PIN); \
                         OLED_PORT->PU  &= ~BIT(CS_PIN); \
                         OLED_PORT->PD  &= ~BIT(CS_PIN);}

#define OLED_RES_INIT()  {OLED_PORT->DIR &= ~BIT(RES_PIN); \
                         OLED_PORT->DIE |=  BIT(RES_PIN); \
                         OLED_PORT->PU  &= ~BIT(RES_PIN); \
                         OLED_PORT->PD  &= ~BIT(RES_PIN);}

#define OLED_DC_INIT()  {OLED_PORT->DIR &= ~BIT(DC_PIN); \
                         OLED_PORT->DIE |=  BIT(DC_PIN); \
                         OLED_PORT->PU  &= ~BIT(DC_PIN); \
                         OLED_PORT->PD  &= ~BIT(DC_PIN);}

#define OLED_SCLK_INIT()  {OLED_PORT->DIR &= ~BIT(SCLK_PIN); \
                         OLED_PORT->DIE |=  BIT(SCLK_PIN); \
                         OLED_PORT->PU  &= ~BIT(SCLK_PIN); \
                         OLED_PORT->PD  &= ~BIT(SCLK_PIN);}

#define OLED_SDIN_INIT()  {OLED_PORT->DIR &= ~BIT(SDIN_PIN); \
                         OLED_PORT->DIE |=  BIT(SDIN_PIN); \
                         OLED_PORT->PU  &= ~BIT(SDIN_PIN); \
                         OLED_PORT->PD  &= ~BIT(SDIN_PIN);}

#define OLED_PORT_INIT()  {OLED_CS_INIT();  OLED_RES_INIT();  OLED_DC_INIT();  OLED_SCLK_INIT();  OLED_SDIN_INIT();}
                         
                         

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据
#define OLED_W    96
#define OLED_H    64

extern void OLED_Writ_Bus(u8 dat);
extern void OLED_WR_DATA8(u16 dat);
extern void OLED_WR_DATA16(u16 dat);
extern void OLED_WR_REG(u8 dat);
extern void OLED_Address_Set(u8 x1,u8 y1,u8 x2,u8 y2);
extern void OLED_Init(void); 
extern void OLED_Clear(u16 Color);
extern void OLED_ShowChinese(u16 x,u16 y,u8 index,u8 size,u16 color);
extern void OLED_DrawPoint(u16 x,u16 y,u16 color);
extern void OLED_DrawPoint_big(u16 x,u16 y,u16 color);
extern void OLED_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);
extern void OLED_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);
extern void OLED_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
extern void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color);
extern void OLED_ShowChar(u16 x,u16 y,u8 num,u16 color);
extern void OLED_ShowString(u16 x,u16 y,const u8 *p,u16 color);
extern u32 mypow(u8 m,u8 n);
extern void OLED_ShowNum(u16 x,u16 y,float num,u8 len,u16 color);
void OLED_ShowInt(u16 x,u16 y,int num, u16 color);
extern void OLED_ShowPicture(u8 *pic, img_pos *pos, img_size *size);
void oled_fill_frame(u8 *frame, img_pos *pos, img_size *frame_size);


//颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE           	 0x001F  
#define BRED             0XF81F
#define GRED 			       0XFFE0
#define GBLUE			       0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			     0XBC40 //棕色
#define BRRED 			     0XFC07 //棕红色
#define GRAY  			     0X8430 //灰色

//GUI颜色

#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
//以上三色为PANEL的颜色 
 
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			     0XC618 //浅灰色(PANNEL),窗体背景色

#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅棕蓝色(选择条目的反色)

#endif /*if 1*/

#endif /*_oled_h_*/ 