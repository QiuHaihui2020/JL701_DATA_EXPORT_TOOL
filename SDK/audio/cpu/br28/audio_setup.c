#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_setup.data.bss")
#pragma data_seg(".audio_setup.data")
#pragma const_seg(".audio_setup.text.const")
#pragma code_seg(".audio_setup.text")
#endif
/*
 ******************************************************************************************
 *							Audio Setup
 *
 * Discription: 音频模块初始化，配置，调试等
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "sdk_config.h"
#include "asm/audio_adc.h"
#include "media/audio_energy_detect.h"
#include "adc_file.h"
#include "linein_file.h"
#include "update.h"
#include "user_cfg.h"
#include "gpio_config.h"
#include "app_main.h"
#include "audio_demo/audio_demo.h"
#include "media/audio_general.h"
#include "fm_file.h"
#include "asm/dac.h"

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_dvol.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#if defined(TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt_app.h"
#endif

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice.h"
#endif

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

struct audio_dac_hdl dac_hdl;
struct audio_adc_hdl adc_hdl;

typedef struct {
    u8 audio_inited;
    atomic_t ref;
} audio_setup_t;
audio_setup_t audio_setup = {0};
#define __this      (&audio_setup)

#if TCFG_MC_BIAS_AUTO_ADJUST
u8 mic_bias_rsel_use_save[AUDIO_ADC_MIC_MAX_NUM] = {0};
u8 save_mic_bias_rsel[AUDIO_ADC_MIC_MAX_NUM]     = {0};
u8 mic_ldo_vsel_use_save = 0;
u8 save_mic_ldo_vsel     = 0;
#endif // #if TCFG_MC_BIAS_AUTO_ADJUST

___interrupt
static void audio_irq_handler()
{
    if (JL_AUDIO->DAC_CON & BIT(7)) {
        JL_AUDIO->DAC_CON |= BIT(6);
        if (JL_AUDIO->DAC_CON & BIT(5)) {
            audio_dac_irq_handler(&dac_hdl);
        }
    }

    if (JL_AUDIO->ADC_CON & BIT(7)) {
        JL_AUDIO->ADC_CON |= BIT(6);
        if ((JL_AUDIO->ADC_CON & BIT(5)) && (JL_AUDIO->ADC_CON & BIT(4))) {
            audio_adc_irq_handler(&adc_hdl);
        }
    }
}

struct dac_config_param {
    u8 output;
    u8 mode;
    u8 light_close;
    u8 power_on_mode;
    u8 vcm_cap_en;
};

struct dac_platform_data dac_data = {
    .output             = TCFG_AUDIO_DAC_CONNECT_MODE,                //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    .mode               = TCFG_AUDIO_DAC_MODE,
    .light_close        = TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE,
    .power_on_mode      = TCFG_AUDIO_DAC_POWER_ON_MODE,
    .vcm_cap_en         = TCFG_AUDIO_VCM_CAP_EN,
    .dma_buf_time_ms    = TCFG_AUDIO_DAC_BUFFER_TIME_MS,
    .pa_isel            = TCFG_AUDIO_DAC_PA_ISEL, //DAC电流挡位，范围：0~6
    .l_ana_gain         = TCFG_AUDIO_L_CHANNEL_GAIN,
    .r_ana_gain         = TCFG_AUDIO_R_CHANNEL_GAIN,
};

void audio_fade_in_fade_out(u8 left_vol, u8 right_vol);
extern u32 read_capless_DTB(void);

int get_dac_channel_num(void)
{
    return dac_hdl.channel;
}

void audio_dac_initcall(void)
{
    printf("audio_dac_initcall\n");

    dac_data.bit_width = audio_general_out_dev_bit_width();
    if (dac_data.mode == DAC_MODE_H2_SINGLE) {
        dac_data.power_boost = TCFG_AUDIO_DAC_POWER_BOOST;
        //power_boost使能的情况下，ldo_volt最大可配置到3，再提高一档输出幅度
        dac_data.ldo_volt = dac_data.power_boost ? 2 : 0;
    } else {
        dac_data.power_boost = 0;
        dac_data.ldo_volt = 0;
    }
    dac_data.max_sample_rate    = AUDIO_DAC_MAX_SAMPLE_RATE;
    audio_dac_init(&dac_hdl, &dac_data);
    /* dac_hdl.ng_threshold = 4; //dac底噪优化阈值 */

