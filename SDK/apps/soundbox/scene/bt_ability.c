#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_ability.data.bss")
#pragma data_seg(".bt_ability.data")
#pragma const_seg(".bt_ability.text.const")
#pragma code_seg(".bt_ability.text")
#endif
#include "btstack/avctp_user.h"
#include "btstack/a2dp_media_codec.h"
#include "ability_uuid.h"
#include "app_main.h"
#include "bt_tws.h"
#include "soundbox.h"
#include "a2dp_player.h"
#include "bt_slience_detect.h"
#include "low_latency.h"



#define BT_HCI_EVENT    0x1000
#define BT_STACK_EVENT  0x2000

#define BT_APP_MSG_A2DP_START   0x0001

enum {
    EVT_CONNECT_SUSS = 0x80,
    EVT_CONNECT_DETACH,
};

enum {
    DATA_ID_A2DP_PREEMPTED_ADDR,
    DATA_ID_A2DP_DETACH_ADDR,
};

static u8 a2dp_preempted_addr[6];

static u8 g_a2dp_detach_addr[2][6] = {
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
};

static u8 all_ff_addr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

#if TCFG_USER_TWS_ENABLE
#define BT_SYNC_DATA_FUNC_ID 0xCEF06127

static void bt_sync_data_func_in_irq(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;

    switch (data[0]) {
    case DATA_ID_A2DP_PREEMPTED_ADDR:
        memcpy(a2dp_preempted_addr, data + 1, 6);
        break;
    case DATA_ID_A2DP_DETACH_ADDR:
        memcpy(g_a2dp_detach_addr, data + 1, 12);
        break;
    }
}
REGISTER_TWS_FUNC_STUB(bt_sync_data_stub) = {
    .func_id = BT_SYNC_DATA_FUNC_ID,
    .func    = bt_sync_data_func_in_irq,
};
#endif

static void bt_tx_data_to_slave(u8 data_id)
{
#if TCFG_USER_TWS_ENABLE
    u8 data[16];

    data[0] = data_id;

    switch (data_id) {
    case DATA_ID_A2DP_PREEMPTED_ADDR:
        memcpy(data + 1, a2dp_preempted_addr, 6);
        tws_api_send_data_to_sibling(data, 7, BT_SYNC_DATA_FUNC_ID);
        break;
    case DATA_ID_A2DP_DETACH_ADDR:
        memcpy(data + 1, g_a2dp_detach_addr, 12);
        tws_api_send_data_to_sibling(data, 13, BT_SYNC_DATA_FUNC_ID);
        break;
    }
#endif
}

static bool bt_event_enter_sniff(struct scene_param *param)
{
    struct bt_event *bt = (struct bt_event *)param->msg;
    if (bt) {
        return bt->value == 1;
    }
    return 1;
}

static bool bt_event_exit_sniff(struct scene_param *param)
{
    struct bt_event *bt = (struct bt_event *)param->msg;
    if (bt) {
        return bt->value == 0;
    }
    return 1;
}


static const struct scene_event btstack_event_table[] = {
    [0] = {
        .event = BT_STACK_EVENT | BT_STATUS_INIT_OK,
    },
    [1] = {
        .event = BT_STACK_EVENT | BT_STATUS_FIRST_CONNECTED,
    },
    [2] = {
        .event = BT_STACK_EVENT | BT_STATUS_SECOND_CONNECTED,
    },
    [3] = {
        .event = BT_HCI_EVENT | HCI_EVENT_CONNECTION_COMPLETE,
    },
    [4] = {
        .event = BT_HCI_EVENT | HCI_EVENT_DISCONNECTION_COMPLETE,
    },
    [5] = {
        .event = BT_STACK_EVENT | BT_STATUS_PHONE_INCOME,
    },
    [6] = {
        .event = BT_STACK_EVENT | BT_STATUS_PHONE_OUT,
    },
    [7] = {
        .event = BT_STACK_EVENT | BT_STATUS_PHONE_ACTIVE,
    },
    [8] = {
        .event = BT_STACK_EVENT | BT_STATUS_PHONE_HANGUP,
    },
    [9] = {
        .event = BT_STACK_EVENT | BT_STATUS_SNIFF_STATE_UPDATE,
        .match = bt_event_enter_sniff
    },
    [10] = {
        .event = BT_STACK_EVENT | BT_STATUS_SNIFF_STATE_UPDATE,
        .match = bt_event_exit_sniff
    },
    [11] = {
        .event = BT_STACK_EVENT | BT_STATUS_A2DP_MEDIA_START,
    },
    [12] = {
        .event = BT_STACK_EVENT | BT_STATUS_A2DP_MEDIA_STOP,
    },
    [13] = {
        .event = BT_APP_MSG_A2DP_START,
    },
    [16] = {
        .event = BT_STACK_EVENT | BT_STATUS_AVDTP_START,
    },
    [17] = {
        .event = BT_STACK_EVENT | BT_STATUS_CONN_A2DP_CH,
    },
    [18] = {
        .event = BT_STACK_EVENT | BT_STATUS_DISCON_A2DP_CH,
    },
    [21] = {
        .event = BT_STACK_EVENT | BT_STATUS_SIRI_OPEN,
    },
    [22] = {
        .event = BT_STACK_EVENT | BT_STATUS_SIRI_CLOSE,
    },
    [23] = {
        .event = BT_STACK_EVENT | BT_STATUS_SIRI_GET_STATE,
    },
    [24] = {
        .event = BT_STACK_EVENT | BT_STATUS_SCO_CONNECTION_REQ,
    },


    { 0xffffffff, NULL },
};

