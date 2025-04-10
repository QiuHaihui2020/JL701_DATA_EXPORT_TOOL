#ifndef _AUD_DEBUG_H_
#define _AUD_DEBUG_H_

#include "generic/typedef.h"

/*定时打印音频配置参数开关，默认3s打印一次
 *量产版本应该关闭，仅作debug调试使用
 */
#define TCFG_AUDIO_CONFIG_TRACE     		0	//跟踪使能配置
#define TCFG_AUDIO_CONFIG_TRACE_INTERVAL    3000//跟踪频率配置unit:ms

/*
**************************************************************
*                  Audio Config Trace Setup
* Description: 音频配置跟踪函数
* Arguments  : interval 跟踪间隔，单位ms
* Return	 : None.
* Note(s)    : None.
**************************************************************
*/
void audio_config_trace_setup(int interval);

#endif
