/*
 ****************************************************************
 *							AUDIO ANC
 * File  : audio_anc.c
 * By    :
 * Notes : ref_mic = 参考mic
 *		   err_mic = 误差mic
 *
 ****************************************************************
 */
#include "system/includes.h"
#include "audio_anc.h"
#include "system/task.h"
#include "timer.h"
#include "online_db_deal.h"
#include "app_config.h"
#include "app_tone.h"
#include "asm/audio_adc.h"
#include "audio_anc_coeff.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "audio_manager.h"
#include "audio_config.h"
#include "esco_player.h"
#include "a2dp_player.h"
#include "adc_file.h"
#include "clock.h"
#include "jlstream.h"
#include "audio_anc_common_plug.h"
#include "clock_manager/clock_manager.h"
#include "dac_node.h"
#if ANC_HOWLING_DETECT_EN
#include "anc_howling_detect.h"
#endif

#if TCFG_AUDIO_ANC_EXT_TOOL_ENABLE
#include "anc_ext_tool.h"
#endif
#if RCSP_ADV_EN
#include "adv_anc_voice.h"
#endif
#if ANC_EAR_ADAPTIVE_EN
#include "adv_adaptive_noise_reduction.h"
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt.h"
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/
#if THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN
#include "ble_iot_anc_manager.h"
#endif

#if ANC_MULT_ORDER_ENABLE
#include "audio_anc_mult_scene.h"
#endif/*ANC_MULT_ORDER_ENABLE*/

#if TCFG_AUDIO_FIT_DET_ENABLE
#include "icsd_dot_app.h"
#endif

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   bss_seg(".anc_user.data.bss")
#pragma  data_seg(".anc_user.data")
#pragma const_seg(".anc_user.text.const")
#pragma  code_seg(".anc_user.text")
#endif/*SUPPORT_MS_EXTENSIONS*/

#define AT_VOLATILE_RAM_CODE_ANC        AT(.anc_user.text.cache.L1)

#if INEAR_ANC_UI
extern u8 inear_tws_ancmode;
#endif/*INEAR_ANC_UI*/

#if TCFG_AUDIO_ANC_ENABLE

#if 1
#define user_anc_log	printf
#else
#define user_anc_log(...)
#endif/*log_en*/


/*************************ANC增益配置******************************/
#define ANC_DAC_GAIN			13			//ANC Speaker增益
#define ANC_REF_MIC_GAIN	 	8			//ANC参考Mic增益
#define ANC_ERR_MIC_GAIN	    8			//ANC误差Mic增益
/*****************************************************************/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
const u8 CONST_ANC_ADT_EN = 1;
#else
const u8 CONST_ANC_ADT_EN = 0;
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if ANC_EAR_ADAPTIVE_EN
const u8 CONST_ANC_EAR_ADAPTIVE_EN = 1;
#else
const u8 CONST_ANC_EAR_ADAPTIVE_EN = 0;
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if TCFG_ANC_SELF_DUT_GET_SZ
const u8 CONST_ANC_DUT_SZ_EN = 1;
#else
const u8 CONST_ANC_DUT_SZ_EN = 0;
#endif/*TCFG_ANC_SELF_DUT_GET_SZ*/

#if TCFG_AUDIO_FIT_DET_ENABLE
const u8 CONST_ANC_DOT_EN = 1;
#else
const u8 CONST_ANC_DOT_EN = 0;
#endif /*TCFG_AUDIO_FIT_DET_ENABLE*/

#if ANC_HOWLING_DETECT_EN
const u8 CONST_ANC_HOWLING_MSG_DEBUG = ANC_HOWLING_MSG_DEBUG;
#else
const u8 CONST_ANC_HOWLING_MSG_DEBUG = 0;
#endif

#define TWS_ANC_SYNC_TIMEOUT	400 //ms


#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
float anc_trans_lfb_gain = 0.0625f;

float anc_trans_rfb_gain = 0.0625f;

const double anc_trans_lfb_coeff[] = {
    0.195234751212410628795623779296875,
    0.1007948522455990314483642578125,
    0.0427739980514161288738250732421875,
    -1.02504855208098888397216796875,
    0.36385215423069894313812255859375,
    /*
    0.0508266850956715643405914306640625,
    -0.1013386586564593017101287841796875,
    0.0505129448720254004001617431640625,
    -1.99860572628676891326904296875,
    0.9986066981218755245208740234375,
    */
};

const double anc_trans_rfb_coeff[] = {
    0.195234751212410628795623779296875,
    0.1007948522455990314483642578125,
    0.0427739980514161288738250732421875,
    -1.02504855208098888397216796875,
    0.36385215423069894313812255859375,
};
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

static void anc_mix_out_audio_drc_thr(float thr);
static void anc_fade_in_timeout(void *arg);
static void anc_timer_deal(void *priv);
static int anc_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size);
static anc_coeff_t *anc_cfg_test(u8 coeff_en, u8 gains_en);
static void anc_fade_in_timer_add(audio_anc_t *param);
void anc_dmic_io_init(audio_anc_t *param, u8 en);
u8 anc_btspp_train_again(u8 mode, u32 dat);
static void anc_tone_play_and_mode_switch(u8 mode, u8 preemption, u8 cb_sel);
void anc_mode_enable_set(u8 mode_enable);
void audio_anc_post_msg_drc(void);
void audio_anc_post_msg_debug(void);
void audio_anc_post_msg_user_train_run(void);
void audio_anc_mic_management(audio_anc_t *param);
void audio_anc_adc_ch_set(void);

static void audio_anc_sz_fft_dac_check_slience_cb(void *buf, int len);
void audio_anc_post_msg_sz_fft_run(void);
static void audio_icsd_biquad2ab_out(float gain, float f, float fs, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, int type);

extern u8 bt_phone_dec_is_running();
extern u8 bt_media_is_running(void);
extern void audio_adc_fixed_digital_set_buffs(void);

void audio_anc_howldet_fade_set(u16 gain);

#define ESCO_MIC_DUMP_CNT	10
extern void esco_mic_dump_set(u16 dump_cnt);


typedef struct {
    u8 mode_num;					/*模式总数*/
    u8 mode_enable;					/*使能的模式*/
    u8 anc_parse_seq;
    u8 suspend;						/*挂起*/
    u8 ui_mode_sel; 				/*ui菜单降噪模式选择*/
    u8 new_mode;					/*记录模式切换最新的目标模式*/
    u8 last_mode;					/*记录模式切换的上一个模式*/
    u8 tone_mode;					/*记录当前提示音播放的模式*/
    u8 mic_dy_state;        		/*动态MIC增益状态*/
    char mic_diff_gain;				/*动态MIC增益差值*/
#if ANC_MULT_ORDER_ENABLE
    u16 scene_id;					/*多场景滤波器-场景ID*/
    u8 scene_max;					/*多场景滤波器-最大场景个数*/
#endif/*ANC_MULT_ORDER_ENABLE*/
    u16 ui_mode_sel_timer;
    u16 fade_in_timer;
    u16 fade_gain;
    u16 mic_resume_timer;			/*动态MIC增益恢复定时器id*/
    float drc_ratio;				/*动态MIC增益，对应DRC增益比例*/
    volatile u8 state;				/*ANC状态*/
    volatile u8 mode_switch_lock;	/*切模式锁存，1表示正在切ANC模式*/
    volatile u8 sync_busy;
    audio_anc_t param;				/*ANC lib句柄*/
    audio_adc_mic_mana_t dy_mic_param[4];	/*MIC动态增益参数管理*/
#if ANC_EAR_ADAPTIVE_EN
    u8 ear_adaptive;				/*耳道自适应-模式标志*/
#endif/*ANC_EAR_ADAPTIVE_EN*/

} anc_t;
static anc_t *anc_hdl = NULL;
extern struct audio_dac_hdl dac_hdl;
extern struct audio_adc_hdl adc_hdl;

__attribute__((weak))
void audio_anc_event_to_user(int mode)
{

}
u32 get_anc_gains_sign()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.gain_sign;
    } else {
        return 0;
    }
}

u32 get_anc_gains_alogm()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.alogm;
    } else {
        return 0;
    }
}

void set_anc_gains_alogm(u32 alogm)
{
    if (anc_hdl) {
        anc_hdl->param.gains.alogm = alogm;
    }
}

u32 get_anc_gains_trans_alogm()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.trans_alogm;
    } else {
        return 0;
    }
}

u32 audio_anc_gains_alogm_get(enum ANC_IIR_TYPE type)
{
    if (anc_hdl) {
        switch (type) {
        case ANC_FF_TYPE:
        case ANC_FB_TYPE:
        case ANC_CMP_TYPE:
            return anc_hdl->param.gains.alogm;
        case ANC_TRANS_TYPE:
            return anc_hdl->param.gains.trans_alogm;
        }
    }
    return -1;
}

void *get_anc_lfb_coeff()
{
    if (anc_hdl) {
        return anc_hdl->param.lfb_coeff;
    } else {
        return NULL;
    }
}

float get_anc_gains_l_fbgain()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.l_fbgain;
    } else {
        return 0;
    }
}

u8 get_anc_l_fbyorder()
{
    if (anc_hdl) {
        return anc_hdl->param.lfb_yorder;
    } else {
        return 0;
    }
}

void *get_anc_lff_coeff()
{
    if (anc_hdl) {
        printf("anc hdl lff coeff:%x\n", (int)anc_hdl->param.lff_coeff);
        return anc_hdl->param.lff_coeff;
    } else {
        printf("anc hdl NULL~~~~~~~~\n");
        return NULL;
    }
}

float get_anc_gains_l_ffgain()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.l_ffgain;
    } else {
        return 0;
    }
}

u8 get_anc_l_ffyorder()
{
    if (anc_hdl) {
        return anc_hdl->param.lff_yorder;
    } else {
        return 0;
    }
}

void *get_anc_ltrans_coeff()
{
    if (anc_hdl) {
        return anc_hdl->param.ltrans_coeff;
    } else {
        return NULL;
    }
}

float get_anc_gains_l_transgain()
{
    if (anc_hdl) {
        return anc_hdl->param.gains.l_transgain;
    } else {
        return 0;
    }
}

u8 get_anc_l_transyorder()
{
    if (anc_hdl) {
        return anc_hdl->param.ltrans_yorder;
    } else {
        return 0;
    }
}

void *get_anc_ltrans_fb_coeff()
{
    if (anc_hdl) {
        return anc_hdl->param.ltrans_fb_coeff;
    } else {
        return NULL;
    }
}

float get_anc_gains_lfb_transgain()
{
    if (anc_hdl) {
        return anc_hdl->param.ltrans_fbgain;
    } else {
        return 0;
    }
}

u8 get_anc_lfb_transyorder()
{
    if (anc_hdl) {
        return anc_hdl->param.ltrans_fb_yorder;
    } else {
        return 0;
    }
}

void set_anc_adt_state(u8 state)
{
    if (anc_hdl) {
        anc_hdl->param.adt_state = state;
    }
}

