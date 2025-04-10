#ifndef _RCSP_CH_LOADER_DOWNLOAD_H_
#define _RCSP_CH_LOADER_DOWNLOAD_H_

/**
 * @brief 断开连接时，需要清空正在升级的bthdl
 */
void rcsp_clean_update_hdl_for_end_update(u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief rcsp升级继续
 */
void rcsp_update_resume(void);

#endif

