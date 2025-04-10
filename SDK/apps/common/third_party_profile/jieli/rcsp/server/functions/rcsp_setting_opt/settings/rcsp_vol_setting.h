#ifndef __RCSP_VOL_SETTING_H__
#define __RCSP_VOL_SETTING_H__

#include "app_config.h"

/**
 * @brief 获取当前设备音量
 *
 * @param vol_info 当前设备音量大小
 */
void rcsp_get_cur_dev_vol_info(u8 *vol_info);
/**
 * @brief 获取当前设备最大音量
 *
 * @param vol_info 当前设备最大音量
 */
void rcsp_get_max_vol_info(u8 *vol_info);


#endif
