#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cvp_dms_node.data.bss")
#pragma data_seg(".cvp_dms_node.data")
#pragma const_seg(".cvp_dms_node.text.const")
#pragma code_seg(".cvp_dms_node.text")
#endif
#include "jlstream.h"
#include "media/audio_base.h"
#include "circular_buf.h"
#include "cvp_node.h"
#include "app_config.h"
#include "cvp_dms.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif

/*(双MIC+ANS通话) || (双MIC+DNS通话)*/
#if ((TCFG_AUDIO_CVP_DMS_ANS_MODE) || \
     (TCFG_AUDIO_CVP_DMS_DNS_MODE) || \
     (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || \
     (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE) || \
     (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE) || \
     (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE))

#define CVP_INPUT_SIZE		256*3	//CVP输入缓存，short

//------------------stream.bin CVP参数文件解析结构-START---------------//
struct CVP_MIC_SEL_CONFIG {
    u8 talk_mic;		//主MIC通道选择
    u8 talk_ref_mic;	//副MIC通道选择
} __attribute__((packed));

struct CVP_REF_MIC_CONFIG {
    u8 en;          //ref 回采硬使能
    u8 ref_mic_ch;	//ref 硬回采MIC通道选择
} __attribute__((packed));

struct CVP_PRE_GAIN_CONFIG {
    u8 en;
    float talk_mic_gain;	//主MIC前级数字增益，default:0dB(-90 ~ 40dB)
    float talk_ref_mic_gain;	//副MIC前级数字增益，default:0dB(-90 ~ 40dB)
} __attribute__((packed));

struct CVP_AEC_CONFIG {
    u8 en;
    int aec_process_maxfrequency;	//default:8000,range[3000:8000]
    int aec_process_minfrequency;	//default:0,range[0:1000]
    int af_length;					//default:128 range[128:256]
} __attribute__((packed));

struct CVP_NLP_CONFIG {
    u8 en;
    int nlp_process_maxfrequency;	//default:8000,range[3000:8000]
    int nlp_process_minfrequency;	//default:0,range[0:1000]
    float overdrive;				//default:1,range[0:30]
} __attribute__((packed));

struct CVP_ENC_CONFIG {
    u8 en;
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    int sir_maxfreq;				//default:3000,range[1000:8000]
    float mic_distance;				//default:0.015,range[0.035:0.015]
    float target_signal_degradation;//default:1,range[0:1]
    float enc_aggressfactor;		//default:4.f,range[0:4]
    float enc_minsuppress;			//default:0.09f,range[0:0.1]
} __attribute__((packed));

struct CVP_ENC_FLEXIBLE_CONFIG {
    u8 en;
    float enc_suppress_pre;		//ENC前级压制，越大越强 default:0.6f,range[0:1]
    float enc_suppress_post;	//ENC后级压制，越大越强 default:0.15f,range[0:1]
    float enc_minsuppress;		//ENC后级压制下限 default:0.09f,range[0:1]
    float enc_disconverge_erle_thr;	//滤波器发散控制阈值，越大控制越强 default:-6.f,range[-20:5]
} __attribute__((packed));

struct CVP_ENC_HYBRID_CONFIG {
    u8 en;
    int enc_process_maxfreq;		//default:8000,range[3000:8000]
    int enc_process_minfreq;		//default:0,range[0:1000]
    float snr_db_T0;  //sir设定阈值
    float snr_db_T1;  //sir设定阈值
    float floor_noise_db_T;
    float compen_db; //mic增益补偿, dB
} __attribute__((packed));

struct CVP_ANS_CONFIG {
    u8 en;
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75dB,range[-100:-30]
} __attribute__((packed));

struct CVP_DNS_CONFIG {
    u8 en;
    int dns_process_maxfrequency;	//default:8000,range[3000:8000]
    int dns_process_minfrequency;	//default:0,range[0:1000]
    float aggressfactor;			//default:1.25,range[1:2]
    float minsuppress;				//default:0.04,range[0.01:0.1]
    float init_noise_lvl;			//default:-75dB,range[-100:-30]
} __attribute__((packed));

struct CVP_GLOBAL_CONFIG {
    float global_minsuppress;		//default:0.4,range[0.0:0.09]
} __attribute__((packed));

struct CVP_AGC_CONFIG {
    u8 en;
    float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
    float dt_max_gain;   	//双端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -40 dB)
    float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -40 dB)
} __attribute__((packed));

struct CVP_AGC_INTERNAL_CONFIG {
    /*JLSP AGC*/
    u8 en;
    u8 agc_type;
    /*AGC_EXTERNAL*/
    float ndt_fade_in;  	//单端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float ndt_fade_out;  	//单端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float dt_fade_in;  		//双端讲话淡入步进default: 1.3f(0.1 ~ 5 dB)
    float dt_fade_out;  	//双端讲话淡出步进default: 0.7f(0.1 ~ 5 dB)
    float ndt_max_gain;   	//单端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float ndt_min_gain;   	//单端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float ndt_speech_thr;   //单端讲话放大阈值,default: -50.f(-70 ~ -40 dB)
    float dt_max_gain;   	//双端讲话放大上限,default: 12.f(0 ~ 24 dB)
    float dt_min_gain;   	//双端讲话放大下限,default: 0.f(-20 ~ 24 dB)
    float dt_speech_thr;    //双端讲话放大阈值,default: -40.f(-70 ~ -40 dB)
    float echo_present_thr; //单端双端讲话阈值,default:-70.f(-70 ~ -40 dB)
    /*AGC_INTERNAL*/
    int min_mag_db_level;
    int max_mag_db_level;
    int addition_mag_db_level;
    int clip_mag_db_level;
    int floor_mag_db_level;
} __attribute__((packed));

