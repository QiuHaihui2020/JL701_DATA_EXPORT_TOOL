#ifndef __RCSP_DEVICE_FEATURE_H__
#define __RCSP_DEVICE_FEATURE_H__

#include "typedef.h"
#include "app_config.h"

enum {
    RCSP_DEVICE_FEATURE_ATTR_TYPE_PROTOCOL_VERSION = 0,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_SYS_INFO         = 1,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_EDR_ADDR         = 2,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_PLATFORM         = 3,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_FUNCTION_INFO    = 4,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_VERSION      = 5,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_SDK_TYPE         = 6,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_UBOOT_VERSION    = 7,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DOUBLE_PARITION  = 8,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_UPDATE_STATUS    = 9,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_VID_PID      = 10,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_AUTHKEY      = 11,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_PROCODE      = 12,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_MAX_MTU      = 13,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_CONNECT_BLE_ONLY = 17,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_BT_EMITTER_INFO  = 18,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_MD5_GAME_SUPPORT = 19,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_FILE_TRANSFER_INFO = 21,
    RCSP_DEVICE_FEATURE_ATTR_TYPE_MAX,
};

// le aduio mode
typedef enum {
    RCSP_LeAudioModeNone             			= 0x00,
    RCSP_LeAudioModeBig						= 0x01,
    RCSP_LeAudioModeCig						= 0x02,
} RCSP_LeAudioMode;
/**
 * @brief 设置leaudio的状态
 *
 * #param RCSP_LeAudioMode
 */
void rcsp_set_LeAudio_mode(RCSP_LeAudioMode mode);
/**
 * @brief 获取leaudio的状态
 *
 * @result RCSP_LeAudioMode
 */
RCSP_LeAudioMode rcsp_get_LeAudio_mode();

// 解析设备特征相关的rcsp数据
u32 rcsp_target_feature_parse_packet(void *priv, u8 *buf, u16 buf_size, u32 mask);

#endif//__RCSP_DEVICE_FEATURE_H__

