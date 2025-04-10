
    . = ALIGN(4);
    media_text_start = .;
    audio_sync_code_begin = .;
    *(.audio_sync_code)
    audio_sync_code_end = .;

    /*libkws_commin.a*/
#if (AUDIO_KWS_COMMON_AT_RAM == 0)
    *(.jlsp_kws_common_code)
    *(.jlsp_kws_common_const)
#endif
    /*JL_Phone_Call.a*/
#if (AUDIO_KWS_YES_NO_AT_RAM == 0)
    *(.jlsp_kws_yesno_code)
    *(.jlsp_kws_yesno_const)
#endif
    /*libjlsp_kws.a*/
#if (AUDIO_KWS_CHINESE_AT_RAM == 0)
    *(.kws_common_data_multi_26k)
    *(.kws_common_data_multi_57k)
    *(.jlsp_kws_music_code)
    *(.jlsp_kws_music_const)
    *(.jlsp_kws_phone_call_code)
    *(.jlsp_kws_phone_call_const)
#elif (AUDIO_KWS_CHINESE_AT_RAM == 2)
    *(.kws_common_data_multi_57k)
    *(.jlsp_kws_phone_call_code)
    *(.jlsp_kws_phone_call_const)
#endif
    /*libjlsp_kws_india_english.a*/
#if (AUDIO_KWS_INDIA_ENGLISH_AT_RAM == 0)
    *(.jlsp_kws_india_english_music_code)
    *(.jlsp_kws_india_english_music_const)
    *(.jlsp_kws_india_english_phone_call_code)
    *(.jlsp_kws_india_english_phone_call_const)
#endif
    /*libjlsp_kws_far_keyword.a*/
#if (AUDIO_KWS_CHINESE_FAR_AT_RAM == 0)
    *(.jlsp_far_keyword_const)
    *(.jlsp_far_keyword_code)
    *(.jlsp_far_front_code)
    *(.jlsp_far_front_const)
#endif

	/********************CODEC LD Begin*************************/
#if (AUDIO_MSBC_CODEC_AT_RAM == 0)
    *(.msbc_enc)
    *(.msbc_code)
    *(.msbc_const)
    *(.msbc_codec_code)
    *(.msbc_codec_const)
#endif
#if (AUDIO_CVSD_CODEC_AT_RAM == 0)
    *(.cvsd_codec)
    *(.cvsd_code)
    *(.cvsd_const)
#endif
#if AUD_AAC_DEC_AT_RAM == 0
    aac_dec_code_begin = .;
    *(.bt_aac_dec_code)
    *(.bt_aac_dec_sparse_code)
    aac_dec_code_end = .;
    aac_dec_code_size  = aac_dec_code_end - aac_dec_code_begin ;

    . = ALIGN(4);
    bt_aac_dec_const_begin = .;
    *(.bt_aac_dec_const)
    *(.bt_aac_dec_sparse_const)
    bt_aac_dec_const_end = .;
    bt_aac_dec_const_size = bt_aac_dec_const_end -  bt_aac_dec_const_begin ;

    . = ALIGN(4);
    *(.aac_mem)
    *(.aac_ctrl_mem)
#endif

    . = ALIGN(4);
    *(.bt_audioplc_code)
    *(.bt_audioplc_sparse_code)
    *(.bt_audioplc_const)
    *(.lf_audioplc_code)
    *(.lf_audioplc_sparse_code)
    *(.lf_audioplc_const)
    *(.lfaudio_plc.text.L2.run)

    . = ALIGN(4);
    *(.pcm_code)
    *(.pcm_const)

    *(.lhdc.text.const)
    *(.lhdc.text)
    *(.lhdcv4decode.text)
    *(.lhdcv4decode.const)

    *(.lhdc_v5.text.const)
    *(.lhdc_v5.text)
    *(.lhdcv5decode.text)
    *(.lhdcv5decode.const)

