#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_cvp.data.bss")
#pragma data_seg(".audio_cvp.data")
#pragma const_seg(".audio_cvp.text.const")
#pragma code_seg(".audio_cvp.text")
#endif
/*
 ****************************************************************
 *							AUDIO SMS(SingleMic System)
 * File  : audio_aec_sms.c
 * By    :
 * Notes : AEC回音消除 + 单mic降噪
 *
 ****************************************************************
 */
#include "audio_cvp.h"
#include "system/includes.h"
#include "media/includes.h"
#include "effects/eq_config.h"
#include "effects/audio_pitch.h"
#include "circular_buf.h"
#include "overlay_code.h"
#include "audio_cvp_online.h"
#include "audio_config.h"
#include "cvp_node.h"
#include "sdk_config.h"
#include "audio_dc_offset_remove.h"
#include "adc_file.h"
#include "audio_cvp_def.h"
#include "effects/audio_gain_process.h"

#if TCFG_AUDIO_CVP_SYNC
#include "audio_cvp_sync.h"
#endif/*TCFG_AUDIO_CVP_SYNC*/

#if !defined(TCFG_CVP_DEVELOP_ENABLE) || (TCFG_CVP_DEVELOP_ENABLE == 0)

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/* TCFG_USER_TWS_ENABLE */

#if TCFG_SMART_VOICE_ENABLE
#include "smart_voice/smart_voice.h"
#endif

#if (TCFG_AUDIO_DUAL_MIC_ENABLE == 0) && (TCFG_AUDIO_TRIPLE_MIC_ENABLE == 0)

#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#include "audio_cvp_debug.c"

#define AEC_USER_MALLOC_ENABLE	1

/*CVP_TOGGLE:CVP模块(包括AEC、NLP、NS等)总开关，Disable则数据完全不经过处理，释放资源*/
#define CVP_TOGGLE				TCFG_AEC_ENABLE	/*CVP模块总开关*/


/*使用输入立体声参考数据的TDE回音消除算法*/
const u8 CONST_SMS_TDE_STEREO_REF_ENABLE = 0;


/*使能即可跟踪通话过程的内存情况*/
#define CVP_MEM_TRACE_ENABLE	0

/*
 *延时估计使能
 *点烟器需要做延时估计
 *其他的暂时不需要做
 */
const u8 CONST_AEC_DLY_EST = 0;

/*
 *ANS等级:0~2,
 *等级1比等级0多6k左右的ram
 *等级2比等级1多3k左右的ram:优化了连续说话变小声问题
 */
const u8 CONST_ANS_MODE = 2;

/*
 *ANS版本配置
 *ANS_V100:传统降噪
 *ANS_V200:AI降噪，需要更多的ram和mips
 **/
#if (TCFG_AUDIO_CVP_NS_MODE == CVP_ANS_MODE)
const u8 CONST_ANS_VERSION = ANS_V100;
#else
const u8 CONST_ANS_VERSION = ANS_V200;
#endif/*TCFG_AUDIO_CVP_NS_MODE*/

//*********************************************************************************//
//                                DNS配置                                          //
//*********************************************************************************//
/* SNR估计,可以实现场景适配 */
const u8 CONST_DNS_SNR_EST = 0;

/* DNS后处理 */
const u8 CONST_DNS_POST_ENHANCE = 0;

/*
 *SMS DNS版本配置
 *SMS_DNS_V100:第一版dns降噪算法，拆分aec nlp dns模块
 *SMS_DNS_V200:第二版dns回音消除算法，集成aec nlp dns模块
 */
#if (TCFG_AUDIO_CVP_NS_MODE == CVP_DNS_MODE) && (defined TCFG_AUDIO_SMS_DNS_VERSION) && (TCFG_AUDIO_SMS_DNS_VERSION == SMS_DNS_V200)
const u8 CONST_SMS_DNS_VERSION = SMS_DNS_V200;
#else
const u8 CONST_SMS_DNS_VERSION = SMS_DNS_V100;
#endif

