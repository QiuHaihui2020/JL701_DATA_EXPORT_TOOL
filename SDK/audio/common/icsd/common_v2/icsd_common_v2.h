#ifndef _ICSD_COMMON_V2_H
#define _ICSD_COMMON_V2_H

#include "generic/typedef.h"
#include "math.h"
#include "stdlib.h"
#include "timer.h"
#include "asm/math_fast_function.h"

enum {
    DEBUG_ADT_STATE = 0,
    DEBUG_ADT_WIND,
    DEBUG_ADT_WIND_MIC_DATA,
    DEBUG_ADT_ENVNL,
    DEBUG_ADT_INF,
    DEBUG_ADT_VDT_TRAIN,
    DEBUG_ADT_RESET,
    DEBUG_ADT_DP_STATE,
};

enum {
    ICSD_FULL_INEAR = 0,
    ICSD_HALF_INEAR,
    ICSD_HEADSET,
};

enum {
    ICSD_BR50 = 0,
    ICSD_BR28,
};

#define ADT_DMA_BUF_LEN     	512
#define ADT_FFT_LENS   			256

#define TARLEN2   					120
#define TARLEN2_L					0 //40
#define DRPPNT2  					0 //10

#define FS 375000

#define FS_TANC   46875
#define FFTLEN   				1024
#define FFTLEN_L 				4096
#define SWARM_NUM 60
#define FLEN_V2 ((TARLEN2+TARLEN2_L-DRPPNT2)/2)  //96

#include "anc_DeAlg_v2.h"
//LIB调用的算术函数
float angle_float(float x, float y);

unsigned int hw_fft_config(int N, int log2N, int is_same_addr, int is_ifft, int is_real);
void hw_fft_run(unsigned int fft_config, const int *in, int *out);
void  icsd_common_version();

//---------------------------
// multi float  conj
//---------------------------
//---------------------------
// multi conj
//---------------------------

//---------------------------
// div
//---------------------------
//hard upspeed

extern double log10_anc(double x);

float icsd_anc_pow10(float n);
double icsd_log10_anc(double x);
double icsd_sqrt_anc(double x);

void icsd_complex_1plus_divf(float *input1, float *input2, float *out);
void icsd_complex_muln_dsmp(float *input1, float *input2, float *out, int len, int downsample);
void icsd_complex_conj_mulf_1p(float *input1, float *input2, float *out);

void icsd_anc_fft256(int *in, int *out);
void icsd_anc_fft128(int *in, int *out);
void icsd_anc_fft(int *in, int *out);
//--------------------------------------------
// biquad_operation
//--------------------------------------------
//void ff_fgq_2_aabb(double *iir_ab, float *ff_fgq);
void icsd_cal_wz_with_dsmp(float *ab, float gain, int tap, float *freq, float fs, float *wz, float *w1, float *w2, int len, int downsample);
void icsd_biquad2ab_out_3(float f, float gain, float q, float fs, float *a0, float *a1, float *a2, float *b0, float *b1, float *b2, u8 type);
void icsd_fgq_2_aabb(float *iir_ab, float *ff_fgq, u8 *ff_type, float fs, int order);
void icsd_fgq2hz(float gain, float *ff_fgq, u8 *ff_type, int order, float *freq, float *hz, float *w1, float *w2, int len, float fs, int downsample);
void icsd_gfq2hz(float gain, float *ff_gfq, u8 *ff_type, int order, float *freq, float *hz, float *w1, float *w2, int len, float fs, int downsample);
void icsd_cal_wz(double *ab, float gain, int tap, float *freq, float fs, float *wz, int len);
void icsd_fgq2hz_v2(float total_gain, float *fgq, u8 *type, u8 order, float *freq, float *hz, int len, float fs);
void icsd_biquad2ab_out_2(float gain, float f, float q, u8 type, float fs, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2);
void icsd_biquad2ab_double_v2(float gain, float f, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, u8 type, float fs);
void icsd_biquad2ab_out_v2(float gain, float f, float fs, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, u8 type);
void icsd_biquad2ab_double_pn(float gain, float f, float q, double *a0, double *a1, double *a2,
                              double *b0, double *b1, double *b2, u8 type);
//

//--------------------------------------------
// freq_response
//--------------------------------------------
void icsd_anc_h_freq_init(float *freq, u8 mode);
void icsd_hz2pxdB(float *hz, float *px, int len);
void icsd_HanningWin_pwr(s16 *input, int *output, int len);
void icsd_HanningWin_pwr_s1(s16 *input, int *output, int len);
void icsd_HanningWin_pwr_s2(s16 *input, int *output, int len);
void icsd_FFT_radix1024(int *in_cur, int *out);
void icsd_FFT_radix256(int *in_cur, int *out);
void icsd_FFT_radix128(int *in_cur, int *out);
void icsd_FFT_lowfreq4096(s16 *in_cur, float *out, int len);
void icsd_htarget(float *Hpz, float *Hsz, float *Hflt, int len);
void icsd_pxydivpxxpyy(float *pxy, float *pxx, float *pyy, float *out, int len);

//--------------------------------------------
//math
//--------------------------------------------
float icsd_get_abs(float a);
float icsd_abs_float(float f);
float icsd_cos_hq_v2(float x);
float icsd_sin_hq_v2(float x);
void icsd_complex_mulf(float *input1, float *input2, float *out, int len);
void icsd_complex_mul_v2(int *input1, float *input2, float *out, int len);
void icsd_unwrap(float *pha1, float *pha2);
void icsd_complex_div_v2(float *input1, float *input2, float *out, int len);
float icsd_mean_cal(float *data0, u8 len);
float icsd_min_cal(float *data, u8 len);
double icsd_devide_float(float data_0, float data_1);
double icsd_devide_double(double data_0, double data_1);
void icsd_complex_mul(int *input1, float *input2, float *out, int len);
void icsd_complex_muln(float *input1, float *input2, float *out, int len);
void icsd_complex_muln_2(float *input1, float *input2, float *input3, float *out, int len, float alpha);
void icsd_dsf8_2ch(int *ptr, int len, s16 *datah, s16 *datal);
void icsd_dsf8_4ch(int *ptr, int len, s16 *datah_l, s16 *datal_l, s16 *datah_r, s16 *datal_r);
void icsd_dsf4_2chto2ch(s16 *in1, s16 *in2, int len, s16 *datah, s16 *datal);
u8 icsd_cal_score(u8 valud, u8 *buf, u8 ind, u8 len);
void icsd_cic8_4ch(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2, s16 *output3, s16 *output4);
void icsd_cic8_2ch(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2);
void icsd_cic8_2ch_4order(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2);
float icsd_abs_float(float f);

extern const u8 ICSD_ANC_CPU;

#endif
