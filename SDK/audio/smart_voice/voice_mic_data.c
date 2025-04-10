#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".voice_mic_data.data.bss")
#pragma data_seg(".voice_mic_data.data")
#pragma const_seg(".voice_mic_data.text.const")
#pragma code_seg(".voice_mic_data.text")
#endif
/*****************************************************************
>file name : voice_mic_data.c
>author : lichao
>create time : Mon 01 Nov 2021 11:33:32 AM CST
*****************************************************************/
#include "app_config.h"
#include "voice_mic_data.h"
#include "smart_voice.h"
#include "app_main.h"
#include "adc_file.h"
#include "aec_uart_debug.h"
#include "effects/convert_data.h"
#include "circular_buf.h"
#include "media/audio_general.h"
#include "audio_splicing.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif /*TCFG_AUDIO_ANC_ENABLE*/

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
#include "vad_mic.h"
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/

#if TCFG_SMART_VOICE_USE_AEC
#include "audio_cvp.h"
#include "overlay_code.h"
#endif

#if ((defined TCFG_SMART_VOICE_ENABLE) && TCFG_SMART_VOICE_ENABLE)
extern struct audio_adc_hdl adc_hdl;

#define CONFIG_VOICE_MIC_DATA_DUMP          0

#define CONFIG_VOICE_MIC_DATA_EXPORT          0

#define MAIN_ADC_GAIN               8
#define MAIN_ADC_BUF_NUM            3

struct main_adc_context {
    struct audio_adc_output_hdl dma_output;
    struct adc_mic_ch mic_ch;
    s16 *dma_buf;
    s16 *mic_sample_data;;
    u8 adc_ch_num;  //打开的adc ch数量
    u8 adc_seq;     //记录当前用的那个mic
    u8 bit_width;
};
/*
 * Mic 数据接收buffer(循环buffer，动态大小)
 */
struct voice_mic_data {
    u8 open;
    u8 source;
    struct main_adc_context *main_adc;
    void *vad_mic;
    struct list_head head;
    cbuffer_t cbuf;
    u8 buf[0];
};

struct voice_mic_capture_channel {
    void *priv;
    void (*output)(void *priv, s16 *data, int len);
    struct list_head entry;
};

static struct voice_mic_data *voice_handle = NULL;
extern const u8 const_adc_async_en;
#define __this      (voice_handle)

#if CONFIG_VOICE_MIC_DATA_DUMP
static u8 mic_data_dump = 0;
#endif

int audio_smart_voice_aec_run(void *priv, s16 *data, int len);
#if TCFG_SMART_VOICE_USE_AEC
struct voice_aec_hdl_t {
    cbuffer_t aec_cbuf;
    u8 aec_data_buf[640 * 3];
    u8 in_tmpbuf[3][512];
    u8 *ref_tmpbuf;
    int ref_tmpbuf_len;
    int last_data_len;
    u8 bit_width;
    u8 ref_channel;
    u8 multiple_cnt;//mic sr和ref sr的倍数

};
static struct voice_aec_hdl_t *voice_aec_hdl = NULL;

extern int audio_dac_read_reset(void);
extern int audio_dac_read(s16 points_offset, void *data, int len, u8 read_channel);
extern int a2dp_player_runing();

void audio_smart_voice_aec_cbuf_data_clear()
{
    if (voice_aec_hdl) {
        cbuf_clear(&voice_aec_hdl->aec_cbuf);
        audio_dac_read_reset();
    }
}

