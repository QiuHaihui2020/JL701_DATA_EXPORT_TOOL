#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_general.data.bss")
#pragma data_seg(".audio_general.data")
#pragma const_seg(".audio_general.text.const")
#pragma code_seg(".audio_general.text")
#endif

#include "system/init.h"
#include "media/audio_general.h"
#include "app_config.h"
#include "media/audio_def.h"
#include "jlstream.h"
#include "uartPcmSender.h"
#include "debug/audio_debug.h"
#include "audio_config_def.h"
#include "scene_update.h"
#include "effects/voiceChanger_api.h"

#if TCFG_USER_TWS_ENABLE
const int config_media_tws_en = 1;
#else
const int config_media_tws_en = 0;
#endif

/* 16bit数据流中也存在32bit位宽数据的处理 */
const int config_ch_adapter_32bit_enable = 1;
const int config_mixer_32bit_enable = 1;
const int config_jlstream_fade_32bit_enable = 1;
const int config_audio_eq_xfade_enable = 1;

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_L)
const int config_audio_dac_channel_left_enable = 1;
const int config_audio_dac_channel_right_enable = 0;
#elif (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_MONO_R)
const int config_audio_dac_channel_left_enable = 0;
const int config_audio_dac_channel_right_enable = 1;
#else
const int config_audio_dac_channel_left_enable = 1;
const int config_audio_dac_channel_right_enable = 1;
#endif
#ifdef TCFG_AUDIO_DAC_POWER_ON_MODE
const int config_audio_dac_power_on_mode = TCFG_AUDIO_DAC_POWER_ON_MODE;
#endif
#ifdef TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE
const int config_audio_dac_power_off_lite = TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE;
#endif
#if TCFG_CFG_TOOL_ENABLE
const int config_audio_cfg_online_enable = 1;
#else
const int config_audio_cfg_online_enable = 0;
#endif

const int config_audio_dac_dma_buf_realloc_enable = 1;

const int config_audio_gain_enable = TCFG_GAIN_NODE_ENABLE;
const int config_audio_split_gain_enable = TCFG_SPLIT_GAIN_NODE_ENABLE;
const int config_audio_stereomix_enable = TCFG_STEROMIX_NODE_ENABLE;
/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
////////////////////ID3信息使能/////////////////
const u8 config_flac_id3_enable = 0;
const u8 config_ape_id3_enable  = 0;
const u8 config_m4a_id3_enable  = 0;
const u8 config_wav_id3_enable = 0;
const u8 config_wma_id3_enable = 0;

/////////////////////wma codec//////////////////
const int const_audio_codec_wma_dec_supoort_POS_play = 1; //是否支持指定位置播放

/////////////////////wav codec/////////////////
const int const_audio_codec_wav_dec_bitDepth_set_en = 0;

/*
 *******************************************************************
 *						Audio SYNCTS Config
 *******************************************************************
 */

const float FRAME_DURATION_THREAD = 1.5f;	//范围1.5f~2,采样率和时间戳抖动阈值倍数(丢帧检测阈值,时间戳间隔超过1.5帧，判定丢帧)

/*
 *******************************************************************
 *						Audio Effects Config
 *******************************************************************
 */
//输出级限幅使能
const int config_out_dev_limiter_enable = 0;

const float config_bandmerge_node_fade_step = 0.0f;//淡入步进 0:默认不淡入 非0：淡入步进，范围：0.01f~10.0f，建议值0.1f,步进越大，更新越快
const int config_bandmerge_node_processing_method = 0;//0：bandmerge 拿到所有iport的数据后，一次性叠加完成。 1：逐个叠加到目标地址，不做等待


/*控制 eq_design.c中的butterworth 函数 设计的系数是定点还是浮点 */
#if defined(EQ_CORE_V1)
const int butterworth_iir_filter_coeff_type_select = 0;//虚拟低音根据此变量使用相应的滤波器设计函数 0:float  1:int
#else
const int butterworth_iir_filter_coeff_type_select = 1;//虚拟低音根据此变量使用相应的滤波器设计函数 0:float  1:int
#endif

const int limiter_run_mode = EFx_PRECISION_PRO
#if defined(TCFG_AUDIO_EFX_4E5B_RUN_MODE)
                             | TCFG_AUDIO_EFX_4E5B_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_F58A_RUN_MODE)
                             | TCFG_AUDIO_EFX_F58A_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_4E5B_RUN_MODE) && !defined(TCFG_AUDIO_EFX_F58A_RUN_MODE)
                             | 0xFFFF
#endif
                             ;

