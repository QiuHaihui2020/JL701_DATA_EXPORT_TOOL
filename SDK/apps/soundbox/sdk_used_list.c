#include "app_config.h"


source_node_adapter

#if TCFG_MIXER_NODE_ENABLE
mixer_node_adapter
#endif

#if TCFG_DAC_NODE_ENABLE
dac_node_adapter
#endif

clk_sync_node_adapter
decoder_node_adapter
resample_node_adapter
bt_audio_sync_node_adapter

#if TCFG_ADC_NODE_ENABLE
adc_file_plug
#endif

tone_file_plug
ring_file_plug
key_tone_file_plug
#if TCFG_FILEPLAY_NODE_ENABLE
file_play_plug
#endif

sbc_hwaccel
sbc_decoder_plug
msbc_decoder_plug

#if TCFG_PDM_NODE_ENABLE
pdm_mic_file_plug
#endif

#if TCFG_BT_SUPPORT_AAC || TCFG_DEC_AAC_ENABLE || TCFG_TONE_AAC_ENABLE
aac_dec_plug
#endif

sine_dec_plug
cvsd_decoder_plug
pcm_dec_plug

#if TCFG_ZERO_ACTIVE_NODE_ENABLE
zero_file_plug
#endif

#if TCFG_BT_SUPPORT_LHDC
lhdc_dec_plug
#endif

#if TCFG_BT_SUPPORT_LHDC_V5
lhdc_v5_dec_plug
#endif

#if TCFG_BT_SUPPORT_LDAC
ldac_dec_plug
#endif

#if TCFG_ENC_MSBC_ENABLE
msbc_encoder_hw_plug
#endif


#if TCFG_ENC_SBC_ENABLE
sbc_encoder_soft_plug
#endif

#if TCFG_ENC_CVSD_ENABLE
cvsd_encoder_plug
#endif

#if TCFG_ENC_MP3_ENABLE
mp3_encoder_plug
#endif

#if TCFG_ENC_ADPCM_ENABLE
wav_encoder_plug
wav_package
#endif

#if TCFG_ENC_PCM_ENABLE
pcm_encoder_plug
#endif

#if TCFG_MULTI_CH_IIS_RX_NODE_ENABLE
mulit_ch_iis0_file_plug
mulit_ch_iis1_file_plug
#endif

#if TCFG_MULTI_CH_IIS_NODE_ENABLE
multi_ch_iis0_node_adapter
multi_ch_iis1_node_adapter
#endif



#if TCFG_IIS_NODE_ENABLE
iis_node_adapter
iis1_node_adapter
iis_file_plug
iis1_file_plug
#endif

#if TCFG_AUDIO_LINEIN_ENABLE
linein_file_plug
#endif

#if TCFG_AUDIO_FM_ENABLE
fm_file_plug
#endif

music_file_plug

#if TCFG_ENERGY_DETECT_NODE_ENABLE
energy_detect_node_adapter
#endif


#if TCFG_CONVERT_NODE_ENABLE
convert_node_adapter
convert_data_round_node_adapter
#endif

#if TCFG_PITCH_SPEED_NODE_ENABLE
pitch_speed_node_adapter
#endif

#if TCFG_3BAND_MERGE_ENABLE
three_band_merge_node_adapter
#endif

#if TCFG_2BAND_MERGE_ENABLE
two_band_merge_node_adapter
#endif

#if TCFG_BASS_TREBLE_NODE_ENABLE
bass_treble_eq_node_adapter
#endif


#if TCFG_SOUND_SPLITTER_NODE_ENABLE
sound_splitter_node_adapter
#endif

#if TCFG_VOCAL_TRACK_SYNTHESIS_NODE_ENABLE
vocal_track_synthesis_node_adapter
#endif

#if TCFG_VOCAL_TRACK_SEPARATION_NODE_ENBALE
vocal_track_separation_node_adapter
#endif

#if TCFG_CROSSOVER_NODE_ENABLE
crossover_node_adapter
crossover_2band_node_adapter
#endif

#if TCFG_WDRC_NODE_ENABLE
wdrc_node_adapter
#endif


#if TCFG_STEROMIX_NODE_ENABLE
steromix_node_adapter
#endif

#if TCFG_DYNAMIC_EQ_NODE_ENABLE
dynamic_eq_node_adapter
#endif
#if TCFG_DYNAMIC_EQ_EXT_DETECTOR_NODE_ENABLE
dynamic_eq_ext_detector_node_adapter
#endif

