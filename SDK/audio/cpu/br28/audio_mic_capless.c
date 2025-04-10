#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_mic_capless.data.bss")
#pragma data_seg(".audio_mic_capless.data")
#pragma const_seg(".audio_mic_capless.text.const")
#pragma code_seg(".audio_mic_capless.text")
#endif
/*
 ****************************************************************************
 *							Audio Mic_Capless Module
 *								省电容mic模块
 *
 *Notes:省电容MIC表示直接将mic的信号输出接到芯片的MICIN引脚
 ****************************************************************************
 */
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "audio_config.h"
#include "gpadc.h"
#include "adc_file.h"

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define MC_LOG_ENABLE
#ifdef MC_LOG_ENABLE
#define MC_LOG_I	g_printf
#define MC_LOG_E	r_printf
#else
#define MC_LOG_E(...)
#define MC_LOG_I(...)
#endif/*MC_LOG_ENABLE*/

const int const_mic_capless_dtb = TCFG_MC_DTB_FIXED;
#if TCFG_SUPPORT_MIC_CAPLESS
extern struct audio_adc_hdl adc_hdl;
extern struct audio_adc_private_param adc_private_param;
extern void delay_2ms(int cnt);
extern void wdt_clear(void);
typedef struct {
    u8 save_bias_res;	/*trim结果是否保存*/
    u8 trim;			/*trim状态*/
    u8 cur_bias_res;
    u8 cur_bias1_res;
    OS_SEM sem;
} MicCapless_trim_t;
MicCapless_trim_t *mc_trim = NULL;

#define MC_TRIM_P	BIT(0)
#define MC_TRIM_N	BIT(1)

#define LADC_CAPLESS_ADJUST_SAVE
#ifdef LADC_CAPLESS_ADJUST_SAVE
#define DIFF_RANGE		50
#define CFG_DIFF_RANGE	100
#define CHECK_INTERVAL  10
#endif/*LADC_CAPLESS_ADJUST_SAVE*/

#define DACDTB_RANGE_MAX	8100
#define DACDTB_RANGE_MIN	5000

#define MIC_CAPLESS_ADJUST_BUD_DEFAULT	0
#define MIC_CAPLESS_ADJUST_BUD			100

#define MC_TRIM_SUCC	0
#define MC_TRIM_ERR		1

#define MC_TRIM_TIMEOUT				300

static const u8 mic_bias_tab[] = {0, 20, 12, 56, 4, 18, 10, 26, 2, 22, 14, 30, 21, 6, 11, 1, 5, 7, 32, 96};
#define MIC_BIAS_RSEL(x) 	SFR(JL_ADDA->ADA_CON1,  6,  7,  x)     // MICA_BIAS_RSEL_11v[6:0]
#define MIC1_BIAS_RSEL(x) 	SFR(JL_ADDA->ADA_CON2,  6,  7,  x)     // MICB_BIAS_RSEL_11v[6:0]

/*不支持自动校准，使用快速收敛*/
#if TCFG_MC_BIAS_AUTO_ADJUST
u8	mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD_DEFAULT;
#else
u8	mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/

u32 read_capless_DTB(void)
{
    u32 dacr32 = 0;
    int ret = syscfg_read(CFG_DAC_DTB, &dacr32, sizeof(dacr32));
    if (ret != sizeof(dacr32)) {
        /*
         *没有记忆值,使用默认值
         *没有收敛值的时候，使用快速收敛
         */
        log_info("DAC_DTB NULL,use fast feedback");
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        return (DACR32_DEFAULT | (DACR32_DEFAULT << 16));
    }
    log_info("cfg DTB0:%d,DTB1:%d,DTB:%x,ret = %d\n", (dacr32 & 0xFFFF), (dacr32 >> 16), dacr32, ret);
    return dacr32;
}

static u32 read_vm_capless_DTB(void)
{
    u32 vm_dacr32 = 0;
    int ret = syscfg_read(CFG_DAC_DTB, &vm_dacr32, sizeof(vm_dacr32));
    if (ret != sizeof(vm_dacr32)) {
        log_info("vm DTB read err\n");
        return (DACR32_DEFAULT | (DACR32_DEFAULT << 16));
    }
    log_info("vm DTB0:%d,DTB1:%d,DTB:%x,ret = %d\n", (vm_dacr32 & 0xFFFF), (vm_dacr32 >> 16), vm_dacr32, ret);
    return vm_dacr32;
}

