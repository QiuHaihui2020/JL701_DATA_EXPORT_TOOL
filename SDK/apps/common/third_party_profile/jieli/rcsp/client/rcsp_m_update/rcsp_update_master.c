#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_update_master.data.bss")
#pragma data_seg(".rcsp_update_master.data")
#pragma const_seg(".rcsp_update_master.text.const")
#pragma code_seg(".rcsp_update_master.text")
#endif
#include "rcsp_update_master.h"
#include "app_config.h"
#include "JL_rcsp_protocol.h"
#include "ble_rcsp_server.h"

#if RCSP_MODE && RCSP_BLE_MASTER && RCSP_UPDATE_EN

#include "fs/fs.h"

#define RCSP_DEBUG_EN

#ifdef  RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         printf_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

struct rcsp_update_m {
    bool isDualBankUpdate;		// 是否双备份
    bool isForceUpdate;			// 是否强制升级
    bool needDownloadLoader;	// 需要下载Loader
    bool canUpdate;				// 获取设备信息后才可升级
    void *otaFd;					// 升级文件文件句柄
    char *otaFilePath;			// 升级文件路径
    float otaProgress;			// 升级进度
    u32 otaSent;				// 已发送数据大小
    u32 otaFileSize;			// 升级文件尺寸
    bool isRelink;				// 是否已经在回连流程
};
static struct rcsp_update_m _rcsp_update_m = {0};
#define __this (&_rcsp_update_m)

/**
 * @brief 设置设备不能升级
 */
void rcsp_set_device_cant_update()
{
    __this->canUpdate = false;
}

/**
 * @brief 获取升级设备信息，升级前需要调用一次，用于获取设备当前的升级状态
 */