static void anc_task(void *p)
{
    int res;
    int anc_ret = 0;
    int msg[16];
    u8 speak_to_chat_flag = 0;
    u8 adt_flag = 0;
    u8 cur_anc_mode;
    user_anc_log(">>>ANC TASK<<<\n");
    while (1) {
        res = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case ANC_MSG_TRAIN_OPEN:/*启动训练模式*/
                audio_mic_pwr_ctl(MIC_PWR_ON);
                anc_dmic_io_init(&anc_hdl->param, 1);
                user_anc_log("ANC_MSG_TRAIN_OPEN");
                audio_anc_train(&anc_hdl->param, 1);
                os_time_dly(1);
                anc_train_close();
                audio_mic_pwr_ctl(MIC_PWR_OFF);
                break;
            case ANC_MSG_TRAIN_CLOSE:/*训练关闭模式*/
                user_anc_log("ANC_MSG_TRAIN_CLOSE");
                anc_train_close();
                break;
            case ANC_MSG_RUN:
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
                if (anc_hdl->param.gains.trans_alogm != 5) {
                    ASSERT(0, "ERR!!ANC ADT trans_alogm %d != 5\n", anc_hdl->param.gains.trans_alogm);
                }
                audio_anc_mode_switch_in_adt(anc_hdl->param.mode);
                if (get_adt_open_in_anc_state() && anc_hdl->param.mode == ANC_OFF) {
                    user_anc_log("[ADT] ANC_MSG_RUN:%s \n", anc_mode_str[anc_hdl->param.mode]);
                    anc_hdl->param.anc_fade_gain = 0;
                    anc_hdl->param.mode = ANC_ON;
                    speak_to_chat_flag = 1;
                } else {
                    anc_hdl->param.anc_fade_gain = 16384;
                    speak_to_chat_flag = 0;
                }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
                cur_anc_mode = anc_hdl->param.mode;/*保存当前anc模式，防止切换过程anc_hdl->param.mode发生切换改变*/
                audio_anc_event_to_user(cur_anc_mode);

#if ANC_MULT_ORDER_ENABLE
                if (cur_anc_mode != ANC_OFF) {
#if ANC_EAR_ADAPTIVE_EN
                    if (anc_hdl->param.anc_coeff_mode == ANC_COEFF_MODE_ADAPTIVE) {
                        audio_anc_coeff_adaptive_set(ANC_COEFF_MODE_ADAPTIVE, 0);
                    } else {
                        anc_mult_scene_set(anc_hdl->scene_id);
                    }
#else
                    anc_mult_scene_set(anc_hdl->scene_id);
#endif/*ANC_EAR_ADAPTIVE_EN*/
                }
#endif/*ANC_MULT_ORDER_ENABLE*/

                user_anc_log("ANC_MSG_RUN:%s \n", anc_mode_str[cur_anc_mode]);
#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
                /*APP增益设置*/
                anc_hdl->param.anc_fade_gain = rcsp_adv_anc_voice_value_get(cur_anc_mode);
#endif/*RCSP_ADV_EN && RCSP_ADV_ANC_VOICE*/
#if THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN
                /*APP增益设置*/
                anc_hdl->param.anc_fade_gain = iot_anc_effect_value_get(cur_anc_mode);
#endif

#if ANC_HOWLING_DETECT_EN
                //复位啸叫检测相关变量
                anc_howling_detect_reset();
#endif

#if TCFG_AUDIO_DYNAMIC_ADC_GAIN
                if (anc_hdl->mic_resume_timer) {
                    sys_timer_del(anc_hdl->mic_resume_timer);
                    anc_hdl->mic_resume_timer = 0;
                }
#endif/*TCFG_AUDIO_DYNAMIC_ADC_GAIN*/
#if (TCFG_AUDIO_ADC_MIC_CHA == LADC_CH_PLNK)
                /* esco_mic_dump_set(ESCO_MIC_DUMP_CNT); */
#endif/*LADC_CH_PLNK*/
                if (anc_hdl->state == ANC_STA_INIT) {
                    audio_mic_pwr_ctl(MIC_PWR_ON);
                    anc_dmic_io_init(&anc_hdl->param, 1);
#if ANC_MODE_SYSVDD_EN
                    clock_set_lowest_voltage(SYSVDD_VOL_SEL_105V);	//进入ANC时提高SYSVDD电压
#endif/*ANC_MODE_SYSVDD_EN*/
                    anc_ret = audio_anc_run(&anc_hdl->param);
                    if (anc_ret == 0) {
                        anc_hdl->state = ANC_STA_OPEN;
                    } else {
                        /*
                         *-EPERM(-1):不支持ANC(非差分MIC，或混合馈使用4M的flash)
                         *-EINVAL(-22):参数错误(anc_coeff.bin参数为空, 或不匹配)
                         */
                        user_anc_log("audio_anc open Failed:%d\n", anc_ret);
                        anc_hdl->mode_switch_lock = 0;
                        anc_hdl->state = ANC_STA_INIT;
                        anc_hdl->param.mode = ANC_OFF;
                        anc_hdl->new_mode = anc_hdl->param.mode;
                        audio_mic_pwr_ctl(MIC_PWR_OFF);
                        break;
                    }
                } else {
                    audio_anc_run(&anc_hdl->param);
                    if (cur_anc_mode == ANC_OFF) {
                        anc_hdl->state = ANC_STA_INIT;
                        anc_dmic_io_init(&anc_hdl->param, 0);
                        audio_mic_pwr_ctl(MIC_PWR_OFF);
                    }
                }
                if (cur_anc_mode == ANC_OFF) {
#if ANC_MODE_SYSVDD_EN
                    clock_set_lowest_voltage(SYSVDD_VOL_SEL_084V);	//退出ANC恢复普通模式
#endif/*ANC_MODE_SYSVDD_EN*/
                    /*anc关闭，如果没有连接蓝牙，倒计时进入自动关机*/
                    /* extern u8 bt_get_total_connect_dev(void); */
                    /* if (bt_get_total_connect_dev() == 0) {    //已经没有设备连接 */
                    /* sys_auto_shut_down_enable(); */
                    /* } */
                }
#if TCFG_AUDIO_DYNAMIC_ADC_GAIN
                if (esco_player_runing() && (anc_hdl->state == ANC_STA_OPEN)) {
                    if (anc_hdl->mic_dy_state == ANC_MIC_DY_STA_START) {
                        audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
                    } else if (anc_hdl->mic_dy_state == ANC_MIC_DY_STA_INIT) {
                        anc_dynamic_micgain_start(app_var.aec_mic_gain);
                    }
                }
#endif/*TCFG_AUDIO_DYNAMIC_ADC_GAIN*/
                anc_hdl->mode_switch_lock = 0;

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
                if (speak_to_chat_flag) {
                    cur_anc_mode = ANC_OFF;
                    anc_hdl->param.mode = ANC_OFF;
                    speak_to_chat_flag = 0;
                }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if ANC_MUSIC_DYNAMIC_GAIN_EN
                audio_anc_music_dynamic_gain_reset();	/*音乐动态增益，切模式复位状态*/
#if ANC_MUSIC_DYNAMIC_GAIN_SINGLE_FB
                if (anc_hdl->param.mode == ANC_ON) {
                    //ANC初始化会复位FB GAIN，导致动态修改无效，在ANCmute前重新赋值
                    audio_anc_music_dynamic_fb_gain_reset();
                }
#endif
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/
                if ((anc_hdl->param.mode != ANC_OFF) && anc_hdl->param.anc_fade_en) {
                    os_time_dly(6);	//延时避免切模式反馈有哒哒声
                }
#if ANC_ADAPTIVE_EN
                audio_anc_power_adaptive_reset();
#endif
                anc_fade_in_timer_add(&anc_hdl->param);

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
                audio_icsd_adt_resume();
                audio_speak_to_chat_open_in_anc_done();
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if ANC_EAR_ADAPTIVE_EN
                if (anc_ear_adaptive_busy_get()) {
                    anc_ear_adaptive_mode_end();	//耳道自适应训练结束
                }
#endif

                anc_hdl->last_mode = cur_anc_mode;
#if ANC_MULT_ORDER_ENABLE
                audio_anc_mult_scene_coeff_free();
#endif/*ANC_MULT_ORDER_ENABLE*/

                /*anc on自动打开免摘，anc off自动关闭免摘*/
#if TCFG_AUDIO_SPEAK_TO_CHAT_ENABLE && SPEAK_TO_CHAT_AUTO_OPEN_IN_ANC
                /*anc on打开adt*/
                if (anc_hdl->param.mode == ANC_ON && !get_adt_switch_trans_state()) {
                    audio_speak_to_chat_open();
                } else if (!get_adt_switch_trans_state()) {
                    /*anc off关闭adt || 如果不是免摘切的trans关闭adt*/
                    audio_speak_to_chat_close();
                }
                set_adt_switch_trans_state(0);
#endif
                break;
            case ANC_MSG_MODE_SYNC:
                user_anc_log("anc_mode_sync:%d", msg[2]);
                anc_mode_switch(msg[2], 0);
                break;
#if TCFG_USER_TWS_ENABLE
            case ANC_MSG_TONE_SYNC:
                user_anc_log("anc_tone_sync_play:%d", msg[2]);
#if !ANC_MODE_EN_MODE_NEXT_SW
                if (anc_hdl->mode_switch_lock) {
                    user_anc_log("anc mode switch lock\n");
                    break;
                }
                anc_hdl->mode_switch_lock = 1;
#endif/*ANC_MODE_EN_MODE_NEXT_SW*/
                if (msg[2] == SYNC_TONE_ANC_OFF) {
                    anc_hdl->param.mode = ANC_OFF;
                } else if (msg[2] == SYNC_TONE_ANC_ON) {
                    anc_hdl->param.mode = ANC_ON;
                } else {
                    anc_hdl->param.mode = ANC_TRANSPARENCY;
                }
                if (anc_hdl->suspend) {
                    anc_hdl->param.tool_enablebit = 0;
                }
                anc_hdl->new_mode = anc_hdl->param.mode;	//确保准备切的新模式，与最后一个提示音播放的模式一致
                anc_tone_play_and_mode_switch(anc_hdl->param.mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
                //anc_mode_switch_deal(anc_hdl->param.mode);
                anc_hdl->sync_busy = 0;
                break;
#endif/*TCFG_USER_TWS_ENABLE*/
            case ANC_MSG_FADE_END:
                break;
            case ANC_MSG_DRC_TIMER:
                audio_anc_drc_process();
                break;
            case ANC_MSG_DEBUG_OUTPUT:
                audio_anc_debug_data_output();
                break;
#if ANC_ADAPTIVE_EN
            case ANC_MSG_ADAPTIVE_SYNC:
                audio_anc_power_adaptive_tone_play((u8)msg[2], (u8)msg[3]);
                break;
#endif/*ANC_ADAPTIVE_EN*/

#if ANC_MUSIC_DYNAMIC_GAIN_EN
            case ANC_MSG_MUSIC_DYN_GAIN:
                audio_anc_music_dynamic_gain_process();
                break;
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/
#if TCFG_AUDIO_FIT_DET_ENABLE
            case ANC_MSG_DOT:
                printf("ANC_MSG_DOT\n");
                icsd_dot_anctask_handle((u8)msg[2]);
                break;
#endif /*TCFG_AUDIO_FIT_DET_ENABLE*/

#if ANC_EAR_ADAPTIVE_EN
            case ANC_MSG_ICSD_ANC_V2_CMD:
            case ANC_MSG_ICSD_ANC_V2_INIT:
            case ANC_MSG_ICSD_ANC_CMD:
            case ANC_MSG_USER_TRAIN_INIT:
            case ANC_MSG_USER_TRAIN_RUN:
            case ANC_MSG_USER_TRAIN_SETPARAM:
            case ANC_MSG_USER_TRAIN_TIMEOUT:
            case ANC_MSG_USER_TRAIN_END:
                anc_ear_adaptive_cmd_handler(&anc_hdl->param, msg);
                break;
#endif/*ANC_EAR_ADAPTIVE_EN*/

            case ANC_MSG_MIC_DATA_GET:
                /* extern void anc_spp_mic_data_get(audio_anc_t *param); */
                /* anc_spp_mic_data_get(&anc_hdl->param); */
                break;
            case ANC_MSG_RESET:
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
                adt_flag = audio_icsd_adt_is_running();
                if (adt_flag) {	////免摘挂起
                    audio_icsd_adt_suspend();
                }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
                if (msg[2] && anc_hdl->param.anc_fade_en) {
                    int alogm = (anc_hdl->param.mode == ANC_TRANSPARENCY) ? anc_hdl->param.gains.trans_alogm : anc_hdl->param.gains.alogm;
                    int dly = audio_anc_fade_dly_get(anc_hdl->param.anc_fade_gain, alogm);
                    audio_anc_fade_ctr_set(ANC_FADE_MODE_RESET, AUDIO_ANC_FDAE_CH_ALL, 0);
                    os_time_dly(dly / 10 + 1);
                    audio_anc_reset(&anc_hdl->param, 1);
                    //避免param.anc_fade_gain 被修改，这里使用固定值
                    audio_anc_fade_ctr_set(ANC_FADE_MODE_RESET, AUDIO_ANC_FDAE_CH_ALL, AUDIO_ANC_FADE_GAIN_DEFAULT);
                } else {
                    audio_anc_reset(&anc_hdl->param, 0);
                }

#if ANC_HOWLING_DETECT_EN
                //复位啸叫检测相关变量
                anc_howling_detect_reset();
#endif

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
                if (adt_flag) { //免摘恢复, 更新免摘参数coeff gain
                    set_icsd_adt_param_updata();
                    audio_icsd_adt_resume();
                }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
#if ANC_MULT_ORDER_ENABLE
                audio_anc_mult_scene_coeff_free();		//释放空间
#endif/*ANC_MULT_ORDER_ENABLE*/
                break;
#if TCFG_ANC_SELF_DUT_GET_SZ
            case ANC_MSG_SZ_FFT_OPEN:
                clock_refurbish();	//模块时钟消耗96M
                anc_hdl->param.mode = ANC_TRAIN;
                anc_hdl->new_mode = anc_hdl->param.mode;
                anc_hdl->param.test_type = ANC_TEST_TYPE_FFT;	//设置当前的测试类型
                anc_hdl->param.sz_fft.alogm = ANC_ALOGM3;		//设置当前测试模式采样率
                anc_hdl->param.sz_fft.data_sel = msg[2];		//传入目标数据通道
                anc_hdl->param.sz_fft.check_flag = 1;			//检查DAC有效数据段
                audio_anc_train(&anc_hdl->param, 1);			//打开训练模式
                dac_node_write_callback_add("ANC_SZ_FFT", 0XFF, audio_anc_sz_fft_dac_check_slience_cb);	//注册DAC回调接口-静音检测
                break;
            case ANC_MSG_SZ_FFT_RUN:
                audio_anc_sz_fft_run(&anc_hdl->param);			//SZ_FFT 频响计算主体
                break;
            case ANC_MSG_SZ_FFT_CLOSE:
                audio_anc_sz_fft_stop(&anc_hdl->param);			//SZ_FFT 停止计算
                anc_train_close();								//关闭ANC，模式设置为ANC_OFF
                break;
#endif/*TCFG_ANC_SELF_DUT_GET_SZ*/
            case ANC_MSG_MODE_SWITCH_IN_ANCTASK:
                user_anc_log("anc_mode_in_anctask:%d, %d", msg[2], msg[3]);
                anc_mode_switch(msg[2], msg[3]);
                break;
            }
        } else {
            user_anc_log("res:%d,%d", res, msg[1]);
        }
    }
}