/*MFDT Parameters*/
struct CVP_MFDT_CONFIG {
    u8 en;
    float detect_time;           // // 检测时间s，影响状态切换的速度
    float detect_eng_diff_thr;   // 0~-90 dB 两个mic能量差异持续大于此阈值超过检测时间则会检测为故障
    float detect_eng_lowerbound; // 0~-90 dB 当处于故障状态时，正常的mic能量大于此阈值才会检测能量差异，避免安静环境下误判切回正常状态
    int MalfuncDet_MaxFrequency;// 检测信号的最大频率成分
    int MalfuncDet_MinFrequency;// 检测信号的最小频率成分
    int OnlyDetect;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换
}  __attribute__((packed));

struct CVP_WNC_CONFIG {
    u8 en;
    float coh_val_T;    //双麦非相关性阈值,default:0.6f(0 ~ 1)
    float eng_db_T;        //麦增益能量阈值, default:80.f(0-255)dB
}  __attribute__((packed));

struct CVP_DEBUG_CONFIG {
    u8 output_sel; //输出数据选择
}  __attribute__((packed));

struct cvp_dms_beamfroming_cfg_t {
    struct CVP_MIC_SEL_CONFIG mic_sel;
    struct CVP_REF_MIC_CONFIG ref_mic;
    struct CVP_PRE_GAIN_CONFIG pre_gain;
    struct CVP_AEC_CONFIG aec;
    struct CVP_NLP_CONFIG nlp;
    struct CVP_ENC_CONFIG enc;
    struct CVP_ANS_CONFIG ans;
    struct CVP_GLOBAL_CONFIG global;
    struct CVP_AGC_CONFIG agc;
    struct CVP_MFDT_CONFIG mfdt;
    struct CVP_DEBUG_CONFIG debug;
} __attribute__((packed));

struct cvp_dms_flexible_cfg_t {
    struct CVP_MIC_SEL_CONFIG mic_sel;
    struct CVP_REF_MIC_CONFIG ref_mic;
    struct CVP_PRE_GAIN_CONFIG pre_gain;
    struct CVP_AEC_CONFIG aec;
    struct CVP_NLP_CONFIG nlp;
    struct CVP_ENC_FLEXIBLE_CONFIG enc;
    struct CVP_ANS_CONFIG ans;
    struct CVP_AGC_CONFIG agc;
    struct CVP_DEBUG_CONFIG debug;
} __attribute__((packed));

struct cvp_dms_hybrid_cfg_t {
    struct CVP_MIC_SEL_CONFIG mic_sel;
    struct CVP_REF_MIC_CONFIG ref_mic;
    struct CVP_PRE_GAIN_CONFIG pre_gain;
    struct CVP_AEC_CONFIG aec;
    struct CVP_NLP_CONFIG nlp;
    struct CVP_ENC_HYBRID_CONFIG enc;
    struct CVP_DNS_CONFIG dns;
    struct CVP_AGC_INTERNAL_CONFIG agc;
    struct CVP_WNC_CONFIG wnc;
    struct CVP_DEBUG_CONFIG debug;
} __attribute__((packed));

struct cvp_dms_awn_cfg_t {
    struct CVP_MIC_SEL_CONFIG mic_sel;
    struct CVP_REF_MIC_CONFIG ref_mic;
    struct CVP_PRE_GAIN_CONFIG pre_gain;
    struct CVP_AEC_CONFIG aec;
    struct CVP_NLP_CONFIG nlp;
    struct CVP_DNS_CONFIG dns;
    struct CVP_AGC_INTERNAL_CONFIG agc;
    struct CVP_WNC_CONFIG wnc;
    struct CVP_DEBUG_CONFIG debug;
} __attribute__((packed));

struct cvp_cfg_t {
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    struct cvp_dms_beamfroming_cfg_t dms_beamfroming;
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    struct cvp_dms_flexible_cfg_t dms_flexible;
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    struct cvp_dms_hybrid_cfg_t dms_hybrid;
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    struct cvp_dms_awn_cfg_t dms_awn;
#endif
} __attribute__((packed));

//------------------stream.bin CVP参数文件解析结构-END---------------//

struct cvp_node_hdl {
    char name[16];
    union  {
        AEC_DMS_CONFIG dms_beamfroming;
        DMS_FLEXIBLE_CONFIG dms_flexible;
        DMS_HYBRID_CONFIG dms_hybrid;
        DMS_AWN_CONFIG dms_awn;
    } online_cfg;
    struct stream_frame *frame[3];	//输入frame存储，算法输入缓存使用
    u8 buf_cnt;						//循环输入buffer位置
    u8 mic_swap;					//主副MIC顺序交换标志
    s16 buf[CVP_INPUT_SIZE];
    s16 buf_1[CVP_INPUT_SIZE];
    s16 *buf_2;
    u32 ref_sr;
    u16 source_uuid; //源节点uuid
    struct CVP_MIC_SEL_CONFIG mic_sel;
    struct CVP_REF_MIC_CONFIG ref_mic;
};

static struct cvp_node_hdl *g_cvp_hdl;

int cvp_node_output_handle(s16 *data, u16 len)
{
    struct stream_frame *frame;
    frame = jlstream_get_frame(hdl_node(g_cvp_hdl)->oport, len);
    if (!frame) {
        return 0;
    }
    frame->len = len;
    memcpy(frame->data, data, len);
    jlstream_push_frame(hdl_node(g_cvp_hdl)->oport, frame);
    return len;
}

