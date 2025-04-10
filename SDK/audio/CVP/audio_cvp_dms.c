#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp_dms.data.bss")
#pragma data_seg(".audio_cvp_dms.data")
#pragma const_seg(".audio_cvp_dms.text.const")
#pragma code_seg(".audio_cvp_dms.text")
#endif
/*
 ****************************************************************
 *							AUDIO DMS(DualMic System)
 * File  : audio_aec_dms.c
 * By    :
 * Notes : AEC回音消除 + 双mic降噪(ENC)
 *
 ****************************************************************
 */
#include "audio_cvp.h"
#include "system/includes.h"
#include "media/includes.h"
#include "effects/eq_config.h"
#include "effects/audio_pitch.h"
#include "circular_buf.h"
#include "audio_cvp_online.h"
#include "cvp_node.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/
#include "overlay_code.h"
#if TCFG_AUDIO_CVP_SYNC
#include "audio_cvp_sync.h"
#endif/*TCFG_AUDIO_CVP_SYNC*/
#include "audio_dc_offset_remove.h"
#include "audio_cvp_def.h"
#include "effects/audio_gain_process.h"
#include "app_main.h"
#include "lib_h/jlsp_ns.h"
#if TCFG_AUDIO_DUT_ENABLE
//#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#if !defined(TCFG_CVP_DEVELOP_ENABLE) || (TCFG_CVP_DEVELOP_ENABLE == 0)


#if TCFG_AUDIO_DUAL_MIC_ENABLE
#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#include "audio_cvp_debug.c"
#include "adc_file.h"

//*********************************************************************************//
//                          DMS COMMON 配置                                        //
//*********************************************************************************//
#define AEC_USER_MALLOC_ENABLE	1

/*CVP_TOGGLE:CVP模块(包括AEC、NLP、NS等)总开关，Disable则数据完全不经过处理，释放资源*/
#define CVP_TOGGLE				1

/*数据输出开头丢掉的数据包数*/
#define AEC_OUT_DUMP_PACKET		15
/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1

/*使能即可跟踪通话过程的内存情况*/
#define CVP_MEM_TRACE_ENABLE	0

/*
 *延时估计使能
 *点烟器需要做延时估计
 *其他的暂时不需要做
 */
const u8 CONST_AEC_DLY_EST = 0;

/*骨传导配置*/
const u8 CONST_BONE_CONDUCTION_ENABLE = 0;	/*骨传导使能*/
const u8 CONST_BCS_MAP_WEIGHT_SAVE = 0;		/*骨传导收敛信息保存*/

/********麦克风异常检测配置*********/
/*麦克风异常检测，双mic切单mic*/
const u8 CONST_DMS_MALFUNCTION_DETECT = 1;
/*
 * 是否设置通话mic默认状态
 * 需要重回定义audio_dms_get_malfunc_state(void)
 */
const u8 CONST_DMS_SET_MFDT_STATE = 1;

//**************************DMS COMMON 配置 end************************************//

//*********************************************************************************//
//                          DMS_GLOBAL_V100(beamfroming)配置                       //
//*********************************************************************************//
/*
 *ANS版本配置
 *DMS_V100:传统降噪
 *DMS_V200:AI降噪，需要更多的ram和mips
 **/
#if (TCFG_AUDIO_CVP_NS_MODE == CVP_ANS_MODE)
const u8 CONST_DMS_VERSION = DMS_V100;
#else
const u8 CONST_DMS_VERSION = DMS_V200;
#endif/*TCFG_AUDIO_CVP_NS_MODE*/

/********DNS配置********/
/* SNR估计,可以实现场景适配 */
const u8 CONST_DNS_SNR_EST = 0;//注意：双麦没有该功能
/* DNS后处理 */
const u8 CONST_DNS_POST_ENHANCE = 0;
/*
 *风噪自适应GainFloor使能控制
 *0:关闭, 1:开启
 */
const u8 CONST_DMS_WNC = 0;

/*
 * 风噪下ENC是否保留人声
 * 0:加强对风噪的压制，
 * 1:保留更多的人声
 */
const u8 CONST_DMS_WINDREPLACE = 1;

/*
 * 副麦回音消除使能控制
 * 0 : 不对副mic做AEC
 * 1 : 对副mic做AEC(需要打开AEC模块)
 */
const u8 CONST_DMS_REF_MIC_AEC_ENABLE = 0;

//**************************DMS_GLOBAL_V100配置 end*******************************//

//*********************************************************************************//
//                          DMS_GLOBAL_V200(beamfroming)配置                       //
//*********************************************************************************//
/*
 *DMS版本配置
 *DMS_GLOBAL_V100:第一版双麦算法
 *DMS_GLOBAL_V200:第二版双麦算法，更少的ram和mips
 注意：
 双麦耳机使用ans时,需要配置使用DMS_GLOBAL_V100
 话务耳机使用dns时,需要配置使用DMS_GLOBAL_V100
 */
#if ((TCFG_AUDIO_DMS_GLOBAL_VERSION == DMS_GLOBAL_V200) && (TCFG_AUDIO_DMS_SEL == DMS_NORMAL) && (TCFG_AUDIO_CVP_NS_MODE == CVP_DNS_MODE))
const u8 CONST_DMS_GLOBAL_VERSION = DMS_GLOBAL_V200;
#else
const u8 CONST_DMS_GLOBAL_VERSION = DMS_GLOBAL_V100;
#endif

/*
 * 非线性压制模式选择
 * JLSP_NLP_MODE1: 模式1为单独的NLP回声抑制，回声压制会偏过，该模式下NLP模块可以单独开启
 * JLSP_NLP_MODE2: 模式2下回声信号会先经过AEC线性压制，然后进行NLP非线性压制
 *                 此模式NLP不能单独打开需要同时打开AEC,使用AEC模块压制不够时，建议开启该模式
 */
const u8 CONST_JLSP_NLP_MODE = JLSP_NLP_MODE1;

/*
 * 风噪降噪模式选择
 * JLSP_WD_MODE1: 模式1为常规的降风噪模式，风噪残余会偏大些
 * JLSP_WD_MODE2: 模式2为神经网络降风噪，风噪抑制会比较干净，但是会需要多消耗31kb的flash
 */
const u8 CONST_JLSP_WD_EN = 0;
const u8 CONST_JLSP_WD_MODE = JLSP_WD_MODE1;

//**************************DMS_GLOBAL_V200配置 end*******************************//

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

