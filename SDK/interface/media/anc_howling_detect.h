
#ifndef _ANC_HOWLING_DETCET_H_
#define _ANC_HOWLING_DETCET_H_

#include "generic/typedef.h"

//啸叫检测相关结构体
struct anc_howling_detect_cfg {
    u16 detect_time;	//啸叫检测时间(单位ms)
    u16 hold_time;		//增益冷却时间(单位ms)
    u16 resume_time;	//增益恢复时间(单位ms)
    audio_anc_t *param;
    void (*fade_gain_set)(u16 gain);
};

//啸叫检测打印控制变量
extern const u8 CONST_ANC_HOWLING_MSG_DEBUG;
//啸叫检测初始化
void anc_howling_detect_init(struct anc_howling_detect_cfg *cfg);
//啸叫检测使能控制, 支持在线开关
void anc_howling_detect_toggle(u8 toggle);
//啸叫检测中断函数
void anc_howling_detect_isr_handle(u8 pend);
/*ANC啸叫抑制参数初始化-用于工具在线更新*/
void anc_howling_detect_cfg_update(audio_anc_t *param);
/*啸叫检测参数复位*/
void anc_howling_detect_reset(void);


#endif/*_ANC_HOWLING_DETCET_H_*/

