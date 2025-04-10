#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cfg_tool_cdc.data.bss")
#pragma data_seg(".cfg_tool_cdc.data")
#pragma const_seg(".cfg_tool_cdc.text.const")
#pragma code_seg(".cfg_tool_cdc.text")
#endif
#include "cfg_tool_cdc.h"
#include "app_config.h"
#include "cfg_tool.h"
#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif

#if TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_USB_COMM)

/**
 *	@brief 获取工具cdc最大支持的协议包大小
 */
u16 cfg_tool_cdc_rx_max_mtu()
{
    return 1024 * 4 + 22;
}


/**
 *	@brief 获取cdc的配置/调音工具相关数据
 */
void cfg_tool_data_from_cdc(u8 *buf, u32 rlen) // in irq
{
    /* printf("cfg_tool cdc rx:\n"); */
    /* put_buf(buf, rlen); */

    cfg_tool_combine_rx_data(buf, rlen);

}

#endif

