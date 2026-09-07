/*
*********************************************************************************
* CRC 算法说明 (移植到其他平台时必须一致)
*
* 本文件下面所有接口用到的 CRC16, 在 SDK 里是 crc.h 的 CRC16(), 底层走硬件
* CRC 外设; 中断上下文里外设可能被抢占, 会退化成软件的 crc16_xmodem()。
* 两者结果相同, 就是标准的 CRC-16/XMODEM:
*
*     Width       : 16
*     Polynomial  : 0x1021        (x^16 + x^12 + x^5 + 1)
*     Init        : 0x0000
*     RefIn       : false         (输入字节不反转, 高位先入)
*     RefOut      : false         (结果不反转)
*     XorOut      : 0x0000        (无末异或)
*     Check       : CRC16("123456789", 9) == 0x31C3
*
* 别名: CRC-16/ACORN, CRC-16/LTE, CRC-16/V-41-MSB。注意不是 CRC-16/CCITT-FALSE
* (那个 Init = 0xFFFF), 也不是 CRC-16/MODBUS/IBM。移植前先用上面的 Check 值
* 自测一遍, 这一步省不掉 —— CRC 配错的现象是接收端 100% 丢帧, 和硬件不通、
* 波特率不对完全一样, 极难区分。
*
* 移植参考实现 (无表, 逐位; 要提速再换 256 项查表):
*
*     u16 crc16_xmodem(const void *buf, u32 len)
*     {
*         const u8 *p = (const u8 *)buf;
*         u16 crc = 0x0000;
*         while (len--) {
*             crc ^= (u16)(*p++) << 8;
*             for (u8 i = 0; i < 8; i++) {
*                 crc = (crc & 0x8000) ? (u16)((crc << 1) ^ 0x1021) : (u16)(crc << 1);
*             }
*         }
*         return crc;
*     }
*
* 三处用到 CRC, 覆盖范围和存放位置都不同, 别混:
*
*   1) V1 传输帧尾  aec_uart_write()
*      帧 = [ch0 len][ch1 len]...[chN-1 len][CRC16 2B][填充 2B]
*      覆盖 len * nch 字节净载荷; 以 u16 形式存在净载荷之后,
*      即 uartSendBuf[pcm_packet_size - 2] (u16 * 下标, 字节偏移 len * nch)。
*
*   2) V2 数据包头  aec_uart_fill_v2()
*      header.crc = CRC16(data, len), 只覆盖本包数据, 不含 20 字节包头。
*      接收端靠它 + magic 找包边界, 所以这个必须对, 否则嗅探不出 V2。
*
*   3) V2 传输帧尾  aec_uart_write_v2()
*      帧 = [包流净载荷][CRC16 2B][填充 2B]
*      覆盖净载荷字节数(本实现是 512); 存在 uartSendBuf1[净载荷 / 2]。
*
* 共同点: CRC 都按本机字节序当 u16 写入(小端平台即小端), 不做字节序转换;
* 帧尾那 2 字节填充不参与校验, 内容随意。
*
* 净载荷长度必须是偶数 —— 接收端是按 u16 下标去取帧尾 CRC 的。
*********************************************************************************
*/
#ifndef _AEC_UART_DEBUG_H_
#define _AEC_UART_DEBUG_H_

#include "generic/typedef.h"

/*
*********************************************************
*                  aec_uart_open
* Description: 打开数据写卡接口
* Arguments  : nch 总通道数，single_size 单个通道的数据大小
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_open(u8 nch, u16 single_size);

/*
*********************************************************
*                  aec_uart_init
* Description: 数据写卡初始化
* Arguments  : None.
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_init(void);

/*
*********************************************************
*                  aec_uart_fill
* Description: 填写对应通道的数据
* Arguments  : ch 通道号，buf 数据地址，size 数据大小
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_fill(u8 ch, void *buf, u16 size);

/*
*********************************************************
*                  aec_uart_write
* Description: 将写入通话的数据写入串口buffer
* Arguments  : None.
* Return     : None.
* Note(s)    : None.
*********************************************************
*/
void aec_uart_write(void);

/*
*********************************************************
*                  aec_uart_close
* Description: 关闭数据写卡
* Arguments  : None.
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_close(void);


/*
*********************************************************
*                  aec_uart_open_v2
* Description: 打开数据写卡接口
* Arguments  : nch 总通道数，single_size 单次运行发送的数据大小
* Return     : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************
*/
int aec_uart_open_v2(u8 nch, u16 single_size);

/* Description: 填写对应通道的数据
* Arguments  : ch 通道号，buf 数据地址，size 数据大小*/
int aec_uart_fill_v2(u8 ch, void *buf, u16 size);

/*将数据写入串口buffer*/
void aec_uart_write_v2(void);

/*关闭数据写卡*/
int aec_uart_close_v2(void);
#endif/*_AEC_UART_DEBUG_H_*/
