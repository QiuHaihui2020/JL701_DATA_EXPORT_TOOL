#ifndef _CVP_COMMON_H_
#define _CVP_COMMON_H_
#include "audio_cvp_def.h"

/*DMS输出选择*/
typedef enum {
    DMS_OUTPUT_SEL_DEFAULT = 0,	/*默认输出：dms处理后的数据*/
    DMS_OUTPUT_SEL_MASTER,		/*主mic原始数据*/
    DMS_OUTPUT_SEL_SLAVE,		/*副mic原始数据*/
    DMS_OUTPUT_SEL_FBMIC,		/*FB mic原始数据*/
} CVP_OUTPUT_ENUM;

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				aec_dms_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				aec_dms_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
enum {
    CVP_AEC_SWITCH = 1,
    CVP_NLP_SWITCH,
    CVP_NS_SWITCH,
    CVP_AGC_SWITCH,
    CVP_ENC_SWITCH,
    CVP_AGC_MAX_LVL,
    CVP_ANS_NOISE_FLOOR,
    CVP_ANS_LOWCUTTHR,
    CVP_WNC_SWITCH,
    CVP_MFDT_SWITCH,
    CVP_OUTWAY_REF_IGNORE,
    CVP_DNS_NOISEGATE_VAL,
};
int aec_ioctl(int cmd, int value, void *priv);
int sms_tde_ioctl(int cmd, int value, void *priv);
int aec_dms_ioctl(int cmd, int value, void *priv);
int aec_dms_flexible_ioctl(int cmd, int value, void *priv);
int aec_dms_hybrid_ioctl(int cmd, int value, void *priv);
int aec_dms_awn_ioctl(int cmd, int value, void *priv);
int aec_tms_ioctl(int cmd, int value, void *priv);


#endif /*_CVP_COMMON_H_*/

