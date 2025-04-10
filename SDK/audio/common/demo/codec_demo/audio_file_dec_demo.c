//////////////////////////////////////////////////////////////
//
//	文件类解码 demo
//  适用于 dec_demo wav wma 等文件类编码
//	文件解码默认输出双声道数据
//
//////////////////////////////////////////////////////////////
#include "audio_config.h"
#include "audio_base.h"
#include "media/audio_decoder.h"
#include "codec/audio_decode_common_api.h"
#include "audio_def.h"

//-----------------------------------------------------------------------------
//   decoder demo
//-----------------------------------------------------------------------------
struct audio_dec_demo {
    struct if_decoder_io dec_io;
    struct decoder_inf *info;
    struct audio_dec_breakpoint *bp;
    decoder_ops_t *ops;
    deccfg_inf dcf_obj;
    void *priv;
    void *dec_buf;//解码用的work_buf
    void *stk_buf;//解码需要的堆栈scratch

    int ff_fr_step;
    int ff_fr_cnt;
    int time;
    u32 file_len; //文件长度
    void *dest_play_arg;//定点播放的参数
    void *repeat_arg;//无缝循环的参数

    u8 play_mode;
    u8 ch_num ;
    u8 start;
};

extern const int CONFIG_DEC_SUPPORT_CHANNELS;
extern const int CONFIG_DEC_SUPPORT_SAMPLERATE;

extern decoder_ops_t *get_mp3_ops(void);

static int audio_demo_dec_data_read(void *priv, u8 *buf, int len, u32 offset);
/*
 * 数据输入到解码器buf
 */
static int audio_dec_demo_input(void *priv, u32 addr, void *buf, int len, u8 type)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;
    int rlen = 0;
    rlen = audio_demo_dec_data_read(dec_demo->priv, buf, len, addr);

    /* printf("dec input :%d--%d\n",len,rlen); */

    return rlen ;
}




//解码数据输出，数据输出不完，下次run的时候会先输出剩余数据，
static int audio_dec_demo_output(void *priv, void *data, int len)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;

    /*
     * 文件类解码出来的都是双通道

     */
    /* printf("dec out len : %d\n",len); */

    return len;
}

/*
 * 获取文件长度
 */
static u32 audio_dec_demo_get_lslen(void *priv)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;
    if (dec_demo->file_len != 0) {
        return dec_demo->file_len;
    } else {
        return 0x7fffffff;
    }
}

/*
 * 检查数据是否完毕
 */
static int audio_dec_demo_check_buf(void *priv, u32 addr, void *buf)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;

    int rlen = audio_dec_demo_input(dec_demo->priv, addr, buf, 512, addr);
    if (rlen != 512) {
        printf("dec_check_buf read rlen= %d, len= %d\n", rlen, 512);
    }
    return rlen;
}

static u32 audio_dec_demo_store_rev_data(void *priv, u32 addr, int len)
{
    printf("audio_dec_demo_store_rev_data..:len=%d\n", len);

    return len;
}

/*打开解码器，申请资源*/
void *audio_dec_demo_open(void *priv)
{
    struct audio_dec_demo *dec_demo = NULL;
    dec_demo = zalloc(sizeof(struct audio_dec_demo));
    ASSERT(dec_demo != NULL);
    dec_demo->priv = priv;
    dec_demo->bp = NULL;
    //配置解码支持的最大采样率，最大声道数，和解码输出位宽
    dec_demo->dcf_obj.bitwidth = 16;//   24bit 或者 16bit
    dec_demo->dcf_obj.channels = CONFIG_DEC_SUPPORT_CHANNELS;  //  支持的最大声道数
    dec_demo->dcf_obj.sr = CONFIG_DEC_SUPPORT_SAMPLERATE;    //  支持的最大采样率

    return dec_demo;
}



