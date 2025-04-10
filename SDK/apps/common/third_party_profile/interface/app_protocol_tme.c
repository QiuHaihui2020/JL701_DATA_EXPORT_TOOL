#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_protocol_tme.data.bss")
#pragma data_seg(".app_protocol_tme.data")
#pragma const_seg(".app_protocol_tme.text.const")
#pragma code_seg(".app_protocol_tme.text")
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_protocol_api.h"
#include "system/includes.h"
#include "audio_config.h"
#include "app_power_manage.h"
#include "user_cfg.h"
#include "btstack/avctp_user.h"
#include "bt_tws.h"
#include "classic/tws_event.h"
#include "resfile.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & TME_EN)

extern void TME_set_edr_connected(u8 flag);

//*********************************************************************************//
//                                 TME认证信息                                     //
//*********************************************************************************//

//*********************************************************************************//
//                                 TME提示音                                       //
//*********************************************************************************//

const char *tme_notice_tab[APP_RROTOCOL_TONE_MAX] = {
    [APP_RROTOCOL_TONE_SPEECH_KEY_START]	    	= "tone_en/normal.*",
};

//*********************************************************************************//
//                                 TME弱函数实现                                   //
//*********************************************************************************//

//*********************************************************************************//
//                                 TME私有消息处理                                 //
//*********************************************************************************//
#if 1
extern int le_controller_set_mac(void *addr);
extern int le_controller_get_mac(void *addr);

#if TCFG_USER_TWS_ENABLE
static void tme_tws_rx_from_sibling(u16 cmd, u8 *data, u16 len)
{
#if TCFG_USER_TWS_ENABLE
    switch (cmd) {
    }
#endif
}

static void tme_update_ble_addr()
{
    u8 ble_old_addr[6];
    u8 ble_new_addr[6];
    u8 comm_addr[6];

    printf("%s\n", __func__);

    tws_api_get_local_addr(comm_addr);
    le_controller_get_mac(ble_old_addr);
    bt_make_ble_address(ble_new_addr, comm_addr);
    le_controller_set_mac(ble_new_addr); //地址发生变化，更新地址
    if (is_tws_master_role() && memcmp(ble_old_addr, ble_new_addr, 6)) {
        app_protocol_disconnect(NULL);
        app_protocol_ble_adv_switch(0);
        app_protocol_ble_adv_switch(1);
    }
}

static int tme_bt_tws_event_handler(struct tws_event *bt)
{
    int role = bt->args[0];
    int phone_link_connection = bt->args[1];
    int reason = bt->args[2];

    switch (bt->event) {
    case TWS_EVENT_CONNECTED:
        tme_update_ble_addr();
        if (BT_STATUS_WAITINT_CONN != bt_get_connect_status()) {
            TME_set_edr_connected(1);
        }
        break;
    case TWS_EVENT_REMOVE_PAIRS:
        tme_update_ble_addr();
        break;
    }
    return 0;
}
#endif

static int tme_bt_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        TME_set_edr_connected(1); //连接过经典蓝牙标志
        break;
    }
    return 0;
}

/*int tme_sys_event_deal(struct sys_event *event)
{
    switch (event->type) {
    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            tme_bt_status_event_handler(&event->u.bt);
        }
#if TCFG_USER_TWS_ENABLE
        else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
            tme_bt_tws_event_handler(&event->u.bt);
        }
#endif
        break;

    }

    return 0;

}*/

struct app_protocol_private_handle_t tme_private_handle = {
    /* .tws_rx_from_siblling = tme_tws_rx_from_sibling, */
    .tws_event_handler = tme_bt_tws_event_handler,
    .bt_event_handler = tme_bt_status_event_handler,
};
#endif

#endif