__attribute__((weak))u32 usb_mic_is_running()
{
    return 0;
}

/*复用lmp rx buf(一般通话的时候复用)
 *rx_buf概率产生碎片，导致alloc失败，因此默认配0
 */
#define MALLOC_MULTIPLEX_EN		0
extern void *lmp_malloc(int);
extern void lmp_free(void *);
void *zalloc_mux(int size)
{
#if MALLOC_MULTIPLEX_EN
    void *p = NULL;
    do {
        p = lmp_malloc(size);
        if (p) {
            break;
        }
        printf("aec_malloc wait...\n");
        os_time_dly(2);
    } while (1);
    if (p) {
        memset(p, 0, size);
    }
    printf("[malloc_mux]p = 0x%x,size = %d\n", p, size);
    return p;
#else
    return zalloc(size);
#endif
}

void free_mux(void *p)
{
#if MALLOC_MULTIPLEX_EN
    printf("[free_mux]p = 0x%x\n", p);
    lmp_free(p);
#else
    free(p);
#endif
}

struct audio_aec_hdl {
    u8 start;				//aec模块状态
    u8 inbuf_clear_cnt;		//aec输入数据丢掉
    u8 output_fade_in;		//aec输出淡入使能
    u8 output_fade_in_gain;	//aec输出淡入增益
    u8 EnableBit;			//aec使能模块
    u8 input_clear;			//清0输入数据标志
    u16 dump_packet;		//前面如果有杂音，丢掉几包
#if TCFG_SUPPORT_MIC_CAPLESS
    void *dcc_hdl;
#endif
    struct dms_attr attr;	//aec模块参数属性
    struct audio_cvp_pre_param_t pre;	//预处理配置
    void *transfer_func;
};
#if AEC_USER_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
struct audio_aec_hdl *aec_hdl = &aec_handle;
#endif/*AEC_USER_MALLOC_ENABLE*/

