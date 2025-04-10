#ifndef NODE_UUID_H
#define NODE_UUID_H


#define NODE_UUID_SOURCE            0x1100
#define NODE_UUID_DEMUXER           0x1200
#define NODE_UUID_DECODER           0x1A85
#define NODE_UUID_ENCODER           0x0A2C

#define NODE_UUID_TONE              0x768A
#define NODE_UUID_RING              0xCADC
#define NODE_UUID_FILEPLAY          0XF03B
#define NODE_UUID_KEY_TONE          0x8346
#define NODE_UUID_EQ                0x737B
#define NODE_UUID_SRC               0x1ECD
#define NODE_UUID_MIXER             0xE62A
#define NODE_UUID_DAC               0xDCCD
#define NODE_UUID_IIS0_TX           0x69A5
#define NODE_UUID_IIS1_TX           0xF606
#define NODE_UUID_IIS0_RX           0x6963
#define NODE_UUID_IIS1_RX           0xF5C4
#define NODE_UUID_ADC               0xD06D
#define NODE_UUID_A2DP_RX           0xF975
#define NODE_UUID_ESCO_RX           0x8458
#define NODE_UUID_ESCO_TX           0x849A
#define NODE_UUID_BT_AUDIO_SYNC     0xA576
#define NODE_UUID_VOLUME_CTRLER     0x74E3
#define NODE_UUID_MULTIPLEX         0x2C15
#define NODE_UUID_UART_DUMP         0xE76E
#define NODE_UUID_MUTE              0xD5E0
#define NODE_UUID_PLC               0xED7F
#define NODE_UUID_CVP_SMS_ANS      	0xD0BC
#define NODE_UUID_CVP_SMS_DNS      	0xDBF5
#define NODE_UUID_CVP_DMS_ANS      	0x2115
#define NODE_UUID_CVP_DMS_DNS      	0x420E
#define NODE_UUID_CVP_DMS_FLEXIBLE_ANS      	0x90F9
#define NODE_UUID_CVP_DMS_FLEXIBLE_DNS      	0x68F2
#define NODE_UUID_CVP_3MIC          0X0048
#define NODE_UUID_CVP_DMS_HYBRID_DNS            0X5DF0
#define NODE_UUID_CVP_DMS_AWN_DNS               0XF0C3
#define NODE_UUID_CVP_DEVELOP		0X76EF
#define NODE_UUID_AI_TX		      	0xDFDA
#define NODE_UUID_NOISE_SUPPRESSOR 	0x3BC9
#define NODE_UUID_SURROUND_DEMO		0x3F20
#define NODE_UUID_AUTOMUTE          0X86B9
#define NODE_UUID_LINEIN            0X0624
#define NODE_UUID_SPDIF             0XFB3B
#define NODE_UUID_PC_SPK             0XD186
#define NODE_UUID_PC_MIC             0XB711
#define NODE_UUID_MUSIC             0x6FEC
#define NODE_UUID_PDM_MIC	        0XA09F
#define NODE_UUID_CHANNLE_SWAP	    0X36D7
#define NODE_UUID_FM	            0X7398
#define NODE_UUID_SPATIAL_EFFECTS   0X83E1
#define NODE_UUID_WRITE_FILE        0x23C1
#define NODE_UUID_FILE_PACKAGE      0xE2CB
#define NODE_UUID_LE_AUDIO_PCM      0xC4B7
#define NODE_UUID_LE_AUDIO_TX       0xDEF2
#define NODE_UUID_LE_AUDIO_RX       0xDEB0
#define NODE_UUID_LE_AUDIO_PCM      0xC4B7
#define NODE_UUID_LE_AUDIO_SOURCE   0x2D57
#define NODE_UUID_LE_AUDIO_SINK     0x2DFB
#define NODE_UUID_CAPTURE_SYNC      0xFF09
#define NODE_UUID_SPDIF_MASTER      0x9412
#define NODE_UUID_SWITCH            0x2357
#define NODE_UUID_LOCAL_TWS_SOURCE  0xD45F
#define NODE_UUID_LOCAL_TWS_SINK  	0x7303
#define NODE_UUID_TDM_TX            0xB455
#define NODE_UUID_TDM_RX            0xB413

/*通话CVP子模块UUID*/
#define NODE_UUID_AEC				0xA9CE
#define NODE_UUID_NLP				0xD3C9
#define NODE_UUID_ANS				0x69BD
#define NODE_UUID_DNS				0xBC6F
#define NODE_UUID_AGC				0xD0D0