#if defined(TCFG_AUDIO_DAC_24BIT_MODE) && TCFG_AUDIO_DAC_24BIT_MODE
    audio_dac_set_bit_mode(&dac_hdl, 1);
#endif

    audio_dac_set_analog_vol(&dac_hdl, 0);

    request_irq(IRQ_AUDIO_IDX, 2, audio_irq_handler, 0);
    struct audio_dac_trim dac_trim = {0};
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(dac_trim)) {
        audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
        syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    }
#if TCFG_SUPPORT_MIC_CAPLESS
    audio_dac_set_capless_DTB(&dac_hdl, read_capless_DTB());
    mic_capless_trim_run();
#endif
    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    /* audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt)); */

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_open();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE
}

#if defined(TCFG_AUDIO_DAC_IO_ENABLE) && TCFG_AUDIO_DAC_IO_ENABLE
static void audio_dac_io_initcall()
{
    struct audio_dac_io_param param = {0};
    param.state[0] = 1;                 //左声道初始电平状态
    param.state[1] = 1;                 //右声道初始电平状态
    param.irq_points = 256;
    param.channel = BIT(0) | BIT(1);    //使能左右声道
    param.digital_gain = 16384;         //数字音量：-16384~16384
    param.ldo_volt = 0;                 //电压档位：0~3
    //此外IOVDD配置也会影响DAC输出电平
    audio_dac_io_init(&param);
}
#endif

static u8 audio_init_complete()
{
    if (!__this->audio_inited) {
        return 0;
    }
    return 1;
}

REGISTER_LP_TARGET(audio_init_lp_target) = {
    .name = "audio_init",
    .is_idle = audio_init_complete,
};

void audio_fast_mode_test()
{
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo_open(AUDIO_ADC_MIC_CH, 10, 16000, 1);

}

struct audio_adc_private_param adc_private_param = {
    .mic_ldo_vsel   = TCFG_AUDIO_MIC_LDO_VSEL,
    .mic_ldo_isel   = TCFG_AUDIO_MIC_LDO_ISEL,
    .adca_reserved0 = 0,
    .adcb_reserved0 = 0,
    .lowpower_lvl = 0,
};

#if TCFG_AUDIO_ADC_ENABLE
const struct adc_platform_cfg adc_platform_cfg_table[AUDIO_ADC_MAX_NUM] = {
#if TCFG_ADC0_ENABLE
    [0] = {
        .mic_mode           = TCFG_ADC0_MODE,
        .mic_ain_sel        = TCFG_ADC0_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC0_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC0_BIAS_RSEL,
        .power_io           = TCFG_ADC0_POWER_IO,
        .mic_dcc            = TCFG_ADC0_DCC_LEVEL,
    },
#endif
#if TCFG_ADC1_ENABLE
    [1] = {
        .mic_mode           = TCFG_ADC1_MODE,
        .mic_ain_sel        = TCFG_ADC1_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC1_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC1_BIAS_RSEL,
        .power_io           = TCFG_ADC1_POWER_IO,
        .mic_dcc            = TCFG_ADC1_DCC_LEVEL,
    },
#endif
#if TCFG_ADC2_ENABLE
    [2] = {
        .mic_mode           = TCFG_ADC2_MODE,
        .mic_ain_sel        = TCFG_ADC2_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC2_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC2_BIAS_RSEL,
        .power_io           = TCFG_ADC2_POWER_IO,
        .mic_dcc            = TCFG_ADC2_DCC_LEVEL,
    },
#endif
#if TCFG_ADC3_ENABLE
    [3] = {
        .mic_mode           = TCFG_ADC3_MODE,
        .mic_ain_sel        = TCFG_ADC3_AIN_SEL,
        .mic_bias_sel       = TCFG_ADC3_BIAS_SEL,
        .mic_bias_rsel      = TCFG_ADC3_BIAS_RSEL,
        .power_io           = TCFG_ADC3_POWER_IO,
        .mic_dcc            = TCFG_ADC3_DCC_LEVEL,
    },
#endif
};
#endif

