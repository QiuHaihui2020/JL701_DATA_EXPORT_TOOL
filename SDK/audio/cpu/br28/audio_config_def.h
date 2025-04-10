#ifndef _AUDIO_CONFIG_DEF_H_
#define _AUDIO_CONFIG_DEF_H_
/*
 *******************************************************************
 *						Audio CPU Definitions
 *
 *Note(s):
 *		 Only macro definitions can be defined here.
 *******************************************************************
 */

#include "app_config.h"
#include "audio_def.h"
#include "audio_platform_config.h"

//**************************************
// 			ASS通用配置
//**************************************
#define MEDIA_24BIT_ENABLE		TCFG_AUDIO_BIT_WIDTH

//**************************************
// 		    场景参数更新使能
//**************************************
#define TCFG_SCENE_UPDATE_ENABLE            0

//**************************************
// 			音频模块链接配置
//**************************************
/*音效处理链接配置*/
#define AFx_VBASS_AT_RAM				    1	//虚拟低音
#define AFx_REVERB_AT_RAM				    1	//混响
#define AFx_ECHO_AT_RAM				        1	//回声
#define AFx_VOICECHANGER_AT_RAM			    1	//变声
#define AFx_DRC_AT_RAM 					    1	//DRC
#define AFx_HARMONIC_EXCITER_AT_RAM 	    1	//谐波激励
#define AFx_DYN_EQ_AT_RAM 				    1	//动态EQ
#define AFx_NOTCH_HOWLING_AT_RAM 		    1	//啸叫抑制：陷波
#define AFx_FREQ_SHIFT_AT_RAM	 		    1	//啸叫抑制：移频
#define AFx_NOISEGATE_AT_RAM	 		    1	//噪声门
#define AFx_ADVAUDIO_PLC_AT_RAM	    	    1
#define AFX_AUDIO_LINK_AT_RAM               0   //iis驱动
#define AFX_AUDIO_SYNC_AT_RAM               0   //sync
#define AFx_EQ_AT_RAM                       0	//eq
#define AFx_VOCAL_TRACK_AT_RAM              0   //声道组合与声道拆分
#define AFX_AUDIO_DIGITAL_VOL_AT_RAM        0   //数字音量
#define AFX_AUDIO_ENERGY_DET_AT_RAM         0   //能量检测
#define AFX_LIMITER_AT_RAM                  0   //限幅器
#define AFX_MULTIBAND_CROSSOVER_AT_RAM      0   //多带限幅器与多带drc使用的多带分频器
#define AFX_MULTIBAND_LIMITER_AT_RAM        0   //多带限幅器
#define AFX_MULTIBAND_DRC_AT_RAM            0   //多带drc
#define AFX_VIRTUAL_SURRUOUND_PRO_AT_RAM    0   //虚拟环绕声pro/2t4/2t5
#define AFX_SW_EQ_AT_RAM                    0   //软件EQ
#define AFx_SPATIAL_EFFECT_AT_RAM           1   //空间音效

/*通话语音处理算法*/
#define AUDIO_CVP_TEXT_AT_RAM	    	0	//COMMON TEXT
#define AUDIO_CVP_AEC_AT_RAM	    	0	//AEC
#define AUDIO_CVP_NLP_AT_RAM	    	0	//NLP
#define AUDIO_CVP_NS_AT_RAM		    	0	//ANS/下行降噪
#define AUDIO_CVP_COMMON_AT_RAM	    	0	//COMMON
#define AUDIO_CVP_DNS_AT_RAM	    	0	//DNS
#define AUDIO_CVP_AGC_AT_RAM	    	0	//AGC
#define AUDIO_CVP_DMS_AT_RAM	    	0	//双MIC DMS
#define AUDIO_CVP_SMS_AT_RAM		    0	//单MIC SMS_TDE
#define AUDIO_CVP_PREP_AT_RAM		    0	//COMMON 预处理
#define AUDIO_CVP_WN_AT_RAM			    0	//抗风噪
#define AUDIO_CVP_THIRD_AT_RAM		    0	//3MIC

