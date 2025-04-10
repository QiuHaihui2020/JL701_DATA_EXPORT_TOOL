#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "audio_config.h"
#include "media/includes.h"
#include "system/includes.h"
#include "iis_player.h"
#include "iis.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"

#if TCFG_APP_IIS_EN

int iis_app_msg_handler(int *msg)
{
    const struct key_remap_table *table;

    if (false == app_in_mode(APP_MODE_IIS)) {
        return 0;
    }
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_IIS_START:
        printf("app msg iis start\n");

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        if (app_broadcast_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
        if (app_connected_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }
#endif

        iis_start();
        /* UI_REFLASH_WINDOW(true);//刷新主页并且支持打断显示 */
        break;
    case APP_MSG_MUSIC_PP:
        iis_volume_pp();
        break;
    case APP_MSG_VOL_UP:
        iis_key_vol_up();
        break;
    case APP_MSG_VOL_DOWN:
        iis_key_vol_down();
        break;
    case APP_MSG_PITCH_UP:
        break;
    case APP_MSG_PITCH_DOWN:
        break;
    default:
        app_common_key_msg_handler(msg);
        break;
    }

    return 0;
}



#endif


