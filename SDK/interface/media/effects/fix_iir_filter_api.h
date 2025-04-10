#ifndef FIX_IIR_FILTER_API_H
#define FIX_IIR_FILTER_API_H

#include "AudioEffect_DataType.h"

#ifdef WIN32
#define AT_IIR_FILTER(x)
#define AT_IIR_FILTER_CODE
#define AT_IIR_FILTER_CONST
#define AT_IIR_FILTER_SPARSE_CODE
#define AT_IIR_FILTER_SPARSE_CONST
#else
#define AT_IIR_FILTER(x)              __attribute__((section(#x)))
#define AT_IIR_FILTER_CODE			  AT_IIR_FILTER(.iir_filter.text.cache.L2.code)
#define AT_IIR_FILTER_CONST			  AT_IIR_FILTER(.iir_filter.text.cache.L2.const)
#define AT_IIR_FILTER_SPARSE_CODE     AT_IIR_FILTER(.iir_filter.text)
#define AT_IIR_FILTER_SPARSE_CONST    AT_IIR_FILTER(.iir_filter.text.const)
#endif

struct iir_filter_param {
    int channel;       //通道
    int n_section;     //段数，多级高低通滤波器，阶数不一样段数不一样
    float *sos_matrix;  //EQ 系数， 传一个通道的
    int reverse[8];     //保留位
    af_DataType pcm_info;
};

int need_fix_iir_filter_buf(struct iir_filter_param *param);
int fix_iir_filter_init(void *work_buf, struct iir_filter_param *param);
int fix_iir_filter_update(void *work_buf, struct iir_filter_param *param);
int fix_iir_filter_run(void *work_buf, void *indata, void *outdata, int per_channel_npoint);
int fix_iir_filter_clear_mem(void *work_buf);

#endif // !FIX_IIR_FILTER_API_H
