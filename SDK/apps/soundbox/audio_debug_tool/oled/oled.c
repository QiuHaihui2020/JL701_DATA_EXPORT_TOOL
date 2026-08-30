#include "oled.h"

#if (OLED_INTERFACE == OLED_INTERFACE_IIC)


//#include "oledbmp.h"
#include "oledfont.h"
#include "clock.h"

// #define _IIC_USE_HW//硬件iic

#include "iic_api.h"
#define iic_dev 0
#define DELAY_CNT 0
#define delay delay_nops

void OLED_WrCmd(u8 ucCmd)
{
     int i;
     u32 retry;
     int ret;
     u32 tx_len;
     int iic = iic_dev;
     retry = 100;
     do {
        iic_start(iic);
        ret = iic_tx_byte(iic, 0X78);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*         continue; */
        /*     } else { */
        /*          goto __exit; */
        /*     } */
        /* } */
        delay(DELAY_CNT);
        ret = iic_tx_byte(iic, 0X00);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*     continue; */
        /*     } else { */
        /*         goto __exit; */
        /*     } */
        /* } */
        delay(DELAY_CNT);
        ret = iic_tx_byte(iic, ucCmd);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*     continue; */
        /*     } else { */
        /*         goto __exit; */
        /*     } */
        /* } */
        iic_stop(iic);
        delay(DELAY_CNT);
     } while (0);

     __exit:
    iic_stop(iic);
}


void OLED_WrDat(u8 ucData)
{
     int i;
     u32 retry;
     int ret;
     u32 tx_len;
     int iic = iic_dev;
     retry = 100;
     do {
        iic_start(iic);
        ret = iic_tx_byte(iic, 0X78);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*         continue; */
        /*     } else { */
        /*         goto __exit; */
        /*     } */
        /* } */
        delay(DELAY_CNT);
        ret = iic_tx_byte(iic, 0X40);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*     continue; */
        /*     } else { */
        /*         goto __exit; */
        /*     } */
        /* } */
        delay(DELAY_CNT);
        ret = iic_tx_byte(iic, ucData);
        /* if (!ret) { */
        /*     if (--retry) { */
        /*     continue; */
        /*     } else { */
        /*         goto __exit; */
        /*     } */
        /* } */
        iic_stop(iic);
        delay(DELAY_CNT);
     } while (0);

     __exit:
    iic_stop(iic);
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
/*********************OLED复位************************************/
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
    OLED_RES_INIT();
    OLED_RES_Set();

    iic_init(iic_dev, get_iic_config(iic_dev));
	delay(500);//初始化之前的延时很重要！
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

#endif
