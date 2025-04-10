#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".btstack.data.bss")
#pragma data_seg(".btstack.data")
#pragma const_seg(".btstack.text.const")
#pragma code_seg(".btstack.text")
#endif
#include "app_msg.h"
#include "classic/tws_api.h"
#include "app_config.h"

#define SYS_BT_EVENT_TYPE_CON_STATUS   (('C' << 24) | ('O' << 16) | ('N' << 8) | '\0')
#define SYS_BT_EVENT_TYPE_HCI_STATUS   (('H' << 24) | ('C' << 16) | ('I' << 8) | '\0')
struct _bt_event {
    u8 event;
    u8 args[7];
    u32 value;
};

static void bt_stack_event_update_to_user(u8 *addr, u32 type, u8 event, u32 value)
{
    int msg[4];
    struct _bt_event *evt = (struct _bt_event *)msg;

    evt->event = event;
    if (addr) {
        memcpy(evt->args, addr, 6);
    }
    evt->value = value;

    int from = type == SYS_BT_EVENT_TYPE_CON_STATUS ? MSG_FROM_BT_STACK : MSG_FROM_BT_HCI;
    int err = os_taskq_post_type("app_core", from, sizeof(*evt) / 4, msg);
    if (err != OS_NO_ERR) {
        printf("bt_stack_event_update_to_user error:%d", err);
    }

    /* 防止短时间内太多事件,app_core处理不过来导致qfull */
    os_time_dly(1);
}

#if TCFG_USER_TWS_ENABLE
static void bt_event_rx_handler_in_irq(void *data, u16 len, bool rx)
{
    struct _bt_event *evt = (struct _bt_event *)data;

    if (rx) {
        int from = evt->args[6] == 0 ? MSG_FROM_BT_STACK : MSG_FROM_BT_HCI;
        os_taskq_post_type("app_core", from, sizeof(*evt) / 4, data);
    }
}
REGISTER_TWS_FUNC_STUB(bt_event_sync_stub) = {
    .func_id = 0xC9073872,
    .func = bt_event_rx_handler_in_irq,
};
#endif

//从机APP大部分情况不再处理协议栈更新的消息
void bt_event_update_to_user(u8 *addr, u32 type, u8 event, u32 value)
{
#if TCFG_USER_TWS_ENABLE
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (tws_api_get_role_async() == TWS_ROLE_SLAVE) {
            return;
        }
        struct _bt_event evt;

        evt.event = event;
        evt.value = value;
        evt.args[6] = type == SYS_BT_EVENT_TYPE_CON_STATUS ? 0  : 1;
        if (addr) {
            memcpy(evt.args, addr, 6);
        }
        tws_api_send_data_to_sibling(&evt, sizeof(evt), 0xC9073872);
    }
#endif
    bt_stack_event_update_to_user(addr, type, event, value);
}

