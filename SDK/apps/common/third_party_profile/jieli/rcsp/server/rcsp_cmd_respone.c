#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_cmd_respone.data.bss")
#pragma data_seg(".rcsp_cmd_respone.data")
#pragma const_seg(".rcsp_cmd_respone.text.const")
#pragma code_seg(".rcsp_cmd_respone.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "ble_rcsp_server.h"
#include "rcsp_event.h"
#include "rcsp_update.h"
#include "file_transfer.h"
#if	(defined CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE)
#include "asm/debug_record.h"
#endif

#if RCSP_MODE

/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

///>>>>>>>>>>>>>>>设备接收到上报给APP端命令的回复
void rcsp_cmd_respone(void *priv, u8 OpCode, u8 status, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("cmd_respone:%x\n", OpCode);
#if RCSP_UPDATE_EN
    if (0 == JL_rcsp_update_cmd_receive_resp(priv, OpCode, status, data, len)) {
        return;
    }
#endif

    switch (OpCode) {
#if (TCFG_DEV_MANAGER_ENABLE && RCSP_FILE_OPT)
    case JL_OPCODE_FILE_TRANSFER_END:
        rcsp_file_transfer_download_end(status, data, len);
        break;
    case JL_OPCODE_FILE_TRANSFER_CANCEL:
        rcsp_file_transfer_download_active_cancel_response(status, data, len);
        break;
    case JL_OPCODE_FILE_RENAME:
        rcsp_file_transfer_file_rename(status, data, len);
        break;
#endif
#if	(defined CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE)
    case JL_OPCODE_REQUEST_EXCEPTION_INFO:
        if (status == JL_PRO_STATUS_SUCCESS) {
            user_debug_record_info_clear();
        }
        break;
#endif
    default:
        break;
    }
}

#endif
