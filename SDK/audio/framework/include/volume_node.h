#ifndef _VOLUME_NODE_H_
#define _VOLUME_NODE_H_

#include "audio_config.h"

struct volume_cfg {
    u8  bypass;         //是否bypass掉当前节点,复用高4bit用于传递 cmd（VOLUME_NODE_CMD_SET_VOL,VOLUME_NODE_CMD_SET_MUTE）
    u16 cfg_level_max;    //最大音量等级
    s32 cfg_vol_min;      //最小音量,dB
    u8  vol_table_custom; //是否自定义音量表
    s32 cfg_vol_max;     //最大音量,dB
    s16 cur_vol;        //当前音量
#if VOL_TAB_CUSTOM_EN
    u16 tab_len;         //音量表的字节长度
#endif
    float vol_table[0];     //音量表
} __attribute__((packed));

//Volume Node Command List
#define VOLUME_NODE_CMD_SET_VOL   (1<<4)
#define VOLUME_NODE_CMD_SET_MUTE  (1<<5)

#define VOLUME_TABLE_CUSTOM_EN       2

//初步判断是否为音量结构体参数的阈值,高8位是 volume_cfg 成员cfg_level_max的低8位,低8位是bypass的8位,cfg_level_max 最小值为01,bypass最小值为0，故此处阈值设为0x0100;
#define VOL_CFG_THRESHOLD           ((1<<8 |0 ) - 1 )

int volume_ioc_get_cfg(const char *name, struct volume_cfg *vol_cfg);//获取名字对应节点的音量配置

u16 volume_ioc_get_max_level(const char *name);	//获取名字对应节点的最大音量

#endif