/*编解码编译链接配置*/
#define AUD_AAC_DEC_AT_RAM		        1   //AAC解码
#define AUDIO_LDAC_AT_RAM			    1	//LDAC解码
#define AUDIO_MSBC_CODEC_AT_RAM		    0	//MSBC 编解码
#define AUDIO_CVSD_CODEC_AT_RAM		    0	//CVSD 编解码
#define AUDIO_JLA_CODEC_AT_RAM			1	//JLA 编解码
#define AUDIO_LC3_CODEC_AT_RAM			0	//LC3 编解码

/*语音识别算法编译链接配置*/
#define AUDIO_KWS_COMMON_AT_RAM             0   //kws公共部分 ，0:放flash，1:放ram
#define AUDIO_KWS_YES_NO_AT_RAM             0   //yes/no识别 ， 0:放flash，1:放ram
#define AUDIO_KWS_CHINESE_AT_RAM            2   //近场中文识别，0:放flash，1:放ram，2:一部分放ram，一部分放flash
#define AUDIO_KWS_INDIA_ENGLISH_AT_RAM      0   //印度英语识别，0:放flash，1:放ram
#define AUDIO_KWS_CHINESE_FAR_AT_RAM        1   //远场中文识别，0:放flash，1:放ram


//**************************************
// 			模块使能控制
//**************************************

//***************End********************

//**************************************
// 			音效使能控制
//**************************************
#define AUDIO_VBASS_LINK_VOLUME     0 //虚拟低音与音量联动调节
//**************************************
// 			麦克风音效配置
//**************************************
#define DUAL_ADC_EFFECT     0 //两路ADC输入使能(如：麦克风+吉他)
#if TCFG_BT_VOL_SYNC_ENABLE
#define TCFG_MAX_VOL_PROMPT						 0
#else
#define TCFG_MAX_VOL_PROMPT						 1
#endif

//ADC中断点数
#define AUDIO_ADC_IRQ_POINTS   256
#define AUDIO_ADC_IRQ_POINTS_MUSIC_MODE 256

#if TCFG_MIC_EFFECT_ENABLE
/*如果混响开启，则LINEIN共享ADC IRQ配置*/
#define AUDIO_LINEIN_IRQ_POINTS AUDIO_ADC_IRQ_POINTS
#else
#define AUDIO_LINEIN_IRQ_POINTS 128
#endif

#if (TCFG_LOCAL_TWS_ENABLE)
#undef AUDIO_LINEIN_IRQ_POINTS
#define AUDIO_LINEIN_IRQ_POINTS 256
#endif

#define AUDIO_DAC_MAX_SAMPLE_RATE           48000
//**************************************
// 			IIS模块配置
//**************************************
//ADC中断点数
#define AUDIO_IIS_IRQ_POINTS 128
//***************End********************


/*
 *br36 支持24bit位宽输出
 * */
#define TCFG_AUDIO_DAC_24BIT_MODE           0
#define AUDIO_DAC_MAX_SAMPLE_RATE           48000

/*
 * Hi-Res Audio使能LHDC/LDAC要求：
 * <1>DAC最高采样率调整到96K
 * <2>同时全局采样率至少要96kHz，或者Disable（即自适应）
 */
#ifdef TCFG_HI_RES_AUDIO_ENEBALE
#undef AUDIO_DAC_MAX_SAMPLE_RATE
#define AUDIO_DAC_MAX_SAMPLE_RATE           96000
#if (TCFG_AUDIO_GLOBAL_SAMPLE_RATE && (TCFG_AUDIO_GLOBAL_SAMPLE_RATE < 96000))
#error "Hi-Res Audio：请将全局采样率TCFG_AUDIO_GLOBAL_SAMPLE_RATE设置到至少96000，或Disable！"
#endif
#endif

#define TCFG_AUDIO_DAC_NOISEGATE_ENABLE     1

/*
 *该配置适用于没有音量按键的产品，防止打开音量同步之后
 *连接支持音量同步的设备，将音量调小过后，连接不支持音
 *量同步的设备，音量没有恢复，导致音量小的问题
 *默认是没有音量同步的，将音量设置到最大值，可以在vol_sync.c
 *该宏里面修改相应的设置。
 */
