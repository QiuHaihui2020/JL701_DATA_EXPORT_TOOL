#ifndef __FILE_TRANSFER_H__
#define __FILE_TRANSFER_H__

#include "typedef.h"
#include "app_config.h"

//*----------------------------------------------------------------------------*/
/**@brief    文件传输初始化处理
   @param	 end_callback:文件传输结束处理回调
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_init(void (*end_callback)(void));
//*----------------------------------------------------------------------------*/
/**@brief    文件下载拓展参数配置
   @param    OpCode_SN:数据包序列号，data:数据， len:数据长度
   @return   文件路径长度
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_parm_extra(u8 OpCode_SN, u8 *data, u16 len);
// rcsp文件下载开始命令处理
void rcsp_file_transfer_download_start(void *priv, u8 OpCode_SN, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件下载结束(F->A的回复处理)
   @param    status:命令执行状态，data:数据， len:数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_end(u8 status, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输过程处理(A->F)
   @param    data:数据， len:数据长度
   @return
   @note	 此过程接收的数据是实际文件传输的内容
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_doing(u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输，被动取消处理
   @param    OpCode_SN:数据包sn码， 回复的时候用的
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_passive_cancel(u8 OpCode_SN, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输，主动取消处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_active_cancel(void);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输，主动取消处理回复处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_download_active_cancel_response(u8 status, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输， 重命名处理
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_file_rename(u8 status, u8 *data, u16 len);
//*----------------------------------------------------------------------------*/
/**@brief    文件传输关闭处理
   @param
   @return
   @note	 主要是做一些资源释放处理
*/
/*----------------------------------------------------------------------------*/
void rcsp_file_transfer_close(void);

#endif//__FILE_TRANSFER_H__

