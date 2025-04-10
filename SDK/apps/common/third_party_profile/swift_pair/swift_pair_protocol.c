#include "sdk_config.h"
#include "app_msg.h"
/* #include "earphone.h" */
#include "bt_tws.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "swift_pair_api.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & SWIFT_PAIR_EN)

static int swift_pair_bt_status_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        break;
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        swift_pair_exit_pair_mode();
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        swift_pair_enter_pair_mode();
        break;
    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        break;
    case BT_STATUS_PHONE_HANGUP:
        break;
    }
    return 0;
}
APP_MSG_HANDLER(swift_pair_msg_handler) = {
    .owner      = 0xFF,
    .from       = MSG_FROM_BT_STACK,
    .handler    = swift_pair_bt_status_event_handler,
};

#endif