#if AUDIO_LDAC_AT_RAM == 0
    *(.ldac_dec.text)
    *(.ldac_dec.text.const)
    *(.ldac.text)
    *(.ldac.text.const)
#endif
    *(.ldac_enc.text)
    *(.ldac_enc.text.const)

    *(.sbc_code)
    *(.sbc_const)
    *(.sbc_eng_code)
    *(.sbc_eng_const)
    *(.bt_aac_dec.text)
    *(.bt_aac_dec.text.const)
    *(.btaac_rom_dec.text)
    *(.aac.text.const)
    *(.aac.text)
    *(.aac_code)
    *(.aac_const)
    *(.bt_aac_dec_core_sparse_code)
    *(.bt_aac_dec_eng_code)
    *(.bt_aac_dec_core_code)
    *(.bt_aac_dec_eng_const)
    *(.plc_code)
    *(.plc_const)

    *(.wtg_dec.text)
    *(.g729.text)
    *(.g729.text.const)
    *(.g729_code)
    *(.g729_const)
    *(.wtgv2_code)
    *(.wtgv2_const)
    *(.wtgv2dec_code)
    *(.wtgv2dec_const)
    *(.wtgv2dec_str)
    *(.wtg_decv2_sparse_code)
    *(.wtgv2_dec.text)
    *(.wts.text)
    *(.wts.text.const)
    *(.f2a_dec.text)
    *(.f2a.text)
    *(.f2a.text.const)

    *(.mp3_dec.text)
    *(.mp3.text)
    *(.mp3.text.const)
    *(.mp3tsy_dec.text)
    *(.mp3_code)
    *(.mp3_const)
    *(.mp3_decstream_const)
    *(.mp3_decstream_code)
    *(.mp3_decstream_sparse_code)
    *(.mp3_decstream_sparse_const)
    *(.mp3_dec_sparse_code)
    *(.mp3_dec_sparse_const)
    *(.mp3_dec_code)
    *(.mp3_dec_const)
    *(.mp3tsy_dec_code)
    *(.mty_code)
    *(.mp3tsy_dec_sparse_code)
    *(.mp3tsy_dec_const)
    *(.mty.text)
    *(.mty.text.const)
    *(.f2a_dec.text)
    *(.f2a.text)
    *(.f2a.text.const)

    *(.opus_enc.text)
    *(.opus_enc.text.const)
    *(.opus.text)
    *(.opus.text.const)
    *(.ogg_opus_dec.text)
    *(.ogg.text)
    *(.ogg.text.const)
    *(.ogg_vorbis_dec.text)

    *(.ape_dec.text)
    *(.ape_dec.text.const)
    *(.ape.text)
    *(.ape.text.const)
    *(.ape_dec_sparse_code)
    *(.ape_dec_sparse_const)
    *(.ape_dec_code)
    *(.ape_dec_const)
    *(.ape_const)
    *(.ape_code)

    *(.alac_const)
    *(.alac_code)
    *(.alac_dec_code)
    *(.wma_dec.text)
    *(.wma.text)
    *(.wma.text.const)
    *(.wma_code)
    *(.wma_const)
    *(.wma_dec_code)
    *(.wma_dec_const)
    *(.wma_dec_sparse_code)
    *(.wma_dec_sparse_const)
    *(.wma_decstream_const)
    *(.wma_decstream_code)
    *(.wma_decstream_sparse_code)
    *(.wma_decstream_sparse_const)

    *(.wav_dec.text)
    *(.wav.text)
    *(.wav.text.const)
    *(.wav_dec_sparse_code)
    *(.wav_dec_sparse_const)

    *(.wav_dec_code)
    *(.wav_dec_const)
    *(.wav_const)
    *(.wav_code)

    *(.wtg_dec_code )
    *(.wtg_dec_const)
    *(.wtg_dec_sparse_code)
    /**(.bfilt_code)*/
    *(.sine_code)
    *(.sine_const)
    /* *(.wma_code) */
    /* *(.wma_const) */
    *(.aptx_dec.text)
    *(.aptx_dec.text.const)
    *(.amr_dec.text)
    *(.amr_dec.text.const)
    *(.amr.text)
    *(.amr.text.const)
    *(.amr_code)
    *(.amr_const)
    *(.amr_dec_sparse_code)
    *(.amr_dec_sparse_const)
    *(.amr_dec_const)
    *(.amr_dec_code)
    *(.audio_decoder_code)
    *(.audio_decoder_const)
    *(.audio_track_const)
    *(.audio_track_code)

    *(.dts_dec.text)
    *(.dts_dec.text.const)
    *(.dts.text)
    *(.dts.text.const)
    *(.dts_dec_ff_const)
    *(.dts_dec_sparse_code)
    *(.dts_dec_sparse_const)
    *(.dts_dec_const)
    *(.dts_code)
    *(.dts_dec_code)
    *(.flac_dec.text)
    *(.flac_dec.text.const)
    *(.flac.text)
    *(.flac.text.const)
    *(.flac_code)
    *(.flac_dec_code)
    *(.flac_dec_const)
    *(.flac_dec_sparse_code)
    *(.m4a_dec.text)
    *(.m4a_dec.text.const)
    *(.m4a_dec.bt_aac.text.const)
    *(.m4a.text)
    *(.m4a.text.const)
    *(.m4a_dec_code)
    *(.m4a_dec_ff_const)
    *(.m4a_dec_const)
    *(.m4a_const)
    *(.m4a_code)
    *(.m4a_dec_sparse_code)
    *(.m4a_dec_sparse_const)
    *(.alac.text)
    *(.alac.text.const)

    *(.mp3_encode_code)
    *(.mp3_enc_code)
    *(.mp3_enc_const)
    *(.mp3_enc_sparse_code)
    *(.mp3_enc_sparse_const)
    . = ALIGN(4);
    *(.mp2_encode_code)
    *(.mp2_encode_const)
    *(.mp2_encode_sparse_code)
    *(.mp2_encode_sparse_const)
    . = ALIGN(4);
    *(.adpcm_encode_code)
    *(.adpcm_encode_const)
    *(.adpcm_encode_sparse_code)
    *(.adpcm_encode_sparse_const)
    . = ALIGN(4);
    . = ALIGN(4);
