SECTIONS
{
    .data ALIGN(32):SUBALIGN(4)
    {
        audio_cvp_data_begin = .;
	    . = ALIGN(4);
        *(.jlsp_aec_data)
        *(.jlsp_nlp_data)
        *(.jlsp_dns_data)
        *(.jlsp_enc_data)
        *(.jlsp_prep_data)
        *(.jlsp_wn_data)
        *(.jlsp_tri_data)
        *(.jlsp_agc_data)
        *(.jlsp_dms_hybrid_data)
        *(.jlsp_dms_awn_data)
        *(.sms_data)
        *(.sms_stereo_data)
        *(.dms_data)
        *(.data._*)
        *(.aec_data)
        *(.res_data)
        *(.ns_data)		
        *(.jlsp_data)
        *(.nlp_data)
        *(.der_data)
        *(.qmf_data)
        *(.fft_data)
        audio_cvp_data_end = .;
        audio_cvp_data_size = audio_cvp_data_end - audio_cvp_data_begin ;
	    . = ALIGN(4);
    } > ram0

    .bss (NOLOAD):SUBALIGN(4)
    {
	    audio_cvp_bss_begin = .;
        . = ALIGN(4);
        *(.sms_bss)
        *(.sms_stereo_bss)
        *(.aec_bss)
        *(.nlp_bss)
        *(.res_bss)
        *(.ns_bss)
        *(.der_bss)
        . = ALIGN(4);
        *(.jlsp_bss)
        *(.jlsp_aec_bss)
        *(.jlsp_nlp_bss)
        *(.jlsp_dns_bss)
        *(.jlsp_enc_bss)
        *(.jlsp_prep_bss)
        *(.jlsp_wn_bss)
        *(.jlsp_tri_bss)
        *(.jlsp_agc_bss)
        *(.jlsp_dms_hybrid_bss)
        *(.jlsp_dms_awn_bss)
	    audio_cvp_bss_end = .;
	    audio_cvp_bss_size = audio_cvp_bss_end - audio_cvp_bss_begin ;
    } > ram0

    .text ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);
        cvp_code_begin = . ;
        *(.dns_16k_data)
        *(.dns_8k_data)
        . = ALIGN(4);
		*(.jlsp_const)
		*(.jlsp_code)
        *(.jlsp_dns_code)
        *(.jlsp_dns_const)
        *(.dns_param_data_wind)
        *(.dns_common_data_wind)
        
		*(.ns_sparse_code )
		*(.aec_sparse_code)
		*(.nlp_sparse_code)
		*(.der_sparse_code)
		*(.qmf_sparse_code)
        *(.sms_sparse_code)
        *(.sms_stereo_sparse_code)
        *(.dms_sparse_code)
        *(.opcore_maskrom)
#if AUDIO_CVP_TEXT_AT_RAM == 0
        . = ALIGN(4);
        *(.text._*)
#endif
#if AUDIO_CVP_AEC_AT_RAM == 0
        *(.jlsp_aec_code)
		*(.jlsp_aec_const)
#endif
#if AUDIO_CVP_NLP_AT_RAM == 0
        *(.nlp_code)
        *(.nlp_const)
		*(.qmf_code)
		*(.qmf_const)
        *(.jlsp_nlp_code)
		*(.jlsp_nlp_const)
#endif
#if AUDIO_CVP_NS_AT_RAM == 0
		*(.ns_code)
		*(.ns_const)
#endif
#if AUDIO_CVP_COMMON_AT_RAM == 0
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
#if AUDIO_CVP_DNS_AT_RAM == 0
	    . = ALIGN(4);
        *(.dns_common_data)
        *(.dns_param_data_single)
        *(.dns_param_data_dual)
	    . = ALIGN(4);
#endif
#if AUDIO_CVP_AGC_AT_RAM == 0
        *(.agc_code)
        *(.agc_const)
        *(.jlsp_agc_code)
        *(.jlsp_agc_const)
#endif
#if AUDIO_CVP_DMS_AT_RAM == 0
        *(.dms_code)
        *(.dms_const)
        *(.jlsp_enc_code)
        *(.jlsp_enc_const)
        *(.jlsp_dms_hybrid_code)
        *(.jlsp_dms_hybrid_const)
		*(.jlsp_dms_awn_code)
        *(.jlsp_dms_awn_const)
#endif
#if AUDIO_CVP_SMS_AT_RAM == 0
        *(.sms_const)
        *(.sms_code)
        *(.sms_stereo_const)
        *(.sms_stereo_code)

#endif
#if AUDIO_CVP_PREP_AT_RAM == 0
        *(.jlsp_prep_code)
        *(.jlsp_prep_const)
#endif
#if AUDIO_CVP_WN_AT_RAM == 0
        *(.jlsp_wn_code)
        *(.jlsp_wn_const)
#endif
#if AUDIO_CVP_THIRD_AT_RAM == 0
        *(.jlsp_tri_code)
        *(.jlsp_tri_const)
#endif
		. = ALIGN(4);
		cvp_code_end = . ;
		cvp_code_size = cvp_code_end - cvp_code_begin ;

    } > code0

}

