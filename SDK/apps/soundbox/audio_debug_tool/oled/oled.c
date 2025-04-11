#if 1

#include "oled.h"
#include "oledbmp.h"
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

    OLED_32x32_Chinese(1,48,&JL_LOGO[0][0]);
    OLED_P16x16Ch(30,5,1);//`显示杰理科技
	OLED_P16x16Ch(48,5,2);
	OLED_P16x16Ch(66,5,3);
	OLED_P16x16Ch(84,5,4);

}



/***************功能描述：显示6*8一组标准ASCII字符串	显示的坐标（x,y），y为页范围0～7****************/
void OLED_P6x8Str(unsigned char x, unsigned char y, unsigned char ch[])
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
void OLED_P8x16Str(unsigned char x, unsigned char y, unsigned char ch[])
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
void Draw_BMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1,unsigned char BMP[])
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


//---------------------------------------------------------//
#else

#include "oled.h"
#include "oledfont.h"
#include "bmp.h"
#include "asm/spi.h"

u16 BACK_COLOR, POINT_COLOR = BLACK;
#define OLED_USE_HW_SPI    0
static spi_dev spi1_hdl = 1;

            /* .di_pin = IO_PORTB_05, */
            /* .do_pin = IO_PORTB_07, */
            /* .clk_pin = IO_PORTB_06, */
const struct spi_platform_data spi1_p_data = {
    .port = 'A',
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 8000000,
    .role = SPI_ROLE_MASTER,
};

const struct spi_platform_data spi2_p_data = {
    .port = 'A',
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 1000000,
    .role = SPI_ROLE_SLAVE,
};

/******************************************************************************
      函数说明：OLED串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void OLED_Writ_Bus(u8 dat) 
{	
	u8 i;	
#if OLED_USE_HW_SPI
    spi_send_byte(spi1_hdl, dat);
#else
  OLED_CS_Clr();
  for(i=0;i<8;i++)
  {             
      OLED_SCLK_Clr();
      if(dat&0x80)
        OLED_SDIN_Set();
      else
        OLED_SDIN_Clr();
      OLED_SCLK_Set();
      dat <<= 1;
  }       
  OLED_CS_Set();
#endif
}



/******************************************************************************
      函数说明：OLED写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void OLED_WR_DATA8(u16 dat)
{
	/* OLED_DC_Set();//写数据 */
	OLED_Writ_Bus(dat);
}

