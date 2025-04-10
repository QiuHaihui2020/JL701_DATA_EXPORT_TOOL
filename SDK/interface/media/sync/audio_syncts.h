/*****************************************************************
>file name : audio_syncts.h
>create time : Mon 22 Mar 2021 02:41:39 PM CST
>description:
*****************************************************************/

#ifndef _AUDIO_SYNCTS_H_
#define _AUDIO_SYNCTS_H_
#include "typedef.h"
#include "asm/audio_src_base.h"


#define PCM_INSIDE_DAC                      0
#define PCM_OUTSIDE_DAC                     1
#define PCM_TRANSMIT                        2

#define AUDIO_NETWORK_LOCAL                 0
#define AUDIO_NETWORK_BT2_1                 1
#define AUDIO_NETWORK_BLE                   2
#define AUDIO_NETWORK_IPV4                  3

#define TIME_US_FACTOR      32

#define STREAM_TIMESTAMP_ENABLE             0x00000001
#define STREAM_ERROR_SUSPEND                0x00000002
#define STREAM_ERROR_RESUME                 0x00000004
/*
 * Audio同步变采样参数
 */
struct audio_syncts_params {
    unsigned char network;      /*网络选择*/
    unsigned char pcm_device;   /*PCM设备选择*/
    unsigned char nch;          /*声道数*/
    unsigned char factor;       /*timestamp的整数放大因子*/
    unsigned char bit_wide;     /*src 位宽控制 0：16ibt 1:24bit*/
    unsigned char low_latency;  /*低延迟配置*/
    unsigned char mode;
    int rin_sample_rate;        /*变采样输入采样率*/
    int rout_sample_rate;       /*变采样输出采样率*/
    void *priv;                 /*私有数据*/
    int (*output)(void *, void *, int);     /*变采样输出callback*/
};

struct audio_syncts_ioc_params {
    int cmd;
    u32 data[4];
};

enum audio_syncts_cmd {
    AUDIO_SYNCTS_CMD_NONE = 0,
    AUDIO_SYNCTS_UPDATE_PCM_FRAMES,
    AUDIO_SYNCTS_MOUNT_ON_SNDPCM,
    AUDIO_SYNCTS_UMOUNT_ON_SNDPCM,
    AUDIO_SYNCTS_GET_TIMESTAMP,
};

int audio_syncts_open(void **syncts, struct audio_syncts_params *params);

int audio_syncts_next_pts(void *syncts, u32 timestamp);

int audio_syncts_frame_filter(void *syncts, void *data, int len);

void audio_syncts_close(void *syncts);

u32 audio_syncts_get_dts(void *syncts);

int audio_syncts_set_dts(void *syncts, u32 dts);

int audio_syncts_trigger_resume(void *syncts, void *priv, void (*resume)(void *priv));

int audio_syncts_update_sample_rate(void *syncts, int sample_rate);

void audio_syncts_resample_suspend(void *syncts);

void audio_syncts_resample_resume(void *syncts);

int audio_syncts_buffered_frames(void *syncts);

int audio_syncts_drift_sample_rate(void *syncts);

u8 audio_syncts_get_hw_src_id(void *syncts);

int audio_syncts_push_data_out(void *syncts);

int audio_syncts_latch_enable(void *syncts);

int sound_pcm_update_frame_num(void *syncts, int frames);

int sound_pcm_syncts_latch_trigger(void *syncts);

u32 sound_buffered_between_syncts_and_device(void *priv, u8 time_select);

int sound_pcm_enter_update_frame(void *syncts);

int sound_pcm_exit_update_frame(void *syncts);

void sound_pcm_update_overflow_frames(void *syncts, int frames);

int sound_pcm_get_syncts_network(void *syncts);

int sound_pcm_update_frame_num_and_time(void *syncts, int frames, u32 time, int buffered_frames);

int sound_pcm_device_register(void *syncts, int pcm_device);

int sound_pcm_device_get(void *syncts);//获取关联的alink 模块

int audio_syncts_set_trigger_timestamp(void *syncts, u32 timestamp, u8 enable);

u8 audio_syncts_get_trigger_timestamp(void *syncts, u32 *timestamp);

u8 audio_syncts_support_use_trigger_timestamp(void *syncts);

int audio_syncts_frame_resample(void *syncts, struct resample_frame *in_frame, struct resample_frame *out_frame);

void audio_syncts_compensate_filter_latency(void *syncts, u8 enable);
#endif
