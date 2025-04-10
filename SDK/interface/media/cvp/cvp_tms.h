#ifndef _COMMPROC_DMS_H_
#define _COMMPROC_DMS_H_

#include "generic/typedef.h"
#include "cvp_common.h"

//tms_cfg:
typedef struct {
    u8 ver;						//Ver:01
    u8 mic_again;			//MIC增益,default:3(0~14)
    u8 fb_mic_again;			//FB MIC增益,default:3(0~14)
    u8 dac_again;			//DAC增益,default:22(0~31)
    u8 enable_module;       //使能模块
    u8 ul_eq_en;        	//上行EQ使能,default:enable(disable(0), enable(1))
    u8 agc_type;    //AGC类型选择，0：AGC_EXTERNAL，1：AGC_INTERNAL
    union {
        /*AGC*/
        struct {
            float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
            float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
            float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
            float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
            float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(-90 ~ 40 dB)
            float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-90 ~ 40 dB)
            float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
            float dt_max_gain;   	//双端讲话放大上限,default: 12.f(-90 ~ 40 dB)
            float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-90 ~ 40 dB)
            float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -30 dB)
            float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -30 dB)
        } agc_ext;

        /*JLSP AGC*/
        struct {
            int min_mag_db_level;//语音能量放大下限阈值，范围：-90 ~ -35 db，默认-50，单位：dB
            int max_mag_db_level;//语音能量放大上限阈值，范围：-90 ~ 0 dB，默认-3 ，单位dB
            int addition_mag_db_level;//语音补偿能量值，范围：0 ~ 20 dB，默认0 ，单位dB
            int clip_mag_db_level;//语音最大截断能量值，范围：-10 ~ 0 dB，默认-3，单位dB
            int floor_mag_db_level;//语音最小截断能量值，范围：-90 ~ -35 dB，默认-70，单位dB
        } agc_int;
    } agc;
    /*aec*/
    int aec_process_maxfrequency;	//default:8000,range[3000:8000]
    int aec_process_minfrequency;	//default:0,range[0:1000]
    int af_length;					//default:128 range[128:256]
    /*nlp*/
    int nlp_process_maxfrequency;	//default:8000,range[3000:8000]
    int nlp_process_minfrequency;	//default:0,range[0:1000]
    float overdrive;				//default:1,range[0:30]
    /*dns*/
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75dB,range[-100:-30]
    /*enc*/
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    int sir_maxfreq;				//default:3000,range[1000:8000]
    float mic_distance;				//default:0.015,range[0.035:0.015]
    float target_signal_degradation;//default:1,range[0:1]
    float enc_aggressfactor;		//default:4.f,range[0:4]
    float enc_minsuppress;			//default:0.09f,range[0:0.1]
    float Tri_SnrThreshold0;  //sir设定阈值
    float Tri_SnrThreshold1;  //sir设定阈值
    float Tri_CompenDb; //mic增益补偿, dB

    /*wn*/
    float wn_msc_th;    //双麦非相关性阈值，范围：0 ~ 1，默认值：0.6
    float ms_th;        //麦增益能量阈值，范围：0 ~ 255 dB，默认值：80，单位：dB
    float wn_gain_offset;//风噪能量增益偏移阈值，默认值：0，单位dB
    /*回采*/
    u16 adc_ref_en;    /*adc回采参考数据使能*/
    /*debug*/
    u8 output_sel;/*dms output选择: 0:Default, 1:Master Raw, 2:Slave Raw, 3:Fbmic Raw*/
    /*MFDT Parameters*/
    float detect_time;           // // 检测时间s，影响状态切换的速度
    float detect_eng_diff_thr;   // 0~-90 dB 两个mic能量差异持续大于此阈值超过检测时间则会检测为故障
    float detect_eng_lowerbound; // 0~-90 dB 当处于故障状态时，正常的mic能量大于此阈值才会检测能量差异，避免安静环境下误判切回正常状态
    int MalfuncDet_MaxFrequency;// 检测信号的最大频率成分
    int MalfuncDet_MinFrequency;// 检测信号的最小频率成分
    int OnlyDetect;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换

} _GNU_PACKED_ AEC_TMS_CONFIG;

struct tms_attr {
    u8 ul_eq_en: 1;
    u8 wideband: 1;
    u8 wn_en: 1;
    u8 dly_est : 1;
    u8 aptfilt_only: 1;
    u8 reserved: 3;

