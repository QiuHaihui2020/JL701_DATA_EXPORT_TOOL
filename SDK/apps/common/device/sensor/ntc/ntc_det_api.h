#ifndef __NTC_DET_API_H__
#define __NTC_DET_API_H__

#include "typedef.h"
#include "app_config.h"

#if NTC_DET_EN
extern void ntc_det_start(void);
extern void ntc_det_stop(void);
extern u16 ntc_det_working();
extern u32 ntc_get_inside_pull_up_res_trim_value(void);
extern u32 ntc_get_detect_upper(u32 ntc_res);
extern u32 ntc_get_detect_lower(u32 ntc_res);
#endif

#endif //__NTC_DET_API_H__

