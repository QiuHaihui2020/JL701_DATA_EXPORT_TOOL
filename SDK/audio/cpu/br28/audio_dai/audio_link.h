#ifndef _AUDIO_LINK_H_
#define _AUDIO_LINK_H_

#include "generic/typedef.h"





#define ALINK_SEL(module, reg)             (((JL_ALNK_TypeDef    *)(((u8 *)JL_ALNK0) + module*0))->reg)

#define ALNK_CON_RESET(module)	do {ALINK_SEL(module, CON0) = 0; ALINK_SEL(module, CON1) = 0; ALINK_SEL(module, CON2) = 0; ALINK_SEL(module, CON3) = 0;} while(0)
#define ALNK_HWPTR_RESET(module)	do {ALINK_SEL(module, HWPTR0) = 0; ALINK_SEL(module, HWPTR1) = 0; ALINK_SEL(module, HWPTR2) = 0; ALINK_SEL(module, HWPTR3) = 0;} while(0)
#define ALNK_SWPTR_RESET(module)	do {ALINK_SEL(module, SWPTR0) = 0; ALINK_SEL(module, SWPTR1) = 0; ALINK_SEL(module, SWPTR2) = 0; ALINK_SEL(module, SWPTR3) = 0;} while(0)
#define ALNK_SHN_RESET(module)	do {ALINK_SEL(module, SHN0) = 0; ALINK_SEL(module, SHN1) = 0; ALINK_SEL(module, SHN2) = 0; ALINK_SEL(module, SHN3) = 0;} while(0)
#define ALNK_ADR_RESET(module)	do {ALINK_SEL(module, ADR0) = 0; ALINK_SEL(module, ADR1) = 0; ALINK_SEL(module, ADR2) = 0; ALINK_SEL(module, ADR3) = 0;} while(0)
#define ALNK_PNS_RESET(module)	do {ALINK_SEL(module, PNS) = 0;} while(0)

#define	ALINK_DA2BTSRC_SEL(module, x)	SFR(ALINK_SEL(module, CON0), 0, 2, x)
#define	ALINK_DMA_MODE_SEL(module, x)	SFR(ALINK_SEL(module, CON0), 2, 1, x)
#define ALINK_DSPE(module, x)			SFR(ALINK_SEL(module, CON0), 6, 1, x)
#define ALINK_SOE(module, x) 			SFR(ALINK_SEL(module, CON0), 7, 1, x)
#define ALINK_MOE(module, x) 			SFR(ALINK_SEL(module, CON0), 8, 1, x)
#define F32_EN(module, x)    		    SFR(ALINK_SEL(module, CON0), 9, 1, x)
#define SCLKINV(module, x)   		   	SFR(ALINK_SEL(module, CON0),10, 1, x)
#define ALINK_EN(module, x)  		    SFR(ALINK_SEL(module, CON0),11, 1, x)
#define ALINK_24BIT_MODE(module)	(ALINK_SEL(module, CON1) |= (BIT(2) | BIT(6) | BIT(10) | BIT(14)))
#define ALINK_16BIT_MODE(module)	(ALINK_SEL(module, CON1) &= (~(BIT(2) | BIT(6) | BIT(10) | BIT(14))))


#define ALINK_CHx_DIR_MODE(module, ch, x)  	 SFR(ALINK_SEL(module, CON1), 3 + 4 * ch, 1, x)
#define ALINK_CHx_MODE_SEL(module, ch, x)  	 SFR(ALINK_SEL(module, CON1), 4 * ch, 2, x)
#define ALINK_CHx_CLOSE(module, ch)  	 SFR(ALINK_SEL(module, CON1), 4 * ch, 2, 0)


#define ALINK_CLR_ALL_PND(module)				(ALINK_SEL(module, CON2) |= BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define ALINK_CLR_CHx_PND(module, ch)			(ALINK_SEL(module, CON2) |= BIT(ch))
#define ALINK_CHx_IE(module, ch, x)			SFR(ALINK_SEL(module, CON2), ch + 12, 1, x)
#define ALINK_CHx_IE_GET(module, ch)			(ALINK_SEL(module, CON2) & BIT(ch + 12))?1:0

