#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_data_respone.data.bss")
#pragma data_seg(".rcsp_data_respone.data")
#pragma const_seg(".rcsp_data_respone.text.const")
#pragma code_seg(".rcsp_data_respone.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_event.h"

////>>>>>>>>>>>>>>>设备接收到上报数据的回复
#if (RCSP_MODE)
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


void rcsp_data_respone(void *priv, u8 status, u8 CMD_OpCode, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    rcsp_printf("data_respone, %x\n", CMD_OpCode);
}
#endif//RCSP_MODE