#if TCFG_SMART_VOICE_ENABLE
static struct vad_mic_platform_data vad_mic_data = {
    .mic_data = {
        /*mic类型：隔直/差分/省电容*/
        .mic_mode = AUDIO_MIC_CAP_DIFF_MODE,
        .mic_ldo_isel = 2,
        .mic_ldo_vsel = 5,
        .mic_ldo2PAD_en = 1,
        .mic_bias_en = 0,
        .mic_bias_res = 0,
        /*是否使用mic_bias*/
        .mic_bias_inside = 0,
        /* ADC偏置电阻配置*/
        .adc_rbs = 3,
        /* ADC输入电阻配置*/
        .adc_rin = 3,
        /* .adc_test = 1, */
    },
    .power_data = {
        /*VADLDO电压档位：0~7*/
        .ldo_vs = 3,
        /*VADLDO误差运放电流档位：0~3*/
#if TCFG_VAD_LOWPOWER_CLOCK == VAD_CLOCK_USE_PMU_STD12M
        .ldo_is = 1,
#else
        .ldo_is = 2,
#endif
        .clock = TCFG_VAD_LOWPOWER_CLOCK, /*VAD时钟选项*/
        .acm_select = 8,
    },
};
#endif

extern const u8 const_adc_async_en;
void audio_input_initcall(void)
{
    printf("audio_input_initcall\n");
#if TCFG_MC_BIAS_AUTO_ADJUST
    int ret = syscfg_read(CFG_MC_BIAS, &save_mic_bias_rsel[0], 1);
    printf("mic_bias_res:%d", save_mic_bias_rsel[0]);
    if (ret != 1) {
        printf("mic_bias_adjust NULL");
    } else {
        mic_bias_rsel_use_save[0] = 1;
    }
    u8 mic_ldo_idx;
    ret = syscfg_read(CFG_MIC_LDO_VSEL, &mic_ldo_idx, 1);
    if (ret == 1) {
        save_mic_ldo_vsel = mic_ldo_idx & 0x3;
        mic_ldo_vsel_use_save = 1;
        printf("mic_ldo_vsel:%d,%d\n", save_mic_ldo_vsel, mic_ldo_idx);
    } else {
        printf("mic_ldo_vsel_adjust NULL\n");
    }
    if (mic_ldo_vsel_use_save) {
        adc_private_param.mic_ldo_vsel = save_mic_ldo_vsel;
    }
#endif

    audio_adc_init(&adc_hdl, &adc_private_param);
    audio_adc_file_init();

#if TCFG_APP_FM_EN
    audio_fm_file_init();
#endif

#if TCFG_AUDIO_LINEIN_ENABLE
    audio_linein_file_init();
#endif/*TCFG_AUDIO_LINEIN_ENABLE*/
    if (const_adc_async_en) {
        //ICSD ADT启动时不可注册所有ADC_CH
#if !((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN)
        audio_all_adc_file_init();
#endif
    }
}

#ifndef TCFG_AUDIO_DAC_LDO_VOLT_HIGH
#define TCFG_AUDIO_DAC_LDO_VOLT_HIGH 0
#endif

static void wl_audio_clk_on(void)
{
    JL_WL_AUD->CON0 = 1;
}

extern int audio_timer_init(void);

