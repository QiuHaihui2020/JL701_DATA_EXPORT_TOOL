#ifndef _SPATIAL_EFFECTS_PROCESS_
#define _SPATIAL_EFFECTS_PROCESS_

#include "system/includes.h"
#include "effects/audio_eq.h"
#include "app_config.h"

/*1：音效节点固定输出立体声，0：音效节点根据传进来的ch来输出单声道还是立体声*/
#define SPATIAL_AUDIO_EFFECT_OUT_STEREO_EN    1

/* 配置空间音效的起始节点前面的节点和空间音效结束节点的后一个节点
 * 用于配置开关空间音效*/
/*起始节点uuid和名字*/
#define BREAKER_SOURCE_NODE_UUID NODE_UUID_CONVERT
#define BREAKER_SOURCE_NODE_NEME NULL
/*目标节点uuid和名字*/
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
#define BREAKER_TARGER_NODE_UUID NODE_UUID_EQ
#define BREAKER_TARGER_NODE_NEME "MusicEq"
#else
#define BREAKER_TARGER_NODE_UUID NODE_UUID_CHANNEL_MERGE
#define BREAKER_TARGER_NODE_NEME NULL
#endif

/*数据流延时切换时间ms, 根据eq淡入淡出时间确定*/
#define BREAKER_DELAY_TIME  (1000)
/*删除断点恢复数据流后，延后切换音效的时间*/
#define EFFECT_DELAY_TIME  (500)

#define SPATIAL_EFX_TRACKED_CLK (10 * 1000000L)
#define SPATIAL_EFX_FIXED_CLK (16 * 1000000L)

//1、空间音效模式定义枚举
enum SPATIAL_EFX_MODE {
    SPATIAL_EFX_OFF = 0,		//空间音效关闭
    SPATIAL_EFX_FIXED,	//固定头部
    SPATIAL_EFX_TRACKED,	//头部跟踪
};

/*节点是否在跑*/
u8 spatial_effect_node_is_running();

/*资源初始化*/
int aud_spatial_sensor_init();
/*关闭资源*/
int aud_spatial_sensor_exit();
/*定时读取传感器数据到cbuf*/
int aud_spatial_sensor_run(void *priv, void *data, int len);
/*从cbuf读取传感器数据*/
int aud_spatial_sensor_data_read(s16 *data, int len);
/*当前传感器数据量*/
int aud_spatial_sensor_get_data_len();

u8 get_spatial_effects_busy();

void set_a2dp_spatial_audio_mode(enum SPATIAL_EFX_MODE mode);

enum SPATIAL_EFX_MODE get_a2dp_spatial_audio_mode(void);

/*打开空间音频*/
int audio_effect_process_start(void);

/*关闭空间音频*/
int audio_effect_process_stop(void);

/*数据处理*/
int audio_spatial_effects_data_handler(u8 out_channel, s16 *data, u16 len);

/*空间音频模式选择*/
void audio_spatial_effects_mode_switch(enum SPATIAL_EFX_MODE mode);

/*默认eq参数设置*/
int spatial_effect_eq_default_parm_set(char name[16], struct eq_default_parm *get_eq_parm);

int spatial_effects_node_param_cfg_read(void *cfg, int size);

#endif
