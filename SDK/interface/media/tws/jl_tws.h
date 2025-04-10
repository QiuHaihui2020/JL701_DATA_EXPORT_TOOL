/*************************************************************************************************/
/*!
*  \file      jl_tws.h
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _JL_TWS_H_
#define _JL_TWS_H_
#include "audio_def.h"

struct jl_tws_header {
    unsigned version : 2;       //版本信息
    unsigned x : 1;             //扩展信息
    unsigned reserved : 1;      //保留信息0
    unsigned frame_num : 4;     //帧数量(duration)
    short drift_sample_rate;    //采样率偏移
    unsigned int timestamp;     //时间戳
} __attribute__((packed));

struct jl_tws_audio_format {
    u8 channel;
    u32 coding_type;
    int sample_rate;
    int frame_dms;
    int bit_rate;
};

struct local_tws_switch_params {
    u8 enable;
    u8 channel;
};

#define SAMPLE_RATE_32000   0
#define SAMPLE_RATE_44100   1
#define SAMPLE_RATE_48000   2
#define SAMPLE_RATE_64000   3

#define FRAME_DURATION_2_5_MS   0
#define FRAME_DURATION_5_MS     1
#define FRAME_DURATION_10_MS    2

#define JL_TWS_CH_MONO      0
#define JL_TWS_CH_STEREO    1
/*
 * 从TWS的sample rate信息得到具体的采样率值
 */
static inline int jl_tws_sample_rate(u8 sample_rate)
{
    int sample_rate_table[] = {
        32000, 44100, 48000, 64000,
    };

    return sample_rate_table[sample_rate];
}

/*
 * 从实际的采样率值匹配为采样率信息值
 */
static inline u8 jl_tws_match_sample_rate(int sample_rate)
{
    int sample_rate_table[] = {
        32000, 44100, 48000, 64000,
    };

    int i = 0;
    for (i = 0; i < SAMPLE_RATE_64000; i++) {
        if (sample_rate == sample_rate_table[i]) {
            return i;
        }
    }
    return 0;
}

/*
 * 从TWS的编码格式信息转换为实际的编码格式参数
 */
static inline int jl_tws_coding_type(u8 codec_type)
{
    u32 coding_type_table[] = {
        AUDIO_CODING_JLA,
        AUDIO_CODING_SBC,
    };

    return coding_type_table[codec_type];
}

/*
 * 实际的编码格式参数转换为TWS的编码格式
 */
static inline int jl_tws_coding_type_id(u32 coding_type)
{
    u32 coding_type_table[] = {
        AUDIO_CODING_JLA,
        AUDIO_CODING_SBC,
    };
    int i = 0;
    for (i = 0; i < sizeof(coding_type_table) / 4; i++) {
        if (coding_type == coding_type_table[i]) {
            return i;
        }
    }
    return 0;
}
/*
 * 从TWS的帧间隔信息转换为实际的帧间隔值
 */
static inline int jl_tws_frame_duration(u8 duration)
{
    u32 frame_durations[] = {
        25, 50, 100,
    };

    return frame_durations[duration];
}
/*
 * 从实际的帧间隔配置到TWS的帧间隔信息
 */
static inline u8 jl_tws_match_frame_duration(int duration)
{
    u32 frame_durations[] = {
        25, 50, 100,
    };
    int i = 0;
    for (i = 0; i < FRAME_DURATION_10_MS; i++) {
        if (duration == frame_durations[i]) {
            return i;
        }
    }
    return FRAME_DURATION_10_MS;
}

#endif
