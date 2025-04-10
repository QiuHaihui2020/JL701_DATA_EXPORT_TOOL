#ifndef ABILITY_UUID_H
#define ABILITY_UUID_H

#include "scene_manager/scene_manager.h"

#define UUID_TONE           1

#define UUID_KEY            2
#define UUID_KEY_PWR        3
#define UUID_KEY_NEXT       4
#define UUID_KEY_PREV       5
#define UUID_KEY_SLIDER     6

#define UUID_IO             16
#define UUID_BT             17
#define UUID_TWS            18
#define UUID_LED            19
#define UUID_CLOCK          20
#define UUID_APP            21
#define UUID_BATTERY        22
#define UUID_EARTOUCH       23
#define UUID_AUDIO          24
#define UUID_ADIO           25      //通用AD采样IO

#define UUID_SYS_TIMER      40
#define UUID_COUNTER        41



struct scene_private_data {
    u8 local_action;        // 本地单独执行动作标志
};








#endif
