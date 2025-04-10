#ifndef _ANC_EXT_TOOL_H_
#define _ANC_EXT_TOOL_H_

#include "typedef.h"
#include "generic/list.h"

//自适应类型
#define ANC_ADAPTIVE_TYPE_TWS            	1 //显示: TWS
#define ANC_ADAPTIVE_TYPE_HEADSET        	2 //显示：HEADSET

//自适应训练模式
#define ANC_TRAIN_MODE_TONE_PNC          	1 //显示: TONE_PNC
#define ANC_TRAIN_MODE_TONE_BYPASS_PNC		2 //显示：TONE_BYPASS_PNC

//ANC_EXT功能列表
#define ANC_EXT_FUNC_EN_ADAPTIVE			BIT(0)	//自适应功能

//文件 SUBFILE ID
enum {
    /* FILE_ID_ANC_EXT_START = 0XB0, */
    FILE_ID_ANC_EXT_EAR_ADAPTIVE_BASE = 0XB0,		//BASE 界面参数 文件ID
    FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_IIR = 0XB1,		//FF->滤波器设置文件ID
    FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_WEIGHT = 0XB2,	//FF->权重和性能设置文件ID
    FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_TARGET = 0XB3,	//FF->TARGET和补偿值设置文件ID

    FILE_ID_ANC_EXT_BIN = 0xB4,						//anc_ext.bin 总文件ID

    FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_RECORDER = 0xB5,//FF->耳道记忆 文件ID
    FILE_ID_ANC_EXT_EAR_ADAPTIVE_TIME_DOMAIN = 0xB6,//时域文件ID 文件ID

    FILE_ID_ANC_EXT_EAR_ADAPTIVE_DUT_CMP = 0xB7,		//耳道自适应产测补偿 文件ID

    /* FILE_ID_ANC_EXT_STOP = 0XD0, */
};

//数据类型 ID
enum {
    //BASE 界面参数 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_BASE
    ANC_EXT_EAR_ADAPTIVE_BASE_CFG_ID = 0x1,				//base 界面下的参数

    //======================左声道参数ID========================
    //FF->滤波器设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_IIR_FF
    ANC_EXT_EAR_ADAPTIVE_FF_GENERAL_ID = 0x11,			//通用设置参数

    ANC_EXT_EAR_ADAPTIVE_FF_HIGH_GAINS_ID = 0x12,		//高->gain相关参数配置
    ANC_EXT_EAR_ADAPTIVE_FF_HIGH_IIR_ID = 0x13,			//高->滤波器及滤波器限制配置

    ANC_EXT_EAR_ADAPTIVE_FF_MEDIUM_GAINS_ID = 0x14,		//中->gain相关参数配置
    ANC_EXT_EAR_ADAPTIVE_FF_MEDIUM_IIR_ID = 0x15,		//中->滤波器及滤波器限制配置

    ANC_EXT_EAR_ADAPTIVE_FF_LOW_GAINS_ID = 0x16,		//低->gain相关参数配置
    ANC_EXT_EAR_ADAPTIVE_FF_LOW_IIR_ID = 0x17,			//低->滤波器及滤波器限制配置

    //FF->权重和性能设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_WEIGHT
    ANC_EXT_EAR_ADAPTIVE_FF_WEIGHT_PARAM_ID = 0x18,		//权重通用参数配置

    ANC_EXT_EAR_ADAPTIVE_FF_HIGH_WEIGHT_ID = 0x19,		//高->权重配置
    ANC_EXT_EAR_ADAPTIVE_FF_MEDIUM_WEIGHT_ID = 0x1A,	//中->权重配置
    ANC_EXT_EAR_ADAPTIVE_FF_LOW_WEIGHT_ID = 0x1B,		//低->权重配置

    ANC_EXT_EAR_ADAPTIVE_FF_HIGH_MSE_ID = 0x1C,			//高->性能配置
    ANC_EXT_EAR_ADAPTIVE_FF_MEDIUM_MSE_ID = 0x1D,		//中->性能配置
    ANC_EXT_EAR_ADAPTIVE_FF_LOW_MSE_ID = 0x1E,			//低->性能配置

    //FF->TARGE挡位设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_TARGET
    ANC_EXT_EAR_ADAPTIVE_FF_TARGET_PARAM_ID = 0x1F,		//TARGET相关参数配置
    ANC_EXT_EAR_ADAPTIVE_FF_TARGET_SV_ID = 0x20,		//TARGET配置
    ANC_EXT_EAR_ADAPTIVE_FF_TARGET_CMP_ID = 0x21,		//TARGET 补偿值配置