void anc_param_gain_t_printf(u8 cmd)
{
    char *str[2] = {
        "ANC_CFG_READ",
        "ANC_CFG_WRITE"
    };
    anc_gain_param_t *gains = &anc_hdl->param.gains;
    user_anc_log("-------------anc_param anc_gain_t-%s--------------\n", str[cmd - 1]);
    user_anc_log("developer_mode %d\n",    gains->developer_mode);
    user_anc_log("dac_gain %d\n", gains->dac_gain);
    user_anc_log("l_ffmic_gain %d\n", gains->l_ffmic_gain);
    user_anc_log("l_fbmic_gain %d\n", gains->l_fbmic_gain);
    user_anc_log("r_ffmic_gain %d\n", gains->r_ffmic_gain);
    user_anc_log("r_fbmic_gain %d\n", gains->r_fbmic_gain);
    user_anc_log("cmp_en %d\n",		  gains->cmp_en);
    user_anc_log("drc_en %d\n",  	  gains->drc_en);
    user_anc_log("ahs_en %d\n",		  gains->ahs_en);
    user_anc_log("ff_1st_dcc %d\n",	  gains->ff_1st_dcc);
    user_anc_log("fb_1st_dcc %d\n",	  gains->fb_1st_dcc);
    user_anc_log("ff_2nd_dcc %d\n",	  gains->ff_2nd_dcc);
    user_anc_log("fb_2nd_dcc %d\n",	  gains->fb_2nd_dcc);
    user_anc_log("drc_ff_2dcc %d\n",  gains->drc_ff_2dcc);
    user_anc_log("drc_fb_2dcc %d\n",  gains->drc_fb_2dcc);
    user_anc_log("drc_dcc_det_time %d\n",  gains->drc_dcc_det_time);
    user_anc_log("drc_dcc_res_time %d\n",  gains->drc_dcc_res_time);

    user_anc_log("gain_sign 0X%x\n",  gains->gain_sign);
    user_anc_log("noise_lvl 0X%x\n",  gains->noise_lvl);
    user_anc_log("fade_step 0X%x\n",  gains->fade_step);
    user_anc_log("alogm %d\n",		  gains->alogm);
    user_anc_log("trans_alogm %d\n",  gains->trans_alogm);

    user_anc_log("ancl_ffgain %d.%d\n", ((int)(gains->l_ffgain * 100.0)) / 100, ((int)(gains->l_ffgain * 100.0)) % 100);
    user_anc_log("ancl_fbgain %d.%d\n", ((int)(gains->l_fbgain * 100.0)) / 100, ((int)(gains->l_fbgain * 100.0)) % 100);
    user_anc_log("ancl_trans_gain %d.%d\n", ((int)(gains->l_transgain * 100.0)) / 100, ((int)(gains->l_transgain * 100.0)) % 100);
    user_anc_log("ancl_cmpgain %d.%d\n", ((int)(gains->l_cmpgain * 100.0)) / 100, ((int)(gains->l_cmpgain * 100.0)) % 100);
    user_anc_log("ancr_ffgain %d.%d\n", ((int)(gains->r_ffgain * 100.0)) / 100, ((int)(gains->r_ffgain * 100.0)) % 100);
    user_anc_log("ancr_fbgain %d.%d\n", ((int)(gains->r_fbgain * 100.0)) / 100, ((int)(gains->r_fbgain * 100.0)) % 100);
    user_anc_log("ancr_trans_gain %d.%d\n", ((int)(gains->r_transgain * 100.0)) / 100, ((int)(gains->r_transgain * 100.0)) % 100);
    user_anc_log("ancr_cmpgain %d.%d\n", ((int)(gains->r_cmpgain * 100.0)) / 100, ((int)(gains->r_cmpgain * 100.0)) % 100);

    user_anc_log("drcff_zero_det %d\n",    gains->drcff_zero_det);
    user_anc_log("drcff_dat_mode %d\n",    gains->drcff_dat_mode);
    user_anc_log("drcff_lpf_sel %d\n",     gains->drcff_lpf_sel);
    user_anc_log("drcfb_zero_det %d\n",    gains->drcfb_zero_det);
    user_anc_log("drcfb_dat_mode %d\n",    gains->drcfb_dat_mode);
    user_anc_log("drcfb_lpf_sel %d\n",     gains->drcfb_lpf_sel);

    user_anc_log("drcff_low_thr %d\n",     gains->drcff_lthr);
    user_anc_log("drcff_high_thr %d\n",    gains->drcff_hthr);
    user_anc_log("drcff_low_gain %d\n",    gains->drcff_lgain);
    user_anc_log("drcff_high_gain %d\n",   gains->drcff_hgain);
    user_anc_log("drcff_nor_gain %d\n",    gains->drcff_norgain);

    user_anc_log("drcfb_low_thr %d\n",     gains->drcfb_lthr);
    user_anc_log("drcfb_high_thr %d\n",    gains->drcfb_hthr);
    user_anc_log("drcfb_low_gain %d\n",    gains->drcfb_lgain);
    user_anc_log("drcfb_high_gain %d\n",   gains->drcfb_hgain);
    user_anc_log("drcfb_nor_gain %d\n",    gains->drcfb_norgain);

    user_anc_log("drctrans_low_thr %d\n",     gains->drctrans_lthr);
    user_anc_log("drctrans_high_thr %d\n",    gains->drctrans_hthr);
    user_anc_log("drctrans_low_gain %d\n",    gains->drctrans_lgain);
    user_anc_log("drctrans_high_gain %d\n",   gains->drctrans_hgain);
    user_anc_log("drctrans_nor_gain %d\n",    gains->drctrans_norgain);

    user_anc_log("ahs_dly %d\n",     gains->ahs_dly);
    user_anc_log("ahs_tap %d\n",     gains->ahs_tap);
    user_anc_log("ahs_wn_shift %d\n", gains->ahs_wn_shift);
    user_anc_log("ahs_wn_sub %d\n",  gains->ahs_wn_sub);
    user_anc_log("ahs_shift %d\n",   gains->ahs_shift);
    user_anc_log("ahs_u %d\n",       gains->ahs_u);
    user_anc_log("ahs_gain %d\n",    gains->ahs_gain);
    user_anc_log("ahs_nlms_sel %d\n",    gains->ahs_nlms_sel);

    user_anc_log("audio_drc_thr %d/10\n", (int)(gains->audio_drc_thr * 10.0));

    user_anc_log("adaptive_ref_en %d\n", gains->adaptive_ref_en);
    user_anc_log("adaptive_ref_fb_f %d/10\n", (int)(gains->adaptive_ref_fb_f * 10.0));
    user_anc_log("adaptive_ref_fb_g %d/10\n", (int)(gains->adaptive_ref_fb_g * 10.0));
    user_anc_log("adaptive_ref_fb_q %d/10\n", (int)(gains->adaptive_ref_fb_q * 10.0));

}

/*ANC配置参数填充*/
void anc_param_fill(u8 cmd, anc_gain_t *cfg)
{
    if (cmd == ANC_CFG_READ) {
        anc_gain_t *db_gain = (anc_gain_t *)anc_db_get(ANC_DB_GAIN, &anc_hdl->param.gains_size);
        if (db_gain) {
            cfg->gains = db_gain->gains;	//直接读flash区域
        } else {
            cfg->gains = anc_hdl->param.gains;	//flash区域为空，则使用默认参数
        }
        /* cfg->gains = anc_hdl->param.gains; */
    } else if (cmd == ANC_CFG_WRITE) {
        anc_hdl->param.gains = cfg->gains;
        anc_hdl->param.gains.hd_en = ANC_HOWLING_DETECT_EN;
        anc_hdl->param.gains.hd_corr_thr = ANC_HOWLING_DETECT_CORR_THR;
        anc_hdl->param.gains.hd_corr_dly = 13;
        anc_hdl->param.gains.hd_corr_gain = ANC_HOWLING_TARGET_GAIN;
        anc_hdl->param.gains.hd_pwr_rate = 2;
        anc_hdl->param.gains.hd_pwr_ctl_gain_en = 0;
        anc_hdl->param.gains.hd_pwr_ctl_ahsrst_en = 0;
        anc_hdl->param.gains.hd_pwr_thr = ANC_HOWLING_DETECT_PWR_THR;
        anc_hdl->param.gains.hd_pwr_ctl_gain = 1638;

        anc_hdl->param.gains.hd_pwr_ref_ctl_en = 0;
        anc_hdl->param.gains.hd_pwr_ref_ctl_hthr = 2000;
        anc_hdl->param.gains.hd_pwr_ref_ctl_lthr1 = 1000;
        anc_hdl->param.gains.hd_pwr_ref_ctl_lthr2 = 200;

        anc_hdl->param.gains.hd_pwr_err_ctl_en = 0;
        anc_hdl->param.gains.hd_pwr_err_ctl_hthr = 2000;
        anc_hdl->param.gains.hd_pwr_err_ctl_lthr1 = 1000;
        anc_hdl->param.gains.hd_pwr_err_ctl_lthr2 = 200;
        anc_hdl->param.gains.developer_mode = ANC_DEVELOPER_MODE_EN;

        audio_anc_param_normalize(&anc_hdl->param);

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        if (anc_hdl->param.gains.trans_alogm != anc_hdl->param.gains.alogm) {
            user_anc_log("===========================\n\n");
            // anc_hdl->param.gains.trans_alogm = anc_hdl->param.gains.alogm;
            user_anc_log("err : anc_hdl->param.gains.trans_alogm != anc_hdl->param.gains.alogm !!!");
            user_anc_log("\n\n===========================");
        }
#endif

#if ANC_EAR_ADAPTIVE_EN
        //左右参数映射，测试使用
        audio_anc_param_map(0, 1);
        if (anc_hdl->param.anc_coeff_mode == ANC_COEFF_MODE_ADAPTIVE) {
            audio_anc_ear_adaptive_param_init(&anc_hdl->param);
        }

#endif/*ANC_EAR_ADAPTIVE_EN*/
    }
    /* anc_param_gain_t_printf(cmd); */
}

//ANC配置文件读取并更新
int audio_anc_db_cfg_read(void)
{
    /*读取ANC增益配置*/
    anc_gain_t *db_gain = (anc_gain_t *)anc_db_get(ANC_DB_GAIN, &anc_hdl->param.gains_size);
    if (db_gain) {
        user_anc_log("anc_gain_db get succ,len:%d\n", anc_hdl->param.gains_size);
        if (audio_anc_gains_version_verify(&anc_hdl->param, db_gain)) {	//检查版本差异
            user_anc_log("The anc gains version is older,0X%x\n", db_gain->gains.version);
            db_gain = (anc_gain_t *)anc_db_get(ANC_DB_GAIN, &anc_hdl->param.gains_size);
        }
        anc_param_fill(ANC_CFG_WRITE, db_gain);
    } else {
        user_anc_log("anc_gain_db get failed");
        /* anc_param_gain_t_printf(ANC_CFG_READ); */
        return 1;
    }
    /*读取ANC滤波器系数*/
#if ANC_MULT_ORDER_ENABLE
    return audio_anc_mult_coeff_file_read();
#endif/*ANC_MULT_ORDER_ENABLE*/

    anc_coeff_t *db_coeff = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    if (!anc_coeff_fill(db_coeff)) {
        user_anc_log("anc_coeff_db get succ,len:%d", anc_hdl->param.coeff_size);
    } else {
        user_anc_log("anc_coeff_db get failed");
        return 1;
#if 0	//测试数据写入是否正常，或写入自己想要的滤波器数据
        extern anc_coeff_t *anc_cfg_test(audio_anc_t *param, u8 coeff_en, u8 gains_en);
        anc_coeff_t *test_coeff = anc_cfg_test(&anc_hdl->param, 1, 1);
        if (test_coeff) {
            int ret = anc_db_put(&anc_hdl->param, NULL, test_coeff);
            if (ret == 0) {
                user_anc_log("anc_db put test succ\n");
            } else {
                user_anc_log("anc_db put test err\n");
            }
        }
        anc_coeff_t *test_coeff1 = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
        anc_coeff_fill(test_coeff1);
#endif
    }
#if ANC_EAR_ADAPTIVE_EN
    audio_anc_param_map(1, 0);
#endif/*ANC_EAR_ADAPTIVE_EN*/
    return 0;

}

