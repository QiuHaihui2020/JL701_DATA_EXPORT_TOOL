#ifndef __RCSP_FM_FUNC_H__
#define __RCSP_FM_FUNC_H__
#include "typedef.h"
#include "app_config.h"


#define FM_INFO_ATTR_STATUS                    (0)
#define FM_INFO_ATTR_FRE                       (1)

void rcsp_fm_func_stop(void);
void rcsp_fm_msg_deal(int msg);


#endif
