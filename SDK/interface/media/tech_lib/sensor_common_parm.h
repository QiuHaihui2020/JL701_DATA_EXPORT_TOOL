#ifndef _SENSOR_COMMON_PARM_H_
#define _SENSOR_COMMON_PARM_H_

//陀螺仪偏置
typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
} gyro_cel_t;

//加速度计偏置
typedef struct {
    float acc_offx;
    float acc_offy;
    float acc_offz;
} acc_cel_t;

#endif