int audio_cvp_probe_param_update(struct audio_cvp_pre_param_t *cfg)
{
    if (aec_hdl) {
        aec_hdl->pre = *cfg;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Probe
* Description: AEC模块数据前处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在源数据经过AEC模块前，可以增加自定义处理
*********************************************************************
*/
static int audio_aec_probe(short *talk_mic, short *talk_ref_mic, short *mic3, short *ref, u16 len)
{
#if TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE
    //表示使用主副麦差值计算，且仅减小增益
    if (app_var.audio_mic_array_trim_en) {
        if (app_var.audio_mic_array_diff_cmp != 1.0f) {
            if (app_var.audio_mic_array_diff_cmp > 1.0f) {
                float mic0_gain = 1.0 / app_var.audio_mic_array_diff_cmp;
                GainProcess_16Bit(talk_mic, talk_mic, mic0_gain, 1, 1, 1, len >> 1);
            } else {
                float mic1_gain = app_var.audio_mic_array_diff_cmp;
                GainProcess_16Bit(talk_ref_mic, talk_ref_mic, mic1_gain, 1, 1, 1, len >> 1);
            }
        } else {       //表示使用每个MIC与金机曲线的差值
            GainProcess_16Bit(talk_mic, talk_mic, app_var.audio_mic_cmp.talk, 1, 1, 1, len >> 1);
            GainProcess_16Bit(talk_ref_mic, talk_ref_mic, app_var.audio_mic_cmp.ff, 1, 1, 1, len >> 1);
        }
    }
#endif

    if (aec_hdl->pre.pre_gain_en) {
        GainProcess_16Bit(talk_mic, talk_mic, aec_hdl->pre.talk_mic_gain, 1, 1, 1, len >> 1);
        GainProcess_16Bit(talk_ref_mic, talk_ref_mic, aec_hdl->pre.talk_ref_mic_gain, 1, 1, 1, len >> 1);
    }
#if TCFG_SUPPORT_MIC_CAPLESS
    if (aec_hdl->dcc_hdl) {
        audio_dc_offset_remove_run(aec_hdl->dcc_hdl, (void *)talk_mic, len);
    }
#endif
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Post
* Description: AEC模块数据后处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在数据处理完毕，可以增加自定义后处理
*********************************************************************
*/
static int audio_aec_post(s16 *data, u16 len)
{
    return 0;
}


/*跟踪系统内存使用情况:physics memory size xxxx bytes*/
static void sys_memory_trace(void)
{
    static int cnt = 0;
    if (cnt++ > 200) {
        cnt = 0;
        mem_stats();
    }
}

int audio_aec_sync_buffer_set(s16 *data, int len)
{
    return cvp_node_output_handle(data, len);
}
/*
 *********************************************************************
 *                  Audio AEC Output Handle
 * Description: AEC模块数据输出回调
 * Arguments  : data 输出数据地址
 *			   len	输出数据长度
 * Return	 : 数据输出消耗长度
 * Note(s)    : None.
 *********************************************************************
 */
extern void esco_enc_resume(void);
static int audio_aec_output(s16 *data, u16 len)
{
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    //Voice Recognition get mic data here
    extern void kws_aec_data_output(void *priv, s16 * data, int len);
    kws_aec_data_output(NULL, data, len);

#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if CVP_MEM_TRACE_ENABLE
    sys_memory_trace();
#endif/*CVP_MEM_TRACE_ENABLE*/

    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
        if (aec_hdl->output_fade_in) {
            s32 tmp_data;
            //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
            for (int i = 0; i < len / 2; i++) {
                tmp_data = data[i];
                data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
            }
            aec_hdl->output_fade_in_gain += 12;
            if (aec_hdl->output_fade_in_gain >= 128) {
                aec_hdl->output_fade_in = 0;
            }
        }
    }
#if TCFG_AUDIO_CVP_SYNC
    audio_cvp_sync_run(data, len);
    return len;
#endif/*TCFG_AUDIO_CVP_SYNC*/
    return cvp_node_output_handle(data, len);
}

/*
*********************************************************************
*                  Audio AEC Parameters
* Description: AEC模块配置参数
* Arguments  : p	参数指针
* Return	 : None.
* Note(s)    : 读取配置文件成功，则使用配置文件的参数配置，否则使用默
*			   认参数配置
*********************************************************************
*/
static void audio_aec_param_init(struct dms_attr *p)
{
    int ret = 0;
    AEC_DMS_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    //APP在线调试，APP参数覆盖工具配置参数(不覆盖预处理参数)
    ret = aec_cfg_online_update_fill(&cfg, sizeof(AEC_DMS_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    log_info("CVP_DMS_NS_MODE = %d\n", TCFG_AUDIO_CVP_NS_MODE);
    if (ret == sizeof(AEC_DMS_CONFIG)) {
        log_info("read dms_param ok\n");
        p->EnableBit = cfg.enable_module;
        p->ul_eq_en = cfg.ul_eq_en;
        p->AGC_NDT_fade_in_step = cfg.ndt_fade_in;
        p->AGC_NDT_fade_out_step = cfg.ndt_fade_out;
        p->AGC_DT_fade_in_step = cfg.dt_fade_in;
        p->AGC_DT_fade_out_step = cfg.dt_fade_out;
        p->AGC_NDT_max_gain = cfg.ndt_max_gain;
        p->AGC_NDT_min_gain = cfg.ndt_min_gain;
        p->AGC_NDT_speech_thr = cfg.ndt_speech_thr;
        p->AGC_DT_max_gain = cfg.dt_max_gain;
        p->AGC_DT_min_gain = cfg.dt_min_gain;
        p->AGC_DT_speech_thr = cfg.dt_speech_thr;
        p->AGC_echo_present_thr = cfg.echo_present_thr;

        p->aec_process_maxfrequency = cfg.aec_process_maxfrequency;
        p->aec_process_minfrequency = cfg.aec_process_minfrequency;
        p->af_length = cfg.af_length;

        p->nlp_process_maxfrequency = cfg.nlp_process_maxfrequency;
        p->nlp_process_minfrequency = cfg.nlp_process_minfrequency;
        p->overdrive = cfg.overdrive;

        p->aggressfactor = cfg.aggressfactor;
        p->minsuppress = cfg.minsuppress;
        p->init_noise_lvl = cfg.init_noise_lvl;
        p->DNS_highGain = 2.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.5f;   /*混响强度，范围：0~0.9f,越大越强*/

        p->wn_detect_time = 0.32f;
        p->wn_detect_time_ratio_thr = 0.8f;
        p->wn_detect_thr = 0.5f;
        p->wn_minsuppress = 0.5f;

        p->enc_process_maxfreq = cfg.enc_process_maxfreq;
        p->enc_process_minfreq = cfg.enc_process_minfreq;
        p->sir_maxfreq = cfg.sir_maxfreq;
        p->mic_distance = cfg.mic_distance;
        p->target_signal_degradation = cfg.target_signal_degradation;
        p->enc_aggressfactor = cfg.enc_aggressfactor;
        p->enc_minsuppress = cfg.enc_minsuppress;

        p->adc_ref_en = cfg.adc_ref_en;

        p->global_minsuppress = cfg.global_minsuppress;

        p->detect_time = cfg.detect_time;            // in second
        /*0~-90 dB 两个mic能量差异持续大于此阈值超过检测时间则会检测为故障*/
        p->detect_eng_diff_thr = cfg.detect_eng_diff_thr;     //  dB
        /*0~-90 dB 当处于故障状态时，正常的mic能量大于此阈值才会检测能量差异，避免安静环境下误判切回正常状态*/
        p->detect_eng_lowerbound = cfg.detect_eng_lowerbound; // 0~-90 dB start detect when mic energy lower than this
        p->MalfuncDet_MaxFrequency = cfg.MalfuncDet_MaxFrequency;  //检测频率上限
        p->MalfuncDet_MinFrequency = cfg.MalfuncDet_MinFrequency;   //检测频率下限
        p->OnlyDetect = cfg.OnlyDetect;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换

        p->output_sel = cfg.output_sel;

    } else {
        p->EnableBit = NLP_EN | ANS_EN | ENC_EN | AGC_EN;
        p->ul_eq_en = 1;
        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        p->aec_process_maxfrequency = 8000;
        p->aec_process_minfrequency = 0;
        p->af_length = 128;

        p->nlp_process_maxfrequency = 8000;
        p->nlp_process_minfrequency = 0;
        p->overdrive = 1;

#if (TCFG_AUDIO_CVP_NS_MODE == CVP_ANS_MODE)
        p->aggressfactor = 1.25f;
        p->minsuppress = 0.04f;
#else /*(TCFG_AUDIO_CVP_NS_MODE == CVP_DNS_MODE)*/
        p->aggressfactor = 1.0f;
        p->minsuppress = 0.1f;
#endif/*TCFG_AUDIO_CVP_NS_MODE*/
        p->init_noise_lvl = -75.f;
        p->DNS_highGain = 2.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.5f;   /*混响强度，范围：0~0.9f,越大越强*/

        p->wn_detect_time = 0.32f;
        p->wn_detect_time_ratio_thr = 0.8f;
        p->wn_detect_thr = 0.5f;
        p->wn_minsuppress = 0.5f;

        p->enc_process_maxfreq = 8000;
        p->enc_process_minfreq = 0;
        p->sir_maxfreq = 3000;
        p->mic_distance = 0.015f;
        p->target_signal_degradation = 1;
        p->enc_aggressfactor = 4.f;
        p->enc_minsuppress = 0.09f;

        p->global_minsuppress = 0.04f;

        /*检测时间*/
        p->detect_time = 1.0f;            // in second
        /*0~-90 dB 两个mic能量差异持续大于此阈值超过检测时间则会检测为故障*/
        p->detect_eng_diff_thr = 6.f;     //  dB
        /*0~-90 dB 当处于故障状态时，正常的mic能量大于此阈值才会检测能量差异，避免安静环境下误判切回正常状态*/
        p->detect_eng_lowerbound = -55.f; // 0~-90 dB start detect when mic energy lower than this
        p->MalfuncDet_MaxFrequency = 8000;  //检测频率上限
        p->MalfuncDet_MinFrequency = 400;   //检测频率下限
        p->OnlyDetect = 0;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换

        p->output_sel = DMS_OUTPUT_SEL_DEFAULT;
        log_error("read dms_param default\n");
    }
    log_info("DMS:AEC[%d] NLP[%d] NS[%d] ENC[%d] AGC[%d]", !!(p->EnableBit & AEC_EN), !!(p->EnableBit & NLP_EN), !!(p->EnableBit & ANS_EN), !!(p->EnableBit & ENC_EN), !!(p->EnableBit & AGC_EN));

#if TCFG_AUDIO_MIC_ARRAY_TRIM_ENABLE
    app_var.enc_degradation = p->target_signal_degradation;
#endif

    p->AGC_echo_hold = 0;
    p->AGC_echo_look_ahead = 0;

    if (CONST_BONE_CONDUCTION_ENABLE) {
        p->bone_process_maxfreq = 800;
        p->bone_process_minfreq = 100;
        p->bone_init_noise_lvl = 20.0f;
        p->Bone_AEC_Process_MaxFrequency = 800;
        p->Bone_AEC_Process_MinFrequency  = 100;
    }

    //aec_param_dump(p);
}

static void audio_dms_flexible_param_init(struct dms_attr *p)
{
    int ret = 0;
    DMS_FLEXIBLE_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    ret = aec_cfg_online_update_fill(&cfg, sizeof(DMS_FLEXIBLE_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    log_info("CVP_DMS_Flexible_NS_MODE = %d\n", TCFG_AUDIO_CVP_NS_MODE);
    if (ret == sizeof(cfg)) {
        log_info("read dms_flexible param ok\n");
        p->EnableBit = cfg.enable_module;
        p->ul_eq_en = cfg.ul_eq_en;
        p->AGC_NDT_fade_in_step = cfg.ndt_fade_in;
        p->AGC_NDT_fade_out_step = cfg.ndt_fade_out;
        p->AGC_DT_fade_in_step = cfg.dt_fade_in;
        p->AGC_DT_fade_out_step = cfg.dt_fade_out;
        p->AGC_NDT_max_gain = cfg.ndt_max_gain;
        p->AGC_NDT_min_gain = cfg.ndt_min_gain;
        p->AGC_NDT_speech_thr = cfg.ndt_speech_thr;
        p->AGC_DT_max_gain = cfg.dt_max_gain;
        p->AGC_DT_min_gain = cfg.dt_min_gain;
        p->AGC_DT_speech_thr = cfg.dt_speech_thr;
        p->AGC_echo_present_thr = cfg.echo_present_thr;

        p->aec_process_maxfrequency = cfg.aec_process_maxfrequency;
        p->aec_process_minfrequency = cfg.aec_process_minfrequency;
        p->af_length = cfg.af_length;

        p->nlp_process_maxfrequency = cfg.nlp_process_maxfrequency;
        p->nlp_process_minfrequency = cfg.nlp_process_minfrequency;
        p->overdrive = cfg.overdrive;

        p->aggressfactor = cfg.aggressfactor;
        p->minsuppress = cfg.minsuppress;
        p->init_noise_lvl = cfg.init_noise_lvl;
        p->DNS_highGain = 2.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.5f;   /*混响强度，范围：0~0.9f,越大越强*/

        p->target_signal_degradation = cfg.enc_suppress_pre;
        p->enc_aggressfactor = cfg.enc_suppress_post;
        p->enc_minsuppress = cfg.enc_minsuppress;
        p->Disconverge_ERLE_Thr = cfg.enc_disconverge_erle_thr;

        p->output_sel = cfg.output_sel;
    } else {
        p->EnableBit =  AEC_EN | NLP_EN | ANS_EN | ENC_EN | AGC_EN;
        p->ul_eq_en = 1;
        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        p->aec_process_maxfrequency = 8000;
        p->aec_process_minfrequency = 0;
        p->af_length = 128;

        p->nlp_process_maxfrequency = 8000;
        p->nlp_process_minfrequency = 0;
        p->overdrive = 1.f;

        p->aggressfactor = 1.25f;
        p->minsuppress = 0.04f;
        p->init_noise_lvl = -75.f;
        p->DNS_highGain = 2.0f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.5f;   /*混响强度，范围：0~0.9f,越大越强*/

        p->target_signal_degradation = 0.6f;
        p->enc_aggressfactor = 0.15f;
        p->enc_minsuppress = 0.09f;
        p->Disconverge_ERLE_Thr = -6.0f;

        p->output_sel = DMS_OUTPUT_SEL_DEFAULT;
        log_error("use dms_flexible param default\n");
    }
    log_info("DMS_Flexible:AEC[%d] NLP[%d] NS[%d] ENC[%d] AGC[%d]", !!(p->EnableBit & AEC_EN), !!(p->EnableBit & NLP_EN), !!(p->EnableBit & ANS_EN), !!(p->EnableBit & ENC_EN), !!(p->EnableBit & AGC_EN));

    /*DMS Flexible Parameters Reserved*/
    p->flexible_af_length		= 512;	//int AF_Length
    p->enc_process_maxfreq 		= 8000;	//int ENC_Process_MaxFrequency
    p->enc_process_minfreq 		= 0;	//int ENC_Process_MinFrequency
    p->sir_minfreq              = 100;	//int SIR_MinFrequency
    p->sir_maxfreq	            = 3000; //int SIR_MaxFrequency
    p->SIR_mean_MinFrequency    = 100;	//int SIR_mean_MinFrequency
    p->SIR_mean_MaxFrequency    = 1000;	//int SIR_mean_MaxFrequency
    p->ENC_CoheFlgMax_gamma     = 0.5f; //float ENC_CoheFlgMax_gamma
    p->coheXD_thr				= 0.5f;	//float coheXD_thr
    p->mic_distance				= 0.1f;	//float Engdiff_overdrive;
    p->global_minsuppress		= 0.08f;//float AdaptiveRate
    p->bone_init_noise_lvl		= 4.0f;	//float AggressFactor

    p->AGC_echo_hold = 0;
    p->AGC_echo_look_ahead = 0;
    //aec_param_dump(p);
}

#define AUDIO_DMS_HYBRID_COEFF_FILE 	(FLASH_RES_PATH"dms_hybrid.bin")
void *read_dms_hybrid_mic_coeff()
{
    if (aec_hdl == NULL) {
        return NULL;
    }
    RESFILE *fp = NULL;
    u32 param_len = 0;
    //===============================//
    //          打开参数文件         //
    //===============================//
    fp = resfile_open(AUDIO_DMS_HYBRID_COEFF_FILE);
    if (!fp) {
        printf("[err] open dms_hybrid.bin fail !!!");
        return NULL;
    }

    param_len = resfile_get_len(fp);
    printf("param_len %d", param_len);

    if (param_len) {
        aec_hdl->transfer_func = zalloc(param_len);
    }
    if (aec_hdl->transfer_func == NULL) {
        return NULL;
    }
    /* resfile_seek(fp, ptr, RESFILE_SEEK_SET); */
    int rlen = resfile_read(fp, aec_hdl->transfer_func, param_len);
    if (rlen != param_len) {
        printf("[error] read dms_hybrid.bin err !!! %d =! %d", rlen, param_len);
        return NULL;
    }
    resfile_close(fp);
    return aec_hdl->transfer_func;
}

static void audio_dms_hybrid_param_init(struct dms_attr *p)
{
    int ret = 0;
    DMS_HYBRID_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    ret = aec_cfg_online_update_fill(&cfg, sizeof(DMS_HYBRID_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/

    if (ret == sizeof(DMS_HYBRID_CONFIG)) {
        log_info("read dms_hybrid_param ok\n");
        p->EnableBit = cfg.enable_module;
        p->ul_eq_en = cfg.ul_eq_en;
        p->agc_type = cfg.agc_type;
        if (p->agc_type == AGC_EXTERNAL) {
            p->AGC_NDT_fade_in_step = cfg.agc.agc_ext.ndt_fade_in;
            p->AGC_NDT_fade_out_step = cfg.agc.agc_ext.ndt_fade_out;
            p->AGC_DT_fade_in_step = cfg.agc.agc_ext.dt_fade_in;
            p->AGC_DT_fade_out_step = cfg.agc.agc_ext.dt_fade_out;
            p->AGC_NDT_max_gain = cfg.agc.agc_ext.ndt_max_gain;
            p->AGC_NDT_min_gain = cfg.agc.agc_ext.ndt_min_gain;
            p->AGC_NDT_speech_thr = cfg.agc.agc_ext.ndt_speech_thr;
            p->AGC_DT_max_gain = cfg.agc.agc_ext.dt_max_gain;
            p->AGC_DT_min_gain = cfg.agc.agc_ext.dt_min_gain;
            p->AGC_DT_speech_thr = cfg.agc.agc_ext.dt_speech_thr;
            p->AGC_echo_present_thr = cfg.agc.agc_ext.echo_present_thr;
        } else {
            p->min_mag_db_level = cfg.agc.agc_int.min_mag_db_level;
            p->max_mag_db_level = cfg.agc.agc_int.max_mag_db_level;
            p->addition_mag_db_level = cfg.agc.agc_int.addition_mag_db_level;
            p->clip_mag_db_level = cfg.agc.agc_int.clip_mag_db_level;
            p->floor_mag_db_level = cfg.agc.agc_int.floor_mag_db_level;
        }

        p->aec_process_maxfrequency = cfg.aec_process_maxfrequency;
        p->aec_process_minfrequency = cfg.aec_process_minfrequency;
        p->af_length = cfg.af_length;

        p->nlp_process_maxfrequency = cfg.nlp_process_maxfrequency;
        p->nlp_process_minfrequency = cfg.nlp_process_minfrequency;
        p->overdrive = cfg.overdrive;

        p->dns_process_maxfrequency = cfg.dns_process_maxfrequency;
        p->dns_process_minfrequency = cfg.dns_process_minfrequency;
        p->aggressfactor = cfg.aggressfactor;
        p->minsuppress = cfg.minsuppress;
        p->init_noise_lvl = cfg.init_noise_lvl;

        p->FB_EnableBit = AEC_EN | NLP_EN;
        p->enc_process_maxfreq = cfg.enc_process_maxfreq;
        p->enc_process_minfreq = cfg.enc_process_minfreq;//sir设定阈值
        p->snr_db_T0 = cfg.snr_db_T0;//sir设定阈值
        p->snr_db_T1 = cfg.snr_db_T1;//sir设定阈值
        p->floor_noise_db_T = cfg.floor_noise_db_T;
        p->compen_db = cfg.compen_db;//mic补偿增益

        p->coh_val_T = cfg.coh_val_T;
        p->eng_db_T = cfg.eng_db_T;

        p->adc_ref_en = cfg.adc_ref_en;

        p->output_sel = cfg.output_sel;

    } else {
        p->EnableBit = WNC_EN | AEC_EN | ANS_EN | ENC_EN | AGC_EN;
        p->ul_eq_en = 1;
        p->agc_type = AGC_INTERNAL;
        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        p->min_mag_db_level = -50;       //语音能量放大下限阈值（单位db,默认-50，范围（-90db~-35db））
        p->max_mag_db_level = -3;       //语音能量放大上限阈值（单位db,默认-3，范围（-90db~0db））
        p->addition_mag_db_level = 0;   //语音补偿能量值（单位db,默认0，范围（0db~20db））
        p->clip_mag_db_level = -3;      //语音最大截断能量值（单位db,默认-3，范围（-10db~db））
        p->floor_mag_db_level = -70;    //语音最小截断能量值（单位db,默认-70，范围（-90db~-35db）

        p->aec_process_maxfrequency = 8000;
        p->aec_process_minfrequency = 0;
        p->af_length = 256;

        p->nlp_process_maxfrequency = 8000;
        p->nlp_process_minfrequency = 0;
        p->overdrive = 1.0f;

        p->dns_process_maxfrequency = 8000;
        p->dns_process_minfrequency = 0;
        p->aggressfactor = 1.0f;
        p->minsuppress = 0.1f;
        p->init_noise_lvl = -75.f;

        p->FB_EnableBit = AEC_EN | NLP_EN;
        p->enc_process_maxfreq = 8000;
        p->enc_process_minfreq = 0;//sir设定阈值
        p->snr_db_T0 = 0.0f;//sir设定阈值
        p->snr_db_T1 = 5.5f;//sir设定阈值
        p->floor_noise_db_T = 55.0f;
        p->compen_db = 12.f;//mic补偿增益

        p->coh_val_T = 0.85f;
        p->eng_db_T = 85.0f;

        p->adc_ref_en = 0;

        p->output_sel = DMS_OUTPUT_SEL_DEFAULT;
        log_error("read tms_param default\n");
    }
    log_info("DMS_HYBRID:WNC[%d] AEC[%d] NLP[%d] NS[%d] ENC[%d] AGC[%d] WNC[%d]", !!(p->EnableBit & WNC_EN), !!(p->EnableBit & AEC_EN), !!(p->EnableBit & NLP_EN), !!(p->EnableBit & ANS_EN), !!(p->EnableBit & ENC_EN), !!(p->EnableBit & AGC_EN), !!(p->EnableBit & WNC_EN));

    /* p->transfer_func = (float *)fb2talk_eq; */
    p->transfer_func = read_dms_hybrid_mic_coeff();
    if (p->transfer_func) {
        printf("dms_hybrid_coeff read ok %x", (int)p->transfer_func);
    }

    p->AGC_echo_hold = 0;
    p->AGC_echo_look_ahead = 0;

    //aec_param_dump(p);
}


static void audio_dms_awn_param_init(struct dms_attr *p)
{
    int ret = 0;
    DMS_AWN_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    ret = aec_cfg_online_update_fill(&cfg, sizeof(DMS_AWN_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/

    if (ret == sizeof(DMS_AWN_CONFIG)) {
        log_info("read dms_awn_param ok\n");
        p->EnableBit = cfg.enable_module;
        p->ul_eq_en = cfg.ul_eq_en;
        p->agc_type = cfg.agc_type;
        if (p->agc_type == AGC_EXTERNAL) {
            p->AGC_NDT_fade_in_step = cfg.agc.agc_ext.ndt_fade_in;
            p->AGC_NDT_fade_out_step = cfg.agc.agc_ext.ndt_fade_out;
            p->AGC_DT_fade_in_step = cfg.agc.agc_ext.dt_fade_in;
            p->AGC_DT_fade_out_step = cfg.agc.agc_ext.dt_fade_out;
            p->AGC_NDT_max_gain = cfg.agc.agc_ext.ndt_max_gain;
            p->AGC_NDT_min_gain = cfg.agc.agc_ext.ndt_min_gain;
            p->AGC_NDT_speech_thr = cfg.agc.agc_ext.ndt_speech_thr;
            p->AGC_DT_max_gain = cfg.agc.agc_ext.dt_max_gain;
            p->AGC_DT_min_gain = cfg.agc.agc_ext.dt_min_gain;
            p->AGC_DT_speech_thr = cfg.agc.agc_ext.dt_speech_thr;
            p->AGC_echo_present_thr = cfg.agc.agc_ext.echo_present_thr;
        } else {
            p->min_mag_db_level = cfg.agc.agc_int.min_mag_db_level;
            p->max_mag_db_level = cfg.agc.agc_int.max_mag_db_level;
            p->addition_mag_db_level = cfg.agc.agc_int.addition_mag_db_level;
            p->clip_mag_db_level = cfg.agc.agc_int.clip_mag_db_level;
            p->floor_mag_db_level = cfg.agc.agc_int.floor_mag_db_level;
        }

        p->aec_process_maxfrequency = cfg.aec_process_maxfrequency;
        p->aec_process_minfrequency = cfg.aec_process_minfrequency;
        p->af_length = cfg.af_length;

        p->nlp_process_maxfrequency = cfg.nlp_process_maxfrequency;
        p->nlp_process_minfrequency = cfg.nlp_process_minfrequency;
        p->overdrive = cfg.overdrive;

        p->dns_process_maxfrequency = cfg.dns_process_maxfrequency;
        p->dns_process_minfrequency = cfg.dns_process_minfrequency;
        p->aggressfactor = cfg.aggressfactor;
        p->minsuppress = cfg.minsuppress;
        p->init_noise_lvl = cfg.init_noise_lvl;

        p->FB_EnableBit = AEC_EN | NLP_EN;

        p->coh_val_T = cfg.coh_val_T;
        p->eng_db_T = cfg.eng_db_T;

        p->adc_ref_en = cfg.adc_ref_en;

        p->output_sel = cfg.output_sel;

    } else {
        p->EnableBit = WNC_EN | AEC_EN | ANS_EN | ENC_EN | AGC_EN;
        p->ul_eq_en = 1;
        p->agc_type = AGC_INTERNAL;
        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        p->min_mag_db_level = -50;       //语音能量放大下限阈值（单位db,默认-50，范围（-90db~-35db））
        p->max_mag_db_level = -3;       //语音能量放大上限阈值（单位db,默认-3，范围（-90db~0db））
        p->addition_mag_db_level = 0;   //语音补偿能量值（单位db,默认0，范围（0db~20db））
        p->clip_mag_db_level = -3;      //语音最大截断能量值（单位db,默认-3，范围（-10db~db））
        p->floor_mag_db_level = -70;    //语音最小截断能量值（单位db,默认-70，范围（-90db~-35db）

        p->aec_process_maxfrequency = 8000;
        p->aec_process_minfrequency = 0;
        p->af_length = 256;

        p->nlp_process_maxfrequency = 8000;
        p->nlp_process_minfrequency = 0;
        p->overdrive = 1.0f;

        p->dns_process_maxfrequency = 8000;
        p->dns_process_minfrequency = 0;
        p->aggressfactor = 1.0f;
        p->minsuppress = 0.1f;
        p->init_noise_lvl = -75.f;

        p->FB_EnableBit = AEC_EN | NLP_EN;
        p->coh_val_T = 0.85f;
        p->eng_db_T = 85.0f;

        p->adc_ref_en = 0;

        p->output_sel = DMS_OUTPUT_SEL_DEFAULT;
        log_error("read tms_param default\n");
    }
    log_info("DMS_AWN:WNC[%d] AEC[%d] NLP[%d] NS[%d] ENC[%d] AGC[%d] WNC[%d]", !!(p->EnableBit & WNC_EN), !!(p->EnableBit & AEC_EN), !!(p->EnableBit & NLP_EN), !!(p->EnableBit & ANS_EN), !!(p->EnableBit & ENC_EN), !!(p->EnableBit & AGC_EN), !!(p->EnableBit & WNC_EN));

    p->AGC_echo_hold = 0;
    p->AGC_echo_look_ahead = 0;

    //aec_param_dump(p);
}

/*
 * 开mic异常检测，设置默认使用的mic
 * 返回参数 0 : 使用双麦
 * 返回参数 -1 : 使用副麦
 * 返回参数 1 : 使用主麦
 */
int audio_dms_get_malfunc_state(void)
{
    int malfunc_state = 0;
    int ret = syscfg_read(CFG_DMS_MALFUNC_STATE_ID, &malfunc_state, sizeof(int));
    if (ret != sizeof(int)) {
        return 0;
    }
    printf("%s : %d", __func__, malfunc_state);
    return malfunc_state;
}

static int audio_cvp_advanced_options(void *aec, void *nlp, void *ns, void *enc, void *agc, void *wn, void *mfdt)
{
    printf("%s:%d", __func__, __LINE__);

#if (TCFG_AUDIO_DMS_SEL == DMS_NORMAL)
    /*tws双麦*/
    if (CONST_DMS_GLOBAL_VERSION == DMS_GLOBAL_V200) {
        JLSP_dms_wind_cfg *wn_cfg = (JLSP_dms_wind_cfg *)wn;
        wn_cfg->wn_msc_th = 0.6f;
        wn_cfg->ms_th = 80.0f;//110.0f,
        wn_cfg->wn_inty1 = 100;
        wn_cfg->wn_inty2 = 30;
        wn_cfg->wn_gain1 = 1.0f;
        wn_cfg->wn_gain2 = 1.1f;

        wn_cfg->t1_bot = 0;
        wn_cfg->t1_top = 0;
        wn_cfg->t2_bot = 0;
        wn_cfg->t2_top = 0;
        wn_cfg->offset = 0;

        wn_cfg->t1_bot_cnt_limit = 3;
        wn_cfg->t1_top_cnt_limit = 10;
        wn_cfg->t2_bot_cnt_limit = 3;
        wn_cfg->t2_top_cnt_limit = 10;
    } else {

    }
#else
    /*话务耳机*/
#endif
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : sr 			采样率(8000/16000)
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(struct audio_aec_init_param_t *init_param, s16 enablebit, int (*out_hdl)(s16 *data, u16 len))
{
    s16 sample_rate = init_param->sample_rate;
    u32 ref_sr = init_param->ref_sr;
    u8 ref_channel = init_param->ref_channel;
    struct dms_attr *aec_param;
    printf("audio_dms_open\n");
    mem_stats();
#if AEC_USER_MALLOC_ENABLE
    aec_hdl = zalloc(sizeof(struct audio_aec_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }
#endif/*AEC_USER_MALLOC_ENABLE*/

#if TCFG_AUDIO_CVP_SYNC
    audio_cvp_sync_open(sample_rate);
#endif/*TCFG_AUDIO_CVP_SYNC*/

    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();

    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;
    aec_param->cvp_advanced_options = audio_cvp_advanced_options;
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
    aec_param->output_handle = audio_aec_output;

    if (ref_sr) {
        aec_param->ref_sr  = ref_sr;
    } else {
        aec_param->ref_sr  = usb_mic_is_running();
    }
    if (aec_param->ref_sr == 0) {
        if (TCFG_ESCO_DL_CVSD_SR_USE_16K && (sample_rate == 8000)) {
            aec_param->ref_sr = 16000;	//CVSD 下行为16K
        } else {
            aec_param->ref_sr = sample_rate;
        }
    }

    if (ref_channel != 2) {
        ref_channel = 1;
    }
    aec_param->ref_channel = ref_channel;

#if (TCFG_AUDIO_DMS_SEL == DMS_NORMAL)
    audio_aec_param_init(aec_param);
#elif (TCFG_AUDIO_DMS_SEL == DMS_FLEXIBLE)
    audio_dms_flexible_param_init(aec_param);
#elif (TCFG_AUDIO_DMS_SEL == DMS_HYBRID)
    audio_dms_hybrid_param_init(aec_param);
#elif (TCFG_AUDIO_DMS_SEL == DMS_AWN)
    audio_dms_awn_param_init(aec_param);
#endif/*TCFG_AUDIO_DMS_SEL*/

    if (enablebit >= 0) {
        aec_param->EnableBit = enablebit;
    }
    if (out_hdl) {
        aec_param->output_handle = out_hdl;
    }

    if (aec_param->adc_ref_en == 0) {
        /*内部读取DAC数据做参考数据才需要做24bit转16bit*/
        extern struct dac_platform_data dac_data;
        aec_param->ref_bit_width = dac_data.bit_width;
    } else {
        aec_param->ref_bit_width = DATA_BIT_WIDE_16BIT;
    }

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
    aec_param->EnableBit = AEC_MODE_SIMPLEX;
    if (sr == 8000) {
        aec_param->SimplexTail = aec_param->SimplexTail / 2;
    }
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    if (sample_rate == 16000) { //WideBand宽带
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 60;
    } else {//NarrowBand窄带
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 55;
    }


#if TCFG_SUPPORT_MIC_CAPLESS
    if (audio_adc_file_get_mic_mode(0) == AUDIO_MIC_CAPLESS_MODE) {
        aec_hdl->dcc_hdl = audio_dc_offset_remove_open(sample_rate, 1);
    }
#endif

    //aec_param_dump(aec_param);
    aec_hdl->EnableBit = aec_param->EnableBit;
    aec_param->aptfilt_only = 0;
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    extern u8 kws_get_state(void);
    if (kws_get_state()) {
        aec_param->EnableBit = AEC_EN;
        aec_param->aptfilt_only = 1;
        printf("kws open,aec_enablebit=%x", aec_param->EnableBit);
        //临时关闭aec, 对比测试
        //aec_param->toggle = 0;
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

    if (CONST_DMS_WNC || CONST_JLSP_WD_EN) {
        aec_param->EnableBit |= WNC_EN;
    }

    if (aec_param->EnableBit & MFDT_EN) {
        aec_param->EnableBit |= ENC_EN;
    }

    y_printf("[aec_user]aec_open\n");
#if CVP_TOGGLE
    int ret = aec_open(aec_param);
    ASSERT(ret == 0, "aec_open err %d!!", ret);
#endif
    aec_hdl->start = 1;
    mem_stats();
    printf("audio_dms_open succ\n");
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Init
* Description: 初始化AEC模块
* Arguments  : sample_rate 采样率(8000/16000)
*              ref_sr 参考采样率
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_init(struct audio_aec_init_param_t *init_param)
{
    return audio_aec_open(init_param, -1, NULL);
}

/*
*********************************************************************
*                  Audio AEC Reboot
* Description: AEC模块复位接口
* Arguments  : reduce 复位/恢复标志
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_reboot(u8 reduce)
{
    if (aec_hdl) {
        printf("audio_aec_dms_reboot:%x,%x,start:%d", aec_hdl->EnableBit, aec_hdl->attr.EnableBit, aec_hdl->start);
        if (aec_hdl->start) {
            if (reduce) {
                aec_hdl->attr.EnableBit = AEC_EN;
                aec_hdl->attr.aptfilt_only = 1;
                aec_dms_reboot(aec_hdl->attr.EnableBit);
            } else {
                if (aec_hdl->EnableBit != aec_hdl->attr.EnableBit) {
                    aec_hdl->attr.aptfilt_only = 0;
                    aec_dms_reboot(aec_hdl->EnableBit);
                }
            }
        }
    } else {
        printf("audio_aec close now\n");
    }
}

/*
*********************************************************************
*                  Audio AEC Output Select
* Description: 输出选择
* Arguments  : sel = DMS_OUTPUT_SEL_DEFAULT 默认输出算法处理结果
*				   = DMS_OUTPUT_SEL_MASTER	输出主mic(通话mic)的原始数据
*				   = DMS_OUTPUT_SEL_SLAVE	输出副mic(降噪mic)的原始数据
*			   agc 输出数据要不要经过agc自动增益控制模块
* Return	 : None.
* Note(s)    : 可以通过选择不同的输出，来测试mic的频响和ENC指标
*********************************************************************
*/
void audio_aec_output_sel(CVP_OUTPUT_ENUM sel, u8 agc)
{
    if (aec_hdl)	{
        printf("dms_output_sel:%d\n", sel);
        if (agc) {
            aec_hdl->attr.EnableBit |= AGC_EN;
        } else {
            aec_hdl->attr.EnableBit &= ~AGC_EN;
        }
        aec_hdl->attr.output_sel = sel;
    }
}

/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    if (aec_hdl) {
        aec_hdl->start = 0;

#if CVP_TOGGLE
        if (CONST_DMS_MALFUNCTION_DETECT && CONST_DMS_SET_MFDT_STATE) {
            int malfunc_state = cvp_dms_get_malfunc_state();
            int ret = syscfg_write(CFG_DMS_MALFUNC_STATE_ID, &malfunc_state, sizeof(int));
            if (ret != sizeof(int)) {
                printf("vm read  err !!!");
            }
            printf("cvp_dms_get_malfunc_state:%d", malfunc_state);
        }

        aec_close();
#endif

#if TCFG_AUDIO_CVP_SYNC
        //在AEC关闭之后再关，否则还会跑cvp_sync_run,导致越界
        audio_cvp_sync_close();
#endif/*TCFG_AUDIO_CVP_SYNC*/

#if TCFG_SUPPORT_MIC_CAPLESS
        if (aec_hdl->dcc_hdl) {
            audio_dc_offset_remove_close(aec_hdl->dcc_hdl);
            aec_hdl->dcc_hdl = NULL;
        }
#endif

        if (aec_hdl->transfer_func) {
            free(aec_hdl->transfer_func);
            aec_hdl->transfer_func = NULL;
        }

        local_irq_disable();
#if AEC_USER_MALLOC_ENABLE
        free(aec_hdl);
#endif/*AEC_USER_MALLOC_ENABLE*/
        aec_hdl = NULL;
        local_irq_enable();

        aec_code_movable_unload();
    }
}

/*
*********************************************************************
*                  Audio AEC Status
* Description: AEC模块当前状态
* Arguments  : None.
* Return	 : 0 关闭 其他 打开
* Note(s)    : None.
*********************************************************************
*/
u8 audio_aec_status(void)
{
    if (aec_hdl) {
        return aec_hdl->start;
    }
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        if (aec_hdl->input_clear) {
            memset(buf, 0, len);
        }
#if CVP_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
        int ret = aec_in_data(buf, len);
        if (ret == -1) {
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
        }
#else
        aec_hdl->attr.output_handle(buf, len);
#endif/*CVP_TOGGLE*/
    }
}

/*
*********************************************************************
*                  Audio AEC Input Reference
* Description: AEC源参考数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 双mic ENC的参考mic数据输入,单mic的无须调用该接口
*********************************************************************
*/
void audio_aec_inbuf_ref(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        aec_in_data_ref(buf, len);
    }
}

/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void audio_aec_refbuf(s16 *data0, s16 *data1, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if CVP_TOGGLE
        aec_ref_data(data0, data1, len);
#endif/*CVP_TOGGLE*/
    }
}

/*
*********************************************************************
*                  			Audio CVP IOCTL
* Description: CVP功能配置
* Arguments  : cmd 		操作命令
*		       value 	操作数
*		       priv 	操作内存地址
* Return	 : 0 成功 其他 失败
* Note(s)    : (1)比如动态开关降噪NS模块:
*				audio_cvp_ioctl(CVP_NS_SWITCH,1,NULL);	//降噪关
*				audio_cvp_ioctl(CVP_NS_SWITCH,0,NULL);  //降噪开
*********************************************************************
*/
int audio_cvp_ioctl(int cmd, int value, void *priv)
{
    if (!aec_hdl) {
        return -1;
    }
    return aec_dms_ioctl(cmd, value, priv);
}

/*
*********************************************************************
*                  Audio CVP Toggle Set
* Description: CVP模块算法开关使能
* Arguments  : toggle 0 关闭算法 1 打开算法
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int audio_cvp_toggle_set(u8 toggle)
{
    if (aec_hdl) {
        aec_dms_toggle(toggle);
        return 0;
    }
    return 1;
}

/*获取风噪的检测结果，1：有风噪，0：无风噪*/
int audio_cvp_dms_wnc_state(void)
{
    int state = 0;
    if (aec_hdl) {
        state = cvp_dms_get_wind_detect_state();
        printf("wnc state : %d", state);
    } else {
        state = -1;
    }
    return state;
}

/*获取单双麦切换状态
 * 0: 正常双麦 ;
 * 1: 副麦坏了，触发故障
 * -1: 主麦坏了，触发故障
 */
int audio_cvp_dms_malfunc_state()
{
    int state = 0;
    if (aec_hdl) {
        state = cvp_dms_get_malfunc_state();
        printf("malfunc state : %d", state);
    } else {
        state = -2;
        printf("cvp malfunc disable !!!");
    }
    return state;
}

/*
 * 获取mic的能量值，开了MFDT_EN才能用
 * mic: 0 获取主麦能量
 * mic：1 获取副麦能量
 * return：返回能量值，[0~90.3],返回-1表示错误
 */
float audio_cvp_dms_mic_energy(u8 mic)
{
    float mic_db = 0;
    if (aec_hdl) {
        mic_db = cvp_dms_get_mic_energy(mic);
        printf("malfunc mic[%d] energy : %d", mic, (int)mic_db);
    } else {
        mic_db = -1;
        printf("cvp malfunc disable !!!");
    }
    return mic_db;
}

/**
 * 以下为用户层扩展接口
 */
//pbg profile use it,don't delete
void aec_input_clear_enable(u8 enable)
{
    if (aec_hdl) {
        aec_hdl->input_clear = enable;
        log_info("aec_input_clear_enable= %d\n", enable);
    }
}



#endif/*TCFG_AUDIO_DUAL_MIC_ENABLE == 1*/
#endif /*TCFG_CVP_DEVELOP_ENABLE*/

