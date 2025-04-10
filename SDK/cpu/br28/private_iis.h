#ifndef __PRIVATE_IIS_H
#define __PRIVATE_IIS_H

#include "cpu.h"

#define PRIVATE_MESSAGE_SIZE	512			//接收消息
#define PRIVATE_IIS_CMD		0x12345678


#define PRIVATE_IIS_RX_SR	44100
#define PRIVATE_IIS_TX_SR	44100
#define PRIVATE_IIS_DMA_LEN  	(2 * 1024)
#define PRIVATE_IIS_RX_MODULE 	0			//模块0
#define PRIVATE_IIS_RX_CH		0			//通道1
#define PRIVATE_IIS_TX_MODULE 	0			//模块0
#define PRIVATE_IIS_TX_CH		0			//通道0
#define PRIVATE_IIS_TX_LINEIN_CH_IDX  (BIT(0) | BIT(1))
#define PRIVATE_IIS_TX_LINEIN_GAIN     4
#define PRIVATE_IIS_TX_LINEIN_SR	44100


//定义命令的长度为多少字节, 变量个数* 变量类型字节数
#define  CMD_LEN      4 * 4


void private_iis_rx_open(void);
u8 get_private_iis_get_msg_flag(void);
void show_private_msg(void);
void private_iis_rx_close(void);



#endif


