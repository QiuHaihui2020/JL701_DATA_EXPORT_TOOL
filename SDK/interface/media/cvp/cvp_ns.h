#ifndef _CVP_NS_H_
#define _CVP_NS_H_

#include "generic/typedef.h"

/******************************* ANS ***************************/
/*ans参数*/
typedef struct {
    char  wideband;
    char  mode;
    float AggressFactor;
    float MinSuppress;
    float NoiseLevel;
} noise_suppress_param;

int noise_suppress_frame_point_query(noise_suppress_param *param);
int noise_suppress_mem_query(noise_suppress_param *param);
int noise_suppress_open(noise_suppress_param *param);
int noise_suppress_close(void);
int noise_suppress_run(short *in, short *out, int npoint);

enum {
    NS_CMD_NOISE_FLOOR = 1,
    NS_CMD_LOWCUTTHR,
    NS_CMD_AGGRESSFACTOR,
    NS_CMD_MINSUPPRESS,
};
/*
*  (1)NOISEFLOOR : 设定噪声估计的最小阈值，低于阈值的信号不会被估计。
*                 主要为了应对手机处理过的信号中无声段的幅度很低，
*                 语音段又抬高导致的噪声估计失效问题。
* (2)LOWCUTTHR : 设定经过降噪后的信号清0阈值，低于此阈值的信号会被清0。
*                主要为了应对硬件FFT精度不够，反变换太小的值时会出现噪声。
*/
int noise_suppress_config(u32 cmd, int arg, void *priv);


/******************************* DNS ***************************/
/*dns参数*/
typedef struct {
    float DNS_OverDrive;    //降噪强度 range[0:6.0]
    float DNS_GainFloor;    //最小压制 range[0:1.00]
    float DNS_NoiseLevel;
    float DNS_highGain;     //EQ强度   range[1.0:3.5]
    float DNS_rbRate;       //混响强度 range[0:0.9]
    int sample_rate;
} dns_param_t;

int audio_dns_run(void *hdl, short *in, short *out, int len);
void *audio_dns_open(dns_param_t *param);
int audio_dns_close(void *hdl);
/*在线更新参数*/
int audio_dns_updata_param(void *hdl, float DNS_GainFloor, float DNS_OverDrive);


#endif/*_CVP_NS_H_*/
