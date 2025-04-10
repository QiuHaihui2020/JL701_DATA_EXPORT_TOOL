#ifndef _SPATIAL_EFFECT_H_
#define _SPATIAL_EFFECT_H_
#include "system/includes.h"
#include "generic/typedef.h"
#include "app_config.h"
#include "tech_lib/effect_surTheta_api.h"
#include "tech_lib/3DSpatial_ctrl.h"

#define SPATIAL_AUDIO_EXPORT_DATA       0
#define SPATIAL_AUDIO_EXPORT_MODE       0

#define SPATIAL_SENSOR_TASK_NAME   "imu_sensor"

/*音效版本*/
#define SPATIAL_EFFECT_V1   0
#define SPATIAL_EFFECT_V2   1

/*音效使能*/
#define SPATIAL_AUDIO_EFFECT_ENABLE     1

#if TCFG_SPATIAL_AUDIO_SENSOR_ENABLE
#define SPATIAL_AUDIO_SENSOR_ENABLE     1/*传感器数据读取使能*/
#define SPACE_MOTION_CALCULATOR_ENABLE  1/*传感器角度计算使能*/
#define SPATIAL_AUDIO_ANGLE_TWS_SYNC    (TCFG_USER_TWS_ENABLE && SPATIAL_AUDIO_SENSOR_ENABLE && SPACE_MOTION_CALCULATOR_ENABLE)/*传感器角度TWS同步*/
#else /*TCFG_SPATIAL_AUDIO_SENSOR_ENABLE == 0*/
/*没有传感器的情况*/
#define SPATIAL_AUDIO_SENSOR_ENABLE     0/*传感器数据读取使能*/
#define SPACE_MOTION_CALCULATOR_ENABLE  0/*传感器角度计算使能*/
#define SPATIAL_AUDIO_ANGLE_TWS_SYNC    0/*传感器角度TWS同步*/
#endif /*TCFG_SPATIAL_AUDIO_SENSOR_ENABLE*/


/*头部跟踪灵敏度：范围 0.001 ~ 1，1：表示即时跟踪，数值越小跟踪越慢*/
#define TRACK_SENSITIVITY     (1.0f)

/*静止角度复位灵敏度：范围 0.001 ~ 1，1：表示立刻复位回正，数值越小回正越慢*/
#define ANGLE_RESET_SENSITIVITY     (0.01f)

/*头部跟踪角度复位时间,单位：秒*/
#define ANGLE_RESET_TIME      (8)

#if SPATIAL_AUDIO_EXPORT_DATA
struct data_export_header {
    u8 magic;
    u8 ch;
    u16 seqn;
    u16 crc;
    u16 len;
    u32 timestamp;
    u32 total_len;
    u8 data[0];
};
void audio_export_task(void *arg);
#endif

struct spatial_audio_context {
    void *sensor;
    void *effect;
    void *calculator;
#if SPATIAL_AUDIO_ANGLE_TWS_SYNC
    void *tws_conn;
    int tws_angle;
#endif /*SPATIAL_AUDIO_ANGLE_TWS_SYNC*/
    u8 mapping_channel;
    u32 head_tracked;
#if SPATIAL_AUDIO_EXPORT_DATA
    cbuffer_t audio_cbuf;
    cbuffer_t space_cbuf;
    u8 *audio_buf;
    u8 *space_buf;
    void *space_fifo_buf;
    u16 audio_seqn;
    u16 audio_out_seqn;
    u16 space_seqn;
    u32 audio_data_len;
    u32 space_data_len;
    u32 timestamp;
    int space_data_single_len;
    u8 export;
#if SPATIAL_AUDIO_EXPORT_MODE == 0
    void *audio_file;
    void *space_file;
#endif /*SPATIAL_AUDIO_EXPORT_MODE*/
#endif /*SPATIAL_AUDIO_EXPORT_DATA*/
    int sensor_task_state;
    int sensor_task_busy;
    u8 data[0];
};

typedef struct {
    float track_sensitivity;
    float angle_reset_sensitivity;
} angle_cfg_t;

typedef struct {
    /*传感器角度参数*/
    angle_cfg_t angle;
    /*第一版音效算法参数*/
    RP_PARM_CONIFG rp_parm;
    /*第二版音效算法参数*/
    af_DataType pcm_info;
    source_cfi scfi;
    source_ang sag;
    core cor;

} spatial_effect_cfg_t;

/*设置并更新混响参数*/
void spatial_effect_online_updata(void *params);

/*获取当前混响的参数*/
void get_spatial_effect_reverb_params(RP_PARM_CONIFG *params);

/*打开空间音效*/
void *spatial_audio_open(void);

/*关闭空间音效*/
void spatial_audio_close(void *sa);

/*音效处理*/
int spatial_audio_filter(void *sa, void *data, int len);

/*设置空间音频的输出声道映射*/
int spatial_audio_set_mapping_channel(void *sa, u8 channel);

/*读取传感器的数据*/
int spatial_audio_space_data_read(void *data);

/*头部跟踪使能*/
void spatial_audio_head_tracked_en(struct spatial_audio_context *ctx, u8 en);

/*获取头部跟踪使能状态*/
u8 get_spatial_audio_head_tracked(struct spatial_audio_context *ctx);

void spatial_audio_sensor_sleep(u8 en);

/*声道映射*/
int spatial_audio_remapping_data_handler(u8 mapping_channel, u8 bit_width, void *data, int len);

/*角度归0回正*/
void spatial_audio_angle_reset();

void spatial_effect_update_parm(u8 mode_index, char *node_name, u8 cfg_index);

/*获取节点位宽*/
u8 get_spatial_effect_node_bit_width();

/*获取节点bypass*/
u8 get_spatial_effect_node_bypass();

extern void put_float(double fv);
#endif
