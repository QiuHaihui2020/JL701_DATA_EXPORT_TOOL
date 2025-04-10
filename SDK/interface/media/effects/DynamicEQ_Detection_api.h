#ifndef DYNAMICEQDETECTION_API_H
#define DYNAMICEQDETECTION_API_H

#include "effects/AudioEffect_DataType.h"
#ifdef WIN32

#define AT_DYNAMICEQ_DETECT(x)
#define AT_DYNAMICEQ_DETECT_CODE
#define AT_DYNAMICEQ_DETECT_CONST
#define AT_DYNAMICEQ_DETECT_SPARSE_CODE
#define AT_DYNAMICEQ_DETECT_SPARSE_CONST

#else

#define AT_DYNAMICEQ_DETECT(x)				__attribute__((section(#x)))
#define AT_DYNAMICEQ_DETECT_CODE			AT_DYNAMICEQ_DETECT(.dynamiceq_detect.text.cache.L2.code)
#define AT_DYNAMICEQ_DETECT_CONST			AT_DYNAMICEQ_DETECT(.dynamiceq_detect.text.cache.L2.const)
#define AT_DYNAMICEQ_DETECT_SPARSE_CODE		AT_DYNAMICEQ_DETECT(.dynamiceq_detect.text)
#define AT_DYNAMICEQ_DETECT_SPARSE_CONST	AT_DYNAMICEQ_DETECT(.dynamiceq_detect.text.const)

#endif
typedef struct _DynamicEQDetectionParam {
    int fc;                  //中心频率  与动态eq参数中的fc一致
} DynamicEQDetectionParam;   //检测参数

int getDynamicEQDetectionBuf(int nSection, int channel);
int *getDynamicEQDetectionCoeff(void *WorkBuf);
void DynamicEQDetectionInit(void *WorkBuf, DynamicEQDetectionParam *pram, int nSection, int channel, int SampleRate);
void DynamicEQDetectionUpdate(void *WorkBuf, DynamicEQDetectionParam *pram);
int DynamicEQDetectionRun(void *WorkBuf, short *indata, int *outdata, int per_channel_npoint);

#endif // !DYNAMICEQDETECTION_API_H

