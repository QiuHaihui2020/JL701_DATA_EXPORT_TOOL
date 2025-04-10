/*************************************************************************************************/
/*!
*  \file      reference_time.h
*
*  \brief     音频的参考时钟选择与设置
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/

#ifndef _REFERENCE_TIME_H_
#define _REFERENCE_TIME_H_
#include "typedef.h"

int audio_reference_clock_select(void *addr, u8 network);

u32 audio_reference_clock_time(void);

u32 audio_reference_network_clock_time(u8 network);

u8 is_audio_reference_clock_enable(void);

u8 audio_reference_clock_match(void *addr, u8 network);

u8 audio_reference_clock_network(void *addr);

void audio_reference_clock_exit(u8 id);

u32 audio_reference_clock_remapping(u8 now_network, u8 dst_network, u32 clock);

u8 audio_reference_network_exist(u8 reference);
#endif
