#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".app_protocol_deal.data.bss")
#pragma data_seg(".app_protocol_deal.data")
#pragma const_seg(".app_protocol_deal.text.const")
#pragma code_seg(".app_protocol_deal.text")
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_protocol_api.h"
#include "system/includes.h"
#include "tone_player.h"
#include "bt_tws.h"
#include "app_main.h"

#if 0 //AI_APP_PROTOCOL

#if 1
#define APP_PROTOCOL_LOG    printf
#define APP_PROTOCOL_DUMP   put_buf
#else
#define APP_PROTOCOL_LOG(...)
#define APP_PROTOCOL_DUMP(...)
#endif

static int sys_clk_before_rec;
#define AUDIO_ENC_SYS_CLK_HZ     128 * 1000000L

extern void clk_set_en(u8 en);
void mic_rec_clock_set(void)
{
    sys_clk_before_rec = clk_get("sys");
    clk_set("sys", AUDIO_ENC_SYS_CLK_HZ);
    clk_set_en(0);
}

void mic_rec_clock_recover(void)
{
    clk_set_en(1);
    clk_set("sys", sys_clk_before_rec);
}

static void tone_play_in_app_core(int index)
{
    if (app_protocol_get_tone(index)) {
        tone_play_with_callback(app_protocol_get_tone(index), 1, app_speech_tone_play_end_callback, (void *)index);
    } else {
        app_speech_tone_play_end_callback((void *)index, 0);
    }
}

extern u8 key_table[KEY_NUM_MAX][KEY_EVENT_MAX];
int app_protocol_key_event_handler(struct sys_event *event)
{
    int ret = false;
    u8 key_event;

    struct key_event *key = &event->u.key;
    key_event = key_table[key->value][key->event];

    switch (key_event) {
#if APP_PROTOCOL_SPEECH_EN
    case KEY_SEND_SPEECH_START:
        APP_PROTOCOL_LOG("KEY_SEND_SPEECH_START \n");
        app_protocol_start_speech_by_key(event);
        break;
    case KEY_SEND_SPEECH_STOP:
        APP_PROTOCOL_LOG("KEY_SEND_SPEECH_STOP \n");
        app_protocol_stop_speech_by_key();
        break;
#endif
    }
    return ret;
}

#endif

