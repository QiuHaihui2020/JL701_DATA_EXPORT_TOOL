#ifndef __RCSP_ADV_BLUETOOTH__
#define __RCSP_ADV_BLUETOOTH__
#include "typedef.h"
#include "system/event.h"


int JL_rcsp_adv_event_handler(struct rcsp_event *rcsp);
int JL_rcsp_adv_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);

/**
 * @brief 请求手机tws操作C4
 */
u8 adv_info_device_request(u8 *buf, u16 len);

#endif