extern float eq_db2mag(float x);
void cvp_node_dms_beamfroming_cfg_update(struct cvp_dms_beamfroming_cfg_t *cfg, void *priv)
{
    AEC_DMS_CONFIG *p = (AEC_DMS_CONFIG *)priv;

    if (g_cvp_hdl) {
        g_cvp_hdl->mic_swap = (cfg->mic_sel.talk_mic > cfg->mic_sel.talk_ref_mic) ? 1 : 0;
        g_cvp_hdl->mic_sel.talk_mic = cfg->mic_sel.talk_mic;
        g_cvp_hdl->mic_sel.talk_ref_mic = cfg->mic_sel.talk_ref_mic;
        g_cvp_hdl->ref_mic.en = cfg->ref_mic.en;
        g_cvp_hdl->ref_mic.ref_mic_ch = cfg->ref_mic.ref_mic_ch;
        if (g_cvp_hdl->ref_mic.en && (g_cvp_hdl->buf_2 == NULL)) {
            g_cvp_hdl->buf_2 = zalloc(CVP_INPUT_SIZE * sizeof(short));
        }
        printf("talk_mic %d, talk_ref_mic %d, ref mic en %d, ref_mic_ch %d", g_cvp_hdl->mic_sel.talk_mic, g_cvp_hdl->mic_sel.talk_ref_mic, g_cvp_hdl->ref_mic.en, g_cvp_hdl->ref_mic.ref_mic_ch);
    }
    p->adc_ref_en = cfg->ref_mic.en;

    //更新预处理参数
    struct audio_cvp_pre_param_t pre_cfg;
    pre_cfg.pre_gain_en = cfg->pre_gain.en;
    pre_cfg.talk_mic_gain = eq_db2mag(cfg->pre_gain.talk_mic_gain);
    pre_cfg.talk_ref_mic_gain = eq_db2mag(cfg->pre_gain.talk_ref_mic_gain);
    audio_cvp_probe_param_update(&pre_cfg);

    //更新算法参数
    p->enable_module = cfg->aec.en | (cfg->nlp.en << 1) | (cfg->ans.en << 2) | (cfg->enc.en << 3) | (cfg->agc.en << 4) | (cfg->mfdt.en << 6);

    p->aec_process_maxfrequency = cfg->aec.aec_process_maxfrequency;
    p->aec_process_minfrequency = cfg->aec.aec_process_minfrequency;
    p->af_length = cfg->aec.af_length;

    p->nlp_process_maxfrequency = cfg->nlp.nlp_process_maxfrequency;
    p->nlp_process_minfrequency = cfg->nlp.nlp_process_minfrequency;
    p->overdrive = cfg->nlp.overdrive;

    p->enc_process_maxfreq = cfg->enc.enc_process_maxfreq;
    p->enc_process_minfreq = cfg->enc.enc_process_minfreq;
    p->sir_maxfreq = cfg->enc.sir_maxfreq;
    p->mic_distance = cfg->enc.mic_distance / 1000.0f;	//mm换算成m
    p->target_signal_degradation = eq_db2mag(cfg->enc.target_signal_degradation);	//dB转浮点
    p->enc_aggressfactor = cfg->enc.enc_aggressfactor;
    p->enc_minsuppress = cfg->enc.enc_minsuppress;

    p->aggressfactor = cfg->ans.aggressfactor;
    p->minsuppress = cfg->ans.minsuppress;
    p->init_noise_lvl = cfg->ans.init_noise_lvl;

    p->global_minsuppress = cfg->global.global_minsuppress;

    p->ndt_fade_in = cfg->agc.ndt_fade_in;
    p->ndt_fade_out = cfg->agc.ndt_fade_out;
    p->dt_fade_in = cfg->agc.dt_fade_in;
    p->dt_fade_out = cfg->agc.dt_fade_out;
    p->ndt_max_gain = cfg->agc.ndt_max_gain;
    p->ndt_min_gain = cfg->agc.ndt_min_gain;
    p->ndt_speech_thr = cfg->agc.ndt_speech_thr;
    p->dt_max_gain = cfg->agc.dt_max_gain;
    p->dt_min_gain = cfg->agc.dt_min_gain;
    p->dt_speech_thr = cfg->agc.dt_speech_thr;
    p->echo_present_thr = cfg->agc.echo_present_thr;

    p->detect_time = cfg->mfdt.detect_time;            // in second
    p->detect_eng_diff_thr = cfg->mfdt.detect_eng_diff_thr;     //  dB
    p->detect_eng_lowerbound = cfg->mfdt.detect_eng_lowerbound; // 0~-90 dB start detect when mic energy lower than this
    p->MalfuncDet_MaxFrequency = cfg->mfdt.MalfuncDet_MaxFrequency;  //检测频率上限
    p->MalfuncDet_MinFrequency = cfg->mfdt.MalfuncDet_MinFrequency;   //检测频率下限
    p->OnlyDetect = cfg->mfdt.OnlyDetect;// 0 -> 故障切换到单mic模式， 1-> 只检测不切换

    p->output_sel = cfg->debug.output_sel;

}