#if !AUDIO_LC3_CODEC_AT_RAM
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
#if !AUDIO_JLA_CODEC_AT_RAM
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
#if !(AUDIO_JLA_CODEC_AT_RAM || AUDIO_JLA_CODEC_AT_RAM)
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
	*(.jla_lw_codec_const)
	*(.jla_lw_codec_code)
	*(.jla_lw.const)
	*(.jla_lw.code)

	*(.jla_ll.const)
	*(.jla_ll.code)
	*(.jla_ll_codec.text)
	*(.jla_ll_codec.text.const)
	*(.jla_ll_codec.text.cache.L2.decode)
	*(.jla_ll_codec.text.cache.L2.const)
	*(.jla_ll_codec.text.cache.L2.run)

	*(.jla_v2_enc_const)
	*(.jla_v2_enc_code)
	*(.jla_v2_dec_const)
	*(.jla_v2_dec_code)
	*(.jla_v2_codec_c_const)
	*(.jla_v2_codec_c_code)
	*(.jla_v2_codec_ari_c_const)
	*(.jla_v2_codec_ari_c_code)
	*(.jla_v2_codec_c_sparse_code)
	*(.jla_v2_codec_e_code)
	*(.jla_v2_decoder_const)
	*(.jla_v2_decoder_code)
	*(.jla_v2_codec_d_code)

    *(.speex_code)
    *(.speex_encoder_code)
    *(.opus_code)
    *(.opus_encoder_code)

    *(.audio_codec_code)

	/********************CODEC LD Ends*************************/
	
    *(.media_device_code)
    *(.media_device_const)
    *(.audio_encoder_code)
    *(.audio_encoder_const)
    *(.mixer_code)
    *(.mixer_const)
    *(.dec_server_code)
    *(.dec_server_const)
    *(.rec_server_code)
    *(.rec_server_const)
    *(.auto_mute_code)
    *(.auto_mute_const)
    *(.wireless_sync_code)
    *(.wireless_sync_const)

    anc_code_begin = .;
    *(.anc_user_const)
    *(.anc_user_code)
    *(.anc_const)
    *(.anc_code)
    *(.anc_core_const)
    *(.anc_core_code)
    *(.anc_box_const)
    *(.anc_box_code)
    *(.anc_btspp_const)
    *(.anc_btspp_code)
    *(.anc_debug_const)
    *(.anc_debug_code)
    anc_code_end = .;
    anc_code_size = anc_code_end - anc_code_begin;


    *(.bt_compressor_sparse_const)
    *(.bt_compressor_sparse_code)
    *(.compressor_sparse_code)
    *(.compressor_sparse_const)
    *(.drc_sparse_code)
    *(.drc_sparse_const)

    *(.bt_limiter_sparse_const)
    *(.bt_limiter_sparse_code)
    *(.limiter_sparse_code)
    *(.limiter_sparse_const)

    *(.bt_crossOver_sparse_const)
    *(.bt_crossOver_sparse_code)
    *(.crossOver_sparse_code)
    *(.crossOver_sparse_const)
    *(.reverb_cal_sparse_code)