static int audio_smart_voice_aec_run(void *priv, s16 *data, int len)
{
    if (a2dp_player_runing() && voice_aec_hdl) {
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        /*vad 刚起来时，会把缓存的4包数据一起推出来，需要等数据稳定320byte后再做回音消除*/
        if (len != voice_aec_hdl->last_data_len) {
            audio_dac_read_reset();
            voice_aec_hdl->last_data_len = len;
            return 0;
        }
        voice_aec_hdl->last_data_len = len;
#endif
        int wlen = 0;
        wlen = cbuf_write(&voice_aec_hdl->aec_cbuf, data, len);
        if (wlen != len) {
            putchar('a');
        }
        int dac_read_len;
        if (voice_aec_hdl->bit_width == ADC_BIT_WIDTH_16) {
            dac_read_len = len * 1 * voice_aec_hdl->ref_channel * voice_aec_hdl->multiple_cnt;
            audio_dac_read(0, voice_aec_hdl->ref_tmpbuf, dac_read_len / voice_aec_hdl->ref_channel, voice_aec_hdl->ref_channel);
            if (voice_aec_hdl->ref_channel == 2) {
                pcm_dual_to_single(voice_aec_hdl->ref_tmpbuf, voice_aec_hdl->ref_tmpbuf, dac_read_len);
                dac_read_len >>= 1;
            }
            audio_aec_refbuf((s16 *)voice_aec_hdl->ref_tmpbuf, NULL, dac_read_len);
        } else {
            dac_read_len = len * 2 * voice_aec_hdl->ref_channel * voice_aec_hdl->multiple_cnt;
            audio_dac_read(0, voice_aec_hdl->ref_tmpbuf, dac_read_len / voice_aec_hdl->ref_channel, voice_aec_hdl->ref_channel);
            audio_convert_data_32bit_to_16bit_round((s32 *)voice_aec_hdl->ref_tmpbuf, (s16 *)voice_aec_hdl->ref_tmpbuf, dac_read_len / 4);
            dac_read_len >>= 1;
            if (voice_aec_hdl->ref_channel == 2) {
                pcm_dual_to_single(voice_aec_hdl->ref_tmpbuf, voice_aec_hdl->ref_tmpbuf, dac_read_len);
                dac_read_len >>= 1;
            }
            audio_aec_refbuf((s16 *)voice_aec_hdl->ref_tmpbuf, NULL, dac_read_len);
        }

        static u8 burk = 0;
        if (cbuf_get_data_len(&voice_aec_hdl->aec_cbuf) >= 512) {
            wlen = cbuf_read(&voice_aec_hdl->aec_cbuf, &(voice_aec_hdl->in_tmpbuf[burk][0]), 512);
            audio_aec_inbuf((s16 *) & (voice_aec_hdl->in_tmpbuf[burk][0]), wlen);
            burk++;
            if (burk > 2) {
                burk = 0;
            }
            /* printf("wlen : %d", wlen); */
        } else {
            //printf("data_len : %d", cbuf_get_data_len(&__this->aec_cbuf));
        }
        return len;
    } else {
        return 0;
    }
}

/*清晰语音数据输出*/
static int cvp_output_hdl(s16 *data, u16 len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)__this;
    struct voice_mic_capture_channel *ch;
    list_for_each_entry(ch, &voice->head, entry) {
        if (ch->output) {
            ch->output(ch->priv, data, len);
        }
    }
    int wlen = cbuf_write(&voice->cbuf, data, len);
    if (wlen < len) {
        putchar('D');
    }
    return wlen;
}

int audio_smart_voice_aec_open(void)
{
    if (voice_aec_hdl) {
        printf("voice_aec_hdl is alreadly open !!!");
        return -1;
    }
    struct voice_aec_hdl_t *hdl = zalloc(sizeof(struct voice_aec_hdl_t));
    if (hdl == NULL) {
        printf("hdl malloc fail !!!");
        return -1;
    }

#if TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR
    hdl->ref_channel = 2;
#else
    hdl->ref_channel = 1;
#endif

    hdl->multiple_cnt = TCFG_AUDIO_GLOBAL_SAMPLE_RATE / VOICE_ADC_SAMPLE_RATE;
    hdl->bit_width = audio_general_out_dev_bit_width();
    printf("hdl->bit_width %d", hdl->bit_width);
    if (hdl->bit_width == DATA_BIT_WIDE_16BIT) {
        hdl->ref_tmpbuf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short) * hdl->ref_channel * hdl->multiple_cnt;
    } else {
        hdl->ref_tmpbuf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(int) * hdl->ref_channel * hdl->multiple_cnt;
    }

    hdl->ref_tmpbuf = zalloc(hdl->ref_tmpbuf_len);
    cbuf_init(&hdl->aec_cbuf, hdl->aec_data_buf, sizeof(hdl->aec_data_buf));
    struct audio_aec_init_param_t init_param = {
        .sample_rate = VOICE_ADC_SAMPLE_RATE,
        .ref_sr = TCFG_AUDIO_GLOBAL_SAMPLE_RATE,
    };
    audio_aec_open(&init_param, AEC_EN, cvp_output_hdl);
    audio_dac_read_reset();
    voice_aec_hdl = hdl;
    return 0;
}

