#ifndef __RCSP_UPDATE_MASTER_H__
#define __RCSP_UPDATE_MASTER_H__

#include "system/includes.h"

// 升级状态
typedef enum {
    RCSPOTAProgressStatusSuccess             = 0x00, //OTA升级成功
    RCSPOTAProgressStatusFail                = 0x01, //OTA升级失败
    RCSPOTAProgressStatusDataIsNull          = 0x02, //OTA升级数据为空
    RCSPOTAProgressStatusCommandFail         = 0x03, //OTA指令失败
    RCSPOTAProgressStatusSeekFail            = 0x04, //OTA标示偏移查找失败
    RCSPOTAProgressStatusInfoFail            = 0x05, //OTA升级固件信息错误
    RCSPOTAProgressStatusLowPower            = 0x06, //OTA升级设备电压低
    RCSPOTAProgressStatusEnterFail           = 0x07, //未能进入OTA升级模式
    RCSPOTAProgressStatusUpgrading           = 0x08, //OTA升级中
    RCSPOTAProgressStatusReconnect           = 0x09, //OTA需重连设备(uuid方式)
    RCSPOTAProgressStatusReboot              = 0x0a, //OTA需设备重启
    RCSPOTAProgressStatusPreparing           = 0x0b, //OTA准备中
    RCSPOTAProgressStatusPrepared            = 0x0f, //OTA准备完成
    RCSPOTAProgressStatusFailVerification    = 0xf1, //升级数据校验失败
    RCSPOTAProgressStatusFailCompletely      = 0xf2, //升级失败
    RCSPOTAProgressStatusFailKey             = 0xf3, //升级数据校验失败
    RCSPOTAProgressStatusFailErrorFile       = 0xf4, //升级文件出错
    RCSPOTAProgressStatusFailUboot           = 0xf5, //uboot不匹配
    RCSPOTAProgressStatusFailLenght          = 0xf6, //升级过程长度出错
    RCSPOTAProgressStatusFailFlash           = 0xf7, //升级过程flash读写失败
    RCSPOTAProgressStatusFailCmdTimeout      = 0xf8, //升级过程指令超时
    RCSPOTAProgressStatusFailSameVersion     = 0xf9, //相同版本
    RCSPOTAProgressStatusFailTWSDisconnect   = 0xfa, //TWS耳机未连接
    RCSPOTAProgressStatusFailNotInBin        = 0xfb, //耳机未在充电仓
    RCSPOTAProgressStatusReconnectWithMacAddr = 0xfc, //OTA需重连设备(mac方式)
    RCSPOTAProgressStatusUnknown,                    //OTA未知错误
} RCSPOTAProgressStatus;

/**
 * @brief 设置设备不能升级
 */
void rcsp_set_device_cant_update();

/**
 * @brief 获取升级设备信息，升级前需要调用一次，用于获取设备当前的升级状态
 */
void rcsp_get_device_feature();

/**
 * @brief 获取升级设备信息后传入设备信息
 *
 * @param isDualBankUpdate
 * @param isForceUpdate
 * @param needDownloadLoader
 */
void rcsp_get_device_feature_response(bool isDualBankUpdate, bool isForceUpdate, bool needDownloadLoader);

/**
 * @brief 升级流程信息回调
 *
 * @param status
 * @param progress
 */
__attribute__((weak))
void rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatus status, float progress);

/**
 * @brief 开始升级函数
 *
 * @param otaFilePath ota升级文件路径
 * @param otaFd 文件句柄
 * @result 待升级设备数量，如果>0则可以进行升级
 */
void rcsp_ota_with_file_path(char *otaFilePath, void *otaFd);

/**
 * @brief rcsp命令接收
 *
 * @param priv
 * @param opCode
 * @param status
 * @param opCode_SN -1:为收到命令需回复
 * @param data
 * @param len
 * @result ret success:0
 */
int rcsp_m_update_cmd_handle(void *priv, u8 opCode, u8 status, u8 opCode_SN, u8 *data, u16 len);

#endif // #define __RCSP_UPDATE_MASTER_H__