    //FF->耳道记忆设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_RECORDER
    ANC_EXT_EAR_ADAPTIVE_FF_RECORDER_PARAM_ID = 0x22,	//耳道记忆相关参数配置
    ANC_EXT_EAR_ADAPTIVE_FF_RECORDER_PZ_ID = 0x23,			//耳道记忆PZ配置
    ANC_EXT_EAR_ADAPTIVE_FF_RECORDER_SZ_ID = 0x3D,			//耳道记忆SZ配置

    //时域文件ID 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_TIME_DOMAIN
    // ANC_EXT_TIME_DOMAIN_CH_TONE_SZ_L = 0x24,		//tone_sz_L
    // ANC_EXT_TIME_DOMAIN_CH_BYPASS_SZ_L = 0x25,		//bypass_sz_L
    // ANC_EXT_TIME_DOMAIN_CH_PNC_L = 0x26,			//pnc_sz_L
    // ANC_EXT_TIME_DOMAIN_CH_TONE_SZ_R = 0x27,		//tone_sz_R
    // ANC_EXT_TIME_DOMAIN_CH_BYPASS_SZ_R = 0x28,		//bypass_sz_R
    // ANC_EXT_TIME_DOMAIN_CH_PNC_R = 0x29,			//pnc_sz_R

    //======================右声道参数ID========================
    //FF->滤波器设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_IIR_FF
    ANC_EXT_EAR_ADAPTIVE_FF_R_GENERAL_ID = 0x2A,		//通用设置参数(右)

    ANC_EXT_EAR_ADAPTIVE_FF_R_HIGH_GAINS_ID = 0x2B,		//高->gain相关参数配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_HIGH_IIR_ID = 0x2C,		//高->滤波器及滤波器限制配置(右)

    ANC_EXT_EAR_ADAPTIVE_FF_R_MEDIUM_GAINS_ID = 0x2D,	//中->gain相关参数配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_MEDIUM_IIR_ID = 0x2E,		//中->滤波器及滤波器限制配置(右)

    ANC_EXT_EAR_ADAPTIVE_FF_R_LOW_GAINS_ID = 0x2F,		//低->gain相关参数配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_LOW_IIR_ID = 0x30,		//低->滤波器及滤波器限制配置(右)

    //FF->权重和性能设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_WEIGHT
    ANC_EXT_EAR_ADAPTIVE_FF_R_WEIGHT_PARAM_ID = 0x31,	//权重通用参数配置(右)

    ANC_EXT_EAR_ADAPTIVE_FF_R_HIGH_WEIGHT_ID = 0x32,	//高->权重配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_MEDIUM_WEIGHT_ID = 0x33,	//中->权重配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_LOW_WEIGHT_ID = 0x34,		//低->权重配置(右)

    ANC_EXT_EAR_ADAPTIVE_FF_R_HIGH_MSE_ID = 0x35,		//高->性能配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_MEDIUM_MSE_ID = 0x36,		//中->性能配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_LOW_MSE_ID = 0x37,		//低->性能配置(右)

    //FF->TARGE挡位设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_TARGET
    ANC_EXT_EAR_ADAPTIVE_FF_R_TARGET_PARAM_ID = 0x38,	//TARGET相关参数配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_TARGET_SV_ID = 0x39,		//TARGET配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_TARGET_CMP_ID = 0x3A,		//TARGET 补偿值配置(右)

    //FF->耳道记忆设置界面 文件ID FILE_ID_ANC_EXT_EAR_ADAPTIVE_FF_RECORDER
    ANC_EXT_EAR_ADAPTIVE_FF_R_RECORDER_PARAM_ID = 0x3B,	//耳道记忆相关参数配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_RECORDER_PZ_ID = 0x3C,	//耳道记忆PZ配置(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_RECORDER_SZ_ID = 0x3E,	//耳道记忆SZ配置(右)

    //FF->耳道自适应产测数据
    ANC_EXT_EAR_ADAPTIVE_FF_DUT_PZ_CMP_ID = 0x3F,			//产测PZ补偿
    ANC_EXT_EAR_ADAPTIVE_FF_DUT_SZ_CMP_ID = 0x40,			//产测SZ补偿
    ANC_EXT_EAR_ADAPTIVE_FF_R_DUT_PZ_CMP_ID = 0x41,			//产测PZ补偿(右)
    ANC_EXT_EAR_ADAPTIVE_FF_R_DUT_SZ_CMP_ID = 0x42,			//产测SZ补偿(右)
};