/*ANC初始化*/
void anc_init(void)
{
    anc_hdl = zalloc(sizeof(anc_t));
    user_anc_log("anc_hdl size:%d\n", (int)sizeof(anc_t));
    ASSERT(anc_hdl);
    audio_anc_param_init(&anc_hdl->param);
    anc_debug_init(&anc_hdl->param);
    audio_anc_fade_ctr_init();
#if TCFG_ANC_TOOL_DEBUG_ONLINE
    app_online_db_register_handle(DB_PKT_TYPE_ANC, anc_app_online_parse);
#endif/*TCFG_ANC_TOOL_DEBUG_ONLINE*/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    anc_hdl->param.adt = zalloc(sizeof(anc_adt_param_t));
    ASSERT(anc_hdl->param.adt);
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
#if TCFG_AUDIO_FIT_DET_ENABLE
    anc_hdl->param.dot = zalloc(sizeof(anc_dot_param_t));
    ASSERT(anc_hdl->param.dot);
#endif/*TCFG_AUDIO_FIT_DET_ENABLE*/
#if ANC_EAR_ADAPTIVE_EN
    anc_ear_adaptive_init(&anc_hdl->param);
#endif/*ANC_EAR_ADAPTIVE_EN*/
#if TCFG_AUDIO_ANC_EXT_TOOL_ENABLE
    anc_ext_tool_init();
#endif
#if ANC_MULT_ORDER_ENABLE
    anc_mult_init(&anc_hdl->param);
    anc_hdl->scene_id = ANC_MULT_ORDER_NORMAL_ID;	//默认使用场景1滤波器
    anc_hdl->param.mult_gain_set = audio_anc_mult_gains_id_set;
#endif/*ANC_MULT_ORDER_ENABLE*/

#if ANC_MUSIC_DYNAMIC_GAIN_EN
    audio_anc_music_dynamic_gain_init();
#endif/*ANC_MUSIC_DYNAMIC_GAIN_EN*/

    anc_hdl->param.post_msg_drc = audio_anc_post_msg_drc;
    anc_hdl->param.post_msg_debug = audio_anc_post_msg_debug;
#if TCFG_ANC_MODE_ANC_EN
    anc_hdl->mode_enable |= ANC_ON_BIT;
#endif/*TCFG_ANC_MODE_ANC_EN*/
#if TCFG_ANC_MODE_TRANS_EN
    anc_hdl->mode_enable |= ANC_TRANS_BIT;
#endif/*TCFG_ANC_MODE_TRANS_EN*/
#if TCFG_ANC_MODE_OFF_EN
    anc_hdl->mode_enable |= ANC_OFF_BIT;
#endif/*TCFG_ANC_MODE_OFF_EN*/
    anc_mode_enable_set(anc_hdl->mode_enable);
    anc_hdl->param.cfg_online_deal_cb = anc_cfg_online_deal;
    anc_hdl->param.mode = ANC_OFF;
    anc_hdl->new_mode = anc_hdl->param.mode;
    anc_hdl->param.production_mode = 0;
    anc_hdl->param.developer_mode = ANC_DEVELOPER_MODE_EN;
    anc_hdl->param.anc_fade_en = ANC_FADE_EN;/*ANC淡入淡出，默认开*/
    anc_hdl->param.anc_fade_gain = 16384;/*ANC淡入淡出增益,16384 (0dB) max:32767*/
    anc_hdl->param.tool_enablebit = TCFG_AUDIO_ANC_TRAIN_MODE;
    anc_hdl->param.online_busy = 0;		//在线调试繁忙标志位
    //36
    anc_hdl->param.dut_audio_enablebit = ANC_DUT_CLOSE;
    anc_hdl->param.fade_time_lvl = ANC_MODE_FADE_LVL;
    anc_hdl->param.enablebit = TCFG_AUDIO_ANC_TRAIN_MODE;
    anc_hdl->param.ch = TCFG_AUDIO_ANC_CH;
    anc_hdl->param.mic_type[0] = (ANC_CONFIG_LFF_EN) ? TCFG_AUDIO_ANCL_FF_MIC : MIC_NULL;
    anc_hdl->param.mic_type[1] = (ANC_CONFIG_LFB_EN) ? TCFG_AUDIO_ANCL_FB_MIC : MIC_NULL;
    anc_hdl->param.mic_type[2] = (ANC_CONFIG_RFF_EN) ? TCFG_AUDIO_ANCR_FF_MIC : MIC_NULL;
    anc_hdl->param.mic_type[3] = (ANC_CONFIG_RFB_EN) ? TCFG_AUDIO_ANCR_FB_MIC : MIC_NULL;

    anc_hdl->param.lff_en = ANC_CONFIG_LFF_EN;
    anc_hdl->param.lfb_en = ANC_CONFIG_LFB_EN;
    anc_hdl->param.rff_en = ANC_CONFIG_RFF_EN;
    anc_hdl->param.rfb_en = ANC_CONFIG_RFB_EN;
    anc_hdl->param.debug_sel = 0;

    //对相关控制变量做初始化
    anc_hdl->param.howling_detect_toggle = ANC_HOWLING_DETECT_EN;

    anc_hdl->param.gains.version = ANC_GAINS_VERSION;
    anc_hdl->param.gains.dac_gain = 3;
    anc_hdl->param.gains.l_ffmic_gain = 4;
    anc_hdl->param.gains.l_fbmic_gain = 4;
    anc_hdl->param.gains.r_ffmic_gain = 4;
    anc_hdl->param.gains.r_fbmic_gain = 4;
    anc_hdl->param.gains.cmp_en = 1;
    anc_hdl->param.gains.drc_en = 0;
    anc_hdl->param.gains.ahs_en = 1;
    anc_hdl->param.gains.ff_1st_dcc = 1;
    anc_hdl->param.gains.fb_1st_dcc = 1;
    anc_hdl->param.gains.ff_2nd_dcc = 4;
    anc_hdl->param.gains.fb_2nd_dcc = 4;
    anc_hdl->param.gains.gain_sign = 0;
    anc_hdl->param.gains.alogm = ANC_ALOGM6;
    anc_hdl->param.gains.trans_alogm = ANC_ALOGM4;
    anc_hdl->param.gains.l_ffgain = 1.0f;
    anc_hdl->param.gains.l_fbgain = 1.0f;
    anc_hdl->param.gains.l_cmpgain = 1.0f;
    anc_hdl->param.gains.l_transgain = 1.0f;
    anc_hdl->param.gains.r_ffgain = 1.0f;
    anc_hdl->param.gains.r_fbgain = 1.0f;
    anc_hdl->param.gains.r_cmpgain = 1.0f;
    anc_hdl->param.gains.r_transgain = 1.0f;

    anc_hdl->param.gains.drcff_zero_det = 0;
    anc_hdl->param.gains.drcff_dat_mode = 0;//hbit control l_gain, lbit control h_gain
    anc_hdl->param.gains.drcff_lpf_sel  = 0;
    anc_hdl->param.gains.drcfb_zero_det = 0;
    anc_hdl->param.gains.drcfb_dat_mode = 0;//hbit control l_gain, lbit control h_gain
    anc_hdl->param.gains.drcfb_lpf_sel  = 0;

    anc_hdl->param.gains.drcff_lthr = 0;
    anc_hdl->param.gains.drcff_hthr = 6000 ;
    anc_hdl->param.gains.drcff_lgain = 0;
    anc_hdl->param.gains.drcff_hgain = 512;
    anc_hdl->param.gains.drcff_norgain = 1024 ;

    anc_hdl->param.gains.drctrans_lthr = 0;
    anc_hdl->param.gains.drctrans_hthr = 32767 ;
    anc_hdl->param.gains.drctrans_lgain = 0;
    anc_hdl->param.gains.drctrans_hgain = 1024;
    anc_hdl->param.gains.drctrans_norgain = 1024 ;

    anc_hdl->param.gains.drcfb_lthr = 0;
    anc_hdl->param.gains.drcfb_hthr = 6000;
    anc_hdl->param.gains.drcfb_lgain = 0;
    anc_hdl->param.gains.drcfb_hgain = 512;
    anc_hdl->param.gains.drcfb_norgain = 1024;

    anc_hdl->param.gains.ahs_dly = 1;
    anc_hdl->param.gains.ahs_tap = 100;
    anc_hdl->param.gains.ahs_wn_shift = 9;
    anc_hdl->param.gains.ahs_wn_sub = 1;
    anc_hdl->param.gains.ahs_shift = 210;
    anc_hdl->param.gains.ahs_u = 4000;
    anc_hdl->param.gains.ahs_gain = -1024;
    anc_hdl->param.gains.audio_drc_thr = -6.0f;

    anc_hdl->param.gains.ahs_nlms_sel = 0;

    anc_hdl->param.gains.drc_ff_2dcc = 0;
    anc_hdl->param.gains.drc_fb_2dcc = 0;
    anc_hdl->param.gains.drc_dcc_det_time = 300;
    anc_hdl->param.gains.drc_dcc_res_time = 20;
    anc_hdl->param.gains.hd_en = 0;
    anc_hdl->param.gains.hd_corr_thr = 232;
    anc_hdl->param.gains.hd_corr_gain = 256;
    anc_hdl->param.gains.hd_corr_dly = 13;
    anc_hdl->param.gains.hd_pwr_rate = 2;
    anc_hdl->param.gains.hd_pwr_ctl_gain_en = 0;
    anc_hdl->param.gains.hd_pwr_ctl_ahsrst_en = 0;
    anc_hdl->param.gains.hd_pwr_thr = 18000;
    anc_hdl->param.gains.hd_pwr_ctl_gain = 1638;

    anc_hdl->param.gains.hd_pwr_ref_ctl_en = 0;
    anc_hdl->param.gains.hd_pwr_ref_ctl_hthr = 2000;
    anc_hdl->param.gains.hd_pwr_ref_ctl_lthr1 = 1000;
    anc_hdl->param.gains.hd_pwr_ref_ctl_lthr2 = 200;

    anc_hdl->param.gains.hd_pwr_err_ctl_en = 0;
    anc_hdl->param.gains.hd_pwr_err_ctl_hthr = 2000;
    anc_hdl->param.gains.hd_pwr_err_ctl_lthr1 = 1000;
    anc_hdl->param.gains.hd_pwr_err_ctl_lthr2 = 200;

    anc_hdl->param.gains.adaptive_ref_en = 0;
    anc_hdl->param.gains.adaptive_ref_fb_f = 120.0;
    anc_hdl->param.gains.adaptive_ref_fb_g = 15.0;
    anc_hdl->param.gains.adaptive_ref_fb_q = 0.4;
#if ANC_COEFF_SAVE_ENABLE
    anc_db_init();
    audio_anc_db_cfg_read();
#endif/*ANC_COEFF_SAVE_ENABLE*/

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    anc_hdl->param.adt->dma_done_cb = icsd_adt_dma_done;
    anc_hdl->param.ltrans_fbgain = anc_trans_lfb_gain;
    anc_hdl->param.rtrans_fbgain = anc_trans_rfb_gain;
    anc_hdl->param.ltrans_fb_coeff = (double *)anc_trans_lfb_coeff;
    anc_hdl->param.rtrans_fb_coeff = (double *)anc_trans_rfb_coeff;

    anc_hdl->param.ltrans_fb_yorder = sizeof(anc_trans_lfb_coeff) / 40;
    anc_hdl->param.rtrans_fb_yorder = sizeof(anc_trans_rfb_coeff) / 40;
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_AUDIO_FIT_DET_ENABLE
    anc_hdl->param.dot->dma_done_cb = audio_icsd_dot_anc_dma_done;
#endif/*TCFG_AUDIO_FIT_DET_ENABLE*/

    anc_hdl->param.biquad2ab = audio_icsd_biquad2ab_out;

#if ANC_EAR_ADAPTIVE_EN
    //2.再读取ANC自适应参数, 若存在则覆盖默认参数
    audio_anc_adaptive_data_read();
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if ANC_HOWLING_DETECT_EN
    anc_hdl->param.howling_detect_ch = ANC_HOWLING_DETECT_CHANNEL;
    struct anc_howling_detect_cfg howling_detect_cfg;
    howling_detect_cfg.detect_time = ANC_HOWLING_DETECT_TIME;
    howling_detect_cfg.hold_time = ANC_HOWLING_HOLD_TIME;
    howling_detect_cfg.resume_time = ANC_HOWLING_RESUME_TIME;
    howling_detect_cfg.param = &anc_hdl->param;
    howling_detect_cfg.fade_gain_set = audio_anc_howldet_fade_set;
    anc_howling_detect_init(&howling_detect_cfg);
#endif

    /* sys_timer_add(NULL, anc_timer_deal, 5000); */

//初始化ANC ADC相关参数，注意需在audio_adc_init后调用
    anc_hdl->param.adc_set_buffs_cb = audio_adc_fixed_digital_set_buffs;
    audio_anc_adc_ch_set();
    audio_anc_mic_management(&anc_hdl->param);

#if ANC_ADAPTIVE_EN
    audio_anc_power_adaptive_init(&anc_hdl->param);
#endif/*ANC_ADAPTIVE_EN*/

#if TCFG_ANC_SELF_DUT_GET_SZ
    anc_hdl->param.sz_fft.dma_run_hdl = audio_anc_post_msg_sz_fft_run;
#endif/*TCFG_ANC_SELF_DUT_GET_SZ*/

    audio_anc_max_yorder_verify(&anc_hdl->param);
    anc_mix_out_audio_drc_thr(anc_hdl->param.gains.audio_drc_thr);
    task_create(anc_task, NULL, "anc");
    anc_hdl->state = ANC_STA_INIT;

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    anc_adt_init();
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

    user_anc_log("anc_init ok");
}

void anc_train_open(u8 mode, u8 debug_sel)
{
    user_anc_log("ANC_Train_Open\n");
    local_irq_disable();
    if (anc_hdl && (anc_hdl->state == ANC_STA_INIT)) {
        /*防止重复打开训练模式*/
        if (anc_hdl->param.mode == ANC_TRAIN) {
            local_irq_enable();
            return;
        }
        /*anc工作，退出sniff*/
        bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
        /*anc工作，关闭自动关机*/
        /* sys_auto_shut_down_disable(); */

        anc_hdl->param.debug_sel = debug_sel;
        anc_hdl->param.mode = mode;
        anc_hdl->new_mode = anc_hdl->param.mode;
        anc_hdl->param.test_type = ANC_TEST_TYPE_PCM;
        /*训练的时候，申请buf用来保存训练参数*/
        local_irq_enable();
        os_taskq_post_msg("anc", 1, ANC_MSG_TRAIN_OPEN);
        user_anc_log("%s ok\n", __FUNCTION__);
        return;
    }
    local_irq_enable();
}

void anc_train_close(void)
{
    if (anc_hdl == NULL) {
        return;
    }
    if (anc_hdl->param.mode == ANC_TRAIN) {
        anc_hdl->param.train_para.train_busy = 0;
        anc_hdl->param.mode = ANC_OFF;
        anc_hdl->new_mode = anc_hdl->param.mode;
        anc_hdl->state = ANC_STA_INIT;
        audio_anc_train(&anc_hdl->param, 0);
        user_anc_log("anc_train_close ok\n");
    }
}

/*查询当前ANC是否处于训练状态*/
int anc_train_open_query(void)
{
    if (anc_hdl) {
#if ANC_EAR_ADAPTIVE_EN
        if (anc_ear_adaptive_busy_get()) {
            return 1;
        }
#endif/*ANC_EAR_ADAPTIVE_EN*/
        if (anc_hdl->param.mode == ANC_TRAIN) {
            return 1;
        }
    }
    return 0;
}

extern void audio_dump();
static void anc_timer_deal(void *priv)
{
    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 8) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_gain = (JL_ADDA->ADA_CON8) & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON8 >> 5) & 0x1F;
    u8 mic2_gain = (JL_ADDA->ADA_CON8 >> 10) & 0x1F;
    u8 mic3_gain = (JL_ADDA->ADA_CON8 >> 15) & 0x1F;
    user_anc_log("MIC_G:%d,%d,%d,%d,DAC_AG:%d,%d,DAC_DG:%d,%d\n", mic0_gain, mic1_gain, mic2_gain, mic3_gain, dac_again_l, dac_again_r, dac_dgain_l, dac_dgain_r);
    /* mem_stats(); */
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        printf("%c, master\n", bt_tws_get_local_channel());
    } else {
        printf("%c, slave\n", bt_tws_get_local_channel());
    }
#endif/*TCFG_USER_TWS_ENABLE*/
    /* audio_dump(); */
}

static int anc_tone_play_cb(void *priv, enum stream_event event)
{
    u8 next_mode = anc_hdl->tone_mode;

#if ANC_EAR_ADAPTIVE_EN
    if ((event != STREAM_EVENT_START) && (event != STREAM_EVENT_STOP) && (event != STREAM_EVENT_INIT)) {
        if (anc_hdl->new_mode == next_mode && next_mode == ANC_ON && anc_hdl->ear_adaptive) {
            /*提示音被打断，退出自适应*/
            anc_ear_adaptive_tone_play_err_cb();
        }
    }
#endif /*ANC_EAR_ADAPTIVE_EN*/

    if (event != STREAM_EVENT_STOP) {
        return 0;
    }
    printf("anc_tone_play_cb,anc_mode:%d,%d,new_mode:%d\n", next_mode, anc_hdl->param.mode, anc_hdl->new_mode);
    /*
     *当最新的目标ANC模式和即将切过去的模式一致，才进行模式切换实现。
     *否则表示，模式切换操作（比如快速按键切模式）new_mode领先于ANC模式切换实现next_mode
     *则直接在切模式的时候，实现最新的目标模式
     */
    if (anc_hdl->new_mode == next_mode) {
#if ANC_EAR_ADAPTIVE_EN
        if (next_mode == ANC_ON && anc_hdl->ear_adaptive) {
            anc_hdl->ear_adaptive = 0;
            anc_ear_adaptive_tone_play_cb();
        } else
#endif/*ANC_EAR_ADAPTIVE_EN*/
        {
            anc_mode_switch_deal(next_mode);
        }
    } else {
        anc_hdl->mode_switch_lock = 0;
    }
    return 0;
}