void audio_smart_voice_aec_close(void)
{
    if (voice_aec_hdl) {
        audio_aec_close();
        free(voice_aec_hdl->ref_tmpbuf);
        free(voice_aec_hdl);
        voice_aec_hdl = NULL;
    }
}
#endif // TCFG_SMART_VOICE_USE_AEC

static int voice_mic_data_output(void *priv, s16 *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)priv;
    int wlen = 0;
#if TCFG_SMART_VOICE_USE_AEC
    wlen = audio_smart_voice_aec_run(priv, data, len);
    if (wlen) {
        return wlen;
    } else
#endif // TCFG_SMART_VOICE_USE_AEC
    {
        struct voice_mic_capture_channel *ch;
        list_for_each_entry(ch, &voice->head, entry) {
            if (ch->output) {
                ch->output(ch->priv, data, len);
            }
        }
        wlen = cbuf_write(&voice->cbuf, data, len);
        if (wlen < len) {
            putchar('D');
        }
    }
    return wlen;
}

static void audio_main_adc_dma_data_handler(void *priv, s16 *data, int len)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)priv;
    if (!voice || voice->source != VOICE_MCU_MIC) {
        return;
    }

    s16 *pcm_data = NULL;
    if (voice->main_adc->bit_width != ADC_BIT_WIDTH_16) {
        s32 *s32_src = (s32 *)data;
        s32 *s32_dst = (s32 *)voice->main_adc->mic_sample_data;
        if (voice->main_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 4; i++) {
                s32_dst[i] = s32_src[i * voice->main_adc->adc_ch_num + voice->main_adc->adc_seq];
            }
            audio_convert_data_32bit_to_16bit_round(s32_dst, (s16 *)s32_dst, len / 4);
        } else {
            audio_convert_data_32bit_to_16bit_round(s32_src, (s16 *)s32_dst, len / 4);
        }
        pcm_data = (s16 *)s32_dst;
        len >>= 1;
    } else {
        s16 *s16_src = (s16 *)data;
        s16 *s16_dst = (s16 *)voice->main_adc->mic_sample_data;
        if (voice->main_adc->adc_ch_num > 1) {
            for (int i = 0; i < len / 2; i++) {
                s16_dst[i] = s16_src[i * voice->main_adc->adc_ch_num + voice->main_adc->adc_seq];
            }
            pcm_data = (s16 *)s16_dst;

        } else {
            pcm_data = (s16 *)data;
        }
    }
    /* putchar('M'); */
    voice_mic_data_output(voice, pcm_data, len);
    smart_voice_core_post_msg(1, SMART_VOICE_MSG_DMA);
}

#if TCFG_CALL_KWS_SWITCH_ENABLE
static void audio_main_adc_mic_close(struct voice_mic_data *voice, u8 all_channel);
static void audio_main_adc_suspend_handler(int all_channel, int arg)
{
    OS_SEM *sem = (OS_SEM *)arg;
    if (__this) {
        audio_main_adc_mic_close(__this, all_channel);
    }
    os_sem_post(sem);
}

