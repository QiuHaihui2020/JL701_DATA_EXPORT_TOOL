#ifndef __AUDIO_MULTIBAND_DRC__
#define __AUDIO_MULTIBAND_DRC__

#include "AudioEffect_DataType.h"
#include "audio_wdrc_advance.h"
#include "audio_eq.h"
#include "multi_ch_mix.h"
#include "effects/audio_crossover.h"
#include "effects/audio_multiband_crossover.h"

//mdrc内子模块在线调音的标识
enum update_type {
    COMMON_PARM = 0x01,
    LOW_DRC_PARM,
    MID_DRC_PARM,
    HIGH_DRC_PARM,
    WHOLE_DRC_PARM
};

struct mdrc_param_tool_set {
    struct mdrc_common_param common_param;
    int drc_len[4];                                //drc数据结构长度
    wdrc_advance_param_tool_set *drc_param[4];     //外部暂存的drc参数，用于传参给内部使用，传完即释放
};

struct mdrc_param_update {
    int type;                                      //更新的数据类型
    int len;                                       //更新的数据长度，type = 0x1时工具不发len
    u8 *data;                                      //更新的数据
};

struct multiband_drc_hdl {
    struct audio_wdrc_advance *drc[4];              //DRC句柄
    wdrc_advance_param_tool_set *drc_param[4];     //内部使用的drc参数，点数改变，这个buffer会重新申请，重新申请后需要更新ptr给wdrc内部
    struct mdrc_common_param common_param;
    struct multiband_crossover mcross;
    u8 mdrc_init;
};

struct multiband_drc_open_param {
    struct mdrc_param_tool_set *mdrc_tool_parm;
    u32 sample_rate;
    u8 channel;
    u8 run32bit;                                  //运行位宽32bit使能
    u8 Qval;                                      //数据饱和值15：16bit  23:24bit
    u8 eq_core;                                   //硬件分频器的所使用的eq核
};
/****************************** file read function **********************************/
/* 获取数据结构体长度 */
int multiband_drc_param_size_get(struct mdrc_param_tool_set *param);
/* 通用参数打印 */
void multiband_drc_common_param_debug(struct mdrc_common_param *param);
/* drc参数打印 */
void multiband_drc_param_debug(struct mdrc_param_tool_set *param);
/* 根据读取的drc数据结构长度申请内存，保存参数，需要跟free函数配套使用 */

//注：可以做成mdrc_parm是返回值的用法，参数只传data_buf
void multiband_drc_param_set(struct mdrc_param_tool_set *mdrc_parm, u8 *data_buf);
/* 释放保存drc参数的内存 */
void multiband_drc_param_free(struct mdrc_param_tool_set *mdrc_parm);

/****************************** online update function ******************************/
/* 获取参数更新拿到的数据长度 */
int multiband_drc_online_update_param_size_get(int *online_data);
/* 根据工具下发的数据，读取type、len，根据len申请内存，保存更新数据，需要跟free函数配套使用 */

//注：可以做成param是返回值的用法，参数只传online_data
void multiband_drc_online_update_param_set(struct mdrc_param_update *param, int parm);
/* 释放保存更新参数的内存 */
void multiband_drc_online_update_param_free(struct mdrc_param_update *param);
/* 更新的参数打印 */
void multiband_drc_online_update_param_debug(struct mdrc_param_update *param);

/********************************** mdrc function ***********************************/
struct multiband_drc_hdl *audio_multiband_drc_open(struct multiband_drc_open_param *param);
int audio_multiband_drc_close(struct multiband_drc_hdl *hdl);
int audio_multiband_drc_run(struct multiband_drc_hdl *hdl, s16 *indata, s16 *outdata, u32 len);
void audio_multiband_drc_update_parm(struct multiband_drc_hdl *hdl, struct mdrc_param_update *update_param);

#endif
