#include "cvp/audio_cvp_lib_data_text.ld"

    media_data_code_start = .;
    . = ALIGN(4);
    /*libkws_commin.a*/
#if (AUDIO_KWS_COMMON_AT_RAM)
    *(.jlsp_kws_common_code)
    *(.jlsp_kws_common_const)
#endif
    /*JL_Phone_Call.a*/
#if (AUDIO_KWS_YES_NO_AT_RAM)
    *(.jlsp_kws_yesno_code)
    *(.jlsp_kws_yesno_const)
#endif
    /*libjlsp_kws.a*/
#if (AUDIO_KWS_CHINESE_AT_RAM == 1)
    *(.kws_common_data_multi_26k)
    *(.kws_common_data_multi_57k)
    *(.jlsp_kws_music_code)
    *(.jlsp_kws_music_const)
    *(.jlsp_kws_phone_call_code)
    *(.jlsp_kws_phone_call_const)
#elif (AUDIO_KWS_CHINESE_AT_RAM == 2)
    *(.kws_common_data_multi_26k)
    *(.jlsp_kws_music_code)
    *(.jlsp_kws_music_const)
#endif
    /*libjlsp_kws_india_english.a*/
#if (AUDIO_KWS_INDIA_ENGLISH_AT_RAM)
    *(.jlsp_kws_india_english_music_code)
    *(.jlsp_kws_india_english_music_const)
    *(.jlsp_kws_india_english_phone_call_code)
    *(.jlsp_kws_india_english_phone_call_const)
#endif
    /*libjlsp_kws_far_keyword.a*/
#if (AUDIO_KWS_CHINESE_FAR_AT_RAM)
    *(.jlsp_far_keyword_const)
    *(.jlsp_far_keyword_code)
    *(.jlsp_far_front_code)
    *(.jlsp_far_front_const)
#endif

    /*使用动态加载时*/
#ifdef CONFIG_CODE_MOVABLE_ENABLE
#if AUDIO_MSBC_CODEC_AT_RAM
    *(.msbc_enc)
    *(.msbc_code)
    *(.msbc_const)
    *(.msbc_codec_code)
    *(.msbc_codec_const)
#endif
#if AUDIO_CVSD_CODEC_AT_RAM
    *(.cvsd_codec)
    *(.cvsd_code)
    *(.cvsd_const)
#endif
#if AUD_AAC_DEC_AT_RAM
    aac_dec_code_begin = .;
    *(.bt_aac_dec_code)
    *(.bt_aac_dec_sparse_code)
    *(.bt_aac_dec.text.cache.L2.run)
    aac_dec_code_end = .;
    aac_dec_code_size  = aac_dec_code_end - aac_dec_code_begin ;

    . = ALIGN(4);
    bt_aac_dec_const_begin = .;
    *(.bt_aac_dec_const)
    *(.bt_aac_dec_sparse_const)
*(.bt_aac_dec.text.cache.L2.const)
    bt_aac_dec_const_end = .;
    bt_aac_dec_const_size = bt_aac_dec_const_end -  bt_aac_dec_const_begin ;

    . = ALIGN(4);
    *(.aac_mem)
    *(.aac_ctrl_mem)
#endif
#endif /*CONFIG_CODE_MOVABLE_ENABLE*/ 

    . = ALIGN(4);
    *(.pSOUND360TD_cal_sparse_code)
    *(.pSOUND360TD_cal_code)
    *(.pSOUND360TD_cal_const)
    *(.spatial_mem)
    *(.TDSpatial.text)
    *(.TDSpatial.text.cache.L2)
#if (TCFG_CODE_RUN_RAM_FM_MODE)
    *(.sbc_eng_code)
    *(.pcm_code)
    *(.pcm_const)
    *(.audio_dec_pcm_code)
    *(.audio_dec_pcm_const)
    *(.stream_code_nor_run)			// 数据流普通输出处理
    *(.mixer_code_nor_run)			// mixer数据流直通输出
    *(.audio_codec_code)			// 解码任务流程
    *(.audio_codec_code_read_file)	// 解码读文件
    *(.audio_src_code_nor_run)		// audio_src运行处理
    *(.src_base_code_nor_run)		// src_base运行处理
    *(.audio_dac_code_nor_run)		// audio_dac普通输出运行处理
    *(.audio_cfifo_code_nor_run)	// audio_cfifo普通运行处理
    *(.audio_track_code)
    *(.audio_track_const)
