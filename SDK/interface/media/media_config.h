#ifndef LIB_MEDIA_CONFIG_H
#define LIB_MEDIA_CONFIG_H



extern const int CONFIG_DAC_CACHE_MSEC;
extern const int CONFIG_JLSTREAM_MULTI_THREAD_ENABLE;
extern const int CONFIG_MULTI_THREAD_SELF_ADAPTION_ENABLE;
extern const int config_media_24bit_enable;
extern const int CONFIG_SEAMLESS_RECORDER_ENABLE;
extern const int config_ch_adapter_32bit_enable;
extern const int config_mixer_32bit_enable;
extern const int config_jlstream_fade_32bit_enable;
extern const int config_audio_eq_xfade_enable;

extern const int CONFIG_MAX_ENC_DATA_CACHE_SIZE;
extern const int CONFIG_LOG_OUTPUT_ENABLE;
extern const int config_audio_cfg_online_enable;
extern const int config_audio_dac_dma_buf_realloc_enable;


/*
 *******************************************************************
 *						DAC Configs
 *******************************************************************
 */
extern const int config_audio_dac_channel_left_enable;
extern const int config_audio_dac_channel_right_enable;
extern const int config_audio_dac_power_on_mode;
extern const int config_audio_dac_power_off_lite;

/*
 *******************************************************************
 *						ADC Configs
 *******************************************************************
 */

/*
 *******************************************************************
 *						EQ Configs
 *******************************************************************
 */
extern const int config_audio_eq_hp_enable;		//High Pass
extern const int config_audio_eq_lp_enable;		//Low Pass
extern const int config_audio_eq_bp_enable;		//Band Pass(Peaking)
extern const int config_audio_eq_hs_enable;		//High Shelf
extern const int config_audio_eq_ls_enable;		//Low Shelf
extern const int config_audio_eq_hs_q_enable;	//High Shelf Q
extern const int config_audio_eq_ls_q_enable;	//Low Shelf Q
extern const int config_audio_eq_hp_adv_enable;	//High Pass Advance
extern const int config_audio_eq_lp_adv_enable;	//Low Pass Advance

/*
 *******************************************************************
 *						SRC Configs
 *******************************************************************
 */

/*
 *******************************************************************
 *						Effect Configs
 *******************************************************************
 */
extern const int config_audio_gain_enable;
extern const int config_audio_split_gain_enable;
extern const int config_audio_stereomix_enable;
extern const int voicechanger_effect_v_config;
extern const int mb_limiter_3band_run_en;
extern const int limiter_run_mode;
extern const int drc_advance_run_mode;
extern const int drc_run_mode;
extern const int  stereo_phaser_run_mode;
extern const int  stereo_flanger_run_mode;
extern const int  stereo_chorus_run_mode;
extern const int dynamic_eq_run_mode;
extern const  int drc_detect_run_mode;

/*
 *******************************************************************
 *						Audio Codec Configs
 *******************************************************************
 */
extern const u8 config_flac_id3_enable;
extern const u8 config_ape_id3_enable;
extern const u8 config_m4a_id3_enable;
extern const u8 config_wav_id3_enable;
extern const u8 config_wma_id3_enable;


/*
 *******************************************************************
 *						Audio Mic Capless Config
 *******************************************************************
 */
extern const u8 const_mic_capless_open_delay_debug;
extern const u8 const_mic_capless_trim_delay_debug;








#endif
