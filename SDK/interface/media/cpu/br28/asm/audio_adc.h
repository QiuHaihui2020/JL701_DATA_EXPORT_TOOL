#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"
#include "system/spinlock.h"
#include "cpu/includes.h"

/*无电容电路*/
#define SUPPORT_MIC_CAPLESS     1

#define LADC_STATE_INIT			1
#define LADC_STATE_OPEN      	2
#define LADC_STATE_START     	3
#define LADC_STATE_STOP      	4

#define FPGA_BOARD          	0

#define LADC_MIC                0
#define LADC_LINEIN             1

/* ADC 最大通道数 */
#define AUDIO_ADC_MAX_NUM           (4)
#define AUDIO_ADC_MIC_MAX_NUM       (4)
#define AUDIO_ADC_LINEIN_MAX_NUM    (4)

/* 通道选择 */
#define AUDIO_ADC_MIC(x)					BIT(x)
#define AUDIO_ADC_MIC_0					    BIT(0)
#define AUDIO_ADC_MIC_1					    BIT(1)
#define AUDIO_ADC_MIC_2					    BIT(2)
#define AUDIO_ADC_MIC_3					    BIT(3)
#define AUDIO_ADC_LINE(x) 					BIT(x)
#define AUDIO_ADC_LINE0 					BIT(0)
#define AUDIO_ADC_LINE1  					BIT(1)
#define AUDIO_ADC_LINE2 					BIT(2)
#define AUDIO_ADC_LINE3  					BIT(3)
#define PLNK_MIC		            		BIT(6)
#define ALNK_MIC				            BIT(7)

/*******************************应用层**********************************/
/* 通道选择 */
#define AUDIO_ADC_MIC_CH		            AUDIO_ADC_MIC_0

/********************************************************************************
                          mic_mode 工作模式配置
 ********************************************************************************/
// TCFG_AUDIO_MICx_MODE
#define AUDIO_MIC_CAP_MODE                  0   //单端隔直电容模式
#define AUDIO_MIC_CAP_DIFF_MODE             1   //差分隔直电容模式
#define AUDIO_MIC_CAPLESS_MODE              2   //单端省电容模式

/********************************************************************************
                          linein_mode 工作模式配置
 ********************************************************************************/
#define AUDIO_LINEIN_MODE          			0   //单端linein 模式
#define AUDIO_LINEIN_DIFF_MODE          	1   //差分linein 模式
/********************************************************************************
                MICx  输入IO配置(要注意IO与mic bias 供电IO配置互斥)
 ********************************************************************************/
// TCFG_AUDIO_MIC0_AIN_SEL
#define AUDIO_MIC0_CH0                BIT(0)    // PA1
#define AUDIO_MIC0_CH1                BIT(1)    // 不支持
#define AUDIO_MIC0_CH2                BIT(2)    // 不支持
// 当mic0模式选择差分模式时，输入N端引脚固定为  PA2

// TCFG_AUDIO_MIC1_AIN_SEL
#define AUDIO_MIC1_CH0                BIT(0)    // PA3
#define AUDIO_MIC1_CH1                BIT(1)    // 不支持
#define AUDIO_MIC1_CH2                BIT(2)    // 不支持
// 当mic1模式选择差分模式时，输入N端引脚固定为  PA4

// TCFG_AUDIO_MIC2_AIN_SEL
#define AUDIO_MIC2_CH0                BIT(0)    // PG8
#define AUDIO_MIC2_CH1                BIT(1)    // 不支持
// 当mic2模式选择差分模式时，输入N端引脚固定为  PG7

// TCFG_AUDIO_MIC3_AIN_SEL
#define AUDIO_MIC3_CH0                BIT(0)    // PG6
#define AUDIO_MIC3_CH1                BIT(1)    // 不支持
// 当mic3模式选择差分模式时，输入N端引脚固定为  PG5

/********************************************************************************
                MICx  mic bias 供电IO配置(要注意IO与micin IO配置互斥)
 ********************************************************************************/
// TCFG_AUDIO_MICx_BIAS_SEL
#define AUDIO_MIC_BIAS_NULL           (0)       // no bias
#define AUDIO_MIC_BIAS_CH0            BIT(0)    // PA2
#define AUDIO_MIC_BIAS_CH1            BIT(1)    // PA4
#define AUDIO_MIC_BIAS_CH2            BIT(2)    // PG7
#define AUDIO_MIC_BIAS_CH3            BIT(3)    // PG5
#define AUDIO_MIC_LDO_PWR             BIT(4)    // PA0