//自适应训练模式
enum ANC_EAR_ADPTIVE_MODE {
    EAR_ADAPTIVE_MODE_NORAML = 0,	//正常模式
    EAR_ADAPTIVE_MODE_TEST,			//工具测试模式
    EAR_ADAPTIVE_MODE_SIGN_TRIM,	//工具自适应符号校准模式
};

//自适应失败 滤波器来源
enum ANC_EAR_ADAPTIVE_RESULT_MODE {
    EAR_ADAPTIVE_FAIL_RESULT_FROM_ADAPTIVE = 0,	//由自适应校准
    EAR_ADAPTIVE_FAIL_RESULT_FROM_NORMAL,		//来源默认滤波器
};

struct anc_ext_alloc_bulk_t {
    struct list_head entry;
    u32 alloc_id;			//申请ID
    u32 alloc_len;			//临时内存大小
    u8 *alloc_addr;			//临时内存地址
};

//subfile id head
struct anc_ext_id_head {
    u32 id;
    u32 offset;
    u32 len;
};

//subfile head
struct anc_ext_subfile_head {
    u32 file_id;
    u32 file_len;
    u16 version;
    u16 id_cnt;
    u8 data[0];
};

struct __anc_ext_subfile_id_table {
    u8 pointer_type;	//是否为指针类型
    int id;				//参数对应的ID
    u32 offset;			//目标存储地址相对偏移量
};

//耳道自适应 - BASE界面参数
struct __anc_ext_ear_adaptive_base_cfg {
    s8 calr_sign[4];	//校准符号（SZ/PZ/bypass/performance）

    u8 sign_trim_en;	//符号使能校准，校准过则为1
    s8 vld1;			//入耳检测阈值1				default:-10    	range:-128~127 
    s8 vld2;			//入耳检测阈值2   			default:-9 		range:-128~127 
    u8 adaptive_mode;	//自适应结果模式  			default:0 		range:0~1 

    u8 adaptive_times;	//自适应训练次数   			default:1 		range:1~10 
    u8 reserved[3];		//保留位

    u16 tonel_delay;	//提示应延时n(ms)获取szl	default:50   	range:0~10000
    u16 toner_delay;	//提示应延时n(ms)获取szr	default:2400  	range:0~10000
    u16 pzl_delay;		//提示应延时n(ms)获取pzl	default:900    	range:0~10000
    u16 pzr_delay;		//提示应延时n(ms)获取pzr	default:3200	range:0~10000
    float bypass_vol;	//bypass音量				default:1(0dB) 	range:0~100  
    float sz_pri_thr;	//提示音，bypass优先级阈值	default:60    	range:0~200  
};	//28byte

//IIR滤波器单元
struct __anc_ext_iir {
    float fre;
    float gain;
    float q;
};

//耳道自适应 - 滤波器界面-滤波器参数单元 高/中/低
struct __anc_ext_ear_adaptive_iir {
    u8 type;								//滤波器类型
    u8 fixed_en;							//固定使能
    u8 reserved[2];
    struct __anc_ext_iir def;				//默认滤波器
    struct __anc_ext_iir upper_limit;		//调整上限
    struct __anc_ext_iir lower_limit;		//调整下限
};	//40byte

//耳道自适应 - 滤波器界面通用参数
struct __anc_ext_ear_adaptive_iir_general {
    u8 biquad_level_l;				//滤波器档位划分下限 default:5    range:0~255
    u8 biquad_level_h;				//滤波器档位划分上限 default:9    range:0~255
    u8 reserved[2];
    float total_gain_freq_l;		//总增益对齐频率下限 default:350  range:0~2700
    float total_gain_freq_h;		//总增益对齐频率上限 default:650  range:0~2700
    float total_gain_limit;			//滤波器最高增益限制 default:20   range:-80~80
};	//32byte