typedef struct {
    u8 adjust_complete;
    u8 dtb_step_limit;	//限制dtb收敛步进
    u8 mic_bias_valid;
    u16 save_dtb;
    u16 save_dtb1;
} mc_var_t;
mc_var_t mc_var = {
    .save_dtb = DACR32_DEFAULT,
    .save_dtb1 = DACR32_DEFAULT,
    .adjust_complete = 0,
    .dtb_step_limit = 0,
    .mic_bias_valid = 0,
};

#if TCFG_MC_DTB_STEP_LIMIT
/*获取省电容mic收敛信息配置*/
int get_mc_dtb_step_limit(void)
{
    return mc_var.dtb_step_limit;
}
#endif /*TCFG_MC_DTB_STEP_LIMIT*/

void save_capless_DTB()
{
    s16 diff;
    u32 save_dac_dtb;
    MC_LOG_I("save_capless_DTB\n");
    /*
     *(1)dtb0或者dtb1有一个不是默认值
     *(2)收敛成功
     */
    if (((mc_var.save_dtb != DACR32_DEFAULT) || (mc_var.save_dtb1 != DACR32_DEFAULT)) && mc_var.adjust_complete) {
        /*读取记忆值，比较是否需要更新配置*/
        u32 cfg_dacr32 = read_vm_capless_DTB();
        /*分离dtb0和dtb1*/
        u16 cfg_dtb0 = (cfg_dacr32 & 0xFFFF);
        u16 cfg_dtb1 = (cfg_dacr32 >> 16);
        mc_var.adjust_complete = 0;
        log_info("save_dtb0:%d,save_dtb1:%d\n", mc_var.save_dtb, mc_var.save_dtb1);
        log_info("cfg_dtb:0x%x,cfg_dtb0:%d,cfg_dtb1:%d\n", cfg_dacr32, cfg_dtb0, cfg_dtb1);

        diff = mc_var.save_dtb - cfg_dtb0;
        if ((mc_var.save_dtb != DACR32_DEFAULT) && ((cfg_dtb0 == DACR32_DEFAULT) || ((diff < -CFG_DIFF_RANGE) || (diff > CFG_DIFF_RANGE)))) {
            save_dac_dtb = mc_var.save_dtb;
            log_info("dtb0 update:%d,0x%x,save_dac_dtb:%x\n", mc_var.save_dtb, mc_var.save_dtb, save_dac_dtb);
        } else {
            save_dac_dtb = cfg_dtb0;
            log_info("dtb0 need't update:%d,diff:%d\n", mc_var.save_dtb, diff);
        }

        diff = mc_var.save_dtb1 - cfg_dtb1;
        if ((mc_var.save_dtb1 != DACR32_DEFAULT) && ((cfg_dtb1 == DACR32_DEFAULT) || ((diff < -CFG_DIFF_RANGE) || (diff > CFG_DIFF_RANGE)))) {
            save_dac_dtb |= (mc_var.save_dtb1 << 16);
            log_info("dtb1 update:%d,0x%x,save_dac_dtb:%x\n", mc_var.save_dtb1, mc_var.save_dtb1, save_dac_dtb);
        } else {
            save_dac_dtb |= (cfg_dtb1 << 16);
            log_info("dtb1 need't update:%d,diff:%d\n", mc_var.save_dtb1, diff);
        }

        log_info("dtb0:0x%x,dtb1:0x%x,dtb:0x%x\n", mc_var.save_dtb, mc_var.save_dtb1, save_dac_dtb);
        if (save_dac_dtb != cfg_dacr32) {
            MC_LOG_I("store mc dtb:%x\n", save_dac_dtb);
            syscfg_write(CFG_DAC_DTB, &save_dac_dtb, 4);
            /* u32 tmp_dacr32 = 0;
            syscfg_read(CFG_DAC_DTB,&tmp_dacr32,4);
            MC_LOG_I("read mc dtb:%x\n",tmp_dacr32); */
        }
    } else {
        log_info("dacr32 adjust uncomplete:%d,%d,complete:%d\n", mc_var.save_dtb, mc_var.save_dtb1, mc_var.adjust_complete);
    }
}

/*
 *收敛快调慢调边界
 */
