/*********************************************************************************************
    *   Filename        : le_audio_common.c

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2023-8-18 19:09

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "app_main.h"
#include "audio_base.h"
#include "wireless_trans.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "le_broadcast.h"
#include "le_connected.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"
#include "classic/tws_event.h"
#include "classic/tws_api.h"
#include "bt_tws.h"

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

/**************************************************************************************************
  Static Prototypes
 **************************************************************************************************/
static int default_rx_le_audio_open(void *rx_audio, void *args);
static int default_rx_le_audio_close(void *rx_audio);
static int default_tx_le_audio_close(void *rx_audio);
static void *default_tx_le_audio_open(void *args);

/**************************************************************************************************
  Extern Global Variables
**************************************************************************************************/
extern const struct le_audio_mode_ops le_audio_a2dp_ops;
extern const struct le_audio_mode_ops le_audio_music_ops;
extern const struct le_audio_mode_ops le_audio_linein_ops;
extern const struct le_audio_mode_ops le_audio_iis_ops;
extern const struct le_audio_mode_ops le_audio_mic_ops;
extern const struct le_audio_mode_ops le_audio_spdif_ops;
extern const struct le_audio_mode_ops le_audio_fm_ops;
extern const struct le_audio_mode_ops le_audio_pc_ops;

/**************************************************************************************************
  Local Global Variables
**************************************************************************************************/
static u8 lea_product_test_name[28];
static u8 le_audio_pair_name[28];
static struct le_audio_mode_ops *broadcast_audio_switch_ops = NULL; /*!< le audio和local audio切换回调接口指针 */
static struct le_audio_mode_ops *connected_audio_switch_ops = NULL; /*!< le audio和local audio切换回调接口指针 */
const struct le_audio_mode_ops le_audio_default_ops = {
    .rx_le_audio_open = default_rx_le_audio_open,
    .rx_le_audio_close = default_rx_le_audio_close,
};

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
void read_le_audio_product_name(void)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    int len = syscfg_read(CFG_LEA_PRODUCET_TEST_NAME, lea_product_test_name, sizeof(lea_product_test_name));
    if (len <= 0) {
        r_printf("ERR:Can not read the product test name\n");
        return;
    }

    put_buf((const u8 *)lea_product_test_name, sizeof(lea_product_test_name));
    r_printf("product_test_name:%s", lea_product_test_name);
#endif
}

void read_le_audio_pair_name(void)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    int len = syscfg_read(CFG_LEA_PAIR_NAME, le_audio_pair_name, sizeof(le_audio_pair_name));
    if (len <= 0) {
        r_printf("ERR:Can not read the le audio pair name\n");
        return;
    }

    put_buf((const u8 *)le_audio_pair_name, sizeof(le_audio_pair_name));
    y_printf("pair_name:%s", le_audio_pair_name);
#endif
}

const char *get_le_audio_product_name(void)
{
    return (const char *)lea_product_test_name;
}

const char *get_le_audio_pair_name(void)
{
    return (const char *)le_audio_pair_name;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 注册le audio和local audio切换回调接口
 *
 * @param ops:le audio和local audio切换回调接口结构体
 */
/* ----------------------------------------------------------------------------*/
static void le_audio_switch_ops_callback(void *ops)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
    broadcast_audio_switch_ops = (struct le_audio_mode_ops *)ops;
#elif (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    connected_audio_switch_ops = (struct le_audio_mode_ops *)ops;
#endif
}

struct le_audio_mode_ops *get_broadcast_audio_sw_ops()
{
    return broadcast_audio_switch_ops;
}

struct le_audio_mode_ops *get_connected_audio_sw_ops()
{
    return connected_audio_switch_ops;
}