static void tws_anc_tone_callback(int priv, enum stream_event event)
{
    r_printf("tws_anc_tone_callback: %d\n",  event);
    anc_tone_play_cb(NULL, event);
}

#define TWS_ANC_TONE_STUB_FUNC_UUID   0x1FE4698C
REGISTER_TWS_TONE_CALLBACK(tws_anc_tone_stub) = {
    .func_uuid  = TWS_ANC_TONE_STUB_FUNC_UUID,
    .callback   = tws_anc_tone_callback,
};

/*
*********************************************************************
*                  anc_play_tone
* Description: ANC提示音播放和模式切换
* Arguments  : file_name	提示音名字
*			   cb_sel	 	切模式方式选择
* Return	 : None.
* Note(s)    : 通过cb_sel选择是在播放提示音切模式，还是播提示音同时切
*			   模式
*********************************************************************
*/
static int anc_play_tone(const char *file_name, u8 cb_sel)
{
    int ret = 0;
    if (cb_sel) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                ret = tws_play_tone_file_callback(file_name, TWS_ANC_SYNC_TIMEOUT, TWS_ANC_TONE_STUB_FUNC_UUID);
            }
        } else {
            ret = tws_play_tone_file_callback(file_name, TWS_ANC_SYNC_TIMEOUT, TWS_ANC_TONE_STUB_FUNC_UUID);
        }
#else
        ret = play_tone_file_callback(file_name, NULL, anc_tone_play_cb);
#endif
    } else {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state()) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                ret = tws_play_tone_file(file_name, TWS_ANC_SYNC_TIMEOUT);
            }
        } else {
            ret = tws_play_tone_file(file_name, TWS_ANC_SYNC_TIMEOUT);
        }
#else
        ret = play_tone_file(file_name);
#endif
    }
    return ret;
}

/*
*********************************************************************
*                  Audio ANC Tone Play And Mode Switch
* Description: ANC提示音播放和模式切换
* Arguments  : index	    当前模式
*			   preemption	提示音抢断播放标识
*			   cb_sel	 	切模式方式选择
* Return	 : None.
* Note(s)    : 通过cb_sel选择是在播放提示音切模式，还是播提示音同时切
*			   模式
*********************************************************************
*/
static void anc_tone_play_and_mode_switch(u8 mode, u8 preemption, u8 cb_sel)
{
    /* if (!esco_player_runing() && !a2dp_player_runing()) {	//后台没有音频，提示音默认打断播放
        preemption = 1;
    } */
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    audio_anc_mode_switch_in_adt(anc_hdl->param.mode);
#endif
    if (cb_sel) {
        /*ANC打开情况下，播提示音的同时，anc效果淡出*/
        if (anc_hdl->state == ANC_STA_OPEN) {
            audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, 0);
        }
        if (mode == ANC_ON) {
            char *tone = (char *)get_tone_files()->anc_on;
#if ANC_EAR_ADAPTIVE_EN
            if (anc_hdl->ear_adaptive) {   //非ANC模式下切自适应，需要提前开MIC，ADC需要稳定时间
                anc_ear_adaptive_tone_play_start();
                tone = (char *)get_tone_files()->anc_adaptive;
            }
#endif/*ANC_EAR_ADAPTIVE_EN*/

            anc_play_tone(tone, cb_sel);
        } else if (mode == ANC_TRANSPARENCY) {
            anc_play_tone(get_tone_files()->anc_trans, cb_sel);
        } else if (mode == ANC_OFF) {
            anc_play_tone(get_tone_files()->anc_off, cb_sel);
        }
        anc_hdl->tone_mode = anc_hdl->param.mode;

    } else {
        if (mode == ANC_ON) {
            anc_play_tone(get_tone_files()->anc_on, cb_sel);
        } else if (mode == ANC_TRANSPARENCY) {
            anc_play_tone(get_tone_files()->anc_trans, cb_sel);
        } else if (mode == ANC_OFF) {
            anc_play_tone(get_tone_files()->anc_off, cb_sel);
        }
        anc_mode_switch_deal(anc_hdl->param.mode);
    }
}

static void anc_tone_stop(void)
{
    if (anc_hdl && anc_hdl->mode_switch_lock) {
        tone_player_stop();
    }
}

/*
*********************************************************************
*                  anc_fade_in_timer
* Description: ANC增益淡入函数
* Arguments  : None.
* Return	 : None.
* Note(s)    :通过定时器的控制，使ANC的淡入增益一点一点叠加
*********************************************************************
*/
static void anc_fade_in_timer(void *arg)
{
    anc_hdl->fade_gain += (anc_hdl->param.anc_fade_gain / anc_hdl->param.fade_time_lvl);
    if (anc_hdl->fade_gain > anc_hdl->param.anc_fade_gain) {
        anc_hdl->fade_gain = anc_hdl->param.anc_fade_gain;
        usr_timer_del(anc_hdl->fade_in_timer);
    }
    audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, anc_hdl->fade_gain);
}

static void anc_fade_in_timer_add(audio_anc_t *param)
{
    u32 alogm, dly;
    if (param->anc_fade_en) {
        alogm = (param->mode == ANC_TRANSPARENCY) ? param->gains.trans_alogm : param->gains.alogm;
        dly = audio_anc_fade_dly_get(param->anc_fade_gain, alogm);
        anc_hdl->fade_gain = 0;
        if (param->mode == ANC_ON) {		//可自定义模式，当前仅在ANC模式下使用
            user_anc_log("anc_fade_time_lvl  %d\n", param->fade_time_lvl);
            anc_hdl->fade_gain = (param->anc_fade_gain / param->fade_time_lvl);
            audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, anc_hdl->fade_gain);
            if (param->fade_time_lvl > 1) {
                anc_hdl->fade_in_timer = usr_timer_add((void *)0, anc_fade_in_timer, dly, 1);
            }
        } else if (param->mode != ANC_OFF) {
            audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, param->anc_fade_gain);
        }
    }
}
static void anc_fade_out_timeout(void *arg)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
}

/*ANC淡入淡出增益设置接口*/
void audio_anc_fade_gain_set(int gain)
{
    anc_hdl->param.anc_fade_gain = gain;
    if (anc_hdl->param.mode != ANC_OFF) {
        //用户层操作 ANC_FADE_MODE_SWITCH 的模式
        audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, gain);
    }
}
/*ANC淡入淡出增益接口*/
int audio_anc_fade_gain_get(void)
{
    return anc_hdl->param.anc_fade_gain;
}

static void anc_fade(u32 gain)
{
    u32	alogm, dly;
    if (anc_hdl) {
        alogm = (anc_hdl->last_mode == ANC_TRANSPARENCY) ? anc_hdl->param.gains.trans_alogm : anc_hdl->param.gains.alogm;
        dly = audio_anc_fade_dly_get(anc_hdl->param.anc_fade_gain, alogm);
        user_anc_log("anc_fade:%d,dly:%d", gain, dly);
        audio_anc_fade_ctr_set(ANC_FADE_MODE_SWITCH, AUDIO_ANC_FDAE_CH_ALL, gain);
        if (anc_hdl->param.anc_fade_en) {
            usr_timeout_del(anc_hdl->fade_in_timer);
            usr_timeout_add((void *)0, anc_fade_out_timeout, dly, 1);
        } else {/*不淡入淡出，则直接切模式*/
            os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
        }
    }
}

/*
 *mode:降噪/通透/关闭
 *tone_play:切换模式的时候，是否播放提示音
 */
void anc_mode_switch_deal(u8 mode)
{
    user_anc_log("anc switch,state:%s", anc_state_str[anc_hdl->state]);
    anc_hdl->mode_switch_lock = 1;
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    audio_anc_mode_switch_in_adt(anc_hdl->param.mode);
#endif
    if (anc_hdl->state == ANC_STA_OPEN) {
        user_anc_log("anc open now,switch mode:%d", mode);
        anc_fade(0);//切模式，先fade_out
    } else if (anc_hdl->state == ANC_STA_INIT) {
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        if ((anc_hdl->param.mode != ANC_OFF) || get_adt_open_in_anc_state()) {
#else
        if (anc_hdl->param.mode != ANC_OFF) {
#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
            /*anc工作，关闭自动关机*/
            /* sys_auto_shut_down_disable(); */
            os_taskq_post_msg("anc", 1, ANC_MSG_RUN);
        } else {
            user_anc_log("anc close now,new mode is ANC_OFF\n");
            anc_hdl->mode_switch_lock = 0;
        }
    } else {
        user_anc_log("anc state err:%d\n", anc_hdl->state);
        anc_hdl->mode_switch_lock = 0;
    }
}

void anc_gain_app_value_set(int app_value)
{
    static int anc_gain_app_value = -1;
    if (-1 == app_value && -1 != anc_gain_app_value) {
        /* anc_hdl->param.anc_ff_gain = anc_gain_app_value; */
    }
    anc_gain_app_value = app_value;
}

#if TCFG_USER_TWS_ENABLE
/*tws同步播放模式提示音，并且同步进入anc模式*/
void anc_tone_sync_play(int tone_name)
{
    if (anc_hdl) {
        user_anc_log("anc_tone_sync_play:%d", tone_name);
        os_taskq_post_msg("anc", 2, ANC_MSG_TONE_SYNC, tone_name);
    }
}

static void anc_play_tone_at_same_time_handler(int tone_name, int err)
{
    switch (tone_name) {
    case SYNC_TONE_ANC_ON:
    case SYNC_TONE_ANC_OFF:
    case SYNC_TONE_ANC_TRANS:
        anc_tone_sync_play(tone_name);
        break;
    }
}

TWS_SYNC_CALL_REGISTER(anc_tws_tone_play) = {
    .uuid = 0x123A9E53,
    .task_name = "app_core",
    .func = anc_play_tone_at_same_time_handler,
};

void anc_play_tone_at_same_time(int tone_name, int msec)
{
    tws_api_sync_call_by_uuid(0x123A9E53, tone_name, msec);
}
#endif/*TCFG_USER_TWS_ENABLE*/

void anc_mode_switch(u8 mode, u8 tone_play)
{
    if (anc_hdl == NULL) {
        return;
    }
    u8 ignore_same_mode = 0;
    u8 tws_sync_en = 1;
    /*模式切换超出范围*/
    if ((mode > (ANC_MODE_NULL - 1)) || (mode < ANC_OFF)) {
        user_anc_log("anc mode switch err:%d", mode);
        return;
    }
#if ((defined TCFG_AUDIO_FIT_DET_ENABLE) && TCFG_AUDIO_FIT_DET_ENABLE)
    //贴合度检测状态下不允许其他模式切换
    if (audio_icsd_dot_is_running()) {
        user_anc_log("audio_icsd_dot_is_running\n");
        return;
    }
#endif
    /*模式切换同一个*/
#if ANC_EAR_ADAPTIVE_EN
    if (anc_hdl->ear_adaptive) {
        ignore_same_mode = 1;
        tws_sync_en = anc_ear_adaptive_tws_sync_en_get();   //自适应状态关闭左右平衡，则不需要同步切模式
    }
#endif/*ANC_EAR_ADAPTIVE_EN*/
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    /*判断adt是否在关闭过程中切anc*/
    u8 adt_off_state = (audio_icsd_adt_is_running() && !get_icsd_adt_mode()) ? 1 : 0;
    /*需要在切anc模式是开adt || adt关闭时切anc off*/
    if ((get_adt_open_in_anc_state() && get_icsd_adt_mode()) || (adt_off_state && (mode == ANC_OFF))) {
        ignore_same_mode = 1;
    }
#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
    if (anc_hdl->param.mode == mode && (!ignore_same_mode)) {

        user_anc_log("anc mode switch err:same mode");
        return;
    }
#if ANC_MODE_EN_MODE_NEXT_SW
    if (anc_hdl->mode_switch_lock) {
        user_anc_log("anc mode switch lock\n");
        return;
    }
    anc_hdl->mode_switch_lock = 1;
#endif/*ANC_MODE_EN_MODE_NEXT_SW*/

    anc_hdl->new_mode = mode;/*记录最新的目标ANC模式*/
#if ANC_TONE_END_MODE_SW && (!ANC_MODE_EN_MODE_NEXT_SW)
    anc_tone_stop();
#endif/*ANC_TONE_END_MODE_SW*/
    anc_hdl->param.mode = mode;

    if (anc_hdl->suspend) {
        anc_hdl->param.tool_enablebit = 0;
    }

    printf("anc_mode_switch,tws_sync_en = %d\n", tws_sync_en);
    /* anc_gain_app_value_set(-1); */		//没有处理好bypass与ANC_ON的关系
    /*
     *ANC模式提示音播放规则
     *(1)根据应用选择是否播放提示音：tone_play
     *(2)tws连接的时候，主机发起模式提示音同步播放
     *(3)单机的时候，直接播放模式提示音
     */
    if (tone_play) {
#if TCFG_USER_TWS_ENABLE
        if (get_tws_sibling_connect_state() && tws_sync_en) {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                user_anc_log("[tws_master]anc_tone_sync_play");
                anc_hdl->sync_busy = 1;
                if (anc_hdl->new_mode == ANC_ON) {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_ON, TWS_ANC_SYNC_TIMEOUT);
                } else if (anc_hdl->new_mode == ANC_OFF) {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_OFF, TWS_ANC_SYNC_TIMEOUT);
                } else {
                    anc_play_tone_at_same_time(SYNC_TONE_ANC_TRANS, TWS_ANC_SYNC_TIMEOUT);
                }
            }
            return;
        } else {
            user_anc_log("anc_tone_play");
            anc_tone_play_and_mode_switch(mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
        }
#else
        anc_tone_play_and_mode_switch(mode, ANC_TONE_PREEMPTION, ANC_TONE_END_MODE_SW);
#endif/*TCFG_USER_TWS_ENABLE*/
    } else {
        anc_mode_switch_deal(mode);
    }
}