//耳道自适应 - 滤波器界面 gain参数 高/中/低
struct __anc_ext_ear_adaptive_iir_gains {
    float iir_target_gain_limit;	//滤波器和target增益差限制 default:0  range:-30~30
    float def_total_gain;			//默认增益  default 1.0(0dB)  range 0.0316(-30dB) - 31.622(+30dB);
    float upper_limit_gain;			//增益调整上限 default 2.0(6dB) range 0.0316(-30dB) - 31.622(+30dB);
    float lower_limit_gain;			//增益调整下限 default 0.5(-6dB) range 0.0316(-30dB) - 31.622(+30dB);
};	//32byte

//耳道自适应 - 权重和性能界面 参数
struct __anc_ext_ear_adaptive_weight_param {
    u8 	mse_level_l;			//低性能档位划分 	default:5    range:0~255
    u8 	mse_level_h;			//高性能档位划分 	default:9    range:0~255
    u8 reserved[2];
}; //4byte

//耳道自适应 - 权重和性能界面 权重曲线
struct __anc_ext_ear_adaptive_weight {
    float data[60];
}; //240byte

//耳道自适应 - 权重和性能界面 性能曲线
struct __anc_ext_ear_adaptive_mse {
    float data[60];
}; //240byte

//耳道自适应 - TARGET界面 参数
struct __anc_ext_ear_adaptive_target_param {
    u8 cmp_en;
    u8 cmp_curve_num;
    u8 reserved[2];
}; //4byte

//耳道自适应 - TARGET界面 sv
struct __anc_ext_ear_adaptive_target_sv {
    float data[7 * 11];
};	//308 byte

//耳道自适应 - TARGET界面 cmp
struct __anc_ext_ear_adaptive_target_cmp {
    float data[2 * 60 * 11];	//BR28 长度为75 * 11
};	//5180 byte

//耳道自适应 - 耳道记忆界面 参数
struct __anc_ext_ear_adaptive_mem_param {
    u8 mem_curve_nums;          // 记忆曲线数目：DLL函数输出结果  default:0  range:0~60
    s8 ear_recorder;			//耳道记忆使能				default:0    	range:0,1
    u8 reserved[2];
};

//耳道自适应 - 耳道记忆界面 pz/sz数据
struct __anc_ext_ear_adaptive_mem_data {
    float data[0];      // DLL函数输出数组，实际大小为25*2*mem_curve_nums
};

//耳道自适应 - 产测数据 pz/sz数据
struct __anc_ext_ear_adaptive_dut_data {
    float data[0];
};

//耳道自适应工具参数
struct anc_ext_ear_adaptive_param {

    enum ANC_EAR_ADPTIVE_MODE train_mode;		//自适应训练模式
    u8 time_domain_show_en;						//时域debug使能
    u8 *time_domain_buf;					//时域debug buff
    int time_domain_len;					//时域debug buff len

    struct __anc_ext_ear_adaptive_base_cfg	*base_cfg;

    /*----------------头戴式 左声道 / TWS LR------------------*/
    struct __anc_ext_ear_adaptive_iir_general *ff_iir_general;
    struct __anc_ext_ear_adaptive_iir_gains	*ff_iir_high_gains;
    struct __anc_ext_ear_adaptive_iir_gains	*ff_iir_medium_gains;
    struct __anc_ext_ear_adaptive_iir_gains	*ff_iir_low_gains;
    struct __anc_ext_ear_adaptive_iir *ff_iir_high;//[10];
    struct __anc_ext_ear_adaptive_iir *ff_iir_medium;//[10];
    struct __anc_ext_ear_adaptive_iir *ff_iir_low;//[10];

    struct __anc_ext_ear_adaptive_weight_param *ff_weight_param;
    struct __anc_ext_ear_adaptive_weight *ff_weight_high;
    struct __anc_ext_ear_adaptive_weight *ff_weight_medium;
    struct __anc_ext_ear_adaptive_weight *ff_weight_low;
    struct __anc_ext_ear_adaptive_mse *ff_mse_high;
    struct __anc_ext_ear_adaptive_mse *ff_mse_medium;
    struct __anc_ext_ear_adaptive_mse *ff_mse_low;

    struct __anc_ext_ear_adaptive_target_param *ff_target_param;
    struct __anc_ext_ear_adaptive_target_sv *ff_target_sv;
    struct __anc_ext_ear_adaptive_target_cmp *ff_target_cmp;

    struct __anc_ext_ear_adaptive_mem_param *ff_ear_mem_param;
    struct __anc_ext_ear_adaptive_mem_data *ff_ear_mem_pz;
    struct __anc_ext_ear_adaptive_mem_data *ff_ear_mem_sz;