/*
 * 非线性压制模式选择
 * JLSP_NLP_MODE1: 模式1为单独的NLP回声抑制，回声压制会偏过，该模式下NLP模块可以单独开启
 * JLSP_NLP_MODE2: 模式2下回声信号会先经过AEC线性压制，然后进行NLP非线性压制
 *                 此模式NLP不能单独打开需要同时打开AEC,使用AEC模块压制不够时，建议开启该模式
 */
const u8 CONST_JLSP_NLP_MODE = JLSP_NLP_MODE1;

//*******************************DNS配置 end**************************************//

/*Splittingfilter模式：0 or 1
 *mode = 0:运算量和ram小，高频会跌下来
 *mode = 1:运算量和ram大，频响正常（过认证，选择模式1）
 */
const u8 CONST_SPLIT_FILTER_MODE = 0;

/*
 *AEC复杂等级，等级越高，ram和mips越大，适应性越好
 *回音路径不定/回音失真等情况才需要比较高的等级
 *音箱建议使用等级:5
 *耳机建议使用等级:2
 */
#define AEC_TAIL_LENGTH			5 /*range:2~10,default:2*/

/*单工连续清0的帧数*/
#define AEC_SIMPLEX_TAIL 	15
/**远端数据大于CONST_AEC_SIMPLEX_THR,即清零近端数据
 *越小，回音限制得越好，同时也就越容易卡*/
#define AEC_SIMPLEX_THR		100000	/*default:260000*/

/*数据输出开头丢掉的数据包数*/
#define AEC_OUT_DUMP_PACKET		15
/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);
extern int db2mag(int db, int dbQ, int magDQ);//10^db/20

extern int esco_player_runing();
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
    struct aec_s_attr attr;
    struct audio_cvp_pre_param_t pre;	//预处理配置
};
#if AEC_USER_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
struct audio_aec_hdl *aec_hdl = &aec_handle;
#endif/*AEC_USER_MALLOC_ENABLE*/
static u8 global_output_way = 0;

void audio_cvp_set_output_way(u8 en)
{
    global_output_way = en;
}

