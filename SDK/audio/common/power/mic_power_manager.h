#ifndef _MIC_POWER_MANAGER_H_
#define _MIC_POWER_MANAGER_H_

#include "generic/typedef.h"

//MIC电源控制
typedef enum {
    MIC_PWR_INIT = 1,	/*开机状态*/
    MIC_PWR_ON,			/*工作状态*/
    MIC_PWR_OFF,		/*空闲状态*/
    MIC_PWR_DOWN,		/*低功耗状态*/
} audio_mic_pwr_t;

/*
*********************************************************************
*           audio_mic_pwr_ctl
* Description: MIC电源普通IO供电管理
* Arguments  : None.
* Return	 : state MIC电源状态
* Note(s)    : None.
*********************************************************************
*/
void audio_mic_pwr_ctl(audio_mic_pwr_t state);

#endif /*_MIC_POWER_MANAGER_H_*/

