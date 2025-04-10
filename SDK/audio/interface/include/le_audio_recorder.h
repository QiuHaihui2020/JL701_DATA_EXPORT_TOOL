/*************************************************************************************************/
/*!
*  \file      le_audio_recorder.h
*
*  \brief
*
*  Copyright (c) 2011-2023 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _LE_AUDIO_RECORDER_H_
#define _LE_AUDIO_RECORDER_H_

int le_audio_a2dp_recorder_open(u8 *btaddr, void *arg, void *le_audio);
void le_audio_a2dp_recorder_close(u8 *btaddr);
u8 *le_audio_a2dp_recorder_get_btaddr(void);

int le_audio_linein_recorder_open(void *params, void *le_audio, int latency);
void le_audio_linein_recorder_close(void);

int le_audio_spdif_recorder_open(void *params, void *le_audio, int latency);
void le_audio_spdif_recorder_close(void);

int le_audio_iis_recorder_open(void *params, void *le_audio, int latency);
void le_audio_iis_recorder_close(void);

int le_audio_mic_recorder_open(void *params, void *le_audio, int latency);
void le_audio_mic_recorder_close(void);

int le_audio_fm_recorder_open(void *params, void *le_audio, int latency);
void le_audio_fm_recorder_close(void);

int le_audio_pc_recorder_open(void *params, void *le_audio, int latency);
void le_audio_pc_recorder_close(void);
#endif

