#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_config.data.bss")
#pragma data_seg(".audio_config.data")
#pragma const_seg(".audio_config.text.const")
#pragma code_seg(".audio_config.text")
#endif
/*
 ******************************************************************************************
 *							Audio Config
 *
 * Discription: 音频模块配置
 *
 * Notes:
 ******************************************************************************************
 */
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"

/*
 *******************************************************************
 *						Audio Codec Config
 *******************************************************************
 */
const int config_audio_dac_mix_enable = 1;

//***********************
//*		AAC Codec       *
//***********************
#ifdef AAC_DEC_IN_MASK
const int AAC_DEC_MP4A_LATM_ANALYSIS = 0;
const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;
#else
const int AAC_DEC_MP4A_LATM_ANALYSIS = 1;
const int AAC_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
#endif

#ifdef MP3_DEC_IN_MASK
const int MP3_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;
#else
const int MP3_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
#endif

#ifdef WTS_DEC_IN_MASK
const int WTS_DEC_LIB_SUPPORT_24BIT_OUTPUT = 0;
#else
const int WTS_DEC_LIB_SUPPORT_24BIT_OUTPUT = 1;
#endif

//***********************
//*		Audio Params    *
//***********************
void audio_adc_param_fill(struct mic_open_param *mic_param, struct adc_platform_cfg *platform_cfg)
{
    mic_param->mic_mode      = platform_cfg->mic_mode;
    mic_param->mic_ain_sel   = platform_cfg->mic_ain_sel;
    mic_param->mic_bias_sel  = platform_cfg->mic_bias_sel;
    mic_param->mic_bias_rsel = platform_cfg->mic_bias_rsel;
    mic_param->mic_dcc       = platform_cfg->mic_dcc;
}

void audio_linein_param_fill(struct linein_open_param *linein_param, const struct adc_platform_cfg *platform_cfg)
{
    linein_param->linein_mode    = platform_cfg->mic_mode;
    linein_param->linein_ain_sel = platform_cfg->mic_ain_sel;
    linein_param->linein_dcc     = platform_cfg->mic_dcc;
}
