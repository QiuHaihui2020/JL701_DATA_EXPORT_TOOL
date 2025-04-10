#ifndef __AUDIO_GAIN_PORCESS__H
#define __AUDIO_GAIN_PORCESS__H

#include "media/audio_stream.h"
#include "media_config.h"

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

struct _aud_gain_parm_update {
    int revere_phase;
    float gain;
};
typedef struct _gain_process_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct _aud_gain_parm_update parm;
} gain_param_tool_set;

struct aud_steromix_gain_parm_update {
    float gain[4];
};
typedef struct _steromix_gain_process_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct aud_steromix_gain_parm_update parm;
} stereo_mix_gain_param_tool_set;

struct aud_split_gain_parm_update {
    int revere_phase_l;
    float gain_l;
    int revere_phase_r;
    float gain_r;
};
struct split_gain_param_tool_set {
    int is_bypass;
    struct aud_split_gain_parm_update parm;
};


struct aud_gain_parm {
    float gain[4];//增加多少dB
    u8 channel;//通道数
    u8 indata_inc;//channel ==1 ?1:2;
    u8 outdata_inc;//channel ==1 ?1:2;
    u8 bit_wide;//0:16bit  1：32bit
    u8 divide;//0:gain  1:split_gain  2:stereo_mix
};
struct aud_gain_parm_update {
    float gain[4];//增加多少dB
};
struct aud_gain_process {
    struct aud_gain_parm parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};





struct aud_gain_process *audio_gain_process_open(struct aud_gain_parm *parm);
void audio_gain_process_run(struct aud_gain_process *hdl, s16 *data, int len);
void audio_gain_process_close(struct aud_gain_process *hdl);
void audio_gain_process_update_parm(struct aud_gain_process *hdl,  struct aud_gain_parm_update *parm);
void audio_gain_process_bypass(struct aud_gain_process *hdl, u8 bypass);
struct aud_gain_process *get_cur_gain_hdl_by_name(u32 gain_name);

/*
 * *in:输入数据地址
 * *out:输出数据地址
 * gain:dB值(powf(10, 界面值gain / 20.0f))
 * channel:输入数据通道数
 * Indatainc:同个声道相邻两点的差值  channel ==1 ?1:2
 * OutdataInc:同个声道相邻两点的差值  channel ==1 ?1:2
 * per_channel_npoint:一个声道的点数
 * */
extern void GainProcess_16Bit(short *in, short *out, float gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint); //16位宽
/*
 * *in:输入数据地址
 * *out:输出数据地址
 * gain:dB值powf(10, 界面值gain / 20.0f)
 * channel:输入数据通道数
 * Indatainc:同个声道相邻两点的差值  channel ==1 ?1:2
 * OutdataInc:同个声道相邻两点的差值  channel ==1 ?1:2
 * per_channel_npoint:一个声道的点数
 * */
extern void GainProcess_32Bit(int *in, int *out, float gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint); //32位宽
void SplitGainProcess_16Bit(short *in, short *out, float *gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint); //16位宽
void SplitGainProcess_32Bit(int *in, int *out, float *gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint); //32位宽

//功能：包含增益调节、相位取反、左右声道混合、左右声道互换
/* 原理上是  */
// 左声道的数据 = 左声道的数据 * gain[0] + 右声道的数据 * gain[1]
/* 右声道的数据 = 左声道的数据 * gain[2] + 右声道的数据 * gain[3] */
//default: gain[0] = 1.0f gain[1] = 0.0f  gain[2] = 0.0f ,gain[3] = 1.0f
void SteroMix_16bit(short *in, short *out, float *gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint);
void SteroMix_32bit(int *in, int *out, float *gain, int channel, int IndataInc, int OutdataInc, int per_channel_npoint);
#endif
