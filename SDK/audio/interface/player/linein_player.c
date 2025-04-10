#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".linein_player.data.bss")
#pragma data_seg(".linein_player.data")
#pragma const_seg(".linein_player.text.const")
#pragma code_seg(".linein_player.text")
#endif
#include "jlstream.h"
#include "linein_player.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
struct linein_player {
    struct jlstream *stream;
    s8 linein_pitch_mode;
};

static struct linein_player *g_linein_player = NULL;


static void linein_player_callback(void *private_data, int event)
{
    struct linein_player *player = g_linein_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
#if AUDIO_VBASS_LINK_VOLUME
        vbass_link_volume();
#endif
        break;
    }
}

int linein_player_open()
{
    int err;
    struct linein_player *player;
    if (g_linein_player) {
        return 0;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"linein");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }
    player->linein_pitch_mode = PITCH_0;

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_LINEIN);
    if (!player->stream) {
        player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);
    }

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置中断点数

#ifdef AUDIO_LINEIN_IRQ_POINTS
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_LINEIN_IRQ_POINTS);
    jlstream_node_ioctl(player->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_LINEIN_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数
#else
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);

    jlstream_node_ioctl(player->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数
#endif
    jlstream_set_callback(player->stream, player->stream, linein_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_LINEIN);

#if TCFG_VIRTUAL_SURROUND_PRO_MODULE_NODE_ENABLE
    //解码帧长短得情况下，使用三线程推数
    jlstream_add_thread(player->stream, "media0");
    jlstream_add_thread(player->stream, "media1");
#if defined(CONFIG_CPU_BR28)
    jlstream_add_thread(player->stream, "media2");
#endif
#endif

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    g_linein_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool linein_player_runing()
{
    return g_linein_player != NULL;
}


void linein_player_close()
{
    struct linein_player *player = g_linein_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_linein_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"linein");
}

//变调接口
int linein_file_pitch_up()
{
    struct linein_player *player = g_linein_player;
    if (!player) {
        return -1;
    }
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    player->linein_pitch_mode++;
    if (player->linein_pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        player->linein_pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    printf("play pitch up+++%d\n", player->linein_pitch_mode);
    int ret = linein_file_set_pitch(player->linein_pitch_mode);
    ret = (ret == true) ? player->linein_pitch_mode : -1;
    return ret;
}

int linein_file_pitch_down()
{
    struct linein_player *player = g_linein_player;
    if (!player) {
        return -1;
    }
    player->linein_pitch_mode--;
    if (player->linein_pitch_mode < 0) {
        player->linein_pitch_mode = 0;
    }
    printf("play pitch down---%d\n", player->linein_pitch_mode);
    int ret = linein_file_set_pitch(player->linein_pitch_mode);
    ret = (ret == true) ? player->linein_pitch_mode : -1;
    return ret;
}

int linein_file_set_pitch(enum _pitch_level pitch_mode)
{
    struct linein_player *player = g_linein_player;
    float pitch_param_table[] = {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12};
    if (pitch_mode > ARRAY_SIZE(pitch_param_table) - 1) {
        pitch_mode = ARRAY_SIZE(pitch_param_table) - 1;
    }
    pitch_speed_param_tool_set pitch_param = {
        .pitch = pitch_param_table[pitch_mode],
        .speed = 1,
    };
    if (player) {
        player->linein_pitch_mode = pitch_mode;
        return jlstream_node_ioctl(player->stream, NODE_UUID_PITCH_SPEED, NODE_IOC_SET_PARAM, (int)&pitch_param);
    }
    return -1;
}

void linein_file_pitch_mode_init(enum _pitch_level pitch_mode)
{
    struct linein_player *player = g_linein_player;
    if (player) {
        player->linein_pitch_mode = pitch_mode;
    }
}
