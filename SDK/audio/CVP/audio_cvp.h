#ifndef _AUDIO_CVP_H_
#define _AUDIO_CVP_H_

#include "generic/typedef.h"
#include "user_cfg.h"
#include "app_config.h"
#include "audio_cvp_def.h"

#define AEC_DEBUG_ONLINE	0
#define AEC_READ_CONFIG		1

extern const u8 CONST_AEC_EXPORT;
extern const u8 CONST_REF_SRC;

/*
 *CVP(清晰语音模式定义)
 */
#define CVP_MODE_NONE		0x00	//清晰语音处理关闭
#define CVP_MODE_NORMAL		0x01	//通用模式
#define CVP_MODE_DMS		0x02	//双mic降噪(ENC)模式
#define CVP_MODE_SMS		0x03	//单mic系统(外加延时估计)
#define CVP_MODE_SMS_TDE	0x04	//单mic系统(内置延时估计)
#define CVP_MODE_SEL		CVP_MODE_NORMAL

#define AEC_EN				BIT(0)
#define NLP_EN				BIT(1)
#define	ANS_EN				BIT(2)
#define	ENC_EN				BIT(3)
#define	AGC_EN				BIT(4)
#define WNC_EN              BIT(5)
#define MFDT_EN             BIT(6)
/*仅单麦使用，TDE_EN和TDEYE_EN只有在TCFG_AUDIO_SMS_SEL = SMS_TDE有效*/
#define TDE_EN				BIT(5)	//延时估计模块使能
#define TDEYE_EN			BIT(6)	//延时估计模块结果使用(前提是模块使能)

/*aec module enable bit define*/
#define AEC_MODE_ADVANCE	(AEC_EN | NLP_EN | ANS_EN )
#define AEC_MODE_REDUCE		(NLP_EN | ANS_EN )

#define AEC_MODE_SIMPLEX	(ANS_EN)

extern const u8 CONST_AEC_SIMPLEX;

struct audio_aec_init_param_t {
    s32 sample_rate;
    u32 ref_sr;
    u8 ref_channel;
    u8 mic_num;
};

//CVP预处理结构
struct audio_cvp_pre_param_t {
    u8 pre_gain_en;
    float talk_mic_gain;	//主MIC
    float talk_ref_mic_gain;	//副MIC
    float talk_fb_mic_gain;	//fb MIC
};

/*兼容SMS和DMS*/
#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
#include "cvp_tms.h"
#define aec_open		    aec_tms_init
#define aec_close		    aec_tms_exit
#define aec_in_data		    aec_tms_fill_in_data
#define aec_in_data_ref     aec_tms_fill_in_ref_data
#define aec_in_data_ref_1	aec_tms_fill_in_ref_1_data
#define aec_ref_data	    aec_tms_fill_ref_data

#elif TCFG_AUDIO_DUAL_MIC_ENABLE
#include "cvp_dms.h"
#if (TCFG_AUDIO_DMS_SEL == DMS_NORMAL)
#define aec_open		aec_dms_init
#define aec_close		aec_dms_exit
#define aec_in_data		aec_dms_fill_in_data
#define aec_in_data_ref	aec_dms_fill_in_ref_data
#define aec_ref_data	aec_dms_fill_ref_data
#elif (TCFG_AUDIO_DMS_SEL == DMS_FLEXIBLE)
#define aec_open		aec_dms_flexible_init
#define aec_close		aec_dms_flexible_exit
#define aec_in_data		aec_dms_flexible_fill_in_data
#define aec_in_data_ref	aec_dms_flexible_fill_in_ref_data
#define aec_ref_data	aec_dms_flexible_fill_ref_data
#elif (TCFG_AUDIO_DMS_SEL == DMS_HYBRID)
#define aec_open		aec_dms_hybrid_init
#define aec_close		aec_dms_hybrid_exit
#define aec_in_data		aec_dms_hybrid_fill_in_data
#define aec_in_data_ref	aec_dms_hybrid_fill_in_ref_data
#define aec_ref_data	aec_dms_hybrid_fill_ref_data
#elif (TCFG_AUDIO_DMS_SEL == DMS_AWN)
#define aec_open		aec_dms_awn_init
#define aec_close		aec_dms_awn_exit
#define aec_in_data		aec_dms_awn_fill_in_data
#define aec_in_data_ref	aec_dms_awn_fill_in_ref_data
#define aec_ref_data	aec_dms_awn_fill_ref_data
#endif/*TCFG_AUDIO_DMS_SEL*/