void rcsp_get_device_feature()
{
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    u8 send_buf[5] = {0xff, 0xff, 0xff, 0xff, 0x03};	//0:Android;1:iOS;2:小程序;3:固件
    JL_CMD_send(JL_OPCODE_GET_TARGET_FEATURE, send_buf, sizeof(send_buf), JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief 获取升级设备信息后传入设备信息
 *
 * @param isDualBankUpdate
 * @param isForceUpdate
 * @param needDownloadLoader
 */
void rcsp_get_device_feature_response(bool isDualBankUpdate, bool isForceUpdate, bool needDownloadLoader)
{
    rcsp_printf("%s, %d, isDualBankUpdate:%d, isForceUpdate:%d, needDownloadLoader:%d\n", __FUNCTION__, __LINE__, isDualBankUpdate, isForceUpdate, needDownloadLoader);
    __this->isDualBankUpdate = isDualBankUpdate;
    __this->isForceUpdate = isForceUpdate;
    __this->needDownloadLoader = needDownloadLoader;
    __this->canUpdate = true;
    if (__this->isForceUpdate && ((__this->otaFd != NULL) || (__this->otaFilePath != NULL))) {
        // 强制升级
        rcsp_ota_with_file_path(__this->otaFilePath, __this->otaFd);
    }
}

/**
 * @brief 升级流程信息回调
 *
 * @param status
 * @param progress
 */
__attribute__((weak))
void rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatus status, float progress)
{
    rcsp_printf("rcsp_update_m status:%x, progress:%.1f%%\n", status, progress * 100.0f);
}

/**
 * @brief e1获取offset与size
 */
static void _rcsp_ota_e1_get_offset_and_size()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    JL_CMD_send(JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET, NULL, 0, JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief e2是否允许升级
 *
 * @param markData
 * @param markDataSize
 */
static void _rcsp_ota_e2_ask_can_update(u8 *markData, u16 markDataSize)
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    JL_CMD_send(JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE, markData, markDataSize, JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief 升级第一步函数，e1、e2命令流程相关
 */
static void _rcsp_ota_first_step()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);

    _rcsp_ota_e1_get_offset_and_size();
}

/**
 * @brief e3进入升级模式
 */
static void _rcsp_ota_e3_enter_update_mode()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    JL_CMD_send(JL_OPCODE_ENTER_UPDATE_MODE, NULL, 0, JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief e4退出升级模式(no used in update progress)
 */
static void _rcsp_ota_e4_exit_update_mode()
{
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    __this->canUpdate = false;
    JL_CMD_send(JL_OPCODE_EXIT_UPDATE_MODE, NULL, 0, JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief e5发送升级数据
 *
 * @param opCode_SN
 * @param data
 * @param len
 */
static void _rcsp_ota_e5_response_update_data(u8 opCode_SN, u8 *data, u16 len)
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    JL_CMD_response_send(JL_OPCODE_SEND_FW_UPDATE_BLOCK, JL_PRO_STATUS_SUCCESS, opCode_SN, data, len, 0, NULL);
}
/**
 * @brief e5发送升级数据失败
 *
 * @param opCode_SN
 */
static void _rcsp_ota_e5_response_update_data_error(u8 opCode_SN)
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    JL_CMD_response_send(JL_OPCODE_SEND_FW_UPDATE_BLOCK, JL_PRO_STATUS_FAIL, opCode_SN, NULL, 0, 0, NULL);
}

/**
 * @brief e6查询设备升级状态
 */
static void _rcsp_ota_e6_query_device_upadte_status()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    JL_CMD_send(JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS, NULL, 0, JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief e7强制设备重启
 */
static void _rcsp_ota_e7_reboot_device()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    u8 send_buf[1] = {0x00};	// 0x00，重启设备
    JL_CMD_send(JL_OPCODE_SET_DEVICE_REBOOT, send_buf, sizeof(send_buf), JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief 切换升级设备通讯方式到ble，用于单备份回连
 */
static void _rcsp_switch_device_commucation_mode_to_ble()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    u8 send_buf[2] = {0x00, 0x01};	//0x00：ble通讯，0x01：根据广播包信息回连
    JL_CMD_send(JL_OPCODE_SWITCH_DEVICE, send_buf, sizeof(send_buf), JL_NEED_RESPOND, 0, NULL);
}

/**
 * @brief 升级第二步函数，e3、e5、e6命令流程相关
 */
static void _rcsp_ota_second_step()
{
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update!\n", __FUNCTION__, __LINE__);
        return;
    }
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);

    _rcsp_ota_e3_enter_update_mode();
}

/**
 * @brief seek升级文件
 *
 * @param offset
 * @result success:0
 */
static int _rcsp_get_update_file_data_seek(u32 offset)
{
    if (!__this->otaFd) {
        rcsp_printf("%s, no otaFd\n", __FUNCTION__);
        if (__this->otaFilePath) {
            __this->otaFd = fopen(__this->otaFilePath, "r");
            if (!__this->otaFd) {
                rcsp_printf("%s, file open err!\n", __FUNCTION__);
                return -1;
            }
        } else {
            rcsp_printf("%s, no otaFilePath\n", __FUNCTION__);
            return -1;
        }
    }
    int ret = fseek(__this->otaFd, offset, SEEK_SET);
    return ret;
}

/**
 * @brief 获取升级文件数据
 *
 * @param buff
 * @param len
 * @result buff's length
 */
u16 _rcsp_get_update_file_data_read(u8 *buff, u16 len)
{
    if (!__this->otaFd) {
        rcsp_printf("%s, no otaFd\n", __FUNCTION__);
        return (u16) - 1;
    }
    len = fread(__this->otaFd, buff, len);
    return len;
}

/**
 * @brief 关闭升级变量
 */
static void _rcsp_release_update_var()
{
    rcsp_printf("%s, %d\n", __FUNCTION__, __LINE__);
    if (__this->otaFd != NULL) {
        fclose(__this->otaFd);
        __this->otaFd = NULL;
    }
    __this->otaFilePath = NULL;
}

/**
 * @brief 升级命令处理
 *
 * @param priv
 * @param opCode
 * @param status
 * @param opCode_SN -1:为收到命令需回复
 * @param data
 * @param len
 * @result success:0
 */
int rcsp_m_update_cmd_handle(void *priv, u8 opCode, u8 status, u8 opCode_SN, u8 *data, u16 len)
{
    int ret = 0;
    rcsp_printf("%s, opCode:%x\n", __FUNCTION__, opCode);
    switch (opCode) {
    case JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET:
        if (status == JL_PRO_STATUS_SUCCESS) {
            if (data == NULL || len == 0) {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusCommandFail, 0.0f);
                _rcsp_release_update_var();
                break;
            }
            u32 seek = READ_BIG_U32(data);
            u16 markDataSize = READ_BIG_U16(data + 4);
            u8 *markData = NULL;
            u16 u_len = markDataSize;
            if (markDataSize != 0) {
                markData = malloc(markDataSize);
                if (markData == NULL) {
                    rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET, malloc fail!\n");
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFail, 0.0f);
                    _rcsp_release_update_var();
                    break;
                }
            }
            if (!(seek == 0 && markDataSize == 0)) {
                if (markData == NULL) {
                    rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET, markData NULL!\n");
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFail, 0.0f);
                    _rcsp_release_update_var();
                    break;
                }
                _rcsp_get_update_file_data_seek(seek);
                u_len = _rcsp_get_update_file_data_read(markData, markDataSize);
            } else {
                markDataSize = 1;
                markData = malloc(markDataSize);
                if (markData == NULL) {
                    rcsp_printf("JL_OPCODE_GET_DEVICE_UPDATE_FILE_INFO_OFFSET, markData NULL!\n");
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFail, 0.0f);
                    _rcsp_release_update_var();
                    break;
                }
                memset(markData, 0x00, markDataSize);
                u_len = markDataSize;
            }
            _rcsp_ota_e2_ask_can_update(markData, u_len);
            if (markData != NULL) {
                free(markData);
            }
        } else {
            rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusCommandFail, 0.0f);
            _rcsp_release_update_var();
        }
        break;
    case JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE:
        if ((status == JL_PRO_STATUS_SUCCESS) && (len > 0)) {
            u8 ret = data[0];
            if (ret == 0x00) {
                rcsp_printf("JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE, result: device can update!\n");
                if (!__this->isDualBankUpdate) {
                    // 单备份
                    if (!__this->needDownloadLoader) {
                        rcsp_printf("JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE, result: device can update! no need to download loader!\n");
                        if (!__this->isForceUpdate) {
                            __this->isRelink = true;

                            // 通知使用ble通讯方式
                            _rcsp_switch_device_commucation_mode_to_ble();

                        } else {
                            _rcsp_ota_second_step();
                        }
                    } else {
                        rcsp_printf("JL_OPCODE_INQUIRE_DEVICE_IF_CAN_UPDATE, result: device can update! need to download loader!\n");
                    }
                } else {
                    // 双备份
                    _rcsp_ota_second_step();
                }
                break;
            } else if (ret == 0x02) {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusLowPower, 0.0f);
            } else if (ret == 0x03) {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailSameVersion, 0.0f);
            } else if (ret == 0x04) {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailTWSDisconnect, 0.0f);
            } else if (ret == 0x05) {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailNotInBin, 0.0f);
            } else {
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusUnknown, 0.0f);
            }
        } else {
            rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusCommandFail, 0.0f);
        }
        _rcsp_release_update_var();
        break;
    case JL_OPCODE_ENTER_UPDATE_MODE:
        if ((status == JL_PRO_STATUS_SUCCESS) && (len > 0)) {
            u8 ret = data[0];
            if (ret == 0x00) {
                rcsp_printf("JL_OPCODE_ENTER_UPDATE_MODE, success!\n");
                break;
            } else {
                rcsp_printf("JL_OPCODE_ENTER_UPDATE_MODE, no!\n");
                break;
            }
        } else {
            rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusCommandFail, 0.0f);
        }
        _rcsp_release_update_var();
        break;
    case JL_OPCODE_EXIT_UPDATE_MODE:
        // dont need to implement
        break;
    case JL_OPCODE_SEND_FW_UPDATE_BLOCK:
        // 从机拉取升级数据
        u32 seek = READ_BIG_U32(data);
        u16 dataLen = READ_BIG_U16(data + 4);
        if (seek == 0 && dataLen == 0) {
            // 查看升级状态
            JL_CMD_response_send(JL_OPCODE_SEND_FW_UPDATE_BLOCK, JL_PRO_STATUS_SUCCESS, opCode_SN, NULL, 0, 0, NULL);

            _rcsp_ota_e6_query_device_upadte_status();
        } else {
            // 升级进度
            __this->otaSent = __this->otaSent + dataLen;
            float progress = 0.0;
            if (__this->otaFileSize != 0) {
                progress = __this->otaSent / (double)__this->otaFileSize;
            }

            if (!__this->needDownloadLoader) {
                // 下载app code进度
                rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK, download app:%.1f %d/%d\n", progress * 100.0f, __this->otaSent, __this->otaFileSize);
                if (progress > __this->otaProgress) {
                    __this->otaProgress = progress;
                }
                if (__this->otaProgress > 1.0f) {
                    __this->otaProgress = 1.0f;
                }
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusUpgrading, __this->otaProgress);
            } else {
                // 下载loader进度
                rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK, download loader:%.1f %d/%d\n", progress * 100.0f, __this->otaSent, __this->otaFileSize);
                __this->otaProgress = 0;
                if (progress > __this->otaProgress) {
                    __this->otaProgress = progress;
                }
                if (__this->otaProgress > 1.0f) {
                    __this->otaProgress = 1.0f;
                }
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusUpgrading, __this->otaProgress);
            }
            rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK, seek:%d, dataLen:%d\n", seek, dataLen);
            if (seek < 0) {
                rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK, seek error!\n");
                _rcsp_ota_e5_response_update_data_error(opCode_SN);
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusSeekFail, 0.0f);
            }
            u8 *updateData = malloc(dataLen);
            if (updateData == NULL) {
                rcsp_printf("JL_OPCODE_SEND_FW_UPDATE_BLOCK, malloc fail!\n");
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFail, 0.0f);
                _rcsp_release_update_var();
                break;
            }
            _rcsp_get_update_file_data_seek(seek);
            u16 u_len = _rcsp_get_update_file_data_read(updateData, dataLen);
            _rcsp_ota_e5_response_update_data(opCode_SN, updateData, u_len);
            if (updateData != NULL) {
                free(updateData);
            }
        }
        break;
    case JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS:
        if ((status == JL_PRO_STATUS_SUCCESS) && (len > 0)) {
            u8 ret = data[0];
            rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS, result code:%x, %x\n", ret, data[0]);
            if (ret == 0x00) {
                // 升级完成！
                rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS, update success!\n");
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusSuccess, 1.0f);

                // 重启设备
                _rcsp_ota_e7_reboot_device();

                _rcsp_release_update_var();

                extern void setRcspConnectBleAddr(u8 * addr);
                setRcspConnectBleAddr(NULL);
            } else if (ret == 0x80) {
                // 下载loader成功
                rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS, loader is downloaded success!\n");
                __this->isRelink = true;
                rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusPrepared, 1.0f);
                // 通知使用ble通讯方式，准备进行升级回连
                _rcsp_switch_device_commucation_mode_to_ble();
                break;
            } else {
                rcsp_printf("JL_OPCODE_GET_DEVICE_REFRESH_FW_STATUS, fail! result code:%x\n", ret);
                if (ret == 0x01) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailVerification, 0.0f);
                } else if (ret == 0x02) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailCompletely, 0.0f);
                } else if (ret == 0x03) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailKey, 0.0f);
                } else if (ret == 0x04) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailErrorFile, 0.0f);
                } else if (ret == 0x05) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailUboot, 0.0f);
                } else if (ret == 0x06) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailLenght, 0.0f);
                } else if (ret == 0x07) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailFlash, 0.0f);
                } else if (ret == 0x08) {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFailCmdTimeout, 0.0f);
                } else {
                    rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusFail, 0.0f);
                }
            }
        } else {
            rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusCommandFail, 0.0f);
        }
        _rcsp_release_update_var();
        break;
    case JL_OPCODE_SET_DEVICE_REBOOT:
        if (status == JL_PRO_STATUS_SUCCESS) {
            rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusReboot, 1.0f);
            __this->canUpdate = false;
        }
        break;
    case JL_OPCODE_NOTIFY_UPDATE_CONENT_SIZE:
        if (len >= 4) {
            __this->otaFileSize = READ_BIG_U32(data);
            __this->otaSent = 0;
        }
        if (len == 8) {
            __this->otaSent = READ_BIG_U32(data + 4);
        }

        JL_CMD_response_send(JL_OPCODE_NOTIFY_UPDATE_CONENT_SIZE, JL_PRO_STATUS_SUCCESS, opCode_SN, NULL, 0, 0, NULL);
        break;
    case JL_OPCODE_SWITCH_DEVICE:
        __this->canUpdate = false;
        break;
    default:
        ret = 1;
        break;
    }
    return ret;
}

