#ifndef _SD_ANC_LIB_V2_H
#define _SD_ANC_LIB_V2_H

#include "timer.h"
#include "math.h"
#include "asm/math_fast_function.h"
#include "icsd_common_v2.h"
#include "icsd_anc_v2_config.h"

#define DMA_BELONG_TO_SZL    		1
#define DMA_BELONG_TO_SZR    		2
#define DMA_BELONG_TO_PZL    		3
#define DMA_BELONG_TO_PZR    		4
#define DMA_BELONG_TO_TONEL  		5
#define DMA_BELONG_TO_BYPASSL 		6
#define DMA_BELONG_TO_TONER  		7
#define DMA_BELONG_TO_BYPASSR 	    8

#define ICSD_ANC_DSF8_DATA_DEBUG    0
#define DSF8_DATA_DEBUG_TYPE		DMA_BELONG_TO_PZL
#define USE_TONE_SZ    			    0

//提示音训练模式下参数设置
#define BR50_ANC_TONE_TRAIN_OUT_SEL_L	25
#define BR50_ANC_TONE_TRAIN_OUT_SEL_R 	22
#define BR28_ANC_TONE_TRAIN_OUT_SEL_L	4
#define BR28_ANC_TONE_TRAIN_OUT_SEL_R 	2
//非提示音训练模式下参数设置
#define BR50_ANC_USER_TRAIN_OUT_SEL_L   24
#define BR50_ANC_USER_TRAIN_OUT_SEL_R   21
#define BR28_ANC_USER_TRAIN_OUT_SEL_L   7
#define BR28_ANC_USER_TRAIN_OUT_SEL_R   6


extern int anc_v2_printf_off(const char *format, ...);
#define abs(x)  ((x)>0?(x):-(x))
#if 0
#define _anc_v2_printf printf                  //打开自适应ANC参数调试信息
#else
#define _anc_v2_printf anc_v2_printf_off
#endif
#if 0
#define icsd_printf printf
#else
#define icsd_printf(...)
#endif

#define ANC_V2_TRAIN_TIMEOUT_S         5//10//秒
#define TWS_STA_SIBLING_CONNECTED   0x00000002//tws已连接
#define ANC_V2_DMA_DOUBLE_LEN         (512*4) //DMA一次拿多少数据
#define ANC_V2_DOUBLE_TRAIN_LEN        512    //拿够多少数进行一次part1
#define ANC_V2_DMA_DOUBLE_CNT       32 //32
#define ICSD_ANC_RESOURCE_STAT      0

#define HEADSET_TONES_MODE          1
#define TWS_TONE_MODE               2
#define TWS_HALF_INEAR_MODE         3//提示音中间获取pz，但是如果长按进入的话长按放开的速度慢了会导致pz抖动

#define TWS_2CH_TONE_BYPASS_PZ 	    4//BR28,BR50 TWS默认使用
#define HEADSET_2CH_TONE_BYPASS_PZ  5//BR28 HEADSET默认使用
#define HEADSET_4CH_TONE_BYPASS_PZ  6//BR50 HEADSET默认使用

//user_train_state------------------------
#define ANC_USER_TRAIN_DMA_EN       BIT(3)
#define ANC_USER_TRAIN_TONEMODE     BIT(4)
#define ANC_TRAIN_TONE_END			BIT(6)
#define ANC_TRAIN_TONE_FIRST 		BIT(7)
#define ANC_TRAIN_IIR_READY 		BIT(8)
#define ANC_MASTER_RX_PZSYNC 		BIT(9)
#define ANC_USER_TRAIN_RUN 			BIT(14)
#define ANC_TRAIN_TIMEOUT 			BIT(15)
#define ANC_TONE_DAC_ON				BIT(17)
#define ANC_TONE_ANC_READY			BIT(18)
#define ANC_RX_SYNCINF1             BIT(22)
#define ANC_SYNCINF1_DONE           BIT(23)
#define ANC_HALFANC_DONE            BIT(24)
#define ANC_RX_HALFANC_DONE         BIT(25)

//SZPZ->anc_err---------------------------
#define ANC_ERR_LR_BALANCE          BIT(0)//左右耳不平衡
#define ANC_ERR_SOUND_PRESSURE      BIT(1)//低声压
#define ANC_ERR_SHAKE               BIT(2)//抖动
//icsd_anc_function-----------------------
#define ANC_ADAPTIVE_CMP            BIT(0)
#define ANC_EARPHONE_CHECK          BIT(1)