#elif (TCFG_AUDIO_SMS_SEL == SMS_TDE)
#include "cvp_sms.h"
#define aec_open		sms_tde_init
#define aec_close		sms_tde_exit
#define aec_in_data		sms_tde_fill_in_data
#define aec_in_data_ref(...)
#define aec_ref_data	sms_tde_fill_ref_data
#else
#include "cvp_sms.h"
#define aec_open		aec_init
#define aec_close		aec_exit
#define aec_in_data		aec_fill_in_data
#define aec_in_data_ref(...)
#define aec_ref_data	aec_fill_ref_data
#endif

/*
*********************************************************************
*                  Audio AEC Output Select
* Description: 输出选择
* Arguments  : sel  = Default	默认输出算法处理结果
*					= Master	输出主mic(通话mic)的原始数据
*					= Slave		输出副mic(降噪mic)的原始数据
*			   agc 输出数据要不要经过agc自动增益控制模块
* Return	 : None.
* Note(s)    : 可以通过选择不同的输出，来测试mic的频响和ENC指标
*********************************************************************
*/
void audio_aec_output_sel(CVP_OUTPUT_ENUM sel, u8 agc);

s8 aec_debug_online(void *buf, u16 size);
void aec_input_clear_enable(u8 enable);

int audio_aec_init(struct audio_aec_init_param_t *init_param);
void audio_aec_close(void);
void audio_aec_inbuf(s16 *buf, u16 len);
void audio_aec_inbuf_ref(s16 *buf, u16 len);
void audio_aec_inbuf_ref_1(s16 *buf, u16 len);
void audio_aec_refbuf(s16 *data0, s16 *data1, u16 len);
u8 audio_aec_status(void);
void audio_aec_reboot(u8 reduce);
u8 get_audio_aec_rebooting();
int audio_cvp_probe_param_update(struct audio_cvp_pre_param_t *cfg);
/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : init_param 	sr 			采样率(8000/16000)
*              				ref_sr      参考采样率
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
*              link_type  0: ESCO  1: ACL
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(struct audio_aec_init_param_t *init_param, s16 enablebit, int (*out_hdl)(s16 *data, u16 len));

/*外部参考数据变采样*/
int audio_cvp_ref_src_open(u8 scene, u32 insr, u32 outsr, u8 nch);
int audio_cvp_ref_src_data_fill(void *p, s16 *data, int len);
void audio_cvp_ref_src_close();
/*对齐iis的数据延时，在第一次mic数据来时调用对齐*/
void audio_cvp_ref_data_align();
int audio_cvp_ref_data_align_reset(void);

void audio_cvp_ref_start(u8 en);
void audio_cvp_set_output_way(u8 en);
/*可写长度*/
int get_audio_cvp_output_way_writable_len();

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				audio_cvp_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				audio_cvp_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
int audio_cvp_ioctl(int cmd, int value, void *priv);

int cvp_develop_read_ref_data(void);

int audio_cvp_phase_align(void);

int audio_cvp_toggle_set(u8 toggle);

/*获取风噪的检测结果，1：有风噪，0：无风噪*/
int audio_cvp_dms_wnc_state(void);

/*获取单双麦切换状态
 * 0: 正常双麦 ;
 * 1: 副麦坏了，触发故障
 * -1: 主麦坏了，触发故障
 */
int audio_cvp_dms_malfunc_state();
int audio_cvp_tms_malfunc_state();

/*
 * 获取mic的能量值，开了MFDT_EN才能用
 * mic: 0 获取主麦能量
 * mic：1 获取副麦能量
 * return：返回能量值，[0~90.3],返回-1表示错误
 */
float audio_cvp_dms_mic_energy(u8 mic);
float audio_cvp_tms_mic_energy(u8 mic);

/*
 * 获取风噪检测信息
 * wd_flag: 0 没有风噪，1 有风噪
 * 风噪强度r: 0~BIT(16)
 * wd_lev: 风噪等级，0：弱风，1：中风，2：强风
 * */
int audio_cvp_get_wind_detect_info(int *wd_flag, int *wd_val, int *wd_lev);

#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
/* 设置跑2mic/3mic降噪
 * TMS_MODE_2MIC  跑双麦降噪，不使用FB MIC数据
 * TMS_MODE_3MIC  跑3MIC降噪
 * Notes : CONST_JLSP_3MIC_MODE != JLSP_3MIC_MODE0时设置才有效*/
int audio_tms_mode_choose(enum cvp_tms_mode mode);
#endif

u16 get_cvp_node_uuid();

#endif
