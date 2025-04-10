#include "system/includes.h"
#include "app_config.h"
#include "app_main.h"
#include "bt_tws.h"
#include "dual_conn.h"
#include "local_tws_player.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "app_le_auracast.h"

#if TCFG_USER_TWS_ENABLE
void bt_tws_onoff(u8 onoff)
{
    if (onoff == 1) {
        bt_tws_poweron();
    } else {
#if TCFG_LOCAL_TWS_ENABLE
        local_tws_player_close();
#endif
        tws_delete_pair_timer();
        tws_api_cancel_all();
        /* tws_api_cancle_create_connection(); */
        /* tws_api_cancle_wait_pair(); */
        bt_tws_poweroff();
    }
}
#endif

int bt_work_mode_select(u8 mode)
{
    //先释放当前模式资源
    r_printf("%s %d %d\n", __func__, mode, g_bt_hdl.work_mode);
    if (mode == g_bt_hdl.work_mode) {
        printf("same work mode  : %d", g_bt_hdl.work_mode);
        return 0;
    }
    if (mode == 0) {
        mode = BT_MODE_SIGLE_BOX;
    }

    switch (g_bt_hdl.work_mode) {
    case BT_MODE_SIGLE_BOX:

        break;
    case BT_MODE_TWS:
#if TCFG_USER_TWS_ENABLE
        bt_tws_onoff(0);
#endif
        break;
    case BT_MODE_BROADCAST:
#if LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN
        //app_broadcast_close(APP_BROADCAST_STATUS_STOP);
        le_audio_scene_deal(LE_AUDIO_APP_CLOSE);
        app_broadcast_uninit();
#endif
        break;
    case BT_MODE_AURACAST:
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        le_audio_scene_deal(LE_AUDIO_APP_CLOSE);
        app_auracast_uninit();
#endif
        break;
    case BT_MODE_CIG:

        break;
    }

    g_bt_hdl.last_work_mode = g_bt_hdl.work_mode;
    g_bt_hdl.work_mode = mode;

    switch (mode) {
    case BT_MODE_SIGLE_BOX:
        dual_conn_page_device();
        break;
    case BT_MODE_TWS:
#if TCFG_USER_TWS_ENABLE
        bt_tws_onoff(1);
#endif
        break;
    case BT_MODE_BROADCAST:
        dual_conn_page_device();
#if LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN
        app_broadcast_init();
        app_broadcast_open();
        le_audio_scene_deal(LE_AUDIO_APP_OPEN);
#endif
        break;
    case BT_MODE_AURACAST:
        dual_conn_page_device();
#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        app_auracast_init();
        app_auracast_open();
        le_audio_scene_deal(LE_AUDIO_APP_OPEN);
#endif
        break;
    case BT_MODE_CIG:

        break;
    }
    return 0;
}

void bt_work_mode_switch_to_next(void)
{
    static u8 work_mode = BT_MODE_SIGLE_BOX;
    work_mode ++;
#if TCFG_USER_TWS_ENABLE == 0
#if LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN
    if (work_mode == BT_MODE_TWS) {
        work_mode = BT_MODE_BROADCAST;
    }
#elif (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    if (work_mode == BT_MODE_TWS) {
        work_mode = BT_MODE_AURACAST;
    }
#endif
#endif

#if (!(LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN))
    if (work_mode ==  BT_MODE_BROADCAST) {
        work_mode =  BT_MODE_AURACAST;
    }
#endif

#if (!(TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)))
    if (work_mode ==  BT_MODE_AURACAST) {
        work_mode =  BT_MODE_CIG;
    }
#endif
    if (work_mode == BT_MODE_CIG) {
        work_mode = BT_MODE_SIGLE_BOX;
    }

    bt_work_mode_select(work_mode);
}

