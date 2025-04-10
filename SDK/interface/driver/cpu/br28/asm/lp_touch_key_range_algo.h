#ifndef  __LP_TOUCH_KEY_RANGE_ALGO_H__
#define  __LP_TOUCH_KEY_RANGE_ALGO_H__


#include "typedef.h"


void TouchRangeAlgo_Init(u32 ch_idx, u16 min, u16 max);
void TouchRangeAlgo_Update(u32 ch_idx, u16 x);
void TouchRangeAlgo_Reset(u32 ch_idx, u16 min, u16 max);

u16 TouchRangeAlgo_GetRange(u32 ch_idx, u8 *valid);
void TouchRangeAlgo_SetRange(u32 ch_idx, u16 range);

s32 TouchRangeAlgo_GetSigma(u32 ch_idx);
void TouchRangeAlgo_SetSigma(u32 ch_idx, s32 sigma);


#endif

