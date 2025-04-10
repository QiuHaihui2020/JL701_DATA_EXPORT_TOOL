#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".dev_flow_recoder.data.bss")
#pragma data_seg(".dev_flow_recoder.data")
#pragma const_seg(".dev_flow_recoder.text.const")
#pragma code_seg(".dev_flow_recoder.text")
#endif
#include "jlstream.h"
#include "dev_flow_recoder.h"

struct dev_flow_recoder {
    struct jlstream *stream;
};

static struct dev_flow_recoder *g_dev_flow_recoder = NULL;


static void dev_flow_recoder_callback(void *private_data, int event)
{
    struct dev_flow_recoder *recoder = g_dev_flow_recoder;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

//自定义数据流recoder 启动
int dev_flow_recoder_open(void)
{
    int err = 0;
    struct dev_flow_recoder *recoder;

    if (g_dev_flow_recoder) {
        return -EBUSY;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"dev_flow");
    if (uuid == 0) {
        return -EFAULT;
    }

    recoder = malloc(sizeof(*recoder));
    if (!recoder) {
        return -ENOMEM;
    }

    /*
       导入自定义音频流的源节点UUID，如下ADC节点为NODE_UUID_ADC,
       若是其他源节点，需使用对应节点的UUID
     */
    recoder->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);
    if (!recoder->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    /*-----以下用于设置当前数据对应节点的特性-----*/

    //设置当前数据流-自定义输出节点的参数
    /* struct stream_fmt fmt = {0}; */
    /* err = jlstream_node_ioctl(recoder->stream, NODE_UUID_SINK_DEV, NODE_IOC_SET_FMT, (int)(&fmt)); */
    //设置当前数据流-ADC的中断点数
    /* err += jlstream_node_ioctl(recoder->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, 256); */
    if (err) {
        goto __exit1;
    }
    jlstream_set_callback(recoder->stream, recoder->stream, dev_flow_recoder_callback);
    jlstream_set_scene(recoder->stream, STREAM_SCENE_DEV_FLOW);
    err = jlstream_start(recoder->stream);
    if (err) {
        goto __exit1;
    }

    g_dev_flow_recoder = recoder;

    return 0;

__exit1:
    jlstream_release(recoder->stream);
__exit0:
    free(recoder);
    return err;
}

//自定义数据流recoder 查询是否活动
bool dev_flow_recoder_runing(void)
{
    return g_dev_flow_recoder != NULL;
}

//自定义数据流recoder 关闭
void dev_flow_recoder_close(void)
{
    struct dev_flow_recoder *recoder = g_dev_flow_recoder;

    if (!recoder) {
        return;
    }
    jlstream_stop(recoder->stream, 0);
    jlstream_release(recoder->stream);

    free(recoder);
    g_dev_flow_recoder = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_RECODER, (int)"dev_flow");
}



