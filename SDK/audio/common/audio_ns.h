#ifndef _AUDIO_NOISE_SUPPRESS_H_
#define _AUDIO_NOISE_SUPPRESS_H_

#include "generic/typedef.h"
#include "generic/circular_buf.h"
#include "cvp_ns.h"

/*降噪数据帧长(单位：点数)*/
#define ANS_FRAME_POINTS		256
#define ANS_FRAME_SIZE		(ANS_FRAME_POINTS << 1)
/*降噪输出buf长度*/
#define ANS_OUT_POINTS_MAX	(ANS_FRAME_POINTS << 1)

/*dns降噪数据帧长(单位：点数)*/
#define DNS_FRAME_POINTS		256
#define DNS_FRAME_SIZE	    (DNS_FRAME_POINTS << 1)
#define DNS_OUT_POINTS_MAX	(DNS_FRAME_POINTS << 1)
typedef struct {
    //s16 in[512];
    //cbuffer_t cbuf;
    noise_suppress_param ns_para;
} audio_ans_t;

/*
*********************************************************************
*                  	Noise Suppress Open
* Description: 初始化降噪模块
* Arguments  : sr				数据采样率
*			   mode				降噪模式(0,1,2:越大越耗资源，效果越好)
*			   NoiseLevel	 	初始噪声水平(评估初始噪声，加快收敛)
*			   AggressFactor	降噪强度(越大越强:1~2)
*			   MinSuppress		降噪最小压制(越小越强:0~1)
* Return	 : 降噪模块句柄
* Note(s)    : 采样率只支持8k、16k
*********************************************************************
*/
void *audio_ns_open(u16 sr, u8 mode, float NoiseLevel, float AggressFactor, float MinSuppress);

/*
*********************************************************************
*                  NoiseSuppress Process
* Description: 降噪处理主函数
* Arguments  : ns	降噪句柄
*			   in	输入数据
*			   out	输出数据
*			   len  输入数据长度
* Return	 : 降噪输出长度
* Note(s)    : 由于降噪是固定处理帧长的，所以如果输入数据长度不是降噪
*			   帧长整数倍，则某一次会输出0长度，即没有输出
*********************************************************************
*/
int audio_ns_run(void *ns, short *in, short *out, u16 len);

/*
*********************************************************************
*                  	Noise Suppress Close
* Description: 关闭降噪模块
* Arguments  : ns 降噪模块句柄
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_ns_close(void *ns);

#endif/*_AUDIO_NOISE_SUPPRESS_H_*/
