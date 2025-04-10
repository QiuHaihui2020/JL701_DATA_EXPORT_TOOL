#ifndef _AUD_PDM_H_
#define _AUD_PDM_H_

#include "generic/typedef.h"

#define PLNK_RESET()	do {JL_PLNK->CON = 0; JL_PLNK->ADR = 0; JL_PLNK->LEN = 0; JL_PLNK->DOR = 0; JL_PLNK->CON1;} while(0)


#define PLNK_CLK_EN(x)		SFR(JL_ASS->CLK_CON, 1, 1, x)

#define PLNK_CH0_EN(x)		SFR(JL_PLNK->CON, 0, 1, x)
#define PLNK_CH0_MIC_SEL(x)	SFR(JL_PLNK->CON, 1, 1, x)
#define PLNK_CH0_MD(x)		SFR(JL_PLNK->CON, 2, 2, x)

#define PLNK_CH1_EN(x)		SFR(JL_PLNK->CON, 4, 1, x)
#define PLNK_CH1_MIC_SEL(x)	SFR(JL_PLNK->CON, 5, 1, x)
#define PLNK_CH1_MD(x)		SFR(JL_PLNK->CON, 6, 2, x)

#define PLNK_IE(x)			SFR(JL_PLNK->CON, 8, 2, x)
#define PLNK_USING_BUF(x)	(JL_PLNK->CON & BIT(9))
#define PLNK_PND_CLR()		(JL_PLNK->CON |= BIT(14))
#define PLNK_PND_IS(x)		(JL_PLNK->CON & BIT(15))

#define PLNK_DMA_LEN(x)		(JL_PLNK->LEN = x)
#define PLNK_BUF_SET(x)		(JL_PLNK->ADR = x)

#define PLNK_DOR_SET(x)		(JL_PLNK->DOR = x)
#define PLNK_SCLK_DIV(x)	SFR(JL_ASS->CLK_CON, 2, 5, x)

#define PLNK_CH0_SCALE(x)	SFR(JL_PLNK->CON1, 0, 3, x)
#define PLNK_CH0_SHIFT(x)	SFR(JL_PLNK->CON1, 3, 5, x)
#define PLNK_CH0_ORDER(x)	SFR(JL_PLNK->CON1, 8, 2, x)
#define PLNK_CH0_DFDLY(x)	SFR(JL_PLNK->CON1, 10, 1, x)
#define PLNK_CH0_SHDIR(x)	SFR(JL_PLNK->CON1, 11, 1, x)

#define PLNK_CH1_SCALE(x)	SFR(JL_PLNK->CON1, 16, 3, x)
#define PLNK_CH1_SHIFT(x)	SFR(JL_PLNK->CON1, 19, 5, x)
#define PLNK_CH1_ORDER(x)	SFR(JL_PLNK->CON1, 24, 2, x)
#define PLNK_CH1_DFDLY(x)	SFR(JL_PLNK->CON1, 26, 1, x)
#define PLNK_CH1_SHDIR(x)	SFR(JL_PLNK->CON1, 27, 1, x)

typedef enum {
    DIGITAL_MIC_DATA,
    ANALOG_MIC_DATA,
} PLNK_MIC_SEL;

/*通道0输入模式选择*/
typedef enum {
    DATA0_SCLK_RISING_EDGE,
    DATA0_SCLK_FALLING_EDGE,
    DATA1_SCLK_RISING_EDGE,
    DATA1_SCLK_FALLING_EDGE,
} PLNK_CH_MD;

struct plnk_ch_cfg {
    u8 en;
    PLNK_CH_MD mode;
    PLNK_MIC_SEL mic_type;
};

struct plnk_data_cfg {
    u8 en;
    u8 io;
};

typedef struct _PLNK_PARM {
    u8 sclk_io;						//时钟IO
    u32 sclk_fre;					//时钟频率推荐2M
    struct plnk_ch_cfg ch_cfg[2];		//通道内部配置
    struct plnk_data_cfg data_cfg[2];		//数据IO配置
    u8 ch_num;		/*使能多少个通道*/
    u32 sr;			/*采样率*/
    u32 dma_len;	/*一次中断byte数*/
    void *buf;
    void (*isr_cb)(void *priv, void *buf, u32 len);
    void *private_data;			//音频私有数据
} PLNK_PARM;

void *plnk_init(void *hw_plink);
void plnk_start(void *hw_plink);
void plnk_stop(void *hw_plink);
void plnk_uninit(void *hw_plink);


#endif/*_AUD_PDM_H_*/