extern decoder_ops_t *get_wtgv2dec_ops(void);
/*初始化一些解码器配置，准备开始编码*/
int audio_dec_demo_start(void *priv)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;

    if (dec_demo->start) {
        return 0;
    }

    dec_demo->ops = get_mp3_ops(); // 获取对应类型的解码器
    /* dec_demo->ops =  get_wtgv2dec_ops(); */

    ASSERT(dec_demo->ops != NULL);


    /*
     * 创建解码工作区BUF
     */
    u32 dcbuf_size = dec_demo->ops->need_dcbuf_size(&dec_demo->dcf_obj);
    dec_demo->dec_buf = zalloc(dcbuf_size);
    ASSERT(dec_demo->dec_buf != NULL);

    //堆栈设置
    u32 stk_size = dec_demo->ops->need_skbuf_size(&dec_demo->dcf_obj);
    if (stk_size) {
        dec_demo->stk_buf = zalloc(stk_size);
        ASSERT(dec_demo->stk_buf != NULL);
    }

    //配置解码器回调接口
    dec_demo->dec_io.priv = dec_demo;
    dec_demo->dec_io.input = audio_dec_demo_input;
    dec_demo->dec_io.output = audio_dec_demo_output;
    dec_demo->dec_io.check_buf = audio_dec_demo_check_buf;
    dec_demo->dec_io.get_lslen = audio_dec_demo_get_lslen;
    dec_demo->dec_io.store_rev_data = audio_dec_demo_store_rev_data;

    /*
     * 打开解码器
     */
    if (dec_demo->ops->open) {
        int bp_len = dec_demo->ops->need_bpbuf_size(&dec_demo->dcf_obj);
        if (dec_demo->bp && dec_demo->bp->len == bp_len) {
            dec_demo->ops->open(dec_demo->dec_buf, dec_demo->stk_buf, &dec_demo->dec_io, dec_demo->bp->data, &dec_demo->dcf_obj);//从断点开始解码
        } else {
            dec_demo->ops->open(dec_demo->dec_buf, dec_demo->stk_buf, &dec_demo->dec_io, NULL, &dec_demo->dcf_obj);//没有断点信息，从头开始解码
        }
    }

    /*
     * 格式检查
     */
    if (dec_demo->ops->format_check) {
        int err = dec_demo->ops->format_check(dec_demo->dec_buf, dec_demo->stk_buf);
        if (err) { //格式检查出错返回不支持解码;
            printf("dec_demo format_check err! 0x%x\n", err);
            return err;
        }
    }

    /*
     * 获取解码信息
     */
    if (dec_demo->ops->get_dec_inf) {
        dec_demo->info = dec_demo->ops->get_dec_inf(dec_demo->dec_buf);
        printf("dec_demo info.sr = %d\n", dec_demo->info->sr);
        printf("dec_demo info.br = %d\n", dec_demo->info->br);
        printf("dec_demo info.nch = %d\n", dec_demo->info->nch);
        printf("dec_demo info.total_time = %d\n", dec_demo->info->total_time);
    }
    dec_demo->play_mode = 0;


    AUDIO_DECODE_PARA dec_para;
    dec_para.mode = 1;/*1表示output的时候，要判断实际输出长度，0表示默认全部输出完毕*/
    dec_demo->ops->dec_config(dec_demo->dec_buf, SET_DECODE_MODE, &dec_para.mode);


    dec_demo->start = 1;
    return 0;
}

static int audio_dec_demo_get_fmt(void *priv, struct audio_fmt *fmt)
{
    int err;
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;

    if (dec_demo->start == 0) {
        err = audio_dec_demo_start(dec_demo);
        if (err) {
            return err;
        }
    }

    fmt->sample_rate = dec_demo->info->sr;
    fmt->channel = dec_demo->info->nch;

    return 0;
}


static int audio_dec_demo_run(void *priv)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;
    int ret = 0;
    if (dec_demo->ops->run) {
        //run 的过程中会调用input 读取数据，并且会通过output解口输出数据;
        ret = dec_demo->ops->run(dec_demo->dec_buf, dec_demo->play_mode, dec_demo->stk_buf);
        //根据返回值做解码错误的处理
        if (ret == 0x50) {
            //log_info("dec_demo busy now\n");
            ret = 0;
        }
        if (ret == MAD_ERROR_FILE_END) {
            //文件结束
        }

    }

    return ret;
}

