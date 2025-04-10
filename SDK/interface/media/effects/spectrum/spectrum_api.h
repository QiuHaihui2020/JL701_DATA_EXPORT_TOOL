#ifndef SPECTRUM_API_H
#define SPECTRUM_API_H
#include "effects/AudioEffect_DataType.h"
#include "visual_tool.h"

enum AlgorithmType {
    SPEC_PEAK = 0,
    SPEC_RMS
};

typedef struct RealTime_parameter {
    int fs;
    short lowpass_fc;
    short highpass_fc;
    float attack_time;//追踪时间
    float release_time;//释放时间
} rtpara;


typedef struct spectrum_parameter {
    int fs;
    int channel;//输入通道数  1单 2双
    visualp tool_para;
} specp;



int get_spectrum_buf(specp *spec_para);
int spectrum_init(void *ptr, specp *spec_para, af_DataType *adt);
void rms(void *ptr, void *in_buf, int len);
void get_Db(void *ptr, float *db);
void rms_para_updata(void *ptr, rtpara rtp);
void bpf_filts(void *ptr, specp *spec_para, float *lp_SOSMatrix, float *hp_SOSMatrix);

extern void butterworth_lp_design(int fc, int fs, int nSOS, float *coeff);//顺序按照硬件eq摆放
extern void butterworth_hp_design(int fc, int fs, int nSOS, float *coeff);
extern float exp_float(float x);
extern float ln_float(float x);
#endif // !spectrum


