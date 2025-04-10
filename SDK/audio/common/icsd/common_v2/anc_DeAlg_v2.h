#ifndef _ANC_DEALG_V2_H
#define _ANC_DEALG_V2_H

#include "icsd_common_v2.h"
#define PI 1.0

#define DE_FLEN         70
#define SWARM_NUM 		60


struct icsd_anc_buf_v2 {
    float Wz_fd_init[DE_FLEN * 2];
    float Wz_fd_g[DE_FLEN * 2];
    float perfm[DE_FLEN];
    float mse[DE_FLEN * 2];
    float fitness_sv[60];
    float biquad_coef_g[31];
    float biquad_coef_co_g[31];
    float biquad_coef_List_i[SWARM_NUM][31];
    float biquad_coef_best_de[31];
    float mse_tmp[DE_FLEN];
    float target_gain_db[FLEN_V2];
};

struct icsd_De_param_v2 {
    float Vrange[62];
    float target[DE_FLEN * 2];
    float weight[DE_FLEN];
    float exp_w1[DE_FLEN * 2];
    float exp_w2[DE_FLEN * 2];

    float exp_w1_ndsp[DE_FLEN * 2];
    float exp_w2_ndsp[DE_FLEN * 2];

    float biquad_init_lcl[31];
    float biquad_init_fix[31];
    float fstop_idx1;
    float fstop_idx2;
    int iter_max;
    int iir_coef;
    int iir_num;
    int iir_num_fix;
    int ObjFunc_type;
    u8 type_anc[10];
    float mse_tar[DE_FLEN];
    float mse_tar_d3[DE_FLEN];

    float fs;
    int idx_50Hz;
    int idx_200Hz;
    int De_fast_en;
    float freqz[DE_FLEN];
    float freqz_ndsp[DE_FLEN];
    float gain_limit;
    float gain_limit_all;
    float over_mse_begin;
    float over_mse_end;
    float limit_mse_begin;
    float limit_mse_end;
    int swarm_num;
    int flen;
};

struct icsd_ff_candidate_v2 {
    float *Vrange_H;
    float *Vrange_M;
    float *Vrange_L;
    float *Biquad_init_H;
    float *Biquad_init_M;
    float *Biquad_init_L;
    float *Weight_H;
    float *Weight_M;
    float *Weight_L;
    float *Gold_csv_H;
    float *Gold_csv_M;
    float *Gold_csv_L;
    float *Gold_csv_Perf_Range;
    float *degree_set_0;
    float *degree_set_1;
    float *degree_set_2;

    u8 *biquad_type;
    float total_gain_adj_begin;
    float total_gain_adj_end;
    int IIR_NUM_FLEX;
    int IIR_NUM_FIX;
    int IIR_COEF;
    float  FSTOP_IDX;
    float  FSTOP_IDX2;

    int objFunc_type;
    float gain_limit_all;
};
//
struct icsd_norm_candidate_v2 {
    float *Vrange_M;
    float *Biquad_init_M;
    float *Weight_M;
    float *Gold_csv_M;
    float *Gold_csv_Perf_Range;
    float *degree_set_1;

    int *biquad_type;
    int target_cmp_num;
    float total_gain_adj_begin;
    float total_gain_adj_end;
    int IIR_NUM_FLEX;
    int IIR_NUM_FIX;
    int IIR_COEF;
    float  FSTOP_IDX;
    float  FSTOP_IDX2;

    float gain_limit_all;
};

void anc_de_init();
//void target_reshape(float *target, float *sz, int pz_reshape_en, int sz_reshape_en, int target_reshape_en);
//void target_cmp_out(float *target, float *freqz, float *sz, int *tight_degree, int ear_mem_en);
void get_weight_mse_by_tight_degree(struct icsd_ff_candidate_v2 *_FF_CANDI2, struct icsd_De_param_v2 *de_param, int tight_degree, int flen);
void get_biquad_by_tight_degree(struct icsd_ff_candidate_v2 *_FF_CANDI2, struct icsd_De_param_v2 *de_param, int tight_degree, int iir_num_flex, int iir_num_fix);
//void anc_core_main_ff_v2_3(float *target, float *freqz, float fs, int flen, float *total_gain, float *gfq_best, int tight_degree_MS, int tight_degree_self);


void De_reconfig(float *target, int flen, struct icsd_ff_candidate_v2 *_FF_CANDI2, struct icsd_De_param_v2 *de_param, int tight_degree_MS, int tight_degree_self, struct icsd_anc_buf_v2 *_ANC_BUF);
void DE_init(float *freqz, float fs, int flen, struct icsd_De_param_v2 *_de_param);
void anc_core_main_ff_v2_3(float *total_gain, float *gfq_best, struct icsd_De_param_v2 *de_param, struct icsd_anc_buf_v2 *_ANC_BUF);

void calWz_common(float *biquad_coef, int flen, int iir_num, float *exp_w1, float *exp_w2, float fs, float *Wz_fd, u8 *type);
void DeAlorithm_v3(struct icsd_De_param_v2 *de_param, float *biquad_coef_best, float *fitness, int iir_dim, int swarm_num, int flen, struct icsd_anc_buf_v2 *_ANC_BUF);
float cal_total_gain(int begin_idx, int end_idx, float *target, float *Wz_fd_init, float total_gain_default, float total_gain_limit_l, float total_gain_limit_h);

#endif
