/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/

#ifndef JLSTREAM_NODE_CFG_H
#define JLSTREAM_NODE_CFG_H

// ------------提示音宏定义------------
#define TCFG_TONE_EN_ENABLE                       0               
#define TCFG_TONE_ZH_ENABLE                       0               
#define TCFG_TONE_AAC_ENABLE                      0                // AAC
#define TCFG_TONE_F2A_ENABLE                      0                // F2A
#define TCFG_TONE_MP3_ENABLE                      0                // MP3
#define TCFG_TONE_MSBC_ENABLE                     0                // MSBC
#define TCFG_TONE_MTY_ENABLE                      0                // MTY
#define TCFG_TONE_SBC_ENABLE                      0                // SBC
#define TCFG_TONE_SIN_ENABLE                      0                // SIN
#define TCFG_TONE_WAV_ENABLE                      0                // WAV
#define TCFG_TONE_WTG_ENABLE                      0                // WTG
#define TCFG_TONE_WTS_ENABLE                      0                // WTS
// ------------提示音宏定义------------

// ------------流程图宏定义------------
#define TCFG_AUDIO_BIT_WIDTH                      0                // 位宽
#define TCFG_2BAND_MERGE_ENABLE                   0                // 2Band Merge
#define TCFG_3BAND_MERGE_ENABLE                   0                // 3Band Merge
#define TCFG_3D_PLUS_MODULE_NODE_ENABLE           0                // 3D Plus
#define TCFG_ADC_NODE_ENABLE                      0                // ADC
#define TCFG_AUDIO_CVP_DEVELOP_ENABLE             0                // 通话第三方算法
#define TCFG_AUDIO_CVP_DMS_ANS_MODE               0                // 双MIC通话
#define TCFG_AUDIO_CVP_DMS_DNS_MODE               0                // 双MIC+DNS
#define TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE      0                // 话务双MIC通话
#define TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE      0                // 话务双MIC+DNS
#define TCFG_AUDIO_CVP_SMS_ANS_MODE               0                // 单MIC通话
#define TCFG_AUDIO_CVP_SMS_DNS_MODE               0                // 单MIC+DNS
#define TCFG_AUTO_WAH_NODE_ENABLE                 0                // Auto Wah
#define TCFG_AUTODUCK_NODE_ENABLE                 0                // AutoDuck、AutoDuck Trigger
#define TCFG_AUTOTUNE_NODE_ENABLE                 0                // Autotune
#define TCFG_BASS_TREBLE_NODE_ENABLE              0                // Bass Treble
#define TCFG_CHANNEL_EXPANDER_NODE_ENABLE         0                // Channel Expander
#define TCFG_CHANNEL_MERGE_NODE_ENABLE            0                // Channel Merge
#define TCFG_CHANNEL_SWAP_NODE_ENABLE             0                // 声道交换器
#define TCFG_CHORUS_ADVANCE_NODE_ENABLE           0                // Chorus Advance
#define TCFG_CHORUS_NODE_ENABLE                   0                // Chorus
#define TCFG_CONVERT_NODE_ENABLE                  0                // BitWidth Convert、Convert
#define TCFG_CROSSOVER_NODE_ENABLE                0                // 分频器 两段、分频器 三段
#define TCFG_DAC_NODE_ENABLE                      0                // DAC
#define TCFG_DATA_EXPORT_NODE_ENABLE              0                // data export
#define TCFG_DATA_SATURATION_NODE_ENABLE          0                // Data Saturation
#define TCFG_DISTORTION_CLIPPING_NODE_ENABLE      0                // Distortion Clipping
#define TCFG_DISTORTION_NODE_ENABLE               0                // DistortionExp
#define TCFG_DYNAMIC_EQ_EXT_DETECTOR_NODE_ENABLE  0                // Dynamic EQ  Ext Detector
#define TCFG_DYNAMIC_EQ_NODE_ENABLE               0                // Dynamic EQ
#define TCFG_DYNAMIC_EQ_PRO_EXT_DETECTOR_NODE_ENABLE 0                // Dynamic EQ  Pro Ext Detector
#define TCFG_DYNAMIC_EQ_PRO_NODE_ENABLE           0                // Dynamic EQ Pro
#define TCFG_ECHO_NODE_ENABLE                     0                // Echo
#define TCFG_EFFECT_DEV0_NODE_ENABLE              0                // EffectDev0
#define TCFG_EFFECT_DEV1_NODE_ENABLE              0                // EffectDev1
#define TCFG_EFFECT_DEV2_NODE_ENABLE              0                // EffectDev2
#define TCFG_EFFECT_DEV3_NODE_ENABLE              0                // EffectDev3
#define TCFG_EFFECT_DEV4_NODE_ENABLE              0                // EffectDev4
#define TCFG_ENERGY_DETECT_NODE_ENABLE            0                // Energy Detect
#define TCFG_EQ_ENABLE                            0                // EQ
#define TCFG_ESCO_RX_NODE_ENABLE                  0                // ESCO_RX
#define TCFG_ESCO_TX_NODE_ENABLE                  0                // ESCO_TX
#define TCFG_FADE_NODE_ENABLE                     0                // Fade
#define TCFG_FILE_PACKAGE_NODE_ENABLE             0                // 封装
#define TCFG_FLANGER_NODE_ENABLE                  0                // Flanger
#define TCFG_FREQUENCY_COMPRESSOR_NODE_ENABLE     0                // Frequency Compressor
#define TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE  0                // Frequency Shift
#define TCFG_GAIN_NODE_ENABLE                     0                // Gain
#define TCFG_HARMONIC_EXCITER_NODE_ENABLE         0                // Harmonic Exciter
#define TCFG_HOWLING_GATE_ENABLE                  0                // Howling Gate
#define TCFG_IIS_NODE_ENABLE                      0                // IIS0_RX、IIS0_TX
#define TCFG_INDICATOR_NODE_ENABLE                0                // Indicator
#define TCFG_LIMITER_NODE_ENABLE                  0                // Limiter
#define TCFG_LLNS_NODE_ENABLE                     0                // LLNS
#define TCFG_MIXER_NODE_ENABLE                    0                // MIXER
#define TCFG_MULTI_BAND_DRC_NODE_ENABLE           0                // MDRC
#define TCFG_MULTI_BAND_LIMITER_NODE_ENABLE       0                // MB Limiter
#define TCFG_MULTI_CH_IIS_NODE_ENABLE             0                // MULTI CH IIS0 TX
#define TCFG_MULTI_CH_IIS_RX_NODE_ENABLE          0                // MULTI CH IIS0 RX
#define TCFG_MUTE_NODE_ENABLE                     0                // mute
#define TCFG_NOISEGATE_NODE_ENABLE                0                // NoiseGate
#define TCFG_NOISEGATE_PRO_NODE_ENABLE            0                // NoiseGate Pro
#define TCFG_NOTCH_HOWLING_NODE_ENABLE            0                // Howling Suppress
#define TCFG_NS_NODE_ENABLE                       0                // Noise Suppressor
#define TCFG_PCM_DELAY_NODE_ENABLE                0                // PCM Delay
#define TCFG_PHASER_NODE_ENABLE                   0                // Phaser
#define TCFG_PING_PONG_PCM_DELAY_NODE_ENABLE      0                // Pingpong Pcm Delay
#define TCFG_PINGPONG_ECHO_NODE_ENABLE            0                // Pingpong Echo
#define TCFG_PITCH_SPEED_NODE_ENABLE              0                // Pitch Speed
#define TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE     0                // Plate Reverb Advance
#define TCFG_PLATE_REVERB_NODE_ENABLE             0                // Plate Reverb
#define TCFG_PLC_NODE_ENABLE                      0                // 丢包修复PLC
#define TCFG_REPLACE_NODE_ENABLE                  0                // Replace
#define TCFG_REVERB_NODE_ENABLE                   0                // Reverb
#define TCFG_SIGNAL_GENERATOR_NODE_ENABLE         0                // SignalGen
#define TCFG_SOFWARE_EQ_NODE_ENABLE               0                // SW EQ
#define TCFG_SOUND_SPLITTER_NODE_ENABLE           0                // 音频分流器
#define TCFG_SPDIF_MASTER_NODE_ENABLE             0                // SPDIF_OUT
#define TCFG_SPEAKER_EQ_NODE_ENABLE               0                // SpeakerEQ
#define TCFG_SPECTRUM_ADVANCE_NODE_ENABLE         0                // Spectrum Advance
#define TCFG_SPECTRUM_NODE_ENABLE                 0                // Spectrum
#define TCFG_SPLIT_GAIN_NODE_ENABLE               0                // Split Gain
#define TCFG_STEREO_MTAPS_ECHO_NODE_ENABLE        0                // Stereo Mtaps Echo
#define TCFG_STEREO_SPATIAL_WIDER_NODE_ENABLE     0                // Stereo Spatial Wider
#define TCFG_STEREO_WIDENER_NODE_ENABLE           0                // Stereo Widener
#define TCFG_STEROMIX_NODE_ENABLE                 0                // SteroMix
#define TCFG_SURROUND_NODE_ENABLE                 0                // Surround Effect
#define TCFG_SWITCH_NODE_ENABLE                   0                // Switch
#define TCFG_THREE_D_EFFECT_NODE_ENABLE           0                // ThreeD
#define TCFG_UART_NODE_ENABLE                     0                // 串口打印
#define TCFG_VBASS_NODE_ENABLE                    0                // Virtual Bass
#define TCFG_VIRTUAL_BASS_PRO_MODULE_NODE_ENABLE  0                // Virtual Bass Pro
#define TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE 0                // Virtual Surround 2to5、Virtual Surround 2to4、Virtual Surround Pro
#define TCFG_VIRTUAL_SURROUND_PRO_NODE_ENABLE     0                // upmix 2to5
#define TCFG_VOCAL_REMOVER_NODE_ENABLE            0                // Vocal Remover
#define TCFG_VOCAL_TRACK_SEPARATION_NODE_ENBALE   0                // 声道拆分
#define TCFG_VOCAL_TRACK_SYNTHESIS_NODE_ENABLE    0                // 声道组合
#define TCFG_VOICE_CHANGER_ADV_NODE_ENABLE        0                // Voice Changer Adv
#define TCFG_VOICE_CHANGER_NODE_ENABLE            0                // Voice Changer
#define TCFG_WDRC_ADVANCE_NODE_ENABLE             0                // DRC Advance
#define TCFG_WDRC_DETECTOR_NODE_ENABLE            0                // DRC Detector
#define TCFG_WDRC_NODE_ENABLE                     0                // DRC
#define TCFG_WRITE_FILE_NODE_ENABLE               0                // 写文件
#define TCFG_ZERO_ACTIVE_NODE_ENABLE              0                // Zero Active
#define EQ_SECTION_MAX                            0x0              // EQ_SECTION_MAX
#define TCFG_STREAM_BIN_ENC_ENABLE                0                // stream.bin加密使能
// ------------流程图宏定义------------

