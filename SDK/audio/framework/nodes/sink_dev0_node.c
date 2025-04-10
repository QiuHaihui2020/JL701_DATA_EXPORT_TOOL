#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sink_dev0_node.data.bss")
#pragma data_seg(".sink_dev0_node.data")
#pragma const_seg(".sink_dev0_node.text.const")
#pragma code_seg(".sink_dev0_node.text")
#endif

#include "jlstream.h"
#include "media/audio_base.h"
#include "app_config.h"

#define SINK_DEV0_MSBC_TEST_ENABLE	0		//MSBC编码输出测试

#if TCFG_SINK_DEV0_NODE_ENABLE

struct sink_dev0_hdl {
    struct stream_fmt fmt;		//节点参数
};

/*
   自定义输出节点 初始化
	hdl:节点私有句柄
*/
static int sink_dev0_init(struct sink_dev0_hdl *hdl)
{
    u8 ch_num = AUDIO_CH_NUM(hdl->fmt.channel_mode);	//声道数
    u32 sample_rate = hdl->fmt.sample_rate;				//采样率
    u32 coding_type = hdl->fmt.coding_type;				//数据格式

    printf("sink_dev1_init,ch_num=%x,sr=%d,coding_type=%x\n", ch_num, sample_rate, coding_type);

    //do something
    return 0;
}

/*
   自定义输出节点 关闭
	hdl:节点私有句柄
*/
static int sink_dev0_exit(struct sink_dev0_hdl *hdl)
{
    //do something
    return 0;
}

/*
   自定义输出节点 运行
	hdl:节点私有句柄
 	*data:输入数据地址,位宽16bit
  	data_len :输入数据长度,byte
*/
static void sink_dev0_run(struct sink_dev0_hdl *hdl, void *data, int data_len)
{
    //do something

}

static void sink_dev0_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct sink_dev0_hdl *hdl = (struct sink_dev0_hdl *)iport->node->private_data;
    struct stream_frame *frame;

    while (1) {
        //获取上一个节点的输出
        frame = jlstream_pull_frame(iport, note);
        if (!frame) {
            break;
        }
        sink_dev0_run(hdl, frame->data, frame->len);
        //释放资源
        jlstream_free_frame(frame);
    }
}

static int sink_dev0_bind(struct stream_node *node, u16 uuid)
{
    printf("sink_dev0_bind");
    /* struct sink_dev0_hdl *hdl = (struct sink_dev0_hdl *)node->private_data; */
    return 0;
}

static void sink_dev0_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = sink_dev0_handle_frame;
}

/*
*********************************************************************
*                  sink_dev0_ioc_fmt_nego
* Description: sink_dev0 参数协商
* Arguments  : iport 输入端口句柄
* Return	 : 节点参数协商结果
* Note(s)    : 目的在于检查与上一个节点的参数是否匹配，不匹配则重新协商;
   			   根据输出节点的参数特性，区分为
   				1、固定参数, 或者通过NODE_IOC_SET_FMT获取fmt信息,
   				协商过程会将此参数向前级节点传递, 直至协商成功;
   				2、可变参数，继承上一节点的参数
*********************************************************************
*/
static int sink_dev0_ioc_fmt_nego(struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;	//上一个节点的参数
    /* struct sink_dev0_hdl *hdl = (struct sink_dev0_hdl *)iport->node->private_data; */

    //1、固定节点参数, 向前级节点传递
#if SINK_DEV0_MSBC_TEST_ENABLE
    in_fmt->coding_type = AUDIO_CODING_MSBC;		//数据格式
    in_fmt->sample_rate = 16000;					//采样率
    in_fmt->channel_mode = AUDIO_CH_MIX;			//数据通道
#endif
    printf("sink_dev nego,type=%x,sr=%d,ch_mode=%x\n", in_fmt->coding_type, in_fmt->sample_rate, in_fmt->channel_mode);

    //2、继承前一节点的参数
    /* hdl->fmt = &in_fmt; */
    return NEGO_STA_ACCPTED;
}

static int sink_dev0_ioc_start(struct sink_dev0_hdl *hdl)
{
    printf("sink_dev0_ioc_start");
    sink_dev0_init(hdl);
    return 0;
}

static int sink_dev0_ioc_stop(struct sink_dev0_hdl *hdl)
{
    printf("sink_dev0_ioc_stop");
    sink_dev0_exit(hdl);
    return 0;
}

static int sink_dev0_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    struct sink_dev0_hdl *hdl = (struct sink_dev0_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        sink_dev0_open_iport(iport);
        break;
    case NODE_IOC_START:
        sink_dev0_ioc_start(hdl);
        break;
    case NODE_IOC_STOP:
        sink_dev0_ioc_stop(hdl);
        break;
    case NODE_IOC_SET_FMT:
        //外部传参, 设置当前节点的参数
        /* struct stream_fmt *fmt = (struct stream_fmt *)arg; */
        /* hdl->fmt.coding_type = fmt->coding_type; */
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= sink_dev0_ioc_fmt_nego(iport);
        break;
    case NODE_IOC_GET_DELAY:
        break;
    }

    return 0;
}

//释放当前节点资源
static void sink_dev0_release(struct stream_node *node)
{
    printf("sink_dev0_release");
}

REGISTER_STREAM_NODE_ADAPTER(sink_dev0_adapter) = {
    .name       = "sink_dev0",
    .uuid       = NODE_UUID_SINK_DEV0,
    .bind       = sink_dev0_bind,
    .ioctl      = sink_dev0_ioctl,
    .release    = sink_dev0_release,
    .hdl_size   = sizeof(struct sink_dev0_hdl),
};

#endif

