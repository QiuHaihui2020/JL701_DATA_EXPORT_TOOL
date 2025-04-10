#ifndef _HPEST_LOOP_
#define _HPEST_LOOP_

void anc_HpEst_loop(s16 *input0, s16 *input1, float *out0_sum, float *out1_sum, int len);
void anc_HpEst_loop_out(float *out0_sum, float *out1_sum, int flen);
void anc_HpEst_loop4096(s16 *input0, s16 *input1, float *out0_sum, float *out1_sum, int len);

void anc_HpEst_psd(s16 *input0, float *out0_sum, int len, int *cnt);
void anc_HpEst_psd_out(float *out0_sum, int flen, int cnt);

#endif

