/**@file        usb_config.h
  * @brief      usb_config配置头文件
  * @details    功能函数声明
  * @author     jieli
  * @date       2021-8-1
  * @version    V1.0
  * @copyright  Copyright(c)2010-2021   珠海市杰理科技股份有限公司
  *********************************************************
  * @attention
  * 硬件平台：AC695N
  * SDK版本：AC695N_V1.0.0_SDK
  * @修改日志：
  * <table>
  * <tr><th>Date        <th>Version     <th>Author      <th>Description
  * <tr><td>2021-8-1    <td>1.0         <td>jieli       <td>创建初始版本
  * </table>
  *
  *********************************************************
  */
#ifndef  __USB_CONFIG_H__
#define  __USB_CONFIG_H__

#include "typedef.h"
#include "usb/usb.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_host.h"

/**@brief   USB主机模式配置
  * @param[in]  usb_id USB的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_host_config(usb_id);
  * @encode
  */
void usb_host_config(usb_dev usb_id);

/**@brief   USB主机模式释放
  * @param[in]  usb_id USB的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_host_free(usb_id);
  * @encode
  */
void usb_host_free(usb_dev usb_id);

/**@brief   USB主机模式分配端点DMA BUFFER
  * @param[in]  usb_id USB的id号
  * @param[in]  ep 端点号
  * @param[in]  dma_size 数据长度
  * @return     分配的dma buffer
  * @par    示例：
  * @code
  * usb_h_alloc_ep_buffer(usb_id , ep, 16);
  * @encode
  */
void *usb_h_alloc_ep_buffer(const usb_dev usb_id, u32 ep, u32 dma_size);

/**@brief   USB主机模式释放端点的DMA BUFFER
  * @param[in]  usb_id USB的id号
  * @param[in]  buf 需要释放的buffer
  * @return     无
  * @par    示例：
  * @code
  * usb_h_free_ep_buffer(usb_id , buf);
  * @encode
  */
void usb_h_free_ep_buffer(const usb_dev usb_id, void *buf);

/**@brief   USB主机模式中断注册
  * @param[in]  usb_id USB的id号
  * @param[in]  priority 优先级
  * @param[in]  cpu_id cpu的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_h_isr_reg(usb_id , ep);
  * @encode
  */
void usb_h_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id);

/**@brief   USB从机模式中断注册
  * @param[in]  usb_id USB的id号
  * @param[in]  priority 优先级
  * @param[in]  cpu_id cpu的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_g_isr_reg(usb_id , ep);
  * @encode
  */
void usb_g_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id);



/**@brief   USB SOF中断注册
  * @param[in]  usb_id USB的id号
  * @param[in]  priority 优先级
  * @param[in]  cpu_id cpu的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_sof_isr_reg(usb_id , ep);
  * @encode
  */
void usb_sof_isr_reg(const usb_dev usb_id, u8 priority, u8 cpu_id);

/**@brief   分配端点的DMA BUFFER
  * @param[in]  usb_id USB的id号
  * @param[in]  ep 端点号
  * @param[in]  dma_size 数据长度
  * @return     分配的dma buffer
  * @par    示例：
  * @code
  * usb_alloc_ep_dmabuffer(usb_id , ep , size);
  * @encode
  */
void *usb_alloc_ep_dmabuffer(const usb_dev usb_id, u32 ep, u32 dma_size);

/**@brief   释放端点的DMA BUFFER
  * @param[in]  usb_id USB的id号
  * @param[in]  buf 需要释放的buffer
  * @return     无
  * @par    示例：
  * @code
  * usb_free_ep_dmabuffer(usb_id, buf);
  * @encode
  */
void usb_free_ep_dmabuffer(const usb_dev usb_id, void *buf);

/**@brief   USB从机初始化配置
  * @param[in]  usb_id USB的id号
  * @return     0:成功
  * @par    示例：
  * @code
  * usb_config(usb_id);
  * @encode
  */
u32 usb_config(const usb_dev usb_id);

/**@brief   USB从机释放
  * @param[in]  usb_id USB的id号
  * @return     0:成功
  * @par    示例：
  * @code
  * usb_release(usb_id);
  * @encode
  */
u32 usb_release(const usb_dev usb_id);

void usb_phy_resume(const usb_dev usb_id);

void usb_phy_suspend(const usb_dev usb_id);

u32  usb_get_suspend_resume_status(const usb_dev usb_id);//返回0:失败   返回1:成功

#endif  /*USB_CONFIG_H*/