/*audio模块初始化*/
static int audio_init()
{
    puts("audio_init\n");

    wl_audio_clk_on();

    audio_general_init();
    audio_input_initcall();

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif

#if TCFG_DAC_NODE_ENABLE
    audio_dac_initcall();
#elif (defined(TCFG_AUDIO_DAC_IO_ENABLE) && TCFG_AUDIO_DAC_IO_ENABLE)
    audio_dac_io_initcall();
#elif (defined(TCFG_AUDIO_ADC_ENABLE) && TCFG_AUDIO_ADC_ENABLE)
    extern void audio_dac_io_clock_init();
    audio_dac_io_clock_init();
    request_irq(IRQ_AUDIO_IDX, 2, audio_irq_handler, 0);
#endif

#if TCFG_AUDIO_ANC_ENABLE
    anc_init();
#endif

#if TCFG_AUDIO_DUT_ENABLE
    audio_dut_init();
#endif /*TCFG_AUDIO_DUT_ENABLE*/

#if TCFG_SMART_VOICE_ENABLE
    /*获取mic0的配置*/
    vad_mic_data.mic_data.mic_mode = audio_adc_file_get_mic_mode(0);
    vad_mic_data.mic_data.mic_bias_inside = (audio_adc_file_get_mic_mode(0) == AUDIO_MIC_CAP_DIFF_MODE) ? 0 : 1;
    audio_smart_voice_detect_init((struct vad_mic_platform_data *)&vad_mic_data);
#endif /* #if TCFG_SMART_VOICE_ENABLE */

#if TCFG_AUDIO_ANC_ENABLE && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    /*需要再dac trim后调用*/
    /*同时开ANC和免摘功能时，配置ADC固定打开的adc数字通道数*/
    u16 mic_ch = 0;
    if (TCFG_AUDIO_ANCL_FF_MIC != MIC_NULL) {
        mic_ch |= BIT(TCFG_AUDIO_ANCL_FF_MIC);
    }
    if (TCFG_AUDIO_ANCL_FB_MIC != MIC_NULL) {
        mic_ch |= BIT(TCFG_AUDIO_ANCL_FB_MIC);
    }

    if (get_icsd_adt_mic_num() == 3) {
        mic_ch |= icsd_get_talk_mic_ch();
    }

    for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
        if (mic_ch & BIT(i)) {
            audio_adc_add_ch(&adc_hdl, i);
        }
    }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

    audio_timer_init();
    __this->audio_inited = 1;
    return 0;
}
platform_initcall(audio_init);

static void audio_uninit()
{
    dac_power_off();
}
platform_uninitcall(audio_uninit);


/*关闭audio相关模块使能*/
void audio_disable_all(void)
{
    //DAC:DACEN
    JL_AUDIO->DAC_CON &= ~BIT(4);
    //ADC:ADCEN
    JL_AUDIO->ADC_CON &= ~BIT(4);
    //EQ:
    JL_EQ->CON0 &= ~BIT(1);
    //FFT:
    JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //ANC:anc_en anc_start
#if 0//build
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));
#endif

}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};

/*
 *capless模式一开始不要的数据包数量
 */
u16 get_ladc_capless_dump_num(void)
{
    return 10;
}


/*
 *dac快速校准
 */
//#define DAC_TRIM_FAST_EN
#ifdef DAC_TRIM_FAST_EN
u8 dac_trim_fast_en()
{
    return 1;
}
#endif/*DAC_TRIM_FAST_EN*/

/*
 *自定义dac上电延时时间，具体延时多久应通过示波器测量
 */
#if 1
void dac_power_on_delay()
{
    /* #if TCFG_MC_BIAS_AUTO_ADJUST */
    /* void mic_capless_auto_adjust_init(); */
    /* mic_capless_auto_adjust_init(); */
    /* #endif */
    os_time_dly(50);
}
#endif

void dac_power_on(void)
{
    /* log_info(">>>dac_power_on:%d", __this->ref.counter); */
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
    audio_dac_close(&dac_hdl);
}


#define TRIM_VALUE_LR_ERR_MAX           (600)   // 距离参考值的差值限制
#define abs(x) ((x)>0?(x):-(x))
int audio_dac_trim_value_check(struct audio_dac_trim *dac_trim)
{
    printf("audio_dac_trim_value_check %d %d\n", dac_trim->left, dac_trim->right);
    s16 reference = 0;
    if (TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_R) {
        if (abs(dac_trim->left - reference) > TRIM_VALUE_LR_ERR_MAX) {
            printf("dac trim channel l err\n");
            return -1;
        }
    }
    if (TCFG_AUDIO_DAC_CONNECT_MODE != DAC_OUTPUT_MONO_L) {
        if (abs(dac_trim->right - reference) > TRIM_VALUE_LR_ERR_MAX) {
            printf("dac trim channel r err\n");
            return -1;
        }
    }

    return 0;
}