void cvp_node_dms_flexible_cfg_update(struct cvp_dms_flexible_cfg_t *cfg, void *priv)
{
    DMS_FLEXIBLE_CONFIG *p = (DMS_FLEXIBLE_CONFIG *)priv;

    if (g_cvp_hdl) {
        g_cvp_hdl->mic_swap = (cfg->mic_sel.talk_mic > cfg->mic_sel.talk_ref_mic) ? 1 : 0;
        g_cvp_hdl->mic_sel.talk_mic = cfg->mic_sel.talk_mic;
        g_cvp_hdl->mic_sel.talk_ref_mic = cfg->mic_sel.talk_ref_mic;
        g_cvp_hdl->ref_mic.en = cfg->ref_mic.en;
        g_cvp_hdl->ref_mic.ref_mic_ch = cfg->ref_mic.ref_mic_ch;
        if (g_cvp_hdl->ref_mic.en && (g_cvp_hdl->buf_2 == NULL)) {
            g_cvp_hdl->buf_2 = zalloc(CVP_INPUT_SIZE * sizeof(short));
        }
        printf("talk_mic %d, talk_ref_mic %d, ref mic en %d, ref_mic_ch %d", g_cvp_hdl->mic_sel.talk_mic, g_cvp_hdl->mic_sel.talk_ref_mic, g_cvp_hdl->ref_mic.en, g_cvp_hdl->ref_mic.ref_mic_ch);
    }
    p->adc_ref_en = cfg->ref_mic.en;

    //更新预处理参数
    struct audio_cvp_pre_param_t pre_cfg;
    pre_cfg.pre_gain_en = cfg->pre_gain.en;
    pre_cfg.talk_mic_gain = eq_db2mag(cfg->pre_gain.talk_mic_gain);
    pre_cfg.talk_ref_mic_gain = eq_db2mag(cfg->pre_gain.talk_ref_mic_gain);
    audio_cvp_probe_param_update(&pre_cfg);

    //更新算法参数
    p->enable_module = cfg->aec.en | (cfg->nlp.en << 1) | (cfg->ans.en << 2) | (cfg->enc.en << 3) | (cfg->agc.en << 4);

    p->aec_process_maxfrequency = cfg->aec.aec_process_maxfrequency;
    p->aec_process_minfrequency = cfg->aec.aec_process_minfrequency;
    p->af_length = cfg->aec.af_length;

    p->nlp_process_maxfrequency = cfg->nlp.nlp_process_maxfrequency;
    p->nlp_process_minfrequency = cfg->nlp.nlp_process_minfrequency;
    p->overdrive = cfg->nlp.overdrive;

    p->enc_suppress_pre = cfg->enc.enc_suppress_pre;
    p->enc_suppress_post = cfg->enc.enc_suppress_post;
    p->enc_minsuppress = cfg->enc.enc_minsuppress;
    p->enc_disconverge_erle_thr = cfg->enc.enc_disconverge_erle_thr;

    p->aggressfactor = cfg->ans.aggressfactor;
    p->minsuppress = cfg->ans.minsuppress;
    p->init_noise_lvl = cfg->ans.init_noise_lvl;


    p->ndt_fade_in = cfg->agc.ndt_fade_in;
    p->ndt_fade_out = cfg->agc.ndt_fade_out;
    p->dt_fade_in = cfg->agc.dt_fade_in;
    p->dt_fade_out = cfg->agc.dt_fade_out;
    p->ndt_max_gain = cfg->agc.ndt_max_gain;
    p->ndt_min_gain = cfg->agc.ndt_min_gain;
    p->ndt_speech_thr = cfg->agc.ndt_speech_thr;
    p->dt_max_gain = cfg->agc.dt_max_gain;
    p->dt_min_gain = cfg->agc.dt_min_gain;
    p->dt_speech_thr = cfg->agc.dt_speech_thr;
    p->echo_present_thr = cfg->agc.echo_present_thr;

    p->output_sel = cfg->debug.output_sel;
}

void cvp_node_dms_awn_cfg_update(struct cvp_dms_awn_cfg_t *cfg, void *priv)
{
    DMS_AWN_CONFIG *p = (DMS_AWN_CONFIG *)priv;
    if (g_cvp_hdl) {
        g_cvp_hdl->mic_swap = (cfg->mic_sel.talk_mic > cfg->mic_sel.talk_ref_mic) ? 1 : 0;
        g_cvp_hdl->mic_sel.talk_mic = cfg->mic_sel.talk_mic;
        g_cvp_hdl->mic_sel.talk_ref_mic = cfg->mic_sel.talk_ref_mic;
        g_cvp_hdl->ref_mic.en = cfg->ref_mic.en;
        g_cvp_hdl->ref_mic.ref_mic_ch = cfg->ref_mic.ref_mic_ch;
        if (g_cvp_hdl->ref_mic.en && (g_cvp_hdl->buf_2 == NULL)) {
            g_cvp_hdl->buf_2 = zalloc(CVP_INPUT_SIZE * sizeof(short));
        }
        printf("talk_mic %d, talk_ref_mic %d, ref mic en %d, ref_mic_ch %d", g_cvp_hdl->mic_sel.talk_mic, g_cvp_hdl->mic_sel.talk_ref_mic, g_cvp_hdl->ref_mic.en, g_cvp_hdl->ref_mic.ref_mic_ch);
    }
    p->adc_ref_en = cfg->ref_mic.en;

    //更新预处理参数
    struct audio_cvp_pre_param_t pre_cfg;
    pre_cfg.pre_gain_en = cfg->pre_gain.en;
    pre_cfg.talk_mic_gain = eq_db2mag(cfg->pre_gain.talk_mic_gain);
    pre_cfg.talk_ref_mic_gain = eq_db2mag(cfg->pre_gain.talk_ref_mic_gain);
    audio_cvp_probe_param_update(&pre_cfg);

    //更新算法参数
    p->enable_module = cfg->aec.en | (cfg->nlp.en << 1) | (cfg->dns.en << 2) | (0  << 3) | (cfg->agc.en << 4) | (cfg->wnc.en << 5);

    p->aec_process_maxfrequency = cfg->aec.aec_process_maxfrequency;
    p->aec_process_minfrequency = cfg->aec.aec_process_minfrequency;
    p->af_length = cfg->aec.af_length;

    p->nlp_process_maxfrequency = cfg->nlp.nlp_process_maxfrequency;
    p->nlp_process_minfrequency = cfg->nlp.nlp_process_minfrequency;
    p->overdrive = cfg->nlp.overdrive;

    p->dns_process_maxfrequency = cfg->dns.dns_process_maxfrequency;
    p->dns_process_minfrequency = cfg->dns.dns_process_minfrequency;
    p->aggressfactor = cfg->dns.aggressfactor;
    p->minsuppress = cfg->dns.minsuppress;
    p->init_noise_lvl = cfg->dns.init_noise_lvl;

    p->agc_type = cfg->agc.agc_type;
    if (p->agc_type == AGC_EXTERNAL) {
        p->agc.agc_ext.ndt_fade_in = cfg->agc.ndt_fade_in;
        p->agc.agc_ext.ndt_fade_out = cfg->agc.ndt_fade_out;
        p->agc.agc_ext.dt_fade_in = cfg->agc.dt_fade_in;
        p->agc.agc_ext.dt_fade_out = cfg->agc.dt_fade_out;
        p->agc.agc_ext.ndt_max_gain = cfg->agc.ndt_max_gain;
        p->agc.agc_ext.ndt_min_gain = cfg->agc.ndt_min_gain;
        p->agc.agc_ext.ndt_speech_thr = cfg->agc.ndt_speech_thr;
        p->agc.agc_ext.dt_max_gain = cfg->agc.dt_max_gain;
        p->agc.agc_ext.dt_min_gain = cfg->agc.dt_min_gain;
        p->agc.agc_ext.dt_speech_thr = cfg->agc.dt_speech_thr;
        p->agc.agc_ext.echo_present_thr = cfg->agc.echo_present_thr;
    } else {
        p->agc.agc_int.min_mag_db_level = cfg->agc.min_mag_db_level;
        p->agc.agc_int.max_mag_db_level = cfg->agc.max_mag_db_level;
        p->agc.agc_int.addition_mag_db_level = cfg->agc.addition_mag_db_level;
        p->agc.agc_int.clip_mag_db_level = cfg->agc.clip_mag_db_level;
        p->agc.agc_int.floor_mag_db_level = cfg->agc.floor_mag_db_level;
    }
    p->coh_val_T = cfg->wnc.coh_val_T;
    p->eng_db_T = cfg->wnc.eng_db_T;
    p->output_sel = cfg->debug.output_sel;
}

