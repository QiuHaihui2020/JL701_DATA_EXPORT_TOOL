
#ifndef _WDRC_ADVANCE_API_H_
#define _WDRC_ADVANCE_API_H_

#include "typedef.h"
#include "system/init.h"
#include "generic/list.h"
#include "effects/drc_advance_api.h"
#include "effects/effects_adj.h"
#include "audio_drc_common.h"

struct wdrc_advance_struct {
    float attack_time;					//启动时间
    float release_time;			    	//释放时间
    float compress_attack_time;	    	//压缩启动时间
    float compress_release_time;	   	//压缩释放时间
    float rms_time;				    	//rms_time
    float detect_time;					//检测时间
    float input_gain;					//输入增益
    float output_gain;					//输出增益
    int threshold_num;			    	//阈值组个数
    float max_knee_width;
    u8 algorithm;			            //算法类型 PEAK RMS
    u8 mode;				            //检测模式 precision  precision+
    u8 flag;
    u8 reverved;
    int reverved1[4];
    struct threshold_group threshold[0];
};


//动态范围控制 DRC:
typedef struct _wdrc_advance_struct_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct wdrc_advance_struct parm;
} wdrc_advance_param_tool_set;


struct audio_wdrc_advance_filter_info {
    wdrc_advance_param_tool_set *wdrc_advance;
};

typedef int (*audio_wdrc_advance_filter_cb)(void *drc, struct audio_wdrc_advance_filter_info *info);

struct audio_wdrc_advance_param {
    u32 sample_rate;           //数据采样率
    u8 channel;                //通道数
    u8 run32bit;               //是否使能32bit的位宽数据处理  1:使能  0：不使能
    u8 Qval;
    wdrc_advance_param_tool_set *wdrc_advance;
};

struct audio_wdrc_advance {
    void *work_buf;             //软件drc句柄
    wdrc_advance_param_tool_set *wdrc_advance_inside;//内部使用
    wdrc_advance_param_tool_set *wdrc_advance;//由输入参数指定


    u32 sample_rate;           //采样率
    u8 channel;                //通道数
    u8 run32bit;               //是否使能32bit位宽数据处理1:使能  0：不使能
    u8 updata;                 //系数更标志
    u8 Qval;
};


struct audio_wdrc_advance *audio_wdrc_advance_open(struct audio_wdrc_advance_param *param);
int audio_wdrc_advance_run(struct audio_wdrc_advance *drc, s16 *indata, s16 *outdata, u32 len);
int audio_wdrc_advance_close(struct audio_wdrc_advance *drc);
void audio_wdrc_advance_set_bit_wide(struct audio_wdrc_advance *drc, u8 run32bit);
void audio_wdrc_advance_update_parm(struct audio_wdrc_advance *drc, void *wdrc_advance_parm);
void audio_wdrc_advance_update_parm_ptr(struct audio_wdrc_advance *drc, void *wdrc_advance_parm);//坐标点数发生变化时，重新设定参数记录的地址
int wdrc_advance_get_param_size(u8 threshold_num);//根据坐标点的个数，计算结构体的长度
void wdrc_advance_printf(void *_wdrc_advance);
int wdrc_advance_init(struct audio_wdrc_advance *drc, struct wdrc_advance_struct *wdrc_advance_parm, u8 init);
void wdrc_advance_uninit(struct audio_wdrc_advance *drc);


#endif

