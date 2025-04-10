//////////////////////////////////////////////////////////////
//
// demo流程： adc->pcm_data->enc->enc_data->dec->pcm_data->dac
//
// 示例格式及参数： LC3  :  sample_rate = 48000;
//							frame_len = 100;
//							channel = 1;
//							bit_rate = 96000;
//////////////////////////////////////////////////////////////
#include "audio_splicing.h"
#include "system/includes.h"
#include "cpu/includes.h"
#include "app_main.h"
#include "audio_config.h"
#include "audio_base.h"
#include "asm/audio_adc.h"
#include "asm/dac.h"
#include "codec/lc3_codec_api.h"
#include "codec/audio_decode_common_api.h"


/*编码参数，不同的类型，使用的参数也不同，有些参数是一些类型特有的*/
struct audio_demo_codec_params {
    u8  ch_num;        // 声道数
    u16 frame_len;      //帧长时间，dci-ms
    u32 coding_type;
    u32 sample_rate;
    u32 bit_rate;
};

static int audio_demo_enc_resume(void *priv);
static int audio_demo_dec_resume(void *priv);
static int audio_demo_pcm_data_write(void *priv, void *data, u16 len);
static int audio_demo_pcm_data_read(void *priv, void *buf, u16 len);
static int audio_demo_enc_data_write(void *priv, void *data, u16 len);
static int audio_demo_enc_data_read(void *priv, void *buf, u16 len);


//-----------------------------------------------------------------------------
//   dac demo
//-----------------------------------------------------------------------------
extern struct audio_dac_hdl dac_hdl;
static int audio_demo_dac_write(void *priv, u8 *data, u16 len)
{
    int wlen = audio_dac_write(&dac_hdl, data, len);
    if (wlen != len) {
        printf("DAC_W : %d,%d\n", len, wlen);
    }

    return wlen;
}

static int audio_demo_dac_open(u32 sample_rate)
{
    app_audio_state_switch(APP_AUDIO_STATE_MUSIC, app_audio_volume_max_query(AppVol_BT_MUSIC), NULL);
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
    audio_dac_set_sample_rate(&dac_hdl, sample_rate);
    audio_dac_start(&dac_hdl);
    audio_dac_channel_start(NULL);

    printf("===  audio_demo_dac_open\n");

    return 0;
}


void audio_demo_dac_close(void)
{
    audio_dac_stop(&dac_hdl);
    audio_dac_close(&dac_hdl);
}


//-----------------------------------------------------------------------------
//   enccoder demo
//-----------------------------------------------------------------------------

struct audio_demo_encoder {
    void *priv;
    u8 *enc_buf;//编码用的work_buf
    u8 *stk_buf;//编码需要的堆栈scratch
    u8 start;

    //编码器的结构 ,不同的编码器不一样;
    LC3_CODEC_INFO info;
    LC3_CODEC_IO enc_io;
    LC3_CODEC_OPS *ops;

    struct audio_demo_codec_params enc_param; //编码参数
};


//读入待编码的pcm数据
static u16 audio_demo_enc_input_data(void *priv, u8 *buf, u16 len)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;

    int rlen =  audio_demo_pcm_data_read(enc_demo->priv, buf, len);

    /* printf("enc input : %d---%d",len,rlen); */

    return rlen;
}
//编码数据输出
static u16 audio_demo_enc_output_data(void *priv, u8 *buf, u16 len)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;

    int wlen = audio_demo_enc_data_write(enc_demo->priv, buf, len);
    /* printf("enc out len: %d-%d",len,wlen); */

    if (enc_demo->priv) {
        audio_demo_dec_resume(enc_demo->priv);
    }

    return wlen;
}