static void anc_ui_mode_sel_timer(void *priv)
{
    if (anc_hdl->ui_mode_sel && (anc_hdl->ui_mode_sel != anc_hdl->param.mode)) {
        /*
         *提示音不打断
         *tws提示音同步播放完成
         */
        /* if ((tone_get_status() == 0) && (anc_hdl->sync_busy == 0)) { */
        if (anc_hdl->sync_busy == 0) {
            user_anc_log("anc_ui_mode_sel_timer:%d,sync_busy:%d", anc_hdl->ui_mode_sel, anc_hdl->sync_busy);
            anc_mode_switch(anc_hdl->ui_mode_sel, 1);
            sys_timer_del(anc_hdl->ui_mode_sel_timer);
            anc_hdl->ui_mode_sel_timer = 0;
        }
    }
}

/*ANC通过ui菜单选择anc模式,处理快速切换的情景*/
void anc_ui_mode_sel(u8 mode, u8 tone_play)
{
    /*
     *timer存在表示上个模式还没有完成切换
     *提示音不打断
     *tws提示音同步播放完成
     */
    /* if ((anc_hdl->ui_mode_sel_timer == 0) && (tone_get_status() == 0) && (anc_hdl->sync_busy == 0)) { */
    if ((anc_hdl->ui_mode_sel_timer == 0) && (anc_hdl->sync_busy == 0)) {
        user_anc_log("anc_ui_mode_sel[ok]:%d", mode);
        anc_mode_switch(mode, tone_play);
    } else {
        user_anc_log("anc_ui_mode_sel[dly]:%d,timer:%d,sync_busy:%d", mode, anc_hdl->ui_mode_sel_timer, anc_hdl->sync_busy);
        anc_hdl->ui_mode_sel = mode;
        if (anc_hdl->ui_mode_sel_timer == 0) {
            anc_hdl->ui_mode_sel_timer = sys_timer_add(NULL, anc_ui_mode_sel_timer, 50);
        }
    }
}

/*ANC模式同步(tws模式)*/
void anc_mode_sync(u8 *data)
{
    if (anc_hdl) {
#if ANC_EAR_ADAPTIVE_EN
        anc_ear_adaptive_seq_set(data[1]);
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if ANC_MULT_ORDER_ENABLE
        anc_hdl->scene_id = data[2];
#endif/*ANC_MULT_ORDER_ENABLE*/
        os_taskq_post_msg("anc", 2, ANC_MSG_MODE_SYNC, data[0]);
    }
}

/*在anc任务里面切换anc模式，避免上一次切换没有完成，这次切换被忽略的情况*/
void anc_mode_switch_in_anctask(u8 mode, u8 tone_play)
{
    if (anc_hdl) {
        os_taskq_post_msg("anc", 3, ANC_MSG_MODE_SWITCH_IN_ANCTASK, mode, tone_play);
    }
}

/*ANC挂起*/
void anc_suspend(void)
{
    if (anc_hdl) {
        user_anc_log("anc_suspend\n");
        anc_hdl->suspend = 1;
        anc_hdl->param.tool_enablebit = 0;	//使能设置为0
        if (anc_hdl->param.anc_fade_en) {	  //挂起ANC增益淡出
            audio_anc_fade_ctr_set(ANC_FADE_MODE_SUSPEND, AUDIO_ANC_FDAE_CH_ALL, 0);
        }
        audio_anc_en_set(0);
    }
}

/*ANC恢复*/
void anc_resume(void)
{
    if (anc_hdl) {
        user_anc_log("anc_resume\n");
        anc_hdl->suspend = 0;
        anc_hdl->param.tool_enablebit = anc_hdl->param.enablebit;	//使能恢复
        /*处理anc off开adt后出仓调用这里把anc关闭导致dac没有声音的问题*/
#if (defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        u8 adt_flag = 0;
        if (get_icsd_adt_mode() && anc_hdl->param.mode == ANC_OFF) {
            adt_flag = 1;
            anc_hdl->param.mode = ANC_ON;
        }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
        audio_anc_en_set(1);
#if (defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
        if (adt_flag) {
            anc_hdl->param.mode = ANC_OFF;
        }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
        if (anc_hdl->param.anc_fade_en) {	  //恢复ANC增益淡入
            //避免param.anc_fade_gain 被修改，这里使用固定值
            audio_anc_fade_ctr_set(ANC_FADE_MODE_SUSPEND, AUDIO_ANC_FDAE_CH_ALL, AUDIO_ANC_FADE_GAIN_DEFAULT);
        }
    }
}

/*ANC信息保存*/
void anc_info_save()
{
    if (anc_hdl) {
        anc_info_t anc_info;
        int ret = syscfg_read(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret == sizeof(anc_info)) {
#if INEAR_ANC_UI
            if (anc_info.mode == anc_hdl->param.mode && anc_info.inear_tws_mode == inear_tws_ancmode) {
#else
            if (anc_info.mode == anc_hdl->param.mode) {
#endif/*INEAR_ANC_UI*/
                user_anc_log("anc info.mode == cur_anc_mode");
                return;
            }
        } else {
            user_anc_log("read anc_info err");
        }

        user_anc_log("save anc_info");
        anc_info.mode = anc_hdl->param.mode;
#if INEAR_ANC_UI
        anc_info.inear_tws_mode = inear_tws_ancmode;
#endif/*INEAR_ANC_UI*/
        ret = syscfg_write(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret != sizeof(anc_info)) {
            user_anc_log("anc info save err!\n");
        }

    }
}

/*系统上电的时候，根据配置决定是否进入上次的模式*/
void anc_poweron(void)
{
    if (anc_hdl) {
#if ANC_INFO_SAVE_ENABLE
        anc_info_t anc_info;
        int ret = syscfg_read(CFG_ANC_INFO, &anc_info, sizeof(anc_info));
        if (ret == sizeof(anc_info)) {
            user_anc_log("read anc_info succ,state:%s,mode:%s", anc_state_str[anc_hdl->state], anc_mode_str[anc_info.mode]);
#if INEAR_ANC_UI
            inear_tws_ancmode = anc_info.inear_tws_mode;
#endif/*INEAR_ANC_UI*/
            if ((anc_hdl->state == ANC_STA_INIT) && (anc_info.mode != ANC_OFF)) {
                anc_mode_switch(anc_info.mode, 0);
            }
        } else {
            user_anc_log("read anc_info err");
        }
#endif/*ANC_INFO_SAVE_ENABLE*/
    }
}

/*ANC poweroff*/
void anc_poweroff(void)
{
    if (anc_hdl) {
#if ANC_EAR_ADAPTIVE_EN && (!ANC_DEVELOPER_MODE_EN)
        anc_ear_adaptive_power_off_save_data();
#endif/*ANC_POWEOFF_SAVE_ADAPTIVE_DATA*/

        user_anc_log("anc_cur_state:%s\n", anc_state_str[anc_hdl->state]);
#if ANC_INFO_SAVE_ENABLE
        anc_info_save();
#endif/*ANC_INFO_SAVE_ENABLE*/
        if (anc_hdl->state == ANC_STA_OPEN) {
            user_anc_log("anc_poweroff\n");
            /*close anc module when fade_out timeout*/
            anc_hdl->param.mode = ANC_OFF;
            anc_hdl->new_mode = anc_hdl->param.mode;
            anc_fade(0);
        }
    }
}

/*模式切换测试demo*/
#define ANC_MODE_NUM	3 /*ANC模式循环切换*/
static const u8 anc_mode_switch_tab[ANC_MODE_NUM] = {
    ANC_OFF,
    ANC_TRANSPARENCY,
    ANC_ON,
};
void anc_mode_next(void)
{
    if (anc_hdl) {
        if (anc_train_open_query()) {
            return;
        }
        u8 next_mode = 0;
        local_irq_disable();
        anc_hdl->param.anc_fade_en = ANC_FADE_EN;	//防止被其他地方清0
        for (u8 i = 0; i < ANC_MODE_NUM; i++) {
            if (anc_mode_switch_tab[i] == anc_hdl->param.mode) {
                next_mode = i + 1;
                if (next_mode >= ANC_MODE_NUM) {
                    next_mode = 0;
                }
                if ((anc_hdl->mode_enable & BIT(anc_mode_switch_tab[next_mode])) == 0) {
                    user_anc_log("anc_mode_filt,next:%d,en:%d", next_mode, anc_hdl->mode_enable);
                    next_mode++;
                    if (next_mode >= ANC_MODE_NUM) {
                        next_mode = 0;
                    }
                }
                //g_printf("fine out anc mode:%d,next:%d,i:%d",anc_hdl->param.mode,next_mode,i);
                break;
            }
        }
        local_irq_enable();
        //user_anc_log("anc_next_mode:%d old:%d,new:%d", next_mode, anc_hdl->param.mode, anc_mode_switch_tab[next_mode]);
        u8 new_mode = anc_mode_switch_tab[next_mode];
        user_anc_log("new_mode:%s", anc_mode_str[new_mode]);

#if ANC_EAR_ADAPTIVE_EN && ANC_EAR_ADAPTIVE_EVERY_TIME		//非通话状态下，每次切模式都自适应
        if ((anc_mode_switch_tab[next_mode] == ANC_ON) && (esco_player_runing() == 0)) {
            anc_hdl->param.mode = ANC_ON;
            anc_hdl->new_mode = anc_hdl->param.mode;
            audio_anc_mode_ear_adaptive(1);
        } else
#endif/*ANC_EAR_ADAPTIVE_EVERY_TIME*/
        {
            anc_mode_switch(anc_mode_switch_tab[next_mode], 1);
        }
    }
}

/*设置ANC支持切换的模式*/
void anc_mode_enable_set(u8 mode_enable)
{
    if (anc_hdl) {
        anc_hdl->mode_enable = mode_enable;
        u8 mode_cnt = 0;
        for (u8 i = 1; i < 4; i++) {
            if (mode_enable & BIT(i)) {
                mode_cnt++;
                user_anc_log("%s Select", anc_mode_str[i]);
            }
        }
        user_anc_log("anc_mode_enable_set:%d", mode_cnt);
        anc_hdl->mode_num = mode_cnt;
    }
}

/*获取anc状态，0:空闲，l:忙*/
u8 anc_status_get(void)
{
    u8 status = 0;
    if (anc_hdl) {
        if (anc_hdl->state == ANC_STA_OPEN) {
            status = 1;
        }
    }
    return status;
}

/*获取anc当前模式*/
u8 anc_mode_get(void)
{
    if (anc_hdl) {
        //user_anc_log("anc_mode_get:%s", anc_mode_str[anc_hdl->param.mode]);
        return anc_hdl->param.mode;
    }
    return 0;
}

/*获取anc是否处于切模式状态*/
u8 anc_mode_switch_lock_get(void)
{
    if (anc_hdl) {
        return anc_hdl->mode_switch_lock;
    }
    return 0;
}

/*设置ANC切模式状态*/
void anc_mode_switch_lock_clean(void)
{
    if (anc_hdl) {
        anc_hdl->mode_switch_lock = 0;
    }
}

/*获取anc记录的最新的目标ANC模式*/
u8 anc_new_target_mode_get(void)
{
    if (anc_hdl) {
        //user_anc_log("anc_mode_get:%s", anc_mode_str[anc_hdl->new_mode]);
        return anc_hdl->new_mode;
    }
    return 0;
}

/*获取anc模式，dac左右声道的增益*/
u8 anc_dac_gain_get(u8 ch)
{
    u8 gain = 0;
    if (anc_hdl) {
        gain = anc_hdl->param.gains.dac_gain;
    }
    return gain;
}

/*获取anc模式，ff_mic的增益*/
u8 audio_anc_ffmic_gain_get(void)
{
    u8 gain = 0;
    if (anc_hdl) {
        if (anc_hdl->param.ch & ANC_L_CH) {
            gain = anc_hdl->param.gains.l_ffmic_gain;
        } else {
            gain = anc_hdl->param.gains.r_ffmic_gain;
        }
    }
    return gain;
}

/*获取anc模式，fb_mic的增益*/
u8 audio_anc_fbmic_gain_get(void)
{
    u8 gain = 0;
    if (anc_hdl) {
        if (anc_hdl->param.ch & ANC_L_CH) {
            gain = anc_hdl->param.gains.l_fbmic_gain;
        } else {
            gain = anc_hdl->param.gains.r_fbmic_gain;
        }
    }
    return gain;
}

/*获取anc模式，指定mic的增益, mic_sel:目标MIC通道*/
u8 audio_anc_mic_gain_get(u8 mic_sel)
{
    if (anc_hdl) {
        if (mic_sel < AUDIO_ADC_MAX_NUM)  {
            return anc_hdl->param.mic_param[mic_sel].gain;
        }
    }
    return 0;
}

/*anc coeff读接口*/
int *anc_coeff_read(void)
{
#if ANC_BOX_READ_COEFF
    int *coeff = anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    if (coeff) {
        coeff = (int *)((u8 *)coeff);
    }
    user_anc_log("anc_coeff_read:0x%x", (u32)coeff);
    return coeff;
#else
    return NULL;
#endif/*ANC_BOX_READ_COEFF*/
}


/*anc coeff写接口*/
int anc_coeff_write(int *coeff, u16 len)
{
#if ANC_MULT_ORDER_ENABLE
    return -1;
#endif/*ANC_MULT_ORDER_ENABLE*/
    anc_coeff_t *db_coeff = (anc_coeff_t *)coeff;
    anc_coeff_t *tmp_coeff = NULL;

    user_anc_log("anc_coeff_write:0x%x, len:%d", (u32)coeff, len);
    int ret = anc_coeff_check(db_coeff, len);
    if (ret) {
        return ret;
    }
    if (db_coeff->cnt != 1) {
        ret = anc_db_put(&anc_hdl->param, NULL, (anc_coeff_t *)coeff);
    } else {	//保存对应的单项滤波器
        ret = anc_coeff_single_fill(&anc_hdl->param, (anc_coeff_t *)coeff);
    }
    if (ret) {
        user_anc_log("anc_coeff_write err:%d", ret);
        return -1;
    }
    db_coeff = (anc_coeff_t *)anc_db_get(ANC_DB_COEFF, &anc_hdl->param.coeff_size);
    user_anc_log("anc_db coeff_size %d\n", anc_hdl->param.coeff_size);
    anc_coeff_fill(db_coeff);
#if ANC_EAR_ADAPTIVE_EN
    //左右参数映射，测试使用
    audio_anc_param_map(1, 0);
#endif/*ANC_EAR_ADAPTIVE_EN*/

    if (anc_hdl->param.mode != ANC_OFF) {		//实时更新填入使用
        audio_anc_reset(&anc_hdl->param, 0);
    }
    return 0;
}

static u8 ANC_idle_query(void)
{
    if (anc_hdl) {
        /*ANC训练模式，不进入低功耗*/
        if (anc_train_open_query()) {
            return 0;
        }
    }
    return 1;
}

static enum LOW_POWER_LEVEL ANC_level_query(void)
{
    if (anc_hdl == NULL) {
        return LOW_POWER_MODE_SLEEP;
    }
    /*根据anc的状态选择sleep等级*/
    if (anc_status_get() || anc_hdl->mode_switch_lock) {
        /*anc打开，进入轻量级低功耗*/
        return LOW_POWER_MODE_LIGHT_SLEEP;
    }
    /*anc关闭，进入最优低功耗*/
    return LOW_POWER_MODE_DEEP_SLEEP;
}

AT_VOLATILE_RAM_CODE_ANC
u8 is_lowpower_anc_active()
{
    if (anc_hdl) {
        if (anc_hdl->state == ANC_STA_OPEN || anc_hdl->mode_switch_lock) {
            /* 若anc 开启，则进入轻量级低功耗时，保持住ANC对应时钟 */
            if ((anc_hdl->param.ch == (ANC_L_CH | ANC_R_CH)) && !anc_hdl->param.lr_lowpower_en) {
                return ANC_CLOCK_USE_PLL;
            } else {
                return ANC_CLOCK_USE_BTOSC;
            }
        }
    }
    return ANC_CLOCK_USE_CLOSE;
}

REGISTER_LP_TARGET(ANC_lp_target) = {
    .name       = "ANC",
    .level      = ANC_level_query,
    .is_idle    = ANC_idle_query,
};

void chargestore_uart_data_deal(u8 *data, u8 len)
{
    /* anc_uart_process(data, len); */
}

#if TCFG_ANC_TOOL_DEBUG_ONLINE
//回复包
void anc_ci_send_packet(u32 id, u8 *packet, int size)
{
    if (DB_PKT_TYPE_ANC == id) {
        app_online_db_ack(anc_hdl->anc_parse_seq, packet, size);
        return;
    }
    /* y_printf("anc_app_spp_tx\n"); */
    ci_send_packet(id, packet, size);
}

//接收函数
static int anc_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    /* y_printf("anc_app_spp_rx\n"); */
    anc_hdl->anc_parse_seq  = ext_data[1];
    anc_spp_rx_packet(packet, size);
    return 0;
}
//发送函数
void anc_btspp_packet_tx(u8 *packet, int size)
{
    app_online_db_send(DB_PKT_TYPE_ANC, packet, size);
}
#endif/*TCFG_ANC_TOOL_DEBUG_ONLINE*/