#define NODE_UUID_ENC				0xE2BB
#define NODE_UUID_DMS_AEC			0x1EB2
#define NODE_UUID_DMS_NLP			0xF4CA
#define NODE_UUID_DMS_ANS			0x5721
#define NODE_UUID_DMS_DNS			0x4C53
#define NODE_UUID_DMS_GLOABL		0x62B2

#define NODE_UUID_SOUND_SPLITTER           0xD911//音频分流器
#define NODE_UUID_VOCAL_TRACK_SYNTHESIS    0x503d//声道组合器
#define NODE_UUID_VOCAL_TRACK_SEPARATION   0xA724//声道拆分器
#define NODE_UUID_CROSSOVER                0xBF28//分频器3段
#define NODE_UUID_CROSSOVER_2BAND          0x8D63//分频器2段
#define NODE_UUID_WDRC                     0xDEFE//WDRC
#define NODE_UUID_GAIN                     0xA904//16bit位宽增益控制
#define NODE_UUID_VBASS                    0xB0D5//虚拟低音
#define NODE_UUID_STEROMIX                 0xfb00//立体声左右声道按不同比例混合
#define NODE_UUID_DYNAMIC_EQ               0x87A0//动态eq
#define NODE_UUID_DYNAMIC_EQ_EXT_DETECTOR  0XA60B//动态eq,支持det使用不同的源头
#define NODE_UUID_DYNAMIC_EQ_PRO           0x4731//动态eq pro
#define NODE_UUID_DYNAMIC_EQ_PRO_EXT_DETECTOR 0x793c//动态eq pro ext
#define NODE_UUID_SURROUND                 0x8934//环绕声
#define NODE_UUID_VOICE_CHANGER            0x7293//变声器,针对人声处理
#define NODE_UUID_VOICE_CHANGER_ADV        0x320E//优化了某些场景的变声效果，相应资源消耗更多
#define NODE_UUID_NOISEGATE                0xB7C4//门限噪声
#define NODE_UUID_FREQUENCY_SHIFT          0x6195//移频啸叫抑制
#define NODE_UUID_HOWLING_SUPPRESS         0xC482//陷波啸叫抑制
#define NODE_UUID_PLATE_REVERB_ADVANCE     0x753  //高阶混响
#define NODE_UUID_PLATE_REVERB             0x5101//混响
#define NODE_UUID_ECHO                     0x98a4//回声
#define NODE_UUID_SPECTRUM                 0xF538//频谱计算
#define NODE_UUID_SPECTRUM_ADVANCE         0xF28A//频谱计算,使用eq加速的频谱计算
#define NODE_UUID_AUTOTUNE                 0xc07a//电子音
#define NODE_UUID_VOCAL_REMOVER            0x2F7A//人声消除，只支持立体声
#define NODE_UUID_SPEAKER_EQ               0x3BE6//喇叭频响矫正
#define NODE_UUID_BASS_TREBLE              0xcc8c//高低音
#define NODE_UUID_ZERO_ACTIVE              0x2FE1//零数据产生器
#define NODE_UUID_3BAND_MERGE              0x1B9D//3带合并
#define NODE_UUID_2BAND_MERGE              0x665c//2带合并
#define NODE_UUID_PITCH_SPEED              0x540e//变速变调，针对歌曲处理
#define NODE_UUID_CONVERT                  0x1aa6//饱和溢出位宽转换
#define NODE_UUID_BIT_WIDTH_CONVERT        0x7DE5//位移方式位宽转换
#define NODE_UUID_ENERGY_DETECT            0xA248//能量检测
#define NODE_UUID_STEREO_WIDENER           0x88E5//立体声增强
#define NODE_UUID_PLAY_SYNC                0x9B3B//播放同步
#define NODE_UUID_HARMONIC_EXCITER         0x1B2A//谐波增强
#define NODE_UUID_INDICATOR                0x48E2//分贝指示器
#define NODE_UUID_CHORUS                   0x6DD9//合唱
#define NODE_UUID_CHANNEL_EXPANDER         0xDA15//声道扩展
#define NODE_UUID_CHANNEL_MERGE            0xBF8E//声道合并
#define NODE_UUID_WDRC_DETECTOR            0x9A58//支持外部数据源检测的wdrc
#define NODE_UUID_WDRC_ADVANCE             0x4250//drc_advance
#define NODE_UUID_PCM_DELAY                0xA8F4//延时模块，支持各声道独立设置
#define NODE_UUID_AUTODUCK                 0x6CE5//自动闪避<响应器>
#define NODE_UUID_AUTODUCK_TRIGGER         0x7099//自动闪避<触发器>

