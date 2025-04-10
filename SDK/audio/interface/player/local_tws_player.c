
#include "jlstream.h"
#include "classic/tws_api.h"
#include "media/audio_base.h"
#include "media/bt_audio_timestamp.h"
#include "sdk_config.h"
#include "scene_switch.h"
#include "volume_node.h"
#include "app_main.h"
#include "local_tws_player.h"
#include "tws/jl_tws.h"

#if TCFG_LOCAL_TWS_ENABLE

struct local_tws_player {
    struct jlstream *stream;
};


static struct local_tws_player *g_local_tws_player = NULL;

static void local_tws_player_callback(void *private_data, int event)
{
    struct local_tws_player *player = g_local_tws_player;
    printf("local tws player callback : %d\n", event);
    switch (event) {
    case STREAM_EVENT_START:
#ifdef TCFG_VOCAL_REMOVER_NODE_ENABLE
        musci_vocal_remover_update_parm();
#endif
        break;
    }
}

int local_tws_player_open(struct local_tws_player_param *param)
{
    int err;
    int uuid;
    struct local_tws_player *player = g_local_tws_player;
    uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"local_tws");
    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LOCAL_TWS_SINK);
    if (!player->stream) {
        printf("create local_tws stream faild\n");
        free(player);
        return -EFAULT;
    }
    jlstream_set_scene(player->stream, STREAM_SCENE_LOCAL_TWS);

    jlstream_set_callback(player->stream, player, local_tws_player_callback);

    int channel = AUDIO_CH_LR ; //这里设置解码输出的声道类型

    if (param) {
#if 0
        printf("tws_ch:%d", param->tws_channel);
        printf("channel_mode:%d", param->channel_mode);
        printf("sr:%d", param->sample_rate);
        printf("coding_type:%d", param->coding_type);
        printf("durations:%d", param->durations);
        printf("bit_rate:%d", param->bit_rate);
#endif
        jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                            NODE_IOC_SET_BTADDR, (int)param->tws_channel);
        jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE,
                            NODE_IOC_SET_FMT, (int)param);

        channel = param->channel_mode == JL_TWS_CH_MONO ? AUDIO_CH_MIX : AUDIO_CH_LR ;
    }
    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, channel);

    err = jlstream_start(player->stream);
    if (err) {
        jlstream_release(player->stream);
        free(player);
        g_local_tws_player = NULL;
        return err;
    }

    printf("local tws player open success : 0x%x, 0x%x\n", (u32)player, (u32)player->stream);

    g_local_tws_player = player;

    return 0;

}

void local_tws_player_close(void)
{
    struct local_tws_player *player = g_local_tws_player;

    if (!player) {
        return;
    }
    printf("local tws player close :  0x%x, 0x%x\n", (u32)player, (u32)player->stream);
    if (player->stream) {
        jlstream_stop(player->stream, 0);
        jlstream_release(player->stream);
    }
    free(player);
    g_local_tws_player = NULL;
    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"local_tws");


}

bool local_tws_player_is_playing(void)
{
    if (g_local_tws_player) {
        return 1;
    }
    return 0;
}

#endif