int le_audio_ops_register(u8 mode)
{
#if TCFG_KBOX_1T3_MODE_EN
    mode = APP_MODE_NULL;
#endif

    g_printf("le_audio_ops_register:%d", mode);

    switch (mode) {
#if TCFG_APP_BT_EN
    case APP_MODE_BT:
        le_audio_switch_ops_callback((void *)&le_audio_a2dp_ops);
        break;
#endif
#if TCFG_APP_MUSIC_EN
    case APP_MODE_MUSIC:
        le_audio_switch_ops_callback((void *)&le_audio_music_ops);
        break;
#endif
#if TCFG_APP_LINEIN_EN
    case APP_MODE_LINEIN:
        le_audio_switch_ops_callback((void *)&le_audio_linein_ops);
        break;
#endif
#if TCFG_APP_IIS_EN
    case APP_MODE_IIS:
        le_audio_switch_ops_callback((void *)&le_audio_iis_ops);
        break;
#endif
#if TCFG_APP_MIC_EN
    case APP_MODE_MIC:
        le_audio_switch_ops_callback((void *)&le_audio_mic_ops);
        break;
#endif
#if TCFG_APP_PC_EN
    case APP_MODE_PC:
        le_audio_switch_ops_callback((void *)&le_audio_pc_ops);
        break;
#endif
#if TCFG_APP_SPDIF_EN
    case APP_MODE_SPDIF:
        le_audio_switch_ops_callback((void *)&le_audio_spdif_ops);
        break;
#endif
#if TCFG_APP_FM_EN
    case APP_MODE_FM:
        le_audio_switch_ops_callback((void *)&le_audio_fm_ops);
        break;
#endif
    default:
        le_audio_switch_ops_callback((void *)&le_audio_default_ops);
        break;
    }
    return 0;
}

int le_audio_ops_unregister(void)
{
    le_audio_switch_ops_callback(NULL);
    return 0;
}

static int default_rx_le_audio_open(void *rx_audio, void *args)
{
    int err;
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //打开广播音频播放
    struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
    rx_audio_hdl->le_audio = le_audio_stream_create(params->conn, &params->fmt);
    rx_audio_hdl->rx_stream = le_audio_stream_rx_open(rx_audio_hdl->le_audio, params->fmt.coding_type);
    err = le_audio_player_open(rx_audio_hdl->le_audio, params);
    if (err != 0) {
        ASSERT(0, "player open fail");
    }

    return 0;
}

static int default_rx_le_audio_close(void *rx_audio)
{
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
    le_audio_player_close(rx_audio_hdl->le_audio);
    le_audio_stream_rx_close(rx_audio_hdl->rx_stream);
    le_audio_stream_free(rx_audio_hdl->le_audio);

    return 0;
}

void le_audio_working_status_switch(u8 en)
{
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN))
    if (en) {
        app_broadcast_open_in_other_mode();
    } else {
        if (app_get_current_mode()->name != APP_MODE_BT) {
            app_broadcast_close_in_other_mode();
        } else {
            app_broadcast_close(APP_BROADCAST_STATUS_STOP);
        }
    }
#endif


#if ((LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN))
    if (en) {
        app_connected_open_in_other_mode();
    } else {
        if (app_get_current_mode()->name != APP_MODE_BT) {
            app_connected_close_in_other_mode();
        } else {
            app_connected_close_all(APP_CONNECTED_STATUS_STOP);
        }
    }
#endif

}

#if (TCFG_USER_TWS_ENABLE && TCFG_APP_BT_EN)

#if (TCFG_KBOX_1T3_MODE_EN)
static void set_tws_sibling_mic_status(void *_data, u16 len, bool rx)
{
    u8 *data = (u8 *)_data;
    u8 local_mic_con_num = get_bis_connected_num();

    if (rx) {
        r_printf("mic status tws rx %d\n", data[0]);

        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            ///从机没有连接mic，或者主机有连接mic的情况，从机都将le audio关掉
            if ((local_mic_con_num == 0) || (data[0])) {
                r_printf("slave working mic close\n");
                /* le_audio_working_status_switch(0); */
                app_send_message(APP_MSG_WIRELESS_MIC_CLOSE, 0);
            }
        } else {
            ///主机没有mic连接并且从机有mic连接，主机将le audio关掉
            if ((local_mic_con_num == 0) && (data[0])) {
                r_printf("master working mic close\n");
                /* le_audio_working_status_switch(0); */
                app_send_message(APP_MSG_WIRELESS_MIC_CLOSE, 0);
            }
        }

    }
}

