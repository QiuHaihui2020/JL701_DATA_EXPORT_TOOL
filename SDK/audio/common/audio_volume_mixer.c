/*
 ******************************************************************************************
 *							Volume Mixer
 *
 * Discription: 音量合成器
 *
 * Notes:
 *  (1)音量合成器框图如下：
 *    [bt]----->[app volume]---+
 *                             |
 *    [tone]--->[app volume]---+---->[sys volume]--->[Device(DAC/IIS/...)]
 *                             |
 *    [music]-->[app volume]---+
 *
 *	(2)app volume：应用音量，控制每个音频数据流独立的音量，互不影响
 * 				   对应windows音量合成器中的应用程序音量(Applications)
 *	(3)sys volume：系统音量，控制整个系统的全局音量
 *				   对应windows音量合成器中的设备音量(Device)
 ******************************************************************************************
 */
#include "system/includes.h"
#include "media/includes.h"
#include "user_cfg.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "jlstream.h"
#include "update.h"
#include "audio_config.h"
#include "audio_dvol.h"
#include "audio_dac_digital_volume.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "asm/math_fast_function.h"
#include "volume_node.h"
#include "tone_player.h"
#include "ring_player.h"

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
#include "app_le_broadcast.h"
#endif

#include "le_audio_player.h"

#if TCFG_AUDIO_DUT_ENABLE
#include "audio_dut_control.h"
#endif/*TCFG_AUDIO_DUT_ENABLE*/

#ifdef SUPPORT_MS_EXTENSIONS
#pragma const_seg(	".app_audio_const")
#pragma code_seg(	".app_audio_code")
#endif/*SUPPORT_MS_EXTENSIONS*/

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define WARNING_TONE_VOL_FIXED 1
#define DEFAULT_DIGTAL_VOLUME   16384
struct __audio_bt_vol {
    u8 addr[6];
    s16 music_volume;
    u8 update_flag;
    u8 old;
};

#ifdef	DVOL_2P1_CH_DVOL_ADJUST_NODE
#if (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_LR)
static char *dvol_type[] = {"Music1", "Call", "Tone", "Ring", "KTone"};
#elif (DVOL_2P1_CH_DVOL_ADJUST_NODE == DVOL_2P1_CH_DVOL_ADJUST_SW)
static char *dvol_type[] = {"Music2", "Call", "Tone", "Ring", "KTone"};
#else
static char *dvol_type[] = {"Music", "Call", "Tone", "Ring", "KTone"};
#endif
#else
static char *dvol_type[] = {"Music", "Call", "Tone", "Ring", "KTone"};
#endif

typedef short unaligned_u16 __attribute__((aligned(1)));
struct app_audio_config {
    u8 state;								/*当前声音状态*/
    u8 prev_state;							/*上一个声音状态*/
    u8 prev_state_save;						/*保存上一个声音状态*/
    u8 mute_when_vol_zero;
    volatile u8 fade_gain_l;
    volatile u8 fade_gain_r;
    volatile s16 fade_dgain_l;
    volatile s16 fade_dgain_r;
    volatile s16 fade_dgain_step_l;
    volatile s16 fade_dgain_step_r;
    volatile int fade_timer;
    s16 digital_volume;
    u8 analog_volume_l;
    u8 analog_volume_r;
    s16 max_volume[APP_AUDIO_STATE_WTONE + 1];
    u8 sys_cvol_max;
    u8 call_cvol_max;
    u16 sys_hw_dvol_max;	//系统最大硬件数字音量(非通话模式)
    u16 call_hw_dvol_max;	//通话模式最大硬件数字音量
    u8  music_mute_state;                       /*记录当前音乐是否处于mute*/
    u8  call_mute_state;                       /*记录当前通话是否处于mute*/
    u8  wtone_mute_state;                       /*记录当前提示音是否处于mute*/
    u8  save_vol_cnt;
    u16 save_vol_timer;
#if defined(VOL_NOISE_OPTIMIZE) &&( VOL_NOISE_OPTIMIZE)
    float dac_dB; //dac的增益值
#endif
    u16 target_dig_vol;
};

/*声音状态字符串定义*/
static const char *audio_state[] = {
    "idle",
    "music",
    "call",
    "tone",
    "err",
};

/*音量类型字符串定义*/
static const char *vol_type[] = {
    "VOL_D",
    "VOL_A",
    "VOL_AD",
    "VOL_D_HW",
    "VOL_ERR",
};
#define DVOL_TYPE_NUM 5
static struct app_audio_config app_audio_cfg = {0};

#define __this      (&app_audio_cfg)
extern struct audio_dac_hdl dac_hdl;
extern struct dac_platform_data dac_data;


/*
 *************************************************************
 *
 *	audio volume fade
 *
 *************************************************************
 */

static void audio_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    //printf("<fade:%d-%d-%d-%d>", gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r);
    local_irq_disable();
    if ((gain_l == __this->fade_gain_l) && (gain_r == __this->fade_gain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        audio_dac_set_L_digital_vol(&dac_hdl, gain_l ? __this->digital_volume : 0);
        audio_dac_set_R_digital_vol(&dac_hdl, gain_r ? __this->digital_volume : 0);
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                /* audio_dac_mute(&dac_hdl, 1); */
            }
        }
        local_irq_enable();
        /*
         *淡入淡出结束，也把当前的模拟音量设置一下，防止因为淡入淡出的音量和保存音量的变量一致，
         *而寄存器已经被清0的情况
         */
        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        /* log_info("dac_fade_end, VOL : 0x%x 0x%x\n", JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0); */
        return;
    }
    if (gain_l > __this->fade_gain_l) {
        gain_l--;
    } else if (gain_l < __this->fade_gain_l) {
        gain_l++;
    }

    audio_dac_set_analog_vol(&dac_hdl, gain_l);
    local_irq_enable();
}

static int audio_fade_timer_add(u8 gain_l, u8 gain_r)
{
    /* r_printf("dac_fade_begin:0x%x,gain_l:%d,gain_r:%d\n", __this->fade_timer, gain_l, gain_r); */
    local_irq_disable();
    __this->fade_gain_l = gain_l;
    __this->fade_gain_r = gain_r;
    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_fade_timer, 2, 1);
        /* y_printf("fade_timer:0x%x", __this->fade_timer); */
    }
    local_irq_enable();

    return 0;
}


