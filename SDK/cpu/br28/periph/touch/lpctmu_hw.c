#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lpctmu_hw.data.bss")
#pragma data_seg(".lpctmu_hw.data")
#pragma const_seg(".lpctmu_hw.text.const")
#pragma code_seg(".lpctmu_hw.text")
#endif
#include "system/includes.h"
#include "asm/power_interface.h"
#include "asm/lpctmu_hw.h"
#include "gpadc.h"

#if 1

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"
#else

#define log_debug   printf

#endif



static const u8 ch_port[LPCTMU_CHANNEL_SIZE] = {
    IO_PORTB_00,
    IO_PORTB_01,
    IO_PORTB_02,
    IO_PORTB_04,
    IO_PORTB_05,
};

static struct lpctmu_config_data *__this = NULL;

extern u32 __get_lrc_hz();



void lpctmu_send_m2p_cmd(enum CTMU_M2P_CMD cmd)
{
    P2M_CTMU_CMD_ACK = 0;
    M2P_CTMU_CMD = cmd;
    P11_SYSTEM->M2P_INT_SET = BIT(M2P_CTMU_INDEX);
    while (!(P2M_CTMU_CMD_ACK));
}


u32 lpctmu_get_cur_ch_by_idx(u32 ch_idx)
{
    ch_idx %= __this->ch_num;
    return __this->ch_list[ch_idx];
}

u32 lpctmu_get_idx_by_cur_ch(u32 cur_ch)
{
    for (u32 ch_idx = 0; ch_idx < __this->ch_num; ch_idx ++) {
        if (cur_ch == __this->ch_list[ch_idx]) {
            return ch_idx;
        }
    }
    return 0;
}

void lpctmu_port_init(u32 ch)
{
    u8 port = ch_port[ch];
    gpio_set_mode(port / 16, BIT(port % 16), PORT_HIGHZ);
    if (ch > 2) {
        ch ++;
    }
    SFR(P11_PORT->PB_SEL, ch, 1, 1);
    SFR(P11_PORT->PB_DIR, ch, 1, 1);
    SFR(P11_PORT->PB_DIE, ch, 1, 0);
    SFR(P11_PORT->PB_PU,  ch, 1, 0);
    SFR(P11_PORT->PB_PD,  ch, 1, 0);
}


void lpctmu_set_ana_hv_level(u32 level)
{
    SFR(P11_LPCTM->ANA0, 6, 2, level & 0b11);
}

u32 lpctmu_get_ana_hv_level(void)
{
    u32 level = (P11_LPCTM->ANA0 >> 6) & 0b11;
    return level;
}

void lpctmu_set_ana_lv_level(u32 level)
{
    SFR(P11_LPCTM->ANA0, 4, 2, level & 0b11);
}

u32 lpctmu_get_ana_lv_level(void)
{
    u32 level = (P11_LPCTM->ANA0 >> 4) & 0b11;
    return level;
}

void lpctmu_vsel_trim(void)
{
#if 0
    /* SFR(P11_LPCTM->ANA1, 1, 1, 1);//模拟测试使能 */
    SFR(P11_LPCTM->ANA1, 2, 1, 1);//使能HV
    u32 hv_vol = adc_get_voltage_blocking(AD_CH_LPCTMU);  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
    log_debug("hv_vol = %dmv", lv_vol);

    SFR(P11_LPCTM->ANA1, 2, 1, 0);//使能LV
    u32 lv_vol = adc_get_voltage_blocking(AD_CH_LPCTMU);  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
    log_debug("lv_vol = %dmv", lv_vol);
    /* SFR(P11_LPCTM->ANA1, 1, 1, 0);//模拟测试关闭 */
#endif

    lpctmu_set_ana_hv_level(__this->pdata->hv_level);
    lpctmu_set_ana_lv_level(__this->pdata->lv_level);
}


u32 lpctmu_get_charge_clk(void)
{
    u32 gpcnt_num = 0;
    u32 std24m_clk_khz = 24000;
    u32 charge_clk = std24m_clk_khz * gpcnt_num / 8192;
    return charge_clk;
}

void lpctmu_set_ana_cur_level(u32 ch, u32 cur_level)
{
    SFR(P11_LPCTM->ANA0, 1, 3, cur_level & 0b111);
}

u32 lpctmu_get_ana_cur_level(u32 ch)
{
    u32 level = (P11_LPCTM->ANA0 >> 1) & 0b111;
    return level;
}

void lpctmu_isel_trim(u8 ch)
{
    SFR(P11_LPCTM->CON1, 0, 5, ch + 1);//使能对应通道
    lpctmu_set_ana_cur_level(ch, __this->pdata->cur_level);
    /* u32 charge_clk = lpctmu_get_charge_clk(); */
}

void lpctmu_vsel_isel_trim(void)
{
    SFR(P11_LPCTM->ANA1, 3, 1, 1);//软件控制
    SFR(P11_LPCTM->ANA1, 0, 1, 1);//模拟模块偏置使能
    SFR(P11_LPCTM->ANA0, 0, 1, 1);//模拟模块使能
    SFR(P11_LPCTM->CON1, 0, 5, 0);//先不使能任何通道

    lpctmu_vsel_trim();

    __this->ch_idx = 0;
__isel_trim:

    lpctmu_isel_trim(__this->ch_list[__this->ch_idx]);
    __this->ch_idx ++;
    if (__this->ch_idx < __this->ch_num) {
        goto __isel_trim;
    } else {
        __this->ch_idx = 0;
    }
}

