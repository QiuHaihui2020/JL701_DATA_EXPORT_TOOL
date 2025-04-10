#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_player.data.bss")
#pragma data_seg(".audio_player.data")
#pragma const_seg(".audio_player.text.const")
#pragma code_seg(".audio_player.text")
#endif
#include "app_msg.h"
#include "audio_manager.h"

void tone_event_to_user(int event, u16 fname_uuid)
{
    int msg[2];

    msg[0] = event;
    msg[1] = fname_uuid;
    os_taskq_post_type("app_core", MSG_FROM_TONE, 2, msg);
}

void tone_event_clear()
{
    os_taskq_del_type("app_core", MSG_FROM_TONE);
}

void update_tone_event_clear()
{
    tone_event_clear();
}

void audio_event_send_msg(int event, int arg)
{
    int msg[2];

    msg[0] = event;
    msg[1] = arg;
    os_taskq_post_type("app_core", MSG_FROM_AUDIO, 2, msg);
}

void audio_event_to_user(int event)
{
    audio_event_send_msg(event, 0);
}

void audio_anc_event_to_user(int mode)
{
    /*if (mode == ANC_ON) {
        audio_event_send_msg(AUDIO_EVENT_ANC_ON, 0);
    } else if (mode == ANC_TRANSPARENCY) {
        audio_event_send_msg(AUDIO_EVENT_ANC_TRANS, 0);
    } else {
        audio_event_send_msg(AUDIO_EVENT_ANC_OFF, 0);
    }*/
}
