#ifndef __RCSP_RTC_FUNC_H__
#define __RCSP_RTC_FUNC_H__
#include "typedef.h"
#include "app_config.h"


u8 rcsp_rtc_ring_tone(void);
u16 rcsp_rtc_func_get_ex(void *priv, u8 *buf, u16 buf_size, u8 mask);
bool rcsp_rtc_func_set_ex(void *priv, u8 *data, u16 len);

#endif
