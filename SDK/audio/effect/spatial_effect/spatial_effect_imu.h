/*****************************************************************
>file name : spatial_effect_imu.h
>create time : Mon 03 Jan 2022 02:44:09 PM CST
*****************************************************************/

#ifndef _SPATIAL_EFFECT_IMU_H_
#define _SPATIAL_EFFECT_IMU_H_

void *space_motion_detect_open(void);

void space_motion_detect_close(void *sensor);

int space_motion_data_read(void *sensor, void *data, int len);

void space_motion_sensor_sleep(void *sensor, u8 en);

#endif/*_SPATIAL_EFFECT_IMU_H_*/