#endif /*(TCFG_CODE_RUN_RAM_FM_MODE)*/

#if AUDIO_EFFECTS_SURROUND_AT_RAM 
    *(.audio_surround_code)
    *(.audio_surround_const)
    *(.sur_cal_common_code)
    *(.sur_TYPE0_cal_code)
    *(.sur_TYPE1_cal_code)
    *(.sur_TYPE0_cal_const)
    *(.sur_TYPE1_cal_const)
    /* *(.sur_LRDRIFT2_cal_code) */
    /* *(.sur_LRDRIFT2_cal_const) */
    /* *(.sur_ROTATE_cal_code) */
    /* *(.sur_ROTATE_cal_const) */
    /* *(.sur_LRDRIFT_cal_code) */
    /* *(.sur_LRDRIFT_cal_const) */
#endif

#if AFX_LIMITER_AT_RAM
	*(.limiter.text)
    *(.limiter.text.const)
	*(.limiter.text.cache.L2.code)
	*(.limiter.text.cache.L2.const)
#endif
#if AFX_MULTIBAND_LIMITER_AT_RAM
	*(.multiband_limiter.text)
	*(.multiband_limiter.text.const)
#endif
#if AFX_MULTIBAND_CROSSOVER_AT_RAM
    *(.multiband_crossover.text)
	*(.multiband_crossover.text.const)
#endif
#if AFX_MULTIBAND_DRC_AT_RAM
    *(.multiband_drc.text)
	*(.multiband_drc.text.const)
#endif
#if AFX_VIRTUAL_SURRUOUND_PRO_AT_RAM
	*(.virtual_surround_pro.text)
	*(.virtual_surround_pro.text.const)
	*(.sas.text.cache.L2.code)
	*(.sas.text.cache.L2.const)
    *(.sas.text)
    *(.sas.text.code)
#endif
#if AFX_SW_EQ_AT_RAM 
    *(.iir_filter.text.cache.L2.code)
    *(.iir_filter.text.cache.L2.const)
    *(.iir_filter.text)
    *(.iir_filter.text.const)
	*(.sof_eq.text)
	*(.sof_eq.text.const)
#endif


#if AUDIO_LDAC_AT_RAM
    *(.ldac_dec.text)
    *(.ldac_dec.text.const)
    *(.ldac.text)
    *(.ldac.text.const)
    *(.ldac_dec.text.cache.L2.run)
    *(.ldac_dec.text.cache.L2.const)
#endif
    . = ALIGN(4);
#if AUDIO_LC3_CODEC_AT_RAM
    *(.lc3_dec.text.const)
    *(.lc3_dec.text)
    *(.lc3_enc.text.const)
    *(.lc3_enc.text)
    *(.lc3_codec_c_const)
    *(.lc3_codec_c_code)
    *(.lc3_codec_c_sparse_code)
    *(.lc3_codec_d_code)
    *(.lc3_codec_e_code)
    . = ALIGN(4);
#endif
#if AUDIO_JLA_CODEC_AT_RAM
    *(.jla_dec.text.const)
    *(.jla_dec.text)
    *(.jla_enc.text.const)
    *(.jla_enc.text)
    *(.jla_codec_c_const)
    *(.jla_codec_c_code)
    *(.jla_codec_c_sparse_code)
    *(.jla_codec_d_code)
    *(.jla_codec_e_code)
    . = ALIGN(4);
#endif

#if AUDIO_LC3_CODEC_AT_RAM ||  AUDIO_JLA_CODEC_AT_RAM
	*(.jl_codec_c_const) 
	*(.jl_codec_c_code)  
	*(.jl_codec_ari_c_const)
	*(.jl_codec_ari_c_code)
	*(.jl_codec_c_sparse_code)
	*(.jl_codec_d_code)
	*(.jl_codec_e_code)
	*(.jl_codec_t8k_code)
	*(.jl_codec_t16k_code)
	*(.jl_codec_t24k_code)
	*(.jl_codec_t32k_code)
	*(.jl_codec_t441k_code)
	*(.jl_codec_const) 
