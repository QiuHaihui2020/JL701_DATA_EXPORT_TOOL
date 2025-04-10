#ifndef _AUDIO_STEREO_SPATIAL_WIDER_API_H_
#define _AUDIO_STEREO_SPATIAL_WIDER_API_H_
#include "system/includes.h"
#include "effects/stereo_spatialwider.h"
#define TOOL_TYPE
struct stereo_spatial_wider_udpate {
    float fixL_delay;                     // L声道固定时延（ms）
    float fixR_delay;                     // R声道固定时延（ms）defaul=0, widermode=2 take effect
    float dyn_delay;                      //动态时延（ms）
    float dyn_freq;                       //动态变化频率（ms）
    float dyn_period_control;             //动态变化周期控制[0,1]
    int dyn_genmode;                      //控制动态模式下生成随机因子方式
    int widermode;                        //控制wider模式（改变wider方式）
    float width_ratio;                    // wider强度因子[0，1]
    float width_ratio_max;                // wider强度映射最大值[0，10], widermode=2 take effect
    int inverse;                          // 控制第二通道数据反向
    int resever[10];                      //保留位
};
/*
 *open时指定的eq系数表
 * */
struct filter_cfg {
    u16 nsection;
    void *filter;
};

/*
 *中途更新eq系数表
 * */
struct iir_filter_cfg {
    u16 cmd;  //0:iir0 1:iir1
    struct filter_cfg fcfg;
};


struct stereo_spatial_wider_param_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    int type;      //0:TOOL_PARAM_SET 1:PRIVATE_PARAM_SET,界面内隐藏，默认是0
    union {
        struct stereo_spatial_wider_udpate parm; //工具界面参数
        struct iir_filter_cfg iir_cfg; //私有参数
    };
};

#endif