#ifdef TCFG_AUDIO_EFX_6195_RUN_MODE
const  int frequency_shift_run_mode      = TCFG_AUDIO_EFX_6195_RUN_MODE;
#else
const  int frequency_shift_run_mode      = EFx_BW_16t16 | EFx_BW_32t32;//只有 16进16出， 或者 32进32出
#endif

#ifdef TCFG_AUDIO_EFX_A48F_RUN_MODE
const  int plate_reverb_lite_run_mode    = TCFG_AUDIO_EFX_A48F_RUN_MODE;
#else
const  int plate_reverb_lite_run_mode    = EFx_BW_16t16 | EFx_BW_32t32;//只有 16进16出， 或者 32进32出
#endif

#ifdef TCFG_AUDIO_EFX_98A4_RUN_MODE
const  int echo_run_mode                 = TCFG_AUDIO_EFX_98A4_RUN_MODE;
#else
const  int echo_run_mode                 = EFx_BW_16t16 | EFx_BW_32t32;//只有 16进16出， 或者 32进32出
#endif

#ifdef TCFG_AUDIO_EFX_7293_RUN_MODE
const  int voicechanger_run_mode         = TCFG_AUDIO_EFX_7293_RUN_MODE;
#else
const  int voicechanger_run_mode         = EFx_BW_16t16 | EFx_BW_32t32;//变声位宽控制
#endif

#ifdef TCFG_AUDIO_EFX_C07A_RUN_MODE
const  int autotune_run_mode             = TCFG_AUDIO_EFX_C07A_RUN_MODE;
#else
const  int autotune_run_mode             = EFx_BW_16t16 | EFx_BW_32t32;//autoTune位宽控制
#endif

#ifdef TCFG_AUDIO_EFX_24AB_RUN_MODE
const  int reverb_run_mode               = TCFG_AUDIO_EFX_24AB_RUN_MODE;
#else
const  int reverb_run_mode               = EFx_BW_16t16 | EFx_BW_32t32;//lib_Reverb.a
#endif

#ifdef TCFG_AUDIO_EFX_5101_RUN_MODE
const  int plate_reverb_run_mode         = TCFG_AUDIO_EFX_5101_RUN_MODE;
#else
const  int plate_reverb_run_mode         = EFx_BW_16t16 | EFx_BW_32t32;//lib_reverb_cal.a
#endif
#ifdef TCFG_AUDIO_EFX_0753_RUN_MODE
const  int plate_reverb_adv_run_mode     = TCFG_AUDIO_EFX_0753_RUN_MODE;
#else
const  int plate_reverb_adv_run_mode     = EFx_BW_16t16 | EFx_BW_32t32;//lib_plateReverb_adv.a
#endif

#ifdef TCFG_AUDIO_EFX_E955_RUN_MODE
const  int noisegate_pro_run_mode        = TCFG_AUDIO_EFX_E955_RUN_MODE;
#else
const  int noisegate_pro_run_mode        = EFx_BW_16t16 | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_B7C4_RUN_MODE
const  int noisegate_run_mode            = TCFG_AUDIO_EFX_B7C4_RUN_MODE;
#else
const  int noisegate_run_mode            = EFx_BW_16t16 | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_B0D5_RUN_MODE
const  int virtual_bass_run_mode         = TCFG_AUDIO_EFX_B0D5_RUN_MODE;
#else
const  int virtual_bass_run_mode         = EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_55C9_RUN_MODE
const  int virtual_bass_classic_run_mode = TCFG_AUDIO_EFX_55C9_RUN_MODE;
#else
const  int virtual_bass_classic_run_mode = EFx_BW_16t16 | EFx_BW_32t32;
#endif

const  int drc_advance_run_mode          = EFx_PRECISION_NOR
#if defined(TCFG_AUDIO_EFX_4250_RUN_MODE)
        | TCFG_AUDIO_EFX_4250_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_74CB_RUN_MODE)
        | TCFG_AUDIO_EFX_74CB_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_4250_RUN_MODE) && !defined(TCFG_AUDIO_EFX_74CB_RUN_MODE)
        | EFx_BW_16t16 | EFx_BW_32t16 | EFx_BW_32t32
#endif
        ;


#ifdef TCFG_AUDIO_EFX_9A58_RUN_MODE
const  int drc_detect_run_mode           = TCFG_AUDIO_EFX_9A58_RUN_MODE | EFx_PRECISION_NOR;
#else
const  int drc_detect_run_mode           = EFx_BW_16t16 | EFx_BW_32t16 | EFx_PRECISION_NOR | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_DEFE_RUN_MODE
const  int drc_run_mode                  = TCFG_AUDIO_EFX_DEFE_RUN_MODE | EFx_PRECISION_NOR;
#else
const  int drc_run_mode                  = EFx_BW_16t16 | EFx_BW_32t16 | EFx_PRECISION_NOR | EFx_BW_32t32;
#endif


