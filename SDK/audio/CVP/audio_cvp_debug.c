#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp_debug.data.bss")
#pragma data_seg(".audio_cvp_debug.data")
#pragma const_seg(".audio_cvp_debug.text.const")
#pragma code_seg(".audio_cvp_debug.text")
#endif

#include "audio_cvp.h"

#define ModuleEnable_Max	7
static const char *ModuleEnable_Name[ModuleEnable_Max] = {
    "AEC", "NLP", "ANS", "ENC", "AGC", "WNC", "MFDT"
};

// #define put_float(a) printf("%d / 1000", (int)(a * 1000))

#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
void aec_param_dump(struct tms_attr *param)
{
#ifdef CONFIG_DEBUG_ENABLE
    printf("===========dump dms param==================\n");
    printf("EnableBit:%x\n", param->EnableBit);
    for (u8 i = 0; i < ModuleEnable_Max; i++) {
        if (param->EnableBit & BIT(i)) {
            printf("%s ON", ModuleEnable_Name[i]);
        } else {
            printf("%s OFF", ModuleEnable_Name[i]);
        }
    }
    printf("ul_eq_en:%x\n", param->ul_eq_en);
    extern void put_float(double fv);
    printf("******************* AGC ********************");
    printf("agc_type : %d", param->agc_type);
    if (param->agc_type == AGC_EXTERNAL) {
        puts("AGC_NDT_fade_in_step:");
        put_float(param->AGC_NDT_fade_in_step);
        puts("AGC_NDT_fade_out_step:");
        put_float(param->AGC_NDT_fade_out_step);
        puts("AGC_DT_fade_in_step:");
        put_float(param->AGC_DT_fade_in_step);
        puts("AGC_DT_fade_out_step:");
        put_float(param->AGC_DT_fade_out_step);
        puts("AGC_NDT_max_gain:");
        put_float(param->AGC_NDT_max_gain);
        puts("AGC_NDT_min_gain:");
        put_float(param->AGC_NDT_min_gain);
        puts("AGC_NDT_speech_thr:");
        put_float(param->AGC_NDT_speech_thr);
        puts("AGC_DT_max_gain:");
        put_float(param->AGC_DT_max_gain);
        puts("AGC_DT_min_gain:");
        put_float(param->AGC_DT_min_gain);
        puts("AGC_DT_speech_thr:");
        put_float(param->AGC_DT_speech_thr);
        puts("AGC_echo_present_thr:");
        put_float(param->AGC_echo_present_thr);
    } else {
        printf("min_mag_db_level : %d", param->min_mag_db_level);
        printf("max_mag_db_level : %d", param->max_mag_db_level);
        printf("addition_mag_db_level : %d", param->addition_mag_db_level);
        printf("clip_mag_db_level : %d", param->clip_mag_db_level);
        printf("floor_mag_db_level : %d", param->floor_mag_db_level);
    }

    printf("******************* AEC ********************");
    puts("aec_process_maxfrequency:");
    put_float(param->aec_process_maxfrequency);
    puts("aec_process_minfrequency:");
    put_float(param->aec_process_minfrequency);
    puts("aec_af_length:");
    put_float(param->af_length);

    printf("******************* NLP ********************");
    puts("nlp_process_maxfrequency");
    put_float(param->nlp_process_maxfrequency);
    puts("nlp_process_minfrequency:");
    put_float(param->nlp_process_minfrequency);
    puts("nlp_overdrive:");
    put_float(param->overdrive);

    printf("******************* DNS ********************");
    puts("DNS_AggressFactor:");
    put_float(param->aggressfactor);
    puts("DNS_MinSuppress:");
    put_float(param->minsuppress);
    puts("DNC_init_noise_lvl:");
    put_float(param->init_noise_lvl);
    puts("DNS_highGain:");
    put_float(param->DNS_highGain);
    puts("DNS_rbRate:");
    put_float(param->DNS_rbRate);
    printf("enhance_flag:%d", param->enhance_flag);

    printf("******************* ENC ********************");
    printf("Tri_CutTh:%d", param->Tri_CutTh);
    puts("Tri_SnrThreshold0:");
    put_float(param->Tri_SnrThreshold0);
    puts("Tri_SnrThreshold1:");
    put_float(param->Tri_SnrThreshold1);
    puts("Tri_FbAlignedDb:");
    put_float(param->Tri_FbAlignedDb);
    puts("Tri_FbCompenDb:");
    put_float(param->Tri_FbCompenDb);
    printf("Tri_TfEqSel:%d", param->Tri_TfEqSel);
    printf("enc_process_maxfreq:%d", param->enc_process_maxfreq);
    printf("enc_process_minfreq:%d", param->enc_process_minfreq);
    printf("sir_maxfreq:%d", param->sir_maxfreq);
    puts("mic_distance:");
    put_float(param->mic_distance);
    puts("target_signal_degradation:");
    put_float(param->target_signal_degradation);
    puts("enc_aggressfactor:");
    put_float(param->enc_aggressfactor);
    puts("enc_minsuppress:");
    put_float(param->enc_minsuppress);
    puts("Tri_CompenDb:");
    put_float(param->Tri_CompenDb);
    printf("Tri_Bf_Enhance:%d", param->Tri_Bf_Enhance);
    printf("Tri_LowFreqHoldEn:%d", param->Tri_LowFreqHoldEn);
    puts("Tri_SupressFactor:");
    put_float(param->Tri_SupressFactor);

    printf("******************* WNC ********************");
    puts("wn_msc_th:");
    put_float(param->wn_msc_th);
    puts("ms_th:");
    put_float(param->ms_th);
    puts("wn_gain_offset:");
    put_float(param->wn_gain_offset);

    printf("******************* MFDT ********************");
    puts("MFDT detect_time:");
    put_float(param->detect_time);
    puts("MFDT detect_eng_diff_thr:");
    put_float(param->detect_eng_diff_thr);
    puts("MFDT detect_eng_lowerbound:");
    put_float(param->detect_eng_lowerbound);
    printf("MalfuncDet_MaxFrequency:%d", param->MalfuncDet_MaxFrequency);
    printf("MalfuncDet_MinFrequency:%d", param->MalfuncDet_MinFrequency);
    printf("MFDT OnlyDetect:%d", param->OnlyDetect);

    printf("===============End==================\n");
#endif/*CONFIG_DEBUG_ENABLE*/
}
#elif TCFG_AUDIO_DUAL_MIC_ENABLE
void aec_param_dump(struct dms_attr *param)
{
#if CONFIG_DEBUG_ENABLE
    printf("===========dump dms param==================\n");
    printf("EnableBit:%x\n", param->EnableBit);
    for (u8 i = 0; i < ModuleEnable_Max; i++) {
        if (param->EnableBit & BIT(i)) {
            printf("%s ON", ModuleEnable_Name[i]);
        } else {
            printf("%s OFF", ModuleEnable_Name[i]);
        }
    }
    printf("ul_eq_en:%x\n", param->ul_eq_en);
    extern void put_float(double fv);
    printf("******************* AGC ********************");
    printf("agc_type : %d", param->agc_type);
    if (param->agc_type == AGC_INTERNAL) {
        printf("min_mag_db_level : %d", param->min_mag_db_level);
        printf("max_mag_db_level : %d", param->max_mag_db_level);
        printf("addition_mag_db_level : %d", param->addition_mag_db_level);
        printf("clip_mag_db_level : %d", param->clip_mag_db_level);
        printf("floor_mag_db_level : %d", param->floor_mag_db_level);
    } else {
        puts("AGC_NDT_fade_in_step:");
        put_float(param->AGC_NDT_fade_in_step);
        puts("AGC_NDT_fade_out_step:");
        put_float(param->AGC_NDT_fade_out_step);
        puts("AGC_DT_fade_in_step:");
        put_float(param->AGC_DT_fade_in_step);
        puts("AGC_DT_fade_out_step:");
        put_float(param->AGC_DT_fade_out_step);
        puts("AGC_NDT_max_gain:");
        put_float(param->AGC_NDT_max_gain);
        puts("AGC_NDT_min_gain:");
        put_float(param->AGC_NDT_min_gain);
        puts("AGC_NDT_speech_thr:");
        put_float(param->AGC_NDT_speech_thr);
        puts("AGC_DT_max_gain:");
        put_float(param->AGC_DT_max_gain);
        puts("AGC_DT_min_gain:");
        put_float(param->AGC_DT_min_gain);
        puts("AGC_DT_speech_thr:");
        put_float(param->AGC_DT_speech_thr);
        puts("AGC_echo_present_thr:");
        put_float(param->AGC_echo_present_thr);
    }

    printf("******************* AEC ********************");
    puts("aec_process_maxfrequency:");
    put_float(param->aec_process_maxfrequency);
    puts("aec_process_minfrequency:");
    put_float(param->aec_process_minfrequency);
    puts("aec_af_length:");
    put_float(param->af_length);

    printf("******************* NLP ********************");
    puts("nlp_process_maxfrequency");
    put_float(param->nlp_process_maxfrequency);
    puts("nlp_process_minfrequency:");
    put_float(param->nlp_process_minfrequency);
    puts("nlp_overdrive:");
    put_float(param->overdrive);

    printf("******************* NS ********************");
#if TCFG_AUDIO_DMS_SEL == DMS_HYBRID
    printf("dns_process_maxfrequency:%d", param->dns_process_maxfrequency);
    printf("dns_process_minfrequency:%d", param->dns_process_minfrequency);
#endif
    puts("NS_AggressFactor:");
    put_float(param->aggressfactor);
    puts("NS_MinSuppress:");
    put_float(param->minsuppress);
    puts("NS_init_noise_lvl:");
    put_float(param->init_noise_lvl);

    printf("******************* ENC ********************");
    printf("enc_process_maxfreq:%d", param->enc_process_maxfreq);
    printf("enc_process_minfreq:%d", param->enc_process_minfreq);
#if TCFG_AUDIO_DMS_SEL == DMS_HYBRID
    puts("snr_db_T0:");
    put_float(param->snr_db_T0);
    puts("snr_db_T1:");
    put_float(param->snr_db_T1);
    puts("floor_noise_db_T:");
    put_float(param->floor_noise_db_T);
    puts("compen_db:");
    put_float(param->compen_db);

    printf("******************* WNC ********************");
    puts("coh_val_T:");
    put_float(param->coh_val_T);
    puts("eng_db_T:");
    put_float(param->eng_db_T);
#else
    printf("sir_maxfreq:%d", param->sir_maxfreq);
    puts("mic_distance:");
    put_float(param->mic_distance);
    puts("target_signal_degradation:");
    put_float(param->target_signal_degradation);
    puts("enc_aggressfactor:");
    put_float(param->enc_aggressfactor);
    puts("enc_minsuppress:");
    put_float(param->enc_minsuppress);
    puts("Disconverge_ERLE_Thr:");
    put_float(param->Disconverge_ERLE_Thr);

    puts("GloabalMinSuppress:");
    put_float(param->global_minsuppress);

    printf("******************* MFDT ********************");
    puts("MFDT detect_time:");
    put_float(param->detect_time);
    puts("MFDT detect_eng_diff_thr:");
    put_float(param->detect_eng_diff_thr);
    puts("MFDT detect_eng_lowerbound:");
    put_float(param->detect_eng_lowerbound);
    printf("MalfuncDet_MaxFrequency:%d", param->MalfuncDet_MaxFrequency);
    printf("MalfuncDet_MinFrequency:%d", param->MalfuncDet_MinFrequency);
    printf("MFDT OnlyDetect:%d", param->OnlyDetect);
#endif
    printf("adc_ref_en:%d", param->adc_ref_en);
    printf("output_sel:%d", param->output_sel);

    printf("===============End==================\n");
#endif/*CONFIG_DEBUG_ENABLE*/
}
#else
void aec_param_dump(struct aec_s_attr *param)
{
#if CONFIG_DEBUG_ENABLE
    printf("===========dump sms param==================\n");
    printf("toggle:%d\n", param->toggle);
    printf("EnableBit:%x\n", param->EnableBit);
    printf("ul_eq_en:%x\n", param->ul_eq_en);
    extern void put_float(double fv);
    puts("AGC_NDT_fade_in_step:");
    put_float(param->AGC_NDT_fade_in_step);
    puts("AGC_NDT_fade_out_step:");
    put_float(param->AGC_NDT_fade_out_step);
    puts("AGC_DT_fade_in_step:");
    put_float(param->AGC_DT_fade_in_step);
    puts("AGC_DT_fade_out_step:");
    put_float(param->AGC_DT_fade_out_step);
    puts("AGC_NDT_max_gain:");
    put_float(param->AGC_NDT_max_gain);
    puts("AGC_NDT_min_gain:");
    put_float(param->AGC_NDT_min_gain);
    puts("AGC_NDT_speech_thr:");
    put_float(param->AGC_NDT_speech_thr);
    puts("AGC_DT_max_gain:");
    put_float(param->AGC_DT_max_gain);
    puts("AGC_DT_min_gain:");
    put_float(param->AGC_DT_min_gain);
    puts("AGC_DT_speech_thr:");
    put_float(param->AGC_DT_speech_thr);
    puts("AGC_echo_present_thr:");
    put_float(param->AGC_echo_present_thr);
    puts("AEC_DT_AggressiveFactor:");
    put_float(param->AEC_DT_AggressiveFactor);
    puts("AEC_RefEngThr:");
    put_float(param->AEC_RefEngThr);
    puts("ES_AggressFactor:");
    put_float(param->ES_AggressFactor);
    puts("ES_MinSuppress:");
    put_float(param->ES_MinSuppress);
    puts("NS_AggressFactor:");
    put_float(param->ANS_AggressFactor);
    puts("NS_MinSuppress:");
    put_float(param->ANS_MinSuppress);
    puts("init_noise_lvl:");
    put_float(param->ANS_NoiseLevel);
#endif/*CONFIG_DEBUG_ENABLE*/
}
#endif/*CONFIG_DEBUG_ENABLE*/
