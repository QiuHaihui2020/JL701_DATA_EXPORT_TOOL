#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_anc_user.data.bss")
#pragma data_seg(".icsd_anc_user.data")
#pragma const_seg(".icsd_anc_user.text.const")
#pragma code_seg(".icsd_anc_user.text")
#endif
#include "app_config.h"
#include "icsd_anc_user.h"
#include "adc_file.h"
#include "app_tone.h"
#include "clock_manager/clock_manager.h"
#include "asm/audio_src.h"
#include "audio_config.h"
#include "asm/audio_adc.h"
#include "cvp_node.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if (TCFG_AUDIO_ANC_ENABLE && \
	(((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) || \
	((defined TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN) && TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN)))

struct audio_mic_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 *adc_buf;    //align 2Bytes
    u16 dump_packet;
};
static struct audio_mic_hdl *audio_mic = NULL;
extern struct audio_dac_hdl dac_hdl;
extern struct audio_adc_hdl adc_hdl;

int audio_mic_en(u8 en, audio_mic_param_t *mic_param,
                 void (*data_handler)(void *priv, s16 *data, int len))
{
    printf("audio_mic_en : %d", en);
    if (en) {
        if (audio_mic) {
            printf("audio_mic re-malloc error\n");
            return -1;
        }
        audio_mic = zalloc(sizeof(struct audio_mic_hdl));
        if (audio_mic == NULL) {
            printf("audio mic zalloc failed\n");
            return -1;
        }

        u16 mic_ch = mic_param->mic_ch_sel;
        u16 sr = mic_param->sample_rate;
        u8 gain0 = mic_param->mic0_gain;
        u8 gain1 = mic_param->mic1_gain;
        u8 gain2 = mic_param->mic2_gain;
        u8 gain3 = mic_param->mic3_gain;

        u8 mic_num = 0;
        /*打开mic电压*/
        audio_mic_pwr_ctl(MIC_PWR_ON);
        audio_adc_file_init();
        /*打开mic0*/
        if (mic_ch & AUDIO_ADC_MIC_0) {
            printf("adc_mic0 open, sr:%d, gain:%d\n", sr, gain0);
            adc_file_mic_open(&audio_mic->mic_ch, AUDIO_ADC_MIC_0);
            audio_adc_mic_set_gain(&audio_mic->mic_ch, AUDIO_ADC_MIC_0, gain0);
            mic_num ++;
        }
        /*打开mic1*/
        if (mic_ch & AUDIO_ADC_MIC_1) {
            printf("adc_mic1 open, sr:%d, gain:%d\n", sr, gain1);
            adc_file_mic_open(&audio_mic->mic_ch, AUDIO_ADC_MIC_1);
            audio_adc_mic_set_gain(&audio_mic->mic_ch, AUDIO_ADC_MIC_1, gain1);
            mic_num ++;
        }
        /*打开mic2*/
        if (mic_ch & AUDIO_ADC_MIC_2) {
            printf("adc_mic2 open, sr:%d, gain:%d\n", sr, gain2);
            adc_file_mic_open(&audio_mic->mic_ch, AUDIO_ADC_MIC_2);
            audio_adc_mic_set_gain(&audio_mic->mic_ch, AUDIO_ADC_MIC_2, gain2);
            mic_num ++;
        }
        /*打开mic3*/
        if (mic_ch & AUDIO_ADC_MIC_3) {
            printf("adc_mic3 open, sr:%d, gain:%d\n", sr, gain3);
            adc_file_mic_open(&audio_mic->mic_ch, AUDIO_ADC_MIC_3);
            audio_adc_mic_set_gain(&audio_mic->mic_ch, AUDIO_ADC_MIC_3, gain3);
            mic_num ++;
        }

        int adc_buf_size = mic_param->adc_irq_points * 2 * mic_param->adc_buf_num * mic_num;
        printf("adc irq points %d, adc_buf_size : %d", mic_param->adc_irq_points, adc_buf_size);
        /* audio_mic->adc_buf = esco_adc_buf; */
        /* audio_mic->adc_buf = zalloc(adc_buf_size); */
        /* if (audio_mic->adc_buf == NULL) { */
        /*     printf("audio->adc_buf mic zalloc failed\n"); */
        /*     audio_mic_pwr_ctl(MIC_PWR_OFF); */
        /*     free(audio_mic); */
        /*     audio_mic = NULL; */
        /*     return -1; */
        /* } */

        audio_adc_mic_set_sample_rate(&audio_mic->mic_ch, sr);
        audio_adc_fixed_digital_set_buffs();
        /* audio_adc_mic_set_buffs(&audio_mic->mic_ch, audio_mic->adc_buf, */
        /*                         mic_param->adc_irq_points * 2, mic_param->adc_buf_num); */
        audio_mic->adc_output.handler = data_handler;
        audio_adc_add_output_handler(&adc_hdl, &audio_mic->adc_output);
        audio_adc_mic_start(&audio_mic->mic_ch);
    } else {
        if (audio_mic) {
            /*设置fb mic为复用mic*/
#if TCFG_AUDIO_ANC_ENABLE
            audio_anc_mic_mana_fb_mult_set(1);
#endif
            audio_adc_mic_close(&audio_mic->mic_ch);
            audio_adc_del_output_handler(&adc_hdl, &audio_mic->adc_output);
            if (audio_mic->adc_buf) {
                /* free(audio_mic->adc_buf);  */
            }
#if TCFG_AUDIO_ANC_ENABLE
            /*清除fb mic为复用mic的标志*/
            audio_anc_mic_mana_fb_mult_set(0);
            if (anc_status_get() == 0)
#endif
            {
                audio_mic_pwr_ctl(MIC_PWR_OFF);
            }
            free(audio_mic);
            audio_mic = NULL;
        }
    }
    return 0;
}