static bool is_connected_0_device(struct scene_param *param)
{
    return bt_get_total_connect_dev() == 0;
}

static bool is_connected_1_device(struct scene_param *param)
{
    puts("is_connected_1_device\n");
    return bt_get_total_connect_dev() == 1;
}

static bool is_connected_2_device(struct scene_param *param)
{
    return bt_get_total_connect_dev() == 2;
}


static bool is_state_outgoing_call(void *device, struct scene_param *param)
{
    return btstack_bt_get_call_status(device) == BT_CALL_OUTGOING;
}

static bool is_phone_ring(struct scene_param *param)
{
    return bt_get_call_status() == BT_CALL_ALERT;
}

static bool is_state_phone_active(void *device, struct scene_param *param)
{
    return ((btstack_bt_get_call_status(device) == BT_CALL_ACTIVE));
}

static bool is_state_phone_esco_connect(void *device, struct scene_param *param)
{
    //printf("is_state_phone_esco_connect%d",btstack_get_call_esco_status(device));
    //put_buf(btstack_get_device_mac_addr(device),6);
    return (btstack_get_call_esco_status(device) == BT_ESCO_STATUS_OPEN);
}

static bool is_state_phone(void *device, struct scene_param *param)
{
    int state = btstack_bt_get_call_status(device);

    return (state == BT_CALL_ACTIVE || state == BT_CALL_ALERT  ||
            state == BT_CALL_OUTGOING || state == BT_CALL_INCOMING ||
            state == BT_SIRI_STATE);
}

static bool is_not_state_phone(void *device, struct scene_param *param)
{
    int state = btstack_bt_get_call_status(device);

    if (state == BT_CALL_HANGUP || (state == BT_SIRI_STATE && btstack_get_call_esco_status(device) == BT_ESCO_STATUS_CLOSE)) {
        return true;
    }
    return false;
}

static bool is_not_phone_state(struct scene_param *param)
{
    int state = bt_get_call_status();

    return (state == BT_CALL_HANGUP);
}

static bool is_a2dp_start(struct scene_param *param)
{
    return tws_api_get_tws_state() & TWS_STA_SBC_OPEN;
}

static bool is_a2dp_stop(struct scene_param *param)
{
    return !(tws_api_get_tws_state() & TWS_STA_SBC_OPEN);
}

static bool is_sniff_state(struct scene_param *param)
{
    return false;
}

static bool is_unsniff_state(struct scene_param *param)
{
    return false;
}

static bool is_state_a2dp_connected(void *device, struct scene_param *param)
{
    return (btstack_get_device_channel_state(device) & A2DP_CH) ? true : false;
}

static bool is_state_a2dp_disconnected(void *device, struct scene_param *param)
{
    return  !is_state_a2dp_connected(device, param);
}

static bool is_a2dp_low_latancy_mode(struct scene_param *param)
{
    return bt_get_low_latency_mode();
}

static bool is_a2dp_normal_latancy_mode(struct scene_param *param)
{
    return !bt_get_low_latency_mode();
}


static bool is_phone_incoming(struct scene_param *param)
{
    return bt_get_call_status() == BT_CALL_INCOMING;
}

static bool is_phone_outgoing(struct scene_param *param)
{
    int state = bt_get_call_status();
    return state == BT_CALL_OUTGOING;
}

static bool is_phone_active(struct scene_param *param)
{
    int state = bt_get_call_status();
    return state == BT_CALL_ACTIVE;
}

static bool is_a2dp_playing(struct scene_param *param)
{
    return a2dp_player_runing();
}

static bool is_a2dp_closed(struct scene_param *param)
{
    return !a2dp_player_runing();
}

static bool is_have_connect_info(struct scene_param *param)
{
    int num = btstack_get_num_of_remote_device_recorded();
    return num ? 1 : 0;
}

static bool is_no_connect_info(struct scene_param *param)
{
    int num = btstack_get_num_of_remote_device_recorded();
    return num ? 0 : 1;
}


static bool is_state_incoming_call(void *device, struct scene_param *param)
{
    return btstack_bt_get_call_status(device) == BT_CALL_INCOMING;
}

