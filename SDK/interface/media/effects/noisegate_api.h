
#ifndef NOISEGATE_API_H
#define NOISEGATE_API_H

#include "effects/AudioEffect_DataType.h"
#ifdef WIN32

#define AT_NOISEGATE(x)
#define AT_NOISEGATE_CODE
#define AT_NOISEGATE_CONST
#define AT_NOISEGATE_SPARSE_CODE
#define AT_NOISEGATE_SPARSE_CONST

#else
#define AT_NOISEGATE(x)           __attribute__((section(#x)))
#define AT_NOISEGATE_CODE         AT_NOISEGATE(.noisegate.text.cache.L2.code)
#define AT_NOISEGATE_CONST        AT_NOISEGATE(.noisegate.text.cache.L2.const)
#define AT_NOISEGATE_SPARSE_CODE  AT_NOISEGATE(.noisegate.text)
#define AT_NOISEGATE_SPARSE_CONST AT_NOISEGATE(.noisegate.text.const)
#endif
typedef struct _NoiseGateParam {
    int attackTime;
    int releaseTime;
    int threshold;
    int low_th_gain;
    int sampleRate;
    int channel;
    af_DataType pcm_info;
} NoiseGateParam;

typedef struct _NoiseGate_update_Param {
    int attackTime;  //启动时间
    int releaseTime; //释放时间
    int threshold;	  //阈值mdb  例如-65.5db 例如传下来应是-65500
    int low_th_gain;	//低于阈值增益  放大30bit 例如(int)(0.1 * (1 << 30))
} noisegate_update_param;

int noiseGate_buf();
int noiseGate_init(void *workbuf, NoiseGateParam *param);
int noiseGate_update(void *work_buf, NoiseGateParam *param);

int noiseGate_run(void *work_buf, short *in_buf, short *out_buf, int per_channel_npoint);

#endif // NOISEGATE_API_H