#if (SYS_VOL_TYPE == VOL_TYPE_AD)

#define DGAIN_SET_MAX_STEP (300)
#define DGAIN_SET_MIN_STEP (30)

static unsigned short combined_vol_list[17][2] = {
    { 0,     0}, //0: None
    { 0,  2657}, // 1:-45.00 db
    { 0,  3476}, // 2:-42.67 db
    { 0,  4547}, // 3:-40.33 db
    { 0,  5948}, // 4:-38.00 db
    { 0,  7781}, // 5:-35.67 db
    { 0, 10179}, // 6:-33.33 db
    { 0, 13316}, // 7:-31.00 db
    { 1, 14198}, // 8:-28.67 db
    { 2, 14851}, // 9:-26.33 db
    { 3, 15596}, // 10:-24.00 db
    { 5, 13043}, // 11:-21.67 db
    { 6, 13567}, // 12:-19.33 db
    { 7, 14258}, // 13:-17.00 db
    { 8, 14749}, // 14:-14.67 db
    { 9, 15389}, // 15:-12.33 db
    {10, 16144}, // 16:-10.00 db
};

static unsigned short call_combined_vol_list[16][2] = {
    { 0,     0}, //0: None
    { 0,  4725}, // 1:-40.00 db
    { 0,  5948}, // 2:-38.00 db
    { 0,  7488}, // 3:-36.00 db
    { 0,  9427}, // 4:-34.00 db
    { 0, 11868}, // 5:-32.00 db
    { 0, 14941}, // 6:-30.00 db
    { 1, 15331}, // 7:-28.00 db
    { 2, 15432}, // 8:-26.00 db
    { 3, 15596}, // 9:-24.00 db
    { 4, 15788}, // 10:-22.00 db
    { 5, 15802}, // 11:-20.00 db
    { 6, 15817}, // 12:-18.00 db
    { 7, 15998}, // 13:-16.00 db
    { 8, 15926}, // 14:-14.00 db
    { 9, 15991}, // 15:-12.00 db
};

