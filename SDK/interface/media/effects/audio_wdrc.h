
#ifndef _WDRC_API_H_
#define _WDRC_API_H_

#include "typedef.h"
#include "system/init.h"
#include "generic/list.h"
#include "effects/drc_api.h"
#include "effects/effects_adj.h"
#include "audio_drc_common.h"

struct wdrc_struct {
    u16 attacktime;   //启动时间
    u16 releasetime;  //释放时间
    struct threshold_group threshold[6];
    float inputgain;	   //wdrc输入数据放大或者缩小的db数（正数放大，负数缩小）, 例如配2表示 总体增加2dB
    float outputgain;	   //wdrc输出数据放大或者缩小的db数（正数放大，负数缩小）
    float rms_time;
    u8 threshold_num;
    u8 old_rms_time;//无用
    u8 algorithm;//0:PEAK  1:RMS
    u8 mode;//0:PERPOINT  1:TWOPOINT
    u16 holdtime; //预留位置
    u8 reverved[2];//预留位置
};


//动态范围控制 DRC:
typedef struct _wdrc_struct_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct wdrc_struct parm;
} wdrc_param_tool_set;


struct audio_wdrc_filter_info {
    wdrc_param_tool_set *wdrc;
};

typedef int (*audio_wdrc_filter_cb)(void *drc, struct audio_wdrc_filter_info *info);

struct audio_wdrc_param {
    u32 sample_rate;           //数据采样率
    u8 channel;                //通道数
    u8 run32bit;               //是否使能32bit的位宽数据处理  1:使能  0：不使能
    u8 Qval;
    wdrc_param_tool_set *wdrc;
};

struct audio_wdrc {
    void *work_buf;             //软件drc句柄
    wdrc_param_tool_set *wdrc_inside;//内部使用
    wdrc_param_tool_set *wdrc;//由输入参数指定

    u32 sample_rate;           //采样率
    u8 channel;                //通道数
    u8 run32bit;               //是否使能32bit位宽数据处理1:使能  0：不使能
    u8 Qval;
    u8 updata;                 //系数更标志
};


struct audio_wdrc *audio_wdrc_open(struct audio_wdrc_param *param);
int audio_wdrc_run(struct audio_wdrc *drc, s16 *indata, s16 *outdata, u32 len);
int audio_wdrc_close(struct audio_wdrc *drc);
void audio_wdrc_set_bit_wide(struct audio_wdrc *drc, u8 run32bit);
void audio_wdrc_update_parm(struct audio_wdrc *drc, void *wdrc_parm);


#endif