#define TCFG_VOL_RESET_WHEN_NO_SUPPORT_VOL_SYNC	 0 //不支持音量同步的设备默认最大音量

#define TCFG_1T2_VOL_RESUME_WHEN_NO_SUPPORT_VOL_SYNC	1	//1T2不支持音量同步的设备则恢复上次设置的音量值

/*省电容mic模块使能*/
#if ((TCFG_ADC0_ENABLE && (TCFG_ADC0_MODE == 2)) || \
        (TCFG_ADC1_ENABLE && (TCFG_ADC1_MODE == 2)) || \
        (TCFG_ADC2_ENABLE && (TCFG_ADC2_MODE == 2)) || \
        (TCFG_ADC3_ENABLE && (TCFG_ADC3_MODE == 2)))
#define TCFG_SUPPORT_MIC_CAPLESS        1
#else
#define TCFG_SUPPORT_MIC_CAPLESS        0
#endif

/*省电容mic校准方式选择*/
#define MC_BIAS_ADJUST_DISABLE			0	//省电容mic偏置校准关闭
#define MC_BIAS_ADJUST_ONE			 	1	//省电容mic偏置只校准一次（跟dac trim一样）
#define MC_BIAS_ADJUST_POWER_ON		 	2	//省电容mic偏置每次上电复位都校准(Power_On_Reset)
#define MC_BIAS_ADJUST_ALWAYS		 	3	//省电容mic偏置每次开机都校准(包括上电复位和其他复位)
/*
 *省电容mic偏置电压自动调整(因为校准需要时间，所以有不同的方式)
 *1、烧完程序（完全更新，包括配置区）开机校准一次
 *2、上电复位的时候都校准,即断电重新上电就会校准是否有偏差(默认)
 *3、每次开机都校准，不管有没有断过电，即校准流程每次都跑
 */
#if TCFG_SUPPORT_MIC_CAPLESS
#define TCFG_MC_BIAS_AUTO_ADJUST	 	MC_BIAS_ADJUST_POWER_ON
#else
#define TCFG_MC_BIAS_AUTO_ADJUST	 	MC_BIAS_ADJUST_DISABLE
#endif/*TCFG_SUPPORT_MIC_CAPLESS*/
#define TCFG_MC_CONVERGE_PRE			0  //省电容mic预收敛
#define TCFG_MC_CONVERGE_TRACE			0  //省电容mic收敛值跟踪
/*
 *省电容mic收敛步进限制
 *0:自适应步进调整, >0:收敛步进最大值
 *注：当mic的模拟增益或者数字增益很大的时候，mic_capless模式收敛过程,
 *变化的电压放大后，可能会听到哒哒声，这个时候就可以限制住这个收敛步进
 *让收敛平缓进行(前提是预收敛成功的情况下)
 */
#define TCFG_MC_DTB_STEP_LIMIT			3  //最大收敛步进值
/*
 *省电容mic使用固定收敛值
 *可以用来测试默认偏置是否合理：设置固定收敛值7000左右，让mic的偏置维持在1.5v左右即为合理
 *正常使用应该设置为0,让程序动态收敛
 */
#define TCFG_MC_DTB_FIXED				0

#define TCFG_ESCO_PLC					1  	//通话丢包修复(1T2已修改为节点)
#define TCFG_AEC_ENABLE					1	//通话回音消除使能

#define MAX_ANA_VOL               (3)	// 系统最大模拟音量,范围: 0 ~ 3
//#define MAX_COM_VOL             (16)    // 数值应该大于等于16，具体数值应小于联合音量等级的数组大小 (combined_vol_list)
//#define MAX_DIG_VOL             (16)    // 数值应该大于等于16，因为手机是16级，如果小于16会导致某些情况手机改了音量等级但是小机音量没有变化