/*打开编码器，申请资源*/
void *audio_demo_encoder_open(void *priv)
{
    struct audio_demo_encoder *enc_demo = NULL;
    enc_demo = zalloc(sizeof(struct audio_demo_encoder));
    if (enc_demo) {
        //获取对应编码器句柄
        enc_demo->ops = get_lc3_enc_ops();
        ASSERT(enc_demo->ops != NULL);
        enc_demo->priv = priv;
        return enc_demo;
    }
    printf("get enc ops error !!\n");
    return NULL;

}

/*设置编码参数*/
int audio_demo_encoder_set_fmt(void *priv, struct audio_fmt *fmt)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;
    enc_demo->enc_param.coding_type = fmt-> coding_type;
    enc_demo->enc_param.sample_rate = fmt-> sample_rate;
    enc_demo->enc_param.ch_num = fmt->channel;
    enc_demo->enc_param.frame_len = fmt->frame_len;
    enc_demo->enc_param.bit_rate = fmt->bit_rate;

    printf(" enc fmt:%d, %d, %d,%d\n", enc_demo->enc_param.sample_rate, enc_demo->enc_param.ch_num, enc_demo->enc_param.frame_len, enc_demo->enc_param.bit_rate);

    return 0;
}

/*初始化一些编码器配置，准备开始编码*/
int audio_demo_encoder_start(void *priv)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;

    enc_demo->info.sr = enc_demo->enc_param.sample_rate;
    enc_demo->info.br = enc_demo->enc_param.bit_rate;//设置成与编码相同的比特率
    enc_demo->info.fr_dms = enc_demo->enc_param.frame_len;//设置成与编码相同
    enc_demo->info.ep_mode = 0;
    enc_demo->info.nch = enc_demo->enc_param.ch_num;//输入通道数
    enc_demo->info.if_s24 = 0;//是否输出24bit

    int enc_buf_size = enc_demo->ops->need_dcbuf_size(&enc_demo->info);
    enc_demo->enc_buf = zalloc(enc_buf_size);
    ASSERT(enc_demo->enc_buf != NULL);
    int stk_buf_size = enc_demo->ops->need_stbuf_size(&enc_demo->info);
    enc_demo->stk_buf = zalloc(stk_buf_size);
    ASSERT(enc_demo->stk_buf != NULL);
    printf("dec_buf_size = %d,stk_buf_size = %d\n", enc_buf_size, stk_buf_size);

    enc_demo->enc_io.priv = enc_demo;
    enc_demo->enc_io.input_data = audio_demo_enc_input_data;
    enc_demo->enc_io.output_data = audio_demo_enc_output_data;
    enc_demo->ops->open(enc_demo->enc_buf, (const LC3_CODEC_IO *)&enc_demo->enc_io, &enc_demo->info);//打开编码器

    enc_demo->start = 1;
    return 0;
}


/*编码运行函数，包括读pcm 数据，进行编码，编码输出,错误处理等*/
/*有些拜编码器需要外部读数据，有些编码器自己有接口读数据，需要不同处理*/
int audio_demo_encoder_run(void *priv)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;

    if (enc_demo->start == 0) {
        audio_demo_encoder_start(enc_demo);
    }

    int ret = enc_demo->ops->run(enc_demo->enc_buf, enc_demo->stk_buf);
    if (ret) {
        /* printf("der err %d\n",ret); */
        //错误处理
    }
    return ret;

}

int audio_demo_encoder_close(void *priv)
{
    struct audio_demo_encoder *enc_demo = (struct audio_demo_encoder *)priv;
    if (enc_demo) {
        enc_demo->start = 0;
        if (enc_demo->enc_buf) {
            free(enc_demo->enc_buf);
            enc_demo->enc_buf = NULL;
        }
        if (enc_demo->stk_buf) {
            free(enc_demo->stk_buf);
            enc_demo->stk_buf = NULL;
        }
        free(enc_demo);
        enc_demo = NULL;
    }
    return 0;

}