/**
 * @brief 开始升级函数
 *
 * @param otaFilePath ota升级文件路径
 * @param otaFd 文件句柄
 * @result 待升级设备数量，如果>0则可以进行升级
 */
void rcsp_ota_with_file_path(char *otaFilePath, void *otaFd)
{
    rcsp_printf("%s, %d, otaFilePath:%s, otaFd:%x\n", __FUNCTION__, __LINE__, otaFilePath, otaFd);
    if (!__this->canUpdate) {
        rcsp_printf("%s, %d, can't update! Please connect device, and call rcsp_get_device_feature Function!\n", __FUNCTION__, __LINE__);
        return;
    }
    __this->otaFilePath = otaFilePath;
    __this->otaFd = otaFd;

    __this->otaSent = 0;
    __this->otaFileSize = 0;
    __this->otaProgress = 0.0f;

    if (otaFilePath == NULL || otaFd == NULL) {
        rcsp_progress_status_with_status_and_progress(RCSPOTAProgressStatusDataIsNull, 0.0f);
        return;
    }

    if (!__this->isDualBankUpdate) {
        // 单备份升级流程
        rcsp_printf("single bank update!");
        if (!__this->isForceUpdate) {
            // 正常升级流程
            rcsp_printf("single bank update, normal update!");
            _rcsp_ota_first_step();
        } else {
            // 强制升级流程
            rcsp_printf("single bank update, force update!");
            if (__this->isRelink) {
                rcsp_printf("ota second step!");
                _rcsp_ota_second_step();
            } else {
                rcsp_printf("ota first step, force update!");
                _rcsp_ota_first_step();
            }
        }
    } else {
        // 双备份升级流程
        rcsp_printf("dual bank update!");
        _rcsp_ota_first_step();
    }

}


#endif // RCSP_MODE && RCSP_BLE_MASTER && RCSP_UPDATE_EN

