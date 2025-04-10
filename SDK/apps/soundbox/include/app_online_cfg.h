#ifndef ONLINE_CONFIG_H
#define ONLINE_CONFIG_H

#define DEVICE_EVENT_FROM_CI_UART	(('C' << 24) | ('I' << 16) | ('U' << 8) | '\0')
#define DEVICE_EVENT_FROM_CI_TWS 	(('C' << 24) | ('I' << 16) | ('T' << 8) | '\0')

#define CI_UART         0
#define CI_TWS          1

void ci_data_rx_handler(u8 type);
u32 eq_cfg_sync(u8 priority);
void ci_uart_write(u8 *buf, u16 len);

/**
 *	@brief 获取工具串口最大支持的协议包大小
 */
u16 get_ci_rx_size();

#endif