static bool is_state_a2dp_start(void *device, struct scene_param *param)
{
    u8 bt_addr[6];

    if (a2dp_player_get_btaddr(bt_addr)) {
        if (memcmp(bt_addr, btstack_get_device_mac_addr(device), 6) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_state_idle(void *device, struct scene_param *param)
{
    int state = btstack_get_device_a2dp_state(device);
    if (state == BT_MUSIC_STATUS_STARTING) {
        return false;
    }
    if (is_state_a2dp_start(device, NULL)) {
        return false;
    }
    state = btstack_bt_get_call_status(device);
    if (state != BT_CALL_HANGUP) {
        return false;
    }

    return true;
}

static bool is_state_a2dp_mute(void *device, struct scene_param *param)
{
    u8 *bt_addr = btstack_get_device_mac_addr(device);
    return a2dp_media_is_mute(bt_addr);
}


static bool is_state_a2dp_preempted(void *device, struct scene_param *param)
{
    if (!memcmp(a2dp_preempted_addr, btstack_get_device_mac_addr(device), 6)) {
        return 1;
    }
    return 0;
}

#define BT_STATE_IDLE               0x53c7
#define BT_STATE_A2DP_START         0x206e
#define BT_STATE_A2DP_MUTE          0x5d92
#define BT_STATE_INCOMING_CALL      0x0961
#define BT_STATE_OUTGOING_CALL      0x8607
#define BT_STATE_PHONE_ACTIVE       0xed05
#define BT_STATE_PHONE              0xe004
#define BT_STATE_A2DP_CONNECTED     0xf180
#define BT_STATE_A2DP_DISCONNECT    0xeecc
#define BT_STATE_A2DP_PREEMPTED     0x370a


struct bt_state_match {
    u16 state;
    bool (*match)(void *device, struct scene_param *param);
};

struct bt_state_match bt_state_table[] = {
    { BT_STATE_IDLE,            is_state_idle },
    { BT_STATE_A2DP_START,      is_state_a2dp_start },
    { BT_STATE_A2DP_MUTE,       is_state_a2dp_mute },
    { BT_STATE_INCOMING_CALL,   is_state_incoming_call },
    { BT_STATE_OUTGOING_CALL,   is_state_outgoing_call },
    { BT_STATE_PHONE_ACTIVE,    is_state_phone_active },
    { BT_STATE_PHONE,           is_state_phone },
    { BT_STATE_A2DP_CONNECTED,  is_state_a2dp_connected },
    { BT_STATE_A2DP_DISCONNECT, is_state_a2dp_disconnected },
    { BT_STATE_A2DP_PREEMPTED,  is_state_a2dp_preempted },
};

void *get_the_other_device(u8 *addr)
{
    void *devices[2] = {NULL, NULL};
    int num = btstack_get_conn_devices(devices, 2);
    /*printf("bt_device: %p, %p\n", devices[0], devices[1]);
    put_buf(btstack_get_device_mac_addr(devices[0]), 6);
    put_buf(btstack_get_device_mac_addr(devices[1]), 6);*/
    for (int i = 0; i < num; i++) {
        if (memcmp(btstack_get_device_mac_addr(devices[i]), addr, 6)) {
            return devices[i];
        }

    }
    return NULL;
}


static bool is_device_a_in_state(struct scene_param *param)
{
    void *device = NULL;
    void *devices[2];

    if (param->uuid == UUID_BT) {
        struct bt_event *event = (struct bt_event *)param->msg;
        device = btstack_get_conn_device(event->args);
        if (!device) {
            return false;
        }
    } else {
        int num = btstack_get_conn_devices(devices, 2);
        if (num < 1) {
            return false;
        }
        device = devices[0];
    }

    u16 state = *(u16 *)param->arg;
    for (int i = 0; i < ARRAY_SIZE(bt_state_table); i++) {
        if (bt_state_table[i].state == state) {
            return bt_state_table[i].match(device, param);
        }
    }
    return false;
}

static bool is_device_b_in_state(struct scene_param *param)
{
    void *device = NULL;
    void *devices[2];

    if (param->uuid == UUID_BT) {
        struct bt_event *event = (struct bt_event *)param->msg;
        device = get_the_other_device(event->args);
        if (!device) {
            return false;
        }
    } else {
        int num = btstack_get_conn_devices(devices, 2);
        if (num == 0) {
            return false;
        }
        device = devices[num - 1];
    }

    u16 state = *(u16 *)param->arg;

    for (int i = 0; i < ARRAY_SIZE(bt_state_table); i++) {
        if (bt_state_table[i].state == state) {
            printf("is_device_b_in_state: %d\n", i);
            return bt_state_table[i].match(device, param);
        }
    }
    return false;
}

static const struct scene_state btstack_state_table[] = {
    [0] = {
        .match = is_device_a_in_state,
    },
    [1] = {
        .match = is_device_b_in_state,
    },
    [2] = {
        .match = is_connected_0_device
    },
    [3] = {
        .match = is_connected_1_device
    },
    [4] = {
        .match = is_connected_2_device
    },
    [5] = {
        .match = is_a2dp_low_latancy_mode,
    },
    [6] = {
        .match = is_a2dp_normal_latancy_mode,
    },
    [7] = {
        .match = is_phone_incoming,
    },
    [8] = {
        .match = is_phone_outgoing,
    },
    [9] = {
        .match = is_phone_active,
    },
    [10] = {
        .match = is_not_phone_state,
    },
    [11] = {
        .match = is_a2dp_playing,
    },
    [12] = {
        .match = is_a2dp_closed,
    },
    [13] = {
        .match = is_have_connect_info,
    },
    [14] = {
        .match = is_no_connect_info,
    },

    { NULL }
};

enum {
    API_MUSIC_PP,
    API_MUSIC_PREV,
    API_MUSIC_NEXT,
    API_CALL_ANSWER,
    API_CALL_HANGUP,
};

static void call_btstack_api(int api, int err)
{
    u8 addr[6];
    u8 *bt_addr = NULL;

    if (api & 0x80) {
        api &= ~0x80;
    } else {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            return;
        }
    }

    if (a2dp_player_get_btaddr(addr)) {
        bt_addr = addr;
    }
    int state = bt_get_call_status();

    switch (api) {
    case API_MUSIC_PP:
        if (state != BT_CALL_HANGUP) {
            break;
        }
        bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
        break;
    case API_MUSIC_PREV:
        if (state != BT_CALL_HANGUP) {
            break;
        }
        bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_PREV, 0, NULL);
        break;
    case API_MUSIC_NEXT:
        if (state != BT_CALL_HANGUP) {
            break;
        }
        bt_cmd_prepare_for_addr(bt_addr, USER_CTRL_AVCTP_OPID_NEXT, 0, NULL);
        break;
    case API_CALL_ANSWER:
        bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        break;
    case API_CALL_HANGUP:
        bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        break;
    }
}

/* TWS_SYNC_CALL_REGISTER(btstack_api_sync_call_entry) = { */
/*     .uuid       = 0x30192A4B, */
/*     .func       = call_btstack_api, */
/*     .task_name  = "app_core", */
/* }; */
static void tws_master_call_btstack_api(int api)
{
    tws_api_sync_call_by_uuid(0x30192A4B, api, 100);
}

static void action_open_inquiry_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
}

