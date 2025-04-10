#ifndef __CONVERT_DATA_H
#define __CONVERT_DATA_H

#include "system/includes.h"
#include "media/audio_stream.h"
#include "effects/effects_adj.h"

struct convert_data {
    u16 dat_len;
    u16 dat_total;
    u16 buf_len;
    u16 *buf;
    u8 type;
    u8 channel;
};

struct convert_data *convet_data_open(int type, u32 buf_len);
void convet_data_close(struct convert_data *hdl);
void audio_convert_data_run(struct convert_data *hdl, void *in, void *out, u16 len);

void user_sat16(s32 *in, s16 *out, u32 npoint);

void audio_DatacConvert_LS_16to32bit(short *in, int *out, int npoint, int Nbit);

/* note: 32BIT转16bit位宽处理
 * *indata:32bit位宽输入地址
 * *outdata:16bit位宽输出地址
 * npoint:总的输入点数(位移方式处理)
 * */
void audio_convert_data_32bit_to_16bit_round(s32 *in, s16 *out, u32 npoint);


/* note: 16BIT转32bit位宽处理
 * *indata:16bit位宽输入地址
 * *outdata:32bit位宽输出地址
 * npoint:总的输入点数(位移方式处理)
 * */
void audio_convert_data_16bit_to_32bit_round(s16 *indata, s32 *outdata, int npoint);

/* note: 32BIT转16bit位宽处理
 * *indata:32bit位宽输入地址
 * *outdata:16bit位宽输出地址
 * npoint:总的输入点数(溢出饱和方式处理)
 * */
void audio_convert_data_32bit_to_16bit(s32 *inata, s16 *outdata, u32 npoint);

/* note: 16BIT转32bit位宽处理
 * *indata:16bit位宽输入地址
 * *outdata:32bit位宽输出地址
 * npoint:总的输入点数(16bit 位宽数据赋值给32bit位宽数据，不做放大)
 * */
void audio_convert_data_16bit_to_32bit(s16 *indata, s32 *outdata, int npoint);

/* note: 16BIT转32bit位宽处理接口2
 * *indata:16bit位宽输入地址
 * *outdata:32bit位宽输出地址
 * channel:输入输出通道数
 * IndataInc: 输入数据同个通道下个点的步进 例如左右左右  步进为2,单声道是1
 * OutdataInc:输出数据同个通道下个点的步进 例如左右左右  步进为2,单声道是1
 * per_channel_npoint:单个通道的输入点数
 * */
void audio_convert_data_16bit_to_32bit_api2(short *indata, int *outdata, int channel, int IndataInc, int OutdataInc, int per_channel_npoint);

/* note: 32BIT转16bit位宽处理接口2
 * *indata:32bit位宽输入地址
 * *outdata:16bit位宽输出地址
 * channel:输入输出通道数
 * IndataInc: 输入数据同个通道下个点的步进 例如左右左右  步进为2,单声道是1
 * OutdataInc:输出数据同个通道下个点的步进 例如左右左右  步进为2,单声道是1
 * per_channel_npoint:单个通道的输入点数
 * */
void audio_convrt_data_32bit_to_16bit_api2(int *indata, short *outdata, int channel, int IndataInc, int OutdataInc, int pre_channel_npoint);

/* note: 3byte_24bit转16bit位宽处理
 * *in:3byte_24bit位宽输入地址
 * *out:16bit位宽输出地址
 *  total_point:总的输入点数
 * */
void audio_convert_data_3byte24bit_to_16bit(int *in, short *out, unsigned int total_point);

/* note: 3byte_24bit转4byte_24bit位宽处理
 * *in:3byte_24bit位宽输入地址
 * *out:4byte_24bit位宽输出地址
 *  total_point:总的输入点数
 * */
void audio_convert_data_3byte24bit_to_4byte24bit(int *in, int *out, unsigned int total_point);

/* note: 4byte_24bit转3byte_24bit位宽处理
 * *in:4byte_24bit位宽输入地址
 * *out:3byte_24bit位宽输出地址
 *  total_point:总的输入点数
 * */
void audio_convert_data_4byte24bit_to_3byte24bit(int *in, int *out, unsigned int total_point);

/* note: 16bit转3byte_24bit位宽处理
 * *in:16bit位宽输入地址
 * *out:3byte_24bit位宽输出地址
 *  total_point:总的输入点数
 * */
void audio_convert_data_16bit_to_3byte24bit(short *in, int *out, unsigned int total_point);

extern int data_sat_s24(int ind);
extern void audio_data_sat_s24(s32 *data, u32 len);

/* note: 三路输入叠加并24bit饱和输出
 * *indata1:32bit位宽输入地址
 * *indata2:32bit位宽输入地址
 * *indata3:32bit位宽输入地址
 * *outdata:24bit位宽输出地址
 *  npoint:总的输入点数
 * */
void audio_mix_3t1_data_sat_s24(int *indata1, int *indata2, int *indata3, int *outdata, int npoint);

//type
#define CONVERT_32_TO_16 0
#define CONVERT_16_TO_32 1

extern const int config_media_24bit_enable;

#endif/*__CONVERT_DATA_H*/
