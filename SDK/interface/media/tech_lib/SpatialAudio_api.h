#ifndef SPATIALAUDIO_API_H
#define SPATIALAUDIO_API_H

#include "sensor_common_parm.h"


typedef struct TRANVAL {
    int trans_x[3];
    int trans_y[3];
    int trans_z[3];

} tranval_t;

typedef struct COMMON_INFO {
    float fs;//采样率
    int len;//一包数据长度
    float sensitivity;//陀螺仪灵敏度
    int range;//加速度计量程（正）
} info_spa_t;

typedef struct {
    float beta;
    float val;//陀螺仪参考阈值
    float cel_val;//动态校准参考阈值
    float time;//动态校准时长
    float SerialTime;//动态校准窗长
    float sensval;//角度灵敏度，越小越灵敏，范围：0.01~0.1，默认0.1
} spatial_config_t;

extern inline float root_float(float x);
extern inline float angle_float(float x, float y);
int get_Spatial_buf(int len);
void init_Spatial(void *ptr, info_spa_t *, tranval_t *, spatial_config_t *, gyro_cel_t *, acc_cel_t *ac);
void Spatial_cacl(void *ptr, short *data);
/* alpha : 头部跟踪灵敏度：范围 0.001 ~ 1，1：表示即时跟踪，数值越小跟踪越慢
 * voloct : 静止角度复位灵敏度：范围 0.001 ~ 1，1：表示立刻复位回正，数值越小回正越慢*/
int get_Spa_angle(void *ptr, float alpha, float voloct);
void Spatial_reset(void *ptr);
/* time : 头部跟踪角度复位时间,单位：秒
 * val1 : 建议范围0.1~0.6；值越小，越接近静止条件下的回正，0.1为静止下的参考值*/
int Spatial_stra(void *ptr, int time, float val1);
int get_test_angle(void *ptr);
//int get_Pitch_angle(void* ptr);

#endif // !1


