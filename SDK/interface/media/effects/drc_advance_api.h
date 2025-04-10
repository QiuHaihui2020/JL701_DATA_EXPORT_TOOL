#ifndef DRC_API_ADVANCE_H
#define DRC_API_ADVANCE_H

#include "AudioEffect_DataType.h"
#include "drc_api.h"

#if 0
#ifdef WIN32

#define AT_DRC(x)
#define AT_DRC_CODE
#define AT_DRC_CONST
#define AT_DRC_SPARSE_CODE
#define AT_DRC_SPARSE_CONST

#else
#define AT_DRC(x)           __attribute((section(#x)))
#define AT_DRC_CODE         AT_DRC(.drc_advance.text.cache.L2.code)
#define AT_DRC_CONST        AT_DRC(.drc_advance.text.cache.L2.const)
#define AT_DRC_SPARSE_CODE  AT_DRC(.drc_advance.text)
#define AT_DRC_SPARSE_CONST AT_DRC(.drc_advance.text.const)
#endif


enum {
    PEAK = 0,
    RMS
};

enum {
    PERPOINT = 0,
    TWOPOINT
};
#endif

typedef struct _DrcAdvanceParam {
    int channel;
    int sampleRate;
    float attackTime;
    float releaseTime;
    float compress_attackTime;
    float compress_releaseTime;
    float rmsTime;
    float DetectTime;
    float InputGain;
    float OutputGain;
    int ThresholdNum;
    float MaxKneeWidth;
    unsigned char algorithm;
    unsigned char mode;
    unsigned char flag;
    unsigned char reverved;
    int reverved1[4];
    float *threshold;
    af_DataType pcm_info;
} DrcAdvanceParam;

int GetDrcAdvanceBuf(DrcAdvanceParam *param); // bufsize 与 algorithm，rmsTime channel、 sampleRate有关
int DrcAdvanceInit(void *WorkBuf, DrcAdvanceParam *param);
void DrcAdvanceUpdate(void *WorkBuf, DrcAdvanceParam *param);
int DrcAdvanceRun(void *WorkBuf, void *indata, void *outdata, int per_channel_npoint);
int lib_drc_advance_version_1_0_0();

#endif // !DRC_ADVABCE_API_H