#define ALINK_MSRC(module, x)		SFR(ALINK_SEL(module, CON3), 0, 2, x)
#define ALINK_MDIV(module, x)		SFR(ALINK_SEL(module, CON3), 2, 3, x)
#define ALINK_LRDIV(module, x)		SFR(ALINK_SEL(module, CON3), 5, 3, x)

#define ALINK_OPNS_SET(module, x)		SFR(ALINK_SEL(module, PNS), 16, 16, x)
#define ALINK_IPNS_SET(module, x)		SFR(ALINK_SEL(module, PNS), 0, 16, x)

#define ALINK_LEN_SET(module, x)		(ALINK_SEL(module, LEN) = x)
#define ALINK_FIFO_LEN(module)          (ALINK_SEL(module, LEN))

#define ALINK_CLK_OUPUT_DISABLE 	0xFF
typedef enum {
    ALINK0 			= 0u,							//BR28只有一个ALNK模块
} ALINK_PORT;

//ch_num
typedef enum {
    ALINK_CH0		= 0u,
    ALINK_CH1,
    ALINK_CH2,
    ALINK_CH3,
} ALINK_CH;

//ch_dir
typedef enum {
    ALINK_DIR_TX	= 0u,
    ALINK_DIR_RX		,
} ALINK_DIR;

typedef enum {
    ALINK_LEN_16BIT = 0u,
    ALINK_LEN_24BIT		, //ALINK_FRAME_MODE需要选择: ALINK_FRAME_64SCLK
} ALINK_DATA_WIDTH;

//ch_mode
typedef enum {
    ALINK_MD_NONE	= 0u,
    ALINK_MD_IIS		,
    ALINK_MD_IIS_LALIGN	,
    ALINK_MD_IIS_RALIGN	,
    ALINK_MD_DSP0		,
    ALINK_MD_DSP1		,
} ALINK_MODE;

//ch_mode
typedef enum {
    ALINK_ROLE_MASTER, //主机
    ALINK_ROLE_SLAVE,  //从机
} ALINK_ROLE;

typedef enum {
    ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE, //下降沿更新数据, 上升沿采样数据
    ALINK_CLK_RAISE_UPDATE_FALL_SAMPLE, //上降沿更新数据, 下升沿采样数据
} ALINK_CLK_MODE;

typedef enum {
    ALINK_FRAME_64SCLK, 	//64 sclk/frame
    ALINK_FRAME_32SCLK, 	//32 sclk/frame
} ALINK_FRAME_MODE;

/*
 * SDK默认PLL是192M,仅支持44100,22050,11025采样率,如需其他采样率,需设置PLL为240M,可支持所有采样率
 * PLL在setup.c中clk_early_init()配置
 */
typedef enum {
    ALINK_SR_192000 = 192000,
    ALINK_SR_96000 = 96000,
    ALINK_SR_48000 = 48000,
    ALINK_SR_44100 = 44100,
    ALINK_SR_32000 = 32000,
    ALINK_SR_24000 = 24000,
    ALINK_SR_22050 = 22050,
    ALINK_SR_16000 = 16000,
    ALINK_SR_12000 = 12000,
    ALINK_SR_11025 = 11025,
    ALINK_SR_8000  = 8000,
} ALINK_SR;

typedef enum {
    ALINK_BUF_DUAL, 	//乒乓BUF
    ALINK_BUF_CIRCLE,	//循环BUF
} ALINK_BUF_MODE;

struct alnk_hw_ch {
    //for user
    u8 data_io;					//data IO配置
    u8 ch_ie;					//中断使能
    ALINK_DIR dir; 				//通道传输数据方向: Tx, Rx
    void (*isr_cb)(void *priv, void *addr, int len);	//中断回调
    void *private_data;			//音频私有数据