//-----------------------------------------------------------------------------
//   decoder demo
//-----------------------------------------------------------------------------
struct audio_demo_decoder {
    void *priv;
    u8 *dec_buf;//解码用的work_buf
    u8 *stk_buf;//解码需要的堆栈scratch
    u8 start;
    u8  output_mode;

    //解码器的结构 ,不同的解码器不一样;
    LC3_CODEC_INFO info;
    LC3_CODEC_IO dec_io;
    LC3_CODEC_OPS *ops;

    struct audio_demo_codec_params dec_param; //编码参数
};

//读数据到解码器，数据写入buf, 读取长度是len
static u16 audio_demo_dec_input_data(void *priv, u8 *buf, u16 len)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;

    int rlen = audio_demo_enc_data_read(dec_demo->priv, buf, len);

    /* printf("dec input :%d--%d\n",len,rlen); */

    return rlen ;
}




//解码数据输出，数据输出不完，下次run的时候会先输出剩余数据，
static u16 audio_demo_dec_output_data(void *priv, u8 *data, u16 len)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;
    int wlen = 0;

    wlen = audio_demo_dac_write(NULL, data, len);
    /* printf("dec out len : %d=%d\n",len, wlen); */

    return len;
}


/*打开解码器，申请资源*/
void *audio_demo_decoder_open(void *priv)
{
    struct audio_demo_decoder *dec_demo = NULL;
    dec_demo = zalloc(sizeof(struct audio_demo_decoder));
    if (dec_demo) {
        dec_demo->ops = get_lc3_dec_ops();
        ASSERT(dec_demo->ops != NULL);
        dec_demo->priv = priv;

        return dec_demo;
    }
    printf("get dec ops error !!\n");
    return NULL;

}



/*设置需要的解码参数*/
int audio_demo_decoder_set_fmt(void *priv, struct audio_fmt *fmt)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;
    dec_demo->dec_param.sample_rate = fmt-> sample_rate;
    dec_demo->dec_param.ch_num = fmt->channel;
    dec_demo->dec_param.frame_len = fmt->frame_len;
    dec_demo->dec_param.bit_rate = fmt->bit_rate;

    printf(" dec fmt:%d, %d, %d,%d\n", dec_demo->dec_param.sample_rate, dec_demo->dec_param.ch_num, dec_demo->dec_param.frame_len, dec_demo->dec_param.bit_rate);

    return 0;
}

/*初始化一些解码器配置，准备开始编码*/
int audio_demo_decoder_start(void *priv)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;

    if (dec_demo->start) {
        return 0;
    }
    dec_demo->info.sr = dec_demo->dec_param.sample_rate;
    dec_demo->info.br = dec_demo->dec_param.bit_rate;//设置成与编码相同的比特率
    dec_demo->info.fr_dms = dec_demo->dec_param.frame_len;//设置成与编码相同
    dec_demo->info.ep_mode = 0;
    dec_demo->info.nch = dec_demo->dec_param.ch_num;//输入通道数
    dec_demo->info.if_s24 = 0;//是否输出24bit

    printf(" dec fmt:%d, %d, %d,%d\n", dec_demo->info.sr, dec_demo->info.br, dec_demo->info.fr_dms, dec_demo->info.nch);
    dec_demo->dec_buf = zalloc(dec_demo->ops->need_dcbuf_size(&dec_demo->info));
    ASSERT(dec_demo->dec_buf != NULL);
    dec_demo->stk_buf = zalloc(dec_demo->ops->need_stbuf_size(&dec_demo->info));
    ASSERT(dec_demo->stk_buf != NULL);

    dec_demo->dec_io.priv = dec_demo;
    dec_demo->dec_io.input_data  = audio_demo_dec_input_data;
    dec_demo->dec_io.output_data = audio_demo_dec_output_data;
    dec_demo->ops->open(dec_demo->dec_buf, (const LC3_CODEC_IO *)&dec_demo->dec_io, &dec_demo->info);

    u32 frame_size = dec_demo->ops->codec_config(dec_demo->dec_buf, GET_EDFRAME_LEN, NULL); //获取到的是编码数据的帧长
    printf("dec: frame_size = %d\n", frame_size);

    dec_demo->ops->codec_config(dec_demo->dec_buf, SET_EDFRAME_LEN, &frame_size); //设置帧长，


    AUDIO_DECODE_PARA par = {0};
    par.mode = 1;//使能输出不完时，内部管理输出buf,即输出不完下次继续输出
    dec_demo->ops->codec_config(dec_demo->dec_buf, SET_DECODE_MODE, (void *)&par);

    dec_demo->start = 1;
    return 0;
}

