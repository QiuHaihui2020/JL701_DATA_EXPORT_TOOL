#ifndef _AUDIO_distortion_clipping_API_H_
#define _AUDIO_distortion_clipping_API_H_
#include "system/includes.h"
#include "effects/distortion_api.h"

struct distortion_clipping_udpate {
    int distortion_type; 			// 失真处理类型 下拉菜单 Hardclip(0) / Softclip1(1) /Softclip2(2)
    float driver_dB; 			// 输入信号放大倍数(dB) 范围 0.00f ~ 80.0f  默认 0.0f  步进0.01 保留两位小数
    float clip_threshold;			// 削波阈值  范围0.1f ~ 1.0f  默认0.1f 步进0.01f 保留两位小数
    float softclip1_kneeWidth;		// softclip1的knee宽度  范围0.05f ~ 1.0f   默认0.1f 步进0.01f 保留两位小数
    float dry_wet;				// 干湿比  范围 0.0f ~ 1.0f 默认0.0f  步进0.01f 保留两位小数
    float volume; 				// 输出信号音量  范围 -30.0f ~ 30.0f 默认 0.0f 步进0.01f 保留两位小数
};

struct distortion_clipping_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct distortion_clipping_udpate parm;
};

#endif