void cvp_node_dms_hybrid_cfg_update(struct cvp_dms_hybrid_cfg_t *cfg, void *priv)
{
    DMS_HYBRID_CONFIG *p = (DMS_HYBRID_CONFIG *)priv;

    if (g_cvp_hdl) {
        g_cvp_hdl->mic_swap = (cfg->mic_sel.talk_mic > cfg->mic_sel.talk_ref_mic) ? 1 : 0;
        g_cvp_hdl->mic_sel.talk_mic = cfg->mic_sel.talk_mic;
        g_cvp_hdl->mic_sel.talk_ref_mic = cfg->mic_sel.talk_ref_mic;
        g_cvp_hdl->ref_mic.en = cfg->ref_mic.en;
        g_cvp_hdl->ref_mic.ref_mic_ch = cfg->ref_mic.ref_mic_ch;
        if (g_cvp_hdl->ref_mic.en && (g_cvp_hdl->buf_2 == NULL)) {
            g_cvp_hdl->buf_2 = zalloc(CVP_INPUT_SIZE * sizeof(short));
        }
        printf("talk_mic %d, talk_ref_mic %d, ref mic en %d, ref_mic_ch %d", g_cvp_hdl->mic_sel.talk_mic, g_cvp_hdl->mic_sel.talk_ref_mic, g_cvp_hdl->ref_mic.en, g_cvp_hdl->ref_mic.ref_mic_ch);
    }
    p->adc_ref_en = cfg->ref_mic.en;

    //更新预处理参数
    struct audio_cvp_pre_param_t pre_cfg;
    pre_cfg.pre_gain_en = cfg->pre_gain.en;
    pre_cfg.talk_mic_gain = eq_db2mag(cfg->pre_gain.talk_mic_gain);
    pre_cfg.talk_ref_mic_gain = eq_db2mag(cfg->pre_gain.talk_ref_mic_gain);
    audio_cvp_probe_param_update(&pre_cfg);

    //更新算法参数
    p->enable_module = cfg->aec.en | (cfg->nlp.en << 1) | (cfg->dns.en << 2) | (cfg->enc.en << 3) | (cfg->agc.en << 4) | (cfg->wnc.en << 5);

    p->aec_process_maxfrequency = cfg->aec.aec_process_maxfrequency;
    p->aec_process_minfrequency = cfg->aec.aec_process_minfrequency;
    p->af_length = cfg->aec.af_length;

    p->nlp_process_maxfrequency = cfg->nlp.nlp_process_maxfrequency;
    p->nlp_process_minfrequency = cfg->nlp.nlp_process_minfrequency;
    p->overdrive = cfg->nlp.overdrive;

    p->enc_process_maxfreq = cfg->enc.enc_process_maxfreq;
    p->enc_process_minfreq = cfg->enc.enc_process_minfreq;
    p->snr_db_T0 = cfg->enc.snr_db_T0;
    p->snr_db_T1 = cfg->enc.snr_db_T1;
    p->floor_noise_db_T = cfg->enc.floor_noise_db_T;
    p->compen_db = cfg->enc.compen_db;

    p->dns_process_maxfrequency = cfg->dns.dns_process_maxfrequency;
    p->dns_process_minfrequency = cfg->dns.dns_process_minfrequency;
    p->aggressfactor = cfg->dns.aggressfactor;
    p->minsuppress = cfg->dns.minsuppress;
    p->init_noise_lvl = cfg->dns.init_noise_lvl;

    p->agc_type = cfg->agc.agc_type;
    if (p->agc_type == AGC_EXTERNAL) {
        p->agc.agc_ext.ndt_fade_in = cfg->agc.ndt_fade_in;
        p->agc.agc_ext.ndt_fade_out = cfg->agc.ndt_fade_out;
        p->agc.agc_ext.dt_fade_in = cfg->agc.dt_fade_in;
        p->agc.agc_ext.dt_fade_out = cfg->agc.dt_fade_out;
        p->agc.agc_ext.ndt_max_gain = cfg->agc.ndt_max_gain;
        p->agc.agc_ext.ndt_min_gain = cfg->agc.ndt_min_gain;
        p->agc.agc_ext.ndt_speech_thr = cfg->agc.ndt_speech_thr;
        p->agc.agc_ext.dt_max_gain = cfg->agc.dt_max_gain;
        p->agc.agc_ext.dt_min_gain = cfg->agc.dt_min_gain;
        p->agc.agc_ext.dt_speech_thr = cfg->agc.dt_speech_thr;
        p->agc.agc_ext.echo_present_thr = cfg->agc.echo_present_thr;
    } else {
        p->agc.agc_int.min_mag_db_level = cfg->agc.min_mag_db_level;
        p->agc.agc_int.max_mag_db_level = cfg->agc.max_mag_db_level;
        p->agc.agc_int.addition_mag_db_level = cfg->agc.addition_mag_db_level;
        p->agc.agc_int.clip_mag_db_level = cfg->agc.clip_mag_db_level;
        p->agc.agc_int.floor_mag_db_level = cfg->agc.floor_mag_db_level;
    }

    p->coh_val_T = cfg->wnc.coh_val_T;
    p->eng_db_T = cfg->wnc.eng_db_T;

    p->output_sel = cfg->debug.output_sel;
}

