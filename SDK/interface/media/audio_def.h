/*
 *******************************************************************
 *						Audio Common Definitions
 *
 *Note(s):
 *		 (1)Only macro definitions can be defined here.
 *		 (2)Use (1UL << (n)) instead of BIT(n)
 *******************************************************************
 */
#ifndef _AUDIO_DEF_H_
#define _AUDIO_DEF_H_

/*
 *******************************************************************
 *						Audio Common Definitions
 *******************************************************************
 */
//Audio I/O Mode
#define AUDIO_IO_SINGLE_ENDED			0	//单端:Single-Ended
#define AUDIO_IO_DIFFERENTIAL			1	//差分:Differential

//Volume Mode
#define  VOL_TYPE_DIGITAL				0	//软件数字音量
#define  VOL_TYPE_ANALOG				1	//硬件模拟音量
#define  VOL_TYPE_AD					2	//联合音量(模拟数字混合调节)
#define  VOL_TYPE_DIGITAL_HW			3  	//硬件数字音量

//数据位宽定义
#define  DATA_BIT_WIDE_16BIT  			0
#define  DATA_BIT_WIDE_24BIT  			1
#define  DATA_BIT_WIDE_32BIT  			2
#define  DATA_BIT_WIDE_32BIT_FLOAT  	3

/*
 *******************************************************************
 *						DAC Definitions
 *******************************************************************
 */
#define DAC_DSM_6MHz                       (0)
#define DAC_DSM_12MHz                      (1)

#define DAC_OUTPUT_MONO_L                  (0)   //左声道
#define DAC_OUTPUT_MONO_R                  (1)   //右声道
#define DAC_OUTPUT_LR                      (2)   //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            (3)   //左右差分输出

#define DAC_OUTPUT_DUAL_LR_DIFF            (6)   //双声道差分
#define DAC_OUTPUT_FRONT_LR_REAR_L         (7)   //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_R         (8)   //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_LR        (9)   //四声道单端输出

#define DAC_BIT_WIDTH_16                   (0)   //16bit 位宽
#define DAC_BIT_WIDTH_24                   (1)   //24bit 位宽

#define DAC_CH_FL                          (1UL << 0)
#define DAC_CH_FR                          (1UL << 1)
#define DAC_CH_RL                          (1UL << 2)
#define DAC_CH_RR                          (1UL << 3)

#define DAC_UNMUTE                         (0)
#define DAC_MUTE                           (1)
/*
 *******************************************************************
 *						Class-D Driver Definitions
 *******************************************************************
 */

#define CLASSD_PA_MODE                     (1UL << 0)
#define CLASSD_APIO_MODE                   (1UL << 1)
#define CLASSD_GPIO_MODE				   (1UL << 2)

#define EPA_DSM_MODE_375K                  (0)
#define EPA_DSM_MODE_750K                  (1)
#define EPA_DSM_MODE_1500K                 (2)

#define EPA_PWM_MODE0                      (0)
#define EPA_PWM_MODE1                      (1)
#define EPA_PWM_MODE2                      (2)
/*
 *******************************************************************
 *						Analog Aux Definitions
 *******************************************************************
 */
#define AUX_CH0                            (1UL << 0)
#define AUX_CH1                            (1UL << 1)

#define AUX_AIN_PORT0                      (1UL << 0)
#define AUX_AIN_PORT1                      (1UL << 1)
#define AUX_AIN_PORT2                      (1UL << 2)
#define AUX_AIN_PORT3                      (1UL << 3)
#define AUX_AIN_PORT4                      (1UL << 4)
/*
 *******************************************************************
 *						ADC Definitions
 *******************************************************************
 */
#define ADC_BIT_WIDTH_16                   (0)   //16bit 位宽
#define ADC_BIT_WIDTH_24                   (1)   //24bit 位宽

#define ADC_AIN_PORT0                      (1UL << 0)
#define ADC_AIN_PORT1                      (1UL << 1)
#define ADC_AIN_PORT2                      (1UL << 2)
#define ADC_AIN_PORT3                      (1UL << 3)
#define ADC_AIN_PORT4                      (1UL << 4)

#define MIC_LDO_STA_CLOSE                  (0)   //MICLDO电源关闭
#define MIC_LDO_STA_OPEN                   (1)   //MICLDO电源开启

/*
 *******************************************************************
 *						FFT Definitions
 *******************************************************************
 */
/*
 * 标准模式:支持2的指数次幂点数
 * Platforms：br23/br25/br30/br34/br36/br29
 */
#define FFT_V3				3

/*
 * 扩展模式 V1:FFT硬件模块支持非2的指数次幂点数.
 * Platforms：br27/br28/br50/br52
 */
#define FFT_EXT				4

/*
 * 扩展模式 V2:
 * Platforms：br56
 */
#define FFT_EXT_V2			5

/*
 *******************************************************************
 *						Codec Definitions
 *******************************************************************
 */