void audio_combined_vol_init(u8 cfg_en)
{
    u16 sys_cvol_len = 0;
    u16 call_cvol_len = 0;
    u8 *sys_cvol  = NULL;
    u8 *call_cvol  = NULL;

    __this->sys_cvol_max = ARRAY_SIZE(combined_vol_list) - 1;
    __this->sys_cvol = (unaligned_u16 *)combined_vol_list;
    __this->call_cvol_max = ARRAY_SIZE(call_combined_vol_list) - 1;
    __this->call_cvol = (unaligned_u16 *)call_combined_vol_list;

    if (cfg_en) {
        sys_cvol  = syscfg_ptr_read(CFG_COMBINE_SYS_VOL_ID, &sys_cvol_len);
        //ASSERT(((u32)sys_cvol & BIT(0)) == 0, "sys_cvol addr unalignd(2):%x\n", sys_cvol);
        if (sys_cvol && sys_cvol_len) {
            __this->sys_cvol = (unaligned_u16 *)sys_cvol;
            __this->sys_cvol_max = sys_cvol_len / 4 - 1;
            //y_printf("read sys_combine_vol succ:%x,len:%d",__this->sys_cvol,sys_cvol_len);
            /* cvol = __this->sys_cvol;
            for(int i = 0,j = 0;i < (sys_cvol_len / 2);j++) {
            	printf("sys_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read sys_cvol false:%x,%x\n", sys_cvol, sys_cvol_len);
        }

        call_cvol  = syscfg_ptr_read(CFG_COMBINE_CALL_VOL_ID, &call_cvol_len);
        //ASSERT(((u32)call_cvol & BIT(0)) == 0, "call_cvol addr unalignd(2):%d\n", call_cvol);
        if (call_cvol && call_cvol_len) {
            __this->call_cvol = (unaligned_u16 *)call_cvol;
            __this->call_cvol_max = call_cvol_len / 4 - 1;
            //y_printf("read call_combine_vol succ:%x,len:%d",__this->call_cvol,call_cvol_len);
            /* cvol = __this->call_cvol;
            for(int i = 0,j = 0;i < call_cvol_len / 2;j++) {
            	printf("call_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read call_combine_vol false:%x,%x\n", call_cvol, call_cvol_len);
        }
    }

    log_info("sys_cvol_max:%d,call_cvol_max:%d\n", __this->sys_cvol_max, __this->call_cvol_max);
}


static void audio_combined_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    s16 dgain_l = dac_hdl.d_volume[DA_LEFT];
    s16 dgain_r = dac_hdl.d_volume[DA_RIGHT];

    __this->fade_dgain_step_l = __builtin_abs(dgain_l - __this->fade_dgain_l) / \
                                (__builtin_abs(gain_l - __this->fade_gain_l) + 1);
    if (__this->fade_dgain_step_l > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_l < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MIN_STEP;
    }

    __this->fade_dgain_step_r = __builtin_abs(dgain_r - __this->fade_dgain_r) / \
                                (__builtin_abs(gain_r - __this->fade_gain_r) + 1);
    if (__this->fade_dgain_step_r > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_r < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MIN_STEP;
    }

    /* log_info("<a:%d-%d-%d-%d d:%d-%d-%d-%d-%d-%d>\n", \ */
    /* gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r, \ */
    /* dgain_l, dgain_r, __this->fade_dgain_l, __this->fade_dgain_r, \ */
    /* __this->fade_dgain_step_l, __this->fade_dgain_step_r); */

    local_irq_disable();

    if ((gain_l == __this->fade_gain_l) \
        && (gain_r == __this->fade_gain_r) \
        && (dgain_l == __this->fade_dgain_l)\
        && (dgain_r == __this->fade_dgain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                audio_dac_mute(&dac_hdl, 1);
            }
        }

        local_irq_enable();
        /* log_info("dac_fade_end,VOL:0x%x-0x%x-%d-%d-%d-%d\n", \ */
        /* JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0,  \ */
        /* __this->fade_gain_l, __this->fade_gain_r, \ */
        /* __this->fade_dgain_l, __this->fade_dgain_r); */
        return;
    }
    if ((gain_l != __this->fade_gain_l) \
        || (gain_r != __this->fade_gain_r)) {
        if (gain_l > __this->fade_gain_l) {
            gain_l--;
        } else if (gain_l < __this->fade_gain_l) {
            gain_l++;
        }

        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        local_irq_enable();//fix : 不同时调模拟和数字可以避免杂音
        return;
    }

    if ((dgain_l != __this->fade_dgain_l) \
        || (dgain_r != __this->fade_dgain_r)) {

        if (gain_l != __this->fade_gain_l) {
            if (dgain_l > __this->fade_dgain_l) {
                if ((dgain_l - __this->fade_dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l -= __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            } else if (dgain_l < __this->fade_dgain_l) {
                if ((__this->fade_dgain_l - dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l += __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            }
        } else {
            dgain_l = __this->fade_dgain_l;
        }

        audio_dac_set_digital_vol(&dac_hdl, dgain_l);
    }

    local_irq_enable();
}


static int audio_combined_fade_timer_add(u8 gain_l, u8 gain_r)
{
    u8  gain_max;
    u8  target_again_l = 0;
    u8  target_again_r = 0;
    u16 target_dgain_l = 0;
    u16 target_dgain_r = 0;

    if (__this->state == APP_AUDIO_STATE_CALL) {
        gain_max = __this->call_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->call_cvol[gain_l * 2]);
        target_again_r = *(&__this->call_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->call_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->call_cvol[gain_r * 2 + 1]);
    } else {
        gain_max = __this->sys_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->sys_cvol[gain_l * 2]);
        target_again_r = *(&__this->sys_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->sys_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->sys_cvol[gain_r * 2 + 1]);
    }
#if 0//TCFG_AUDIO_ANC_ENABLE
    target_again_l = anc_dac_gain_get(ANC_DAC_CH_L);
    target_again_r = anc_dac_gain_get(ANC_DAC_CH_R);
#endif

    printf("[l]v:%d,Av:%d,Dv:%d", gain_l, target_again_l, target_dgain_l);
    //y_printf("[r]v:%d,Av:%d,Dv:%d", gain_r, target_again_r, target_dgain_r);
    /* log_info("dac_com_fade_begin:0x%x\n", __this->fade_timer); */

    local_irq_disable();

    __this->fade_gain_l  = target_again_l;
    __this->fade_gain_r  = target_again_r;
    __this->fade_dgain_l = target_dgain_l;
    __this->fade_dgain_r = target_dgain_r;

    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_combined_fade_timer, 2, 1);
        /* log_info("combined_fade_timer:0x%x", __this->fade_timer); */
    }

    local_irq_enable();

    return 0;
}

#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
#define DVOL_HW_LEVEL_MAX	31	/*注意:总共是(DVOL_HW_LEVEL_MAX + 1)级*/
const u16 hw_dig_vol_table[DVOL_HW_LEVEL_MAX + 1] = {
    0	, //0
    93	, //1
    111	, //2
    132	, //3
    158	, //4
    189	, //5
    226	, //6
    270	, //7
    323	, //8
    386	, //9
    462	, //10
    552	, //11
    660	, //12
    789	, //13
    943	, //14
    1127, //15
    1347, //16
    1610, //17
    1925, //18
    2301, //19
    2751, //20
    3288, //21
    3930, //22
    4698, //23
    5616, //24
    6713, //25
    8025, //26
    9592, //27
    10222,//28
    14200,//29
    16000,//30
    16384 //31
};

void audio_hw_digital_vol_init(u8 cfg_en)
{
    float dB_value = BT_MUSIC_VOL_MAX ;
#if (TCFG_AUDIO_ANC_ENABLE)
    dB_value = (dB_value > ANC_MODE_DIG_VOL_LIMIT) ? ANC_MODE_DIG_VOL_LIMIT : dB_value;
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    app_var.aec_dac_gain = (app_var.aec_dac_gain > BT_CALL_VOL_LEAVE_MAX) ? BT_CALL_VOL_LEAVE_MAX : app_var.aec_dac_gain;
    __this->sys_hw_dvol_max = (u16)(16384.f * dB_Convert_Mag(dB_value));
    float call_hw_dvol_max_dB = BT_MUSIC_VOL_MAX + (BT_CALL_VOL_LEAVE_MAX - app_var.aec_dac_gain) * BT_CALL_VOL_STEP;
    printf("aec_dac_gain:%d,call_hw_dvol_max_dB:%.1f\n", app_var.aec_dac_gain, call_hw_dvol_max_dB);
    __this->call_hw_dvol_max = (u16)(16384.f * dB_Convert_Mag(call_hw_dvol_max_dB));
    printf("sys_hw_dvol_max:%d,call_hw_dvol_max:%d\n", __this->sys_hw_dvol_max, __this->call_hw_dvol_max);
}

#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW*/


#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)

#define DVOL_SW_LEVEL_MAX	31	/*注意:总共是(DVOL_SW_LEVEL_MAX + 1)级*/
u16 sw_dig_vol_table[DVOL_SW_LEVEL_MAX + 1] = {0};

void audio_sw_digital_vol_init(u8 cfg_en)
{
    /*
       develop分支音量配置从stream.bin获取
       包括通话/音乐 音量区间，以及音量等级
       故以下代码先屏蔽
    */
#if 0
    float dB_value = DEFAULT_DIGITAL_VOLUME;
#if (TCFG_AUDIO_ANC_ENABLE)
    dB_value = (dB_value > ANC_MODE_DIG_VOL_LIMIT) ? ANC_MODE_DIG_VOL_LIMIT : dB_value;
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    __this->sys_hw_dvol_max = 16384.0f * dB_Convert_Mag(dB_value);

    int vol = app_audio_volume_max_query(AppVol_BT_MUSIC);
    float temp = 0;
    //printf("sw digital volume list:\n");
    while (vol) {
        temp = 16384.0f * dB_Convert_Mag(dB_value);
        sw_dig_vol_table[vol] = (u16)temp;
        //printf("[%d] dB:%.1f %.1f %d\n", vol, dB_value, temp, sw_dig_vol_table[vol]);
        dB_value -= -BT_MUSIC_VOL_STEP;
        vol--;
    }
    sw_dig_vol_table[0] = 0;

    audio_digital_vol_init(sw_dig_vol_table, app_audio_volume_max_query(AppVol_BT_MUSIC));

    app_var.aec_dac_gain = (app_var.aec_dac_gain > BT_CALL_VOL_LEAVE_MAX) ? BT_CALL_VOL_LEAVE_MAX : app_var.aec_dac_gain;
    __this->call_hw_dvol_max = (u16)(__this->sys_hw_dvol_max * dB_Convert_Mag((BT_CALL_VOL_LEAVE_MAX - app_var.aec_dac_gain) * BT_CALL_VOL_STEP));
    printf("sys_hw_dvol_max:%d,call_hw_dvol_max:%d\n", __this->sys_hw_dvol_max, __this->call_hw_dvol_max);
#endif
}
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

void audio_volume_list_init(u8 cfg_en)
{
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
    audio_combined_vol_init(cfg_en);
#elif (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    /* audio_hw_digital_vol_init(cfg_en); */
#endif/*SYS_VOL_TYPE*/
}

static void set_audio_device_volume(u8 type, s16 vol)
{
    audio_dac_set_analog_vol(&dac_hdl, vol);
}

static int get_audio_device_volume(u8 vol_type)
{
    return 0;
}

void volume_up_down_direct(s16 value)
{

}

/*
*********************************************************************
*          			Audio Volume Fade
* Description: 音量淡入淡出
* Arguments  : left_vol
*			   right_vol
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_fade_in_fade_out(u8 left_vol, u8 right_vol)
{
    __this->analog_volume_l = dac_hdl.pd->l_ana_gain;
    __this->analog_volume_r = dac_hdl.pd->r_ana_gain;
    /*根据audio state切换的时候设置的最大音量,限制淡入淡出的最大音量*/
    u8 max_vol_l = __this->max_volume[__this->state];
    if (__this->state == APP_AUDIO_STATE_IDLE && max_vol_l == 0) {
        max_vol_l = __this->max_volume[__this->state] = IDLE_DEFAULT_MAX_VOLUME;
    }
    u8 max_vol_r = max_vol_l;
    printf("[fade]state:%s,max_volume:%d,cur:%d,%d", audio_state[__this->state], max_vol_l, left_vol, left_vol);
    /*淡入淡出音量限制*/
    u8 left_gain = left_vol > max_vol_l ? max_vol_l : left_vol;
    u8 right_gain = right_vol > max_vol_r ? max_vol_r : right_vol;

    /*数字音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    s16 volume = right_gain;
    if (volume > __this->max_volume[__this->state]) {
        volume = __this->max_volume[__this->state];
    }
    printf("set_vol[%s]:=%d\n", audio_state[__this->state], volume);

    /*按照配置限制通话时候spk最大音量*/
    /* if (__this->state == APP_AUDIO_STATE_CALL) { */
    /* __this->digital_volume = __this->call_hw_dvol_max; */
    /* } else { */
    /* __this->digital_volume = __this->sys_hw_dvol_max; */
    /* } */
    printf("[SW_DVOL]Gain:%d,AVOL:%d,DVOL:%d\n", left_gain, __this->analog_volume_l, __this->digital_volume);
    audio_dac_vol_set(TYPE_DAC_AGAIN, 0x1, __this->analog_volume_l, 1);
    audio_dac_vol_set(TYPE_DAC_AGAIN, 0x2, __this->analog_volume_r, 1);
#if defined(VOL_NOISE_OPTIMIZE) &&( VOL_NOISE_OPTIMIZE)
    if (__this->dac_dB) { //设置回目标数字音量
        audio_dac_vol_set(TYPE_DAC_DGAIN, 0x3, __this->target_dig_vol, 1);
    } else {
#else
    if (1) {
#endif
        audio_dac_vol_set(TYPE_DAC_DGAIN, 0x3, __this->digital_volume, 1);
    }
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
    /*模拟音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
    audio_fade_timer_add(left_gain, right_gain);
#endif/*SYS_VOL_TYPE == VOL_TYPE_ANALOG*/

    /*模拟数字联合音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_AD)
    audio_combined_fade_timer_add(left_gain, right_gain);
#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/

    /*硬件数字音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    u8 dvol_hw_level;
    extern float eq_db2mag(float x);
    float dvol_db;
    float dvol_gain;
    int dvol_max;
    if (__this->state == APP_AUDIO_STATE_CALL) {
        dvol_hw_level = app_var.aec_dac_gain * DVOL_HW_LEVEL_MAX / BT_CALL_VOL_LEAVE_MAX;
        dvol_max = hw_dig_vol_table[dvol_hw_level];
        dvol_db = (BT_CALL_VOL_LEAVE_MAX - left_vol) * BT_CALL_VOL_STEP;
        dvol_gain = eq_db2mag(dvol_db);//dB转换倍数
        __this->digital_volume = (s16)(dvol_max * dvol_gain);
    } else {
        dvol_db =  BT_MUSIC_VOL_MAX + (BT_MUSIC_VOL_LEAVE_MAX - left_gain) * BT_MUSIC_VOL_STEP;
        __this->digital_volume = (s16)(16384.0f * eq_db2mag(dvol_db));
        /* dvol_hw_level = left_gain * DVOL_HW_LEVEL_MAX / app_audio_volume_max_query(AppVol_BT_MUSIC); */
        /* __this->digital_volume = hw_dig_vol_table[dvol_hw_level]; */
        /* __this->digital_volume = left_gain * 16384 / app_audio_volume_max_query(AppVol_BT_MUSIC); */
    }
    printf("[HW_DVOL]Gain:%d,AVOL:%d,DVOL:%d\n", left_gain, __this->analog_volume_l, __this->digital_volume);
    audio_dac_vol_set(TYPE_DAC_AGAIN, 0x3, __this->analog_volume_l, 1);
    audio_dac_vol_set(TYPE_DAC_DGAIN, 0x3, __this->digital_volume, 1);

#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW*/
}