void cvp_node_param_cfg_update(struct cvp_cfg_t *cfg, void *priv)
{
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    cvp_node_dms_beamfroming_cfg_update(&cfg->dms_beamfroming, priv);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    cvp_node_dms_flexible_cfg_update(&cfg->dms_flexible, priv);
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    cvp_node_dms_hybrid_cfg_update(&cfg->dms_hybrid, priv);
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    cvp_node_dms_awn_cfg_update(&cfg->dms_awn, priv);
#endif
}

struct cvp_cfg_t global_cvp_cfg;
int cvp_param_cfg_read(void)
{
    u8 subid;
    if (g_cvp_hdl) {
        subid = hdl_node(g_cvp_hdl)->subid;
    } else {
        subid = 0XFF;
    }

    /*
     *解析配置文件内效果配置
     * */
    int len = 0;
    struct node_param ncfg = {0};
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_ANS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_FLEXIBLE_ANS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_FLEXIBLE_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_HYBRID_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_AWN_DNS, subid, (u8 *)&ncfg);
#endif

    if (len != sizeof(ncfg)) {
        printf("cvp_dms_name read ncfg err\n");
        return -2;
    }
    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &global_cvp_cfg);
    }

    printf(" %s len %d, sizeof(global_cvp_cfg) %d\n", __func__,  len, (int)sizeof(global_cvp_cfg));
    if (len != sizeof(global_cvp_cfg)) {
        printf("cvp_dms_param read ncfg err\n");
        return -1 ;
    }
    return 0;
}

u8 cvp_get_talk_mic_ch(void)
{
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    return global_cvp_cfg.dms_beamfroming.mic_sel.talk_mic;
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    return global_cvp_cfg.dms_flexible.mic_sel.talk_mic;
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    return global_cvp_cfg.dms_hybrid.mic_sel.talk_mic;
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    return global_cvp_cfg.dms_awn.mic_sel.talk_mic;
#endif
}

u8 cvp_get_talk_ref_mic_ch(void)
{
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    return global_cvp_cfg.dms_beamfroming.mic_sel.talk_ref_mic;
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    return global_cvp_cfg.dms_flexible.mic_sel.talk_ref_mic;
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    return global_cvp_cfg.dms_hybrid.mic_sel.talk_ref_mic;
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    return global_cvp_cfg.dms_awn.mic_sel.talk_ref_mic;
#endif
}

int cvp_node_param_cfg_read(void *priv, u8 ignore_subid)
{
    struct cvp_cfg_t cfg;
    u8 subid;
    if (g_cvp_hdl) {
        subid = hdl_node(g_cvp_hdl)->subid;
    } else {
        subid = 0XFF;
    }
    /*
     *解析配置文件内效果配置
     * */
    int len = 0;
    struct node_param ncfg = {0};
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_ANS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_FLEXIBLE_ANS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_FLEXIBLE_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_HYBRID_DNS, subid, (u8 *)&ncfg);
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    len = jlstream_read_node_data(NODE_UUID_CVP_DMS_AWN_DNS, subid, (u8 *)&ncfg);
#endif
    if (len != sizeof(ncfg)) {
        printf("cvp_dms_node read ncfg err\n");
        return 0;
    }

    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(mode_index, ncfg.name, cfg_index, &info)) {
        len = jlstream_read_form_cfg_data(&info, &cfg);
    }
    printf(" %s len %d, sizeof(cfg) %d\n", __func__,  len, (int)sizeof(cfg));
    if (len != sizeof(cfg)) {
        return 0 ;
    }

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        if (g_cvp_hdl) {
            memcpy(g_cvp_hdl->name, ncfg.name, sizeof(ncfg.name));
            if (jlstream_read_effects_online_param(hdl_node(g_cvp_hdl)->uuid, g_cvp_hdl->name, &cfg, sizeof(cfg))) {
                printf("get cvp online param\n");
            }
        }
    }
    cvp_node_param_cfg_update(&cfg, priv);

#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    return sizeof(AEC_DMS_CONFIG);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    return sizeof(DMS_FLEXIBLE_CONFIG);
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    return sizeof(DMS_HYBRID_CONFIG);
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    return sizeof(DMS_AWN_CONFIG);
#endif
}

