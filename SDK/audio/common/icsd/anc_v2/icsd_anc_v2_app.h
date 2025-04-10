#ifndef _ICSD_ANC_V2_APP_H
#define _ICSD_ANC_V2_APP_H

/*===========接口说明=============================================
一. void sd_anc_init(audio_anc_t *param,u8 mode)
  1.调用该函数启动sd anc训练
  2.param为ANC ON模式下的audio_anc_t参数指针
  3.mode：0. 普通模式   1. htarget 数据上传模式

二. void sd_anc_htarget_data_send(float *h_freq,float *hszpz_out,float *hpz_out, float *htarget_out,int len)
  1.htarget数据上传模式下，当数据准备好后会自动调用该函数

三. void sd_anc_htarget_data_end()
  1.htarget数据上传模式下，当数据上传完成后需要调用该函数进入训练后的ANC ON模式

四. void anc_user_train_tone_play_cb()
  1.当前ANC ON提示音结束时回调该函数进入 SD ANC 训练
  2.SD ANC训练结束后会切换到ANC ON模式

 	1:rdout   + lout		2:rdout   + rerrmic
 	3:rdout   + rrefmic		4:ldout   + lerrmic
 	5:ldout   + lrefmic		6:rerrmic + rrefmic
 	7:lerrmic + lrefmic

	anc_sr, 0: 11.7k  1: 23.4k  2: 46.9k  3: 93.8k  4: 187.5k     5: 375k     6: 750k      7: 1.5M

================================================================*/

//#include "asm/anc.h"
#include "asm/dac.h"
#include "app_config.h"
#include "icsd_anc_v2.h"
#include "asm/anc.h"
#include "icsd_anc_v2_interactive.h"
#include "anc_ext_tool.h"

//#define ICSD_ANC_MODE     				HEADSET_4CH_TONE_BYPASS_PZ	//TWS_TONE_BYPASS_MODE
#define ANC_ADAPTIVE_FF_ORDER			10	    	/*ANC自适应FF滤波器阶数, 原厂指定*/
#define ANC_ADAPTIVE_FB_ORDER			5			/*ANC自适应FB滤波器阶数，原厂指定*/
#define ANC_ADAPTIVE_CMP_ORDER			8			/*ANC自适应CMP滤波器阶数，原厂指定*/
#define ANC_ADAPTIVE_RECORD_FF_ORDER	10  		/*ANC自适应耳道记忆FF滤波器阶数，原厂指定*/
#define ANC_EARPHONE_CHECK_EN           0
#define ANC_ADAPTIVE_CMP_EN             0 			//自适应CMP补偿
#define ANC_ADAPTIVE_TONE_DELAY	        700//900


#define ANC_HEADSET_TONE_00				0
#define ANC_HEADSET_TONE_01		    	1
#define ANC_HEADSET_TONE				ANC_HEADSET_TONE_00

#if ANC_HEADSET_TONE == ANC_HEADSET_TONE_00
#define  TONEL_DELAY     				50
#define  TONER_DELAY     				2400
#define  PZL_DELAY       				900
#define  PZR_DELAY       				3300
#else
#define TONEL_DELAY     				50
#define TONER_DELAY     				1000
#define PZL_DELAY       				1900
#define PZR_DELAY       				3200
#endif

#define HALF_INEAR_PZ_DELAY         	1310
#define HALF_INEAR_TONE_DELAY       	1900
#define ANC_USER_TRAIN_TONE_MODE    	1
#define ANC_EAR_RECORD_EN           	0 //耳道记忆
#define EAR_RECORD_MAX  		    	5
#define EAR_CANAL_MEM_LEN           	(19*2*5*2)//(19*2*10*2)

#define ANC_USE_RECORD     			1	//ANC自适应失败使用耳道记忆参数
#define ANC_SUCCESS        			2	//ANC自适应成功
#define ANC_USE_DEFAULT    			3	//ANC自适应使用ICSD内部默认参数，目前不用;

//TOOL_DATA->anc_combination结果含义
#define ANC_TFF                     BIT(0)	//使用FF 自适应参数
#define ANC_TFB                     BIT(1)	//使用FB 自适应参数
#define ANC_TCMP                    BIT(2)	//使用CMP 自适应参数

//TOOL_DATA->result_l / TOOL_DATA->result_r结果含义
#define FF_ANC_SUCCESS              0   //成功:自适应成功
#define FF_ANC_FAIL                 1   //失败:自适应失败，使用配置low的默认滤波器
#define FF_ANC_JITTER2              2   //抖动:自适应抖动1.0k后高频抖动，使用部分耳道数据自适应
#define FF_ANC_JITTER1              3   //抖动:自适应抖动1.5k后高频抖动，使用部分耳道数据自适应
#define FF_ANC_RECORD               4   //抖动:自适应曲线抖动，使用耳道数据自适应
#define FF_ANC_ERROR                5   //异常:不存在该状态

//自适应训练结果 u8
#define ANC_ADAPTIVE_RESULT_LFF		BIT(0)	//使用LFF自适应or记忆参数
#define ANC_ADAPTIVE_RESULT_LFB    	BIT(1)	//使用LFB自适应参数
#define ANC_ADAPTIVE_RESULT_LCMP    BIT(2)	//使用LCMP自适应参数
#define ANC_ADAPTIVE_RESULT_RFF     BIT(3)	//使用RFF自适应or记忆参数
#define ANC_ADAPTIVE_RESULT_RFB     BIT(4)	//使用RFB自适应参数
#define ANC_ADAPTIVE_RESULT_RCMP    BIT(5)	//使用RCMP自适应参数

struct icsd_fb_ref {
    float m_value;
    float sen;
    float in_q;
};

struct icsd_ff_ref {
    float fre[75];
    float db[75];
};

float icsd_anc_v2_readfbgain();
void icsd_anc_v2_dma_4ch_on(u8 out_sel_ch01, u8 out_sel_ch23, int *buf, int len);
void icsd_anc_v2_dma_2ch_on(u8 out_sel, int *buf, int len);
void icsd_anc_v2_dma_done();
void icsd_anc_v2_board_config();
void icsd_anc_v2_output(struct icsd_anc_v2_tool_data *_TOOL_DATA, u8 OUTPUT_STATE);
void anc_v2_use_train_open_ancout(u8 ff_yorder, float bypass_vol, s8 bypass_sign);
u8 icsd_anc_v2_train_result_get(struct icsd_anc_v2_tool_data *TOOL_DATA);

//-----form icsd_anc_v2.c
void icsd_anc_v2_init(void *_hdl, struct anc_ext_ear_adaptive_param *ext_cfg);
void icsd_anc_v2_forced_exit();


#endif/*_ICSD_ANC_V2_APP_H*/