/*设置解码输出声道类型*/
/*根据dac 数据声道类型，或是后级需要的声道类型配置*/
int audio_demo_decoder_set_output_channel(void *priv, enum audio_channel output_channel)
{

    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;
    LC3_DEC_CH_OPUT_PARA lc3_nch_config_pram;
    u8 dec_out_ch_mode = 0;

    if (!dec_demo->start) {
        audio_demo_decoder_start(dec_demo);
    }
    switch (output_channel) {
    case AUDIO_CH_LR:
    case AUDIO_CH_DUAL_L:
    case AUDIO_CH_DUAL_R:
    case AUDIO_CH_DUAL_LR:
        lc3_nch_config_pram.mode = 0;
        dec_out_ch_mode = LC3_NORMAL_CH;
        break;
    case AUDIO_CH_L:
        lc3_nch_config_pram.mode = 1;
        lc3_nch_config_pram.pL = 8192;//8192是输出原始数据的大小
        dec_out_ch_mode = LC3_L_OUT;
        break;
    case AUDIO_CH_R:
        lc3_nch_config_pram.mode = 2;
        lc3_nch_config_pram.pR = 8192;
        dec_out_ch_mode = LC3_R_OUT;
        break;
    case AUDIO_CH_DIFF:
    case AUDIO_CH_MIX:
        lc3_nch_config_pram.mode = 3;
        lc3_nch_config_pram.pL = 8192;
        lc3_nch_config_pram.pR = 8192;
        dec_out_ch_mode = LC3_LR_MEAN;
        break;
    default:
        printf("dec_out_ch_mode err !!\n");
        break;
    }
    printf(" dec_out_ch_mode %d\n", dec_out_ch_mode);
    dec_demo->ops->codec_config(dec_demo->dec_buf, SET_DEC_CH_MODE, &lc3_nch_config_pram);
    dec_demo->output_mode = output_channel; //记录输出的声道类型，

    return 0;
}

static int audio_demo_decoder_run(void *priv)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;

    int ret = dec_demo->ops->run(dec_demo->dec_buf, dec_demo->stk_buf); //run 的过程中会调用input 读取数据，并且会通过output解口输出数据;
    if (ret) { //错误处理
        /* if(ret != LC3_ERROR_NO_DATA){ */
        printf("lc3_decoder err !!, %d", ret);
        /* } */
    }

    return ret;
}

static int audio_demo_decoder_close(void *priv)
{
    struct audio_demo_decoder *dec_demo = (struct audio_demo_decoder *)priv;
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


//-----------------------------------------------------------------------------
// mic
//-----------------------------------------------------------------------------
#define ADC_DEMO_CH_NUM        	2	/*支持的最大采样通道(max = 2)*/
#define ADC_DEMO_BUF_NUM        2	/*采样buf数*/
#define ADC_DEMO_IRQ_POINTS     480	/*采样中断点数*/
#define ADC_DEMO_BUFS_SIZE      (ADC_DEMO_CH_NUM * ADC_DEMO_BUF_NUM * ADC_DEMO_IRQ_POINTS)

struct audio_demo_mic_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch    mic_ch;
    s16 adc_buf[ADC_DEMO_BUFS_SIZE];
    s16 *sample_buffer;                     //单双声道转换缓存
    volatile u8 start;
    u8 out_ch_num;
    u8 mic_nch;
    void *priv;
};