static void action_close_inquiry_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
}

static void action_open_page_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);

}

static void action_close_page_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
}

static void action_start_page(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    printf("---action_start_page\n");
    app_send_message(APP_MSG_BT_PAGE_DEVICE, 0);

}

static void action_stop_page(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
}

static void action_open_inquiry_scan_page_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    lmp_hci_write_scan_enable((1 << 1) | 1);
}

static void action_close_inquiry_scan_open_page_scan(struct scene_param *param)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    lmp_hci_write_scan_enable((1 << 1) | 0);
}

static void action_close_inquiry_scan_page_scan(struct scene_param *param)
{
    lmp_hci_write_scan_enable((0 << 1) | 0);
}

static void action_a2dp_play_pause(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (priv->local_action && tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_master_call_btstack_api(API_MUSIC_PP);
    } else {
        call_btstack_api(0x80 | API_MUSIC_PP, 0);
    }
}

static void action_a2dp_play_prev(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (priv->local_action && tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_master_call_btstack_api(API_MUSIC_PREV);
    } else {
        call_btstack_api(0x80 | API_MUSIC_PREV, 0);
    }
}

static void action_a2dp_play_next(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (priv->local_action && tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_master_call_btstack_api(API_MUSIC_NEXT);
    } else {
        call_btstack_api(0x80 | API_MUSIC_NEXT, 0);
    }
}

static void action_call_answer(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (priv->local_action && tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_master_call_btstack_api(API_CALL_ANSWER);
    } else {
        call_btstack_api(0x80 | API_CALL_ANSWER, 0);
    }
}

static void action_call_hangup(struct scene_param *param)
{
    struct scene_private_data *priv = (struct scene_private_data *)param->user_type;

    if (priv->local_action && tws_api_get_role() == TWS_ROLE_SLAVE) {
        tws_master_call_btstack_api(API_CALL_HANGUP);
    } else {
        call_btstack_api(0x80 | API_CALL_HANGUP, 0);
    }
}

static void action_enter_low_latency_mode(struct scene_param *param)
{
    int low_latency = bt_get_low_latency_mode();
    printf("enter_low_latency: %d\n", low_latency);
    if (!low_latency) {
        bt_set_low_latency_mode(1, 1, 300);
    }
}

static void action_exit_low_latency_mode(struct scene_param *param)
{
    int low_latency = bt_get_low_latency_mode();
    printf("exit_low_latency: %d\n", low_latency);
    if (low_latency) {
        bt_set_low_latency_mode(0, 1, 300);
    }
}

