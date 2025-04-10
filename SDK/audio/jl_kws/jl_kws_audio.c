#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".jl_kws_audio.data.bss")
#pragma data_seg(".jl_kws_audio.data")
#pragma const_seg(".jl_kws_audio.text.const")
#pragma code_seg(".jl_kws_audio.text")
#endif
#include "jl_kws_common.h"
#include "jl_kws_api.h"
#include "adc_file.h"
#include "asm/audio_adc.h"
#include "circular_buf.h"
#include "effects/convert_data.h"

#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
//===================================================//
//                    ADC buf配置                    //
//===================================================//
#define ADC_DEMO_BUF_NUM        2
#define ADC_DEMO_IRQ_POINTS     256
#define ADC_DEMO_BUFS_SIZE      (ADC_DEMO_BUF_NUM * ADC_DEMO_IRQ_POINTS)
#define KWS_CBUF_SIZE 	(ADC_DEMO_BUFS_SIZE * sizeof(s16))


//==============================================//
//       KWS 数据来自外部输入(AEC)配置          //
//==============================================//
#define TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN 			1 //配置为1, 由AEC外部输入数据


//==============================================//
//          KWS RAM使用OVERLAY配置              //
//==============================================//
#define KWS_USE_OVERLAY_RAM_ENABLE 			0
#define KWS_USE_OVERLAY_RAM_SIZE 			(35 * 1024)
#if KWS_USE_OVERLAY_RAM_ENABLE
static int kws_ram[KWS_USE_OVERLAY_RAM_SIZE / sizeof(int)] sec(.jl_kws_ram);
#endif /* #if KWS_USE_OVERLAY_RAM_ENABLE */

enum KWS_AUDIO_STATE {
    KWS_AUDIO_STATE_IDLE = 0,
    KWS_AUDIO_STATE_RUN,
    KWS_AUDIO_STATE_STOP,
    KWS_AUDIO_STATE_CLOSE,
};

struct kws_adc_mic {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 *dma_buf;
    s16 *mic_sample_data;;
    u8 adc_ch_num;  //打开的adc ch数量
    u8 adc_seq;     //记录当前用的那个mic
    u8 bit_width;
};

struct jl_kws_audio {
    u8 kws_audio_state;
    OS_SEM rx_sem;
    struct kws_adc_mic *kws_adc;
    cbuffer_t kws_cbuf;
    u32 cbuf[KWS_CBUF_SIZE / 4];
};

static struct jl_kws_audio *__kws_audio = NULL;

extern struct audio_adc_hdl adc_hdl;
extern const u8 const_adc_async_en;
static void kws_adc_mic_output(void *priv, s16 *data, int len)
{
    int wlen = 0;

    local_irq_disable();/*防止关闭kws时，kws任务和aec任务互相打断导致释放信号量错误死机的问题*/
    if (__kws_audio == NULL) {
        local_irq_enable();
        return;
    }
    if (__kws_audio->kws_audio_state != KWS_AUDIO_STATE_RUN) {
        local_irq_enable();
        return;
    }

    /* kws_putchar('w'); */
    wlen = cbuf_write(&(__kws_audio->kws_cbuf), data, len);
    if (wlen < len) {
        kws_info("kws cbuf full");
    } else {
        //kws_debug("wlen: %d", wlen);
    }

    os_sem_post(&(__kws_audio->rx_sem));
    local_irq_enable();
}

static void audio_main_adc_dma_data_handler(void *priv, s16 *data, int len)
{
    if ((__kws_audio == NULL) || (__kws_audio->kws_adc == NULL)) {
        return;
    }
    if (__kws_audio->kws_audio_state != KWS_AUDIO_STATE_RUN) {
        return;
    }

    s16 *pcm_data = NULL;
    if (__kws_audio->kws_adc->bit_width != ADC_BIT_WIDTH_16) {
        s32 *s32_src = (s32 *)data;
        s32 *s32_dst = (s32 *)__kws_audio->kws_adc->mic_sample_data;
        if (__kws_audio->kws_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 4; i++) {
                s32_dst[i] = s32_src[i * __kws_audio->kws_adc->adc_ch_num + __kws_audio->kws_adc->adc_seq];
            }
            audio_convert_data_32bit_to_16bit_round(s32_dst, (s16 *)s32_dst, len / 4);
        } else {
            audio_convert_data_32bit_to_16bit_round(s32_src, (s16 *)s32_dst, len / 4);
        }
        pcm_data = (s16 *)s32_dst;
        len >>= 1;
    } else {
        s16 *s16_src = (s16 *)data;
        s16 *s16_dst = (s16 *)__kws_audio->kws_adc->mic_sample_data;
        if (__kws_audio->kws_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 2; i++) {
                s16_dst[i] = s16_src[i * __kws_audio->kws_adc->adc_ch_num + __kws_audio->kws_adc->adc_seq];
            }
            pcm_data = (s16 *)s16_dst;

        } else {
            pcm_data = (s16 *)data;
        }
    }

    kws_adc_mic_output(priv, pcm_data, len);
}