void audio_cvp_ref_start(u8 en)
{
    if (aec_hdl && (aec_hdl->attr.fm_tx_start == 0)) {
        aec_hdl->attr.fm_tx_start = en;
        y_printf("fm_tx_start:%d\n", en);
    }
}

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
    if (aec_hdl->pre.pre_gain_en) {
        GainProcess_16Bit(talk_mic, talk_mic, aec_hdl->pre.talk_mic_gain, 1, 1, 1, len >> 1);
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

static int audio_aec_update(u8 EnableBit)
{
    printf("aec_update,wideband:%d,EnableBit:%x", aec_hdl->attr.wideband, EnableBit);
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

/*通话上行同步输出回调*/
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
    if (esco_player_runing()) {
        extern void kws_aec_data_output(void *priv, s16 * data, int len);
        kws_aec_data_output(NULL, data, len);
    }
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
static void audio_aec_param_init(struct aec_s_attr *p)
{
    int ret = 0;
    AEC_CONFIG cfg;
    //读取工具配置参数+预处理参数
    ret = cvp_node_param_cfg_read(&cfg, 0);
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    //APP在线调试，APP参数覆盖工具配置参数(不覆盖预处理参数)
    ret = aec_cfg_online_update_fill(&cfg, sizeof(AEC_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    log_info("CVP_NS_MODE = %d\n", TCFG_AUDIO_CVP_NS_MODE);
    if (ret == sizeof(AEC_CONFIG)) {
        log_info("audio_aec read config ok\n");
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
        p->AEC_DT_AggressiveFactor = cfg.aec_dt_aggress;
        p->AEC_RefEngThr = cfg.aec_refengthr;
        p->ES_AggressFactor = cfg.es_aggress_factor;
        p->ES_MinSuppress = cfg.es_min_suppress;
        p->ES_Unconverge_OverDrive = cfg.es_min_suppress;
        p->ANS_AggressFactor = cfg.ans_aggress;
        p->ANS_MinSuppress = cfg.ans_suppress;
        p->DNS_OverDrive = cfg.ans_aggress;
        p->DNS_GainFloor = cfg.ans_suppress;
        p->DNS_highGain = 2.5f;
        p->DNS_rbRate = 0.3f;

        p->ANS_NoiseLevel = db2mag((int)(cfg.init_noise_lvl * (1 << 8)), 8, 23);//初始噪声水平

        p->adc_ref_en = cfg.adc_ref_en;

        p->toggle = (cfg.aec_mode) ? 1 : 0;
        p->EnableBit = (cfg.aec_mode & AEC_MODE_ADVANCE);
        p->agc_en = (cfg.aec_mode & AGC_EN) ? 1 : 0;
        p->ul_eq_en = 0;
        g_printf("p->EnableBit %x\n", p->EnableBit);
    } else {
        log_error("read audio_aec param err:%x", ret);
        p->toggle = 1;
        p->EnableBit = AEC_MODE_REDUCE;
        p->wideband = 0;
        p->ul_eq_en = 1;

        p->agc_en = 1;
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

        /*AEC*/
        p->AEC_DT_AggressiveFactor = 1.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
        p->AEC_RefEngThr = -70.f;

        /*ES*/
        p->ES_AggressFactor = -3.0f;
        p->ES_MinSuppress = 4.f;
        p->ES_Unconverge_OverDrive = p->ES_MinSuppress;

        /*ANS*/
        p->ANS_AggressFactor = 1.25f;	/*范围：1~2,动态调整,越大越强(1.25f)*/
        p->ANS_MinSuppress = 0.04f;	/*范围：0~1,静态定死最小调整,越小越强(0.09f)*/

        /*DNS*/
        p->DNS_GainFloor = 0.1f; /*增益最小值控制，范围：0~1.0，越小降噪越强，默认0.1，建议值:0~0.2*/
        p->DNS_OverDrive = 1.0f;   /*控制降噪强度，范围：0~3.0，越大降噪越强，正常降噪时为1.0，建议调节范围:0.3~3*/
        p->DNS_highGain = 2.5f; /*EQ强度, 范围：1.0f~3.5f,越大越强*/
        p->DNS_rbRate = 0.3f;   /*混响强度，范围：0~0.9f,越大越强*/

        p->ANS_NoiseLevel = db2mag((int)(75.0f * (1 << 8)), 8, 23);//初始噪声水平
    }
    p->ANS_mode = 1;
    p->wn_gain = 331;
    p->SimplexTail = AEC_SIMPLEX_TAIL;
    p->SimplexThr = AEC_SIMPLEX_THR;
    p->dly_est = 0;
    p->dst_delay = 50;
    p->AGC_echo_look_ahead = 0;
    p->AGC_echo_hold = 0;
    p->packet_dump = 50;/*0~255(u8)*/
    p->aec_tail_length = AEC_TAIL_LENGTH;
    p->ES_OverSuppressThr = 0.02f;
    p->ES_OverSuppress = 2.f;

    if (CONST_SMS_DNS_VERSION == SMS_DNS_V200) {
        p->AEC_Process_MaxFrequency = 8000;
        p->AEC_Process_MinFrequency = 0;
        p->NLP_Process_MaxFrequency = 8000;
        p->NLP_Process_MinFrequency = 0;
    }
    /* aec_param_dump(p); */
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : init_param 	sr 			采样率(8000/16000)
*              				ref_sr  	参考采样率
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
    struct aec_s_attr *aec_param;
    printf("audio_aec_open\n");
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
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    aec_param->aec_update = audio_aec_update;
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    aec_param->output_handle = audio_aec_output;
    aec_param->far_noise_gate = 10;
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

    audio_aec_param_init(aec_param);
    if (enablebit >= 0) {
        aec_param->EnableBit = enablebit;
    }
    if (out_hdl) {
        aec_param->output_handle = out_hdl;
    }

    aec_param->output_way = global_output_way;

    if (aec_param->output_way == 0 && aec_param->adc_ref_en == 0) {
        /*内部读取DAC数据做参考数据才需要做24bit转16bit*/
        extern struct dac_platform_data dac_data;
        aec_param->ref_bit_width = dac_data.bit_width;
    } else {
        aec_param->ref_bit_width = DATA_BIT_WIDE_16BIT;
    }

#if (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC)
    printf("esco_player_runing() : %d", esco_player_runing());
    if (!esco_player_runing()) {
        /* aec_param->ref_sr = TCFG_AUDIO_GLOBAL_SAMPLE_RATE; */

        aec_param->output_way = 1;
        aec_param->fm_tx_start = 1;
        /*在语音识别做24bit转16bit*/
        aec_param->ref_bit_width = DATA_BIT_WIDE_16BIT;

        /*AGC*/
        aec_param->agc_en = 0;

        /*AEC*/
        aec_param->EnableBit = AEC_EN;
        aec_param->AEC_DT_AggressiveFactor = 4.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
        aec_param->AEC_RefEngThr = -70.f;
    }
#endif // TCFG_SMART_VOICE_USE_AEC

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
    aec_param.EnableBit = AEC_MODE_SIMPLEX;
    if (sr == 8000) {
        aec_param.SimplexTail = aec_param.SimplexTail / 2;
    }
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    /*根据清晰语音处理模块配置，配置相应的系统时钟*/
    if (sample_rate == 16000) {
        /*宽带wide-band*/
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 60;
    } else {
        /*窄带narrow-band*/
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 55;
    }

    /* clk_set("sys", 0); */

#if TCFG_SUPPORT_MIC_CAPLESS
    if (audio_adc_file_get_mic_mode(0) == AUDIO_MIC_CAPLESS_MODE) {
        aec_hdl->dcc_hdl = audio_dc_offset_remove_open(sample_rate, 1);
    }
#endif

    //aec_param_dump(aec_param);
    aec_hdl->EnableBit = aec_param->EnableBit;
#if (((defined TCFG_KWS_VOICE_RECOGNITION_ENABLE) && TCFG_KWS_VOICE_RECOGNITION_ENABLE) || \
     ((defined TCFG_CALL_KWS_SWITCH_ENABLE) && TCFG_CALL_KWS_SWITCH_ENABLE))
    extern u8 kws_get_state(void);
    if (esco_player_runing()) {
        if (kws_get_state()) {
            aec_param->EnableBit = AEC_EN;
            aec_param->agc_en = 0;
            printf("kws open,aec_enablebit=%x", aec_param->EnableBit);
            //临时关闭aec, 对比测试
            //aec_param->toggle = 0;
        }
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if CVP_TOGGLE
    if (aec_param->output_way && (aec_param->adc_ref_en || aec_param->ref_bit_width)) {
        ASSERT(0, "output_way and adc_ref_en can not open at same time, or output_way nonsupport 24bit Width");
    }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    if (CONST_SMS_TDE_STEREO_REF_ENABLE && aec_param->ref_channel != 2) {
        ASSERT(0, "ref_channel need set 2 ch when CONST_SMS_TDE_STEREO_REF_ENABLE is 1");
    }
#endif
    int ret = aec_open(aec_param);
    ASSERT(ret == 0, "aec_open err %d!!", ret);
#endif/*CVP_TOGGLE*/
    aec_hdl->start = 1;
    mem_stats();
    printf("audio_aec_open succ\n");
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
        printf("audio_aec_reboot:%x,%x,start:%d", aec_hdl->EnableBit, aec_hdl->attr.EnableBit, aec_hdl->start);
        if (aec_hdl->start) {
            if (reduce) {
                aec_hdl->attr.EnableBit = AEC_EN;
                aec_hdl->attr.agc_en = 0;
            } else {
                if (aec_hdl->EnableBit != aec_hdl->attr.EnableBit) {
                    aec_hdl->attr.EnableBit = aec_hdl->EnableBit;
                }
                aec_hdl->attr.agc_en = 1;
            }
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
            sms_tde_reboot(aec_hdl->attr.EnableBit);
#else
            aec_reboot(aec_hdl->attr.EnableBit);
#endif
        }
    } else {
        printf("audio_aec close now\n");
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
#if TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC
            audio_smart_voice_aec_cbuf_data_clear();
#endif
        }
#else	/*不经算法，直通到输出*/
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
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    return sms_tde_ioctl(cmd, value, priv);
#else
    return aec_ioctl(cmd, value, priv);
#endif
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
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        sms_tde_toggle(toggle);
#else
        aec_toggle(toggle);
#endif
    }
    return 0;
}

/*是否在重启*/
u8 get_audio_aec_rebooting()
{
    if (aec_hdl && aec_hdl->start) {
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        return get_sms_tde_rebooting();
#else
        return get_aec_rebooting();
#endif
    }
    return 0;
}

/*可写长度*/
int get_audio_cvp_output_way_writable_len()
{
    if (aec_hdl && aec_hdl->start) {
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
        return get_cvp_sms_tde_output_way_writable_len();
#else
        return get_cvp_sms_output_way_writable_len();
#endif
    }
    return 0;
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

/* void aec_estimate_dump(int DlyEst) */
/* { */
/*     printf("DlyEst:%d\n", DlyEst); */
/* } */

#endif/*TCFG_AUDIO_DUAL_MIC_ENABLE == 0) && TCFG_AUDIO_TRIPLE_MIC_ENABLE == 0*/
#endif /*TCFG_CVP_DEVELOP_ENABLE*/

int audio_cvp_phase_align(void)
{
    if (audio_aec_status() == 0) {
        return 0;
    }
#if defined(TCFG_CVP_DEVELOP_ENABLE) && (TCFG_CVP_DEVELOP_ENABLE)
    return cvp_develop_read_ref_data();
#endif

#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
    /*3MIC*/
    return cvp_tms_read_ref_data();
#elif TCFG_AUDIO_DUAL_MIC_ENABLE
#if (TCFG_AUDIO_DMS_SEL == DMS_NORMAL)
    /*TWS双麦*/
    return cvp_dms_read_ref_data();
#elif (TCFG_AUDIO_DMS_SEL == DMS_FLEXIBLE)
    /*话务耳机双麦*/
    return cvp_dms_flexible_read_ref_data();
#elif (TCFG_AUDIO_DMS_SEL == DMS_HYBRID)
    /*双麦hybrid*/
    return cvp_dms_hybrid_read_ref_data();
#elif (TCFG_AUDIO_DMS_SEL == DMS_AWN)
    /*双麦awn*/
    return cvp_dms_awn_read_ref_data();
#endif
#else
    /*单麦*/
#if (TCFG_AUDIO_SMS_SEL == SMS_TDE)
    /*TDE*/
    return cvp_sms_tde_read_ref_data();
#else
    return cvp_sms_read_ref_data();
#endif
#endif
}

u16 get_cvp_node_uuid()
{
    /* 设置源节点是哪个 */
    u16 node_uuid = 0;
#if TCFG_AUDIO_CVP_DEVELOP_ENABLE
    node_uuid = NODE_UUID_CVP_DEVELOP;
#elif TCFG_AUDIO_CVP_SMS_ANS_MODE
    node_uuid = NODE_UUID_CVP_SMS_ANS;
#elif TCFG_AUDIO_CVP_SMS_DNS_MODE
    node_uuid = NODE_UUID_CVP_SMS_DNS;
#elif TCFG_AUDIO_CVP_DMS_ANS_MODE
    node_uuid = NODE_UUID_CVP_DMS_ANS;
#elif TCFG_AUDIO_CVP_DMS_DNS_MODE
    node_uuid = NODE_UUID_CVP_DMS_DNS;
#elif TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE
    node_uuid = NODE_UUID_CVP_DMS_FLEXIBLE_ANS;
#elif TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE
    node_uuid = NODE_UUID_CVP_DMS_FLEXIBLE_DNS;
#elif TCFG_AUDIO_CVP_3MIC_MODE
    node_uuid = NODE_UUID_CVP_3MIC;
#elif TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE
    node_uuid = NODE_UUID_CVP_DMS_HYBRID_DNS;
#elif TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE
    node_uuid = NODE_UUID_CVP_DMS_AWN_DNS;
#endif
    return node_uuid;
}
