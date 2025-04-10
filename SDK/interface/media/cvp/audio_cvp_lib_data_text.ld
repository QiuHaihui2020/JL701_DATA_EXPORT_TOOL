
        /*使用动态加载时*/
#ifdef CONFIG_CODE_MOVABLE_ENABLE
    . = ALIGN(4);
	cvp_data_code_begin = . ;
#if AUDIO_CVP_TEXT_AT_RAM
    . = ALIGN(4);
    *(.text._*)
#endif
#if AUDIO_CVP_AEC_AT_RAM
    *(.jlsp_aec_code)
    *(.jlsp_aec_const)
#endif
#if AUDIO_CVP_NLP_AT_RAM
    *(.nlp_code)
    *(.nlp_const)
    *(.qmf_code)
    *(.qmf_const)
    *(.jlsp_nlp_code)
    *(.jlsp_nlp_const)
#endif
#if AUDIO_CVP_NS_AT_RAM
    *(.ns_code)
    *(.ns_const)
#endif
#if AUDIO_CVP_COMMON_AT_RAM
    . = ALIGN(4);
    *(.aec_mem)
    *(.aec_mux)
    *(.aec_code)
    *(.aec_const)
    *(.res_code)
    *(.res_const)
    *(.bark_const)
    *(.fft_code)
    *(.fft_const)
    *(.der_code)
    *(.der_const)
#endif
#if AUDIO_CVP_DNS_AT_RAM
    . = ALIGN(4);
    *(.dns_common_data)
    *(.dns_param_data_single)
    *(.dns_param_data_dual)
    . = ALIGN(4);
#endif
#if AUDIO_CVP_AGC_AT_RAM
    *(.agc_code)
    *(.agc_const)
    *(.jlsp_agc_code)
    *(.jlsp_agc_const)
#endif
#if AUDIO_CVP_DMS_AT_RAM
    *(.dms_code)
    *(.dms_const)
    *(.jlsp_enc_code)
    *(.jlsp_enc_const)
    *(.jlsp_dms_hybrid_code)
    *(.jlsp_dms_hybrid_const)
    *(.jlsp_dms_awn_code)
    *(.jlsp_dms_awn_const)
#endif
#if AUDIO_CVP_SMS_AT_RAM
    *(.sms_const)
    *(.sms_code)
#endif
#if AUDIO_CVP_PREP_AT_RAM
    *(.jlsp_prep_code)
    *(.jlsp_prep_const)
#endif
#if AUDIO_CVP_WN_AT_RAM
    *(.jlsp_wn_code)
    *(.jlsp_wn_const)
#endif
#if AUDIO_CVP_THIRD_AT_RAM
    *(.jlsp_tri_code)
    *(.jlsp_tri_const)
#endif
    . = ALIGN(4);
    cvp_data_code_end = . ;
    cvp_data_code_size = cvp_data_code_end - cvp_data_code_begin ;
#endif /*CONFIG_CODE_MOVABLE_ENABLE*/ 

