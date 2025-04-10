#ifndef _ANC_V2_CONFIG_H
#define _ANC_V2_CONFIG_H

extern const u8 ICSD_EP_TYPE_V2;
extern const int FF_objFunc_type;
extern const float target_cmp_dat[];
extern const u8 ICSD_ANC_V2_BYPASS_ON_FIRST;

extern const float gfq_bypass[];
extern const u8 tap_bypass;
extern const u8 type_bypass[];
extern const float fs_bypass;
extern double bbbaa_bypass[1 * 5];

//
extern const float fgq_init_table[10 * 3];
extern const float fgq_init_table_fb[10 * 3];
extern const float fgq_init_table2[10 * 3];
extern const float tg_cmp[];
extern const float errweight[];
extern const float ref_spl_thr[];
extern const float err_spl_thr[];


//
extern const int cmp_idx_begin;
extern const int cmp_idx_end;
extern const int cmp_total_len;
extern const int cmp_en;
extern const float pz_gain;
extern const float target_sv[];
extern const float tg_cmp[];


extern const float Gold_csv_Perf_Range[];
extern float degree_set0[];
extern float degree_set1[];
extern float degree_set2[];
extern const float FSTOP_IDX;
extern const float FSTOP_IDX2;

//
extern const float cmp_Vrange_tmp_m[];
extern const float cmp_biquad_init_tmp_m[];
extern const float cmp_weight_m[];
extern const float cmp_mse_tar_m[];
//extern const int cmp_iir_type[];
extern const int ICSD_CMP_IIR_NUM_FLEX;
extern const int ICSD_CMP_IIR_NUM_FIX;
extern const int ICSD_CMP_IIR_COEF;
extern const float CMP_FSTOP_IDX;
extern const float CMP_FSTOP_IDX2;
extern const int   CMP_objFunc_type;


//
extern const float pz_table[];
extern const float sz_table[];
extern const float left_pz_table[];
extern const float left_sz_table[];
extern const float right_pz_table[];
extern const float right_sz_table[];


struct icsd_anc_v2_tool_data {
    int h_len;
    int yorderb;//int fb_yorder;
    int yorderf;//int ff_yorder;
    int yorderc;//int cmp_yorder;
    float *h_freq;
    float *data_out1;//float *hszpz_out_l;
    float *data_out2;//float *hpz_out_l;
    float *data_out3;//float *htarget_out_l;
    float *data_out4;//float *fb_fgq_l;
    float *data_out5;//float *ff_fgq_l;
    float *data_out6;//float *hszpz_out_r;
    float *data_out7;//float *hpz_out_r;
    float *data_out8;//float *htarget_out_r;
    float *data_out9;//float *fb_fgq_r;,
    float *data_out10;//float *ff_fgq_r;
    float *data_out11;//float *cmp_fgq_l;
    float *data_out12;//float *cmp_fgq_r;
    float *data_out13;//float *tool_target_out_l;
    float *data_out14;//float *tool_target_out_r;
    float *wz_temp;
    u8 *lff_iir_type;
    u8 *lfb_iir_type;
    u8 *lcmp_iir_type;
    u8 *rff_iir_type;
    u8 *rfb_iir_type;
    u8 *rcmp_iir_type;
    s8 calr_sign[4];// 顺序 sz_calr_sign、pz_calr_sign、bypass_calr_sign、perf_calr_sign
    u8 result_l;   // 左耳结果
    u8 result_r;   // 右耳结果
    u8 anc_combination;//FF,FB,CMP

};

enum {
    TFF_TFB = 0,
    TFF_DFB,
    DFF_TFB,
    DFF_DFB,
};
#endif/*_SD_ANC_LIB_V2_H*/
