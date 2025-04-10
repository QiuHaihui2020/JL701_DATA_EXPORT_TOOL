#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key_tone.data.bss")
#pragma data_seg(".key_tone.data")
#pragma const_seg(".key_tone.text.const")
#pragma code_seg(".key_tone.text")
#endif
#include "fs/resfile.h"
#include "app_main.h"
#include "app_tone.h"
#include "key_driver.h"

static u8 g_have_key_tone_file = 0;

static int key_tone_msg_handler(int *msg)
{
    struct key_event *key = (struct key_event *)msg;

    if (app_in_mode(APP_MODE_IDLE)) {
        return 0;
    }

    if (g_have_key_tone_file == 0) {
        char file_path[48];
        strcpy(file_path, FLASH_RES_PATH);
        strcpy(file_path + strlen(FLASH_RES_PATH), get_tone_files()->key_tone);
        void *file = resfile_open(file_path);
        if (file) {
            g_have_key_tone_file = 1;
            resfile_close(file);
        } else {
            g_have_key_tone_file = 0xff;
        }
    }

    /*printf("key_tone_msg_handler: %d, %d\n", key->event, g_have_key_tone_file);*/

    switch (key->event) {
    case KEY_ACTION_HOLD:
    case KEY_ACTION_HOLD_3SEC:
    case KEY_ACTION_HOLD_5SEC:
        break;
    default:
        if (g_have_key_tone_file == 1) {
            play_key_tone_file(get_tone_files()->key_tone);
        }
        break;
    }
    return 0;
}

APP_MSG_HANDLER(key_tone_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_KEY,
    .handler    = key_tone_msg_handler,
};
