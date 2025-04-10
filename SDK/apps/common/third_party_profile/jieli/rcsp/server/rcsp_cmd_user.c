#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_cmd_user.data.bss")
#pragma data_seg(".rcsp_cmd_user.data")
#pragma const_seg(".rcsp_cmd_user.text.const")
#pragma code_seg(".rcsp_cmd_user.text")
#endif
#include "rcsp_cmd_user.h"
#include "app_config.h"
#include "ble_rcsp_server.h"

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

//*----------------------------------------------------------------------------*/
/**@brief    rcsp自定义命令数据接收处理
   @param    priv:全局rcsp结构体， OpCode:当前命令， OpCode_SN:当前的SN值， data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
void rcsp_user_cmd_recieve(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    //自定义数据接收
    rcsp_printf("%s:", __FUNCTION__);
    rcsp_put_buf(data, len);
#if 0
    ///以下是发送测试代码
    u8 test_send_buf[] = {0x04, 0x05, 0x06};
    rcsp_user_cmd_send(test_send_buf, sizeof(test_send_buf));
#endif

    JL_CMD_response_send(OpCode, JL_PRO_STATUS_SUCCESS, OpCode_SN, NULL, 0, ble_con_handle, spp_remote_addr);

}


//*----------------------------------------------------------------------------*/
/**@brief    rcsp自定义命令数据发送接口
   @param    data:数据， len:数据长度
   @return
   @note	 二次开发需要增加自定义命令，通过JL_OPCODE_CUSTOMER_USER进行扩展，
  			 不要定义这个命令以外的命令，避免后续SDK更新导致命令冲突
*/
/*----------------------------------------------------------------------------*/
JL_ERR rcsp_user_cmd_send(u8 *data, u16 len)
{
    //自定义数据接收
    rcsp_printf("%s:", __FUNCTION__);
    rcsp_put_buf(data, len);
    return JL_CMD_send(JL_OPCODE_CUSTOMER_USER, data, len, 1, 0, NULL);
}

#endif
