#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_spdif_func.data.bss")
#pragma data_seg(".rcsp_spdif_func.data")
#pragma const_seg(".rcsp_spdif_func.text.const")
#pragma code_seg(".rcsp_spdif_func.text")
#endif
#include "rcsp_spdif_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "app_action.h"
#include "key_event_deal.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_attr.h"
#include "audio_config.h"

#if (RCSP_MODE && TCFG_APP_SPDIF_EN)
#include "spdif_file.h"
#include "spdif_player.h"

#define SPDIF_INFO_ATTR_STATUS       (0)
#define SPDIF_INFO_ATTR_SOURCE       (1)

bool rcsp_spdif_func_set(void *priv, u8 *data, u16 len)
{
    r_printf("\n\n\n\nrcsp_spdif_func_set\n");
    put_buf(data, len);
    u8 fun_cmd = data[1];

    printf("\n\n\nfun_cmd %d\n", fun_cmd);

    switch (fun_cmd) {
    case 0:
        u8 status = data[2];
        app_send_message(APP_MSG_MUSIC_PP, 0);
        break;
    case 1:
        u8 index = data[2];
        app_send_message(APP_MSG_SPDIF_SET_SOURCE, index);
        break;
    default:
        break;
    }

    return true;
}

//获取固件spdif信息
u32 rcsp_spdif_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset  = 0;


    r_printf("\n\n\nget mask %d\n", mask);

    if (mask & BIT(SPDIF_INFO_ATTR_STATUS)) {
        u8 status = spdif_player_runing() && (!app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC));

        printf("spdif %x,%x\n", spdif_player_runing(), app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC));


        offset = add_one_attr(buf, buf_size, offset, SPDIF_INFO_ATTR_STATUS, (u8 *)&status, sizeof(status));
    }

    if (mask & BIT(SPDIF_INFO_ATTR_SOURCE)) {
        u8 source = spdif_get_cur_port_index();
        offset = add_one_attr(buf, buf_size, offset, SPDIF_INFO_ATTR_SOURCE, (u8 *)&source, sizeof(source));
    }

    return offset;
}

void rcsp_spdif_msg_deal(int msg)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return ;
    }

    switch (msg) {
    case APP_MSG_SPDIF_STATUS_UPDATE:
    case APP_MSG_MUSIC_PP:
        rcsp_device_status_update(SPDIF_FUNCTION_MASK, BIT(SPDIF_INFO_ATTR_STATUS));
        break;
    case APP_MSG_SPDIF_SOURCE_UPDATE:
    case APP_MSG_SPDIF_SWITCH_SOURCE:
        rcsp_device_status_update(SPDIF_FUNCTION_MASK, BIT(SPDIF_INFO_ATTR_SOURCE));
        break;
    default:
        break;
    }

}

void rcsp_spdif_func_stop(void)
{

#if (RCSP_MSG_DISTRIBUTION_VER != RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL)
#endif
}

#endif