#ifdef TCFG_AUDIO_EFX_540E_RUN_MODE
const int pitch_speed_run_mode       = TCFG_AUDIO_EFX_540E_RUN_MODE;
#else
const int pitch_speed_run_mode       = EFx_BW_32t32 | EFx_BW_16t16;
#endif
const int resample_fast_cal_run_mode = EFx_BW_16t16 | EFx_BW_32t32;

#ifdef TCFG_AUDIO_EFX_A8F4_RUN_MODE
const int pcm_delay_run_mode         = TCFG_AUDIO_EFX_A8F4_RUN_MODE;
#else
const int pcm_delay_run_mode         = EFx_BW_16t16 | EFx_BW_32t32;
#endif
#ifdef TCFG_AUDIO_EFX_1B2A_RUN_MODE
const int harmonic_exciter_run_mode  = TCFG_AUDIO_EFX_1B2A_RUN_MODE;
#else
const int harmonic_exciter_run_mode  = EFx_BW_16t16 | EFx_BW_32t32;
#endif
#ifdef TCFG_AUDIO_EFX_ED7F_RUN_MODE
const int lfaudio_plc_run_mode       = TCFG_AUDIO_EFX_ED7F_RUN_MODE;
#else
const int lfaudio_plc_run_mode       = EFx_BW_16t16 | EFx_BW_32t32;
#endif

#ifdef TCFG_AUDIO_EFX_BC44_RUN_MODE
const int  stereo_flanger_run_mode = TCFG_AUDIO_EFX_BC44_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_flanger_run_mode = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_7C2B_RUN_MODE
const int  stereo_chorus_run_mode  = TCFG_AUDIO_EFX_7C2B_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_chorus_run_mode  = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_D2E8_RUN_MODE
const int  stereo_phaser_run_mode  = TCFG_AUDIO_EFX_D2E8_RUN_MODE | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#else
const int  stereo_phaser_run_mode  = EFx_BW_32t32 | EFx_BW_16t16 | EFx_MODULE_MONO_EN | EFx_MODULE_STEREO_EN;
#endif

#ifdef TCFG_AUDIO_EFX_AB66_RUN_MODE
const int  pingpong_echo_run_mode  = TCFG_AUDIO_EFX_AB66_RUN_MODE;
#else
const int  pingpong_echo_run_mode  = EFx_BW_32t32 | EFx_BW_16t16;
#endif

#ifdef TCFG_AUDIO_EFX_97AA_RUN_MODE
const int distortion_run_mode = TCFG_AUDIO_EFX_97AA_RUN_MODE;
#else
const int distortion_run_mode = EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t16 | EFx_BW_32t32;
#endif

const int dynamic_eq_run_mode = EFx_BW_32t32 | EFx_PRECISION_NOR; //只支持32进32出 不会优化代码预留

const int dynamic_eq_pro_run_mode = EFx_BW_32t32;//只支持32进32出 不会优化代码预留

const int iir_filter_run_mode = 0  //不支持32进16出
#if defined(TCFG_AUDIO_EFX_3845_RUN_MODE)
                                | TCFG_AUDIO_EFX_3845_RUN_MODE
#endif
#if defined(TCFG_AUDIO_EFX_6700_RUN_MODE)
                                | TCFG_AUDIO_EFX_6700_RUN_MODE
#endif
#if !defined(TCFG_AUDIO_EFX_3845_RUN_MODE) && !defined(TCFG_AUDIO_EFX_6700_RUN_MODE)
                                | EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t32  //不支持32进16出
#endif
                                ;

/*变声模式使能*/
const int voicechanger_effect_v_config = (0
        | BIT(EFFECT_VOICECHANGE_PITCHSHIFT)
        /* | BIT(EFFECT_VOICECHANGE_CARTOON) */
        /* | BIT(EFFECT_VOICECHANGE_SPECTRUM) */
        /* | BIT(EFFECT_VOICECHANGE_ROBORT) */
        /* | BIT(EFFECT_VOICECHANGE_MELODY) */
        /* | BIT(EFFECT_VOICECHANGE_WHISPER) */
        /* | BIT(EFFECT_VOICECHANGE_F0_DOMAIN) */
        /* | BIT(EFFECT_VOICECHANGE_F0_TD) */
        /* | BIT(EFFECT_VOICECHANGE_FEEDBACK) */
                                         );

/*mb limiter 3带使能(1.2k) */
const int mb_limiter_3band_run_en       = 1;
/*
 *******************************************************************
 *						Audio Mic Capless Config
 *******************************************************************
 */