#if (AUDIO_EFFECTS_REVERB_AT_RAM == 0)
    *(.reverb_cal_code)
    *(.reverb_cal_const)
    *(.platereverb_code)
    *(.platereverb_const)
    *(.audio_reverb_code)
    *(.audio_reverb_const)
    *(.reverb0_code)
    *(.reverb0_const)
#endif/*AUDIO_EFFECTS_REVERB_AT_RAM*/

#if (AUDIO_EFFECTS_ECHO_AT_RAM == 0)
    *(.audio_echo_code)
    *(.audio_echo_const)
    *(.echo_cal_code)
    *(.echo_cal_const)
#endif

#if (AUDIO_EFFECTS_EQ_AT_RAM == 0)
    *(.audio_eq_code)
    *(.audio_eq_const)
#endif/*AUDIO_EFFECTS_EQ_AT_RAM*/
#if (AUDIO_EFFECTS_DRC_AT_RAM == 0)
    *(.audio_drc_code)
    *(.audio_drc_const)
    *(.drc_const)
    *(.drc_code)
    *(.sw_drc_code)
    *(.sw_drc_const)
#endif/*AUDIO_EFFECTS_DRC_AT_RAM*/

#if (AUDIO_EFFECTS_AFC_AT_RAM == 0)
    *(.howlings_phf_code)
    *(.howlings_phf_const)
    /*AFC_Lib_陷波*/
    *(.notchhowling_code)
    *(.notchhowling_const)
    *(.notchhowling_sparse_code)
    *(.notchhowling_sparse_const)
#endif/*AUDIO_EFFECTS_AFC_AT_RAM*/
    *(.howlings_phf_sparse_code)

#if (AUDIO_EFFECTS_NOISEGATE_AT_RAM == 0)
    *(.audio_noisegate_code)
    *(.audio_noisegate_const)
    /*Lib_NoiseGate*/
    *(.noisegate_code)
    *(.noisegate_const)
    *(.noisegate_sparse_code)
    *(.noisegate_sparse_const)
#endif/*AUDIO_EFFECTS_NOISEGATE_AT_RAM*/

#if (AUDIO_EFFECTS_GAIN_AT_RAM == 0)
    *(.audio_gain_code)
    *(.audio_gain_const)
#endif

