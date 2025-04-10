#ifndef VIRTUALBASS_CLASSIC_API_H
#define VIRTUALBASS_CLASSIC_API_H

#include "AudioEffect_DataType.h"

extern const int virtual_bass_classic_run_mode;
extern const int butterworth_iir_filter_coeff_type_select;

#ifdef WIN32

#define AT_VBSS_ClASSIC(x)
#define AT_VBSS_ClASSIC_CODE
#define AT_VBSS_ClASSIC_CONST
#define AT_VBSS_ClASSIC_SPARSE_CODE
#define AT_VBSS_ClASSIC_SPARSE_CONST

#else
#define AT_VBSS_ClASSIC(x)           __attribute((section(#x)))
#define AT_VBSS_ClASSIC_CODE         AT_VBSS_ClASSIC(.vbss_classic.text.cache.L2.code)
#define AT_VBSS_ClASSIC_CONST        AT_VBSS_ClASSIC(.vbss_classic.text.cache.L2.const)
#define AT_VBSS_ClASSIC_SPARSE_CODE  AT_VBSS_ClASSIC(.vbss_classic.text)
#define AT_VBSS_ClASSIC_SPARSE_CONST AT_VBSS_ClASSIC(.vbss_classic.text.const)
#endif


struct virtual_bass_class_param {
    int attackTime;
    int releaseTime;
    int attack_hold_time;
    float threshold;
    int resever[10];
    int channel;
    int sample_rate;
    af_DataType pcm_info;
};


int get_virtual_bass_classic_buf(struct virtual_bass_class_param *param);
int virtual_bass_classic_init(void *WorkBuf, struct virtual_bass_class_param *param);
int virtual_bass_classic_update(void *WorkBuf, struct virtual_bass_class_param *param);
int virtual_bass_classic_run(void *WorkBuf, int *tmpbuf, void *in, void *out, int per_channel_npoint);


#endif // !VIRTUALBASS_CLASSIC_API_H