#if TCFG_SURROUND_NODE_ENABLE
surround_node_adapter
#endif

#if TCFG_VOICE_CHANGER_NODE_ENABLE
voice_changer_node_adapter
#endif

#if TCFG_VOICE_CHANGER_ADV_NODE_ENABLE
voice_changer_adv_node_adapter
#endif

#if  TCFG_NOISEGATE_NODE_ENABLE
noisegate_node_adapter
#endif

#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
frequency_shift_node_adapter
#endif

#if TCFG_NOTCH_HOWLING_NODE_ENABLE
howling_suppress_node_adapter
#endif

#if TCFG_PLATE_REVERB_NODE_ENABLE
plate_reverb_node_adapter
#endif

#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
plate_reverb_advance_node_adapter
#endif

#if TCFG_ECHO_NODE_ENABLE
echo_node_adapter
#endif

#if TCFG_SPECTRUM_NODE_ENABLE
spectrum_node_adapter
#endif
#if TCFG_SPECTRUM_ADVANCE_NODE_ENABLE
spectrum_advance_node_adapter
#endif

#if TCFG_AUTOTUNE_NODE_ENABLE
autotune_node_adapter
#endif

#if TCFG_VOCAL_REMOVER_NODE_ENABLE
vocal_remover_node_adapter
#endif

#if TCFG_SPEAKER_EQ_NODE_ENABLE
spk_eq_node_adapter
#endif

#if TCFG_EQ_ENABLE
eq_node_adapter
#endif

#if TCFG_GAIN_NODE_ENABLE
gain_node_adapter
#endif

#if TCFG_VBASS_NODE_ENABLE
vbass_node_adapter
#endif

#if TCFG_NS_NODE_ENABLE
ns_node_adapter
#endif

#if TCFG_DNS_NODE_ENABLE
dns_node_adapter
#endif

#if TCFG_DEC_WTG_ENABLE || TCFG_TONE_WTG_ENABLE
g729_dec_plug
#endif

#if TCFG_DEC_MTY_ENABLE || TCFG_TONE_MTY_ENABLE
mty_dec_plug
#endif

#if TCFG_DEC_WAV_ENABLE || TCFG_TONE_WAV_ENABLE
wav_dec_plug
#endif

#if TCFG_DEC_AIFF_ENABLE
aiff_dec_plug
#endif
#if TCFG_DEC_OGG_OPUS_ENABLE
opus_dec_plug
#endif
#if TCFG_DEC_OGG_VORBIS_ENABLE
ogg_dec_plug
#endif

#if TCFG_DEC_SPEEX_ENABLE
speex_dec_plug
#endif

#if TCFG_ENC_SPEEX_ENABLE
speex_encoder_plug
#endif

#if TCFG_DEC_AMR_ENABLE
amr_dec_plug
#endif

#if TCFG_DEC_DTS_ENABLE
dts_dec_plug
#endif

#if TCFG_DEC_MP3_ENABLE || TCFG_TONE_MP3_ENABLE
mp3_dec_plug
#endif

#if TCFG_DEC_F2A_ENABLE || TCFG_TONE_F2A_ENABLE
f2a_dec_plug
#endif

#if TCFG_DEC_WMA_ENABLE || TCFG_TONE_WMA_ENABLE
wma_dec_plug
#endif

#if TCFG_DEC_FLAC_ENABLE
flac_dec_plug
#endif

#if TCFG_DEC_M4A_ENABLE
m4a_dec_plug
#endif

#if TCFG_DEC_ALAC_ENABLE
alac_dec_plug
#endif

#if TCFG_DEC_APE_ENABLE
ape_dec_plug
#endif

#if TCFG_ENC_OPUS_ENABLE
opus_encoder_plug
#endif

#if TCFG_DEC_WTS_ENABLE || TCFG_TONE_WTS_ENABLE
wts_dec_plug
#endif

#if TCFG_ENC_JLA_ENABLE
jla_encoder_plug
#endif

#if TCFG_DEC_JLA_ENABLE
jla_dec_plug
#endif