#if (AUDIO_EFFECTS_DYNAMIC_EQ_AT_RAM == 0)
    *(.dynamic_eq_code)
    *(.dynamic_eq_const)
    *(.dynamic_eq_detect_code)
    *(.dynamic_eq_detect_const)
    *(.dynamic_eq_sparse_code)
    *(.dynamic_eq_sparse_const)
    *(.dynamic_eq_detect_sparse_code)
    *(.dynamic_eq_detect_sparse_const)

#endif/*AUDIO_EFFECTS_DYNAMIC_EQ_AT_RAM*/

#if (AUDIO_EFFECTS_MIC_EFFECT_AT_RAM == 0)
    *(.audio_mic_effect_const)
    *(.audio_mic_effect_code)
#endif/*AUDIO_EFFECTS_MIC_EFFECT_AT_RAM*/
#if (AUDIO_EFFECTS_MIC_STREAM_AT_RAM == 0)
    *(.audio_mic_stream_const)
    *(.audio_mic_stream_code)
#endif/*AUDIO_EFFECTS_MIC_STREAM_AT_RAM*/
#if (AUDIO_EFFECTS_DEBUG_AT_RAM == 0)
    *(.audio_effect_adj_const)
    *(.audio_effect_adj_code)
#endif/*AUDIO_EFFECTS_DEBUG_AT_RAM*/

#if (AUDIO_EFFECTS_VBASS_AT_RAM == 0)
    *(.audio_vbass_code)
    *(.audio_vbass_const)
    *(.vbss_code)
    *(.vbss_const)
    *(.vbss_sparse_code)
    *(.vbss_sparse_const)
#endif/*AUDIO_EFFECTS_VBASS_AT_RAM*/

    *(.pitchshifter_code)
    *(.pitchshifter_const)
    *(.audio_pitch_code)
    *(.audio_pitch_const)
    *(.audio_pitchspeed_code)
    *(.audio_pitchspeed_const)
    *(.pitchshifter_sparse_code)

#if (AUDIO_EFFECTS_SURROUND_AT_RAM == 0)
    *(.audio_surround_code)
    *(.audio_surround_const)
    *(.sur_cal_common_code)
    *(.sur_TYPE0_cal_code)
    *(.sur_TYPE1_cal_code)
    *(.sur_TYPE0_cal_const)
    *(.sur_TYPE1_cal_const)
#endif
    *(.sur_LRDRIFT2_cal_code)
    *(.sur_LRDRIFT2_cal_const)
    *(.sur_ROTATE_cal_code)
    *(.sur_ROTATE_cal_const)
    *(.sur_LRDRIFT_cal_code)
    *(.sur_LRDRIFT_cal_const)
    *(.sur_cal_sparse_code)
#if (AFX_LIMITER_AT_RAM == 0)
	*(.limiter.text)
    *(.limiter.text.const)
	*(.limiter.text.cache.L2.code)
	*(.limiter.text.cache.L2.const)
#endif
#if (AFX_MULTIBAND_LIMITER_AT_RAM == 0)
	*(.multiband_limiter.text)
	*(.multiband_limiter.text.const)
#endif
#if (AFX_MULTIBAND_CROSSOVER_AT_RAM == 0)
    *(.multiband_crossover.text)
	*(.multiband_crossover.text.const)
#endif
#if (AFX_MULTIBAND_DRC_AT_RAM == 0)
    *(.multiband_drc.text)
	*(.multiband_drc.text.const)
#endif
#if (AFX_VIRTUAL_SURRUOUND_PRO_AT_RAM == 0)
	*(.virtual_surround_pro.text)
	*(.virtual_surround_pro.text.const)
	*(.sas.text.cache.L2.code)
	*(.sas.text.cache.L2.const)
    *(.sas.text)
    *(.sas.text.code)
#endif
#if (AFX_SW_EQ_AT_RAM == 0)
    *(.iir_filter.text.cache.L2.code)
    *(.iir_filter.text.cache.L2.const)
    *(.iir_filter.text)
    *(.iir_filter.text.const)
	*(.sof_eq.text)
	*(.sof_eq.text.const)
