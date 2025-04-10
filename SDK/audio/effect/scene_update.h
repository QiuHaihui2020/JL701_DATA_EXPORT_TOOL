#ifndef _SCENE_UPDATE_H_
#define _SCENE_UPDATE_H_

#include "generic/typedef.h"

//遍历流程图中所有模块加入链表管理，开机后调用一次即可，不可重复调用
void get_music_pipeline_node_uuid();
void get_mic_pipeline_node_uuid();

//根据mode_index和cfg_index直接更新整个流程图中所有模块的参数
void update_music_pipeline_node_list(u8 mode_index, u8 cfg_index);
void update_mic_pipeline_node_list(u8 mode_index, u8 cfg_index);

//整合所有音效模块参数更新的接口
int node_param_update(u16 uuid, char *node_name, u8 mode_index, u8 cfg_index);


#endif