static char *tone_index_to_name(u8 index)
{
    char *file_name = NULL;
    switch (index) {
    case ICSD_ADT_TONE_NUM0 :
    case ICSD_ADT_TONE_NUM1 :
    case ICSD_ADT_TONE_NUM2 :
    case ICSD_ADT_TONE_NUM3 :
    case ICSD_ADT_TONE_NUM4 :
    case ICSD_ADT_TONE_NUM5 :
    case ICSD_ADT_TONE_NUM6 :
    case ICSD_ADT_TONE_NUM7 :
    case ICSD_ADT_TONE_NUM8 :
    case ICSD_ADT_TONE_NUM9 :
        file_name = (char *)get_tone_files()->num[index];
        break;
    case ICSD_ADT_TONE_NORMAL :
        file_name = (char *)get_tone_files()->normal;
        break;
    case ICSD_ADT_TONE_SPKCHAT_ON :
        file_name = (char *)get_tone_files()->spkchat_on;
        break;
    case ICSD_ADT_TONE_SPKCHAT_OFF :
        file_name = (char *)get_tone_files()->spkchat_off;
        break;
    case ICSD_ADT_TONE_WCLICK_ON :
        file_name = (char *)get_tone_files()->wclick_on;
        break;
    case ICSD_ADT_TONE_WCLICK_OFF :
        file_name = (char *)get_tone_files()->wclick_off;
        break;
    case ICSD_ADT_TONE_WINDDET_ON :
        file_name = (char *)get_tone_files()->winddet_on;
        break;
    case ICSD_ADT_TONE_WINDDET_OFF :
        file_name = (char *)get_tone_files()->winddet_off;
        break;
    }
    return file_name;
}

__attribute__((weak))
void icsd_adt_task_play_tone_cb(void *priv)
{

}

static int icsd_adt_tone_play_cb(void *priv, enum stream_event event)
{
    printf("%s : %d", __func__, (int)event);
    if (event != STREAM_EVENT_STOP) {
        return 0;
    }
    icsd_adt_task_play_tone_cb(priv);
    return 0;
}
static void tws_icsd_adt_tone_callback(int priv, enum stream_event event)
{
    r_printf("tws_icsd_adt_tone_callback: %d\n",  event);
    icsd_adt_tone_play_cb(NULL, event);
}

#define TWS_ICSD_ADT_SYNC_TIMEOUT 400
#define TWS_ICSD_ADT_TONE_STUB_FUNC_UUID   0x3101333A
REGISTER_TWS_TONE_CALLBACK(tws_icsd_adt_tone_stub) = {
    .func_uuid  = TWS_ICSD_ADT_TONE_STUB_FUNC_UUID,
    .callback   = tws_icsd_adt_tone_callback,
};