static int audio_dec_demo_close(void *priv)
{
    struct audio_dec_demo *dec_demo = (struct audio_dec_demo *)priv;
    if (dec_demo) {
        dec_demo->start = 0;
        if (dec_demo->dec_buf) {
            free(dec_demo->dec_buf);
            dec_demo->dec_buf = NULL;
        }
        if (dec_demo->stk_buf) {
            free(dec_demo->stk_buf);
            dec_demo->stk_buf = NULL;
        }
        free(dec_demo);
        dec_demo = NULL;
    }
    return 0;
}


////////////////////////////////////////
////////     dec_demo test     /////////
////////////////////////////////////////


#include "fs/resfile.h"
#include "fs/fs.h"
#include "app_tone.h"

#define AUDIO_DEC_DEMO_TASK              "audio_dec_demo_task"

struct audio_dec_demo_hdl {
    struct audio_dec_demo *dec;
    u8 start;
    void *file;
};

void audio_dec_demo_test_close(void *priv);
static void audio_dec_demo_test_task(void *priv)
{
    struct audio_dec_demo_hdl *hdl = (struct audio_dec_demo_hdl *)priv;
    int res = 0;
    int msg[16];
    u8 pend_taskq = 1;

    while (1) {
        res = audio_dec_demo_run(hdl->dec);
        if (res) {
            /* printf("res = %d\n",res);	 */
            break;
        }
        os_time_dly(100);
    }

    audio_dec_demo_test_close(hdl->dec);
}


//外部给数据接口，把需要编码的数据写入到buf ,这里的pcm 数据的声道数要与编码参数一致
static int audio_demo_dec_data_read(void *priv, u8 *buf, int len, u32 offset)
{
    struct audio_dec_demo_hdl *hdl = (struct audio_dec_demo_hdl *)priv;
    /* printf("== offset = %d\n",offset); */
    if (offset != (u32) - 1) {
        resfile_seek(hdl->file, offset, SEEK_SET); //seek 到需要读数据的位置
    }

    int rlen = resfile_read(hdl->file, buf, len);
    /* int rlen = fread(buf, len , 1, hdl->file); */

    return rlen;
}



//编码数据输出接口,可以进行存储，传递或者播放等操作
static int audio_demo_dec_data_output(void *priv, u8 *data, int len)
{
    struct audio_dec_demo_hdl *hdl = (struct audio_dec_demo_hdl *)priv;

    y_printf("dec out len = %d\n", len);


    return len;
}


void *audio_dec_demo_test()
{
    struct audio_dec_demo_hdl *hdl = zalloc(sizeof(struct audio_dec_demo_hdl));
    ASSERT(hdl != NULL);

    //diff_type_notice;
    //这里模拟播放flash的提示音文件;

    //打开要播放的文件,拿到文件句柄
    const char *file_name = get_tone_files()->power_on;
    printf("file name: %s\n", file_name);
    char file_path[64];
    strcpy(file_path, FLASH_RES_PATH);
    strcpy(file_path + strlen(FLASH_RES_PATH), file_name);
    hdl->file = resfile_open(file_path);

    hdl->dec = audio_dec_demo_open(hdl);
    if (!hdl->dec) {
        printf("audio_dec_demo_open failed\n");
        return NULL;
    }
    struct audio_fmt fmt;
    int ret = audio_dec_demo_get_fmt(hdl->dec, &fmt);

    if (ret) { // 获取格式信息失败,
        audio_dec_demo_close(hdl->dec);
        free(hdl);
        return NULL;
    }

    audio_dec_demo_start(hdl->dec);


    hdl->start = 1;
    ret = os_task_create(audio_dec_demo_test_task, hdl, 5, 512, 32, AUDIO_DEC_DEMO_TASK);

    printf("== audio_dec_demo_test_open success\n");
    return hdl;

}


void audio_dec_demo_test_close(void *priv)
{
    struct audio_dec_demo_hdl *hdl = (struct audio_dec_demo_hdl *)priv;
    printf("== audio_enc_demo_test_close \n");
    if (hdl) {
        hdl->start = 0;
        audio_dec_demo_close(hdl->dec);
        /* os_task_del(AUDIO_DEC_DEMO_TASK); */
    }

}