void audio_main_adc_suspend_in_core_task(u8 all_channel)
{
    if (!__this || !__this->main_adc) {
        return;
    }
    int argv[5];
    OS_SEM *sem = malloc(sizeof(OS_SEM));
    os_sem_create(sem, 0);

    argv[0] = (int)audio_main_adc_suspend_handler;
    argv[1] = 2;
    argv[2] = all_channel;
    argv[3] = (int)sem;

    do {
        int err = os_taskq_post_type(ASR_CORE, Q_CALLBACK, 4, argv);
        if (err == OS_ERR_NONE) {
            break;
        }

        if (err != OS_Q_FULL) {
            audio_main_adc_suspend_handler(all_channel, (int)sem);
            goto exit;
        }
        os_time_dly(2);
    } while (1);

    os_sem_pend(sem, 100);
exit:
    free(sem);
}

void kws_aec_data_output(void *priv, s16 *data, int len)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return;
    }

    audio_main_adc_suspend_in_core_task(0);

    voice_mic_data_output(__this, data, len);
    smart_voice_core_post_msg(1, SMART_VOICE_MSG_DMA);
}

u8 kws_get_state(void)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return 0;
    }
    return 1;
}

void smart_voice_mcu_mic_suspend(void)
{
    if (!__this || __this->source != VOICE_MCU_MIC) {
        return;
    }

    //打开adc前, 如果kws本身打开了mic，需要close
    audio_main_adc_suspend_in_core_task(1);
}
#endif


#define audio_main_adc_mic_ch_setup(ch, mic_ch, ch_map, adc_handle) \
    if (ch_map & BIT(ch)) { \
        audio_adc_mic##ch##_open(mic_ch, ch_map, adc_handle); \
    }

static int audio_main_adc_mic_open(struct voice_mic_data *voice)
{
    int ret = 0;
    if (voice->main_adc) {
        printf("audio main adc mic already  open !!!");
        return 0;
    }
    voice->main_adc = zalloc(sizeof(struct main_adc_context));

    if (!voice->main_adc) {
        return -ENOMEM;
    }

#if TCFG_AUDIO_ANC_ENABLE && (!TCFG_AUDIO_DYNAMIC_ADC_GAIN)
    /* MAIN_ADC_GAIN = anc_mic_gain_get(); */
#elif TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN
    anc_dynamic_micgain_start(MAIN_ADC_GAIN);
#endif/*TCFG_AUDIO_ANC_ENABLE && (!TCFG_AUDIO_DYNAMIC_ADC_GAIN)*/

    if (const_adc_async_en) {
        /*是否4个adc通道都打开，音箱使用*/
        voice->main_adc->adc_ch_num = AUDIO_ADC_MAX_NUM;
    } else {
        /*默认打开一个adc通道做语音识别，耳机使用*/
        voice->main_adc->adc_ch_num = 1;
    }

    voice->main_adc->bit_width = adc_hdl.bit_width;
    /*设置adc buf大小*/
    int dma_len = 0;
    if (voice->main_adc->bit_width == ADC_BIT_WIDTH_16) {
        dma_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short) * MAIN_ADC_BUF_NUM * voice->main_adc->adc_ch_num;
    } else {
        dma_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(int) * MAIN_ADC_BUF_NUM * voice->main_adc->adc_ch_num;
    }
    voice->main_adc->dma_buf = zalloc(dma_len);
    printf("adc dma_len %d ", dma_len);

    /*设置缓存buf大小*/
    int buf_len = 0;
    if (voice->main_adc->bit_width == ADC_BIT_WIDTH_16) {
        if (voice->main_adc->adc_ch_num > 1) {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short);
        }
    } else {
        if (voice->main_adc->adc_ch_num > 1) {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(int);
        } else {
            buf_len = VOICE_MIC_DATA_PERIOD_FRAMES * sizeof(short);
        }
    }
    printf("mic sample data len %d", buf_len);
    if (buf_len) {
        voice->main_adc->mic_sample_data = zalloc(buf_len);
    }

    /*配置使用哪个mic做语音识别*/
#ifdef TCFG_SMART_VOICE_MIC_CH_SEL
    u8 ch = TCFG_SMART_VOICE_MIC_CH_SEL;
