#ifndef __RCSP_SPDIF_FUNC_H__
#define __RCSP_SPDIF_FUNC_H__
#include "typedef.h"
#include "app_config.h"



void rcsp_spdif_func_stop(void);
void rcsp_spdif_msg_deal(int msg);
bool rcsp_spdif_func_set(void *priv, u8 *data, u16 len);
u32 rcsp_spdif_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);


#endif
