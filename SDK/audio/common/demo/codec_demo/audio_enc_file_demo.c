//////////////////////////////////////////////////////////////
//
//	文件类编码 demo
//  适用于 mp3 mp2 adpcm 等文件类编码
//
//////////////////////////////////////////////////////////////
#include "audio_config.h"
#include "encoder_node.h"
#include "audio_base.h"
#include "media/audio_encoder.h"
#include "codec/audio_encode_common_api.h"
#include "system/timer.h"
#include "clock.h"

static int audio_enc_demo_read_data(void *priv, u8 *buf, int len);
static int audio_enc_demo_output(void *priv, u8 *data, int len);

//不同格式需要注意的地方： diff_type_notice;

struct audio_enc_demo {
    ENC_OPS *ops; 		//编码器
    EN_FILE_IO IO; 		//编码器输入输出接口配置
    ENC_DATA_INFO info; //编码参数配置
    void *priv;    		//私有参数 其他模块与编码流程交互的句柄
    void *enc_buf;
    s16 *indata;
    s16 indata_len;
    u8 start;
};
//mp3 编码器
extern ENC_OPS *get_mp2_ops(); //mp2
extern ENC_OPS *get_mpl3_ops();//mp3

//adpcm 编码器
extern ENC_OPS *get_msenadpcm_ops(); //ms
extern ENC_OPS *get_imaenadpcm_ops();//ima
//初始化编码流程,申请资源
static void *audio_enc_demo_open(void *priv)
{
    struct audio_enc_demo *enc_demo = zalloc(sizeof(struct audio_enc_demo));
    if (enc_demo) {
        enc_demo->priv = priv; //配置私有参数
        enc_demo->start = 0;
        return enc_demo;
    }
    return NULL;
}

//diff_type_notice;
//编码器主动读pcm数据接口
//mp3 adpmc 等是区分左右声道读数据的，有些是不区分的，直接给数据就行;
#if 0  //直接读需要的数据 :opus
static u16 audio_enc_demo_input_data(void *priv, s16 *buf,  u16 point)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;

    int rlen = 0;
    int len = point << 1
              rlen =  audio_enc_demo_read_data(enc_demo->priv, (u8 *)buf, len);
    return rlen >> 1;
}

#else// 区分左右声道读数据 : adpcm mp3

static u16 audio_enc_demo_input_data(void *priv, s16 *buf, u8 channel, u16 point)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    int rlen = 0;
    int len = point << 1;

    if (enc_demo->info.nch == 1) {
        rlen = audio_enc_demo_read_data(enc_demo->priv, (u8 *)buf, len);
        printf("input : %d, %d\n", point, rlen);
        return rlen >> 1;
    } else {
        if (enc_demo->indata_len < len) {
            if (enc_demo->indata) {
                free(enc_demo->indata);
                enc_demo->indata = NULL;
            }
        }
        enc_demo->indata_len = len << 2;
        if (!enc_demo->indata) {
            enc_demo->indata = zalloc(enc_demo->indata_len);
            ASSERT(enc_demo->indata != NULL);
        }
        int i = 0;
        if (channel == 0) {
            rlen = audio_enc_demo_read_data(enc_demo->priv, (u8 *)enc_demo->indata, len << 1);
            printf("input : %d, %d\n", point, rlen);
            if (rlen < (len << 1)) {
                return 0;
            }
            for (i = 0; i < point; i++) {
                buf[i] = enc_demo->indata[2 * i];
            }
        } else {
            for (i = 0; i < point; i++) {
                buf[i] = enc_demo->indata[2 * i  + 1];
            }
        }
        return point;
    }
}
#endif

//编码器输出编码数据接口
static u32 audio_enc_demo_output_data(void *priv, u8 *buf, u16 len)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;

    //diff_type_notice;
    if (!enc_demo->start) {
        //未开始编码的输出是给头信息占位置的，非编码成文件，如果不需要可以舍掉
    }

    return audio_enc_demo_output(priv, buf, len); //数据输出;

    /* return len  */
}