// ------------流程图节点位宽定义------------
#define TCFG_AUDIO_EFX_B711_RUN_MODE              EFx_BW_UNUSED    // PCMIC
#define TCFG_AUDIO_EFX_F975_RUN_MODE              EFx_BW_UNUSED    // A2DP_RX
#define TCFG_AUDIO_EFX_D1B7_RUN_MODE              EFx_BW_UNUSED    // ANC
#define TCFG_AUDIO_EFX_D0D0_RUN_MODE              EFx_BW_UNUSED    // AGC
#define TCFG_AUDIO_EFX_D06D_RUN_MODE              EFx_BW_UNUSED    // ADC
#define TCFG_AUDIO_EFX_DEFE_RUN_MODE              EFx_BW_UNUSED    // DRC
#define TCFG_AUDIO_EFX_DCCD_RUN_MODE              EFx_BW_UNUSED    // DAC
#define TCFG_AUDIO_EFX_8458_RUN_MODE              EFx_BW_UNUSED    // ESCO_RX
#define TCFG_AUDIO_EFX_849A_RUN_MODE              EFx_BW_UNUSED    // ESCO_TX
#define TCFG_AUDIO_EFX_E2BB_RUN_MODE              EFx_BW_UNUSED    // ENC
#define TCFG_AUDIO_EFX_737B_RUN_MODE              EFx_BW_UNUSED    // EQ
#define TCFG_AUDIO_EFX_69A5_RUN_MODE              EFx_BW_UNUSED    // IIS0_TX
#define TCFG_AUDIO_EFX_E62A_RUN_MODE              EFx_BW_UNUSED    // MIXER
#define TCFG_AUDIO_EFX_D5E0_RUN_MODE              EFx_BW_UNUSED    // mute
#define TCFG_AUDIO_EFX_1ECD_RUN_MODE              EFx_BW_16t16     // SRC
#define TCFG_AUDIO_EFX_0A2C_RUN_MODE              EFx_BW_UNUSED    // 编码器
#define TCFG_AUDIO_EFX_E76E_RUN_MODE              EFx_BW_UNUSED    // 串口打印
#define TCFG_AUDIO_EFX_BF28_RUN_MODE              EFx_BW_UNUSED    // 分频器 三段
#define TCFG_AUDIO_EFX_A9CE_RUN_MODE              EFx_BW_UNUSED    // 回声消除AEC
#define TCFG_AUDIO_EFX_D3C9_RUN_MODE              EFx_BW_UNUSED    // 回音压制NLP
#define TCFG_AUDIO_EFX_69BD_RUN_MODE              EFx_BW_UNUSED    // 降噪NS
#define TCFG_AUDIO_EFX_1A85_RUN_MODE              EFx_BW_UNUSED    // 解码器
#define TCFG_AUDIO_EFX_A576_RUN_MODE              EFx_BW_UNUSED    // 蓝牙音频同步
#define TCFG_AUDIO_EFX_BC6F_RUN_MODE              EFx_BW_UNUSED    // 神经网络降噪DNS
#define TCFG_AUDIO_EFX_A724_RUN_MODE              EFx_BW_UNUSED    // 声道拆分
#define TCFG_AUDIO_EFX_768A_RUN_MODE              EFx_BW_UNUSED    // 提示音
#define TCFG_AUDIO_EFX_23C1_RUN_MODE              EFx_BW_UNUSED    // 写文件
#define TCFG_AUDIO_EFX_74E3_RUN_MODE              EFx_BW_UNUSED    // 音量控制器
#define TCFG_AUDIO_EFX_D911_RUN_MODE              EFx_BW_UNUSED    // 音频分流器
#define TCFG_AUDIO_EFX_62B2_RUN_MODE              EFx_BW_UNUSED    // DMS全局压制
#define TCFG_AUDIO_EFX_1EB2_RUN_MODE              EFx_BW_UNUSED    // DMS回声消除AEC
#define TCFG_AUDIO_EFX_F4CA_RUN_MODE              EFx_BW_UNUSED    // DMS回声压制NLP
#define TCFG_AUDIO_EFX_5721_RUN_MODE              EFx_BW_UNUSED    // DMS降噪NS
#define TCFG_AUDIO_EFX_4C53_RUN_MODE              EFx_BW_UNUSED    // DMS神经网络降噪DNS
#define TCFG_AUDIO_EFX_ED7F_RUN_MODE              EFx_BW_UNUSED    // 丢包修复PLC
#define TCFG_AUDIO_EFX_CADC_RUN_MODE              EFx_BW_UNUSED    // 铃声
#define TCFG_AUDIO_EFX_8346_RUN_MODE              EFx_BW_UNUSED    // 按键音
#define TCFG_AUDIO_EFX_503D_RUN_MODE              EFx_BW_UNUSED    // 声道组合
#define TCFG_AUDIO_EFX_87A0_RUN_MODE              EFx_BW_UNUSED    // Dynamic EQ
#define TCFG_AUDIO_EFX_B0D5_RUN_MODE              EFx_BW_UNUSED    // Virtual Bass
#define TCFG_AUDIO_EFX_A904_RUN_MODE              EFx_BW_UNUSED    // Gain
#define TCFG_AUDIO_EFX_DFDA_RUN_MODE              EFx_BW_UNUSED    // AI_TX
#define TCFG_AUDIO_EFX_FB00_RUN_MODE              EFx_BW_UNUSED    // SteroMix
#define TCFG_AUDIO_EFX_8934_RUN_MODE              EFx_BW_UNUSED    // Surround Effect
#define TCFG_AUDIO_EFX_7293_RUN_MODE              EFx_BW_UNUSED    // Voice Changer
#define TCFG_AUDIO_EFX_B7C4_RUN_MODE              EFx_BW_UNUSED    // NoiseGate
#define TCFG_AUDIO_EFX_6195_RUN_MODE              EFx_BW_UNUSED    // Frequency Shift
#define TCFG_AUDIO_EFX_C482_RUN_MODE              EFx_BW_UNUSED    // Howling Suppress
#define TCFG_AUDIO_EFX_5101_RUN_MODE              EFx_BW_UNUSED    // Plate Reverb
#define TCFG_AUDIO_EFX_98A4_RUN_MODE              EFx_BW_UNUSED    // Echo
#define TCFG_AUDIO_EFX_3BC9_RUN_MODE              EFx_BW_UNUSED    // Noise Suppressor
#define TCFG_AUDIO_EFX_F538_RUN_MODE              EFx_BW_UNUSED    // Spectrum
#define TCFG_AUDIO_EFX_C07A_RUN_MODE              EFx_BW_UNUSED    // Autotune
#define TCFG_AUDIO_EFX_2F7A_RUN_MODE              EFx_BW_UNUSED    // Vocal Remover
#define TCFG_AUDIO_EFX_3BE6_RUN_MODE              EFx_BW_UNUSED    // SpeakerEQ
#define TCFG_AUDIO_EFX_2FE1_RUN_MODE              EFx_BW_UNUSED    // Zero Active
#define TCFG_AUDIO_EFX_CC8C_RUN_MODE              EFx_BW_UNUSED    // Bass Treble
#define TCFG_AUDIO_EFX_1B9D_RUN_MODE              EFx_BW_UNUSED    // 3Band Merge
#define TCFG_AUDIO_EFX_665C_RUN_MODE              EFx_BW_UNUSED    // 2Band Merge
#define TCFG_AUDIO_EFX_540E_RUN_MODE              EFx_BW_UNUSED    // Pitch Speed
#define TCFG_AUDIO_EFX_1AA6_RUN_MODE              EFx_BW_UNUSED    // Convert
#define TCFG_AUDIO_EFX_A248_RUN_MODE              EFx_BW_UNUSED    // Energy Detect
#define TCFG_AUDIO_EFX_0624_RUN_MODE              EFx_BW_UNUSED    // LINEIN
#define TCFG_AUDIO_EFX_6FEC_RUN_MODE              EFx_BW_UNUSED    // 本地音乐
#define TCFG_AUDIO_EFX_8D63_RUN_MODE              EFx_BW_UNUSED    // 分频器 两段
#define TCFG_AUDIO_EFX_88E5_RUN_MODE              EFx_BW_UNUSED    // Stereo Widener
#define TCFG_AUDIO_EFX_D0BC_RUN_MODE              EFx_BW_UNUSED    // 单MIC通话
#define TCFG_AUDIO_EFX_DBF5_RUN_MODE              EFx_BW_UNUSED    // 单MIC+DNS
#define TCFG_AUDIO_EFX_2115_RUN_MODE              EFx_BW_UNUSED    // 双MIC通话
#define TCFG_AUDIO_EFX_420E_RUN_MODE              EFx_BW_UNUSED    // 双MIC+DNS
#define TCFG_AUDIO_EFX_76EF_RUN_MODE              EFx_BW_UNUSED    // 通话第三方算法
#define TCFG_AUDIO_EFX_1B2A_RUN_MODE              EFx_BW_UNUSED    // Harmonic Exciter
#define TCFG_AUDIO_EFX_9B3B_RUN_MODE              EFx_BW_UNUSED    // 播放同步
#define TCFG_AUDIO_EFX_6DD9_RUN_MODE              EFx_BW_UNUSED    // Chorus
#define TCFG_AUDIO_EFX_48E2_RUN_MODE              EFx_BW_UNUSED    // Indicator
#define TCFG_AUDIO_EFX_FB3B_RUN_MODE              EFx_BW_UNUSED    // SPDIF
#define TCFG_AUDIO_EFX_7398_RUN_MODE              EFx_BW_UNUSED    // FM
#define TCFG_AUDIO_EFX_0753_RUN_MODE              EFx_BW_UNUSED    // Plate Reverb Advance
#define TCFG_AUDIO_EFX_DA15_RUN_MODE              EFx_BW_UNUSED    // Channel Expander
#define TCFG_AUDIO_EFX_BAB1_RUN_MODE              EFx_BW_UNUSED    // test_group
#define TCFG_AUDIO_EFX_6963_RUN_MODE              EFx_BW_UNUSED    // IIS0_RX
#define TCFG_AUDIO_EFX_A60B_RUN_MODE              EFx_BW_UNUSED    // Dynamic EQ  Ext Detector
#define TCFG_AUDIO_EFX_9A58_RUN_MODE              EFx_BW_UNUSED    // DRC Detector
#define TCFG_AUDIO_EFX_A8F4_RUN_MODE              EFx_BW_UNUSED    // PCM Delay
#define TCFG_AUDIO_EFX_BF8E_RUN_MODE              EFx_BW_UNUSED    // Channel Merge
#define TCFG_AUDIO_EFX_90F9_RUN_MODE              EFx_BW_UNUSED    // 话务双MIC通话
#define TCFG_AUDIO_EFX_68F2_RUN_MODE              EFx_BW_UNUSED    // 话务双MIC+DNS
#define TCFG_AUDIO_EFX_7099_RUN_MODE              EFx_BW_UNUSED    // AutoDuck Trigger
#define TCFG_AUDIO_EFX_6CE5_RUN_MODE              EFx_BW_UNUSED    // AutoDuck
#define TCFG_AUDIO_EFX_A4E1_RUN_MODE              EFx_BW_UNUSED    // EffectDev0
#define TCFG_AUDIO_EFX_A4E2_RUN_MODE              EFx_BW_UNUSED    // EffectDev1
#define TCFG_AUDIO_EFX_A4E3_RUN_MODE              EFx_BW_UNUSED    // EffectDev2
#define TCFG_AUDIO_EFX_A4E4_RUN_MODE              EFx_BW_UNUSED    // EffectDev3
#define TCFG_AUDIO_EFX_A4E5_RUN_MODE              EFx_BW_UNUSED    // EffectDev4
#define TCFG_AUDIO_EFX_A09F_RUN_MODE              EFx_BW_UNUSED    // PDM MIC
#define TCFG_AUDIO_EFX_E2CB_RUN_MODE              EFx_BW_UNUSED    // 封装
#define TCFG_AUDIO_EFX_4250_RUN_MODE              EFx_BW_UNUSED    // DRC Advance
#define TCFG_AUDIO_EFX_320E_RUN_MODE              EFx_BW_UNUSED    // Voice Changer Adv
#define TCFG_AUDIO_EFX_7DE5_RUN_MODE              EFx_BW_UNUSED    // BitWidth Convert
#define TCFG_AUDIO_EFX_603A_RUN_MODE              EFx_BW_UNUSED    // Sink
#define TCFG_AUDIO_EFX_EB56_RUN_MODE              EFx_BW_UNUSED    // Source
#define TCFG_AUDIO_EFX_6B29_RUN_MODE              EFx_BW_UNUSED    // Data Saturation
#define TCFG_AUDIO_EFX_4731_RUN_MODE              EFx_BW_UNUSED    // Dynamic EQ Pro
#define TCFG_AUDIO_EFX_793C_RUN_MODE              EFx_BW_UNUSED    // Dynamic EQ  Pro Ext Detector
#define TCFG_AUDIO_EFX_2F5E_RUN_MODE              EFx_BW_UNUSED    // Auto Wah
#define TCFG_AUDIO_EFX_F936_RUN_MODE              EFx_BW_UNUSED    // Pingpong Pcm Delay
#define TCFG_AUDIO_EFX_9412_RUN_MODE              EFx_BW_UNUSED    // SPDIF_OUT
#define TCFG_AUDIO_EFX_8C21_RUN_MODE              EFx_BW_UNUSED    // ThreeD
#define TCFG_AUDIO_EFX_D186_RUN_MODE              EFx_BW_UNUSED    // PCSPK
#define TCFG_AUDIO_EFX_09FE_RUN_MODE              EFx_BW_UNUSED    // LLNS
#define TCFG_AUDIO_EFX_F28A_RUN_MODE              EFx_BW_UNUSED    // Spectrum Advance
#define TCFG_AUDIO_EFX_9E7B_RUN_MODE              EFx_BW_UNUSED    // Stereo Mtaps Echo
#define TCFG_AUDIO_EFX_D45F_RUN_MODE              EFx_BW_UNUSED    // LocalTWS Source
#define TCFG_AUDIO_EFX_2357_RUN_MODE              EFx_BW_UNUSED    // Switch
#define TCFG_AUDIO_EFX_7303_RUN_MODE              EFx_BW_UNUSED    // LocalTWS Sink
#define TCFG_AUDIO_EFX_74CB_RUN_MODE              EFx_BW_UNUSED    // MDRC
#define TCFG_AUDIO_EFX_1BF5_RUN_MODE              EFx_BW_UNUSED    // Fade
#define TCFG_AUDIO_EFX_DE61_RUN_MODE              EFx_BW_UNUSED    // data export
#define TCFG_AUDIO_EFX_225D_RUN_MODE              EFx_BW_UNUSED    // SignalGen
#define TCFG_AUDIO_EFX_B5E1_RUN_MODE              EFx_BW_UNUSED    // Replace
#define TCFG_AUDIO_EFX_24AB_RUN_MODE              EFx_BW_UNUSED    // Reverb
#define TCFG_AUDIO_EFX_B542_RUN_MODE              EFx_BW_UNUSED    // upmix 2to5
#define TCFG_AUDIO_EFX_3845_RUN_MODE              EFx_BW_UNUSED    // SW EQ
#define TCFG_AUDIO_EFX_4E5B_RUN_MODE              EFx_BW_UNUSED    // Limiter
#define TCFG_AUDIO_EFX_F58A_RUN_MODE              EFx_BW_UNUSED    // MB Limiter
#define TCFG_AUDIO_EFX_191F_RUN_MODE              EFx_BW_UNUSED    // Virtual Surround Pro
#define TCFG_AUDIO_EFX_E955_RUN_MODE              EFx_BW_UNUSED    // NoiseGate Pro
#define TCFG_AUDIO_EFX_36D7_RUN_MODE              EFx_BW_UNUSED    // 声道交换器
#define TCFG_AUDIO_EFX_D257_RUN_MODE              EFx_BW_UNUSED    // Virtual Surround 2to4
#define TCFG_AUDIO_EFX_F7BA_RUN_MODE              EFx_BW_UNUSED    // MULTI CH IIS0 RX
#define TCFG_AUDIO_EFX_F7FC_RUN_MODE              EFx_BW_UNUSED    // MULTI CH IIS0 TX
#define TCFG_AUDIO_EFX_D258_RUN_MODE              EFx_BW_UNUSED    // Virtual Surround 2to5
#define TCFG_AUDIO_EFX_4C00_RUN_MODE              EFx_BW_UNUSED    // 3D Plus
#define TCFG_AUDIO_EFX_7B81_RUN_MODE              EFx_BW_UNUSED    // DistortionExp
#define TCFG_AUDIO_EFX_30BE_RUN_MODE              EFx_BW_UNUSED    // Howling Gate
#define TCFG_AUDIO_EFX_FF09_RUN_MODE              EFx_BW_UNUSED    // LE_Audio延时同步
#define TCFG_AUDIO_EFX_2DFB_RUN_MODE              EFx_BW_UNUSED    // LE_Audio_Sink
#define TCFG_AUDIO_EFX_2D57_RUN_MODE              EFx_BW_UNUSED    // LE_Audio_Source
#define TCFG_AUDIO_EFX_02E6_RUN_MODE              EFx_BW_UNUSED    // Virtual Bass Pro
#define TCFG_AUDIO_EFX_2A10_RUN_MODE              EFx_BW_UNUSED    // Split Gain
#define TCFG_AUDIO_EFX_7C2B_RUN_MODE              EFx_BW_UNUSED    // Chorus Advance
#define TCFG_AUDIO_EFX_AB66_RUN_MODE              EFx_BW_UNUSED    // Pingpong Echo
#define TCFG_AUDIO_EFX_D2E8_RUN_MODE              EFx_BW_UNUSED    // Phaser
#define TCFG_AUDIO_EFX_BC44_RUN_MODE              EFx_BW_UNUSED    // Flanger
#define TCFG_AUDIO_EFX_6700_RUN_MODE              EFx_BW_UNUSED    // Stereo Spatial Wider
#define TCFG_AUDIO_EFX_97AA_RUN_MODE              EFx_BW_UNUSED    // Distortion Clipping
#define TCFG_AUDIO_EFX_BFE4_RUN_MODE              EFx_BW_UNUSED    // Frequency Compressor
// ------------流程图节点位宽定义------------


// ------------流程图EQ节点类型宏定义------------
#define EQ_CFG_TYPE_HIGH_PASS                     0                // High Pass
#define EQ_CFG_TYPE_LOW_PASS                      0                // Low Pass
#define EQ_CFG_TYPE_PEAKING                       0                // Peaking
#define EQ_CFG_TYPE_HIGH_SHELF                    0                // High Shelf
#define EQ_CFG_TYPE_LOW_SHELF                     0                // Low Shelf
#define EQ_CFG_TYPE_HIGH_SHELF_Q                  0                // High Shelf Q
#define EQ_CFG_TYPE_LOW_SHELF_Q                   0                // Low Shelf Q
#define EQ_CFG_TYPE_HP                            0                // Hp
#define EQ_CFG_TYPE_LP                            0                // Lp
// ------------流程图EQ节点类型宏定义------------
#endif