/********************************************************************************
                         LINEINx  输入IO配置
 ********************************************************************************/
// TCFG_AUDIO_LINEIN0_AIN_SEL
#define AUDIO_LINEIN0_CH0                BIT(0)    // PA1
#define AUDIO_LINEIN0_CH1                BIT(1)    // 不支持
#define AUDIO_LINEIN0_CH2                BIT(2)    // 不支持

// TCFG_AUDIO_LINEIN1_AIN_SEL
#define AUDIO_LINEIN1_CH0                BIT(0)    // PB8
#define AUDIO_LINEIN1_CH1                BIT(1)    // 不支持
#define AUDIO_LINEIN1_CH2                BIT(2)    // 不支持

// TCFG_AUDIO_LINEIN2_AIN_SEL
#define AUDIO_LINEIN2_CH0                BIT(0)    // 不支持
#define AUDIO_LINEIN2_CH1                BIT(1)    // 不支持

// TCFG_AUDIO_LINEIN3_AIN_SEL
#define AUDIO_LINEIN3_CH0                BIT(0)    // 不支持
#define AUDIO_LINEIN3_CH1                BIT(1)    // 不支持


/*Audio ADC输入源选择*/
#define AUDIO_ADC_SEL_AMIC0			0
#define AUDIO_ADC_SEL_AMIC1			1
#define AUDIO_ADC_SEL_DMIC0			2
#define AUDIO_ADC_SEL_DMIC1			3

struct capless_low_pass {
    u16 bud; //快调边界
    u16 count;
    u16 pass_num;
    u16 tbidx;
    u32 bud_factor;
};

struct audio_adc_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, s16 *, int);
};

struct audio_adc_private_param {
    u8 mic_ldo_vsel;		//MIC0_LDO[000:1.5v 001:1.8v 010:2.1v 011:2.4v 100:2.7v 101:3.0v]
    u8 mic_ldo_isel; 		//MIC0通道电流档位选择
    u8 adca_reserved0;
    u8 adcb_reserved0;
    u8 lowpower_lvl;
};

struct mic_open_param {
    u8 mic_ain_sel;       // 0/1/2
    u8 mic_bias_sel;      // A(PA0)/B(PA1)/C(PC10)/D(PA5)
    u8 mic_bias_rsel;     // 单端隔直电容mic bias rsel
    u8 mic_mode : 4;      // MIC工作模式
    u8 mic_dcc : 4;       // DCC level
};

struct linein_open_param {
    u8 linein_ain_sel;       // 0/1/2
    u8 linein_mode : 4;      // LINEIN 工作模式
    u8 linein_dcc : 4;       // DCC level
};

struct audio_adc_hdl {
    struct list_head head;
    //const struct adc_platform_data *pd;
    struct audio_adc_private_param *private;
    spinlock_t lock;
#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp0;
    struct capless_low_pass lp1;
    struct capless_low_pass *lp;
#endif/*SUPPORT_MIC_CAPLESS*/
    u8 adc_sel[AUDIO_ADC_MAX_NUM];
    u8 adc_dcc[AUDIO_ADC_MAX_NUM];
    struct mic_open_param mic_param[AUDIO_ADC_MAX_NUM];
    struct linein_open_param linein_param[AUDIO_ADC_MAX_NUM];
    u8 mic_ldo_state;
    u8 state;
    u8 channel;
    u8 channel_num;
    u8 mic_num;
    u8 linein_num;
    s16 *hw_buf;   //ADC 硬件buffer的地址
    u8 max_adc_num; //默认打开的ADC通道数
    u8 buf_fixed;  //是否固定adc硬件使用的buffer地址
    u8 bit_width;
    OS_MUTEX mutex;
    u32 timestamp;
};

struct adc_mic_ch {
    struct audio_adc_hdl *adc;
    u8 gain[AUDIO_ADC_MIC_MAX_NUM];
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_mic_ch *, s16 *, u16);
    u8 channel;
};

struct adc_linein_ch {
    struct audio_adc_hdl *adc;
    u8 gain[AUDIO_ADC_LINEIN_MAX_NUM];
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_linein_ch *, s16 *, u16);
    u8 channel;
};

//MIC相关管理结构
typedef struct {
    u8 en;				//MIC使能
    u8 type;			//ANC MIC类型
    u8 gain;			//MIC增益
    u8 mult_flag;		//通话复用标志
    struct mic_open_param mic_p;
} audio_adc_mic_mana_t;

