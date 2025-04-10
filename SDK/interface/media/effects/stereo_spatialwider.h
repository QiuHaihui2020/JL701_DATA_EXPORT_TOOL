#ifndef StereoSpatialWider_H
#define StereoSpatialWider_H
#include "AudioEffect_DataType.h"
#include "fix_iir_filter_api.h"

struct third_party_eq_func {
    int (*func_getrunbuf)(struct iir_filter_param *param);
    int (*func_init)(void *work_buf, struct iir_filter_param *param);
    int (*func_run)(void *work_buf, void *indata, void *outdata, int per_channel_npoint);
};

struct stereo_spatialwider_config {
    int channel;                          // 通道数
    int samplerate;                       //采样率
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
    struct iir_filter_param *stu_cfg;     //三方库参数结构体指针
    struct iir_filter_param *stu_cfg2;    //三方库参数结构体指针
    struct third_party_eq_func *stu_func; //三方库函数结构体指针
    int resever[10];                      //保留位
    af_DataType pcm_info;
};

struct stereo_spatialwider_func_api {
    unsigned int (*func_getrunbuf)(void *wf_cfg);
    unsigned int (*func_gettmpbuf)(void *wf_cfg);
    int (*func_init)(void *runbuf, void *wf_cfg, void *tempbuf);
    int (*func_set_tmpbuf)(void *runbuf, void *tempbuf);
    int (*func_run)(void *runbuf, void *stereoin, void *output, int per_channale_nbytepoint);
    int (*update)(void *runbuf, void *parm);
};

#ifdef __cplusplus
extern "C"
{
#endif
#if 0
// 获取处理帧长大小
int stereo_spatialwider_getminiframesize();
// 获取算法运行时所需固定空间大小
unsigned int stereo_spatialwider_querybufsize(struct stereo_spatialwider_config *wf_cfg);
// 获取算法运行时所需临时空间大小
unsigned int stereo_spatialwider_querytempbufsize(struct stereo_spatialwider_config *wf_cfg);
//算法临时空间指针设置接口
int stereo_spatialwider_settempbuf(void *runbuf, void *tempbuf);
//算法初始化接口
int stereo_spatialwider_init(void *runbuf, struct stereo_spatialwider_config *wf_cfg, void *tempbuf);
//算法处理接口
int stereo_spatialwider_process(void *runbuf,
                                void *stereoin,
                                void *output,
                                int npoint);
//算法额外预留配置接口（可用于初始化后或者p运行时动态调整相关参数）
int stereo_spatialwider_config(void *runbuf, void *configpara);
#else

extern struct stereo_spatialwider_func_api *get_stereo_spatialwider_func_api();
#endif
#ifdef __cplusplus
}
#endif
#endif