u8 anc_btspp_train_again(u8 mode, u32 dat)
{
    /* tws_api_detach(TWS_DETACH_BY_POWEROFF); */
    //ANC模式下可直接获取REF+ERRMIC的数据
    /* if((dat == 3) || (dat == 5)) { */
    /* anc_hdl->param.debug_sel = dat; */
    /* os_taskq_post_msg("anc", 1, ANC_MSG_MIC_DATA_GET); */
    /* return 0; */
    /* } */
#if ANC_EAR_ADAPTIVE_EN
    if (anc_ear_adaptive_busy_get()) {	//ANC自适应切换时，不支持获取DMA
        return 0;
    }
#if TCFG_USER_TWS_ENABLE
    switch (dat) {
    case 2:
        dat = 4;
        break;
    case 3:
        dat = 5;
        break;
    case 6:
        dat = 7;
        break;
    }
#endif/*TCFG_USER_TWS_ENABLE*/
#endif/*ANC_EAR_ADAPTIVE_EN*/
    user_anc_log("anc_btspp_train_again\n");
    audio_anc_close();
    anc_hdl->state = ANC_STA_INIT;
    anc_train_open(mode, (u8)dat);
    return 1;
}

u8 audio_anc_develop_get(void)
{
    return anc_hdl->param.developer_mode;
}

void audio_anc_develop_set(u8 mode)
{
#if (!ANC_DEVELOPER_MODE_EN) && ANC_EAR_ADAPTIVE_EN	//非强制开发者模式跟随工具设置
    if (mode != anc_hdl->param.developer_mode) {
        anc_hdl->param.developer_mode = mode;
        anc_ear_adaptive_develop_set(mode);
    }
#endif/*ANC_DEVELOPER_MODE_EN*/
}

/*ANC配置在线读取接口*/
int anc_cfg_online_deal(u8 cmd, anc_gain_t *cfg)
{
    /*同步在线更新配置*/
    anc_gain_param_t *gains = &anc_hdl->param.gains;
    anc_param_fill(cmd, cfg);
    if (cmd == ANC_CFG_WRITE) {
        /*实时更新ANC配置*/
        audio_anc_dac_gain(gains->dac_gain, gains->dac_gain);
        audio_anc_mic_management(&anc_hdl->param);
        audio_anc_mic_gain(anc_hdl->param.mic_param, 0);
        anc_mix_out_audio_drc_thr(gains->audio_drc_thr);
        if (anc_hdl->param.mode != ANC_OFF) {		//实时更新填入使用
#if ANC_MULT_ORDER_ENABLE
            anc_mult_scene_set(anc_hdl->scene_id);	//覆盖增益以及增益符号
#endif/*ANC_MULT_ORDER_ENABLE*/
            audio_anc_reset(&anc_hdl->param, 0);		//不可异步，因为后面会更新ANCIF区域内容
#if ANC_MULT_ORDER_ENABLE
            audio_anc_mult_scene_coeff_free();		//释放空间
#endif/*ANC_MULT_ORDER_ENABLE*/
        }
        /*仅修改增益配置*/
        int ret = anc_db_put(&anc_hdl->param, cfg, NULL);
        if (ret) {
            user_anc_log("ret %d\n", ret);
            anc_hdl->param.lff_coeff = NULL;
            anc_hdl->param.rff_coeff = NULL;
            anc_hdl->param.lfb_coeff = NULL;
            anc_hdl->param.rfb_coeff = NULL;
        };
    }
    return 0;
}

#if 0
/*ANC数字MIC IO配置*/
static atomic_t dmic_mux_ref;
#define DMIC_SCLK_FROM_PLNK		0
#define DMIC_SCLK_FROM_ANC		1
void dmic_io_mux_ctl(u8 en, u8 sclk_sel)
{
    user_anc_log("dmic_io_mux,en:%d,sclk:%d,ref:%d\n", en, sclk_sel, atomic_read(&dmic_mux_ref));
    if (en) {
        if (atomic_read(&dmic_mux_ref)) {
            user_anc_log("DMIC_IO_MUX open now\n");
            if (sclk_sel == DMIC_SCLK_FROM_ANC) {
                user_anc_log("plink_sclk -> anc_sclk\n");
                gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_ANC_MICCK, 0, 1);
            }
            atomic_inc_return(&dmic_mux_ref);
            return;
        }
        if (sclk_sel == DMIC_SCLK_FROM_ANC) {
            gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_ANC_MICCK, 0, 1);
        } else {
            gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_PLNK_SCLK, 0, 1);
        }
        gpio_set_direction(TCFG_AUDIO_PLNK_SCLK_PIN, 0);
        gpio_set_die(TCFG_AUDIO_PLNK_SCLK_PIN, 0);
        gpio_direction_output(TCFG_AUDIO_PLNK_SCLK_PIN, 1);
#if TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT
        //anc data0 port init
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT0_PIN), PORT_INPUT_PULLUP_10K);
        gpio_set_fun_input_port(TCFG_AUDIO_PLNK_DAT0_PIN, PFI_PLNK_DAT0);
#endif/*TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT*/

#if TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT
        //anc data1 port init
        gpio_set_mode(IO_PORT_SPILT(TCFG_AUDIO_PLNK_DAT1_PIN), PORT_INPUT_PULLUP_10K);
        gpio_set_fun_input_port(TCFG_AUDIO_PLNK_DAT1_PIN, PFI_PLNK_DAT1);
#endif/*TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT*/
        atomic_inc_return(&dmic_mux_ref);
    } else {
        if (atomic_read(&dmic_mux_ref)) {
            atomic_dec_return(&dmic_mux_ref);
            if (atomic_read(&dmic_mux_ref)) {
                if (sclk_sel == DMIC_SCLK_FROM_ANC) {
                    user_anc_log("anc close now,anc_sclk->plnk_sclk\n");
                    gpio_set_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN, FO_PLNK_SCLK, 0, 1);
                } else {
                    user_anc_log("plnk close,anc_plnk open\n");
                }
            } else {
                user_anc_log("dmic all close,disable plnk io_mapping output\n");
                gpio_disable_fun_output_port(TCFG_AUDIO_PLNK_SCLK_PIN);
#if TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT
                gpio_disable_fun_input_port(TCFG_AUDIO_PLNK_DAT0_PIN);
#endif/*TCFG_AUDIO_PLNK_DAT0_PIN != NO_CONFIG_PORT*/
#if TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT
                gpio_disable_fun_input_port(TCFG_AUDIO_PLNK_DAT1_PIN);
#endif/*TCFG_AUDIO_PLNK_DAT1_PIN != NO_CONFIG_PORT*/
            }
        } else {
            user_anc_log("dmic_mux_ref NULL\n");
        }
    }
}
void anc_dmic_io_init(audio_anc_t *param, u8 en)
{
    if (en) {
        int i;
        for (i = 0; i < 4; i++) {
            if ((param->mic_type[i] > A_MIC1) && (param->mic_type[i] != MIC_NULL)) {
                user_anc_log("anc_dmic_io_init %d:%d\n", i, param->mic_type[i]);
                dmic_io_mux_ctl(1, DMIC_SCLK_FROM_ANC);
                break;
            }
        }
    } else {
        dmic_io_mux_ctl(0, DMIC_SCLK_FROM_ANC);
    }
}
#else
void anc_dmic_io_init(audio_anc_t *param, u8 en)
{
}
#endif

extern void mix_out_drc_threadhold_update(float threadhold);
static void anc_mix_out_audio_drc_thr(float thr)
{
    /* if (thr > 0.0 || thr < -6.0 || anc_hdl->param.enablebit == ANC_FB_EN) {
        thr = 0.0;
    }
    mix_out_drc_threadhold_update(thr); */
}

#if TCFG_AUDIO_DYNAMIC_ADC_GAIN
void anc_dynamic_micgain_start(u8 audio_mic_gain)
{
    int anc_mic_gain, i, diff_gain;
    u8 flag = 0;
    float ffgain;
    float multiple = 1.259;
    if (!anc_status_get()) {
        anc_hdl->mic_dy_state = ANC_MIC_DY_STA_INIT;
        return;
    }
    memcpy(anc_hdl->dy_mic_param, anc_hdl->param.mic_param, sizeof(audio_adc_mic_mana_t) * 4);

    for (i = 0; i < 4; i++) {   //动态MIC增益调整时，忽略FBmic
        if ((anc_hdl->dy_mic_param[i].type % 2) == 1) {
            anc_hdl->dy_mic_param[i].mult_flag = 0;
            printf("clean anc_hdl->dy_mic_param[%d].mult_flag\n", i);
        }
    }
    anc_hdl->drc_ratio = 1.0;
    ffgain = (float)(anc_hdl->param.mode == ANC_TRANSPARENCY ? anc_hdl->param.gains.drctrans_norgain : anc_hdl->param.gains.drcff_norgain);
    for (i = 0; i < 4; i++) {
        if (anc_hdl->dy_mic_param[i].mult_flag) {
            anc_mic_gain = anc_hdl->dy_mic_param[i].gain;
            anc_hdl->dy_mic_param[i].gain = audio_mic_gain;
            flag = 1;
            user_anc_log("after mic%d en %d, gain %d, type %d, mult_flag %d\n", i, anc_hdl->dy_mic_param[i].en, \
                         anc_hdl->dy_mic_param[i].gain, anc_hdl->dy_mic_param[i].type, anc_hdl->dy_mic_param[i].mult_flag);
        }
    }
    if ((!flag) || (!(audio_mic_gain - anc_mic_gain))) {
        user_anc_log("There is no common MIC\n");
        anc_hdl->mic_dy_state = ANC_MIC_DY_STA_STOP;
        return;
    }
    anc_hdl->mic_dy_state = ANC_MIC_DY_STA_START;
    if (anc_hdl->mic_resume_timer) {
        sys_timer_del(anc_hdl->mic_resume_timer);
        anc_hdl->mic_resume_timer = 0;
    }
    anc_hdl->mic_diff_gain = audio_mic_gain - anc_mic_gain;
    multiple = (anc_hdl->mic_diff_gain < 0) ? (1.0f / multiple) : multiple;
    diff_gain = (anc_hdl->mic_diff_gain < 0) ? 0 - anc_hdl->mic_diff_gain : anc_hdl->mic_diff_gain;
    for (i = 0; i < diff_gain; i++) {
        ffgain /= multiple;
        anc_hdl->drc_ratio *= multiple;
    }
    audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
    audio_anc_mic_gain(anc_hdl->dy_mic_param, 1);
    g_printf("dynamic mic:audio_g %d,anc_g %d, diff_g %d, ff_gain %d, mult %d/100\n", audio_mic_gain, anc_mic_gain, anc_hdl->mic_diff_gain, (s16)ffgain, (int)(multiple * 100.0f));
}