void OLED_WR_DATA16(u16 dat)
{
	/* OLED_DC_Set();//写数据 */
	OLED_Writ_Bus(dat>>8);
	OLED_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：OLED写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void OLED_WR_REG(u8 dat)
{
	/* OLED_DC_Clr();//写命令 */
	OLED_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/


void OLED_Address_Set(u8 x1,u8 y1,u8 x2,u8 y2)
{
	OLED_WR_REG(0x15);//列地址设置
	OLED_WR_REG(x1);
	OLED_WR_REG(x2);
	OLED_WR_REG(0x75);//行地址设置
	OLED_WR_REG(y1);
	OLED_WR_REG(y2);
}

//OLED的初始化
void OLED_Init(void)
{
#if OLED_USE_HW_SPI
    spi_open(spi1_hdl);
#endif

    OLED_PORT_INIT();

	OLED_RES_Clr();
	delay(2000);
	OLED_RES_Set(); 
	
	OLED_WR_REG(0xAE);
	OLED_WR_REG(0xA0);
	OLED_WR_REG(0x72);
	OLED_WR_REG(0xA1);
	OLED_WR_REG(0x00);
	OLED_WR_REG(0xA2);
	OLED_WR_REG(0x00);
	OLED_WR_REG(0xA4);
	OLED_WR_REG(0xA8);
	OLED_WR_REG(0x3F);
	OLED_WR_REG(0xAD);
	OLED_WR_REG(0x8E);
	OLED_WR_REG(0xB0);
	OLED_WR_REG(0x0B);
	OLED_WR_REG(0xB1);
	OLED_WR_REG(0x31);
	OLED_WR_REG(0xB3);
	OLED_WR_REG(0xF0);
	OLED_WR_REG(0x8A);
	OLED_WR_REG(0x64);
	OLED_WR_REG(0x8B);
	OLED_WR_REG(0x78);
	OLED_WR_REG(0x8C);
	OLED_WR_REG(0x64);
	OLED_WR_REG(0xBB);
	OLED_WR_REG(0x3A);
	OLED_WR_REG(0xBE);
	OLED_WR_REG(0x3E);
	OLED_WR_REG(0x87);
	OLED_WR_REG(0x06);
	OLED_WR_REG(0x81);
	OLED_WR_REG(0x91);
	OLED_WR_REG(0x82);
	OLED_WR_REG(0x50);
	OLED_WR_REG(0x83);
	OLED_WR_REG(0x7D);
	OLED_WR_REG(0xAF);
}


/******************************************************************************
      函数说明：OLED清屏函数
      入口数据：无
      返回值：  无
******************************************************************************/
void OLED_Clear(u16 Color)
{
	u16 i,j;  	
	OLED_Address_Set(0,0,OLED_W-1,OLED_H-1);
    for(i=0;i<OLED_H;i++)
	  {
	     for (j=0;j<OLED_W;j++)
	     	{
        	OLED_WR_DATA16(Color);
	      }

	  }
}


/******************************************************************************
      函数说明：OLED显示汉字
      入口数据：x,y   起始坐标
                index 汉字的序号
                size  字号
      返回值：  无
******************************************************************************/
void OLED_ShowChinese(u16 x,u16 y,u8 index,u8 size,u16 color)
{  
	u8 i,j,x1=x;
	u8 *temp,size1;
	if(size==16){temp=Hzk16;}//选择字号
	if(size==32){temp=Hzk32;}
  OLED_Address_Set(x,y,x+size-1,y+size-1);//设置一个汉字的区域
  size1=size*size/8;//一个汉字所占的字节
	temp+=index*size1;//写入的起始位置
	for(j=0;j<size1;j++)
	{
		for(i=0;i<8;i++)
		{
		 	if(*temp&(1<<i))//从数据的低位开始读
			{
				OLED_DrawPoint(x,y,color);//点亮
			}
			else
			{
				OLED_DrawPoint(x,y,BACK_COLOR);
			}
			x++;
			if((x-x1)==size)
			{
				y++;
				x=x1;
			}
		}
		temp++;
	 }
}


/******************************************************************************
      函数说明：OLED显示汉字
      入口数据：x,y   起始坐标
      返回值：  无
******************************************************************************/
void OLED_DrawPoint(u16 x,u16 y,u16 color)
{
	OLED_Address_Set(x,y,x,y);//设置光标位置 
	OLED_WR_DATA16(color);
} 


/******************************************************************************
      函数说明：OLED画一个大的点
      入口数据：x,y   起始坐标
      返回值：  无
******************************************************************************/
void OLED_DrawPoint_big(u16 x,u16 y,u16 color)
{
	OLED_Fill(x-1,y-1,x+1,y+1,color);
} 


/******************************************************************************
      函数说明：在指定区域填充颜色
      入口数据：xsta,ysta   起始坐标
                xend,yend   终止坐标
      返回值：  无
******************************************************************************/
void OLED_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{          
	u16 i,j; 
	OLED_Address_Set(xsta,ysta,xend,yend);      //设置光标位置 
	for(i=ysta;i<=yend;i++)
	{													   	 	
		for(j=xsta;j<=xend;j++)
		{
			OLED_WR_DATA16(color);//设置光标位置 		
		}			
	} 					  	    
}


/******************************************************************************
      函数说明：画线
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
      返回值：  无
******************************************************************************/
void OLED_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		OLED_DrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}


/******************************************************************************
      函数说明：画矩形
      入口数据：x1,y1   起始坐标
                x2,y2   终止坐标
      返回值：  无
******************************************************************************/
void OLED_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	OLED_DrawLine(x1,y1,x2,y1,color);
	OLED_DrawLine(x1,y1,x1,y2,color);
	OLED_DrawLine(x1,y2,x2,y2,color);
	OLED_DrawLine(x2,y1,x2,y2,color);
}