/*
 *************************************************************
 *					Audio Volume Save
 *Notes:如果不想保存音量（比如保存音量到vm，可能会阻塞），可以
 *		定义AUDIO_VOLUME_SAVE_DISABLE来关闭音量保存
 *************************************************************
 */

static void app_audio_volume_save_do(void *priv)
{
    /* log_info("app_audio_volume_save_do %d\n", __this->save_vol_cnt); */
    local_irq_disable();
    if (++__this->save_vol_cnt >= 5) {
        sys_timer_del(__this->save_vol_timer);
        __this->save_vol_timer = 0;
        __this->save_vol_cnt = 0;
        local_irq_enable();
        log_info("VOL_SAVE %d\n", app_var.music_volume);
        if (app_in_mode(APP_MODE_PC) == 0) {                      //pc模式写vm导致丢中断,暂时PC模式不设置音量等到退出pc模式才记录
            syscfg_write(CFG_MUSIC_VOL, &app_var.music_volume, 2);//中断里不能操作vm 关中断不能操作vm
        }
        return;
    }
    local_irq_enable();
}

static void app_audio_volume_change(void)
{
#ifndef AUDIO_VOLUME_SAVE_DISABLE
    local_irq_disable();
    __this->save_vol_cnt = 0;
    if (__this->save_vol_timer == 0) {
        __this->save_vol_timer = sys_timer_add(NULL, app_audio_volume_save_do, 1000);//中断里不能操作vm 关中断不能操作vm
    }
    local_irq_enable();
#endif
}