    /*--------------------头戴式 右声道------------------------*/
    struct __anc_ext_ear_adaptive_iir_general *rff_iir_general;
    struct __anc_ext_ear_adaptive_iir_gains	*rff_iir_high_gains;
    struct __anc_ext_ear_adaptive_iir_gains	*rff_iir_medium_gains;
    struct __anc_ext_ear_adaptive_iir_gains	*rff_iir_low_gains;
    struct __anc_ext_ear_adaptive_iir *rff_iir_high;//[10];
    struct __anc_ext_ear_adaptive_iir *rff_iir_medium;//[10];
    struct __anc_ext_ear_adaptive_iir *rff_iir_low;//[10];

    struct __anc_ext_ear_adaptive_weight_param *rff_weight_param;
    struct __anc_ext_ear_adaptive_weight *rff_weight_high;
    struct __anc_ext_ear_adaptive_weight *rff_weight_medium;
    struct __anc_ext_ear_adaptive_weight *rff_weight_low;
    struct __anc_ext_ear_adaptive_mse *rff_mse_high;
    struct __anc_ext_ear_adaptive_mse *rff_mse_medium;
    struct __anc_ext_ear_adaptive_mse *rff_mse_low;

    struct __anc_ext_ear_adaptive_target_param *rff_target_param;
    struct __anc_ext_ear_adaptive_target_sv *rff_target_sv;
    struct __anc_ext_ear_adaptive_target_cmp *rff_target_cmp;

    struct __anc_ext_ear_adaptive_mem_param *rff_ear_mem_param;
    struct __anc_ext_ear_adaptive_mem_data *rff_ear_mem_pz;
    struct __anc_ext_ear_adaptive_mem_data *rff_ear_mem_sz;

    //FF->耳道自适应产测数据
    struct __anc_ext_ear_adaptive_dut_data *ff_dut_pz_cmp;
    struct __anc_ext_ear_adaptive_dut_data *ff_dut_sz_cmp;
    struct __anc_ext_ear_adaptive_dut_data *rff_dut_pz_cmp;
    struct __anc_ext_ear_adaptive_dut_data *rff_dut_sz_cmp;

};

struct __anc_ext_tool_hdl {

    u8 tool_online;					//工具在线连接标志
    u8 tool_ear_adaptive_en;		//工具启动耳道自适应标志
    struct list_head alloc_list;
    struct anc_ext_ear_adaptive_param ear_adaptive;	//耳道自适应工具参数
};



void anc_ext_tool_init(void);

//自适应结束工具回调函数
void anc_ext_tool_ear_adaptive_end_cb(u8 result);

//耳道自适应-符号校准 执行结果回调函数
void anc_ext_tool_ear_adaptive_sign_trim_end_cb(u8 *buf, int len);

int anc_ext_rsfile_read(void);

u8 *anc_ext_rsfile_get(u32 *file_len);

// 获取工具是否连接
u8 anc_ext_tool_online_get(void);

//获取耳道自适应参数
struct anc_ext_ear_adaptive_param *anc_ext_ear_adaptive_cfg_get(void);

//获取耳道自适应训练模式
enum ANC_EAR_ADPTIVE_MODE anc_ext_ear_adaptive_train_mode_get(void);

//获取耳道自适应-结果来源
u8 anc_ext_ear_adaptive_result_from(void);

// 检查是否有自适应参数, 0 正常/ 1 有参数为空
u8 anc_ext_ear_adaptive_param_check(void);

/*
   ANC_EXT SUBFILE文件解析遍历
	param: 	file_id 文件ID
		   	data ：目标数据地址
			len : 目标数据长度
			alloc_flag : 是否需要申请缓存空间（flash 文件解析传0，工具调试传1）
*/
int anc_ext_subfile_analysis_each(u32 file_id, u8 *data, int len, u8 alloc_flag);

/*-------------------ANCTOOL交互接口---------------------*/
//事件处理
void anc_ext_tool_cmd_deal(u8 *data, u16 len);

//写文件结束
int anc_ext_tool_write_end(u32 file_id, u8 *data, int len, u8 alloc_flag);

//读文件开始
int anc_ext_tool_read_file_start(u32 file_id, u8 **data, u32 *len);

//读文件结束
int anc_ext_tool_read_file_end(u32 file_id);

/*-------------------ANCTOOL交互接口END---------------------*/

#endif/*_ANC_EXT_TOOL_H_*/

