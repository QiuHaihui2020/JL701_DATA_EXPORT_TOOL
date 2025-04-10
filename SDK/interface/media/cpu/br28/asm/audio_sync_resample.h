/*****************************************************************
>file name : audio_sync_resample.h
>author : lichao
>create time : Sat 26 Dec 2020 02:27:11 PM CST
*****************************************************************/
#ifndef _AUDIO_SYNC_RESAMPLE_H_
#define _AUDIO_SYNC_RESAMPLE_H_
#include "typedef.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "audio_src_base.h"

#define audio_sync_resample_open                    audio_src_base_open
#define audio_sync_resample_set_output_handler      audio_src_base_set_output_handler
#define audio_sync_resample_close                   audio_src_base_close
#define audio_sync_resample_set_silence             audio_src_base_set_silence
#define audio_sync_resample_config                  audio_src_base_resample_config
#define audio_sync_resample_write                   audio_src_base_write
#define audio_sync_resample_stop                    audio_src_base_stop
#define audio_sync_resample_position                audio_src_base_position
#define audio_sync_resample_scale_output            audio_src_base_scale_output
#define audio_sync_resample_bufferd_frames          audio_src_base_bufferd_frames
#define audio_sync_resample_out_frames              audio_src_base_out_frames
#define audio_sync_resample_run_scale               audio_src_base_run_scale
#define audio_sync_resample_push_data_out           audio_src_base_push_data_out
#define audio_sync_resample_wait_irq_callback       audio_src_base_wait_irq_callback

#endif

