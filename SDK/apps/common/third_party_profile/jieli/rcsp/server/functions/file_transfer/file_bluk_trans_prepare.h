#ifndef __FILE_BLUK_TRANS_PREPARE_H__
#define __FILE_BLUK_TRANS_PREPARE_H__

#include "typedef.h"
#include "app_config.h"

//文件流传输准备
void rcsp_file_bluk_trans_prepare(void *priv, u8 OpCode_SN, u8 *data, u16 len);
//文件流传输初始化
void rcsp_file_bluk_trans_init(void (*end_callback)(void));
//文件流传输关闭
void rcsp_file_bluk_trans_close(u8 dire);

#endif