int audio_digital_vol_node_name_get(u8 dvol_idx, char *node_name)
{
    struct app_mode *mode;
    mode = app_get_current_mode();
    int i = 0;
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
    if (le_audio_player_is_playing()) {
        sprintf(node_name, "%s%s", "Vol_LE_", "Audio");
        return 0;
    }
#endif


    for (i = 0; i < DVOL_TYPE_NUM; i++) {
        if (dvol_idx & BIT(i)) {
#if !WARNING_TONE_VOL_FIXED
            if (dvol_idx  & TONE_DVOL) {
                if (tone_player_runing()) {
                    sprintf(node_name, "%s%s", "Vol_Sys", "Tone");
                } else if (ring_player_runing()) {
                    sprintf(node_name, "%s%s", "Vol_Sys", "Ring");
                }
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                continue;
            }
#endif
            switch (mode->name) {
            case APP_MODE_BT:
#if TCFG_AUDIO_DUT_ENABLE
                if (audio_dec_dut_en_get(1)) {
                    sprintf(node_name, "%s%s", "Vol_Btd", dvol_type[i]);
                    printf("vol_name:%d,%s\n", __LINE__, node_name);
                    break;
                }
#endif/*TCFG_AUDIO_DUT_ENABLE*/
                if (esco_player_is_playing(NULL)) { //用于区分通话和播歌的提示音
                    sprintf(node_name, "%s%s", "Vol_Btc", dvol_type[i]);
                } else {
                    sprintf(node_name, "%s%s", "Vol_Btm", dvol_type[i]);
                }
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#if TCFG_APP_LINEIN_EN
            case APP_MODE_LINEIN:
                sprintf(node_name, "%s%s", "Vol_Lin", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
#if TCFG_APP_MUSIC_EN
            case APP_MODE_MUSIC:
                sprintf(node_name, "%s%s", "Vol_File", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
#if TCFG_APP_FM_EN
            case APP_MODE_FM:
                sprintf(node_name, "%s%s", "Vol_Fm", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
#if TCFG_APP_SPDIF_EN
            case APP_MODE_SPDIF:
                sprintf(node_name, "%s%s", "Vol_Spd", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
#if TCFG_APP_PC_EN
            case APP_MODE_PC:
                sprintf(node_name, "%s%s", "Vol_Pcspk", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
#if TCFG_APP_IIS_EN
            case APP_MODE_IIS:
                sprintf(node_name, "%s%s", "Vol_IIS", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
#endif
            case APP_MODE_IDLE:
                sprintf(node_name, "%s%s", "Vol_Sys", dvol_type[i]);
                printf("vol_name:%d,%s\n", __LINE__, node_name);
                break;
            default:
                printf("vol_name:%d,NULL\n", __LINE__);
                return -1;
            }
        } //end of if
    } //end of for
    return 0;
}

int audio_digital_vol_update_parm(u8 dvol_idx, s32 param)
{
    int err = 0;
    char vol_name[32];
    err = audio_digital_vol_node_name_get(dvol_idx, vol_name);
    if (!err) {
        err |= jlstream_set_node_param(NODE_UUID_VOLUME_CTRLER, vol_name, (void *)param, sizeof(struct volume_cfg));
    } else {
        printf("[Error]audio_digital_vol_node_name_get err:%x\n", err);
    }
    return err;
}

extern struct volume_cfg default_vol_cfg;
//获取当前模式music数据流节点的默认音量
int audio_digital_vol_default_init(void)
{
    int ret = 0;
    if (app_var.volume_def_state) {
        char vol_name[32];
        struct volume_cfg cfg;
        ret = audio_digital_vol_node_name_get(MUSIC_DVOL, vol_name);
        if (!ret) {
            ret = volume_ioc_get_cfg(vol_name, &cfg);
        } else {
            cfg = default_vol_cfg;
        }
        app_var.music_volume = cfg.cur_vol;
    }
    return ret;
}

//设置是否使用工具上配置的当前音量
void app_audio_set_volume_def_state(u8 volume_def_state)
{
    app_var.volume_def_state = volume_def_state;
}

static void app_audio_set_vol_timer_func(void *arg)
{
    u8 dvol_idx = ((u32)(arg) >> 16) & 0xff;
    s16 volume = ((u32)arg) & 0xffff;

    struct volume_cfg cfg = {0};
    cfg.bypass = VOLUME_NODE_CMD_SET_VOL;
    cfg.cur_vol = volume;
    audio_digital_vol_update_parm(dvol_idx, (s32)&cfg);
}

static void app_audio_set_mute_timer_func(void *arg)
{
    u8 dvol_idx = ((u32)(arg) >> 16) & 0xff;
    s16 mute_en = ((u32)arg) & 0xffff;
    struct volume_cfg cfg = {0};
    cfg.bypass = VOLUME_NODE_CMD_SET_MUTE;
    cfg.cur_vol = mute_en;
    audio_digital_vol_update_parm(dvol_idx, (s32)&cfg);
}

/*
*********************************************************************
*          			Audio Volume Set
* Description: 音量设置
* Arguments  : state	目标声音通道
*			   volume	目标音量值
*			   fade		是否淡入淡出
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_set(u8 state, s16 volume, u8 fade)
{
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用



#if (RCSP_MODE && RCSP_ADV_EQ_SET_ENABLE)
    extern bool rcsp_set_volume(s8 volume);
    if (rcsp_set_volume(volume)) {
        return;
    }
#endif


    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        dvol_idx = MUSIC_DVOL;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        dvol_idx = CALL_DVOL;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume = volume;
        dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    app_audio_volume_change();
    printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);

    if (__this->max_volume[state]) {
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        if (volume > __this->max_volume[state]) {
            volume = __this->max_volume[state];
        }
        u32 param = dvol_idx << 16 | volume;
        sys_timeout_add((void *)param, app_audio_set_vol_timer_func, 5); //5ms后更新音量
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
        if ((state == __this->state) && !app_audio_get_dac_digital_mute()) {
            if ((__this->state == APP_AUDIO_STATE_CALL) && (volume == 0)) {
                //来电报号发下来通话音量为0的时候不设置模拟音量,避免来电报号音量突然变成0导致提示音不完整
                return;
            }
            audio_dac_set_volume(&dac_hdl, volume);
            if (fade) {
                audio_fade_in_fade_out(volume, volume);
            } else {
                /* audio_dac_set_analog_vol(&dac_hdl, volume); */
                audio_dac_ch_analog_gain_set(&dac_hdl, DAC_CH_FL, dac_hdl.pd->l_ana_gain);
                audio_dac_ch_analog_gain_set(&dac_hdl, DAC_CH_FR, dac_hdl.pd->r_ana_gain);
#ifdef VOL_HW_RL_RR_EN
                audio_dac_ch_analog_gain_set(&dac_hdl, DAC_CH_RL, dac_hdl.pd->rl_ana_gain);
                audio_dac_ch_analog_gain_set(&dac_hdl, DAC_CH_RR, dac_hdl.pd->rr_ana_gain);
#endif
            }
        }
    }
}

/*
*********************************************************************
*          			Audio Volume MUTE
* Description: 将数据静音或者解开静音
* Arguments  : mute_en	是否使能静音, 0:不使能,1:使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_mute_en(u8 mute_en)
{
    u8 dvol_idx = 0; //记录音量通道供数字音量控制使用
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        dvol_idx = MUSIC_DVOL;
        __this->music_mute_state = mute_en;
        break;
    case APP_AUDIO_STATE_CALL:
        dvol_idx = CALL_DVOL;
        __this->call_mute_state = mute_en;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        dvol_idx = TONE_DVOL | RING_DVOL | KEY_TONE_DVOL;
        __this->wtone_mute_state = mute_en;
        break;
    default:
        break;
    }
    u32 param = dvol_idx << 16 | mute_en;
    sys_timeout_add((void *)param, app_audio_set_mute_timer_func, 5); //5ms后将数据mute 或者解mute
}
/*
*********************************************************************
*                  Audio Volume Get
* Description: 音量获取
* Arguments  : state	要获取音量值的音量状态
* Return	 : 返回指定状态对应得音量值
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_volume(u8 state)
{
    s16 volume = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
        volume = app_var.wtone_volume;
        if (!volume) {
            volume = app_var.music_volume;
        }
        break;
    case APP_AUDIO_STATE_KTONE:
        volume = app_var.ktone_volume;
        if (!volume) {
            volume = app_var.music_volume;
        }
        break;
    case APP_AUDIO_STATE_RING:
        volume = app_var.ring_volume;
        if (!volume) {
            volume = app_var.music_volume;
        }
        break;
    case APP_AUDIO_CURRENT_STATE:
        volume = app_audio_get_volume(__this->state);
        break;
    default:
        break;
    }

    return volume;
}

/*
*********************************************************************
*                  Audio Mute Get
* Description: mute状态获取
* Arguments  : state	要获取是否mute的音量状态
* Return	 : 返回指定状态对应的mute状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_mute_state(u8 state)
{
    u8 mute_state = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        mute_state = __this->music_mute_state;
        break;
    case APP_AUDIO_STATE_CALL:
        mute_state = __this->call_mute_state;
        break;
    case APP_AUDIO_STATE_WTONE:
        mute_state = __this->wtone_mute_state;
        break;
    case APP_AUDIO_CURRENT_STATE:
        mute_state = app_audio_get_mute_state(__this->state);
        break;
    default:
        break;
    }
    return mute_state;
}


/*
*********************************************************************
*                  Audio Mute
* Description: 静音控制
* Arguments  : value Mute操作
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
static const char *audio_mute_string[] = {
    "mute_default",
    "unmute_default",
    "mute_L",
    "unmute_L",
    "mute_R",
    "unmute_R",
};

void app_audio_mute(u8 value)
{
    printf("audio_mute:%s", audio_mute_string[value]);
    switch (value) {
    case AUDIO_MUTE_DEFAULT:
        audio_dac_digital_mute(&dac_hdl, 1);
        break;
    case AUDIO_UNMUTE_DEFAULT:
        audio_dac_digital_mute(&dac_hdl, 0);
        break;
    }
}
u8 app_audio_get_dac_digital_mute() //获取DAC 是否mute
{
    return audio_dac_digital_mute_state(&dac_hdl);
}

/*
*********************************************************************
*                  Audio Volume Up
* Description: 增加当前音量通道的音量
* Arguments  : value	要增加的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_up(u8 value)
{
#if (RCSP_MODE && RCSP_ADV_EQ_SET_ENABLE)
    extern bool rcsp_key_volume_up(u8 value);
    if (rcsp_key_volume_up(value)) {
        return;
    }
#endif
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume += value;
        if (app_var.music_volume > app_audio_get_max_volume()) {
            app_var.music_volume = app_audio_get_max_volume();
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume += value;

        /*模拟音量类型，通话的时候直接限制最大音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
        if (app_var.call_volume > app_var.aec_dac_gain) {
            app_var.call_volume = app_var.aec_dac_gain;
        }
#endif/*SYS_VOL_TYPE == VOL_TYPE_ANALOG*/
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume += value;
        if (app_var.wtone_volume > app_audio_get_max_volume()) {
            app_var.wtone_volume = app_audio_get_max_volume();
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }
    app_audio_set_volume(__this->state, volume, 1);
}

/*
*********************************************************************
*                  Audio Volume Down
* Description: 减少当前音量通道的音量
* Arguments  : value	要减少的音量值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_app_volume_down(u8 value)
{
#if (RCSP_MODE && RCSP_ADV_EQ_SET_ENABLE)
    extern bool rcsp_key_volume_down(u8 value);
    if (rcsp_key_volume_down(value)) {
        return;
    }
#endif
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume -= value;
        if (app_var.music_volume < 0) {
            app_var.music_volume = 0;
        }
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume -= value;
        if (app_var.call_volume < 0) {
            app_var.call_volume = 0;
        }
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume -= value;
        if (app_var.wtone_volume < 0) {
            app_var.wtone_volume = 0;
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }

    app_audio_set_volume(__this->state, volume, 1);
}

/*level:0~15*/
static const u16 phone_call_dig_vol_tab[] = {
    0,	//0
    111,//1
    161,//2
    234,//3
    338,//4
    490,//5
    708,//6
    1024,//7
    1481,//8
    2142,//9
    3098,//10
    4479,//11
    6477,//12
    9366,//13
    14955,//14
    16384 //15
};

/*
*********************************************************************
*          			Audio Volume Set
* Description: 音量设置
* Arguments  : state	目标声音通道
*			   volume	目标音量值
*			   fade		是否淡入淡出
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_init_dig_vol(u8 state, s16 volume, u8 fade, dvol_handle *dvol_hdl)
{
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        break;
    case APP_AUDIO_STATE_WTONE:
        app_var.wtone_volume = volume;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (volume > __this->max_volume[state]) {
        volume = __this->max_volume[state];
    }
    audio_digital_vol_set(dvol_hdl, volume);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    if (state == __this->state && (!app_audio_get_dac_digital_mute())) {
        if ((__this->state == APP_AUDIO_STATE_CALL) && (volume == 0)) {
            //来电报号发下来通话音量为0的时候不设置模拟音量,避免来电报号音量突然变成0导致提示音不完整
            return;
        }
        audio_dac_set_volume(&dac_hdl, volume);
        if (fade) {
            audio_fade_in_fade_out(volume, volume);
        } else {
            audio_dac_set_analog_vol(&dac_hdl, volume);
        }
    }
    app_audio_volume_change();
}


/*
*********************************************************************
*                  Audio State Switch
* Description: 切换声音状态
* Arguments  : state
*			   max_volume
               dvol_hdl  //软件数字音量句柄,没有的时候传NULL
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/

void app_audio_state_switch(u8 state, s16 max_volume, dvol_handle *dvol_hdl)
{
    printf("audio_state:%s->%s,max_vol:%d\n", audio_state[__this->state], audio_state[state], max_volume);

    __this->prev_state_save = __this->prev_state;
    __this->prev_state = __this->state;
    __this->state = state;
    int scale = 100;
    if (max_volume != __this->max_volume[state] && __this->max_volume[state] != 0) {
        scale = max_volume * 100 / __this->max_volume[state];
    }

    float dB_value = DEFAULT_DIGITAL_VOLUME;
#if (TCFG_AUDIO_ANC_ENABLE)
    dB_value = (dB_value > ANC_MODE_DIG_VOL_LIMIT) ? ANC_MODE_DIG_VOL_LIMIT : dB_value;
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    u16 dvol_max = (u16)(16384.0f * dB_Convert_Mag(dB_value));

    /*记录当前状态对应的最大音量*/
    __this->max_volume[state] = max_volume;
    __this->analog_volume_l = MAX_ANA_VOL;
    __this->analog_volume_r = MAX_ANA_VOL;
    __this->digital_volume = dvol_max;

    int cur_vol = app_audio_get_volume(state) * scale / 100 ;
    cur_vol = (cur_vol > __this->max_volume[state]) ? __this->max_volume[state] : cur_vol;
    app_audio_init_dig_vol(state, cur_vol, 1, dvol_hdl);
}

