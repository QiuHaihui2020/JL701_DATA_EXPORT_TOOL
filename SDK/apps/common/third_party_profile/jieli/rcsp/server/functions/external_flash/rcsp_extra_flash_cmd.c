#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_extra_flash_cmd.data.bss")
#pragma data_seg(".rcsp_extra_flash_cmd.data")
#pragma const_seg(".rcsp_extra_flash_cmd.text.const")
#pragma code_seg(".rcsp_extra_flash_cmd.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_device_status.h"
#include "rcsp.h"
#include "rcsp_event.h"
#include "rcsp_extra_flash_cmd.h"
#include "rcsp_extra_flash_opt.h"
#include "ble_rcsp_server.h"
#include "rcsp_setting_opt.h"
#include "rcsp_manage.h"

#if (RCSP_MODE && JL_RCSP_EXTRA_FLASH_OPT)
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

/**
 * @brief 外挂flash命令发送
 *
 * @param dire 非0:命令发送；0:命令回复
 */
int rcsp_extra_flash_opt_resp(u8 dire, u8 OpCode, u8 OpCode_SN, u8 *resp_data, u16 data_len)
{
    if (OpCode) {
        if (dire) {
            JL_CMD_send(OpCode, resp_data, data_len, JL_NOT_NEED_RESPOND, 0, NULL);
        } else {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, resp_data, data_len, 0, NULL);
        }
    }
    return 0;
}

//外挂flash命令处理
int JL_rcsp_extra_flash_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len)
{
    rcsp_printf("JL_rcsp_extra_flash_cmd_resp, OpCode : %x, OpCode_SN : %x\n", OpCode, OpCode_SN);
    int ret = 0;
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ret;
    }

    switch (OpCode) {
    case JL_OPCODE_EXTRA_FLASH_OPT:	// 0x1A - 读写外部Flash命令
        ret = rcsp_extra_flash_opt(data, len, OpCode, OpCode_SN);
        if (ret < 0) {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
        } else {
            ret = 0;
        }
        break;
    case JL_OPTCODE_EXTRA_FLASH_INFO: // 0xD6 - 获取外部Flash信息
        ret = rcsp_get_extra_flash_info(priv, data);
        if (ret >= 0) {
            u16 data_len = (u16) ret;
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, data, data_len, 0, NULL);
            ret = rcsp_extra_flash_opt_start();
        } else {
            JL_CMD_response_send(OpCode, JL_PRO_STATUS_FAIL, OpCode_SN, NULL, 0, 0, NULL);
        }
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

//外挂flash不回复命令处理
int JL_rcsp_extra_flash_cmd_no_resp(void *priv, u8 OpCode, u8 *data, u16 len)
{
    rcsp_printf("JL_rcsp_extra_flash_cmd_no_resp, OpCode : %x\n", OpCode);
    int ret = 0;
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return ret;
    }
    switch (OpCode) {
    case JL_OPCODE_EXTRA_FLASH_OPT:
        ret = rcsp_extra_flash_opt(data, len, 0, 0);
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

#endif
