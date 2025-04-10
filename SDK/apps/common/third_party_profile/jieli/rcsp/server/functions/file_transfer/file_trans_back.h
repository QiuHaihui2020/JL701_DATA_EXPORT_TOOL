#ifndef __FILE_TRANS_BACK__
#define __FILE_TRANS_BACK__

#include "typedef.h"
#include "app_config.h"

// 向手机发送文件
void rcsp_file_trans_back_opt(void *priv, u8 OpCode_SN, u8 *data, u16 len);
// 关闭向手机发送文件
void rcsp_file_trans_back_close(void);

#endif
