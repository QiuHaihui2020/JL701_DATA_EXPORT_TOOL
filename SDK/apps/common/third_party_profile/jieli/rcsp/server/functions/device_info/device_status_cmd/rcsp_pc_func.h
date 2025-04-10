#ifndef __RCSP_PC_FUNC_H__
#define __RCSP_PC_FUNC_H__
#include "typedef.h"
#include "app_config.h"



void rcsp_pc_func_stop(void);
void rcsp_pc_msg_deal(int msg);
bool rcsp_pc_func_set(void *priv, u8 *data, u16 len);
u32 rcsp_pc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);


#endif