//icsd_anc_contral
#define ICSD_ANC_FORCED_EXIT        BIT(0)
#define ICSD_ANC_SUSPENDED          BIT(1)
#define ICSD_ANC_TONE_END           BIT(3)
#define ICSD_ANC_INITED             BIT(4)
#define ICSD_ANC_FORCED_BEFORE_INIT BIT(5)
#define ICSD_ANC_PART1_END          BIT(6)

#define ANC_MAX_ORDER               12


typedef struct {
    float tool_target[TARLEN2 + TARLEN2_L];
    float ff_fgq[ANC_MAX_ORDER * 3 + 1];
    float fb_fgq[1 * 3 + 1];
    float cmp_fgq[ANC_MAX_ORDER * 3 + 1];
} __anc_tool;

struct icsd_anc_v2_tws_packet {
    s8  *data;
    u16 len;
};


struct icsd_iir_ab_inf {
    double *ff_iir_ab;
    double *fb_iir_ab;
    double *music_iir_ab;
    float ff_iir_gain;
    float fb_iir_gain;
    float music_iir_gain;
};
extern struct icsd_iir_ab_inf *IIR_INF_L_V2;
extern struct icsd_iir_ab_inf *IIR_INF_R_V2;

struct icsd_anc_v2_libfmt {
    int lib_alloc_size; //算法ram需求大小
    int anc_dma_sample;
};

struct icsd_anc_v2_infmt {
    void *alloc_ptr;    //外部申请的ram地址
    float ff_gain;
    float fb_gain;
    s8 tool_ffgain_sign;
    s8 tool_fbgain_sign;
};

// anc tool buffer
typedef struct {
    // target配置
    int   cmp_en;
    int   target_cmp_num;

    float  pz_gain;
    float *target_sv;
    float *target_cmp_dat;

    // 算法配置
    u8    biquad_type[10];
    u8    fb_biquad_type[10];
    u8    cmp_biquad_type[10];
    float Vrange_H[62];
    float Vrange_M[62];
    float Vrange_L[62];
    float Biquad_init_H[31];
    float Biquad_init_M[31];
    float Biquad_init_L[31];
    float degree_set0[7];
    float degree_set1[7];
    float degree_set2[7];

    float *Weight_H;
    float *Weight_M;
    float *Weight_L;
    float *Gold_csv_H;
    float *Gold_csv_M;
    float *Gold_csv_L;

    float total_gain_adj_begin;
    float total_gain_adj_end;
    float gain_limit_all;
    int   IIR_NUM_FLEX;
    int   IIR_NUM_FIX;

    // 耳道记忆曲线配置
    u8    mem_curve_nums;
    float *pz_table;
    float *sz_table;
} adpt_anc_cfg;

typedef struct {
    //工具界面
    u16	  tonel_delay;
    u16	  toner_delay;
    u16	  pzl_delay;
    u16	  pzr_delay;
    u8    train_mode;

    s8	  sz_calr_sign;
    s8	  pz_calr_sign;
    s8    bypass_calr_sign;
    s8    perf_calr_sign;

    float bypass_vol;
    float sz_pri_thr;
    s8	  vld1;
    s8    vld2;
    u8    ear_recorder;
    u8    pnc_times;
    //其他配置
    s8 	  tool_ffgain_sign;
    s8 	  tool_fbgain_sign;
    u8 	  ff_yorder;
    u8    fb_yorder;
    u8    cmp_yorder;
    u8    normal_out_sel_l;
    u8    normal_out_sel_r;
    u8    tone_out_sel_l;
    u8    tone_out_sel_r;
    u8    fb_agc_en;

    adpt_anc_cfg adpt_cfg;
    adpt_anc_cfg adpt_cfg_r;
} __icsd_anc_config_data;
extern __icsd_anc_config_data	*SD_CFG;

enum {
    ANC_V2_CMD_DAC_ON = 0,
    ANC_V2_CMD_TONE_END,
    ANC_V2_CMD_TONEL_START,
    ANC_V2_CMD_TONER_START,
    ANC_V2_CMD_PZL_START,
    ANC_V2_CMD_PZR_START,
    ANC_V2_CMD_GET_DMA_DATA,
    ANC_V2_CMD_FORCED_EXIT,
    ANC_V2_CMD_USER_TRAIN_END,
    ANC_V2_CMD_USER_TRAIN_TIMEOUT,
    ANC_V2_CMD_SETPARAM,
    ANC_V2_CMD_SYNCINF2_FUNCTION,
    ANC_V2_CMD_HALF_ANC_RUN,
    ANC_V2_CMD_HALF_ANC_ON_PRE,
    ANC_V2_CMD_HALF_TIMEOUT,
    ANC_V2_CMD_MASTER_ANCON_SYNC,
};

