
#ifndef __DEV_FORMAT_H__
#define __DEV_FORMAT_H__

#include "typedef.h"
#include "app_config.h"

//*----------------------------------------------------------------------------*/
/**@brief    设备格式化初始化(手机客户端发指令)
   @param
   @return   格式化完成回调
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_dev_format_init(void (*end_callback)(void));
//*----------------------------------------------------------------------------*/
/**@brief    设备格式化处理响应(手机客户端发指令)
   @param
   @return   格式化完成回调
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_dev_format_start(u8 OpCode_SN, u8 *data, u16 len);

#endif//__DEV_FORMAT_H__