//初始化编码器，配置编码参数，
static int audio_enc_demo_start(void *priv)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;

    if (!enc_demo) {
        return -EINVAL;
    }
    if (enc_demo->start) { //避免重复初始化编码器
        return 0;
    }

    //diff_type_notice;
    //根据格式获取对应的编码器句柄;
    enc_demo->ops = get_mpl3_ops();
    /* enc_demo->ops = get_mp2_ops(); */
    /* enc_demo->ops = get_msenadpcm_ops(); */
    /* enc_demo->ops = get_imaenadpcm_ops(); */

    ASSERT(enc_demo->ops != NULL);

    u32 need_buf_size = enc_demo->ops->need_buf((enc_demo->info.nch == 1) ? 1 : 2); //根据声道类型，获取编码器使用的buf 大小
    enc_demo->enc_buf = zalloc(need_buf_size);
    ASSERT(enc_demo->enc_buf != NULL);

    //编码器配置输入输出接口
    enc_demo->IO.priv = enc_demo;
    enc_demo->IO.input_data  = audio_enc_demo_input_data;
    enc_demo->IO.output_data = audio_enc_demo_output_data;


    //diff_type_notice;
    //打开编码器 配置输入输出接口,
    //adpcm 的这个流程会调用输出接口，输出一段数据，这个是给头信息占位置的，编码结束需要重新获取头信息到这个位置。
    //一般是编码成文件，编码结束后重新写头信息用的， 如果不需要，可以舍掉本次输出的数据。
    enc_demo->ops->open(enc_demo->enc_buf, &enc_demo->IO);

    //设置编码参数
    enc_demo->ops->set_info(enc_demo->enc_buf, &enc_demo->info);

    //初始化编码器，可以进行编码了;
    enc_demo->ops->init(enc_demo->enc_buf);

    enc_demo->start = 1;
    printf("audio_enc_demo start \n");
    return 0;
}

//设置编码格式 注：不同的类型可能需要配的参数也不一样
static void audio_enc_demo_set_fmt(void *priv, struct audio_fmt *fmt)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    if (enc_demo) {
        enc_demo->info.sr  = fmt->sample_rate;

        //diff_type_notice;
        enc_demo->info.br  = fmt->bit_rate / 1000;	 //mp3  码率 设置为真实码率/ 1000
        /* enc_demo->info.br  = 512; 				// adpcm 的码率 参数对用的是块大小 就是编码一次输出的大小 */

        enc_demo->info.nch = fmt->channel;

        printf("enc_demo : sr = %d,br = %d, nch = %d\n", enc_demo->info.sr, enc_demo->info.br, enc_demo->info.nch);
    }
}

//获取编码当前时间 : 秒
static int audio_enc_demo_get_cur_time(void *priv)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    int ret = 0;

    if (enc_demo && enc_demo->enc_buf) {
        if (enc_demo->ops && enc_demo->ops->get_time) {
            ret = enc_demo->ops->get_time(enc_demo->enc_buf);
        }
    }
    return ret;

}

//获取编码的头文件信息 注：有头信息的编码才需要，如: adpcm
static u8 *audio_enc_demo_get_head_info(void *priv, int *head_len)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    if (enc_demo && enc_demo->enc_buf && head_len) {
        return (u8 *)enc_demo->ops->write_head(enc_demo->enc_buf, (u16 *)head_len);
    }
    return NULL;
}


//编码运行主函数，调用编码器的run, 会自动回调读数据和编码数据接口
static int audio_enc_demo_run(void *priv)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    int ret  = 0;
    if (enc_demo->start == 0) {
        return -EINVAL;
    }

    if (enc_demo->ops) {
        putchar('R');
        ret = enc_demo->ops->run(enc_demo->enc_buf);
        if (ret) { // 编码错误处理

        }
    }

    return ret;
}


static void audio_enc_demo_close(void *priv)
{
    struct audio_enc_demo *enc_demo = (struct audio_enc_demo *)priv;
    printf("audio_enc_demo_close \n");
    if (enc_demo) {
        enc_demo->start = 0;
        if (enc_demo->enc_buf) {
            free(enc_demo->enc_buf);
            enc_demo->enc_buf = NULL;
        }
        if (enc_demo->indata) {
            free(enc_demo->indata);
            enc_demo->indata = NULL;
        }
        free(enc_demo);
    }
}


/////////////////////////////////////////////////////////////////////////////
///// 				  编码流程使用demo
/////
////////////////////////////////////////////////////////////////////////////
#define AUDIO_ENC_DEMO_TASK              "audio_enc_demo_task"

struct audio_enc_demo_hdl {
    struct audio_enc_demo *enc;
    u8 start;
    u16 timer; //测试timer
};

#define AUDIO_ENC_DEMO_RUN             0