/*
*********************************************************************
*                  Audio State Exit
* Description: 退出当前的声音状态
* Arguments  : state	要退出的声音状态
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_state_exit(u8 state)
{
    if (state == __this->state) {
        __this->state = __this->prev_state;
        if ((__this->prev_state == APP_AUDIO_STATE_CALL) && (!esco_player_is_playing(NULL))) { //切回通话状态需要判断通话的数据流是否在跑
            __this->state = APP_AUDIO_STATE_IDLE;
        }
        __this->prev_state = __this->prev_state_save;
        __this->prev_state_save = APP_AUDIO_STATE_IDLE;
    } else if (state == __this->prev_state) {
        __this->prev_state = __this->prev_state_save;
        __this->prev_state_save = APP_AUDIO_STATE_IDLE;
    }
}

/*
*********************************************************************
*                  Audio Set Max Volume
* Description: 设置最大音量
* Arguments  : state		要设置最大音量的声音状态
*			   max_volume	最大音量
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void app_audio_set_max_volume(u8 state, s16 max_volume)
{
    __this->max_volume[state] = max_volume;
}

/*
*********************************************************************
*                  Audio State Get
* Description: 获取当前声音状态
* Arguments  : None.
* Return	 : 返回当前的声音状态
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_get_state(void)
{
    return __this->state;
}

/*
*********************************************************************
*                  Audio Volume_Max Get
* Description: 获取当前声音通道的最大音量
* Arguments  : None.
* Return	 : 返回当前的声音通道最大音量
* Note(s)    : None.
*********************************************************************
*/
s16 app_audio_get_max_volume(void)
{
    if (__this->state == APP_AUDIO_STATE_IDLE) {
        return  app_audio_volume_max_query(AppVol_BT_MUSIC);
    }
    return __this->max_volume[__this->state];
}

