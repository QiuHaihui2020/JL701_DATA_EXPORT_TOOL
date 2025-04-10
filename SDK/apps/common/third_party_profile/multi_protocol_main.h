#ifndef _MULTI_PROTOCOL_MAIN_H_
#define _MULTI_PROTOCOL_MAIN_H_

#include "system/includes.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "bt_common.h"
#include "app_ble_spp_api.h"
#include "app_config.h"
#include "classic/tws_api.h"
#include "classic/tws_event.h"
#include "btstack/avctp_user.h"
#include "app_main.h"

enum {
    //Q_USER          =0x400000
    MULTI_PROTOCOL_RX_DATA_EVENT = (Q_USER + 100),
    MULTI_PROTOCOL_TX_DATA_EVENT,
    MULTI_PROTOCOL_TASK_EXIT,
};

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
extern const uint8_t rcsp_profile_data[];
#endif

extern void multi_protocol_bt_init(void);
extern void multi_protocol_bt_exit(void);

extern void bt_ble_adv_enable(u8 enable);
extern void multi_protocol_common_callback_init(void);
extern void multi_protocol_common_callback_exit(void);

extern void multi_protocol_bt_tws_poweroff_handler(void);
extern void multi_protocol_state_update_callback(void *_hdl, uint8_t state, uint8_t *packet, uint16_t size);
extern void multi_protocol_tws_sync_send(void);
#endif
