#include "audio_cvp.h"


/*参考数据变采样处理
 *<1>Dongle通话上行
 *<2>通话下行超过16kHz采样率
 *<3>语音识别使用回音消除
 *<4>USB Audio MIC通话录音
 *<5>LE Audio通话录音
 */
#if TCFG_BT_DONGLE_ENABLE || \
	(TCFG_ESCO_DL_CVSD_SR_USE_16K > 1) || \
	(TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC) || \
	TCFG_USB_SLAVE_AUDIO_MIC_ENABLE || \
	(((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))) && (defined TCFG_LEA_CALL_DL_GLOBAL_SR) && (TCFG_LEA_CALL_DL_GLOBAL_SR != 0x01))
const u8 CONST_REF_SRC = 1;
#else
const u8 CONST_REF_SRC = 0;
#endif /*TCFG_USB_MIC_CVP_ENABLE*/

/*
 * CVP串口数据导出
 * 0 : 关闭数据导致
 * 1 : 导出 mic0 mic1 far
 * 2 : 导出 mic0 mic1 out
 * 3 : 导出 mic0 far out
 */
#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
const u8 CONST_AEC_EXPORT = 3;/*1:far 2:out*/
#else
const u8 CONST_AEC_EXPORT = 0;
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

/*模块使能，控制代码是否链接*/
const u8 CONST_AEC_ENABLE = 1;
const u8 CONST_NLP_ENABLE = 1;
const u8 CONST_NS_ENABLE  = 1;
const u8 CONST_AGC_ENABLE = 1;

#if TCFG_AEC_SIMPLEX
const u8 CONST_AEC_SIMPLEX = 1;
#else
const u8 CONST_AEC_SIMPLEX = 0;
#endif/*TCFG_AEC_SIMPLEX*/

/*CVP带宽配置*/
const u8 CONST_CVP_BAND_WIDTH_CFG = TCFG_AUDIO_CVP_BAND_WIDTH_CFG;  //ANS
const u8 CONST_DNS_PARAM_TYPE = TCFG_AUDIO_CVP_BAND_WIDTH_CFG;      //DNS