REGISTER_TWS_FUNC_STUB(mic_sync_stub) = {
    .func_id = TWS_FUNC_ID_MIC_SYNC,
    .func    = set_tws_sibling_mic_status,
};

void le_audio_tws_sync_mic_status(void)
{

    u8 mic_con_num = get_bis_connected_num();

    u8 data[1];
    data[0] = mic_con_num;
    tws_api_send_data_to_sibling(data, 1, TWS_FUNC_ID_MIC_SYNC);

    r_printf("tws_sync_mic_num: %d\n", mic_con_num);
}
#endif

int le_audio_sync_tws_event_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    u8 role = evt->args[0];
    u8 state = evt->args[1];
    u8 *bt_addr = evt->args + 3;
    u8 addr[6];

    printf(">>>>>>>>>>>>>>>>>>>>> le_auido_sync_tws_event_handler %d\n", evt->event);
    printf("tws role: %s\n", (tws_api_get_role() == TWS_ROLE_MASTER) ? "master" : "slave");
    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
#if (TCFG_KBOX_1T3_MODE_EN)
#if 0
        le_audio_tws_sync_mic_status();
#else
        app_send_message(APP_MSG_WIRELESS_MIC_CLOSE, 0);
        app_send_message(APP_MSG_WIRELESS_MIC_OPEN, 1);
#endif
#endif

        break;
    case TWS_EVENT_CONNECTION_TIMEOUT:
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        /*
         * TWS连接断开
         */
        if (app_var.goto_poweroff_flag) {
            break;
        }


#if (TCFG_KBOX_1T3_MODE_EN)
        /* if (role == TWS_ROLE_SLAVE) { */
        //r_printf("\n\nworking mic open\n\n");
        /* le_audio_working_status_switch(1); */
        app_send_message(APP_MSG_WIRELESS_MIC_CLOSE, 0);
        app_send_message(APP_MSG_WIRELESS_MIC_OPEN, 1);
        /* } */
#endif


        break;
    case TWS_EVENT_MONITOR_START:
        break;
    case TWS_EVENT_PHONE_LINK_DETACH:
        /*
         * 跟手机的链路LMP层已完全断开, 只有tws在连接状态才会收到此事件
         */
        break;
    case TWS_EVENT_ROLE_SWITCH:
        break;
    }
    return 0;
}

APP_MSG_HANDLER(le_audio_tws_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = le_audio_sync_tws_event_handler,
};
#endif

#endif

int le_audio_scene_deal(int scene)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    return app_broadcast_deal(scene);
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    return app_connected_deal(scene);
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    return app_auracast_deal(scene);
#endif

    return -EPERM;
}


int update_le_audio_deal_scene(int scene)
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    return update_app_broadcast_deal_scene(scene);
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    return update_app_connected_deal_scene(scene);
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    return update_app_auracast_deal_scene(scene);
#endif

    return -EPERM;
}



u8 get_le_audio_app_mode_exit_flag()
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    return get_broadcast_app_mode_exit_flag();
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    return get_connected_app_mode_exit_flag();
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    return get_auracast_app_mode_exit_flag();
#endif

    return 0;
}


u8 get_le_audio_curr_role() //1:transmitter; 2:recevier
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    return get_broadcast_role();
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
#if  (LEA_CIG_TRANS_MODE == 2)
    return 1;
#else
    return get_connected_role();
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    return get_auracast_role();
#endif

    return 0;
}


u8 get_le_audio_switch_onoff() //1:on; 0:off
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    return get_bis_switch_onoff();
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    return get_cis_switch_onoff();
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    return get_auracast_switch_onoff();
#endif

    return 0;
}



