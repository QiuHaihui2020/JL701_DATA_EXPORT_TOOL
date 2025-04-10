#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_pc_func.data.bss")
#pragma data_seg(".rcsp_pc_func.data")
#pragma const_seg(".rcsp_pc_func.text.const")
#pragma code_seg(".rcsp_pc_func.text")
#endif
#include "rcsp_pc_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "app_action.h"
#include "key_event_deal.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_attr.h"
#include "pc_spk_player.h"

#if (RCSP_MODE && TCFG_APP_PC_EN && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE)

#define PC_INFO_ATTR_STATUS		(0)

bool rcsp_pc_func_set(void *priv, u8 *data, u16 len)
{
    r_printf("\n\n\n\nrcsp_pc_func_set\n");
    put_buf(data, len);
    u8 fun_cmd = data[1];
    u8 state = data[2];
    printf("\n\n\nfun_cmd %d\n", fun_cmd);
    switch (fun_cmd) {
    case 0:
        app_send_message(APP_MSG_MUSIC_PP, 0);
        break;
    case 1:
        if (state == 1) {
            app_send_message(APP_MSG_MUSIC_NEXT, 0);
        } else if (state == 0) {
            app_send_message(APP_MSG_MUSIC_PREV, 0);
        }
        break;
    default:
        break;
    }

    return true;
}

//获取固件pc信息
u32 rcsp_pc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset  = 0;

    r_printf("\n\n\nget mask %d\n", mask);

    if (mask & BIT(PC_INFO_ATTR_STATUS)) {
        u8 status = pc_spk_player_runing();
        offset = add_one_attr(buf, buf_size, offset, PC_INFO_ATTR_STATUS, (u8 *)&status, sizeof(status));
    }

    return offset;
}

void rcsp_pc_msg_deal(int msg)
{
    r_printf("\n\n\nmsg deal %d\n", msg);
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return ;
    }

    switch (msg) {
    case APP_MSG_MUSIC_PP:
        rcsp_device_status_update(PC_FUNCTION_MASK, BIT(PC_INFO_ATTR_STATUS));
        break;
    }

}

void rcsp_pc_func_stop(void)
{

#if (RCSP_MSG_DISTRIBUTION_VER != RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL)
#endif
}

#endif