u16 get_ladc_capless_bud(void)
{
    //printf("mc_bud:%d",mic_capless_adjust_bud);
    return mic_capless_adjust_bud;
}

#define MC_TRIM_BUF_NUM        2
#define MC_TRIM_IRQ_POINTS     256
#define MC_TRIM_BUFS_SIZE      (MC_TRIM_BUF_NUM * MC_TRIM_IRQ_POINTS)

struct adc_mc_trim {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[MC_TRIM_BUFS_SIZE];    //align 4Bytes
};
static struct adc_mc_trim *adc_mc = NULL;

static void adc_mic_demo_output(void *priv, s16 *data, int len)
{
    putchar('o');
}

static struct audio_adc_private_param adc_private = {
    .mic_ldo_vsel   = 5,
    .mic_ldo_isel   = 2,
    .adca_reserved0 = 0,
    .adcb_reserved0 = 0,
};

static void audio_adc_mic_trim_open(u8 mic_idx, u8 gain)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    u16 mic_sr = 16000;
    printf("audio_adc_mic_trim_open:%d\n", mic_idx);
    audio_adc_init(&adc_hdl, &adc_private);
    adc_mc = zalloc(sizeof(struct adc_mc_trim));
    if (adc_mc) {
        adc_file_mic_open(&adc_mc->mic_ch, adc_cfg->mic_en_map);
        audio_adc_mic_set_sample_rate(&adc_mc->mic_ch, mic_sr);
        audio_adc_mic_set_buffs(&adc_mc->mic_ch, adc_mc->adc_buf, MC_TRIM_IRQ_POINTS * 2, MC_TRIM_BUF_NUM);
        adc_mc->adc_output.handler = adc_mic_demo_output;
        audio_adc_add_output_handler(&adc_hdl, &adc_mc->adc_output);
        audio_adc_mic_start(&adc_mc->mic_ch);
        printf("audio_adc_mic_trim open succ\n");
    } else {
        printf("audio_adc_mic_trim open err\n");
    }
}

static void audio_adc_mic_trim_close(u8 mic_idx)
{
    printf("audio_adc_mic_trim close:%d\n", mic_idx);
    if (adc_mc) {
        audio_adc_mic_close(&adc_mc->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &adc_mc->adc_output);
        free(adc_mc);
        adc_mc = NULL;
    }
}

static u8 audio_mic_capless_idle_query(void)
{
    return (mc_trim ? 0 : 1);
}
REGISTER_LP_TARGET(audio_mic_capless_device_lp_target) = {
    .name = "audio_mic_capless_device",
    .is_idle = audio_mic_capless_idle_query,
};