#if CONFIG_FATFS_ENABLE
fat_vfs_ops
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
big_tx_op
big_rx_op
#endif
#if LEA_CIG_CENTRAL_EN
cig_central_op
#endif
#if LEA_CIG_PERIPHERAL_EN
cig_perip_op
#endif
wireless_custom_data_trans
wireless_key_sync
le_audio_source_adapter
capture_sync_adapter
#if (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LW)
#if (TCFG_KBOX_1T3_MODE_EN == 0)
jla_lw_encoder_plug
#endif
jla_lw_dec_plug
#elif (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_LL)
#if (TCFG_KBOX_1T3_MODE_EN == 0)
jla_ll_encoder_plug
#endif
jla_ll_dec_plug
#elif (LE_AUDIO_CODEC_TYPE == AUDIO_CODING_JLA_V2)
#if (TCFG_KBOX_1T3_MODE_EN == 0)
jla_v2_enc_plug
#endif
jla_v2_dec_plug
#endif
le_audio_file_plug
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
lc3_dec_plug
lc3_encoder_plug
#endif

#if TCFG_NORFLASH_DEV_ENABLE && VFS_ENABLE
nor_fs_vfs_ops
nor_sdfile_vfs_ops
#endif

#if FLASH_INSIDE_REC_ENABLE
inside_nor_fs_vfs_ops
#endif

#if TCFG_STEREO_WIDENER_NODE_ENABLE
stereo_widener_node_adapter
#endif

#if TCFG_HARMONIC_EXCITER_NODE_ENABLE
harmonic_exciter_node_adapter
#endif

#if TCFG_INDICATOR_NODE_ENABLE
indicator_node_adapter
#endif

#if TCFG_CHORUS_NODE_ENABLE
chorus_node_adapter
#endif

#if TCFG_CHANNEL_EXPANDER_NODE_ENABLE
channel_expander_node_adapter
#endif

#if TCFG_CHANNEL_MERGE_NODE_ENABLE
channel_merge_node_adapter
#endif

#if TCFG_WDRC_DETECTOR_NODE_ENABLE
wdrc_detector_node_adapter
#endif

#if TCFG_WDRC_ADVANCE_NODE_ENABLE
wdrc_advance_node_adapter
#endif


#if TCFG_PCM_DELAY_NODE_ENABLE
pcm_delay_node_adapter
#endif

#if TCFG_AUTODUCK_NODE_ENABLE
autoduck_trigger_node_adapter
autoduck_node_adapter
#endif

#if TCFG_EFFECT_DEV0_NODE_ENABLE
effect_dev0_node_adapter
#endif

#if TCFG_EFFECT_DEV1_NODE_ENABLE
effect_dev1_node_adapter
#endif

#if TCFG_EFFECT_DEV2_NODE_ENABLE
effect_dev2_node_adapter
#endif
#if TCFG_EFFECT_DEV3_NODE_ENABLE
effect_dev3_node_adapter
#endif
#if TCFG_EFFECT_DEV4_NODE_ENABLE
effect_dev4_node_adapter
#endif

#if TCFG_SOURCE_DEV0_NODE_ENABLE
source_dev0_file_plug
#endif

#if TCFG_SOURCE_DEV1_NODE_ENABLE
source_dev1_file_plug
#endif

#if TCFG_SOURCE_DEV2_NODE_ENABLE
source_dev2_file_plug
#endif

#if TCFG_SOURCE_DEV3_NODE_ENABLE
source_dev3_file_plug
#endif

#if TCFG_SOURCE_DEV4_NODE_ENABLE
source_dev4_file_plug
#endif

#if TCFG_SINK_DEV0_NODE_ENABLE
sink_dev0_adapter
#endif

#if TCFG_SINK_DEV1_NODE_ENABLE
sink_dev1_adapter
#endif

#if TCFG_SINK_DEV2_NODE_ENABLE
sink_dev2_adapter
#endif

#if TCFG_SINK_DEV3_NODE_ENABLE
sink_dev3_adapter
#endif

#if TCFG_SINK_DEV4_NODE_ENABLE
sink_dev4_adapter
#endif

#if TCFG_FILE_PACKAGE_NODE_ENABLE
packager_adapter
#endif

#if TCFG_WRITE_FILE_NODE_ENABLE
write_file_adapter
#endif

#if TCFG_DATA_SATURATION_NODE_ENABLE
data_saturation_node_adapter
#endif

#if TCFG_DYNAMIC_EQ_PRO_NODE_ENABLE
dynamic_eq_pro_node_adapter
#endif

#if TCFG_DYNAMIC_EQ_PRO_EXT_DETECTOR_NODE_ENABLE
dynamic_eq_pro_ext_detector_node_adapter
#endif

#if TCFG_AUTO_WAH_NODE_ENABLE
autowah_node_adapter
#endif