#define NODE_UUID_EFFECT_DEV0              0xA4E1//第三方算法0
#define NODE_UUID_EFFECT_DEV1              0xA4E2//第三方算法1
#define NODE_UUID_EFFECT_DEV2              0xA4E3//第三方算法2
#define NODE_UUID_EFFECT_DEV3              0xA4E4//第三方算法3
#define NODE_UUID_EFFECT_DEV4              0xA4E5//第三方算法4

#define NODE_UUID_DATA_SATURATION          0x6B29//位宽饱和处理 24bit或16bit
#define NODE_UUID_AUTOWAH                  0x2F5E//自动哇
#define NODE_UUID_PINGPONG_PCM_DELAY       0xF936//pingpong延时
#define NODE_UUID_THREE_D                  0x8c21 //three d
#define NODE_UUID_LLNS                     0x9FE //低延时降噪
#define NODE_UUID_FADE                     0x1BF5//淡入

#define NODE_UUID_SOURCE_DEV0              0x8FC4//自定义源节点0
#define NODE_UUID_SOURCE_DEV1              0x8FC5//自定义源节点1
#define NODE_UUID_SOURCE_DEV2              0x8FC6//自定义源节点2
#define NODE_UUID_SOURCE_DEV3              0x8FC7//自定义源节点3
#define NODE_UUID_SOURCE_DEV4              0x8FC8//自定义源节点4

#define NODE_UUID_SINK_DEV0                0xB328//自定义输出节点0
#define NODE_UUID_SINK_DEV1                0xB329//自定义输出节点1
#define NODE_UUID_SINK_DEV2                0xB32A//自定义输出节点2
#define NODE_UUID_SINK_DEV3                0xB32B//自定义输出节点3
#define NODE_UUID_SINK_DEV4                0xB32C//自定义输出节点4

#define NODE_UUID_DNS_NOISE_SUPPRESSOR 	   0xAE

#define NODE_UUID_STEREO_MTAPS_ECHO        0x9E7B//立体声echo
#define NODE_UUID_HOWLING_GATE             0x30BE//啸叫门限
#define NODE_UUID_MULTI_CH_IIS0_TX          0xF7FC//多通道iis0
#define NODE_UUID_MULTI_CH_IIS1_TX          0x845D//多通道iis1
#define NODE_UUID_MULTI_CH_TDM_TX          	0x4FEC//多通道TDM
#define NODE_UUID_MULTI_CH_TDM_RX          	0x4FAA//多通道TDM接收
#define NODE_UUID_MDRC                     0x74CB//多带DRC

#define NODE_UUID_DATA_EXPORT 			   0xDE61//数据导出节点

#define NODE_UUID_MULTI_CH_IIS0_RX          0xF7BA//多通道iis0 rx
#define NODE_UUID_MULTI_CH_IIS1_RX          0x841B//多通道iis1 rx

#define NODE_UUID_MULTI_CH_ADC              0xAFC3//多通道adc
#define NODE_UUID_DISTORTION                0x7B81
#define NODE_UUID_NOISEGATE_PRO             0xE955//高阶噪声门限
#define NODE_UUID_SOF_EQ                    0x3845

#define NODE_UUID_LIMITER                  0x4E5B//limiter
#define NODE_UUID_MULTIBAND_LIMITER        0xF58A//多带limiter
#define NODE_UUID_PLATE_REVERB_LITE        0xA48F //混响

#define NODE_UUID_UPMIX_2TO5               0XB542

#define NODE_UUID_REPLACE                  0xb5e1
#define NODE_UUID_SIGNAL_GENERATOR         0x225d
#define NODE_UUID_REVERB                   0x24AB//混响V2
#define NODE_UUID_SPLIT_GAIN               0x2A10
#define NODE_UUID_VIRTUAL_BASS_CLASSIC     0x55C9//virtual bass classic
#define NODE_UUID_FLANGER                  0xBC44
#define NODE_UUID_PHASER                   0xD2E8
#define NODE_UUID_CHORUS_ADVANCE           0x7C2B
#define NODE_UUID_PINGPONG_ECHO            0xAB66
#define NODE_UUID_STEREO_SPATIAL_WIDER     0x6700
#define NODE_UUID_DISTORTION_CLIPPING      0x97AA
#endif
