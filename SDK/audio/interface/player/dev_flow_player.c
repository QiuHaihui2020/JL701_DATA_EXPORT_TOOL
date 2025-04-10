#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_flow_player.data.bss")
#pragma data_seg(".dev_flow_player.data")
#pragma const_seg(".dev_flow_player.text.const")
#pragma code_seg(".dev_flow_player.text")
#endif
#include "jlstream.h"
#include "sdk_config.h"
#include "audio_config_def.h"
#include "dev_flow_player.h"

struct dev_flow_player {
    struct jlstream *stream;
};

static struct dev_flow_player *g_dev_flow_player = NULL;


static void dev_flow_player_callback(void *private_data, int event)
{
    struct dev_flow_player *player = g_dev_flow_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

//自定义数据流播放器 启动
int dev_flow_player_open(void)
{
    int err;
    struct dev_flow_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"dev_flow");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    /*
       导入自定义音频流的源节点UUID，如下自定义源节点为NODE_UUID_SOURCE_DEV,
       若是其他源节点，需使用对应节点的UUID，如ADC节点则为NODE_UUID_ADC
     */
    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV0);
    /* player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_SOURCE_DEV1); */

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置当前数据对应节点的特性
    /* err = jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, arg); */

    jlstream_set_callback(player->stream, player->stream, dev_flow_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_DEV_FLOW);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    g_dev_flow_player = player;

    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

//自定义数据流播放器 查询是否活动
bool dev_flow_player_runing(void)
{
    return g_dev_flow_player != NULL;
}


//自定义数据流播放器 关闭
void dev_flow_player_close(void)
{
    struct dev_flow_player *player = g_dev_flow_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_dev_flow_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"dev_flow");
}