#if TCFG_PING_PONG_PCM_DELAY_NODE_ENABLE
pingpong_pcm_delay_node_adapter
#endif

#if TCFG_THREE_D_EFFECT_NODE_ENABLE
three_d_node_adapter
#endif

#if TCFG_LLNS_NODE_ENABLE
llns_node_adapter
#endif

#if TCFG_FADE_NODE_ENABLE
fade_node_adapter
#endif
#if TCFG_AGC_NODE_ENABLE
agc_node_adapter
#endif

#if TCFG_STEREO_MTAPS_ECHO_NODE_ENABLE
stereo_mtaps_echo_node_adapter
#endif

#if TCFG_HOWLING_GATE_ENABLE
howling_gate_node_adapter
#endif

#if TCFG_MULTI_BAND_DRC_NODE_ENABLE
multiband_drc_node_adapter
#endif

#if TCFG_DATA_EXPORT_NODE_ENABLE
data_export_node_adapter
#endif

#if TCFG_SWITCH_NODE_ENABLE
switch_node_adapter
#endif

#if TCFG_DISTORTION_NODE_ENABLE
distortion_node_adapter
#endif

#ifdef TCFG_LOCAL_TWS_ENABLE
local_tws_file_plug
#endif

#if TCFG_NOISEGATE_PRO_NODE_ENABLE
noisegate_pro_node_adapter
#endif

#if TCFG_SOFWARE_EQ_NODE_ENABLE
sof_eq_node_adapter
#endif

#if TCFG_MULTI_BAND_LIMITER_NODE_ENABLE
multiband_limiter_node_adapter
#endif
#if TCFG_LIMITER_NODE_ENABLE
limiter_node_adapter
#endif

#if TCFG_PLATE_REVERB_LITE_NODE_ENABLE
reverb_lite_node_adapter
#endif

#if TCFG_VIRTUAL_SURROUND_PRO_NODE_ENABLE
upmix_node_adapter
#endif

a2dp_1sbc_codec_private

#if TCFG_BT_SUPPORT_AAC
a2dp_2aac_sink_codec
#endif

#if TCFG_BT_SUPPORT_LDAC
a2dp_4ldac_sink_codec
#endif

#if TCFG_BT_SUPPORT_LC3
a2dp_5lc3_sink_codec
#endif

#if TCFG_BT_SUPPORT_LHDC
a2dp_6lhdc_sink_codec
#endif

#if TCFG_BT_SUPPORT_LHDC_V5
a2dp_7lhdc_v5_sink_codec
#endif


#if	TCFG_CONFIG_DEBUG_RECORD_ENABLE
driver_record
p11_record
ret_instruction_record
sys_stack_record
system_record
task_record
#endif

#if TCFG_SIGNAL_GENERATOR_NODE_ENABLE
signal_generator_file_plug
#endif

#if TCFG_REPLACE_NODE_ENABLE
replace_node_adapter
#endif

#if TCFG_UI_ENABLE
sdfile_resfile_ops
sdfile_vfs_ops
#endif

#if TCFG_REVERB_NODE_ENABLE
reverb_node_adapter
#endif

#if TCFG_TDM_TX_ENABLE
tdm_node_adapter
#endif

#if TCFG_TDM_RX_ENABLE
tdm_file_plug
#endif

#if	TCFG_MULTI_CH_TDM_NODE_ENABLE
multi_ch_tdm_node_adapter
#endif


#if TCFG_MULTI_CH_TDM_RX_NODE_ENABLE
multi_ch_tdm_file_plug
#endif

#if TCFG_SPLIT_GAIN_NODE_ENABLE
split_gain_node_adapter
#endif

#if TCFG_VIRTUAL_BASS_CLASSIC_NODE_ENABLE
virtual_bass_classic_node_adapter
#endif

#if TCFG_PHASER_NODE_ENABLE
phaser_node_adapter
#endif

#if TCFG_FLANGER_NODE_ENABLE
flanger_node_adapter
#endif

#if TCFG_CHORUS_ADVANCE_NODE_ENABLE
chorus_advance_node_adapter
#endif

#if TCFG_PINGPONG_ECHO_NODE_ENABLE
pingpong_echo_node_adapter
#endif

#if TCFG_STEREO_SPATIAL_WIDER_NODE_ENABLE
stereo_spatial_wider_node_adapter
#endif

#if TCFG_DISTORTION_CLIPPING_NODE_ENABLE
distortion_clipping_node_adapter
#endif


