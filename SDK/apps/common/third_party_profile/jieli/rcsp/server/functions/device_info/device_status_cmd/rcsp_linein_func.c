#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_linein_func.data.bss")
#pragma data_seg(".rcsp_linein_func.data")
#pragma const_seg(".rcsp_linein_func.text.const")
#pragma code_seg(".rcsp_linein_func.text")
#endif
#include "rcsp_linein_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "app_action.h"
#include "app_msg.h"
#include "key_event_deal.h"
#include "tone_player.h"
#include "JL_rcsp_attr.h"

#if RCSP_MODE && TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE
#include "linein.h"

#define LINEIN_INFO_ATTR_STATUS		(0)
//设置固件linein行为
bool rcsp_linein_func_set(void *priv, u8 *data, u16 len)
{


    /* printf("\nrcsp_linein_func_set\n\n"); */

    /* if (0 != linein_get_status()) { */
    /*     return true; */
    /* } */

    u8 fun_cmd = data[0];
    u16 param_len = len - 1;
    /* printf("\n fun_cmd %d\n\n",fun_cmd); */
    switch (fun_cmd) {
    case 3:
        app_send_message(APP_MSG_MUSIC_PP, 0);
        break;
    default:
        break;
    }

    return true;
}

//获取固件linein信息
u32 rcsp_linein_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
    if (mask & BIT(LINEIN_INFO_ATTR_STATUS)) {
        u8 status = linein_get_status();
        offset = add_one_attr(buf, buf_size, offset, LINEIN_INFO_ATTR_STATUS, (u8 *)&status, sizeof(status));
    }
    return offset;
}

void rcsp_linein_msg_deal(int msg, u8 ret)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return ;
    }

    switch (msg) {
    case  APP_MSG_VOL_DOWN:
    case  APP_MSG_VOL_UP:
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_VOL));
        break;
    case (int)-1:
    case APP_MSG_MUSIC_PP:
    case APP_MSG_LINEIN_START:
        rcsp_device_status_update(LINEIN_FUNCTION_MASK, BIT(LINEIN_INFO_ATTR_STATUS));
        break;
    }
}

//停止linein功能
void rcsp_linein_func_stop(void)
{
#if RCSP_MSG_DISTRIBUTION_VER != RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL
    if (linein_get_status()) {
        app_send_message(APP_MSG_MUSIC_PP, 0);
    }
#endif
}

#endif