/*
 * p11系统VCO ADC mic模拟配置数据
 * 仅支持mic0，故应和主系统mic0对应
 *
 */
struct vco_adc_platform_data {
    u8 mic_mode;
    u8 mic_ldo_vsel;
    u8 mic_ldo_isel;
    u8 mic_bias_en;
    u8 mic_bias_res;
    u8 mic_bias_inside;
    u8 mic_ldo2PAD_en;
    u8 adc_rbs;
    u8 adc_rin;
    u8 adc_test;        //VCO ADC测试使能，该配置用作ADC测试，届时AVAD、DVAD算法将无效
};

/*
 * p11 VAD电源驱动参数和模拟电源域参数配置
 * 先放在adc模拟部分管理
 */
struct vad_power_data {
    u8 ldo_vs;
    u8 ldo_is;
    u8 lowpower;
    u8 clock;
    u8 clock_x2ds_disable;
    u8 acm_select;
};

struct vad_mic_platform_data {
    struct vco_adc_platform_data mic_data;
    struct vad_power_data power_data;
};

/*
*********************************************************************
*                  Audio ADC Initialize
* Description: 初始化Audio_ADC模块的相关数据结构
* Arguments  : adc	ADC模块操作句柄
*			   pd	ADC模块硬件相关配置参数
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_init(struct audio_adc_hdl *adc, struct audio_adc_private_param *private);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 注册adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_add_output_handler(struct audio_adc_hdl *adc, struct audio_adc_output_hdl *output);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 删除adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : 采样通道关闭的时候，对应的回调也要同步删除，防止内存释
*              放出现非法访问情况
*********************************************************************
*/
void audio_adc_del_output_handler(struct audio_adc_hdl *adc, struct audio_adc_output_hdl *output);

/*
*********************************************************************
*                  Audio ADC IRQ Handler
* Description: Audio ADC中断回调函数
* Arguments  : adc  adc模块操作句柄
* Return	 : None.
* Note(s)    : 仅供Audio_ADC中断使用
*********************************************************************
*/
void audio_adc_irq_handler(struct audio_adc_hdl *adc);

/*
*********************************************************************
*                  Audio ADC Mic Open
* Description: 打开mic采样通道
* Arguments  : mic	    mic操作句柄
*			   ch_map	mic通道索引
*			   adc      adc模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_open(struct adc_mic_ch *mic, u32 ch_map, struct audio_adc_hdl *adc, struct mic_open_param *param);

/*
*********************************************************************
*                  Audio ADC Mic Sample Rate
* Description: 设置mic采样率
* Arguments  : mic			mic操作句柄
*			   sample_rate	采样率
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_set_sample_rate(struct adc_mic_ch *mic, int sample_rate);

/*
*********************************************************************
*                  Audio ADC Mic Get Sample Rate
* Description: 获取mic采样率
* Arguments  : None
* Return	 : sample_rate
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_get_sample_rate();

/*
*********************************************************************
*                  Audio ADC Mic Gain
* Description: 设置mic增益
* Arguments  : mic	mic操作句柄
*			   gain	mic增益
* Return	 : 0 成功	其他 失败
* Note(s)    : MIC增益范围：0(-8dB)~19(30dB),step:2dB,level(4)=0dB
*********************************************************************
*/
int audio_adc_mic_set_gain(struct adc_mic_ch *mic, u32 ch_map, int gain);

/*
*********************************************************************
*                  Audio ADC Mic Buffer
* Description: 设置采样buf和采样长度
* Arguments  : mic		mic操作句柄
*			   bufs		采样buf地址
*			   buf_size	采样buf长度，即一次采样中断数据长度
*			   buf_num 	采样buf的数量
* Return	 : 0 成功	其他 失败
* Note(s)    : (1)需要的总buf大小 = buf_size * ch_num * buf_num
* 		       (2)buf_num = 2表示，第一次数据放在buf0，第二次数据放在
*			   buf1,第三次数据放在buf0，依此类推。如果buf_num = 0则表
*              示，每次数据都是放在buf0
*********************************************************************
*/
int audio_adc_mic_set_buffs(struct adc_mic_ch *mic, s16 *bufs, u16 buf_size, u8 buf_num);

