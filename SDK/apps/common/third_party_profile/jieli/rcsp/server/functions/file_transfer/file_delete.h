#ifndef __FILE_DELETE_H__
#define __FILE_DELETE_H__

#include "typedef.h"
#include "app_config.h"

//*----------------------------------------------------------------------------*/
/**@brief    文件删除初始化
   @param	 end_callback:文件删除结束回调处理
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_delete_init(void (*end_callback)(void));
//文件删除命令处理
void rcsp_file_delete_start(u8 OpCode_SN, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件删除结束处理
   @param
   @return
   @note	 释放资源
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_delete_end(void);
// 删除一个文件
void rcsp_file_delete_one_file(u8 OpCode_SN, u8 *data, u16 len);

#endif//__FILE_DELETE_H__