void mic_capless_trim_open(u8 mic_idx)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();
    log_info("mic_capless_trim_open:%d\n", mic_idx);
    u16 mic_ldo_tab[8] = {1300, 1400, 1500, 2000, 2200, 2400, 2600, 2800};
    u8 cur_bias_res = 0;
    u16 dtb_default = 6500;
    u16 mic_ldo = 0;
    mc_trim = zalloc(sizeof(MicCapless_trim_t));
    if (mc_trim) {
        if (mic_idx & AUDIO_ADC_MIC_0) {
            mc_trim->cur_bias_res = platform_cfg[0].mic_bias_rsel;
            cur_bias_res = platform_cfg[0].mic_bias_rsel;
            mic_ldo = mic_ldo_tab[adc_private_param.mic_ldo_vsel];
        }
        if (mic_idx & AUDIO_ADC_MIC_1) {
            mc_trim->cur_bias1_res = platform_cfg[1].mic_bias_rsel;
            cur_bias_res = platform_cfg[1].mic_bias_rsel;
            mic_ldo = mic_ldo_tab[adc_private_param.mic_ldo_vsel];
        }
        os_sem_create(&mc_trim->sem, 0);
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        audio_adc_mic_trim_open(adc_cfg->mic_en_map, app_var.aec_mic_gain);

        gpio_set_mode(IO_PORT_SPILT(IO_PORTA_01), PORT_HIGHZ);

        SFR(JL_ADDA->DAA_CON0, 9,  1,  1);     //AIN_A0_TEST
        adc_add_sample_ch(AD_CH_AUDIO);
        os_time_dly(40);/*mic上电延时*/
        u16 mic_bias_val = adc_get_voltage(AD_CH_AUDIO);
        log_info("mic_bias_val:%d,mic_ldo:%d\n", mic_bias_val, mic_ldo);

        /*debug mic上电延时是否足够
         *如果mic_bias_val电压采样值和下面debug模块采样值接近
         *表示延时足够，否则请自行调试mic上电延时*/
#if 0
        while (1) {
            os_time_dly(50);
            u16 mic_bias_test = adc_get_voltage(AD_CH_AUDIO);
            printf("mic_bias_test:%d\n", mic_bias_test);
        }
#endif/**/

        while (1) {
            wdt_clear();
            mic_bias_val = adc_get_voltage(AD_CH_AUDIO);
            dtb_default = mic_bias_val * 8196 / mic_ldo;
            log_info("mic_bias_val:%d,%d\n", mic_bias_val, dtb_default);
            if (mic_bias_val < 1500) {
                if (cur_bias_res < (sizeof(mic_bias_tab) - 1)) {
                    cur_bias_res++;
                } else {
                    log_info("mic_bias_res err\n");
                    break;
                }
                u16 bias_rsel = mic_bias_tab[cur_bias_res];
                log_info("cur_bias_res:%d = %d\n", cur_bias_res, bias_rsel);
                MIC_BIAS_RSEL(bias_rsel);
                os_time_dly(10);
            } else {
                log_info("mic_bias_val valid\n");
                break;
            }
        }
        if (mic_idx & AUDIO_ADC_MIC_0) {
            platform_cfg[0].mic_bias_rsel = cur_bias_res;
            /* SFR(JL_AUDIO->DAC_DTB,  0,   16,  dtb_default);	//dac dtb0 */
            mc_var.save_dtb = dtb_default;
        }
        if (mic_idx & AUDIO_ADC_MIC_1) {
            platform_cfg[1].mic_bias_rsel = cur_bias_res;
            /* SFR(JL_AUDIO->DAC_DTB,  16,  16,  dtb_default);	//dac dtb1 */
            mc_var.save_dtb1 = dtb_default;
        }
        log_info("mic_capless_trim_open succ:%d,%d\n", cur_bias_res, dtb_default);
    } else {
        log_info("mic_capless_trim_open err\n");
    }
    SFR(JL_ADDA->DAA_CON0, 9,  1,  0);     //AIN_A0_TEST
    adc_delete_ch(AD_CH_AUDIO);
}

void mic_capless_trim_exit(u8 trim_result)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();
    audio_adc_mic_trim_close(adc_cfg->mic_en_map);

    if (trim_result) {
        log_info("save trim value,bias_res:%d,dtb0:%d,dtb1:%d\n", platform_cfg[0].mic_bias_rsel, mc_var.save_dtb, mc_var.save_dtb1);
        log_info("save trim value,save_bias:%d,complete:%d\n", mc_trim->save_bias_res, mc_var.adjust_complete);
        if (mc_trim->save_bias_res) {
            syscfg_write(CFG_MC_BIAS, &platform_cfg[0].mic_bias_rsel, 1);
        }
        if (mc_var.adjust_complete) {
            save_capless_DTB();
        }
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD_DEFAULT;
        MC_LOG_E("mc_trim succ,use default adjust bud:%d\n", mic_capless_adjust_bud);
    } else {
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        MC_LOG_E("mc_trim err,use fast adjust bud:%d\n", mic_capless_adjust_bud);
    }

    /* SFR(JL_AUDIO->DAC_DTB,  0,   16,  mc_var.save_dtb);		//dac dtb0 */
    /* SFR(JL_AUDIO->DAC_DTB,  16,  16,  mc_var.save_dtb1);	//dac dtb1 */

    free(mc_trim);
    mc_trim = NULL;
    log_info("mic_capless_bias_adjust_exit\n");
}

void mic_capless_trim_result(u8 result)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();
    if (mc_trim) {
        /*预收敛*/
        log_info("mc_pre_auto_adjust_result:%d\n", result);
        if (mc_trim->trim && (mc_trim->trim < (MC_TRIM_P | MC_TRIM_N))) {
            mc_trim->save_bias_res = 1;
            platform_cfg[0].mic_bias_rsel = mc_trim->cur_bias_res;
        }
        os_sem_post(&mc_trim->sem);
    } else {
        /*运行过程收敛*/
        log_info("mc_run_auto_adjust_result:%d\n", result);
    }
}