/*#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#define SYS_MAX_VOL             16
#define SYS_DEFAULT_VOL         16
#define SYS_DEFAULT_TONE_VOL    10
#define SYS_DEFAULT_SIN_VOL    	8

#elif (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
#define SYS_MAX_VOL             MAX_DIG_VOL
#define SYS_DEFAULT_VOL         SYS_MAX_VOL
#define SYS_DEFAULT_TONE_VOL    10
#define SYS_DEFAULT_SIN_VOL    	8

#elif (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
#define SYS_MAX_VOL             MAX_ANA_VOL
#define SYS_DEFAULT_VOL         SYS_MAX_VOL
#define SYS_DEFAULT_TONE_VOL    10
#define SYS_DEFAULT_SIN_VOL    	8

#elif (SYS_VOL_TYPE == VOL_TYPE_AD)
#define SYS_MAX_VOL             MAX_COM_VOL
#define SYS_DEFAULT_VOL         SYS_MAX_VOL
#define SYS_DEFAULT_TONE_VOL    14
#define SYS_DEFAULT_SIN_VOL    	8
#else
#error "SYS_VOL_TYPE define error"
#endif*/

/*数字音量最大值定义*/
#define DEFAULT_DIGITAL_VOLUME   (0.0f)
#define IDLE_DEFAULT_MAX_VOLUME  100     //音量调节IDLE 状态的最大音量

#define ANC_CLIPPING_MODE_MUSIC_LIMIT		1	//ANC防破音-限制音乐幅度，牺牲音乐
#define ANC_CLIPPING_MODE_DYNAMIC_ANC_GAIN	2	//ANC防破音-ANC音乐动态增益，牺牲ANC性能

//ANC防破音-限制音乐幅度时，音乐默认衰减-6dB
#if TCFG_AUDIO_ANC_ENABLE
#if (TCFG_ANC_MUSIC_ANTI_CLIPPING_MODE == ANC_CLIPPING_MODE_MUSIC_LIMIT)
#define ANC_MODE_DIG_VOL_LIMIT	(TCFG_ANC_MUSIC_FIXED_LIMIT_GAIN)
#else
#define ANC_MODE_DIG_VOL_LIMIT	(0.0f)
#endif
#endif/*TCFG_AUDIO_ANC_ENABLE*/


#define BT_MUSIC_VOL_LEAVE_MAX	16		/*高级音频音量等级*/
#define BT_CALL_VOL_LEAVE_MAX	15		/*通话音量等级*/
// #define BT_CALL_VOL_STEP		(-2.0f)	[>通话音量等级衰减步进<]

/*
 *audio state define
 */
#define APP_AUDIO_STATE_IDLE        0
#define APP_AUDIO_STATE_MUSIC       1
#define APP_AUDIO_STATE_CALL        2
#define APP_AUDIO_STATE_WTONE       3
#define APP_AUDIO_STATE_KTONE       4
#define APP_AUDIO_STATE_RING       	5
#define APP_AUDIO_CURRENT_STATE     6

#define TONE_BGM_FADEOUT            0   //播叠加提示音时是否将背景音淡出

#define VOL_TAB_CUSTOM_EN           1  //使能音量表功能

#define TCFG_AUDIO_MIC_DUT_ENABLE	0   //麦克风测试和传递函数测试

//支持 ADC DIGITAL 按最大通道数开启, 中断拿数需按使用的ADC_CH拆分
#define TCFG_AUDIO_ADC_ENABLE_ALL_DIGITAL_CH

/*
 * 2.1声道音量调节节点配置
 */
#define DVOL_2P1_CH_DVOL_ADJUST_TOTAL	    0	//2.1声道总增益
#define DVOL_2P1_CH_DVOL_ADJUST_LR		    1	//2.1声道LR通路增益
#define DVOL_2P1_CH_DVOL_ADJUST_SW		    2	//2.1声道低音通路增益
#define DVOL_2P1_CH_DVOL_ADJUST_NODE		DVOL_2P1_CH_DVOL_ADJUST_TOTAL

#define ANC_EXT_V1			1	//仅支持入耳
#define ANC_EXT_V2			2	//支持入耳、半入耳
//ANC耳道自适应版本
#define TCFG_AUDIO_ANC_EAR_ADAPTIVE_VERSION 	ANC_EXT_V2
#define TCFG_AUDIO_ANC_EXT_VERSION 				ANC_EXT_V2

#endif/*_AUDIO_CONFIG_DEF_H_*/
