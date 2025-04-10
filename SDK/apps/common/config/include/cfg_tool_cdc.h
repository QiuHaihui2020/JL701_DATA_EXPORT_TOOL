#ifndef _APP_CFG_TOOL_CDC_
#define _APP_CFG_TOOL_CDC_

#include "typedef.h"

/**
 *	@brief 获取工具cdc最大支持的协议包大小
 */
u16 cfg_tool_cdc_rx_max_mtu();

/**
 *	@brief 获取cdc的配置/调音工具相关数据
 */
void cfg_tool_data_from_cdc(u8 *buf, u32 rlen);

#endif