#else
    u8 ch = AUDIO_ADC_MIC_0;
#endif
    printf("%s:%d", __func__, ch);
    audio_adc_file_init();

    for (int i = 0; i < AUDIO_ADC_MAX_NUM; i++) {
        if (ch & AUDIO_ADC_MIC(i)) {
            adc_file_mic_open(&voice->main_adc->mic_ch, AUDIO_ADC_MIC(i));
            audio_adc_mic_set_gain(&voice->main_adc->mic_ch, AUDIO_ADC_MIC(i), MAIN_ADC_GAIN);
        }
    }

    audio_adc_mic_set_sample_rate(&voice->main_adc->mic_ch, VOICE_ADC_SAMPLE_RATE);
    ret =  audio_adc_mic_set_buffs(&voice->main_adc->mic_ch, voice->main_adc->dma_buf,
                                   VOICE_MIC_DATA_PERIOD_FRAMES * 2, MAIN_ADC_BUF_NUM);
    if (ret && voice->main_adc->dma_buf) {
        /*已经设置过buf了，并不需要在set buf*/
        free(voice->main_adc->dma_buf);
        voice->main_adc->dma_buf = NULL;
    }
    voice->main_adc->dma_output.priv = voice;
    voice->main_adc->dma_output.handler = audio_main_adc_dma_data_handler;
    audio_adc_add_output_handler(&adc_hdl, &voice->main_adc->dma_output);
    audio_adc_mic_start(&voice->main_adc->mic_ch);
    voice->main_adc->adc_seq = get_adc_seq(&adc_hdl, ch); //查询模拟mic对应的ADC通道,打开4个adc通道时使用
    printf("adc_seq %d", voice->main_adc->adc_seq);

    return 0;
}

static void audio_main_adc_mic_close(struct voice_mic_data *voice, u8 all_channel)
{
    if (voice->main_adc) {
#if TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN
        anc_dynamic_micgain_stop();
#endif/*TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_DYNAMIC_ADC_GAIN*/
        if (all_channel) {
            audio_adc_mic_close(&voice->main_adc->mic_ch);
        }
        audio_adc_del_output_handler(&adc_hdl, &voice->main_adc->dma_output);
        if (voice->main_adc->dma_buf) {
            /*mic close时会自动释放内存*/
            /* free(voice->main_adc->dma_buf); */
            voice->main_adc->dma_buf = NULL;
        }
        if (voice->main_adc->mic_sample_data) {
            free(voice->main_adc->mic_sample_data);
            voice->main_adc->mic_sample_data = NULL;
        }
        free(voice->main_adc);
        voice->main_adc = NULL;
    }
}


void *voice_mic_data_open(u8 source, int buffer_size, int sample_rate)
{
    if (!__this) {
        __this = zalloc(sizeof(struct voice_mic_data) + buffer_size);
    }

    if (!__this) {
        return NULL;
    }

    if (__this->open) {
        return __this;
    }
    cbuf_init(&__this->cbuf, __this->buf, buffer_size);
    __this->source = source;
    INIT_LIST_HEAD(&__this->head);

#if CONFIG_VOICE_MIC_DATA_EXPORT
    aec_uart_open(1, VOICE_MIC_DATA_PERIOD_FRAMES * 2);
#endif

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    if (source == VOICE_VAD_MIC) {
        __this->vad_mic = lp_vad_mic_open((void *)__this, voice_mic_data_output);
    } else if (source == VOICE_MCU_MIC)
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    {
        audio_main_adc_mic_open(__this);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_WAKE);
    }
    __this->open = 1;
    return __this;
}

void voice_mic_data_close(void *mic)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    if (voice->source == VOICE_VAD_MIC) {
        lp_vad_mic_close(voice->vad_mic);
        voice->vad_mic = NULL;
    } else if (voice->source == VOICE_MCU_MIC)
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    {
        audio_main_adc_mic_close(voice, 1);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_STANDBY);
    }

