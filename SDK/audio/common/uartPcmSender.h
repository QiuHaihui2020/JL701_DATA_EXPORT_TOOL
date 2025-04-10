#ifndef _UARTPCMSENDER_H_
#define _UARTPCMSENDER_H_
#include "system/includes.h"
#include "app_config.h"
/*
 *串口导出数据配置
 *注意IO口设置不要和普通log输出uart冲突
 */
#ifdef TCFG_DATA_EXPORT_UART_TX_PORT
#define PCM_UART1_TX_PORT			TCFG_DATA_EXPORT_UART_TX_PORT	/*数据导出发送IO*/
#else
#define PCM_UART1_TX_PORT			IO_PORT_DM	                    /*数据导出发送IO*/
#endif

#define PCM_UART1_RX_PORT			-1

/*数据导出波特率,不用修改，和接收端设置一直*/
#ifdef TCFG_DATA_EXPORT_UART_BAUDRATE
#define PCM_UART1_BAUDRATE			TCFG_DATA_EXPORT_UART_BAUDRATE
#else
#define PCM_UART1_BAUDRATE			2000000
#endif

#if ((TCFG_DEBUG_UART_ENABLE == ENABLE_THIS_MOUDLE) && (PCM_UART1_TX_PORT == TCFG_DEBUG_UART_TX_PIN))
//IO口配置冲突，请检查修改
#error "PCM_UART1_TX_PORT conflict with TCFG_DEBUG_UART_TX_PIN"
#endif/*PCM_UART1_TX_PORT*/

void uartSendInit();                        //串口发数初始化
void uartSendData(void *buf, u16 len); 		//发送数据的接口
void uartSendExit();

#endif /*_UARTPCMSENDER_H_*/