/*
*********************************************************************
*                  Audio ADC Mic Start
* Description: 启动audio_adc采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_start(struct adc_mic_ch *mic);

/*
*********************************************************************
*                  Audio ADC Mic Close
* Description: 关闭mic采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_close(struct adc_mic_ch *mic);
int audio_adc_mic1_close(struct adc_mic_ch *mic);

/*
   *********************************************************************
   *                  Audio ADC is Active
   * Description: 判断ADC是否活动
   * Return	 : 0 空闲 1 繁忙
   * Note(s)    : None.
   *********************************************************************
   */
u8 audio_adc_is_active(void);

/*
*********************************************************************
*                  Audio ADC Mic Pre_Gain
* Description: 设置mic第一级/前级增益
* Arguments  : en 前级增益使能(0:6dB 1:0dB)
* Return	 : None.
* Note(s)    : 前级增益只有0dB和6dB两个档位，使能即为0dB，否则为6dB
*********************************************************************
*/
void audio_adc_mic_0dB_en(u32 ch_map, bool en);

/*
*********************************************************************
*                  Audio ADC Mic Gain Boost
* Description: 设置mic第一级/前级增益
* Arguments  : ch_map AUDIO_ADC_MIC_0/AUDIO_ADC_MIC_1/AUDIO_ADC_MIC_2/AUDIO_ADC_MIC_3,多个通道可以或上同时设置
*              level 前级增益档位(AUD_MIC_GB_0dB/AUD_MIC_GB_6dB)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
#define AUD_MIC_GB_0dB      (0)
#define AUD_MIC_GB_6dB      (1)
void audio_adc_mic_gain_boost(u32 ch_map, u8 level);

/*
*********************************************************************
*                  AUDIO MIC_LDO Control
* Description: mic电源mic_ldo控制接口
* Arguments  : index	ldo索引
* 			   en		使能控制
*			   pd		audio_adc模块配置
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)MIC_LDO输出不经过上拉电阻分压
*				  MIC_BIAS输出经过上拉电阻分压
                  AUDIO_MIC_BIAS_CH0            BIT(0)    // PA2
                  AUDIO_MIC_BIAS_CH1            BIT(1)    // PB7
                  AUDIO_MIC_BIAS_CH2            BIT(2)    // 不支持
                  AUDIO_MIC_BIAS_CH3            BIT(3)    // 不支持
                  AUDIO_MIC_LDO_PWR             BIT(4)    // PA0
*			   (2)打开一个mic_ldo示例：
*				audio_mic_ldo_en(AUDIO_MIC_LDO_PWR, 1, 0);
*			   (2)打开多个mic_ldo示例：
*				audio_mic_ldo_en(AUDIO_MIC_LDO_PWR | AUDIO_MIC_BIAS_CH0, 1, 4);
*********************************************************************
*/
int audio_mic_ldo_en(u8 index_map, u8 en, u8 mic_bias_rsel);

/*
*********************************************************************
*                  Audio MIC Mute
* Description: mic静音使能控制
* Arguments  : mute 静音使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_set_mic_mute(bool mute);
void audio_set_mic1_mute(bool mute);



///////////////////////////////////////// linein adc /////////////////////////////////////////////////

/*
*********************************************************************
*                  Audio ADC Linein Open
* Description: 打开linein采样通道
* Arguments  : lienin	linein采样通道句柄
*			   ch_map   linein通道索引
*			            LADC_CH_LINEIN_L:只操作左声道
*			            LADC_CH_LINEIN_R:只操作右声道
*			            LADC_CH_LINEIN_L|LADC_CH_LINEIN_R:左右声道同时操作
*			   adc  adc模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_open(struct adc_linein_ch *linein, u32 ch_map, struct audio_adc_hdl *adc, struct linein_open_param *param);

/*
*********************************************************************
*                  Audio ADC Linein Gain
* Description: 设置linein增益
* Arguments  : linein	linein操作句柄
*			   gain	    linein增益
* Return	 : 0 成功	其他 失败
* Note(s)    : MIC增益范围：0(-8dB)~19(30dB),step:2dB level(4) = 0dB
*********************************************************************
*/
int audio_adc_linein_set_gain(struct adc_linein_ch *linein, u32 ch_map, int gain);