/*节点输出回调处理，可处理数据或post信号量*/
__NODE_CACHE_CODE(cvp_dms)
static void cvp_handle_frame(struct stream_iport *iport, struct stream_note *note)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)iport->node->private_data;
    struct stream_node *node = iport->node;
    s16 *dat, *tbuf_0, *tbuf_1, *tbuf_2;
    int wlen;
    struct stream_frame *in_frame;
    u8 mic_ch = audio_adc_file_get_mic_en_map();

    while (1) {
        in_frame = jlstream_pull_frame(iport, note);		//从iport读取数据
        if (!in_frame) {
            break;
        }
#if TCFG_AUDIO_DUT_ENABLE
        //产测bypass 模式 不经过算法
        if (cvp_dut_mode_get() == CVP_DUT_MODE_BYPASS) {
            jlstream_push_frame(node->oport, in_frame);
            continue;
        }
#endif
        if (hdl->ref_mic.en) { //参考数据硬回采
            wlen = in_frame->len / 3 / 2;	//一个ADC的点数
            //模仿ADCbuff的存储方法
            tbuf_0 = hdl->buf + (wlen * hdl->buf_cnt);
            tbuf_1 = hdl->buf_1 + (wlen * hdl->buf_cnt);
            tbuf_2 = hdl->buf_2 + (wlen * hdl->buf_cnt);
            if (++hdl->buf_cnt > ((CVP_INPUT_SIZE / 256) - 1)) {
                hdl->buf_cnt = 0;
            }
            /*拆分mic数据*/
            dat = (s16 *)in_frame->data;
            for (int i = 0; i < wlen; i++) {
                tbuf_0[i] = dat[3 * i];
                tbuf_1[i] = dat[3 * i + 1];
                tbuf_2[i] = dat[3 * i + 2];
            }
            u8 cnt = 0;
            u8 talk_data_num = 0;//记录通话MIC数据位置
            s16 *mic_data[3];
            mic_data[0] = tbuf_0;
            mic_data[1] = tbuf_1;
            mic_data[2] = tbuf_2;
            for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
                if ((mic_ch & BIT(i)) == hdl->ref_mic.ref_mic_ch) {
                    audio_aec_refbuf(mic_data[cnt++], NULL, wlen << 1);
                    continue;
                }
                if ((mic_ch & BIT(i)) == hdl->mic_sel.talk_mic) {
                    talk_data_num = cnt++;
                    continue;
                }
                if ((mic_ch & BIT(i)) == hdl->mic_sel.talk_ref_mic) {
                    audio_aec_inbuf_ref(mic_data[cnt++], wlen << 1);
                    continue;
                }
            }
            /*通话MIC数据需要最后传进算法*/
            audio_aec_inbuf(mic_data[talk_data_num], wlen << 1);

        } else {//参考数据软回采
            wlen = in_frame->len >> 2;	//一个ADC的点数
            //模仿ADCbuff的存储方法
            tbuf_0 = hdl->buf + (wlen * hdl->buf_cnt);
            tbuf_1 = hdl->buf_1 + (wlen * hdl->buf_cnt);
            if (++hdl->buf_cnt > ((CVP_INPUT_SIZE / 256) - 1)) {
                hdl->buf_cnt = 0;
            }
            dat = (s16 *)in_frame->data;
            for (int i = 0; i < wlen; i++) {
                tbuf_0[i]     = dat[2 * i];
                tbuf_1[i] = dat[2 * i + 1];
            }
            if (hdl->mic_swap) {
                audio_aec_inbuf_ref(tbuf_0, wlen << 1);
                audio_aec_inbuf(tbuf_1, wlen << 1);
            } else {
                audio_aec_inbuf_ref(tbuf_1, wlen << 1);
                audio_aec_inbuf(tbuf_0, wlen << 1);
            }
        }
        jlstream_free_frame(in_frame);	//释放iport资源
    }
}

/*节点预处理-在ioctl之前*/
static int cvp_adapter_bind(struct stream_node *node, u16 uuid)
{
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)node->private_data;

    node->type = NODE_TYPE_ASYNC;
    g_cvp_hdl = hdl;
    return 0;
}

/*打开改节点输入接口*/
static void cvp_ioc_open_iport(struct stream_iport *iport)
{
    iport->handle_frame = cvp_handle_frame;				//注册输出回调
}

/*节点参数协商*/
static int cvp_ioc_negotiate(struct stream_iport *iport)
{
    struct stream_fmt *in_fmt = &iport->prev->fmt;
    struct stream_oport *oport = iport->node->oport;
    int ret = NEGO_STA_ACCPTED;
    int nb_sr, wb_sr, nego_sr;

#if (TCFG_AUDIO_CVP_BAND_WIDTH_CFG == CVP_WB_EN)
    nb_sr = 16000;
    wb_sr = 16000;
    nego_sr  = 16000;
#elif (TCFG_AUDIO_CVP_BAND_WIDTH_CFG == CVP_NB_EN)
    nb_sr = 8000;
    wb_sr = 8000;
    nego_sr  = 8000;
#else
    nb_sr = 8000;
    wb_sr = 16000;
    nego_sr  = 16000;
#endif
    //要求输入为8K或者16K
    if (in_fmt->sample_rate != nb_sr && in_fmt->sample_rate != wb_sr) {
        in_fmt->sample_rate = nego_sr;
        oport->fmt.sample_rate = in_fmt->sample_rate;
        ret = NEGO_STA_CONTINUE | NEGO_STA_SAMPLE_RATE_LOCK;
    }

    //要求输入16bit位宽的数据
    if (in_fmt->bit_wide != DATA_BIT_WIDE_16BIT) {
        in_fmt->bit_wide = DATA_BIT_WIDE_16BIT;
        in_fmt->Qval = AUDIO_QVAL_16BIT;
        oport->fmt.bit_wide = in_fmt->bit_wide;
        oport->fmt.Qval = in_fmt->Qval;
        ret = NEGO_STA_CONTINUE;
    }

    //固定输出单声道数据
    oport->fmt.channel_mode = AUDIO_CH_MIX;
    return ret;
}