static int jl_kws_adc_mic_open(void)
{
    u16 mic_sr = 16000;
    int ret = 0;

    if (__kws_audio->kws_adc != NULL) {
        return JL_KWS_ERR_AUDIO_MIC_STATE_ERR;
    }

    __kws_audio->kws_adc = zalloc(sizeof(struct kws_adc_mic));
    kws_debug("struct kws_adc_mic = 0x%x", (u32)sizeof(struct kws_adc_mic));
    if (__kws_audio->kws_adc == NULL) {
        return JL_KWS_ERR_AUDIO_MIC_NO_BUF;
    }

    kws_debug("kws local open mic");
    //audio_mic_pwr_ctl(MIC_PWR_ON);
    if (const_adc_async_en) {
        /*是否4个adc通道都打开，音箱使用*/
        __kws_audio->kws_adc->adc_ch_num = AUDIO_ADC_MAX_NUM;
    } else {
        /*默认打开一个adc通道做语音识别，耳机使用*/
        __kws_audio->kws_adc->adc_ch_num = 1;
    }
    __kws_audio->kws_adc->bit_width = adc_hdl.bit_width;
    /*设置adc buf大小*/
    int dma_len = 0;
    if (__kws_audio->kws_adc->bit_width == ADC_BIT_WIDTH_16) {
        dma_len = ADC_DEMO_IRQ_POINTS * sizeof(short) * ADC_DEMO_BUF_NUM * __kws_audio->kws_adc->adc_ch_num;
    } else {
        dma_len = ADC_DEMO_IRQ_POINTS * sizeof(int) * ADC_DEMO_BUF_NUM * __kws_audio->kws_adc->adc_ch_num;
    }
    __kws_audio->kws_adc->dma_buf = zalloc(dma_len);
    printf("adc dma_len %d ", dma_len);

    /*设置缓存buf大小*/
    int buf_len = 0;
    if (__kws_audio->kws_adc->bit_width == ADC_BIT_WIDTH_16) {
        if (__kws_audio->kws_adc->adc_ch_num > 1) {
            buf_len = ADC_DEMO_IRQ_POINTS * sizeof(short);
        }
    } else {
        if (__kws_audio->kws_adc->adc_ch_num > 1) {
            buf_len = ADC_DEMO_IRQ_POINTS * sizeof(int);
        } else {
            buf_len = ADC_DEMO_IRQ_POINTS * sizeof(short);
        }
    }
    printf("mic sample data len %d", buf_len);
    if (buf_len) {
        __kws_audio->kws_adc->mic_sample_data = zalloc(buf_len);
    }

    /*配置使用哪个mic做语音识别*/
#ifdef TCFG_KWS_MIC_CHA_SELECT
    u8 ch = TCFG_KWS_MIC_CHA_SELECT;
#else
    u8 ch = AUDIO_ADC_MIC_0;
#endif
    printf("%s:%d", __func__, ch);
    audio_adc_file_init();

    for (int i = 0; i < AUDIO_ADC_MAX_NUM; i++) {
        if (ch & AUDIO_ADC_MIC(i)) {
            adc_file_mic_open(&__kws_audio->kws_adc->mic_ch, AUDIO_ADC_MIC(i));
            /* audio_adc_mic_set_gain(&__kws_audio->kws_adc->mic_ch, AUDIO_ADC_MIC(i), MAIN_ADC_GAIN); */
        }
    }

    audio_adc_mic_set_sample_rate(&(__kws_audio->kws_adc->mic_ch), mic_sr);
    ret = audio_adc_mic_set_buffs(&(__kws_audio->kws_adc->mic_ch), __kws_audio->kws_adc->dma_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
    if (ret && __kws_audio->kws_adc->dma_buf) {
        /*已经设置过buf了，并不需要在set buf*/
        free(__kws_audio->kws_adc->dma_buf);
        __kws_audio->kws_adc->dma_buf = NULL;
    }
    __kws_audio->kws_adc->adc_output.priv = __kws_audio;
    __kws_audio->kws_adc->adc_output.handler = audio_main_adc_dma_data_handler;
    audio_adc_add_output_handler(&adc_hdl, &(__kws_audio->kws_adc->adc_output));
    __kws_audio->kws_adc->adc_seq = get_adc_seq(&adc_hdl, ch); //查询模拟mic对应的ADC通道,打开4个adc通道时使用
    printf("adc_seq %d", __kws_audio->kws_adc->adc_seq);

    return JL_KWS_ERR_NONE;
}

static int kws_speech_recognition_local_mic_open(void)
{
    jl_kws_adc_mic_open();

    jl_kws_audio_start();

    return 0;
}


//==========================================================//
// 				    JL_KWS AUDIO API                        //
//==========================================================//
int jl_kws_audio_init(void)
{
    int ret = JL_KWS_ERR_NONE;

    if (__kws_audio != NULL) {
        return JL_KWS_ERR_AUDIO_INIT_STATE_ERR;
    }

    __kws_audio = zalloc(sizeof(struct jl_kws_audio));
    kws_debug("struct jl_kws_audio = 0x%x", (u32)sizeof(struct jl_kws_audio));

    if (__kws_audio == NULL) {
        return JL_KWS_ERR_AUDIO_MIC_NO_BUF;
    }

    os_sem_create(&(__kws_audio->rx_sem), 0);

    cbuf_init(&(__kws_audio->kws_cbuf), __kws_audio->cbuf, KWS_CBUF_SIZE);

    /* audio_codec_clock_set(AUDIO_KWS_MODE, AUDIO_CODING_MSBC, 0); */
    return ret;
}

u8 kws_aec_get_state(void)
{
#if TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN
    if (__kws_audio) {
        //aec初始化, 查询是否进入kws模式, 这时有可能需要kws本身打开了mic，需要close
        if (__kws_audio->kws_adc) {
            kws_debug("kws free adc buf");
            audio_adc_mic_close(&__kws_audio->kws_adc->mic_ch);
            audio_adc_del_output_handler(&adc_hdl, &(__kws_audio->kws_adc->adc_output));
            if (__kws_audio->kws_adc->dma_buf) {
                /*mic close时会自动释放内存*/
                /* free(__kws_audio->kws_adc->dma_buf); */
                __kws_audio->kws_adc->dma_buf = NULL;
            }
            if (__kws_audio->kws_adc->mic_sample_data) {
                free(__kws_audio->kws_adc->mic_sample_data);
                __kws_audio->kws_adc->mic_sample_data = NULL;
            }
            free(__kws_audio->kws_adc);
            __kws_audio->kws_adc = NULL;

            cbuf_clear(&(__kws_audio->kws_cbuf));
            os_sem_set(&(__kws_audio->rx_sem), 0);
        }
        return 1;
    } else {
        return 0;
    }

#else

    return 0;

#endif /* #if TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN */
}


u8 kws_get_state(void)
{
    return kws_aec_get_state();
}

u8 kws_is_running(void)
{
    return (__kws_audio != NULL);
}

__attribute__((weak))
int audio_aec_update(u8 EnableBit)
{
    return 0;
}

void kws_aec_data_output(void *priv, s16 *data, int len)
{
    if (__kws_audio == NULL) {
        return ;
    }

    if (__kws_audio->kws_adc) {
        audio_adc_del_output_handler(&adc_hdl, &__kws_audio->kws_adc->adc_output);
        if (__kws_audio->kws_adc->dma_buf) {
            /*mic close时会自动释放内存*/
            /* free(__kws_audio->kws_adc->dma_buf); */
            __kws_audio->kws_adc->dma_buf = NULL;
        }
        if (__kws_audio->kws_adc->mic_sample_data) {
            free(__kws_audio->kws_adc->mic_sample_data);
            __kws_audio->kws_adc->mic_sample_data = NULL;
        }
        kws_debug("kws free adc buf");
        free(__kws_audio->kws_adc);
        __kws_audio->kws_adc = NULL;
    }

    kws_adc_mic_output(priv, data, len);
}


int jl_kws_audio_get_data(void *buf, u32 len)
{
    u32 data_len;
    u32 rlen = 0;

    if (__kws_audio == NULL) {
        return 0;
    }

    data_len = cbuf_get_data_len(&(__kws_audio->kws_cbuf));

    if (data_len >= len) {
        cbuf_read(&(__kws_audio->kws_cbuf), buf, len);
        rlen = len;
    } else {
        os_sem_set(&(__kws_audio->rx_sem), 0);
#if TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN
        int ret = os_sem_pend(&(__kws_audio->rx_sem), 200);
        if (ret == OS_TIMEOUT) {
            kws_speech_recognition_local_mic_open();
        }
#else
        os_sem_pend(&(__kws_audio->rx_sem), 0);
#endif /* #if TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN */
    }

    return rlen;
}


int jl_kws_audio_start(void)
{
    int ret = JL_KWS_ERR_NONE;
    if (__kws_audio == NULL) {
        return JL_KWS_ERR_AUDIO_MIC_STATE_ERR;
    }
    kws_debug("%s", __func__);
    __kws_audio->kws_audio_state = KWS_AUDIO_STATE_RUN;

#if (TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN == 0)
    ret = jl_kws_adc_mic_open();
    if (ret != JL_KWS_ERR_NONE) {
        return ret;
    }
#endif /* #if TCFG_JL_KWS_AUDIO_DATA_FROM_EXTERN */

    if (__kws_audio->kws_adc) {
        audio_adc_mic_start(&(__kws_audio->kws_adc->mic_ch));
    }

    return ret;
}

void jl_kws_audio_stop(void)
{
    if (__kws_audio) {
        __kws_audio->kws_audio_state = KWS_AUDIO_STATE_STOP;
        if (__kws_audio->kws_adc) {
            audio_adc_mic_close(&__kws_audio->kws_adc->mic_ch);
            audio_adc_del_output_handler(&adc_hdl, &(__kws_audio->kws_adc->adc_output));
            if (__kws_audio->kws_adc->dma_buf) {
                /*mic close时会自动释放内存*/
                /* free(__kws_audio->kws_adc->dma_buf); */
                __kws_audio->kws_adc->dma_buf = NULL;
            }
            if (__kws_audio->kws_adc->mic_sample_data) {
                free(__kws_audio->kws_adc->mic_sample_data);
                __kws_audio->kws_adc->mic_sample_data = NULL;
            }
            free(__kws_audio->kws_adc);
            __kws_audio->kws_adc = NULL;
        }

        cbuf_clear(&(__kws_audio->kws_cbuf));
        os_sem_set(&(__kws_audio->rx_sem), 0);
    }
}


void jl_kws_audio_close(void)
{
    kws_debug("%s", __func__);
    if (__kws_audio) {
        __kws_audio->kws_audio_state = KWS_AUDIO_STATE_CLOSE;
        if (__kws_audio->kws_adc) {
            audio_adc_mic_close(&__kws_audio->kws_adc->mic_ch);
            audio_adc_del_output_handler(&adc_hdl, &(__kws_audio->kws_adc->adc_output));
            if (__kws_audio->kws_adc->dma_buf) {
                /*mic close时会自动释放内存*/
                /* free(__kws_audio->kws_adc->dma_buf); */
                __kws_audio->kws_adc->dma_buf = NULL;
            }
            if (__kws_audio->kws_adc->mic_sample_data) {
                free(__kws_audio->kws_adc->mic_sample_data);
                __kws_audio->kws_adc->mic_sample_data = NULL;
            }

            free(__kws_audio->kws_adc);
            __kws_audio->kws_adc = NULL;
        }
        /* audio_codec_clock_del(AUDIO_KWS_MODE); */
        extern u8 audio_aec_status(void);
        if (audio_aec_status()) {
            audio_aec_update(0);
        }
        free(__kws_audio);
        __kws_audio = NULL;
    }
}


#endif /* #if TCFG_KWS_VOICE_RECOGNITION_ENABLE */