/*
*********************************************************************
*                  Audio ADC Linein Gain Boost
* Description: 设置linein第一级/前级增益
* Arguments  : ch_map AUDIO_ADC_LINE0/AUDIO_ADC_LINE1/AUDIO_ADC_LINE2/AUDIO_ADC_LINE3,多个通道可以或上同时设置
*              level 前级增益档位(AUD_LINEIN_GB_0dB/AUD_LINEIN_GB_6dB)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
#define AUD_LINEIN_GB_0dB      (0)
#define AUD_LINEIN_GB_6dB      (1)
void audio_adc_linein_gain_boost(u32 ch_map, u8 level);

/*
*********************************************************************
*                  Audio ADC Linein Pre_Gain
* Description: 设置mic第一级/前级增益
* Arguments  : linein	linein操作句柄
*			   ch_map   linein通道索引
*			            LADC_CH_LINEIN_L:只操作左声道
*			            LADC_CH_LINEIN_R:只操作右声道
*			            LADC_CH_LINEIN_L|LADC_CH_LINEIN_R:左右声道同时操作
*              en 前级增益使能(0:6dB 1:0dB)
* Return	 : None.
* Note(s)    : 前级增益只有0dB和6dB两个档位，使能即为0dB，否则为6dB
*********************************************************************
*/
void audio_adc_linein_0dB_en(u32 ch_map, bool en);

/*
*********************************************************************
*                  Audio MIC Mute
* Description: mic静音使能控制
* Arguments  : linein	linein操作句柄
*			   ch_map   linein通道索引
*			            LADC_CH_LINEIN_L:只操作左声道
*			            LADC_CH_LINEIN_R:只操作右声道
*			            LADC_CH_LINEIN_L|LADC_CH_LINEIN_R:左右声道同时操作
*              mute 静音使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_lienin_set_mute(struct adc_linein_ch *linein, u32 ch_map, bool mute);

/*
*********************************************************************
*                  Audio ADC Linein Sample Rate
* Description: 设置linein采样率
* Arguments  : linein		linein操作句柄
*			   sample_rate	采样率
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_set_sample_rate(struct adc_linein_ch *linein, int sample_rate);

/*
*********************************************************************
*                  Audio ADC Linein Buffer
* Description: 设置采样buf和采样长度
* Arguments  : linein	linein操作句柄
*			   bufs		采样buf地址
*			   buf_size	采样buf长度，即一次采样中断数据长度
*			   buf_num 	采样buf的数量
* Return	 : 0 成功	其他 失败
* Note(s)    : (1)需要的总buf大小 = buf_size * ch_num * buf_num
* 		       (2)buf_num = 2表示，第一次数据放在buf0，第二次数据放在
*			   buf1,第三次数据放在buf0，依此类推。如果buf_num = 0则表
*              示，每次数据都是放在buf0
*********************************************************************
*/
int audio_adc_linein_set_buffs(struct adc_linein_ch *linein, s16 *bufs, u16 buf_size, u8 buf_num);

/*
*********************************************************************
*                  Audio ADC Linein Output Callback
* Description: 注册adc采样输出回调函数
* Arguments  : linein	linein操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_linein_add_output_handler(struct adc_linein_ch *linein, struct audio_adc_output_hdl *output);

/*
*********************************************************************
*                  Audio ADC Linein Output Callback
* Description: 删除adc采样输出回调函数
* Arguments  : linein	linein操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : 采样通道关闭的时候，对应的回调也要同步删除，防止内存释
*              放出现非法访问情况
*********************************************************************
*/
void audio_adc_linein_del_output_handler(struct adc_linein_ch *linein, struct audio_adc_output_hdl *output);

/*
*********************************************************************
*                  Audio ADC Linein Start
* Description: 启动audio_adc采样
* Arguments  : linein	linein操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_start(struct adc_linein_ch *linein);

/*
*********************************************************************
*                  Audio ADC Linein Close
* Description: 关闭linein采样
* Arguments  : linein	linein操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_linein_close(struct adc_linein_ch *linein);

int audio_adc_mic_type(u8 mic_idx);


void audio_anc_mic_gain(audio_adc_mic_mana_t *mic_param, u8 mult_gain_en);

void audio_anc_mic_open(audio_adc_mic_mana_t *mic_param, u8 trans_en, u8 adc_ch);

void audio_anc_mic_close(audio_adc_mic_mana_t *mic_param);

void audio_adc_add_ch(struct audio_adc_hdl *adc, u8 amic_seq);

int get_adc_seq(struct audio_adc_hdl *adc, u16 ch_map);

int audio_adc_set_buf_fix(u8 fix_en, struct audio_adc_hdl *adc);

void audio_adc_mic_ch_analog_close(struct audio_adc_hdl *adc, u32 ch);

#endif/*AUDIO_ADC_H*/