/* #define TCFG_SOUND_PCM_DELAY_TEST   1 */
extern struct audio_adc_hdl adc_hdl;


#if (defined TCFG_SOUND_PCM_DELAY_TEST && TCFG_SOUND_PCM_DELAY_TEST)
static u16 sin_data_offset = 0;
const s16 sin_48k[48] = {
    0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
    14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
    14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
    0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
    -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
    -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
};
static int get_sine_data(s16 *data, u16 points, u8 ch)
{
    while (points--) {
        if (sin_data_offset >= ARRAY_SIZE(sin_48k)) {
            sin_data_offset = 0;
        }
        *data++ = sin_48k[sin_data_offset];
        if (ch == 2) {
            *data++ = sin_48k[sin_data_offset];
        }
        sin_data_offset++;
    }
    return 0;
}
#endif


static void mic_demo_output_data(void *priv, s16 *data, int len)
{
    struct audio_demo_mic_hdl *mic_hdl = (struct audio_demo_mic_hdl *)priv;
    int wlen = 0;
    /* y_printf("mic:0x%x, len=%d\n",data,len); */
    if (!mic_hdl || !mic_hdl->start) {
        return;
    }

#if (defined TCFG_SOUND_PCM_DELAY_TEST && TCFG_SOUND_PCM_DELAY_TEST)
    get_sine_data(data, len / 2, 1);
#endif

    if (mic_hdl->mic_nch == 1 && mic_hdl->out_ch_num >= 2) { //硬件配置单声道，需要输出双声道
        if (mic_hdl->sample_buffer) {
            mic_hdl->sample_buffer = zalloc(len * 2);
        }
        pcm_single_to_dual(mic_hdl->sample_buffer, data, len);
        wlen = audio_demo_pcm_data_write(mic_hdl->priv, mic_hdl->sample_buffer, len * 2);
        if (wlen < len * 2) {
            putchar('L');
        }
    } else {
        wlen = audio_demo_pcm_data_write(mic_hdl->priv, data, len);
        if (wlen < len) {
            putchar('L');
        }
    }

    if (mic_hdl->priv) {
        audio_demo_enc_resume(mic_hdl->priv);
    }
}


static void audio_demo_mic_close(void *priv)
{
    struct audio_demo_mic_hdl *mic_hdl = (struct audio_demo_mic_hdl *)priv;
    if (!mic_hdl) {
        return ;
    }
    mic_hdl->start = 0;
    if (mic_hdl->sample_buffer) {
        free(mic_hdl->sample_buffer);
        mic_hdl->sample_buffer = NULL;
    }
    audio_adc_del_output_handler(&adc_hdl, &mic_hdl->adc_output);
    audio_adc_mic_close(&mic_hdl->mic_ch);
    local_irq_disable();
    free(mic_hdl);
    mic_hdl = NULL;
    local_irq_enable();
}

