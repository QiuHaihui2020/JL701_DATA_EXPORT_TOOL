#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ring_player.data.bss")
#pragma data_seg(".ring_player.data")
#pragma const_seg(".ring_player.text.const")
#pragma code_seg(".ring_player.text")
#endif
#include "tone_player.h"
#include "os/os_api.h"
#include "system/init.h"
#include "app_config.h"

static struct tone_player *g_ring_player = NULL;
static OS_MUTEX g_ring_mutex;

extern const struct stream_file_ops tone_file_ops;

static void ring_player_callback(void *_player_id, int event)
{
    struct tone_player *player = g_ring_player;

    switch (event) {
    case STREAM_EVENT_START:
        os_mutex_pend(&g_ring_mutex, 0);
        if (player && player->player_id == (u8)_player_id) {
            if (player->callback) {
                player->callback(player->priv,  STREAM_EVENT_START);
            }
        }
        os_mutex_post(&g_ring_mutex);
        break;
    case STREAM_EVENT_PREEMPTED:
        break;
    case STREAM_EVENT_NEGOTIATE_FAILD:
    case STREAM_EVENT_STOP:
        os_mutex_pend(&g_ring_mutex, 0);
        if (!player || player->player_id != (u8)_player_id) {
            os_mutex_post(&g_ring_mutex);
            break;
        }
        g_ring_player = NULL;
        jlstream_release(player->stream);
        os_mutex_post(&g_ring_mutex);

        tone_player_free(player);
        break;
    }
}


int ring_player_start(struct tone_player *player)
{
    int err = -EINVAL;

    if (g_ring_player) {
        goto __exit;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"ring");

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_RING);
    if (!player->stream) {
        goto __exit;
    }

    int player_id = player->player_id;
    jlstream_set_callback(player->stream, (void *)player_id, ring_player_callback);
    jlstream_set_scene(player->stream, player->scene);
    jlstream_set_coexist(player->stream, player->coexist);

    if (player->callback) {
        err = player->callback(player->priv, STREAM_EVENT_INIT);
        if (err) {
            goto __exit;
        }
    }
    jlstream_set_dec_file(player->stream, player, &tone_file_ops);

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit;
    }
    g_ring_player = player;

    return 0;

__exit:
    if (player->stream) {
        jlstream_release(player->stream);
    }

    tone_player_free(player);
    return err;
}

static int __play_ring_file(const char *file_name, enum stream_coexist coexist)
{
    struct tone_player *player;

    if (g_ring_player) {
        return -EBUSY;
    }
    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }
    int err = tone_player_init(player, file_name);
    if (err) {
        tone_player_free(player);
        return err;
    }
    player->scene = STREAM_SCENE_RING;
    player->coexist = coexist;

    return ring_player_start(player);
}

int play_ring_file(const char *file_name)
{
    return __play_ring_file(file_name, STREAM_COEXIST_AUTO);
}

int play_ring_file_alone(const char *file_name)
{
    return __play_ring_file(file_name, STREAM_COEXIST_DISABLE);
}

static int __play_ring_file_with_calllback(const char *file_name, enum stream_coexist coexist, void *priv, tone_player_cb_t callback)
{
    struct tone_player *player;

    if (g_ring_player) {
        return -EBUSY;
    }
    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }
    int err = tone_player_init(player, file_name);
    if (err) {
        tone_player_free(player);
        return err;
    }
    player->priv = priv;
    player->callback = callback;
    player->scene = STREAM_SCENE_RING;
    player->coexist = coexist;

    return ring_player_start(player);
}

int play_ring_file_with_callback(const char *file_name, void *priv, tone_player_cb_t callback)
{
    return __play_ring_file_with_calllback(file_name, STREAM_COEXIST_AUTO, priv, callback);
}

int play_ring_file_alone_with_callback(const char *file_name, void *priv, tone_player_cb_t callback)
{
    return __play_ring_file_with_calllback(file_name, STREAM_COEXIST_DISABLE, priv, callback);
}

int ring_player_runing()
{
    return g_ring_player ? 1 : 0;
}


void ring_player_stop()
{
    os_mutex_pend(&g_ring_mutex, 0);
    struct tone_player *player = g_ring_player;
    if (player) {
        if (player->stream) {
            jlstream_stop(player->stream, 50);
            jlstream_release(player->stream);
        }
        g_ring_player = NULL;
    }
    os_mutex_post(&g_ring_mutex);

    if (player) {
        tone_player_free(player);
    }
}

static int __ring_player_init()
{
    os_mutex_create(&g_ring_mutex);
    return 0;
}
__initcall(__ring_player_init);

