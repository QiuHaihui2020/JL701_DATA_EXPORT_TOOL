/*************************************************************************************************/
/*!
*  \file      le_audio_buffer.h
*
*  \brief
*
*  Copyright (c) 2011-2024 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#ifndef _LE_AUDIO_BUFFER_H_
#define _LE_AUDIO_BUFFER_H_
#include "typedef.h"
#include "le_audio_stream.h"

void *le_audio_buffer_open(int frame_len, int size);

int le_audio_push_frame(void *buffer, void *data, int len, u32 timestamp);

struct le_audio_frame *le_audio_get_frame(void *buffer);

void le_audio_free_frame(void *buffer, struct le_audio_frame *frame);

void le_audio_buffer_close(void *buffer);

#endif