#if CONFIG_VOICE_MIC_DATA_EXPORT
    aec_uart_close();
#endif
    if (voice) {
        free(voice);
    }
    __this = NULL;
}

void voice_mic_data_switch_source(void *mic, u8 source, int buffer_size, int sample_rate)
{
    struct voice_mic_data *voice = (struct voice_mic_data *)mic;

    voice->source = source;

    if (voice->source == VOICE_VAD_MIC) {
        audio_main_adc_mic_close(voice, 1);
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        if (!voice->vad_mic) {
            voice->vad_mic = lp_vad_mic_open(voice, voice_mic_data_output);
        }
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    } else if (voice->source == VOICE_MCU_MIC) {
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        if (voice->vad_mic) {
            lp_vad_mic_close(voice->vad_mic);
            voice->vad_mic = NULL;
        }
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
        audio_main_adc_mic_open(voice);
        smart_voice_core_post_msg(1, SMART_VOICE_MSG_WAKE);
    }
}

void *voice_mic_data_capture(int sample_rate, void *priv, void (*output)(void *priv, s16 *data, int len))
{
    struct voice_mic_capture_channel *ch = (struct voice_mic_capture_channel *)zalloc(sizeof(struct voice_mic_capture_channel));

    if (!ch) {
        return NULL;
    }
    voice_mic_data_open(VOICE_VAD_MIC, 2048, sample_rate);
    if (!__this) {
        free(ch);
        return NULL;
    }
    ch->priv = priv;
    ch->output = output;
    list_add(&ch->entry, &__this->head);

#if CONFIG_VAD_PLATFORM_SUPPORT_EN
    lp_vad_mic_test();
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    return ch;
}

void voice_mic_data_stop_capture(void *mic)
{
    struct voice_mic_capture_channel *ch = (struct voice_mic_capture_channel *)mic;

    if (ch) {
        list_del(&ch->entry);
        free(ch);
    }

    if (list_empty(&__this->head)) {
#if CONFIG_VAD_PLATFORM_SUPPORT_EN
        lp_vad_mic_test();
#endif /*CONFIG_VAD_PLATFORM_SUPPORT_EN*/
    }
}

int voice_mic_data_read(void *mic, void *data, int len)
{
    struct voice_mic_data *fb = (struct voice_mic_data *)mic;
    /* printf("%s. %d, %d", __func__, len, cbuf_get_data_len(&fb->cbuf)); */
    if (cbuf_get_data_len(&fb->cbuf) < len) {
        return 0;
    } else {
        int wlen = cbuf_read(&fb->cbuf, data, len);
#if CONFIG_VOICE_MIC_DATA_EXPORT
        aec_uart_fill(0, data, len);
        aec_uart_write();
#endif
        return wlen;
    }
}

int voice_mic_data_buffered_samples(void *mic)
{
    struct voice_mic_data *fb = (struct voice_mic_data *)mic;

    return cbuf_get_data_len(&fb->cbuf) >> 1;
}

void voice_mic_data_clear(void *mic)
{
    struct voice_mic_data *fb = (struct voice_mic_data *)mic;

    cbuf_clear(&fb->cbuf);
}

void voice_mic_data_dump(void *mic)
{
    struct voice_mic_data *fb = (struct voice_mic_data *)mic;

    if (fb == NULL) {
        return;
    }
#if CONFIG_VOICE_MIC_DATA_DUMP
    mic_data_dump = 1;
    int len = 0;
    int i = 0;
    s16 *data = (s16 *)cbuf_read_alloc(&fb->cbuf, &len);

    len >>= 1;
    if (data) {
#if 0
        for (i = 0; i < len; i++) {
            if ((i % 3000) == 0) {
                wdt_clear();
            }
            printf("%d\n", data[i]);
        }
#else
        put_buf(data, len << 1);
#endif
    }
    cbuf_read_updata(&fb->cbuf, len << 1);
    mic_data_dump = 0;
#endif
}

#endif