/*节点start函数*/
static void cvp_ioc_start(struct cvp_node_hdl *hdl)
{
    struct stream_fmt *fmt = &hdl_node(hdl)->oport->fmt;
    struct audio_aec_init_param_t init_param;
    init_param.sample_rate = fmt->sample_rate;
    init_param.ref_sr = hdl->ref_sr;
    init_param.ref_channel = 1;;
    u8 mic_num; //算法需要使用的MIC个数

    audio_aec_init(&init_param);

    if (hdl->source_uuid == NODE_UUID_ADC) {
        if (hdl->ref_mic.en) {
            /*硬回采需要开3个MIC*/
            mic_num = 3;
        } else {
            /*硬回采需要开2个MIC*/
            mic_num = 2;
        }
        if (audio_adc_file_get_esco_mic_num() != mic_num) {
#if TCFG_AUDIO_DUT_ENABLE
            //使能产测时，只有算法模式才需判断
            if (cvp_dut_mode_get() == CVP_DUT_MODE_ALGORITHM) {
                ASSERT(0, "CVP_DMS, ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), mic_num);
            }
#else
            ASSERT(0, "CVP_DMS, ESCO MIC num is %d != %d\n", audio_adc_file_get_esco_mic_num(), mic_num);
#endif
        }
    }
}

/*节点stop函数*/
static void cvp_ioc_stop(struct cvp_node_hdl *hdl)
{
    if (hdl) {
        audio_aec_close();
    }
}

static int cvp_ioc_update_parm(struct cvp_node_hdl *hdl, int parm)
{
    int ret = false;
    struct cvp_cfg_t *cfg = (struct cvp_cfg_t *)parm;
    if (hdl) {
        cvp_node_param_cfg_update(cfg, &hdl->online_cfg);
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_DNS_MODE)
        aec_dms_cfg_update(&hdl->online_cfg.dms_beamfroming);
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) || (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
        aec_dms_flexible_cfg_update(&hdl->online_cfg.dms_flexible);
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
        aec_dms_hybrid_cfg_update(&hdl->online_cfg.dms_hybrid);
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
        aec_dms_awn_cfg_update(&hdl->online_cfg.dms_awn);
#endif
        ret = true;
    }
    return ret;
}

/*节点ioctl函数*/
static int cvp_adapter_ioctl(struct stream_iport *iport, int cmd, int arg)
{
    int ret = 0;
    struct cvp_node_hdl *hdl = (struct cvp_node_hdl *)iport->node->private_data;

    switch (cmd) {
    case NODE_IOC_OPEN_IPORT:
        cvp_ioc_open_iport(iport);
        break;
    case NODE_IOC_OPEN_OPORT:
        break;
    case NODE_IOC_CLOSE_IPORT:
        break;
    case NODE_IOC_NEGOTIATE:
        *(int *)arg |= cvp_ioc_negotiate(iport);
        break;
    case NODE_IOC_SET_FMT:
        hdl->ref_sr = (u32)arg;
        break;
    case NODE_IOC_START:
        cvp_ioc_start(hdl);
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        cvp_ioc_stop(hdl);
        break;
    case NODE_IOC_NAME_MATCH:
        if (!strcmp((const char *)arg, hdl->name)) {
            ret = 1;
        }
        break;
    case NODE_IOC_SET_PARAM:
        ret = cvp_ioc_update_parm(hdl, arg);
        break;
    case NODE_IOC_SET_PRIV_FMT:
        hdl->source_uuid = (u16)arg;
        printf("source_uuid %x", (int)hdl->source_uuid);
        break;
    }

    return ret;
}

/*节点用完释放函数*/
static void cvp_adapter_release(struct stream_node *node)
{
    if (g_cvp_hdl->buf_2) {
        free(g_cvp_hdl->buf_2);
        g_cvp_hdl->buf_2 = NULL;
    }
    g_cvp_hdl = NULL;
}

/*节点adapter 注意需要在sdk_used_list声明，否则会被优化*/
REGISTER_STREAM_NODE_ADAPTER(cvp_node_adapter) = {
#if (TCFG_AUDIO_CVP_DMS_ANS_MODE)
    .name       = "cvp_dms_ans",
    .uuid       = NODE_UUID_CVP_DMS_ANS,
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    .name       = "cvp_dms_dns",
    .uuid       = NODE_UUID_CVP_DMS_DNS,
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE)
    .name       = "cvp_dms_flexible_ans",
    .uuid       = NODE_UUID_CVP_DMS_FLEXIBLE_ANS,
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    .name       = "cvp_dms_flexible_dns",
    .uuid       = NODE_UUID_CVP_DMS_FLEXIBLE_DNS,
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    .name       = "cvp_dms_hybrid_dns",
    .uuid       = NODE_UUID_CVP_DMS_HYBRID_DNS,
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    .name       = "cvp_dms_awn_dns",
    .uuid       = NODE_UUID_CVP_DMS_AWN_DNS,
#endif
    .bind       = cvp_adapter_bind,
    .ioctl      = cvp_adapter_ioctl,
    .release    = cvp_adapter_release,
    .hdl_size   = sizeof(struct cvp_node_hdl),
    .ability_channel_convert = 1, //支持声道转换
};

//注册工具在线调试
REGISTER_ONLINE_ADJUST_TARGET(cvp_dms) = {
#if TCFG_AUDIO_CVP_DMS_ANS_MODE
    .uuid       = NODE_UUID_CVP_DMS_ANS,
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE)
    .uuid       = NODE_UUID_CVP_DMS_DNS,
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE)
    .uuid       = NODE_UUID_CVP_DMS_FLEXIBLE_ANS,
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE)
    .uuid       = NODE_UUID_CVP_DMS_FLEXIBLE_DNS,
#elif (TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE)
    .uuid       = NODE_UUID_CVP_DMS_HYBRID_DNS,
#elif (TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE)
    .uuid       = NODE_UUID_CVP_DMS_AWN_DNS,
#endif
};

#endif/* TCFG_AUDIO_CVP_DMS_ANS_MODE || TCFG_AUDIO_CVP_DMS_DNS_MODE*/