#endif

#if (AFx_PHASER_AT_RAM == 0)
	*(.Phaser.text.cache.L2.code)	
	/* *(.Phaser.text)	 */
	/* *(.Phaser.text.const)	 */
#endif


#if (AFx_FLANGER_AT_RAM == 0)
	*(.flanger.text.cache.L2.code)	
	/* *(.flanger.text)	 */
	/* *(.flanger.text.const)	 */
#endif

#if (AFx_CHORUS_ADVANCE_AT_RAM == 0)
	*(.chorus_adv.text.cache.L2.code)	
	/* *(.chorus_adv.text)	 */
	/* *(.chorus_adv.text.const)	 */
#endif

#if (AFx_PINGPONG_ECHO_AT_RAM == 0)
	*(.pingpong_echo.text.cache.L2.code)	
	/* *(.pingpong_echo.text)	 */
	/* *(.pingpong_echo.text.const)	 */
#endif

#if AFx_DISTORTION_CLIPPING_AT_RAM == 0
	*(.distortion.text.cache.L2.code)	
	*(.distortion.text.cache.L2.const)	
	/* *(.distortion.text)	 */
	/* *(.distortion.text.const)	 */
#endif

    . = ALIGN(4);
    media_code_begin = .;
    *(.media.*.text)
    . = ALIGN(4);

    *(.app_audio_code)
    *(.app_audio_const)

    *(.compressor_code)
    *(.compressor_const)
    *(.bt_compressor_const)
    *(.bt_compressor_code)

    *(.crossOver_code)
    *(.crossOver_const)
    *(.bt_crossOver_const)
    *(.bt_crossOver_code)

    *(.limiter_code)
    *(.limiter_const)
    *(.bt_limiter_const)
    *(.bt_limiter_code)
    *(.drc_const)
    *(.drc_code)

    *(.audio_gain_code)
    *(.audio_gain_const)

    *(.vbss_code)
    *(.vbss_const)
    *(.vbss_sparse_code)
    *(.vbss_sparse_const)

    *(.specshow_code)
    *(.specshow_const)
    *(.specshow_sparse_code)
    *(.specshow_sparse_const)
    *(.spectrum_fft_code)
    *(.spectrum_fft_const)
    *(.spectrum_eq_code)
    *(.spectrum_eq_const)
    *(.audio_vocal_remove_code)


    . = ALIGN(4);
    *(.sur_cal_const)
    *(.sur_cal_code)
    . = ALIGN(4);
    *(.lib_pitchshift_code)
    *(.lib_pitchshift_const)
    *(.resample_fastcal_sparse_code)
    *(.resample_fastcal_code)
    *(.resample_fastcal_const)


    . = ALIGN(4);
    _audio_decoder_begin = .;
    PROVIDE(audio_decoder_begin = .);
KEEP(*(.audio_decoder))
    _audio_decoder_end = .;
    PROVIDE(audio_decoder_end = .);

    _audio_encoder_begin = .;
    PROVIDE(audio_encoder_begin = .);
KEEP(*(.audio_encoder))
    _audio_encoder_end = .;
    PROVIDE(audio_encoder_end = .);

    _audio_package_begin = .;
    PROVIDE(audio_package_begin = .);
KEEP(*(.audio_package))
    _audio_package_end = .;
    PROVIDE(audio_package_end = .);

    _audio_dev_begin = .;
    PROVIDE(audio_dev_begin = .);
KEEP(*(.audio_device))
    _audio_dev_end = .;
    PROVIDE(audio_dev_end = .);

    _audio_hwaccel_begin = .;
    PROVIDE(audio_hwaccel_begin = .);
KEEP(*(.audio_hwaccel))
    _audio_hwaccel_end = .;
    PROVIDE(audio_hwaccel_end = .);

    media_code_end = .;
    . = ALIGN(4);
    media_code_size = media_code_end - media_code_begin;

    . = ALIGN(4);
    media_text_end = .;
