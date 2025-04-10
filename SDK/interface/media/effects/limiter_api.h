#ifndef LIMITER_API
#define LIMITER_API

#include "AudioEffect_DataType.h"

#ifdef WIN32
#define AT_LIMITER(x)
#define AT_LIMITER_CODE
#define AT_LIMITER_CONST
#define AT_LIMITER_SPARSE_CODE
#define AT_LIMITER_SPARSE_CONST
#else
#define AT_LIMITER(x)            __attribute__((section(#x)))
#define AT_LIMITER_CODE			 AT_LIMITER(.limiter.text.cache.L2.code)
#define AT_LIMITER_CONST		 AT_LIMITER(.limiter.text.cache.L2.const)
#define AT_LIMITER_SPARSE_CODE	 AT_LIMITER(.limiter.text)
#define AT_LIMITER_SPARSE_CONST	 AT_LIMITER(.limiter.text.const)
#endif

struct limiter_param {
    int channel;
    int sample_rate;
    int attack_time;
    int release_time;
    int threshold;
    int hold_time;
    int detect_time;
    int input_gain;
    int output_gain;
    int precision;
    int resever[6];
    af_DataType pcm_info;
};

int get_limiter_buf(struct limiter_param *param);
int limiter_init(void *work_buf, struct limiter_param *param);
int limiter_update(void *work_buf, struct limiter_param *param);
int limiter_run(void *work_buf, void *indata, void *outdata, int per_channel_npoint);
int lib_limiter_version_1_0_0();

#endif // !LIMITER_PRO_API
