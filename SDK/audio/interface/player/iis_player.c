#include "jlstream.h"
#include "iis_player.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_config_def.h"
#include "scene_switch.h"


struct iis_file_player {
    struct jlstream *stream;
};

static struct iis_file_player *g_iis_player = NULL;
static u8 wait_iis_player_open_flag = 0;
static u8 wait_iis_player_close_flag = 0;

static void iis_player_callback(void *private_data, int event)
{
    struct iis_file_player *player = g_iis_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
#ifdef TCFG_VOCAL_REMOVER_NODE_ENABLE
        musci_vocal_remover_update_parm();
#endif
        break;
    }
}


int iis_player_open(void)
{
    int err;
    struct iis_file_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"iis");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_IIS0_RX);

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置 IIS 中断点数
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_IIS_IRQ_POINTS);
    jlstream_node_ioctl(player->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_IIS_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数
    jlstream_set_callback(player->stream, player->stream, iis_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_IIS);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    g_iis_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool iis_player_runing(void)
{
    return g_iis_player != NULL;
}

void iis_player_close()
{
    struct iis_file_player *player = g_iis_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_iis_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"iis");
}

static void iis_open_player(int arg)
{
    printf("================ open iis player\n");
    if (!g_iis_player) {
        iis_player_open();
    }
    wait_iis_player_open_flag = 0;
}

/*
 * @description: 在app_core 线程打开 iis 数据流
 * @return：0 表示消息发送成功
 *			-1 表示已经成功发送了消息
 * @node:
 */
int iis_open_player_by_taskq(void)
{
    if (wait_iis_player_open_flag == 1) {
        return -1;
    }
    int msg[2];
    msg[0] = (int)iis_open_player;
    msg[1] = 0;
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    if (ret == 0) {
        //消息发送成功, 等待iis player 打开
        wait_iis_player_open_flag = 1;
    }
    return ret;
}

static void iis_close_player(int arg)
{
    printf("================ close iis player\n");
    if (g_iis_player) {
        iis_player_close();
    }
    wait_iis_player_close_flag = 0;
}

/*
 * @description: 在app_core 线程关闭 iis 数据流
 * @return：0 表示消息发送成功
 *			-1 表示已经成功发送了消息
 * @node:
 */
int iis_close_player_by_taskq(void)
{
    if (wait_iis_player_close_flag == 1) {
        return -1;
    }
    int msg[2];
    msg[0] = (int)iis_close_player;
    msg[1] = 0;
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 2, msg);
    if (ret == 0) {
        //消息发送成功, 等待iis player 打开
        wait_iis_player_close_flag = 1;
    }
    return ret;
}