int icsd_adt_tone_play_callback(u8 index, void (*evt_handler)(void *priv), void *priv)
{
    int ret = 0;
    char *file_name = tone_index_to_name(index);
#if TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state()) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            ret = tws_play_tone_file_callback(file_name, TWS_ICSD_ADT_SYNC_TIMEOUT, TWS_ICSD_ADT_TONE_STUB_FUNC_UUID);
        }
    } else {
        ret = tws_play_tone_file_callback(file_name, TWS_ICSD_ADT_SYNC_TIMEOUT, TWS_ICSD_ADT_TONE_STUB_FUNC_UUID);
    }
#else
    ret = play_tone_file_callback(file_name, priv, icsd_adt_tone_play_cb);
#endif
    return ret;
}

int icsd_adt_tone_play(u8 index)
{
    int ret = 0;
    char *file_name = tone_index_to_name(index);
#if TCFG_USER_TWS_ENABLE
    if (get_tws_sibling_connect_state()) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            ret = tws_play_tone_file(file_name, TWS_ICSD_ADT_SYNC_TIMEOUT);
        }
    } else {
        ret = tws_play_tone_file(file_name, TWS_ICSD_ADT_SYNC_TIMEOUT);
    }
#else
    ret = play_tone_file(file_name);
#endif
    return ret;
}

void icsd_adt_clock_add()
{
    /* clock_refurbish(); */
    clock_alloc("icsd_adt", 16 * 1000000L);
}

void icsd_adt_clock_del()
{
    clock_free("icsd_adt");
}

void icsd_set_clk()
{
    clock_alloc("icsd_adt", 128 * 1000000L);
}

void icsd_anc_fade_set(int gain)
{
#if TCFG_AUDIO_ANC_ENABLE
    audio_anc_fade_ctr_set(ANC_FADE_MODE_WIND_NOISE, AUDIO_ANC_FDAE_CH_ALL, gain);
#endif
}

/* static u8 esco_player_runing(){} */
/* static u8 bt_get_call_status(){} */
/* static u8 bt_a2dp_get_status(){} */
/* static void bt_cmd_prepare(){} */

void *icsd_adt_src_init(int in_rate, int out_rate, int (*handler)(void *, void *, int))
{
    //void *src_hdl = audio_resample_hw_open(1, in_rate, out_rate, 1);
    //audio_resample_hw_set_output_handler(src_hdl, 0, handler);
    //return src_hdl;

    void *src_hdl = zalloc(sizeof(struct audio_src_handle));
    audio_hw_src_open(src_hdl, 1, 1);
    audio_hw_src_set_rate(src_hdl, in_rate, out_rate);
    audio_src_set_output_handler(src_hdl, 0, handler);
    return src_hdl;
}

void icsd_adt_src_write(void *data, int len, void *resample)
{
    //audio_resample_hw_write(resample, data, len);
    audio_src_resample_write(resample, data, len);
}

void icsd_adt_src_push(void *resample)
{
    //audio_resample_hw_push_data_out(resample);
    audio_src_push_data_out((struct audio_src_handle *)resample);
}

void icsd_adt_src_close(void *resample)
{
    //audio_resample_hw_close(resample);
    audio_hw_src_close(resample);
    free(resample);
}

void tws_tx_unsniff_req()
{

}

void icsd_adt_tx_unsniff_req()
{
#if TCFG_USER_TWS_ENABLE
    tws_api_tx_unsniff_req();
#endif /*TCFG_USER_TWS_ENABLE*/
}

void icsd_bt_sniff_set_enable(u8 en)
{
    /* bt_sniff_set_enable(en); */
}

void icsd_set_tws_t_sniff(u16 slot)
{
    /* set_tws_t_sniff(slot); */
}

u8 icsd_get_talk_mic_ch(void)
{
    cvp_param_cfg_read();
    return cvp_get_talk_mic_ch();
}

u8 icsd_get_ref_mic_ch(void)
{
    return BIT(TCFG_AUDIO_ANCL_FF_MIC);
}

u8 icsd_get_fb_mic_ch(void)
{
    return BIT(TCFG_AUDIO_ANCL_FB_MIC);
}

u8 icsd_get_esco_mic_en_map(void)
{
    return audio_adc_file_get_mic_en_map();
}


#endif /*TCFG_AUDIO_ANC_ENABLE*/

