#ifndef _MULTI_CH_MIX_H
#define _MULTI_CH_MIX_H

struct mulit_mix_gain {
    float gain0;
    float gain1;
    float gain2;
};
typedef  struct _MULIT_MIX_TOOL_SET {
    int is_bypass;//打开时，只输出 gain0通道的数据
    struct mulit_mix_gain gain;
} multi_mix_param_tool_set;



typedef struct _MixParam {
    void *data;        //每路数据
    float gain;        //增益 eq_db2mag(gain);
} MixParam;

void Mix16to16(MixParam *mix1, MixParam *mix2, MixParam *mix3, short *out, int channel, int way_num, int per_channel_npoint);
//功能：输入数据16bit mix ,输出数据16bit
//way_num 为2或者3     way_num = 2  取 mix1  mix2的数据，mix3无效
//out  数据输出  带16bit饱和
//per_channel_npoint   每个通道的样点数

void Mix32to32(MixParam *mix1, MixParam *mix2, MixParam *mix3, int *out, int channel, int way_num, int per_channel_npoint);
//功能： 输入数据32bit mix ,输出数据32bit
//way_num 为2或者3     way_num = 2  取 mix1  mix2的数据，mix3无效
//out  数据输出   不带16bit饱和
//per_channel_npoint   每个通道的样点数

void Mix32to16(MixParam *mix1, MixParam *mix2, MixParam *mix3, short *out, int channel, int way_num, int per_channel_npoint);
//功能： 输入数据32bit mix ,输出数据16bit
//way_num 为2或者3     way_num = 2  取 mix1  mix2的数据，mix3无效
//out  数据输出   带16bit饱和
//per_channel_npoint   每个通道的样点数

void Mix16to32(MixParam *mix1, MixParam *mix2, MixParam *mix3, int *out, int channel, int way_num, int per_channel_npoint);
//功能： 输入数据16bit mix ,输出数据32bit
//way_num 为2或者3     way_num = 2  取 mix1  mix2的数据，mix3无效
//out  数据输出   不带16bit饱和
//per_channel_npoint   每个通道的样点数


/*
 *16bit位宽，多带数据叠加
 **out_buf_tmp:输出目标地址
 *point_per_channel:每通道点数
 * */
void band_merging_16bit(short *in0, short *in1, short *in2, short *out_buf_tmp, int points, u8 nband);

/*32bit位宽，多带数据叠加
 *out_buf_tmp:输出目标地址
 *point_per_channel:每通道点数
 * */
void band_merging_32bit(int *in0, int *in1, int *in2, s32 *out_buf_tmp, int points, u8 nband);

/*
 *将数据叠加到输出buf
 *mix:输入数据结构
 *out:输出数据地址
 *npoint:总的点数
 * */
void mix_data_16to16(MixParam *mix, short *out, int npoint);
void mix_data_32to16(MixParam *mix, short *out, int npoint);
void mix_data_32to32(MixParam *mix, int *out, int npoint);
void mix_data_16to32(MixParam *mix, int *out, int npoint);

#endif