static void action_open_dut(struct scene_param *param)
{
    bt_bredr_enter_dut_mode(0, 0);
}

static void action_close_dut(struct scene_param *param)
{
    bt_bredr_exit_dut_mode();
}

static void action_delete_link_key(struct scene_param *param)
{

}

static void action_open_siri(struct scene_param *param)
{
    bt_cmd_prepare(USER_CTRL_HFP_GET_SIRI_OPEN, 0, NULL);
}

struct bt_action_match {
    u16 action;
    void (*func)(void *device, u8 *addr);
};

#define BT_ACTION_A2DP_PLAY         0x206e
#define BT_ACTION_A2DP_PAUSE        0xdbd5
#define BT_ACTION_A2DP_MUTE         0x5d92
#define BT_ACTION_A2DP_DETACH       0x1f9e
#define BT_ACTION_A2DP_RECONN       0x3057
#define BT_ACTION_ESCO_DETACH       0xe3a1
#define BT_ACTION_ESCO_RECONN       0xf45a
#define BT_ACTION_DETACH            0x600d
#define BT_ACTION_SET_PREEMPTED     0x3b0e
#define BT_ACTION_CLR_PREEMPTED     0x40f1
#define BT_ACTION_BACKUP_PREEMPTED  0xF869
#define BT_ACTION_REJECT_ESCO_DETACH  0x8ee6


void bt_action_a2dp_play(void *device, u8 *bt_addr)
{
    int slience = bt_slience_detect_get_result(bt_addr);
    int playing = a2dp_player_is_playing(bt_addr);

    printf("a2dp_play: %d, %d\n", playing, slience);
    put_buf(bt_addr, 6);

    if (slience == BT_SLIENCE_NO_DETECTING) {
        if (!playing) {
            btstack_device_control(device, USER_CTRL_AVCTP_OPID_PLAY);
        }
    } else if (slience == BT_SLIENCE_NO_ENERGY) {
        btstack_device_control(device, USER_CTRL_AVCTP_OPID_PLAY);
    } else {
        int msg[4];
        msg[0] = APP_MSG_BT_A2DP_PLAY;
        memcpy(msg + 1, bt_addr, 6);
        app_send_message_from(MSG_FROM_APP, 12, msg);
    }
}

void bt_action_a2dp_pause(void *device, u8 *bt_addr)
{
    int slience = bt_slience_detect_get_result(bt_addr);
    int playing = a2dp_player_is_playing(bt_addr);

    printf("a2dp_pause: %d, %d\n", playing, slience);
    put_buf(bt_addr, 6);

    if (playing) {
        btstack_device_control(device, USER_CTRL_AVCTP_PAUSE_MUSIC);
    } else {
        if (slience == BT_SLIENCE_HAVE_ENERGY) {
            btstack_device_control(device, USER_CTRL_AVCTP_PAUSE_MUSIC);
        }
    }
    int msg[4];
    msg[0] = APP_MSG_BT_A2DP_PAUSE;
    memcpy(msg + 1, bt_addr, 6);
    app_send_message_from(MSG_FROM_APP, 12, msg);
}

static void bt_action_a2dp_mute(void *device, u8 *bt_addr)
{
    a2dp_media_mute(bt_addr);
}


