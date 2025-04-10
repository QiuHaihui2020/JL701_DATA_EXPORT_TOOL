#include "ui/ui.h"
#include "ui/ui_style.h"
#include "app_config.h"
#include "ui/ui_api.h"
#include "system/timer.h"
#include "audio_config.h"
#include "app_power_manage.h"
#include "asm/charge.h"
#include "sdk_config.h"
#include "app_msg.h"

#if TCFG_LOCAL_TWS_ENABLE
#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_SOUNDBOX))

#define STYLE_NAME  JL

static const struct uimsg_handl ui_msg_handler[] = {
    { NULL, NULL},      /* 必须以此结尾！ */
};

static int sink_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***sink_mode_onchange***\n");
        /* key_ui_takeover(1); */
        ui_register_msg_handler(ID_WINDOW_SINK, ui_msg_handler);
        /*
         * 注册APP消息响应
         */
        break;
    case ON_CHANGE_RELEASE:
        /*
         * 要隐藏一下系统菜单页面，防止在系统菜单插入USB进入USB页面
         */
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(ID_WINDOW_SINK)
.onchange = sink_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

#endif
#endif