/*
 *          用于debug需要配置多长的mic_capless_delay
 * 部分mic 需要更多延时才能达到稳定的电压值
 * debug步骤：
 * 1、打开 const_mic_capless_open_delay_debug，可打印打开adc后，mic_bias需要多长时间才能达到/接近稳定的电压值
 * 2、将该延时配置到 open_delay_ms 中
 * 3、关闭 const_mic_capless_open_delay_debug

 * 4、打开 const_mic_capless_trim_delay_debug，可打印第一次trim后，mic_bias需要多长时间才能达到/接近稳定的电压值
 * 5、将该延时配置到 trim_delay_ms 中
 * 6、关闭 const_mic_capless_trim_delay_debug
 */
const u8 const_mic_capless_open_delay_debug = 0;
const u8 const_mic_capless_trim_delay_debug = 0;



__attribute__((weak))
int get_system_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_media_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_esco_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_mic_eff_stream_bit_width(void *par)
{
    return 0;
}
__attribute__((weak))
int get_usb_audio_stream_bit_width(void *par)
{
    return 0;
}


static struct audio_general_params audio_general_param = {0};

struct audio_general_params *audio_general_get_param(void)
{
    return &audio_general_param;
}
/*
 *输出设备硬件位宽
 * */
int audio_general_out_dev_bit_width()
{
    if (audio_general_param.system_bit_width || audio_general_param.media_bit_width ||
        audio_general_param.esco_bit_width || audio_general_param.mic_bit_width
        || audio_general_param.usb_audio_bit_width) {
        return DATA_BIT_WIDE_24BIT;
    }
    return DATA_BIT_WIDE_16BIT;
}
/*
 *输入设备硬件位宽
 * */
int audio_general_in_dev_bit_width()
{
    if (audio_general_param.media_bit_width || audio_general_param.mic_bit_width) {
        return DATA_BIT_WIDE_24BIT;
    }
    return DATA_BIT_WIDE_16BIT;
}

int audio_general_init()
{
#if defined(TCFG_SCENE_UPDATE_ENABLE) && TCFG_SCENE_UPDATE_ENABLE
    //若流程中有较多音效模块（或渲染封装节点），会导致此处遍历模块耗时较长
    get_music_pipeline_node_uuid();
#if TCFG_MIC_EFFECT_ENABLE
    get_mic_pipeline_node_uuid();
#endif
#endif

#if ((defined TCFG_AUDIO_DATA_EXPORT_DEFINE) && (TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_UART))
    uartSendInit();
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

#if TCFG_AUDIO_CONFIG_TRACE
    audio_config_trace_setup(TCFG_AUDIO_CONFIG_TRACE_INTERVAL);
#endif/*TCFG_AUDIO_CONFIG_TRACE*/

#if MEDIA_24BIT_ENABLE
    struct stream_bit_width stream_par = {0};
    if (get_system_stream_bit_width(&stream_par)) {
        audio_general_param.system_bit_width = stream_par.bit_width;
    }

    if (get_media_stream_bit_width(&stream_par)) {
        audio_general_param.media_bit_width = stream_par.bit_width;
    }

    if (get_esco_stream_bit_width(&stream_par)) {
        audio_general_param.esco_bit_width = stream_par.bit_width;
    }

    if (get_mic_eff_stream_bit_width(&stream_par)) {
        audio_general_param.mic_bit_width = stream_par.bit_width;
    }

    if (get_usb_audio_stream_bit_width(&stream_par)) {
        audio_general_param.usb_audio_bit_width = stream_par.bit_width;
    }
#endif

#if defined(TCFG_AUDIO_GLOBAL_SAMPLE_RATE) &&TCFG_AUDIO_GLOBAL_SAMPLE_RATE
    audio_general_param.sample_rate = TCFG_AUDIO_GLOBAL_SAMPLE_RATE;
#endif
    return 0;
}

#if (CONFIG_CPU_BR27 || CONFIG_CPU_BR28 || CONFIG_CPU_BR29 || CONFIG_CPU_BR36)
static const int general_sample_rate_table[] = {8000, 16000, 32000, 44100, 48000, 96000};
#else
static const int general_sample_rate_table[] = {8000, 16000, 32000, 44100, 48000, 96000, 192000};
#endif

int audio_general_set_global_sample_rate(int sample_rate)
{
    local_irq_disable();
    for (u8 i = 0; i < ARRAY_SIZE(general_sample_rate_table); i++) {
        if (general_sample_rate_table[i] == sample_rate) {
            audio_general_param.sample_rate = sample_rate;
            local_irq_enable();
            printf("cur_global_samplerate %d", sample_rate);
            return 0;
        }
    }
    local_irq_enable();
    return -1;
}