/*音频模块寄存器跟踪*/
void audio_adda_dump(void) //打印所有的dac,adc寄存器
{
    printf("JL_WL_AUD CON0:%x", JL_WL_AUD->CON0);
    printf("AUD_CON:%x", JL_AUDIO->AUD_CON);
    printf("DAC_CON:%x", JL_AUDIO->DAC_CON);
    printf("ADC_CON:%x", JL_AUDIO->ADC_CON);
    printf("ADC_CON1:%x", JL_AUDIO->ADC_CON1);
    printf("DAC_VL0:%x", JL_AUDIO->DAC_VL0);
    printf("DAC_TM0:%x", JL_AUDIO->DAC_TM0);
    printf("DAA_CON 0:%x 	1:%x,	2:%x,	3:%x    7:%x\n", JL_ADDA->DAA_CON0, JL_ADDA->DAA_CON1, JL_ADDA->DAA_CON2, JL_ADDA->DAA_CON3, JL_ADDA->DAA_CON7);
    printf("ADA_CON 0:%x	1:%x	2:%x	3:%x	4:%x	5:%x	6:%x	7:%x	8:%x\n", JL_ADDA->ADA_CON0, JL_ADDA->ADA_CON1, JL_ADDA->ADA_CON2, JL_ADDA->ADA_CON3, JL_ADDA->ADA_CON4, JL_ADDA->ADA_CON5, JL_ADDA->ADA_CON6, JL_ADDA->ADA_CON7, JL_ADDA->ADA_CON8);
}

/*音频模块配置跟踪*/
void audio_config_dump()
{
    u8 dac_bit_width = ((JL_AUDIO->DAC_CON & BIT(20)) ? 24 : 16);
    u8 adc_bit_width = ((JL_AUDIO->ADC_CON & BIT(20)) ? 24 : 16);
    int dac_dgain_max = 16384;
    int dac_again_max = 7;
    int mic_gain_max = 19;
    u8 dac_dcc = (JL_AUDIO->DAC_CON >> 12) & 0xF;
    u8 mic0_dcc = (JL_AUDIO->ADC_CON >> 12) & 0xF;
    u8 mic1_dcc = JL_AUDIO->ADC_CON1 & 0xF;
    u8 mic2_dcc = (JL_AUDIO->ADC_CON1 >> 4) & 0xF;
    u8 mic3_dcc = (JL_AUDIO->ADC_CON1 >> 8) & 0xF;

    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 4) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_0_6 = JL_ADDA->ADA_CON4 & 0x1;
    u8 mic1_0_6 = JL_ADDA->ADA_CON5 & 0x1;
    u8 mic2_0_6 = JL_ADDA->ADA_CON6 & 0x1;
    u8 mic3_0_6 = JL_ADDA->ADA_CON7 & 0x1;
    u8 mic0_gain = JL_ADDA->ADA_CON8 & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON8 >> 5) & 0x1F;
    u8 mic2_gain = (JL_ADDA->ADA_CON8 >> 10) & 0x1F;
    u8 mic3_gain = (JL_ADDA->ADA_CON8 >> 15) & 0x1F;
    int dac_sr = audio_dac_get_sample_rate_base_reg();
    int adc_sr = audio_adc_mic_get_sample_rate();


    printf("[ADC]BitWidth:%d,DCC:%d,%d,%d,%d,SR:%d\n", adc_bit_width, mic0_dcc, mic1_dcc, mic2_dcc, mic3_dcc, adc_sr);
    printf("[ADC]Gain(Max:%d):%d,%d,%d,%d,6dB_Boost:%d,%d,%d,%d,\n", mic_gain_max, mic0_gain, mic1_gain, mic2_gain, mic3_gain, \
           mic0_0_6, mic1_0_6, mic2_0_6, mic3_0_6);

    printf("[DAC]BitWidth:%d,DCC:%d,SR:%d\n", dac_bit_width, dac_dcc, dac_sr);
    printf("[DAC]AGain(Max:%d):%d,%d,DGain(Max:%d):%d,%d\n", dac_again_max, dac_again_l, dac_again_r, \
           dac_dgain_max, dac_dgain_l, dac_dgain_r);

    //short anc_gain = 0;//JL_ANC->CON5 & 0xFFFF;
    //printf("[ANC]Gain:%d\n",anc_gain);

}


