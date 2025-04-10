#ifndef __LOW_POWER_H__
#define __LOW_POWER_H__

void low_power_sys_request(void *priv);

s32 low_power_trace_drift(u32 usec);

void low_power_reset_osc_type(u8 type);

u8 low_power_get_default_osc_type(void);

u8 low_power_get_osc_type(void);

void power_set_soft_poweroff(void);

#include "power_manage.h"

#endif

