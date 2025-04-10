#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_dc_offset_remove.data.bss")
#pragma data_seg(".audio_dc_offset_remove.data")
#pragma const_seg(".audio_dc_offset_remove.text.const")
#pragma code_seg(".audio_dc_offset_remove.text")
#endif
/*
 ****************************************************************
 *							Audio DC-Offset Remove
 * Brief : 去除直流偏移
 * Notes :
 *
 ****************************************************************
 */
#include "audio_dc_offset_remove.h"
#include "app_config.h"

//一段高通滤波器 可调中心截止频率、带宽
//默认Freq:100Hz Gain:0  Q:0.7f
static struct eq_seg_info dccs_eq_tab_8k[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 100,   0, 0.7f},
};
static struct eq_seg_info dccs_eq_tab_16k[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 100,   0, 0.7f},
};

//使用EQ模块去除直流偏置配置
#if TCFG_EQ_ENABLE
#define DCC_VIA_EQ_ENABLE			1
#else
#define DCC_VIA_EQ_ENABLE			0
#endif/*TCFG_EQ_ENABLE*/

/*省电容mic通过eq模块实现去直流滤波*/
#if TCFG_SUPPORT_MIC_CAPLESS
#if (TCFG_EQ_ENABLE == 0 || (DCC_VIA_EQ_ENABLE == 0))
#error "MicCapless enable,Please enable TCFG_EQ_ENABLE and DCC_VIA_EQ_ENABLE"
#endif/*TCFG_EQ_ENABLE*/
#endif/*TCFG_SUPPORT_MIC_CAPLESS*/

/*
*********************************************************************
*                  Audio DC-Offset Remove Open
* Description:
* Arguments  : sample_rate
*              ch_num
* Return	 : Null
* Note(s)    :
*********************************************************************
*/
void *audio_dc_offset_remove_open(u32 sample_rate, u32 ch_num)
{
#if DCC_VIA_EQ_ENABLE
    struct audio_eq_param eq_param = {0};
    eq_param.channels = ch_num;
    eq_param.sr = sample_rate;
    eq_param.cb = eq_get_filter_info;
    eq_param.global_gain = 0;
    if (sample_rate == 8000) {
        eq_param.max_nsection = eq_param.nsection = ARRAY_SIZE(dccs_eq_tab_8k);
        eq_param.seg = dccs_eq_tab_8k;
    } else {
        eq_param.max_nsection = eq_param.nsection = ARRAY_SIZE(dccs_eq_tab_16k);
        eq_param.seg = dccs_eq_tab_16k;
    }
    struct audio_eq *eq = audio_dec_eq_open(&eq_param);
    return (void *)eq;
#else
    return NULL;
#endif
}

/*
*********************************************************************
*                  Audio DC-Offset Remove Run
* Description:
* Arguments  : hdl
*			   data
*			   len
* Return	 : Null
* Note(s)    :
*********************************************************************
*/
void audio_dc_offset_remove_run(void *hdl, void *data, u32 len)
{
#if DCC_VIA_EQ_ENABLE
    if (hdl) {
        audio_dec_eq_run(hdl, (s16 *)data, (s16 *)data, len);
    }
#endif
}

/*
*********************************************************************
*                  Audio DC-Offset Remove Close
* Description:
* Arguments  : hdl
* Return	 : Null
* Note(s)    :
*********************************************************************
*/
void audio_dc_offset_remove_close(void *hdl)
{
#if DCC_VIA_EQ_ENABLE
    if (hdl) {
        audio_dec_eq_close(hdl);
    }
#endif
}



