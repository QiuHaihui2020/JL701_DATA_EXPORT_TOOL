/*************************************************************************************************/
/*!
*  \file      audio/framework/plugs/source/a2dp_streamctrl.h
*
*  \brief
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _A2DP_STREAM_CTRL_H_
#define _A2DP_STREAM_CTRL_H_

enum {
    A2DP_STREAM_DEFAULT_CONTROL = 0,
    A2DP_STREAM_JL_DONGLE_CONTROL = 1,
};

void *a2dp_stream_control_plan_select(void *stream, int low_latency, u32 codec_type, u8 plan);

int a2dp_stream_control_pull_frame(void *_ctrl, struct a2dp_media_frame *frame, int *len);

void a2dp_stream_control_free_frame(void *_ctrl, struct a2dp_media_frame *frame);

int a2dp_stream_control_delay_time(void *_ctrl);

void a2dp_stream_control_free(void *_ctrl);

void a2dp_stream_mark_next_timestamp(void *_ctrl, u32 next_timestamp);

void a2dp_stream_bandwidth_detect_handler(void *_ctrl, int pcm_frames, int sample_rate);

void a2dp_stream_control_set_underrun_callback(void *_ctrl, void *priv, void (*callback)(void *priv));
#endif