static void add_addr_to_a2dp_detach_list(u8 *bt_addr)
{
    for (int i = 0; i < 2; i++) {
        if (memcmp(g_a2dp_detach_addr[i], bt_addr, 6) == 0) {
            return;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (memcmp(g_a2dp_detach_addr[i], all_ff_addr, 6) == 0) {
            memcpy(g_a2dp_detach_addr[i], bt_addr, 6);
            break;
        }
    }
    bt_tx_data_to_slave(DATA_ID_A2DP_DETACH_ADDR);
}

/*static bool del_addr_from_a2dp_detach_list(u8 *bt_addr)
{
    int ret = 0;

    for (int i = 0; i < 2; i++) {
        if (memcmp(g_a2dp_detach_addr[i], bt_addr, 6) == 0) {
            memset(g_a2dp_detach_addr[i], 0xff, 6);
            ret = 1;
        }
    }
    bt_tx_data_to_slave(DATA_ID_A2DP_DETACH_ADDR);
    return ret;
}*/

static void bt_action_a2dp_detach(void *device, u8 *bt_addr)
{
    //A播歌，A打电话，B再打电话,这样操作在断开的位置记录不了。每次开始的时候记录,暂不处理
    u8 pause_music = 1;
    r_printf("bt_action_a2dp_detach=%d", pause_music);
    bt_stack_save_a2dp_auto_discon_addr(bt_addr, pause_music);
    /*add_addr_to_a2dp_detach_list(bt_addr);*/
    btstack_device_control(device, USER_CTRL_DISCONN_A2DP);
}

static void bt_action_a2dp_reconn(void *device, u8 *bt_addr)
{
    int ret = 0;

    if (bt_stack_check_a2dp_auto_discon_addr(bt_addr)) {
        ret = btstack_device_control(device, USER_CTRL_CONN_A2DP);
    }
    /*if (del_addr_from_a2dp_detach_list(bt_addr)) {
        ret = btstack_device_control(device, USER_CTRL_CONN_A2DP);
    }*/
    printf("bt_action_a2dp_reconn--: %d\n", ret);
}

static void bt_action_esco_detach(void *device, u8 *bt_addr)
{
    btstack_device_control(device, USER_CTRL_DISCONN_SCO);
}
static void bt_action_reject_esco_detach(void *device, u8 *bt_addr)
{
    btstack_device_control(device, USER_CTRL_DISCONN_REJECT_SCO);
}


static void bt_action_esco_reconn(void *device, u8 *bt_addr)
{
    btstack_device_control(device, USER_CTRL_CONN_SCO);
}

static void bt_action_detach(void *device, u8 *bt_addr)
{
    btstack_device_detach(device);
}

static void bt_action_set_preempted_flag(void *device, u8 *bt_addr)
{
    memcpy(a2dp_preempted_addr, bt_addr, 6);
    bt_tx_data_to_slave(DATA_ID_A2DP_PREEMPTED_ADDR);
    r_printf("set_preepmpted\n");
    put_buf(bt_addr, 6);
}

static void bt_action_clr_preempted_flag(void *device, u8 *bt_addr)
{
    if (memcmp(a2dp_preempted_addr, bt_addr, 6) == 0) {
        memset(a2dp_preempted_addr, 0xff, 6);
        bt_tx_data_to_slave(DATA_ID_A2DP_PREEMPTED_ADDR);
        r_printf("clr_preepmpted\n");
        put_buf(bt_addr, 6);
    }
}

static void bt_action_backup_preempted_flag(void *device, u8 *bt_addr)
{
    memcpy(a2dp_preempted_addr, bt_addr, 6);
    bt_tx_data_to_slave(DATA_ID_A2DP_PREEMPTED_ADDR);
    r_printf("set_preepmpted\n");
    put_buf(bt_addr, 6);
}

struct bt_action_match bt_action_table[] = {
    { BT_ACTION_A2DP_PLAY,      bt_action_a2dp_play },
    { BT_ACTION_A2DP_PAUSE,     bt_action_a2dp_pause },
    { BT_ACTION_A2DP_MUTE,      bt_action_a2dp_mute },
    { BT_ACTION_A2DP_DETACH,    bt_action_a2dp_detach },
    { BT_ACTION_A2DP_RECONN,    bt_action_a2dp_reconn },
    { BT_ACTION_ESCO_DETACH,    bt_action_esco_detach },
    { BT_ACTION_ESCO_RECONN,    bt_action_esco_reconn },
    { BT_ACTION_DETACH,         bt_action_detach },
    { BT_ACTION_SET_PREEMPTED,  bt_action_set_preempted_flag },
    { BT_ACTION_CLR_PREEMPTED,  bt_action_clr_preempted_flag },
    { BT_ACTION_BACKUP_PREEMPTED,  bt_action_backup_preempted_flag },
    { BT_ACTION_REJECT_ESCO_DETACH,    bt_action_reject_esco_detach },
};

static void device_do_action(void *device, u16 action)
{
    for (int i = 0; i < ARRAY_SIZE(bt_action_table); i++) {
        if (bt_action_table[i].action == action) {
            bt_action_table[i].func(device, btstack_get_device_mac_addr(device));
            break;
        }
    }
}

static void action_device_a(struct scene_param *param)
{
    void *device = NULL;
    void *devices[2];

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    if (param->uuid == UUID_BT) {
        struct bt_event *event = (struct bt_event *)param->msg;
        device = btstack_get_conn_device(event->args);
        if (!device) {
            return;
        }
    } else {
        int num = btstack_get_conn_devices(devices, 2);
        if (num == 0) {
            return;
        }
        device = devices[0];
    }

    u16 action = *(u16 *)param->arg;
    printf("action_device_a: %x, %p\n", action, device);
    device_do_action(device, action);
}

static void action_device_b(struct scene_param *param)
{
    void *device = NULL;
    void *devices[2];

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    if (param->uuid == UUID_BT) {
        struct bt_event *event = (struct bt_event *)param->msg;
        device = get_the_other_device(event->args);
        if (!device) {
            return;
        }
    } else {
        int num = btstack_get_conn_devices(devices, 2);
        if (num == 0) {
            return;
        }
        device = devices[num - 1];
    }

    u16 action = *(u16 *)param->arg;
    printf("action_device_b: %x\n", action);
    device_do_action(device, action);
}

static const struct scene_action btstack_action_table[] = {
    [0] = {
        .action = action_device_a,
    },
    [1] = {
        .action = action_device_b,
    },
    [2] = {
        .action = action_a2dp_play_pause,
    },
    [3] = {
        .action = action_a2dp_play_prev,
    },
    [4] = {
        .action = action_a2dp_play_next,
    },
    [5] = {
        .action = action_call_answer,
    },
    [6] = {
        .action = action_call_hangup,
    },
    [7] = {
        .action = action_open_dut,
    },
    [8] = {
        .action = action_close_dut,
    },
    [9] = {
        .action = action_delete_link_key,
    },
    [10] = {
        .action = action_open_siri,
    },
    [11] = {
        .action = action_enter_low_latency_mode,
    },
    [12] = {
        .action = action_exit_low_latency_mode,
    },
    [13] = {
        .action = action_start_page,
    },
    [14] = {
        .action = action_open_inquiry_scan_page_scan,
    },
    [15] = {
        .action = action_close_inquiry_scan_open_page_scan,
    },
    [16] = {
        .action = action_close_inquiry_scan_page_scan,
    },

    { NULL }
};

/* REGISTER_SCENE_ABILITY(btstack_ability) = { */
/*     .uuid   = UUID_BT, */
/*     .event  = btstack_event_table, */
/*     .state  = btstack_state_table, */
/*     .action = btstack_action_table, */
/* }; */

void scene_mgr_event_match(u8 uuid, int _event, void *msg)
{

}

u16 a2dp_resume_timer = 0xffff;
void a2dp_resume_hdl(void *priv)
{
    void *device = priv;
    if (is_not_phone_state(NULL)) {
        if (is_state_idle(device, NULL) && is_state_a2dp_preempted(device, NULL)) {
            bt_action_a2dp_play(device, btstack_get_device_mac_addr(device));
            bt_action_clr_preempted_flag(device, btstack_get_device_mac_addr(device));
            sys_timer_del(a2dp_resume_timer);
            a2dp_resume_timer = 0xffff;
        }
    }
}

static u8 btstack_phone_event_convert(struct bt_event *event)
{
    u8 match_event = 0;
    if (event->event == BT_STATUS_SCO_CONNECTION_REQ || event->event == BT_STATUS_SCO_DISCON) {
        u8 call_status = bt_get_call_status_for_addr(event->args);
        if (event->event == BT_STATUS_SCO_CONNECTION_REQ) {
            switch (call_status) {
            case BT_CALL_INCOMING:
                puts("match event income_call\n");
                match_event = BT_STATUS_PHONE_INCOME;
                break;
            case BT_SIRI_STATE:
                puts("siri\n");
            case BT_CALL_OUTGOING:
                puts("match event outgoing_call\n");
                match_event = BT_STATUS_PHONE_OUT;
                break;
            }
        } else if (event->event == BT_STATUS_SCO_DISCON) {
            switch (call_status) {
            case BT_SIRI_STATE:
                puts("match handup\n");
                match_event = BT_STATUS_PHONE_HANGUP;
                break;
            }
        }
    }
    return match_event;
}

static int btstack_a2dp_esco_preempted_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    u8 match_event = 0;

    printf("bt_event_happen: %d\n", event->event);

    void *device = get_the_other_device(event->args);
    if (device == NULL) {
        //只存在一个链接，不需要处理抢占逻辑
        return 0;
    }

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
#endif

    match_event =  btstack_phone_event_convert(event);

    if (match_event == 0) {
        match_event = event->event;
    }

    switch (match_event) {
    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
    case BT_STATUS_SIRI_OPEN:
        //设备A来电、去电、接通电话、SIRI打开且设备B播放媒体音频时断开设备B的A2DP链路并设置音频被抢占标志
        if (is_state_a2dp_start(device, NULL)) {
            bt_action_set_preempted_flag(device, btstack_get_device_mac_addr(device));
            bt_action_a2dp_detach(device, btstack_get_device_mac_addr(device));
        }
        break;
    case BT_STATUS_PHONE_HANGUP:
    case BT_STATUS_SIRI_CLOSE:
        //设备A挂断电话、关闭SIRI，设备B回连A2DP链路
        if (is_state_a2dp_disconnected(device, NULL)) {
            bt_action_a2dp_reconn(device, btstack_get_device_mac_addr(device));
        }
        //设备A挂断电话且设备B非通话状态，设备A回连A2DP
        if (is_not_state_phone(device, NULL)) {
            if (is_state_a2dp_disconnected(btstack_get_conn_device(event->args), NULL)) {
                bt_action_a2dp_reconn(btstack_get_conn_device(event->args), event->args);
            }
        }
        break;
    case BT_STATUS_CONN_A2DP_CH:
        //链接上a2dp_ch的设备之前被抢占,创建定时器恢复a2dp播放
        if (is_state_a2dp_preempted(btstack_get_conn_device(event->args), NULL)) {
            if (a2dp_resume_timer == 0xffff) {
                a2dp_resume_timer = sys_timer_add(btstack_get_conn_device(event->args), a2dp_resume_hdl, 200);
            }
        }
        //设备A a2dp回连成功，且设备B 通话中，断开设备A的a2dp链路
        if (is_state_phone(device, NULL)) {
            bt_action_a2dp_detach(btstack_get_conn_device(event->args), event->args);
        }
        break;
    case BT_STATUS_DISCON_A2DP_CH:
        //当前设备A断开a2dp时,设备B非通话状态，设备A回连A2DP
        if (is_not_state_phone(device, NULL)) {
            bt_action_a2dp_reconn(btstack_get_conn_device(event->args), event->args);
        }
        break;
    case BT_STATUS_FIRST_CONNECTED:
    case BT_STATUS_SECOND_CONNECTED:
        bt_action_clr_preempted_flag(btstack_get_conn_device(event->args), event->args);
        break;
    }

    return 0;
}