void lpctmu_lptimer_disable(void)
{
    P11_LPTMR3->CON0 &= ~BIT(0);
}

void lpctmu_lptimer_enable(void)
{
    P11_LPTMR3->CON0 |=  BIT(0);
    P11_LPTMR3->CON2 |=  BIT(8);
}

u32 lpctmu_lptimer_is_working(void)
{
    return (!!(P11_LPTMR3->CON0 & BIT(0)));
}

void lpctmu_lptimer_init(u32 scan_time)
{
    P11_LPTMR3->CON0 = 0x50;											//clr lc pending

    lpctmu_send_m2p_cmd(REQUEST_LPTMR_IRQ);

    u32 lrc_clk = __get_lrc_hz();
    P11_LPTMR3->CON2 = 0x03;								            //sel lrc; div1
    P11_LPTMR3->PRD = scan_time * lrc_clk / 1000;
    P11_LPTMR3->CON1 = 0x50;											//clr ex pending
    P11_LPTMR3->CON0 = 0x5b;											//clr lc pending | lc_ie1 | tmr_ctu | tmr_en

    P11_LPTMR3->CON2 |= BIT(8);
}

void lpctmu_dump(void)
{
    log_debug("P11_LPCTM->CON0 = 0x%x", P11_LPCTM->CON0);
    log_debug("P11_LPCTM->CON1 = 0x%x", P11_LPCTM->CON1);
    log_debug("P11_LPCTM->CON2 = 0x%x", P11_LPCTM->CON2);
    log_debug("P11_LPCTM->CON3 = 0x%x", P11_LPCTM->CON3);
    log_debug("P11_LPCTM->ANA0 = 0x%x", P11_LPCTM->ANA0);
    log_debug("P11_LPCTM->ANA1 = 0x%x", P11_LPCTM->ANA1);
    log_debug("P11_LPCTM->RES  =   %d", P11_LPCTM->RES);
}

void lpctmu_init(struct lpctmu_config_data *cfg_data)
{
    __this = cfg_data;
    if (!__this) {
        return;
    }

    u32 ch_idx, ch;
    u8 ch_en = 0;

    for (ch_idx = 0; ch_idx < __this->ch_num; ch_idx ++) {
        ch = __this->ch_list[ch_idx];
        ch_en |= BIT(ch);
        lpctmu_port_init(ch);
    }

    M2P_CTMU_CH_ENABLE = ch_en;
    M2P_CTMU_CH_WAKEUP_EN = __this->ch_wkp_en;
    M2P_CTMU_SCAN_TIME = __this->pdata->sample_scan_time;
    M2P_CTMU_LOWPOER_SCAN_TIME = __this->pdata->lowpower_sample_scan_time;

    if (!is_wakeup_source(PWR_WK_REASON_P11)) {

        lpctmu_lptimer_disable();
        memset(P11_LPCTM, 0, sizeof(P11_LPCTM_TypeDef));

        lpctmu_send_m2p_cmd(REQUEST_LPCTMU_IRQ);

        //时钟源配置
        u32 lpctmu_clk = __get_lrc_hz();
        log_debug("lpctmu_clk = %dHz", lpctmu_clk);
        SFR(P11_LPCTM->CON0, 3, 1, 0);//ctm_clk sel lrc200K
        SFR(P11_LPCTM->CON2, 0, 4, 2);//div = 2^2 = 4分频
        u8 det_prd = __this->pdata->sample_window_time * lpctmu_clk / 4 / 1000 - 1;
        SFR(P11_LPCTM->CON3, 0, 8, det_prd);

        SFR(P11_LPCTM->CON0, 1, 1, 0);//模块的中断不使能
        SFR(P11_LPCTM->CON0, 0, 1, 1);//模块总开关

        lpctmu_vsel_isel_trim();

        SFR(P11_LPCTM->ANA0, 0, 1, 0);
        SFR(P11_LPCTM->ANA1, 0, 8, 0);

        SFR(P11_LPCTM->CON1, 0, 5, ch_en);

        SFR(P11_LPCTM->CON0, 6, 1, 1);//CLR PEND
        SFR(P11_LPCTM->CON0, 2, 1, 1);//WKUP en
        SFR(P11_LPCTM->CON0, 0, 1, 0);//模块关闭，由lptimer_isr使能
        SFR(P11_LPCTM->CON0, 1, 1, 1);//模块中断使能

        lpctmu_lptimer_init(__this->pdata->sample_scan_time);

    } else {

        if (0 == lpctmu_lptimer_is_working()) {
            lpctmu_lptimer_init(__this->pdata->sample_scan_time);
        }
    }

    lpctmu_dump();
}

void lpctmu_disable(void)
{
    lpctmu_lptimer_disable();
}

void lpctmu_enable(void)
{
    lpctmu_lptimer_enable();
}

u32 lpctmu_is_sf_keep(void)
{
    if (!__this) {
        return 0;
    }
    if (lpctmu_lptimer_is_working()) {
        if (__this->softoff_keep_work == 0) {
            lpctmu_disable();
            return 0;
        } else {
            return 1;
        }
    }
    return 0;
}