enum {
    ANC_V2_OUTPUT_NORMAL = 0,
    ANC_V2_OUTPUT_TIMEOUT,
    ANC_V2_OUTPUT_FORCED,
};

typedef struct {
    s16 TONE_L_CH0[1024];
    s16 TONE_L_CH1[1024];
    s16 BYPASS_L_CH0[1024];
    s16 BYPASS_L_CH1[1024];
    s16 PNC_L_CH0[1024];
    s16 PNC_L_CH1[1024];
    s16 TONE_R_CH0[1024];
    s16 TONE_R_CH1[1024];
    s16 BYPASS_R_CH0[1024];
    s16 BYPASS_R_CH1[1024];
    s16 PNC_R_CH0[1024];
    s16 PNC_R_CH1[1024];
} __icsd_time_data;

struct anc_function {
    //sys
    void (*local_irq_disable)();
    void (*local_irq_enable)();
    u16(*icsd_anc_timeout_add)(void *priv, void (*func)(void *priv), u32 msec);
    void (*icsd_anc_v2_cmd)(u8 cmd);
    int (*jiffies_usec2offset)(unsigned long begin, unsigned long end);
    unsigned long (*jiffies_usec)(void);
    //tws
    int (*tws_api_get_role)(void);
    int (*icsd_anc_v2_get_tws_state)();
    void (*icsd_anc_v2_tws_m2s)(u8 cmd);
    void (*icsd_anc_v2_tws_s2m)(u8 cmd);
    void (*icsd_anc_v2_tws_msync)(u8 cmd);
    void (*icsd_anc_v2_tws_ssync)(u8 cmd);
    //anc
    u8(*anc_dma_done_ppflag)();
    void (*icsd_anc_v2_dma_2ch_on)(u8 out_sel, int *buf, int len);
    void (*icsd_anc_v2_dma_4ch_on)(u8 out_sel_ch01, u8 out_sel_ch23, int *buf, int len);


    void (*icsd_anc_v2_output)(struct icsd_anc_v2_tool_data *_TOOL_DATA, u8 OUTPUT_STATE);
    void (*icsd_anc_v2_end)(void *_param);
};
extern struct anc_function	ANC_FUNC;

typedef struct {
    u8 icsd_anc_v2_tws_balance_en;
    u8 icsd_anc_v2_contral;
    u8 icsd_anc_v2_function;
    u16 time_data_wptr;
    u32 icsd_sample_rate;
    u32 icsd_slience_frames;
    u32 train_time_v2;
    int icsd_anc_v2_id;
    int v2_user_train_state;
    void *anc_v2_ram_addr;
    __icsd_time_data *anc_time_data;
} __icsd_anc_REG;
extern __icsd_anc_REG *ICSD_REG;

extern const u8 ICSD_ANC_V2_MODE;
extern const u8 anc_dma_ch_num;
extern int (*anc_v2_printf)(const char *format, ...);
extern struct icsd_anc_v2_tool_data *TOOL_DATA;

//SDK 调用的lib函数
void icsd_anc_v2_get_libfmt(struct icsd_anc_v2_libfmt *libfmt);
void icsd_anc_v2_set_infmt(struct icsd_anc_v2_infmt *fmt);
void icsd_anc_v2_part1_run(u32 slience_frames, u32 sample_rate, void (*end_cb)(void *priv));
void icsd_anc_v2_part2_run();
void icsd_anc_v2_dma_handler();
void icsd_anc_v2_cmd_handler(u8 cmd, void *_param);

void icsd_anc_v2_m2s_packet(s8 *data, u8 cmd);
void icsd_anc_v2_s2m_packet(s8 *data, u8 cmd);
void icsd_anc_v2_msync_packet(struct icsd_anc_v2_tws_packet *packet, u8 cmd);
void icsd_anc_v2_ssync_packet(struct icsd_anc_v2_tws_packet *packet, u8 cmd);
void icsd_anc_v2_m2s_cb(void *_data, u16 len, bool rx);
void icsd_anc_v2_s2m_cb(void *_data, u16 len, bool rx);
void icsd_anc_v2_msync_cb(void *_data, u16 len, bool rx);
void icsd_anc_v2_ssync_cb(void *_data, u16 len, bool rx);
void icsd_anc_v2_mode_init();

//DEBUG 函数
void icsd_anc_v2_time_data_debug();



#endif/*_SD_ANC_LIB_V2_H*/
