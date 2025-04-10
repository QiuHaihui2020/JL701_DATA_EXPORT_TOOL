#ifndef VISUAL_TOOL_H
#define VISUAL_TOOL_H

typedef struct visual_parameter {
    char mode;//0计算左声道 1计算右声道 2计算双声道 3计算双声道的平均   实时修改后需重新初始化
    char algorithm;//0 peak 1 rms  实时修改后需重新初始化
    char order;//滤波器阶数  实时修改后需重新初始化
    char reserve0;
    short lowpass_fc;//低通截止频率  可实时修改
    short highpass_fc;//高通截止频率 可实时修改
    float attack_time;//追踪时间  可调用更新函数实时修改
    float release_time;//释放时间  可调用更新函数实时修改
    float rms_time;//rms区间  实时修改后需重新初始化
    int reserve[2];
} visualp;


#endif


