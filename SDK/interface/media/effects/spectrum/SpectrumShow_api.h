#ifndef SPECTRUMSHOW_API_H
#define SPECTRUMSHOW_API_H

#include "effects/AudioEffect_DataType.h"

#ifdef WIN32

#define AT_SPECTRUMSHOW(x)
#define AT_SPECTRUMSHOW_CODE
#define AT_SPECTRUMSHOW_CONST
#define AT_SPECTRUMSHOW_SPARSE_CODE
#define AT_SPECTRUMSHOW_SPARSE_CONST

#else
#define AT_SPECTRUMSHOW(x)           __attribute__((section(#x)))
#define AT_SPECTRUMSHOW_CODE         AT_SPECTRUMSHOW(.specshow.text.cache.L1)
#define AT_SPECTRUMSHOW_CONST        AT_SPECTRUMSHOW(.specshow.text.cache.L2.const)
#define AT_SPECTRUMSHOW_SPARSE_CODE  AT_SPECTRUMSHOW(.specshow.text)
#define AT_SPECTRUMSHOW_SPARSE_CONST AT_SPECTRUMSHOW(.specshow.text.const)
#endif

typedef struct _SpectrumShowParam {
    float attackFactor;//下降因子[0,1)
    float releaseFactor;//上升因子[0,1) */
    int SampleRate;
    int channel;
    int mode;//模式，双声道起作用，0 计算的是第一声道的频谱值，//1计算的是第二声道频谱值，2为第一声道与第二声道相加除2的频谱值
    af_DataType pcm_info;
} SpectrumShowParam;

int getSpectrumShowBuf();
int SpectrumShowInit(void *workBuf, SpectrumShowParam *param);
int SpectrumShowRun(void *workBuf, short *in, int len);
int getSpectrumNum(void *workBuf);
float *getCentreFreq(void *workBuf);
short *getSpectrumValue(void *workBuf);

#endif // !SPECTRUMSHOW_API_H