APP_MSG_HANDLER(scene_btstack_a2dp_esco_preempted_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = btstack_a2dp_esco_preempted_msg_handler,
};

static int btstack_2_esco_preempted_msg_handler(int *msg)
{
    struct bt_event *event = (struct bt_event *)msg;
    u8 match_event = 0;

    printf("bt_event_happen: %d\n", event->event);

    void *device = get_the_other_device(event->args);
    if (device == NULL) {
        //只存在一个链接，不需要处理抢占逻辑
        return 0;
    }

#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
#endif
    match_event =  btstack_phone_event_convert(event);

    if (match_event == 0) {
        match_event = event->event;
    }

    switch (match_event) {
    case BT_STATUS_PHONE_INCOME:
        //先来电 > 后来电
        if (is_state_incoming_call(device, NULL)) {
            bt_action_esco_detach(btstack_get_conn_device(event->args), event->args);
        }
        //去电 > 来电（case2）
        if (is_state_outgoing_call(device, NULL)) {
            bt_action_esco_detach(btstack_get_conn_device(event->args), event->args);
        }
        //通话 > 去电
        if (is_state_phone_active(device, NULL) && is_state_phone_esco_connect(device, NULL)) {
            bt_action_esco_detach(btstack_get_conn_device(event->args), event->args);
        }
        break;
    case BT_STATUS_PHONE_OUT:
        //去电 > 来电（case1）
        if (is_state_incoming_call(device, NULL)) {
            bt_action_esco_detach(device, btstack_get_device_mac_addr(device));
        }
        //后去电 > 先去电
        if (is_state_outgoing_call(device, NULL)) {
            bt_action_esco_detach(device, btstack_get_device_mac_addr(device));
        }
        //通话 > 去电
        if (is_state_phone_active(device, NULL) && is_state_phone_esco_connect(device, NULL)) {
            bt_action_esco_detach(btstack_get_conn_device(event->args), event->args);
        }
        break;
    case BT_STATUS_SCO_CONNECTION_REQ:
        //设备A通话esco创建成功时，设备A通话中且设备B去电响铃且设备B的ESCO链路已连接, 断开设备B
        if (is_state_phone_active(btstack_get_conn_device(event->args), NULL) && is_state_phone_esco_connect(device, NULL)) {
            if (is_state_outgoing_call(device, NULL)) { //设备B去电中，抢断
                bt_action_esco_detach(device, btstack_get_device_mac_addr(device));
            } else if (is_state_phone_active(device, NULL)) {   //设备B通话中，不抢断
                bt_action_esco_detach(btstack_get_conn_device(event->args), event->args);
            }
        }
        break;
    }

    return 0;
}

APP_MSG_HANDLER(scene_btstack_2_esco_preempted_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = btstack_2_esco_preempted_msg_handler,
};

static int bthci_msg_handler(int *msg)
{
    u8 *event = (u8 *)msg;

    printf("hci_event_happen: %d\n", event[0]);

    scene_mgr_event_match(UUID_BT, BT_HCI_EVENT | event[0], event);
    return 0;
}
/* APP_MSG_HANDLER(bthci_msg_entry) = { */
/*     .owner      = 0xff, */
/*     .from       = MSG_FROM_BT_HCI, */
/*     .handler    = bthci_msg_handler, */
/* }; */



static int bt_app_msg_handler(int *msg)
{
    if (msg[0] == APP_MSG_BT_A2DP_START) {
        struct bt_event event;
        memcpy(event.args, msg + 1, 6);
        scene_mgr_event_match(UUID_BT, BT_APP_MSG_A2DP_START, &event);
    }
    return 0;
}
/* APP_MSG_HANDLER(bt_app_msg_entry) = { */
/*     .owner      = 0xff, */
/*     .from       = MSG_FROM_APP, */
/*     .handler    = bt_app_msg_handler, */
/* }; */