static void audio_enc_demo_test_task(void *priv)
{
    struct audio_enc_demo_hdl *hdl = (struct audio_enc_demo_hdl *)priv;
    int res = 0;
    int msg[16];
    u8 pend_taskq = 1;
    while (1) {
        res = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case AUDIO_ENC_DEMO_RUN:
                hdl = (struct audio_enc_demo_hdl *)msg[2];
                audio_enc_demo_run(hdl->enc);  //编码主函数
                break;
            }
        }
    }

}
/*
 16k 正选波 pcm 数据
*/
static short const sin_16k[16] = {
    0x0000, 0x30fd, 0x5a83, 0x7641, 0x7fff, 0x7642, 0x5a82, 0x30fc, 0x0000, 0xcf04, 0xa57d, 0x89be, 0x8000, 0x89be, 0xa57e, 0xcf05,
};
static u16 tx1_s_cnt = 0;
static int get_sine_data(u16 *s_cnt, s16 *data, u16 points, u8 ch)
{
    while (points--) {
        if (*s_cnt >= 16) {
            *s_cnt = 0;
        }
        *data++ = sin_16k[*s_cnt] / 2;
        if (ch == 2) {
            *data++ = sin_16k[*s_cnt] / 2;
        }
        (*s_cnt)++;
    }
    return 0;
}



//外部给数据接口，把需要编码的数据写入到buf ,这里的pcm 数据的声道数要与编码参数一致
static int audio_enc_demo_read_data(void *priv, u8 *buf, int len)
{
    /* //如果输入不够，不读数据 */
    /* if(data_len < len){			 */
    /* return 0;		 */
    /* } */

    //这里模拟填正弦波数据，
    /* get_sine_data(&tx1_s_cnt, (s16 *)buf, len/2 ,2);//双声道编码 */
    get_sine_data(&tx1_s_cnt, (s16 *)buf, len / 2, 1); //单声道编码

    return len;
}
//编码数据输出接口,可以进行存储，写文件等操作
static int audio_enc_demo_output(void *priv, u8 *data, int len)
{
    struct audio_enc_demo_hdl *hdl = (struct audio_enc_demo_hdl *)priv;
    /* int wlen = 0; */
    /* wlen = write_file(file,data,len); */
    /* return wlen;	 */

    y_printf("enc out len = %d\n", len);


    return len;
}

static void enc_timer(void *priv)
{
    putchar('M');
    os_taskq_post_msg(AUDIO_ENC_DEMO_TASK, 2, AUDIO_ENC_DEMO_RUN, (int)priv);
}

void *audio_enc_demo_test()
{
    struct audio_enc_demo_hdl *hdl = zalloc(sizeof(struct audio_enc_demo_hdl));
    ASSERT(hdl != NULL);

    //diff_type_notice;
    /*配置编解码格式信息: 所需格式参数 不同格式需要的参数可能不一致，不限于下面几种*/
    struct audio_fmt fmt = {0};
    fmt.coding_type =  AUDIO_CODING_WAV; //编码类型，根据不用的类型使用不同的编码器
    fmt.sample_rate = 16000;
    fmt.channel = 1;
    fmt.bit_rate = 64000;

    printf("fmt: 0x%x,%d,%d \n", fmt.coding_type, fmt.sample_rate,  fmt.channel);

    hdl->enc = audio_enc_demo_open(hdl);
    if (!hdl->enc) {
        printf("audio_enc_demo_open failed\n");
        return NULL;
    }
    audio_enc_demo_set_fmt(hdl->enc, &fmt);
    audio_enc_demo_start(hdl->enc);


    hdl->start = 1;
    int ret = os_task_create(audio_enc_demo_test_task, hdl, 5, 512, 32, AUDIO_ENC_DEMO_TASK);

    hdl->timer = sys_timer_add(hdl, enc_timer, 1000); //测试流程 编码驱动timer;

    printf("== audio_enc_demo_test_open success\n");
    return hdl;

}

void audio_enc_demo_test_close(void *priv)
{
    struct audio_enc_demo_hdl *hdl = (struct audio_enc_demo_hdl *)priv;
    printf("== audio_enc_demo_test_close \n");
    if (hdl) {
        hdl->start = 0;
        if (hdl->timer) {
            sys_timer_del(hdl->timer);
            hdl->timer = 0;
        }
        audio_enc_demo_close(hdl->enc);
        os_task_del(AUDIO_ENC_DEMO_TASK);
    }

}





