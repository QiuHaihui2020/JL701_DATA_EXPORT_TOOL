
#ifndef _AUDIO_LIMITER_API_H_
#define _AUDIO_LIMITER_API_H_

#include "typedef.h"
#include "system/init.h"
#include "generic/list.h"
#include "effects/effects_adj.h"
#include "audio_drc_common.h"
#include "effects/limiter_api.h"


struct limiter_param_tool {//与struct limiter_param关联
    int attack_time;       //命名Attack Time  默认1  步进 1  范围 1 ~ 1500ms (整数、界面显示无小数)
    int release_time;      //命名Release Time  默认 400   步进1   范围 1 ~ 1500ms(整数、界面显示无小数)
    int threshold;         //命名Threshold   默认0  步进1   范围  -138.4 ~ 0db  (界面保留两位小数 传下的参数乘以1000 取整)
    int hold_time;         //命名Limiter Type：  下拉框：soft_limiter(400)、hard_limiter(1000) 默认soft_limiter
    int detect_time;       //命名Detect Time  默认0    步进1  范围 0 ~ 25ms       (界面保留两位小数 传下的参数乘以100 取整)
    int input_gain;        //命名Input Gain  默认0   步进1   范围  -50 ~ 20db   (界面保留两位小数  传下的参数乘以1000 取整)
    int output_gain;       //命名Output Gain  默认0   步进1   范围  -50 ~ 20db      (界面保留两位小数  传下的参数乘以1000 取整)
    int precision;         //命名precision 下拉框：最高(1)、高（2）、普通（4）、最低（8） 默认最高(1)
    int resever[6];        //保留位
};

struct limiter_param_tool_set { // LIMITER界面参数
    int is_bypass;
    struct limiter_param_tool parm;
};


struct audio_limiter_param {
    struct limiter_param_tool_set *limiter;
    u32 sample_rate;           //数据采样率
    u8 channel;                //通道数
    u8 in_bit_width;           //输入位宽配置 0:16bit 1:32bit
    u8 out_bit_width;           //输出位宽配置 0:16bit 1:32bit
    u8 Qval;                   //算法饱和位宽 15：16bit  23:24bit
};

struct audio_limiter {
    void *work_buf;             //软件limiter句柄
    struct limiter_param_tool_set *limiter_inside;//内部使用
    struct audio_limiter_param param;
    u8 updata;                 //系数更标志
};


struct audio_limiter *audio_limiter_open(struct audio_limiter_param *param);
int audio_limiter_run(struct audio_limiter *hdl, s16 *indata, s16 *outdata, u32 len);
int audio_limiter_close(struct audio_limiter *hdl);
void audio_limiter_update_parm(struct audio_limiter *hdl, struct limiter_param_tool_set *parm);


#endif