    //for driver
    ALINK_PORT	module;
    ALINK_CH	ch_idx;
    u8 ch_en;					//使用标记
    void *buf;					//dma buf地址
    u8 bit_width;
};

//===================================//
//多个通道使用需要注意:
//1.数据位宽需要保持一致
//2.buf长度相同
//===================================//
typedef struct _ALINK_PARM {
    ALINK_PORT	module;
    u8 mclk_io; 				//mclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    u8 sclk_io;					//sclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    u8 lrclk_io;				//lrclk IO输出配置: ALINK_CLK_OUPUT_DISABLE不输出该时钟
    struct alnk_hw_ch ch_cfg[4];		//通道内部配置
    ALINK_MODE mode; 					//IIS, left, right, dsp0, dsp1
    ALINK_ROLE role; 			//主机/从机
    ALINK_CLK_MODE clk_mode; 			//更新和采样边沿
    ALINK_DATA_WIDTH  bitwide;   //数据位宽16/32bit
    ALINK_FRAME_MODE sclk_per_frame;  	//32/64 sclk/frame
    u32 dma_len; 						//buf长度: byte
    ALINK_SR sample_rate;					//采样
    ALINK_BUF_MODE 	buf_mode;  	//乒乓buf or 循环buf率
    u32 init_cnt; 						//buf长度: byte
    u16 rx_pns;
    u8 da2sync_ch;             //与蓝牙同步关联的目标通道
} ALINK_PARM;

//iis 模块相关
void *alink_init(void *hw_alink);  //iis 初始化
int alink_start(void *hw_alink);             //iis 开启
int alink_set_sr(void *hw_alink, u32 sr); 			//iis 设置采样率
int alink_get_sr(void *hw_alink); 				//iis 获取采样率
u32 alink_get_dma_len(void *hw_alink); 				//iis 获取DMA_LEN
void alink_set_rx_pns(void *hw_alink, u32 len);		//iis 设置接收PNS
void alink_set_tx_pns(void *hw_alink, u32 len);		//iis 设置发送PNS
int alink_uninit_base(void *hw_alink, void *hw_channel);
int alink_uninit(void *hw_alink, void *hw_channel);
void alink_uninit_force(void *hw_alink);		//iis强制退出全部通道

//iis 通道相关
void alink_set_irq_handler(void *hw_alink, void *hw_channel,  void *priv, void (*handle)(void *priv, void *addr, int len));
void *alink_channel_init_base(void *hw_alink, void *hw_channel, u8 ch_idx);
void *alink_channel_init(void *hw_alink, void *hw_channel);
void alink_channel_io_init(void *hw_alink, void *hw_channel, u8 ch_idx);
void alink_channel_close(void *hw_alink, void *hw_channel);
s32 *alink_get_addr(void *hw_channel);			//iis获取通道DMA地址
u32 alink_get_shn(void *hw_channel);			//iis获取通道SHN
u32 alink_get_shn_lock(void *hw_channel);
u32 alink_get_len(void *hw_channel);			//iis 获取长度LEN
void alink_set_shn(void *hw_channel, u32 len);	//iis设置通道SHN
void alink_set_shn_with_no_sync(void *hw_channel, u32 len);
u32 alink_get_swptr(void *hw_channel);			//iis获取通道swptr
void alink_set_swptr(void *hw_channel, u32 value);		//iis设置通道swptr
u32 alink_get_hwptr(void *hw_channel);			//iis获取通道hwptr
void alink_set_hwptr(void *hw_channel, u32 value);	//iis设置通道hwptr
void alink_set_ch_ie(void *hw_channel, u32 value);	//iis设置通道ie
void alink_clr_ch_pnd(void *hw_channel);			//iis清除通道pnd
int alink_get_ch_ie(void *hw_channel);

void alink_set_da2sync_ch(void *hw_alink);
void audio_alink_lock(u8 module_idx);
void audio_alink_unlock(u8 module_idx);
#endif
