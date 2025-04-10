
#ifndef _ICSD_ANC_INTERACTIVE_H_
#define _ICSD_ANC_INTERACTIVE_H_

#include "asm/anc.h"
#include "icsd_anc.h"

void anc_user_train_tone_play_cb();
extern anc_packet_data_t *anc_adaptive_data;


void anc_ear_adaptive_init(audio_anc_t *param);
void anc_ear_adaptive_cmd_handler(audio_anc_t *param, int *msg);
void anc_ear_adaptive_tone_play_start(void);
void anc_ear_adaptive_tone_play_err_cb(void);
void anc_ear_adaptive_tone_play_cb(void);
void anc_ear_adaptive_develop_set(u8 mode);
void anc_user_train_cb(u8 mode, u8 result, u8 forced_exit);
int audio_anc_adaptive_data_read(void);
void audio_anc_ear_adaptive_param_init(audio_anc_t *param);
void audio_anc_adaptive_data_packet(struct icsd_anc_tool_data *TOOL_DATA);
void audio_anc_param_map(u8 coeff_en, u8 gain_en);
void anc_ear_adaptive_power_off_save_data(void);
u8 anc_ear_adaptive_tws_sync_en_get(void);
void anc_ear_adaptive_seq_set(u8 seq);
u8 anc_ear_adaptive_seq_get(void);

/* 判断当前是否处于训练中 */
u8 anc_ear_adaptive_busy_get(void);
/* 耳道自适应训练结束 */
void anc_ear_adaptive_mode_end(void);


/* ANC自适应训练处理回调函数 */
void audio_anc_adaptive_fail_callback(u8 anc_err);

/*自适应模式-重新检测
 * param: tws_sync_en          1 TWS同步自适应，支持TWS降噪平衡，需左右耳一起调用此接口
 *                             0 单耳自适应, 不支持TWS降噪平衡，可TWS状态下单耳自适应
 */
int audio_anc_mode_ear_adaptive(u8 tws_sync_en);

/*强制停止ANC训练 - 并用回默认参数*/
void anc_ear_adaptive_forced_exit(u8 default_flag);

/*
   自适应/普通参数切换(只切换参数，不更新效果)
	param:  mode 		0 使用普通参数; 1 使用自适应参数
			tone_play 	0 不播放提示音；1 播放提示音
 */
int audio_anc_coeff_adaptive_set(u32 mode, u8 tone_play);

/*
   自适应/普通参数切换(切换参数，并立即更新效果)
   前置条件：需在 "ANC_ON" 模式下调用;
	param:  mode 		0 使用普通参数; 1 使用自适应参数
			tone_play 	0 不播放提示音；1 播放提示音
 */
int audio_anc_coeff_adaptive_update(u32 mode, u8 tone_play);

/*ANC滤波器模式循环切换*/
int audio_anc_coeff_adaptive_switch();

/*当前ANC滤波器模式获取 0:普通参数 1:自适应参数*/
int audio_anc_coeff_mode_get(void);

int audio_anc_ear_adaptive_a2dp_suspend_cb(void);

#endif/*_ICSD_ANC_INTERACTIVE_H_*/