/******************************************************************************
      函数说明：画圆
      入口数据：x0,y0   圆心坐标
                r       半径
      返回值：  无
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	int di;
	a=0;b=r;	  
	while(a<=b)
	{
		OLED_DrawPoint(x0-b,y0-a,color);             //3           
		OLED_DrawPoint(x0+b,y0-a,color);             //0           
		OLED_DrawPoint(x0-a,y0+b,color);             //1                
		OLED_DrawPoint(x0-a,y0-b,color);             //2             
		OLED_DrawPoint(x0+b,y0+a,color);             //4               
		OLED_DrawPoint(x0+a,y0-b,color);             //5
		OLED_DrawPoint(x0+a,y0+b,color);             //6 
		OLED_DrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}



void OLED_ShowChar(u16 x,u16 y,u8 num,u16 color)
{
	u8 pos,t,temp;
	u16 x1=x;
	if(x>OLED_W-16||y>OLED_H-16)return;	    //设置窗口		   
	num=num-' ';//得到偏移后的值
	OLED_Address_Set(x,y,x+8-1,y+16-1);      //设置光标位置 
		for(pos=0;pos<16;pos++)
		{
		    temp=asc2_1608[(u16)num*16+pos];		 //调用1608字体
			 for(t=0;t<8;t++)
		    {
		        if(temp&0x01)OLED_DrawPoint(x+t,y+pos,color);//画一个点
					  else OLED_DrawPoint(x+t,y+pos,BLACK);
		        temp>>=1;
		    }
		}
}


/******************************************************************************
      函数说明：显示字符串
      入口数据：x,y    起点坐标
                *p     字符串起始地址
      返回值：  无
******************************************************************************/
void OLED_ShowString(u16 x,u16 y,const u8 *p,u16 color)
{         
    while(*p!='\0')
    {       
        if(x>OLED_W-16){x=0;y+=16;}
        if(y>OLED_H-16){y=x=0;OLED_Clear(POINT_COLOR);}
        OLED_ShowChar(x,y,*p,color);
        x+=8;
        p++;
    }  
}


/******************************************************************************
      函数说明：显示数字
      入口数据：m底数，n指数
      返回值：  无
******************************************************************************/
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}


/******************************************************************************
      函数说明：显示数字
      入口数据：x,y    起点坐标
                num    要显示的数字
                len    要显示的数字个数
      返回值：  无
******************************************************************************/
void OLED_ShowNum(u16 x,u16 y,float num,u8 len,u16 color)
{         	
	u8 t,temp;
	u8 enshow=0;
	u16 num1;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			OLED_ShowChar(x+8*(len-2),y,'.',color);
			t++;
			len+=1;
		}
	 	OLED_ShowChar(x+8*t,y,temp+48,color);
	}
}

int pow1(int x, int n)
{
    int res = 1;
    while(n--)
    {
        res *= x;
    }

    return res;
}

void OLED_ShowInt(u16 x,u16 y,int num, u16 color)
{
    u8 cnt = 0, t = 0;
    int temp = num;
	int bit = 0;
    while(temp!=0){
        temp/=10;
        cnt++;
    }    
    for(; t<cnt; t++){
		bit = (num / pow1(10, cnt - t - 1))%10;
	 	OLED_ShowChar(x+8*t,y,bit+48,color);
    }
}

/******************************************************************************
      函数说明：显示40x40图片
      入口数据：pic 图片数据   pos 起点坐标
      返回值：  无
******************************************************************************/
void OLED_ShowPicture(u8 *pic, img_pos *pos, img_size *size)
{
	int i,j;
	OLED_Address_Set(pos->x, pos->y, pos->x + size->width - 1, pos->y + size->height - 1);

	for(i=0;i<(size->width*size->height);i++)
	{
		OLED_WR_DATA8(pic[i*2+1]);
		OLED_WR_DATA8(pic[i*2]);
	}			
}


void oled_fill_frame(u8 *frame, img_pos *pos, img_size *frame_size){
    OLED_ShowPicture((u8 *)frame, pos, frame_size);
}

#endif