static void *audio_demo_mic_open(void *priv, int sample_rate, u8 gain, u8 out_ch_num)
{

    struct audio_demo_mic_hdl *mic_demo = zalloc(sizeof(struct audio_demo_mic_hdl));
    printf("mic_demo = 0x%x\n", (int)mic_demo);
    ASSERT(mic_demo);
    mic_demo->out_ch_num = out_ch_num;
    mic_demo->priv = priv;
    //printf("JL_AUDIO->ADC_CON = 0x%x\n", JL_AUDIO->ADC_CON);

    struct mic_open_param mic_param = {0};
    mic_param.mic_mode      = AUDIO_MIC_CAP_MODE;
    mic_param.mic_ain_sel   = AUDIO_MIC0_CH0;
    mic_param.mic_bias_sel  = AUDIO_MIC_BIAS_CH0;
    mic_param.mic_bias_rsel = 4;
    mic_param.mic_dcc       = 8;
    audio_adc_mic_open(&mic_demo->mic_ch, AUDIO_ADC_MIC_0, &adc_hdl, &mic_param);
    audio_adc_mic_set_gain(&mic_demo->mic_ch, AUDIO_ADC_MIC_0, gain);
    audio_adc_mic_gain_boost(AUDIO_ADC_MIC_0, 1);
    mic_demo->mic_nch = 1;


    //step1:设置mic通道采样率
    audio_adc_mic_set_sample_rate(&mic_demo->mic_ch, sample_rate);
    //step2:设置mic采样buf
    audio_adc_mic_set_buffs(&mic_demo->mic_ch, mic_demo->adc_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
    printf("mic_demo->adc_buf = 0x%x\n", (int)mic_demo->adc_buf);
    //step3:设置mic采样输出回调函数
    mic_demo->adc_output.priv    = mic_demo;
    mic_demo->adc_output.handler = mic_demo_output_data;
    audio_adc_add_output_handler(&adc_hdl, &mic_demo->adc_output);
    //step4:启动mic通道采样
    audio_adc_mic_start(&mic_demo->mic_ch);

    mic_demo->start = 1;

    return mic_demo;
}


//////////////////////////////////////////////////////////////
//
// 测试流程： adc->pcm_data->enc->enc_data->dec->pcm_data->dac
//
//////////////////////////////////////////////////////////////


#define AUDIO_DEMO_PCM_BUF_LEN		(4*1024)
#define AUDIO_DEMO_ENC_BUF_LEN		(1024)

#define AUDIO_DEMO_ENCODER_RUN             0
#define AUDIO_DEMO_DECODER_RUN             1
#define AUDIO_DEMO_CODEC_TASK              "audio_codec_task"

struct audio_demo_enc_dec_hdl {
    struct audio_demo_encoder *enc;
    struct audio_demo_decoder *dec;
    struct audio_demo_mic_hdl *mic;

    cbuffer_t pcm_cbuf; //数据交互buf;
    int pcm_buf[AUDIO_DEMO_PCM_BUF_LEN / 4];
    cbuffer_t enc_cbuf;
    int enc_buf[AUDIO_DEMO_ENC_BUF_LEN / 4];
    u8 start;
};

//驱动编码
static int audio_demo_enc_resume(void *priv)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;

    return  os_taskq_post_msg(AUDIO_DEMO_CODEC_TASK, 2, AUDIO_DEMO_ENCODER_RUN, (int)hdl);

}

//驱动解码
static int audio_demo_dec_resume(void *priv)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;
    return  os_taskq_post_msg(AUDIO_DEMO_CODEC_TASK, 2, AUDIO_DEMO_DECODER_RUN, (int)hdl);

}

//保存pcm 数据
static int audio_demo_pcm_data_write(void *priv, void *data, u16 len)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;

    /* y_printf("adc pcm write: len=%d\n",len); */
    if (!hdl || (!hdl->start)) {
        return 0;
    }
    int wlen = cbuf_write(&hdl->pcm_cbuf, data, len);
    if (wlen != len) {
        putchar('W');
    }

    return wlen;
}

//读取pcm 数据
static int audio_demo_pcm_data_read(void *priv, void *buf, u16 len)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;
    if (!hdl || (!hdl->start)) {
        return 0;
    }

    int rlen = cbuf_read(&hdl->pcm_cbuf, buf, len);

    return rlen;
}

//保存编码数据
static int audio_demo_enc_data_write(void *priv, void *data, u16 len)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;
    if (!hdl || (!hdl->start)) {
        return 0;
    }
    int wlen = cbuf_write(&hdl->enc_cbuf, data, len);
    if (wlen != len) {
        printf("enc_w : %d--%d\n", len, wlen);

    }

    return wlen;

}


