#ifndef __SPDIF_PLAYER_H
#define __SPDIF_PLAYER_H

#include "effect/effects_default_param.h"

/* spdif_player_open
 * @description: 打开spdif 数据流
 * @return：0 - 成功。其它值失败
 * @node:
 */
int spdif_player_open(void);


/* spdif_player_close
 * @description: 关闭spdif 数据流
 * @return：
 * @node:
 */
void spdif_player_close(void);

//更新保存数据流音量mute状态
void update_spdif_player_mute_state(void);
/*
 * @description: 通过消息队列重启 spdif 数据流
 * @return：0 表示消息发送成功
 * @node:
 */
int spdif_restart_by_taskq(void);

/*
 * @description: 通过消息队列打开 spdif 数据流
 * @return：0 表示消息发送成功
 * @node:
 */
int spdif_open_player_by_taskq(void);

/*
 * @description: 返回1代表spdif数据流打开了
 * @node:
 */
bool spdif_player_runing();

int spdif_file_pitch_up();

int spdif_file_pitch_down();

int spdif_file_set_pitch(enum _pitch_level pitch_mode);

void spdif_file_pitch_mode_init(enum _pitch_level pitch_mode);
#endif