void app_audio_set_mix_volume(u8 front_volume, u8 back_volume)
{
    /*set_audio_device_volume(AUDIO_MIX_FRONT_VOL, front_volume);
    set_audio_device_volume(AUDIO_MIX_BACK_VOL, back_volume);*/
}
#if 0

void audio_vol_test()
{
    app_set_sys_vol(10, 10);
    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);

    app_set_max_vol(30);
    app_set_ana_vol(25, 24);
    app_set_dig_vol(90, 90);

    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);
}
#endif



int esco_dec_dac_gain_set(u8 gain)
{
    app_var.aec_dac_gain = gain;
    /* if (esco_player_runing()) { */
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
    audio_dac_set_analog_vol(&dac_hdl, gain);
#else
    app_audio_set_max_volume(APP_AUDIO_STATE_CALL, gain);
    app_audio_set_volume(APP_AUDIO_STATE_CALL, app_audio_get_volume(APP_AUDIO_STATE_CALL), 1);
#endif/*SYS_VOL_TYPE*/
    /* } */
    return 0;
}

int esco_dec_dac_gain_get(void)
{
    /* if (esco_player_runing()) { */
#if (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
    int l_gain = audio_dac_ch_analog_gain_get(&dac_hdl, DAC_CH(0));
    int r_gain = audio_dac_ch_analog_gain_get(&dac_hdl, DAC_CH(1));
    return l_gain > r_gain ? l_gain : r_gain;
#else
    return app_audio_get_volume(APP_AUDIO_STATE_CALL);
#endif/*SYS_VOL_TYPE*/
    /* } */
}


