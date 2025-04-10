#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key_tone_player.data.bss")
#pragma data_seg(".key_tone_player.data")
#pragma const_seg(".key_tone_player.text.const")
#pragma code_seg(".key_tone_player.text")
#endif
#include "tone_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "app_config.h"

static struct tone_player *g_player = NULL;
static OS_MUTEX g_mutex;
static u8 g_play_cnt = 0;

extern const struct stream_file_ops tone_file_ops;

static void key_tone_player_callback(void *_player_id, int event)
{
    switch (event) {
    case STREAM_EVENT_START:
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        struct tone_player *player = g_player;
        if (!player || player->player_id != (u8)_player_id) {
            break;
        }
        if (g_play_cnt) {
            g_play_cnt--;
            int err = jlstream_start(player->stream);
            if (err == 0) {
                break;
            }
            g_play_cnt = 0;
        }
        g_player = NULL;
        jlstream_release(player->stream);
        free(player);
        break;
    }
}


static int key_tone_player_start(struct tone_player *player)
{
    int err = -EINVAL;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"tone");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_KEY_TONE);
    if (!player->stream) {
        goto __exit;
    }

    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, key_tone_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_dec_file(player->stream, player, &tone_file_ops);

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit;
    }
    g_player = player;

    return 0;

__exit:
    if (player->stream) {
        jlstream_release(player->stream);
    }
    if (player->file) {
        resfile_close(player->file);
        player->file = NULL;
    }
    free(player);
    return err;
}

int play_key_tone_file(const char *file_name)
{
    struct tone_player *player;

    if (g_player) {
        if (g_play_cnt < 10) {
            g_play_cnt++;
        }
        return 0;
    }
    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }
    int err = tone_player_init(player, file_name);
    if (err) {
        free(player);
        return err;
    }
    player->scene = STREAM_SCENE_KEY_TONE;

    return key_tone_player_start(player);
}

u8 key_tone_player_running(void)
{
    return g_player ? 1 : 0;
}

