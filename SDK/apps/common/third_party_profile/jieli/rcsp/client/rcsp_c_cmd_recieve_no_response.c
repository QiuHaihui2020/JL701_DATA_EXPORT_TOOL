#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_c_cmd_recieve_no_response.data.bss")
#pragma data_seg(".rcsp_c_cmd_recieve_no_response.data")
#pragma const_seg(".rcsp_c_cmd_recieve_no_response.text.const")
#pragma code_seg(".rcsp_c_cmd_recieve_no_response.text")
#endif
#include "rcsp_config.h"
#include "ble_rcsp_server.h"
#include "app_config.h"
#include "JL_rcsp_protocol.h"

///>>>>>>>>>>>>>收到发来的命令处理，不需要回复

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

void rcsp_c_cmd_recieve_no_response(void *priv, u8 opCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("%s, %x\n", __FUNCTION__, opCode);
    switch (opCode) {
    default:
        break;
    }
}
#endif // RCSP_MODE && RCSP_BLE_MASTER