#endif

#if AFx_VBASS_AT_RAM
    *(.vbss.text.cache.L2.code)	
#endif

#if AFx_REVERB_AT_RAM
    *(.reverb_cal.text.cache.L2.run)
    *(.plateReverb_adv.text.cache.L2.run)
    *(.plate_reverb.text)
#endif

#if AFx_ECHO_AT_RAM
    *(.echo.text)
#endif
#if AFX_INDICATOR_AT_RAM
    *(.indicator.text)
#endif

#if AFX_EFFECTS_ADJ_AT_RAM
	*(.effects_adj.text)
#endif

#if AFX_AUDIO_LINK_AT_RAM
	*(.audio_link.text)
#endif

#if AFX_AUDIO_SYNC_AT_RAM
	*(.audio_sync.text)
#endif

#if AFx_VOICECHANGER_AT_RAM
	*(.voiceChanger.text.cache.L2.run)
	*(.voiceChanger.text.const)
	*(.voiceChanger.text)
#endif

#if AFx_ADVAUDIO_PLC_AT_RAM
 	*(.advaudio_plc.text.cache.L2.run)
#endif

#if AFx_DRC_AT_RAM
	*(.wdrc.text)
	*(.drc.text.cache.L2.code)
	*(.crossover.text.cache.L2.code)
	*(.BandMerge.text.cache.L2.code)
	*(.multiband_drc.text)
#endif

#if AFx_HARMONIC_EXCITER_AT_RAM
	*(.HarmonicExciter.text.cache.L2.run)
	*(.HarmonicExciter.text.const)
#endif

#if AFx_DYN_EQ_AT_RAM 				
	*(.dynamiceq.text.cache.L2.code)
#endif
#if AFx_EQ_AT_RAM 				
	*(.eq.text)
	*(.hw_eq.text)
#endif

#if AFx_VOCAL_TRACK_AT_RAM 				
	*(.vocal_track_separation.text)
	*(.vocal_track_synthesis.text)
#endif

#if AFX_AUDIO_DIGITAL_VOL_AT_RAM
	*(.digital_vol.text.cache.L2.run)
#endif
#if AFX_AUDIO_ENERGY_DET_AT_RAM
	*(.audio_energy_detect.text)
#endif

#if AFx_NOTCH_HOWLING_AT_RAM
	*(.notchhowling.text.cache.L2.code) 
 	*(.notchhowling.text.cache.L2.const)
#endif

#if AFx_FREQ_SHIFT_AT_RAM
	*(.FrequencyShift.text.cache.L2.run)
	*(.FrequencyShift.text.const)
#endif

#if AFx_NOISEGATE_AT_RAM
	*(.noisegate.text.cache.L2.code)
#endif

#if AFx_PHASER_AT_RAM
	*(.Phaser.text.cache.L2.code)	
	/* *(.Phaser.text)	 */
	/* *(.Phaser.text.const)	 */
#endif

#if AFx_FLANGER_AT_RAM
	*(.flanger.text.cache.L2.code)	
	/* *(.flanger.text)	 */
	/* *(.flanger.text.const)	 */
#endif

#if AFx_CHORUS_ADVANCE_AT_RAM
	*(.chorus_adv.text.cache.L2.code)	
	/* *(.chorus_adv.text)	 */
	/* *(.chorus_adv.text.const)	 */
#endif

#if AFx_PINGPONG_ECHO_AT_RAM 
	*(.pingpong_echo.text.cache.L2.code)	
	/* *(.pingpong_echo.text)	 */
	/* *(.pingpong_echo.text.const)	 */
#endif

#if AFx_DISTORTION_CLIPPING_AT_RAM
	*(.distortion.text.cache.L2.code)	
	*(.distortion.text.cache.L2.const)	
	/* *(.distortion.text)	 */
	/* *(.distortion.text.const)	 */
#endif


	*(.mixer.text.cache.L2)
	*(.dac.text.cache.L2)
	*(.convert.text.cache.L2.code)
	*(.VocalRemove.text.cache.L2.code)

    . = ALIGN(4);
    media_data_code_end = .;