#if TCFG_1T2_VOL_RESUME_WHEN_NO_SUPPORT_VOL_SYNC

extern u8 get_music_vol_for_no_vol_sync_dev(u8 *addr);
extern void set_music_vol_for_no_vol_sync_dev(u8 *addr, u8 vol);
//保存没有音量同步设备音量
void app_audio_bt_volume_save(u8 state)
{
    u8 avrcp_vol = 0;
    u8 cur_btaddr[6];
    struct app_mode *mode;
    mode = app_get_current_mode();
    if (mode == NULL) {
        //蓝牙状态未初始化时，mode为空，会导致后续访问mode异常
        return;
    }
    if ((state != APP_AUDIO_STATE_MUSIC) || (mode->name != APP_MODE_BT)) {
        //只有蓝牙模式才会保存音量
        //暂时不对其他音量记录，通话音量蓝牙库会返回
        return;
    }
    a2dp_player_get_btaddr(cur_btaddr);
    s16 max_vol = app_audio_volume_max_query(AppVol_BT_MUSIC);
    avrcp_vol = (app_var.music_volume * 127) / max_vol;
    set_music_vol_for_no_vol_sync_dev(cur_btaddr, avrcp_vol);
}

//更新本地音量到协议栈
void app_audio_bt_volume_save_mac(u8 *addr)
{
    u8 avrcp_vol = 0;
    s16 max_vol = app_audio_volume_max_query(AppVol_BT_MUSIC);
    avrcp_vol = (app_var.music_volume * 127) / max_vol;
    set_music_vol_for_no_vol_sync_dev(addr, avrcp_vol);
}

//更新没有音量同步设备音量
u8 app_audio_bt_volume_update(u8 *btaddr, u8 state)
{
    u8 vol = 0;
    vol = get_music_vol_for_no_vol_sync_dev(btaddr);
    if (vol == 0xff) {
        s16 max_vol = app_audio_volume_max_query(AppVol_BT_MUSIC);
        vol = (app_var.music_volume * 127) / max_vol;
        set_music_vol_for_no_vol_sync_dev(btaddr, vol);
    }
    printf("btaddr vol update %s vol %d\n", audio_state[state], vol);
    return vol;
}
#else
void app_audio_bt_volume_save(u8 state) {}
void app_audio_bt_volume_save_mac(u8 *addr) {}
u8 app_audio_bt_volume_update(u8 *btaddr, u8 state) {}
#endif/*TCFG_1T2_VOL_RESUME_WHEN_NO_SUPPORT_VOL_SYNC*/

/*
*********************************************************************
*           app_audio_dac_vol_mode_set
* Description: DAC 音量增强模式切换
* Arguments  : mode		1：音量增强模式  0：普通模式
* Return	 : 0：成功  -1：失败
* Note(s)    : None.
*********************************************************************
*/
#ifdef DAC_VOL_MODE_EN
void app_audio_dac_vol_mode_set(u8 mode)
{
    audio_dac_volume_enhancement_mode_set(&dac_hdl, mode);
    syscfg_write(CFG_VOLUME_ENHANCEMENT_MODE, &mode, 1);
    printf("DAC VOL MODE SET: %s\n", mode ? "VOLUME_ENHANCEMENT_MODE" : "NORMAL_MODE");
}

/*
*********************************************************************
*           app_audio_dac_vol_mode_get
* Description: DAC 音量增强模式状态获取
* Arguments  : None.
* Return	 : 1：音量增强模式  0：普通模式
* Note(s)    : None.
*********************************************************************
*/
u8 app_audio_dac_vol_mode_get(void)
{
    return audio_dac_volume_enhancement_mode_get(&dac_hdl);
}
#endif

void app_audio_set_volume(u8 state, s16 volume, u8 fade)
{
    audio_app_volume_set(state, volume, fade);
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (state == APP_AUDIO_STATE_MUSIC) {
        update_broadcast_sync_data(BROADCAST_SYNC_VOL, volume);
    }
#endif

#if AUDIO_VBASS_LINK_VOLUME
    if (state == APP_AUDIO_STATE_MUSIC) {
        vbass_link_volume();
    }
#endif
}

void app_audio_change_volume(u8 state, s16 volume)
{
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
        break;
    case APP_AUDIO_STATE_WTONE:
#if WARNING_TONE_VOL_FIXED
        return;
#endif
        app_var.wtone_volume = volume;
        break;
    default:
        return;
    }
    app_audio_bt_volume_save(state);
    printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);
    app_audio_volume_change();
}

void app_audio_volume_up(u8 value)
{
    //调音量就取消使用默认值
    app_var.volume_def_state = 0;
    audio_app_volume_up(value);
}

void app_audio_volume_down(u8 value)
{
    //调音量就取消使用默认值
    app_var.volume_def_state = 0;
    audio_app_volume_down(value);
}

s16 app_audio_volume_max_query(audio_vol_index_t index)
{
    if (index < Vol_NULL) {
        return volume_ioc_get_max_level(audio_vol_str[index]);
    } else {
        return volume_ioc_get_max_level(audio_vol_str[Vol_NULL]);
    }
}