void mic_capless_trim_init(int update)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();
    u8 por_flag = 0;
    u8 cur_por_flag = 0;
    if (platform_cfg[0].mic_mode != AUDIO_MIC_CAPLESS_MODE) {
        return;
    }
    /*
     *1.update
     *2.power_on_reset
     *3.pin reset
     */
    if (update || is_reset_source(P33_VDDIO_POR_RST) || (is_reset_source(P33_PPINR_RST))) {
        cur_por_flag = 0xA5;
    }
    syscfg_read(CFG_POR_FLAG, &por_flag, 1);
    log_info("POR flag:%x-%x", cur_por_flag, por_flag);
    if ((cur_por_flag == 0xA5) && (por_flag != cur_por_flag)) {
        //log_info("update POR flag");
        /*需要保存上电状态时因为开机有可能进入soft poweroff，下次开机的时候需要知道上次的开机状态*/
        syscfg_write(CFG_POR_FLAG, &cur_por_flag, 1);
    }
}

/*
 * 省电容mic预收敛
 */
void mic_capless_trim_run(void)
{
    struct adc_file_cfg *adc_cfg = audio_adc_file_get_cfg();
    struct adc_platform_cfg *platform_cfg = audio_adc_platform_get_cfg();

    if (!(adc_cfg->mic_en_map & (AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1))) {
        return;
    }

    if (platform_cfg[0].mic_mode != AUDIO_MIC_CAPLESS_MODE) {
        return;
    }

#if TCFG_MC_BIAS_AUTO_ADJUST
    /*自动校准流程：
     *step0:偏置电压校准
     *step1:预收敛(可选)
     *step2:动态跟踪
     */
    log_info("mic_capless_trim_run\n");

    mic_capless_trim_open(adc_cfg->mic_en_map);
    mc_var.mic_bias_valid = 1;
    mc_var.dtb_step_limit = 0;
#if TCFG_MC_CONVERGE_PRE
    int ret = os_sem_pend(&mc_trim->sem, MC_TRIM_TIMEOUT);
    if (ret == OS_TIMEOUT) {
        log_info("mc_trim timeout!\n");
        mic_capless_trim_exit(0);
    } else {
        log_info("mc_trim ok\n");
        mic_capless_trim_exit(1);
        mc_var.dtb_step_limit = TCFG_MC_DTB_STEP_LIMIT;
    }
#else
    mic_capless_trim_exit(1);
    mc_var.dtb_step_limit = TCFG_MC_DTB_STEP_LIMIT;
#endif/*TCFG_MC_CONVERGE_PRE*/
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
}

void ladc_capless_adjust_post(s32 dacr32, u8 begin)
{
    static s32 last_dacr32 = 0;
    static u8 check_cnt = 0;
    s32 dacr32_diff;

    /*adjust_begin,clear*/
    if (begin) {
        last_dacr32 = 0;
        check_cnt = 0;
        mc_var.adjust_complete = 0;
        mc_var.save_dtb = DACR32_DEFAULT;
        /* printf("DTB init_val:%d,%d\n", (JL_AUDIO->DAC_DTB & 0xFFFF), ((JL_AUDIO->DAC_DTB >> 16) & 0xFFFF)); */
        return;
    }
#if TCFG_MC_CONVERGE_TRACE
    printf("<%d>", dacr32);
#endif/*TCFG_MC_CONVERGE_TRACE*/
    if ((mc_var.adjust_complete == 0) && (mc_var.mic_bias_valid)) {
        if (++check_cnt > CHECK_INTERVAL) {
            check_cnt = 0;
            dacr32_diff = dacr32 - last_dacr32;
            //printf("[capless:%d-%d-%d]",dacr32,last_dacr32,dacr32_diff);
            last_dacr32 = dacr32;
            if (mc_var.adjust_complete == 0) {
                mc_var.save_dtb = dacr32;
            }

            /*调整稳定*/
            if ((dacr32_diff > -DIFF_RANGE) && (dacr32_diff < DIFF_RANGE)) {
                log_info("adjust_OK:%d\n", dacr32);
                mc_var.adjust_complete = 1;
#if TCFG_MC_BIAS_AUTO_ADJUST
                mic_capless_trim_result(MC_TRIM_SUCC);
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
            }
        }
    }
}

#endif/*TCFG_SUPPORT_MIC_CAPLESS*/

