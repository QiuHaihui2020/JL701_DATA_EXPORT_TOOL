#ifndef __RCSP_DEVICE_STATUS_H__
#define __RCSP_DEVICE_STATUS_H__

#include "typedef.h"
#include "app_config.h"

// 0x07、0x08使用以下枚举
enum {
    RCSP_DEVICE_STATUS_ATTR_TYPE_BATTERY 								= 0,
    RCSP_DEVICE_STATUS_ATTR_TYPE_VOL 									= 1,
    RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO 								= 2,
    RCSP_DEVICE_STATUS_ATTR_TYPE_ERROR_STATS							= 3,
    RCSP_DEVICE_STATUS_ATTR_TYPE_EQ_INFO								= 4,
    RCSP_DEVICE_STATUS_ATTR_TYPE_BS_FILE_TYPE							= 5,
    RCSP_DEVICE_STATUS_ATTR_TYPE_FUNCTION_MODE							= 6,
    RCSP_DEVICE_STATUS_ATTR_TYPE_COLOR_LED_SETTING_INFO  				= 7,
    RCSP_DEVICE_STATUS_ATTR_TYPE_FMTX_FREQ								= 8,
    RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_SW							= 9,
    RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_CONNECT_STATES				= 10,
    RCSP_DEVICE_STATUS_ATTR_TYPE_HIGH_LOW_SET							= 11,
    RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO  				= 12,
    RCSP_DEVICE_STATUS_ATTR_TYPE_ANC_VOICE 								= 13,
    RCSP_DEVICE_STATUS_ATTR_TYPE_FETCH_ALL_ANC_VOICE 					= 14,
    RCSP_DEVICE_STATUS_ATTR_TYPE_PHONE_SCO_STATE_INFO  					= 15,
    RCSP_DEVICE_STATUS_ATTR_TYPE_MISC_SETTING_INFO  					= 16,
    RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO				= 17,
    RCSP_DEVICE_STATUS_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO				= 18,
    // 缺一个声卡功能19
    RCSP_DEVICE_STATUS_ATTR_TYPE_ASSISTED_HEARING      					= 20,
    RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION				= 21,
    RCSP_DEVICE_STATUS_ATTR_TYPE_AI_NO_PICK								= 22,
    RCSP_DEVICE_STATUS_ATTR_TYPE_SCENE_NOISE_REDUCTION					= 23,
    RCSP_DEVICE_STATUS_ATTR_TYPE_WIND_NOISE_DETECTION					= 24,
    RCSP_DEVICE_STATUS_ATTR_TYPE_VOICE_ENHANCEMENT_MODE					= 25,
    RCSP_DEVICE_STATUS_ATTR_TYPE_AI										= 26,
    RCSP_DEVICE_STATUS_ATTR_TYPE_1T2									= 27,
    RCSP_DEVICE_STATUS_ATTR_TYPE_MAX,
};

// 设备状态更改
bool rcsp_device_status_cmd_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 function, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
// 设备状态更改
bool rcsp_device_status_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 function, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
// 获取设备状态
u32 rcsp_device_status_get(void *priv, u8 function, u8 *data, u16 len, u8 *buf, u16 buf_size);
// 设备状态更改
void rcsp_device_status_update(u8 function, u32 mask);
// rcsp功能设置关闭
void rcsp_device_status_setting_stop(void);
// 获取当前模式
u8 rcsp_get_cur_mode(u8 app_mode);

void function_change_inform(u8 app_mode, u8 ret);

void rcsp_update_dev_state(u32 event, void *param);

#endif//__RCSP_DEVICE_STATUS_H__

