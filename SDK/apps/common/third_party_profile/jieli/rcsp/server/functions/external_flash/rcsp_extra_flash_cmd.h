#ifndef _RCSP_EXTRA_FLASH_CMD_H_
#define _RCSP_EXTRA_FLASH_CMD_H_
#include "typedef.h"
#include "system/event.h"

//外挂flash命令处理
int JL_rcsp_extra_flash_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len);
//外挂flash不回复命令处理
int JL_rcsp_extra_flash_cmd_no_resp(void *priv, u8 OpCode, u8 *data, u16 len);

/**
 * @brief 外挂flash命令发送
 *
 * @param dire 非0:命令发送；0:命令回复
 */
int rcsp_extra_flash_opt_resp(u8 dire, u8 OpCode, u8 OpCode_SN, u8 *resp_data, u16 data_len);

#endif
