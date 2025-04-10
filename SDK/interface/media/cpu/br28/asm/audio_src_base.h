/*****************************************************************
>file name : audio_src_base.h
>create time : Wed 02 Mar 2022 11:12:07 AM CST
*****************************************************************/
#ifndef _AUDIO_SRC_BASE_H_
#define _AUDIO_SRC_BASE_H_
#include "asm/audio_src.h"

struct resample_frame {
    u8 nch;
    int offset;
    int len;
    int size;
    void *data;
};

void *audio_src_base_open(u8 channel, int in_sample_rate, int out_sample_rate, u8 type);

int audio_src_base_set_output_handler(void *resample,
                                      void *priv,
                                      int (*handler)(void *priv, void *data, int len));

int audio_src_base_set_channel(void *resample, u8 channel);

int audio_src_base_set_in_buffer(void *resample, void *buf, int len);

int audio_src_base_set_input_buff(void *resample, void *buf, int len);

int audio_src_base_resample_config(void *resample, int in_rate, int out_rate);

int audio_src_base_write(void *resample, void *data, int len);

int audio_src_base_stop(void *resample);

int audio_src_base_run_scale(void *resample);

int audio_src_base_input_frames(void *resample);

u32 audio_src_base_out_frames(void *resample);

float audio_src_base_position(void *resample);

int audio_src_base_scale_output(void *resample, int in_sample_rate, int out_sample_rate, int frames);

int audio_src_base_bufferd_frames(void *resample);

int audio_src_base_set_silence(void *resample, u8 silence, int fade_time);

int audio_src_base_wait_irq_callback(void *resample, void *priv, void (*callback)(void *));

int audio_src_base_frame_resample(void *resample, struct resample_frame *in_frame, struct resample_frame *out_frame);

void audio_src_base_close(void *resample);

int audio_src_base_filter_frames(void *resample);

int audio_src_base_push_data_out(void *resample);
u8 audio_src_base_get_hw_core_id(void *resample);

#endif