    u8 dst_delay;/*延时估计目标延时*/
    u8 EnableBit;
    u8 FB_EnableBit;
    u8 packet_dump;
    u8 SimplexTail;
    u8 output_sel;/*dms output选择*/
    u16 hw_delay_offset;/*dac hardware delay offset*/
    u16 wn_gain;/*white_noise gain*/
    u8 agc_type; /*agc类型*/
    /*AGC*/
    float AGC_NDT_fade_in_step; 	//in dB
    float AGC_NDT_fade_out_step; 	//in dB
    float AGC_NDT_max_gain;     	//in dB
    float AGC_NDT_min_gain;     	//in dB
    float AGC_NDT_speech_thr;   	//in dB
    float AGC_DT_fade_in_step;  	//in dB
    float AGC_DT_fade_out_step; 	//in dB
    float AGC_DT_max_gain;      	//in dB
    float AGC_DT_min_gain;      	//in dB
    float AGC_DT_speech_thr;    	//in dB
    float AGC_echo_present_thr; 	//In dB
    int AGC_echo_look_ahead;    	//in ms
    int AGC_echo_hold;          	// in ms
    /*jlsp agc*/
    int min_mag_db_level;
    int max_mag_db_level;
    int addition_mag_db_level;
    int clip_mag_db_level;
    int floor_mag_db_level;
    /*AEC*/
    int aec_process_maxfrequency;	//default:8000,range[3000:8000]
    int aec_process_minfrequency;	//default:0,range[0:1000]
    int af_length;					//default:128 range[128:256]
    /*NLP*/
    int nlp_process_maxfrequency;	//default:8000,range[3000:8000]
    int nlp_process_minfrequency;	//default:0,range[0:1000]
    float overdrive;				//default:1,range[0:30]
    /*ANS*/
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75db,range[-100:-30]
    /*DNS Parameters*/
    float DNS_highGain;     //EQ强度   range[1.0:3.5]
    float DNS_rbRate;       //混响强度 range[0:0.9];
    int enhance_flag;       //是否高频增强 range[0,1]
    /*ENC*/
    int Tri_CutTh;	//fb麦统计截止频率
    float Tri_SnrThreshold0;  //sir设定阈值
    float Tri_SnrThreshold1;  //sir设定阈值
    float *Tri_TransferFunc; //fb -> main传递函数
    float Tri_FbAlignedDb;	//fb和主副麦之间需要补偿增益
    float Tri_FbCompenDb; //fb补偿增益
    int Tri_TfEqSel; //eq,传递函数的选择：0选择eq，1选择传递函数，2选择传递函数和eq
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    int sir_maxfreq;				//default:3000,range[1000:8000]
    float mic_distance;				//default:0.015,range[0.035:0.015]
    float target_signal_degradation;//default:1,range[0:1]
    float enc_aggressfactor;		//default:4.f,range[0:4]
    float enc_minsuppress;			//default:0.09f,range[0:0.1]
    float Tri_CompenDb; //mic增益补偿, dB
    int Tri_Bf_Enhance; //bf是否高频增强
    int Tri_LowFreqHoldEn;
    float Tri_SupressFactor;
    /*wn*/
    float wn_msc_th;    //双麦非相关性阈值,default:0.6f(0 ~ 1)
    float ms_th;        //麦增益能量阈值, default:80.f(0-255)dB
    int wn_gain_offset; //风噪能量增益偏移阈值
    /*common*/
    float global_minsuppress;		//default:0.0,range[0.0:0.09]
    /*data handle*/
    int (*cvp_advanced_options)(void *aec,
                                void *nlp,
                                void *ns,
                                void *enc,
                                void *agc,
                                void *wn,
                                void *mfdt);
    int (*aec_probe)(short *mic0, short *mic1, short *mic2, short *ref, u16 len);
    int (*aec_post)(s16 *dat, u16 len);
    int (*aec_update)(u8 EnableBit);
    int (*output_handle)(s16 *dat, u16 len);

    /*Extended-Parameters*/
    u32 ref_sr;
    u16 ref_channel;    /*参考数据声道数*/
    u16 adc_ref_en;    /*adc回采参考数据使能*/
    u8 mic_bit_width; /*麦克风数据位宽*/
    u8 ref_bit_width; /*参考数据位宽*/

    /*MFDT Parameters*/
    float detect_time;           // // 检测时间s，影响状态切换的速度
    float detect_eng_diff_thr;   // 0~-90 dB 两个mic能量差异持续大于此阈值超过检测时间则会检测为故障
    float detect_eng_lowerbound; // 0~-90 dB 当处于故障状态时，正常的mic能量大于此阈值才会检测能量差异，避免安静环境下误判切回正常状态
    int MalfuncDet_MaxFrequency;// 检测信号的最大频率成分
    int MalfuncDet_MinFrequency;// 检测信号的最小频率成分
    int OnlyDetect;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换
};

s32 aec_tms_init(struct tms_attr *attr);
s32 aec_tms_exit();
s32 aec_tms_fill_in_data(void *dat, u16 len);
int aec_tms_fill_in_ref_data(void *dat, u16 len);
int aec_tms_fill_in_ref_1_data(void *dat, u16 len);
int aec_tms_fill_ref_data(void *data0, void *data1, u16 len);
void aec_tms_toggle(u8 toggle);
int aec_tms_cfg_update(AEC_TMS_CONFIG *cfg);
int aec_tms_reboot(u8 enablebit);
u8 get_cvp_tms_rebooting();

int cvp_tms_read_ref_data(void);

/*获取风噪的检测结果，1：有风噪，0：无风噪*/
int cvp_tms_get_wind_detect_state(void);

/*
 * 获取风噪检测信息
 * wd_flag: 0 没有风噪，1 有风噪
 * 风噪强度r: 0~BIT(16)
 * wd_lev: 风噪等级，0：弱风，1：中风，2：强风
 * */
int jlsp_tms_get_wind_detect_info(int *wd_flag, int *wd_val, int *wd_lev);

enum cvp_tms_mode {
    TMS_MODE_2MIC = 0,  //跑双麦降噪，不使用FB MIC数据
    TMS_MODE_3MIC,      //跑3MIC降噪
};
/* 设置跑2mic/3mic降噪
 * TMS_MODE_2MIC  跑双麦降噪，不使用FB MIC数据
 * TMS_MODE_3MIC  跑3MIC降噪
 * Notes : CONST_JLSP_3MIC_MODE != JLSP_3MIC_MODE0时设置才有效*/
int jlsp_tms_mode_choose(enum cvp_tms_mode mode);

/*单/三麦切换状态
 * 0: 正常三麦 ;
 * 1: 副麦坏了，触发故障
 * -1: 主麦坏了，触发故障
 */
int cvp_tms_get_malfunc_state(void);

/*
 * 获取mic的能量值，开了MFDT_EN才能用
 * mic: 0 获取主麦能量
 * mic：1 获取副麦能量
 * return：返回能量值，[0~90.3],返回-1表示错误
 */
float cvp_tms_get_mic_energy(u8 mic);

#endif/*_COMMPROC_DMS_H_*/