void anc_dynamic_micgain_resume_timer(void *priv)
{
    float multiple = 1.259;
    audio_adc_mic_mana_t mic_param[4];
    if (anc_hdl->mic_diff_gain > 0) {
        anc_hdl->mic_diff_gain--;
        anc_hdl->drc_ratio /= multiple;
        if (!anc_hdl->mic_diff_gain) {
            anc_hdl->drc_ratio = 1.0;
        }
        for (int i = 0; i < 4; i++) {
            if (anc_hdl->dy_mic_param[i].mult_flag) {
                anc_hdl->dy_mic_param[i].gain--;
            }
        }
    } else if (anc_hdl->mic_diff_gain < 0) {
        anc_hdl->mic_diff_gain++;
        anc_hdl->drc_ratio *= multiple;
        if (!anc_hdl->mic_diff_gain) {
            anc_hdl->drc_ratio = 1.0;
        }
        for (int i = 0; i < 4; i++) {
            if (anc_hdl->dy_mic_param[i].mult_flag) {
                anc_hdl->dy_mic_param[i].gain++;
            }
        }
    } else {
        sys_timer_del(anc_hdl->mic_resume_timer);
        anc_hdl->mic_resume_timer = 0;
    }
    audio_anc_drc_ctrl(&anc_hdl->param, anc_hdl->drc_ratio);
    audio_anc_mic_gain(anc_hdl->dy_mic_param, 1);
}

void anc_dynamic_micgain_stop(void)
{
    if ((anc_hdl->mic_dy_state == ANC_MIC_DY_STA_START) && anc_status_get()) {
        anc_hdl->mic_dy_state = ANC_MIC_DY_STA_STOP;
        anc_hdl->mic_resume_timer = sys_timer_add(NULL, anc_dynamic_micgain_resume_timer, 10);
    }
}
#endif/*TCFG_AUDIO_DYNAMIC_ADC_GAIN*/

void audio_anc_post_msg_drc(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_DRC_TIMER);
}

void audio_anc_post_msg_debug(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_DEBUG_OUTPUT);
}

void audio_anc_post_msg_icsd_anc_cmd(u8 cmd)
{
    os_taskq_post_msg("anc", 2, ANC_MSG_ICSD_ANC_CMD, cmd);
}

void audio_anc_post_msg_user_train_init(u8 mode)
{
    if (mode == ANC_ON) {
        os_taskq_post_msg("anc", 1, ANC_MSG_USER_TRAIN_INIT);
    }
}

void audio_anc_post_msg_user_train_run(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_USER_TRAIN_RUN);
}

void audio_anc_post_msg_user_train_setparam(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_USER_TRAIN_SETPARAM);
}

void audio_anc_post_msg_user_train_timeout(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_USER_TRAIN_TIMEOUT);
}

void audio_anc_post_msg_user_train_end(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_USER_TRAIN_END);
}

void audio_anc_post_msg_music_dyn_gain(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_MUSIC_DYN_GAIN);
}

#if TCFG_ANC_SELF_DUT_GET_SZ
void audio_anc_post_msg_sz_fft_open(u8 sel)
{
    os_taskq_post_msg("anc", 2, ANC_MSG_SZ_FFT_OPEN, sel);
}

void audio_anc_post_msg_sz_fft_run(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_SZ_FFT_RUN);
}

void audio_anc_post_msg_sz_fft_close(void)
{
    os_taskq_post_msg("anc", 1, ANC_MSG_SZ_FFT_CLOSE);
}

static void audio_anc_sz_fft_dac_check_slience_cb(void *buf, int len)
{
    if (anc_hdl->param.sz_fft.check_flag) {
        s16 *check_buf = (s16 *)buf;
        for (int i = 0; i < len / 2; i++) {
            if (check_buf[i]) {
                anc_hdl->param.sz_fft.check_flag = 0;
                audio_anc_sz_fft_trigger();
                dac_node_write_callback_del("ANC_SZ_FFT");
                break;
            }
        }
    }
}

//SZ_FFT测试函数
void audio_anc_sz_fft_test(void)
{
#if 0
    static u8 cnt = 0;
    g_printf("cnt %d\n", cnt);
    switch (cnt) {
    case 0:
        audio_anc_post_msg_sz_fft_open(4);
        break;
    case 1:
        audio_anc_sz_fft_trigger();
        /* play_tone_file(get_tone_files()->normal); */
        break;
    case 2:
        audio_anc_post_msg_sz_fft_close();
        break;
    case 3:
        audio_anc_sz_fft_outbuf_release();
        break;
    }
    if (++cnt > 3) {
        cnt = 0;
    }
#endif
}

#endif/* TCFG_ANC_SELF_DUT_GET_SZ*/


void audio_anc_dut_enable_set(u8 enablebit)
{
    anc_hdl->param.dut_audio_enablebit = enablebit;
}

void audio_anc_mic_mana_set_gain(audio_anc_t *param, u8 num, u8 type)
{
    switch (type) {
    case 0:
        param->mic_param[num].gain = param->gains.l_ffmic_gain;
        break;
    case 1:
        param->mic_param[num].gain = param->gains.l_fbmic_gain;
        break;
    case 2:
        param->mic_param[num].gain = param->gains.r_ffmic_gain;
        break;
    case 3:
        param->mic_param[num].gain = param->gains.r_fbmic_gain;
        break;
    }
}

/*设置fb  mic为复用mic*/
void audio_anc_mic_mana_fb_mult_set(u8 mult_flag)
{
    if (anc_hdl) {
        audio_anc_t *param = &anc_hdl->param;
        for (int i = 0; i < 4; i++) {
            if ((param->mic_param[i].type == ANC_MIC_TYPE_LFB) || (param->mic_param[i].type == ANC_MIC_TYPE_RFB)) {
                param->mic_param[i].mult_flag = mult_flag;
            }
        }
    }
}

void audio_anc_mic_management(audio_anc_t *param)
{
    int i;
    struct adc_file_cfg *cfg = audio_adc_file_get_cfg();
    for (i = 0; i < 4; i++) {
        if (param->mic_type[i] == A_MIC0) {
            param->mic_param[0].en = 1;
            param->mic_param[0].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_0) {
                param->mic_param[0].mult_flag = 1;
            }
            audio_anc_mic_mana_set_gain(param, 0, i);
        }
        if (param->mic_type[i] == A_MIC1) {
            param->mic_param[1].en = 1;
            param->mic_param[1].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_1) {
                param->mic_param[1].mult_flag = 1;
            }
            audio_anc_mic_mana_set_gain(param, 1, i);
        }
        if (param->mic_type[i] == A_MIC2) {
            param->mic_param[2].en = 1;
            param->mic_param[2].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_2) {
                param->mic_param[2].mult_flag = 1;
            }
            audio_anc_mic_mana_set_gain(param, 2, i);
        }
        if (param->mic_type[i] == A_MIC3) {
            param->mic_param[3].en = 1;
            param->mic_param[3].type = i;
            if (cfg->mic_en_map & AUDIO_ADC_MIC_3) {
                param->mic_param[3].mult_flag = 1;
            }
            audio_anc_mic_mana_set_gain(param, 3, i);
        }
    }
    for (i = 0; i < 4; i++) {
        printf("mic%d en %d, gain %d, type %d, mult_flag %d\n", i, param->mic_param[i].en, \
               param->mic_param[i].gain, param->mic_param[i].type, param->mic_param[i].mult_flag);
    }
}

void audio_anc_adc_ch_set(void)
{
    struct adc_file_cfg *cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();
    if (cfg->mic_en_map & AUDIO_ADC_MIC_0) {
        anc_hdl->param.adc_ch = (anc_hdl->param.adc_ch << 1) | 1;
    }
    if (cfg->mic_en_map & AUDIO_ADC_MIC_1) {
        anc_hdl->param.adc_ch = (anc_hdl->param.adc_ch << 1) | 1;
    }
    if (cfg->mic_en_map & AUDIO_ADC_MIC_2) {
        anc_hdl->param.adc_ch = (anc_hdl->param.adc_ch << 1) | 1;
    }
    if (cfg->mic_en_map & AUDIO_ADC_MIC_3) {
        anc_hdl->param.adc_ch = (anc_hdl->param.adc_ch << 1) | 1;
    }
    if (!anc_hdl->param.adc_ch) {	//通话没有使用模拟MIC，则adc_ch默认赋1, adc_ch必须赋予初值。
        anc_hdl->param.adc_ch = 1;
    }

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    /* 开免摘功能后，根据通话mic和免摘mic个数，取最大值来打开ADC的数字通道，保证免摘ADC取数正常 */
    u8 esco_mic_num = audio_adc_file_get_esco_mic_num();
    u8 adt_mic_num = get_icsd_adt_mic_num();
    u8 mic_num = esco_mic_num > adt_mic_num ? esco_mic_num : adt_mic_num;
    if (mic_num == 2) {
        anc_hdl->param.adc_ch = BIT(1) | BIT(0);
    } else {
        anc_hdl->param.adc_ch = BIT(2) | BIT(1) | BIT(0);
    }
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

    for (int i = 0; i < AUDIO_ADC_MAX_NUM; i++) {
        anc_hdl->param.mic_param[i].mic_p.mic_mode      = platform_cfg[i].mic_mode;
        anc_hdl->param.mic_param[i].mic_p.mic_ain_sel   = platform_cfg[i].mic_ain_sel;
        anc_hdl->param.mic_param[i].mic_p.mic_bias_sel  = platform_cfg[i].mic_bias_sel;
        anc_hdl->param.mic_param[i].mic_p.mic_bias_rsel = platform_cfg[i].mic_bias_rsel;
        anc_hdl->param.mic_param[i].mic_p.mic_dcc       = platform_cfg[i].mic_dcc;
    }
}


audio_adc_mic_mana_t *audio_anc_mic_param_get(void)
{
    if (anc_hdl) {
        return anc_hdl->param.mic_param;
    }
    return NULL;
}

/*
   ANC 驱动复位（包括滤波器），支持淡入淡出
   前置条件：需在非ANC_OFF前提下调用;
   param: fade_en 1 开启淡入淡出，会有一定执行时间；
   				  0 关闭淡入淡出，会有po声；
 */
void audio_anc_param_reset(u8 fade_en)
{
    if ((anc_hdl->param.mode != ANC_OFF) && !anc_hdl->mode_switch_lock) {
        os_taskq_post_msg("anc", 2, ANC_MSG_RESET, fade_en);
    }
}

#if ANC_EAR_ADAPTIVE_EN
void audio_ear_adaptive_en_set(u8 en)
{
    if (anc_hdl) {
        anc_hdl->ear_adaptive = en;
    }
}

/*耳道自适应互斥功能挂起*/
void audio_ear_adaptive_train_app_suspend(void)
{
    anc_mode_change_tool(ANC_FF_EN);					//设置仅FF通路
#if ANC_ADAPTIVE_EN
    //关闭场景自适应功能
    audio_anc_power_adaptive_suspend();
#endif

    /* audio_anc_drc_toggle_set(0);						//关闭DRC */
}

/*耳道自适应互斥功能恢复*/
void audio_ear_adaptive_train_app_resume(void)
{
    anc_mode_change_tool(TCFG_AUDIO_ANC_TRAIN_MODE);
#if ANC_ADAPTIVE_EN
    //恢复场景自适应功能
    audio_anc_power_adaptive_resume();
#endif
    /* audio_anc_drc_toggle_set(1); */
}
#endif/*ANC_EAR_ADAPTIVE_EN*/

#if ANC_MULT_ORDER_ENABLE

int audio_anc_mult_scene_set(u16 scene_id)
{
    user_anc_log(" audio_anc_mult scene_id set %d\n", scene_id);
    if (audio_anc_mult_scene_id_check(scene_id)) {
        user_anc_log("anc_mult_scene id set err id = %d", scene_id);
        return 1;
    }
    anc_hdl->scene_id = scene_id;
#if ANC_EAR_ADAPTIVE_EN
    if (anc_ear_adaptive_busy_get()) {
        anc_ear_adaptive_forced_exit(1);
        return 1;
    }
    //非自适应训练状态，切场景自动切回普通参数
    anc_hdl->param.anc_coeff_mode = ANC_COEFF_MODE_NORMAL;
#endif/*ANC_EAR_ADAPTIVE_EN*/
    if ((anc_hdl->param.mode != ANC_OFF) && !anc_hdl->mode_switch_lock) {
        anc_mult_scene_set(scene_id);
    }
    return 0;
}

int audio_anc_mult_scene_update(u16 scene_id)
{
    int ret = audio_anc_mult_scene_set(scene_id);
    audio_anc_param_reset(1);
    return ret;
}

u8 audio_anc_mult_scene_get(void)
{
    return anc_hdl->scene_id;
}

void audio_anc_mult_scene_max_set(u8 scene_cnt)
{
    anc_hdl->scene_max = scene_cnt;
}

u8 audio_anc_mult_scene_max_get(void)
{
    return anc_hdl->scene_max;
}

void audio_anc_mult_scene_switch(u8 tone_flag)
{
    u8 cnt = audio_anc_mult_scene_get();
    if (cnt < audio_anc_mult_scene_max_get()) {
        cnt++;
    } else {
        cnt = 1;
    }
    if (tone_flag) {
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() != TWS_ROLE_SLAVE) {
            tws_play_tone_file(get_tone_files()->num[cnt], 400);
        }
#else
        play_tone_file(get_tone_files()->num[cnt]);
#endif
        /* tone_play_index(IDEX_TONE_NUM_0 + cnt, 0); */
    }
    audio_anc_mult_scene_update(cnt);
}
#endif/*ANC_MULT_ORDER_ENABLE*/

void audio_anc_howldet_fade_set(u16 gain)
{
    if (anc_hdl) {
        audio_anc_fade_ctr_set(ANC_FADE_MODE_HOWLDET, AUDIO_ANC_FDAE_CH_ALL, gain);
    }
}

void audio_icsd_biquad2ab_out(float gain, float f, float fs, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, int type)
{
#if (TCFG_AUDIO_ANC_EXT_VERSION == ANC_EXT_V2)
    icsd_biquad2ab_out_v2(gain, f, fs, q, a0, a1, a2, b0, b1, b2, type);
#else
    icsd_biquad2ab_out(gain, f, fs, q, a0, a1, a2, b0, b1, b2, type);
#endif
}

#endif/*TCFG_AUDIO_ANC_ENABLE*/