//读取编码数据
static int audio_demo_enc_data_read(void *priv, void *buf, u16 len)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;
    if (!hdl || (!hdl->start)) {
        return 0;
    }
    int rlen = cbuf_read(&hdl->enc_cbuf, buf, len);


    return rlen;
}

static void audio_codec_test_task(void *priv)
{
    struct audio_demo_enc_dec_hdl *hdl;
    int res = 0;
    int msg[16];
    u8 pend_taskq = 1;
    while (1) {
        res = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case AUDIO_DEMO_ENCODER_RUN:
                hdl = (struct audio_demo_enc_dec_hdl *)msg[2];
                audio_demo_encoder_run(hdl->enc);  //编码主函数
                break;
            case AUDIO_DEMO_DECODER_RUN:
                hdl = (struct audio_demo_enc_dec_hdl *)msg[2];
                audio_demo_decoder_run(hdl->dec);  //解码主函数
                break;
            }

        }
    }
}

////////////*  test   *//////////////
int audio_demo_enc_dec_test_open(void)
{
    struct audio_demo_enc_dec_hdl *hdl = zalloc(sizeof(struct audio_demo_enc_dec_hdl));
    ASSERT(hdl != NULL);

    cbuf_init(&hdl->pcm_cbuf, hdl->pcm_buf, AUDIO_DEMO_PCM_BUF_LEN);
    cbuf_init(&hdl->enc_cbuf, hdl->enc_buf, AUDIO_DEMO_ENC_BUF_LEN);

    /*配置编解码格式信息: 所需格式参数 不同格式需要的参数可能不一致，不限于下面几种*/
    struct audio_fmt fmt = {0};
    fmt.coding_type =  AUDIO_CODING_LC3;
    fmt.sample_rate = 48000;
    fmt.frame_len = 100;
    fmt.channel = 1;
    fmt.bit_rate = 96000;

    printf("fmt: 0x%x,%d,%d,%d\n", fmt.coding_type, fmt.sample_rate, fmt.frame_len, fmt.channel);


    // 开编码流程，配置相关参数
    hdl->enc =  audio_demo_encoder_open(hdl);
    if (!hdl->enc) {
        printf("audio_demo_enc_open failed\n");
    }
    audio_demo_encoder_set_fmt(hdl->enc, &fmt);
    audio_demo_encoder_start(hdl->enc);


    // 开解码流程，配置相关参数
    hdl->dec =  audio_demo_decoder_open(hdl);
    if (!hdl->enc) {
        printf("audio_demo_dec_open failed\n");

    }
    audio_demo_decoder_set_fmt(hdl->dec, &fmt);
    audio_demo_decoder_start(hdl->dec);
    audio_demo_decoder_set_output_channel(hdl->dec, AUDIO_CH_LR);

    //开mic,产生pcm 数据
    hdl->mic = audio_demo_mic_open(hdl, fmt.sample_rate, 10, fmt.channel);
    if (!hdl->mic) {
        printf("audio_demo_mic_open failed\n");
    }
    //开dac, 播放数据
    audio_demo_dac_open(fmt.sample_rate);

    extern int clk_set_api(const char *name, u32 frequency);
    clk_set_api("sys", 128 * 1000000L); //设置时钟。

    hdl->start = 1;
    int ret = os_task_create(audio_codec_test_task, hdl, 5, 512, 32, AUDIO_DEMO_CODEC_TASK);

    printf("== audio_demo_enc_dec_test_open\n");

    return 0;

}

int audio_demo_enc_dec_test_close(void *priv)
{
    struct audio_demo_enc_dec_hdl *hdl = (struct audio_demo_enc_dec_hdl *)priv;

    hdl->start = 0;
    audio_demo_mic_close(hdl->mic);
    audio_demo_encoder_close(hdl->enc);
    audio_demo_decoder_close(hdl->dec);
    os_task_del(AUDIO_DEMO_CODEC_TASK);
    audio_demo_dac_close();
    free(hdl);
    return 0;
}


