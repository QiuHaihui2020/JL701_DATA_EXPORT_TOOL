#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_c_cmd_response.data.bss")
#pragma data_seg(".rcsp_c_cmd_response.data")
#pragma const_seg(".rcsp_c_cmd_response.text.const")
#pragma code_seg(".rcsp_c_cmd_response.text")
#endif
#include "rcsp_config.h"
#include "ble_rcsp_server.h"
#include "app_config.h"
#include "rcsp_update_master.h"
#include "rcsp_device_feature.h"
#include "JL_rcsp_protocol.h"

///>>>>>>>>>>>>>收到发来的命令回复

#if RCSP_MODE && RCSP_BLE_MASTER

/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                putchar(x)
#define rcsp_printf                    printf
#define rcsp_printf_buf(x,len)         put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_printf_buf(...)
#endif

static void _rcsp_get_device_feature_callback(void *priv, u8 opCode, u8 *data, u16 len)
{
    rcsp_printf("%s, %x, len:%d\n", __FUNCTION__, opCode, len);

    if ((data != NULL) && (len > 0)) {
        // 分解设备信息
        // 暂时只提取升级相关的信息
        bool isDualBankUpdate = false;		// 是否双备份
        bool isForceUpdate = false;			// 是否强制升级
        bool needDownloadLoader = false;	// 需要下载Loader

        for (int i = 0; i < len;) {
            // LTV格式
            u8 length = data[i];
            u8 type = data[i + 1];
            if (type == RCSP_DEVICE_FEATURE_ATTR_TYPE_DOUBLE_PARITION) {
                isDualBankUpdate = data[i + 2];
                needDownloadLoader = data[i + 3];
            } else if (type == RCSP_DEVICE_FEATURE_ATTR_TYPE_UPDATE_STATUS) {
                isForceUpdate = data[i + 2];
            }
            i += (length + 1);
        }
        // 传入设备升级信息
        rcsp_get_device_feature_response(isDualBankUpdate, isForceUpdate, needDownloadLoader);
    }
}

void rcsp_c_cmd_response(void *priv, u8 opCode, u8 status, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("%s, %x\n", __FUNCTION__, opCode);
    switch (opCode) {
    case JL_OPCODE_GET_TARGET_FEATURE:
        _rcsp_get_device_feature_callback(priv, opCode, data, len);
        break;
    case JL_OPCODE_SYS_INFO_GET:
        break;
        /* case JL_OPCODE_SWITCH_DEVICE: */
        /* 	// 默认强制使用根据广播包信息回连，所以不处理数据 */
        /* 	rcsp_printf_buf(data, len); */
        break;
    default:
#if RCSP_UPDATE_EN
        if (0 == rcsp_m_update_cmd_handle(priv, opCode, status, -1, data, len)) {
            break;
        }
#endif
        break;
    }
}
#endif // RCSP_MODE && RCSP_BLE_MASTER

