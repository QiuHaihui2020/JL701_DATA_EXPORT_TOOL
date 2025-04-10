#ifndef NOISEGATE_PRO_API_H
#define NOISEGATE_PRO_API_H

#include "AudioEffect_DataType.h"

#ifdef WIN32

#define AT_NOISEGATE_PRO(x)
#define AT_NOISEGATE_PRO_CODE
#define AT_NOISEGATE_PRO_CONST
#define AT_NOISEGATE_PRO_SPARSE_CODE
#define AT_NOISEGATE_PRO_SPARSE_CONST

#else
#define AT_NOISEGATE_PRO(x)           __attribute__((section(#x)))
#define AT_NOISEGATE_PRO_CODE         AT_NOISEGATE_PRO(.noisegate_pro.text.cache.L2.code)
#define AT_NOISEGATE_PRO_CONST        AT_NOISEGATE_PRO(.noisegate_pro.text.cache.L2.const)
#define AT_NOISEGATE_PRO_SPARSE_CODE  AT_NOISEGATE_PRO(.noisegate_pro.text)
#define AT_NOISEGATE_PRO_SPARSE_CONST AT_NOISEGATE_PRO(.noisegate_pro.text.const)
#endif

struct noisegate_pro_update_param {
    int attack_time;
    int release_time;
    //下面两个参数工具下发浮点，需要转换后再往下传
    int threshold;   //x * 1000
    int low_th_gain; //((int)(x * (1 << 20))
    int lp_fc;
    int hp_fc;
    int lp_order;
    int hp_order;
    int lp_bypass;
    int hp_bypass;
    int hold_time;
    int resever[3];
};

struct noisegate_pro_param {
    struct noisegate_pro_update_param update_param;
    int sample_rate;
    int channel;
    int detect_data_bit;
    int detect_data_inc;
    af_DataType pcm_info;
};

int get_noisegate_pro_buf(struct noisegate_pro_param *param);
int noisegate_pro_init(void *work_buf, struct noisegate_pro_param *param);
int noisegate_pro_update(void *work_buf, struct noisegate_pro_param *param);
int noisegate_pro_run(void *work_buf, void *detect_buf, void *in_buf, void *out_buf, int per_channel_npoint);
int lib_noisegate_pro_version_1_0_0();

#endif // NOISEGATE_API_H


