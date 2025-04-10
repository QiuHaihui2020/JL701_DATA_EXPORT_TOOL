#ifndef _RCSP_UPDATE_H_
#define _RCSP_UPDATE_H_

//#include "rcsp_protocol.h"
//#include "rcsp_packet.h"
#include "typedef.h"

int JL_rcsp_update_cmd_resp(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr);
int JL_rcsp_update_msg_deal(void *hdl, u8 event, u8 *msg);

int JL_rcsp_update_cmd_receive_resp(void *priv, u8 OpCode, u8 status, u8 *data, u16 len);
u8 get_jl_update_flag(void);
void set_jl_update_flag(u8 flag);
u8 get_curr_device_type(void);
void update_slave_adv_reopen(void *priv);
u8 rcsp_update_get_role_switch(void);

#endif

