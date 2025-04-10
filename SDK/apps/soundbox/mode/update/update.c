#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".update.data.bss")
#pragma data_seg(".update.data")
#pragma const_seg(".update.text.const")
#pragma code_seg(".update.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "app_tone.h"
#include "audio_config.h"
#include "media/includes.h"
#include "mic_effect.h"


#if 1

static u8 g_resfile_writing = 0;

struct app_mode *app_enter_update_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    r_printf("app_enter_update_mode\n");

#if TCFG_MIC_EFFECT_ENABLE
    mic_effect_player_pause(1);
#endif
    jlstream_global_lock();


    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), NULL)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            if (g_resfile_writing) {
                continue;
            }
            break;
        }
        switch (msg[0]) {
        case MSG_FROM_APP:
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    r_printf("app_exit_update_mode\n");

    jlstream_global_unlock();

#if TCFG_MIC_EFFECT_ENABLE
    mic_effect_player_pause(0);
#endif

    return next_mode;
}

static int update_mode_try_enter()
{
    return 0;
}

static int update_mode_try_exit()
{
    if (g_resfile_writing) {
        return -EBUSY;
    }
    return 0;
}

static const struct app_mode_ops update_mode_ops = {
    .try_enter          = update_mode_try_enter,
    .try_exit           = update_mode_try_exit,
};

/*
 * 注册spdif模式
 */
REGISTER_APP_MODE(update_mode) = {
    .name 	= APP_MODE_UPDATE,
    .index  = 0xff,
    .ops 	= &update_mode_ops,
};


static int app_update_prob_handler(int *msg)
{
    if (msg[0] == APP_MSG_WRITE_RESFILE_START) {
        g_resfile_writing = 1;
        struct app_mode *mode = app_get_current_mode();
        if (mode) {
            app_push_mode(mode->name);
        }
        app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_UPDATE, 0);
    } else if (msg[0] == APP_MSG_WRITE_RESFILE_STOP) {
        g_resfile_writing = 0;
        int name = app_pop_mode();
        if (name != 0xff) {
            app_send_message2(APP_MSG_GOTO_MODE, name, 0);
        }
    }
    return 0;
}

APP_MSG_PROB_HANDLER(app_update_handler_entry) = {
    .owner  = 0xff,
    .from   = MSG_FROM_APP,
    .handler = app_update_prob_handler,
};


#endif