#define AUDIO_CODING_UNKNOW       0x00000000
#define AUDIO_CODING_MP3          0x00000001
#define AUDIO_CODING_WMA          0x00000002
#define AUDIO_CODING_WAV          0x00000004
#define AUDIO_CODING_JLA_LW       0x00000008
#define AUDIO_CODING_SBC          0x00000010
#define AUDIO_CODING_MSBC         0x00000020
#define AUDIO_CODING_G729         0x00000040
#define AUDIO_CODING_CVSD         0x00000080
#define AUDIO_CODING_PCM          0x00000100
#define AUDIO_CODING_AAC          0x00000200
#define AUDIO_CODING_MTY          0x00000400
#define AUDIO_CODING_FLAC         0x00000800
#define AUDIO_CODING_APE          0x00001000
#define AUDIO_CODING_M4A          0x00002000
#define AUDIO_CODING_AMR          0x00004000
#define AUDIO_CODING_DTS          0x00008000
#define AUDIO_CODING_APTX         0x00010000
#define AUDIO_CODING_LDAC         0x00020000
#define AUDIO_CODING_G726         0x00040000
#define AUDIO_CODING_MIDI         0x00080000
#define AUDIO_CODING_OPUS         0x00100000
#define AUDIO_CODING_SPEEX        0x00200000
#define AUDIO_CODING_LC3          0x00400000
#define AUDIO_CODING_JLA_LL       0x00800000
#define AUDIO_CODING_WTGV2        0x01000000
#define AUDIO_CODING_ALAC         0x02000000
#define AUDIO_CODING_SINE         0x04000000
#define AUDIO_CODING_F2A          0x08000000
#define AUDIO_CODING_JLA_V2       0x0A000000
#define AUDIO_CODING_AIFF         0x10000000
#define AUDIO_CODING_JLA          0x20000000
#define AUDIO_CODING_OGG          0x40000000
#define AUDIO_CODING_LHDC         0x80000000
#define AUDIO_CODING_LHDC_V5      0xA0000000

//#define AUDIO_CODING_STU_PICK     0x10000000
//#define AUDIO_CODING_STU_APP      0x20000000

//MSBC Dec Debug
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_DISABLE		0
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_SILENCE		1
#define CONFIG_MSBC_INPUT_FRAME_REPLACE_SINE		2

/*
 *******************************************************************
 *						Linein(Aux) Definitions
 *******************************************************************
 */


/*
 *******************************************************************
 *						ANC Definitions
 *******************************************************************
 */
//ANC Mode Enable
#define ANC_FF_EN 		 			(1UL << (0))
#define ANC_FB_EN  					(1UL << (1))
#define ANC_HYBRID_EN  			 	(1UL << (2))

//ANC芯片版本定义
#define ANC_VERSION_BR30 			0x01	//AD697N/AC897N
#define ANC_VERSION_BR30C			0x02	//AC699N/AD698N
#define ANC_VERSION_BR36			0x03	//AC700N
#define ANC_VERSION_BR28			0x04	//JL701N/BR40
#define ANC_VERSION_BR28_MULT		0xA4	//JL701N 多滤波器
#define ANC_VERSION_BR50			0x05	//JL708N

/*
 *******************************************************************
 *						Smart Voice Definitions
 *******************************************************************
 */
/*离线语音识别语言选择*/
#define KWS_CH          1   /*中文*/
#define KWS_INDIA_EN    2   /*印度英语*/
#define KWS_FAR_CH      3   /*音箱中文*/

/*
 *******************************************************************
 *						Bluetooth Audio Definitions
 *******************************************************************
 */
/*TWS通话，从机是否出声*/
#define TWS_ESCO_MASTER_AND_SLAVE      1    //TWS通话主从机同时出声
#define TWS_ESCO_ONLY_MASTER           2    //TWS通话只有主机出声

/*
 *******************************************************************
 *						Effect Definitions
 *******************************************************************
 */
//算法输入输出位宽使能位定义
#define  EFx_BW_UNUSED 	                    (0)
#define  EFx_BW_16t16 	                    (1UL << (0))
#define  EFx_BW_16t32		                (1UL << (1))
#define  EFx_BW_32t16		                (1UL << (2))
#define  EFx_BW_32t32		                (1UL << (3))
#define  EFx_PRECISION_NOR                  (1UL << (4)) //precision(精度为高 普通 最低使能)
#define  EFx_PRECISION_PRO                  (1UL << (5)) //precision+(精度为最高使能)
#define  EFx_MODULE_MONO_EN                 (1UL << (6)) //支持单进单出
#define  EFx_MODULE_STEREO_EN               (1UL << (7)) //支持双进双出

//Limiter精度使能位定义
#define  LIMITER_PRECISION_HIGH_NORMAL_LOW  EFx_PRECISION_NOR//高、普通、最低
#define  LIMITER_PRECISION_MAX              EFx_PRECISION_PRO //最高

#endif/*_AUDIO_DEF_H_*/
