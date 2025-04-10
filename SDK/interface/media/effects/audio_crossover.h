#ifndef _DRC_API_H_
#define _DRC_API_H_

#include "typedef.h"
#include "system/init.h"
#include "media/audio_stream.h"

//分频器 crossover:
typedef struct _CrossOverParam_TOOL_SET {
    int way_num;        //段数
    int N;				//阶数
    int low_freq;	    //低中分频点
    int high_freq;      //高中分频点
} CrossOverUpdateParam;



//分频器 crossover:
typedef struct _CrossOverParam_EFF_TOOL_SET {
    int is_bypass;     //打开时，只通过低频端口做输出，输出全频的信号,其余端口输出0数据
    CrossOverUpdateParam parm;
} crossover_param_tool_set;

struct audio_crossover_filter_info {
    CrossOverUpdateParam *crossover;
};

typedef int (*audio_crossover_filter_cb)(void *drc, struct audio_crossover_filter_info *info);

struct audio_crossover_param {
    u32 sample_rate;                   //数据采样率
    u8 channel;                        //通道数
    u8 run32bit;                       //是否支持32bit 的输入数据处理  1:使能  0：不使能
    u8 core;
    u8 Qval;
    CrossOverUpdateParam *crossover;
    void *priv;
    int (*output[3])(void *priv, void *data, u32 len);
};

struct audio_crossover {
    void *work_buf;
    CrossOverUpdateParam *crossover_inside;//
    CrossOverUpdateParam *crossover;//
    void *hw_crossover[3];
    void *priv;
    int (*output[3])(void *priv, void *data, u32 len);

    u32 sample_rate;                   //采样率
    u8 channel;                        //通道数
    u8 run32bit;                       //是否使能32bit位宽数据处理1:使能  0：不使能
    u8 updata;                         //系数更标志
    u8 core;
    u8 Qval;
};


struct audio_crossover *audio_crossover_open(struct audio_crossover_param *param);
int audio_crossover_close(struct audio_crossover *hdl);
int audio_crossover_run(struct audio_crossover *hdl, s16 *indata, s16 *outdata0, s16 *outdata1, s16 *outdata2, u32 len);//每次处理多个频带
void audio_crossover_update_parm(struct audio_crossover *hdl, CrossOverUpdateParam *crossover_parm);
void audio_crossover_set_bit_wide(struct audio_crossover *hdl, u8 run32bit);
int audio_crossover_run_pro(struct audio_crossover *hdl, s16 *indata, s16 *outdata, u32 len, int band_tar);//每次处理一个频带
int audio_crossover_flush_out(struct audio_crossover *hdl);

#endif
