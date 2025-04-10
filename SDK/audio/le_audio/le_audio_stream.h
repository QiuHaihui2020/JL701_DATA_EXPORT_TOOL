/*************************************************************************************************/
/*!
*  \file      le_audio_stream.h
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _LE_AUDIO_STREAM_H_
#define _LE_AUDIO_STREAM_H_
#include "typedef.h"
#include "generic/list.h"

#define LE_AUDIO_CURRENT_TIME       0
#define LE_AUDIO_SYNC_ENABLE        1
#define LE_AUDIO_LATCH_ENABLE       2
#define LE_AUDIO_GET_LATCH_TIME     3

struct le_audio_stream_format {
    u8 nch;
    u8 flush_timeout;
    u8 bit_width;
    s16 frame_dms;
    s16 sdu_period;
    int sample_rate;
    int bit_rate;
    u32 coding_type;
    int dec_ch_mode; //解码输出声道类型，
    u32 isoIntervalUs;
};

enum LEA_SERVICE {
    LEA_SERVICE_MEDIA,
    LEA_SERVICE_CALL
};

struct le_audio_stream_params {
    struct le_audio_stream_format fmt;
    enum LEA_SERVICE service_type;
    int latency;
    u16 conn;
};

struct le_audio_frame {
    u8 *data;
    int len;
    u32 timestamp;
    struct list_head entry;
};

struct le_audio_latch_time {
    u32 us;
    u32 event;
    u16 us_1_12th;
};

void *le_audio_stream_create(u16 conn, struct le_audio_stream_format *fmt);

void le_audio_stream_free(void *le_audio);

void le_audio_stream_set_tx_tick_handler(void *le_audio, void *priv, int (*tick_hanlder)(void *, int, u32));

void *le_audio_stream_tx_open(void *le_audio, int coding_type, void *priv, int (*tick_handler)(void *, int, u32));

void le_audio_stream_tx_close(void *stream);

int le_audio_stream_tx_write(void *stream, void *data, int len);

int le_audio_stream_tx_buffered_time(void *stream);

int le_audio_stream_tx_data_handler(void *le_audio, void *data, int len, u32 timestamp, int latency);

void *le_audio_stream_rx_open(void *le_audio, int coding_type);

int le_audio_stream_rx_write(void *stream, void *data, int len);

int le_audio_stream_rx_frame(void *stream, void *data, int len, u32 timestamp);

void le_audio_stream_set_rx_tick_handler(void *le_audio, void *priv, void (*tick_handler)(void *priv));

int le_audio_stream_get_rx_format(void *le_audio, struct le_audio_stream_format *fmt);
int le_audio_stream_get_dec_ch_mode(void *le_audio);

struct le_audio_frame *le_audio_stream_get_frame(void *le_audio);

void le_audio_stream_free_frame(void *le_audio, struct le_audio_frame *frame);

int le_audio_stream_get_frame_num(void *stream);

void le_audio_stream_rx_disconnect(void *stream);

void le_audio_stream_rx_close(void *stream);

int le_audio_stream_clock_select(void *le_audio);

int le_audio_stream_get_latch_time(void *le_audio, u32 *time, u16 *us_1_12th, u32 *event);

void le_audio_stream_latch_time_enable(void *le_audio);

u32 le_audio_stream_current_time(void *le_audio);

int le_audio_stream_set_bit_width(void *le_audio, u8 bit_width);

int le_audio_stream_tx_drain(void *stream);

int le_audio_stream_rx_drain(void *stream);

int le_audio_stream_set_start_time(void *stream, u32 start_time);
/*****************************LE Audio stream 音频流管理简介**************************
 *
 *  1、LE Audio 的一条BIS/CIS可对应一路LE Audio stream
 *  2、LE Audio stream一路数据流可以管理一路TX数据流和一路RX数据流
 *  3、LE Audio stream的RX兼容BIS/CIS传输音频帧以及本地TX后需要播放的同步音频帧
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 **************************************************************************************/
#endif
