#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_link.data.bss")
#pragma data_seg(".audio_link.data")
#pragma const_seg(".audio_link.text.const")
#pragma code_seg(".audio_link.text")
#endif
/*****************************************************************
>file name : lib/media/cpu/br22/audio_link.c
>author :
>create time : Fri 7 Dec 2018 14:59:12 PM CST
*****************************************************************/
#include "includes.h"
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_link.h"
#include "sdk_config.h"
#include "clock.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma   code_seg(".audio_link.text")
#pragma   data_seg(".audio_link.data")
#pragma  const_seg(".audio_link.text.const")
#endif
#if TCFG_IIS_NODE_ENABLE

#define IIS_IO_INIT_AFTER_ALINK_START  1// alink_start之后，再初始化输出io,避免iis起始io输出不定态引起一声杂音的问题
/* #define ALINK_DEBUG_INFO */
#ifdef ALINK_DEBUG_INFO
#define alink_printf  printf
#else
#define alink_printf(...)
#endif

static u32 *ALNK0_BUF_ADR[] = {
    (u32 *)(&(JL_ALNK0->ADR0)),
    (u32 *)(&(JL_ALNK0->ADR1)),
    (u32 *)(&(JL_ALNK0->ADR2)),
    (u32 *)(&(JL_ALNK0->ADR3)),
};

static u32 *ALNK0_HWPTR[] = {
    (u32 *)(&(JL_ALNK0->HWPTR0)),
    (u32 *)(&(JL_ALNK0->HWPTR1)),
    (u32 *)(&(JL_ALNK0->HWPTR2)),
    (u32 *)(&(JL_ALNK0->HWPTR3)),
};

static u32 *ALNK0_SWPTR[] = {
    (u32 *)(&(JL_ALNK0->SWPTR0)),
    (u32 *)(&(JL_ALNK0->SWPTR1)),
    (u32 *)(&(JL_ALNK0->SWPTR2)),
    (u32 *)(&(JL_ALNK0->SWPTR3)),
};

static u32 *ALNK0_SHN[] = {
    (u32 *)(&(JL_ALNK0->SHN0)),
    (u32 *)(&(JL_ALNK0->SHN1)),
    (u32 *)(&(JL_ALNK0->SHN2)),
    (u32 *)(&(JL_ALNK0->SHN3)),
};

static u32 PFI_ALNK0_DAT[] = {
    PFI_ALNK0_DAT0,
    PFI_ALNK0_DAT1,
    PFI_ALNK0_DAT2,
    PFI_ALNK0_DAT3,
};

static u32 FO_ALNK0_DAT[] = {
    FO_ALNK0_DAT0,
    FO_ALNK0_DAT1,
    FO_ALNK0_DAT2,
    FO_ALNK0_DAT3,
};


enum {
    MCLK_11M2896K = 0,
    MCLK_12M288K
};

enum {
    MCLK_EXTERNAL	= 0u,
    MCLK_SYS_CLK		,
    MCLK_OSC_CLK 		,
    MCLK_PLL_CLK		,
};

enum {
    MCLK_DIV_1		= 0u,
    MCLK_DIV_2			,
    MCLK_DIV_4			,
    MCLK_DIV_8			,
    MCLK_DIV_16			,
};

enum {
    MCLK_LRDIV_EX	= 0u,
    MCLK_LRDIV_64FS		,
    MCLK_LRDIV_128FS	,
    MCLK_LRDIV_192FS	,
    MCLK_LRDIV_256FS	,
    MCLK_LRDIV_384FS	,
    MCLK_LRDIV_512FS	,
    MCLK_LRDIV_768FS	,
};

static ALINK_PARM *p_alink0_parm = NULL;

s32 *alink_get_addr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return NULL;
    }
    s32 *buf_addr = NULL;
    buf_addr = (s32 *)(*ALNK0_BUF_ADR[hw_channel_parm->ch_idx]);
    return buf_addr;
}

u32 alink_get_shn(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int shn = 0;
    shn = *ALNK0_SHN[hw_channel_parm->ch_idx];
    return shn;
}

void alink_set_shn(void *hw_channel, u32 len)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_SHN[hw_channel_parm->ch_idx] = len;
    __asm_csync();
}

__attribute__((always_inline))
void alink_set_shn_with_no_sync(void *hw_channel, u32 len)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_SHN[hw_channel_parm->ch_idx] = len;

}

u32 alink_get_swptr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int swptr = 0;
    swptr = *ALNK0_SWPTR[hw_channel_parm->ch_idx];
    return swptr;
}

void alink_set_swptr(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_SWPTR[hw_channel_parm->ch_idx] = value;
    *ALNK0_HWPTR[hw_channel_parm->ch_idx] = value;
}

u32 alink_get_hwptr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int hwptr = 0;
    hwptr = *ALNK0_HWPTR[hw_channel_parm->ch_idx];
    return hwptr;
}

u32 alink_get_len(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    u32 len = 0;

    len = JL_ALNK0->LEN;

    return len;
}
void alink_set_hwptr(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_HWPTR[hw_channel_parm->ch_idx] = value;
}

void alink_set_ch_ie(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    ALINK_CHx_IE(hw_channel_parm->module, hw_channel_parm->ch_idx, value);
}

int alink_get_ch_ie(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int ret =  ALINK_CHx_IE_GET(hw_channel_parm->module, hw_channel_parm->ch_idx);
    return ret;
}

void alink_clr_ch_pnd(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    ALINK_CLR_CHx_PND(hw_channel_parm->module, hw_channel_parm->ch_idx);
}


//==================================================
static void alink_io_in_init(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_INPUT_FLOATING);
}

static void alink_io_out_init(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_OUTPUT_LOW);
}

static void *alink_addr(void *hw_alink, u8 ch)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    s32 *buf_addr = NULL; //can be used
    u32 buf_index;
    buf_addr = (s32 *)alink_get_addr(&hw_alink_parm->ch_cfg[ch]);

    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        s32 swptr = alink_get_swptr(&hw_alink_parm->ch_cfg[ch]);
        buf_addr = (s32 *)(buf_addr + swptr);				//buf为dma起始地址,计算当前循环buf写数位置
        return buf_addr;
    } else {
        u8 index_table[4] = {12, 13, 14, 15};
        u8 bit_index = index_table[ch];
        buf_index = (ALINK_SEL(hw_alink_parm->module, CON0) & BIT(bit_index)) ? 0 : 1;
        buf_addr = buf_addr + ((hw_alink_parm->dma_len / 8) * buf_index);
        return buf_addr;
    }
    return NULL;
}

static u32 alink_isr_get_len(void *hw_alink, u8 ch, u32 *remain)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    u32 len = 0;
    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        s32 shn = alink_get_shn(&hw_alink_parm->ch_cfg[ch]);
        s32 swptr = alink_get_swptr(&hw_alink_parm->ch_cfg[ch]);
        u32 dma_len = alink_get_len(&hw_alink_parm->ch_cfg[ch]);
        if ((swptr + shn) >= dma_len) {					//处理边界情况
            len = dma_len - swptr;
            *remain = (shn - len) * 4;
        } else {
            len = shn;
            *remain = 0;
        }
        len = len * 4;
    } else {
        len = hw_alink_parm->dma_len / 2;
    }

    return len;
}


___interrupt
static void alink0_dma_isr(void)
{

    s32 *buf_addr = NULL ;
    u8 ch = 0;
    u32 len = 0;

    audio_alink_lock(0);
    u32 reg = JL_ALNK0->CON2;

    for (ch = 0; ch < 4; ch++) {
        if (!((reg >> 12) & BIT(ch))) {
            continue;
        }
        if (!((reg >> 4) & BIT(ch))) {
            continue;
        }
        buf_addr = (s32 *)alink_addr(p_alink0_parm, ch);
        if (p_alink0_parm->ch_cfg[ch].isr_cb) {
            if (p_alink0_parm->buf_mode == ALINK_BUF_CIRCLE) {//循环buf
                if (p_alink0_parm->ch_cfg[ch].dir == ALINK_DIR_TX) {//循环buf输出
                    u32 remain;
                    len = alink_isr_get_len(p_alink0_parm, ch, &remain);
                    p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
                } else {
                    u32 remain = 0;
                    int out_len = (JL_ALNK0->PNS & 0xffff) << 2;
                    len = alink_isr_get_len(p_alink0_parm, ch, &remain);
                    if (len >= out_len) {
                        len = out_len;
                        p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
                    } else {
                        p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
                        if ((len + remain) >= out_len) {
                            if (remain) {
                                buf_addr = (s32 *)(*ALNK0_BUF_ADR[ch]);
                                p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, out_len - len);
                            }
                        }
                    }
                    alink_set_shn(&p_alink0_parm->ch_cfg[ch], out_len / 4);
                }
            } else {//兵乓buf
                p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, p_alink0_parm->dma_len / 2);
            }
        } else {
            if (p_alink0_parm->buf_mode == ALINK_BUF_CIRCLE) {//循环buf
                if (p_alink0_parm->ch_cfg[ch].dir == ALINK_DIR_RX) {//循环buf接收,回调函数未配置时，由驱动内部更新shn
                    alink_set_shn(&p_alink0_parm->ch_cfg[ch], (JL_ALNK0->PNS & 0xffff));
                }
            }
        }
        ALINK_CLR_CHx_PND(p_alink0_parm->module, ch);
    }
    audio_alink_unlock(0);
}


extern u32 clock_alink_sel(u8 module, u32 frequency);
extern u32 clk_get_pll_target_frequency();
static void alink_sr(void *hw_alink, u32 rate)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    hw_alink_parm->sample_rate = rate;
    u8 module = hw_alink_parm->module;
    alink_printf("ALINK_SR = %d\n", rate);
    if (hw_alink_parm->role == ALINK_ROLE_SLAVE) {
        ALINK_LRDIV(module, MCLK_LRDIV_EX);
        return;
    }
    u32 pll_target_frequency = clk_get_pll_target_frequency() / MHz;
    switch (rate) {
    case ALINK_SR_192000:
        /*12.288Mhz 64fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_1);
        ALINK_LRDIV(module, MCLK_LRDIV_64FS);
        break ;

    case ALINK_SR_96000:
        /*12.288Mhz 128fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_1);
        ALINK_LRDIV(module, MCLK_LRDIV_128FS);
        break ;

    case ALINK_SR_48000:
        /*12.288Mhz 256fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_1);
        ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        break ;

    case ALINK_SR_44100:
        /*11.289Mhz 256fs*/
        clock_alink_sel(module, 11289600);
        ALINK_MDIV(module, MCLK_DIV_1);
        if ((pll_target_frequency == 192) || (pll_target_frequency == 288)) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        } else if (pll_target_frequency == 240) {
            ALINK_LRDIV(module, MCLK_LRDIV_128FS);
        }
        break ;

    case ALINK_SR_32000:
        /*12.288Mhz 384fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_1);
        ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        break ;

    case ALINK_SR_24000:
        /*12.288Mhz 512fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_2);
        ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        break ;

    case ALINK_SR_22050:
        /*11.289Mhz 512fs*/
        clock_alink_sel(module, 11289600);
        ALINK_MDIV(module, MCLK_DIV_1);
        if ((pll_target_frequency == 192) || (pll_target_frequency == 288)) {
            ALINK_LRDIV(module, MCLK_LRDIV_512FS);
        } else if (pll_target_frequency == 240) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        }
        break ;

    case ALINK_SR_16000:
        /*12.288/2Mhz 384fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_2);
        ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        break ;

    case ALINK_SR_12000:
        /*12.288/2Mhz 512fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_4);
        ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        break ;

    case ALINK_SR_11025:
        /*11.289/2Mhz 512fs*/
        clock_alink_sel(module, 11289600);
        ALINK_MDIV(module, MCLK_DIV_4);
        if ((pll_target_frequency == 192) || (pll_target_frequency == 288)) {
            ALINK_LRDIV(module, MCLK_LRDIV_512FS);
        } else if (pll_target_frequency == 240) {
            ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        }
        break ;

    case ALINK_SR_8000:
        /*12.288/4Mhz 384fs*/
        clock_alink_sel(module, 12288000);
        ALINK_MDIV(module, MCLK_DIV_4);
        ALINK_LRDIV(module, MCLK_LRDIV_384FS);
        break ;

    default:
        //44100
        /*11.289Mhz 256fs*/
        clock_alink_sel(module, 11289600);
        ALINK_MDIV(module, MCLK_DIV_1);
        ALINK_LRDIV(module, MCLK_LRDIV_256FS);
        break;
    }

}
void alink_set_irq_handler(void *hw_alink, void *hw_channel, void *priv, void (*handle)(void *priv, void *addr, int len))
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;

    u8 ch_idx = hw_channel_parm->ch_idx;
    hw_alink_parm->ch_cfg[ch_idx].private_data = hw_channel_parm->private_data;
    hw_alink_parm->ch_cfg[ch_idx].isr_cb = hw_channel_parm->isr_cb;
}
void *alink_channel_init_base(void *hw_alink, void *hw_channel, u8 ch_idx)
{
    if (!hw_alink) {
        return NULL;
    }

    if (!hw_channel) {
        return NULL;
    }
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    u8 module = hw_alink_parm->module;

    if (hw_alink_parm->ch_cfg[ch_idx].ch_en) {
        printf("This channel % is en\n", ch_idx);
        return &hw_alink_parm->ch_cfg[ch_idx];
    }
    if (ch_idx >= 4) {
        printf("This module's channels are fully registered, fail!!!!!\n");
        return NULL;
    }
    hw_alink_parm->ch_cfg[ch_idx].bit_width = hw_alink_parm->bitwide;
    hw_alink_parm->ch_cfg[ch_idx].module = module;
    hw_alink_parm->ch_cfg[ch_idx].ch_idx = ch_idx;
    hw_alink_parm->ch_cfg[ch_idx].data_io = hw_channel_parm->data_io;
    hw_alink_parm->ch_cfg[ch_idx].ch_ie = hw_channel_parm->ch_ie;
    hw_alink_parm->ch_cfg[ch_idx].dir = hw_channel_parm->dir;
    hw_alink_parm->ch_cfg[ch_idx].private_data = hw_channel_parm->private_data;
    hw_alink_parm->ch_cfg[ch_idx].isr_cb = hw_channel_parm->isr_cb;
    hw_alink_parm->ch_cfg[ch_idx].buf = dma_malloc(hw_alink_parm->dma_len);
    memset(hw_alink_parm->ch_cfg[ch_idx].buf, 0x00, hw_alink_parm->dma_len);

    /* printf(">>> alink_channel_init %x\n", hw_alink_parm->ch_cfg[ch_idx].buf); */
    u32 *buf_addr;

    //===================================//
    //           ALNK CH DMA BUF         //
    //===================================//
    buf_addr = ALNK0_BUF_ADR[ch_idx];
    *buf_addr = (u32)(hw_alink_parm->ch_cfg[ch_idx].buf);

    ALINK_CHx_DIR_MODE(module, ch_idx, hw_alink_parm->ch_cfg[ch_idx].dir);
    //===================================//
    //           ALNK工作模式            //
    //===================================//
    if (hw_alink_parm->mode > ALINK_MD_IIS_RALIGN) {
        ALINK_CHx_MODE_SEL(module, ch_idx, (hw_alink_parm->mode - ALINK_MD_IIS_RALIGN));
    } else {
        ALINK_CHx_MODE_SEL(module, ch_idx, (hw_alink_parm->mode));
    }
    //===================================//
    //          ALNK CH DAT IO INIT      //
    //===================================//
    if (hw_alink_parm->ch_cfg[ch_idx].dir == ALINK_DIR_RX) {
        alink_io_in_init(hw_alink_parm->ch_cfg[ch_idx].data_io);
        gpio_set_fun_input_port(hw_alink_parm->ch_cfg[ch_idx].data_io, PFI_ALNK0_DAT[ch_idx]);
    } else {
#if !IIS_IO_INIT_AFTER_ALINK_START
        alink_io_out_init(hw_alink_parm->ch_cfg[ch_idx].data_io);
        gpio_set_fun_output_port(hw_alink_parm->ch_cfg[ch_idx].data_io, FO_ALNK0_DAT[ch_idx], 1, 1);
#endif
    }

    ALINK_CLR_CHx_PND(module, ch_idx);
    ALINK_CHx_IE(module, ch_idx, hw_alink_parm->ch_cfg[ch_idx].ch_ie);

    hw_alink_parm->ch_cfg[ch_idx].ch_en = 1;
    alink_printf("alink%d, ch%d init, 0x%x\n", module, ch_idx, (int)&hw_alink_parm->ch_cfg[ch_idx]);

    hw_alink_parm->init_cnt++;
    return &hw_alink_parm->ch_cfg[ch_idx];
}
void alink_channel_io_init(void *hw_alink, void *hw_channel, u8 ch_idx)
{
    if (!hw_alink) {
        return ;
    }

    if (!hw_channel) {
        return ;
    }

#if IIS_IO_INIT_AFTER_ALINK_START
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    /* struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel; */

    //===================================//
    //          ALNK CH DAT IO INIT      //
    //===================================//
    if (hw_alink_parm->ch_cfg[ch_idx].dir == ALINK_DIR_RX) {
    } else {
        alink_io_out_init(hw_alink_parm->ch_cfg[ch_idx].data_io);

        gpio_set_fun_output_port(hw_alink_parm->ch_cfg[ch_idx].data_io, FO_ALNK0_DAT[ch_idx], 1, 1);


    }
#endif
}
void *alink_channel_init(void *hw_alink, void *hw_channel)
{
    u8 ch_idx;
    void *ret = NULL;
    for (ch_idx = 0; ch_idx < 4; ch_idx++) {
        ret = alink_channel_init_base(hw_alink, hw_channel, ch_idx);
        if (ret) {
            return ret;
        }
    }
    return NULL;
}

void alink_info_dump()
{
    alink_printf("JL_ALNK0->CON0 = 0x%x", JL_ALNK0->CON0);
    alink_printf("JL_ALNK0->CON1 = 0x%x", JL_ALNK0->CON1);
    alink_printf("JL_ALNK0->CON2 = 0x%x", JL_ALNK0->CON2);
    alink_printf("JL_ALNK0->CON3 = 0x%x", JL_ALNK0->CON3);
    alink_printf("JL_ALNK0->LEN = 0x%x", JL_ALNK0->LEN);
    alink_printf("JL_ALNK0->ADR0 = 0x%x", JL_ALNK0->ADR0);
    alink_printf("JL_ALNK0->ADR1 = 0x%x", JL_ALNK0->ADR1);
    alink_printf("JL_ALNK0->ADR2 = 0x%x", JL_ALNK0->ADR2);
    alink_printf("JL_ALNK0->ADR3 = 0x%x", JL_ALNK0->ADR3);
    alink_printf("JL_ALNK0->PNS = 0x%x", JL_ALNK0->PNS);
    alink_printf("JL_ALNK0->HWPTR0 = 0x%x", JL_ALNK0->HWPTR0);
    alink_printf("JL_ALNK0->HWPTR1 = 0x%x", JL_ALNK0->HWPTR1);
    alink_printf("JL_ALNK0->HWPTR2 = 0x%x", JL_ALNK0->HWPTR2);
    alink_printf("JL_ALNK0->HWPTR3 = 0x%x", JL_ALNK0->HWPTR3);
    alink_printf("JL_ALNK0->SWPTR0 = 0x%x", JL_ALNK0->SWPTR0);
    alink_printf("JL_ALNK0->SWPTR1 = 0x%x", JL_ALNK0->SWPTR1);
    alink_printf("JL_ALNK0->SWPTR2 = 0x%x", JL_ALNK0->SWPTR2);
    alink_printf("JL_ALNK0->SWPTR3 = 0x%x", JL_ALNK0->SWPTR3);
    alink_printf("JL_ALNK0->SHN0 = 0x%x", JL_ALNK0->SHN0);
    alink_printf("JL_ALNK0->SHN1 = 0x%x", JL_ALNK0->SHN1);
    alink_printf("JL_ALNK0->SHN2 = 0x%x", JL_ALNK0->SHN2);
    alink_printf("JL_ALNK0->SHN3 = 0x%x", JL_ALNK0->SHN3);
}
void alink_set_da2sync_ch(void *hw_alink)
{
    if (hw_alink == NULL) {
        return;
    }

    ALINK_PARM *hw_alink_user = (ALINK_PARM *)hw_alink;		//传入参数
    //选用与蓝牙同步关联的通道
    u8 ch_idx = hw_alink_user->da2sync_ch;
    if (ch_idx != 0xff) {
        alink_printf("module %d, sel ch_idx %d\n", hw_alink_user->module, ch_idx);
        ALINK_DA2BTSRC_SEL(hw_alink_user->module, ch_idx);
    }
}
void *alink_init(void *hw_alink)
{
    if (hw_alink == NULL) {
        return NULL;
    }
    ALINK_PARM *hw_alink_user = (ALINK_PARM *)hw_alink;		//传入参数
    ALINK_PARM *hw_alink_parm = NULL;						//驱动赋值参数
    u8 module = hw_alink_user->module;
    if (module) {
        alink_printf("br28 alink have one hw\n");
    }
    hw_alink_parm = p_alink0_parm;
    if (hw_alink_parm && hw_alink_parm->init_cnt != 0) {	//ALNK0已被初始化过
        return hw_alink_parm;
    } else if (!hw_alink_parm) {							//ALNK0初始化
        p_alink0_parm = zalloc(sizeof(ALINK_PARM));
        hw_alink_parm = p_alink0_parm;
    }

    hw_alink_parm->module 	= hw_alink_user->module;
    hw_alink_parm->mclk_io 	= hw_alink_user->mclk_io;
    hw_alink_parm->sclk_io 	= hw_alink_user->sclk_io;
    hw_alink_parm->lrclk_io = hw_alink_user->lrclk_io;
    hw_alink_parm->mode 	= hw_alink_user->mode;
    hw_alink_parm->role 	= hw_alink_user->role;
    hw_alink_parm->clk_mode = hw_alink_user->clk_mode;
    hw_alink_parm->bitwide 	= hw_alink_user->bitwide;
    hw_alink_parm->dma_len 	= hw_alink_user->dma_len;
    hw_alink_parm->buf_mode = hw_alink_user->buf_mode;
    hw_alink_parm->sample_rate = hw_alink_user->sample_rate;
    hw_alink_parm->sclk_per_frame = hw_alink_user->sclk_per_frame;
    ALNK_CON_RESET(module);
    ALNK_HWPTR_RESET(module);
    ALNK_SWPTR_RESET(module);
    ALNK_ADR_RESET(module);
    ALNK_SHN_RESET(module);
    ALNK_PNS_RESET(module);
    ALINK_CLR_ALL_PND(module);
    //选用与蓝牙同步关联的通道
    u8 ch_idx = hw_alink_user->da2sync_ch;
    if (ch_idx != 0xff) {
        alink_printf("module %d, sel ch_idx %d\n", module, ch_idx);
        ALINK_DA2BTSRC_SEL(module, ch_idx);
    }


    ALINK_MSRC(module, MCLK_PLL_CLK);	/*MCLK source*/

    //===================================//
    //        输出时钟配置               //
    //===================================//
    if (hw_alink_parm->role == ALINK_ROLE_MASTER) {
        //主机输出时钟
        if (hw_alink_parm->mclk_io != ALINK_CLK_OUPUT_DISABLE) {
            alink_io_out_init(hw_alink_parm->mclk_io);
            gpio_set_fun_output_port(hw_alink_parm->mclk_io, FO_ALNK0_MCLK, 1, 1);
            ALINK_MOE(module, 1);				/*MCLK output to IO*/
        }
        if ((hw_alink_parm->sclk_io != ALINK_CLK_OUPUT_DISABLE) && (hw_alink_parm->lrclk_io != ALINK_CLK_OUPUT_DISABLE)) {
            alink_io_out_init(hw_alink_parm->lrclk_io);
            gpio_set_fun_output_port(hw_alink_parm->lrclk_io, FO_ALNK0_LRCK, 1, 1);
            alink_io_out_init(hw_alink_parm->sclk_io);
            gpio_set_fun_output_port(hw_alink_parm->sclk_io, FO_ALNK0_SCLK, 1, 1);
            ALINK_SOE(module, 1);				/*SCLK/LRCK output to IO*/
        }
    } else {
        //从机输入时钟
        alink_io_in_init(hw_alink_parm->mclk_io);
        alink_io_in_init(hw_alink_parm->sclk_io);
        alink_io_in_init(hw_alink_parm->lrclk_io);
        gpio_set_fun_input_port(hw_alink_parm->mclk_io, PFI_ALNK0_MCLK);
        gpio_set_fun_input_port(hw_alink_parm->sclk_io, PFI_ALNK0_SCLK);
        gpio_set_fun_input_port(hw_alink_parm->lrclk_io, PFI_ALNK0_LRCK);
        ALINK_MOE(module, 0);				/*MCLK input to IO*/
        ALINK_SOE(module, 0);				/*SCLK/LRCK output to IO*/
    }
    //===================================//
    //        基本模式/扩展模式          //
    //===================================//
    ALINK_DSPE(module, hw_alink_parm->mode >> 2);

    //===================================//
    //         数据位宽16/32bit          //
    //===================================//
    //注意: 配置了24bit, 一定要选ALINK_FRAME_64SCLK, 因为sclk32 x 2才会有24bit;
    if (hw_alink_parm->bitwide == ALINK_LEN_24BIT) {
        ASSERT(hw_alink_parm->sclk_per_frame == ALINK_FRAME_64SCLK);
        ALINK_24BIT_MODE(module);
        //一个点需要4bytes, LR = 2, 双buf = 2
    } else {
        ALINK_16BIT_MODE(module);
        //一个点需要2bytes, LR = 2, 双buf = 2
    }
    //===================================//
    //             时钟边沿选择          //
    //===================================//
    SCLKINV(module, hw_alink_parm->clk_mode);
    //===================================//
    //            每帧数据sclk个数       //
    //===================================//
    F32_EN(module, hw_alink_parm->sclk_per_frame);
    //===================================//
    //            设置数据采样率       	 //
    //===================================//
    alink_sr(hw_alink_parm, hw_alink_parm->sample_rate);

    //===================================//
    //            设置DMA MODE       	 //
    //===================================//
    ALINK_DMA_MODE_SEL(module, hw_alink_parm->buf_mode);
    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        ALINK_OPNS_SET(module, hw_alink_parm->dma_len / 16);
        if (hw_alink_parm->rx_pns) {
            ALINK_IPNS_SET(module, hw_alink_parm->rx_pns);
        } else {
            ALINK_IPNS_SET(module, hw_alink_parm->dma_len / 16);
        }
        //一个点需要2bytes, LR = 2
        ALINK_LEN_SET(module, hw_alink_parm->dma_len / 4);
    } else {
        //一个点需要2bytes, LR = 2, 双buf = 2
        ALINK_LEN_SET(module, hw_alink_parm->dma_len / 8);
    }
    request_irq(IRQ_ALINK0_IDX, 3, alink0_dma_isr, 0);

    return hw_alink_parm;
}


void alink_channel_close(void *hw_alink, void *hw_channel)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;

    if (!hw_alink_parm) {
        return;
    }

    if (!hw_channel_parm) {
        return;
    }
    ALINK_CHx_CLOSE(hw_channel_parm->module, hw_channel_parm->ch_idx);
    ALINK_CHx_IE(hw_channel_parm->module, hw_channel_parm->ch_idx, 0);

    gpio_set_mode(IO_PORT_SPILT(hw_channel_parm->data_io), PORT_HIGHZ);
    if (hw_channel_parm->buf) {
        dma_free(hw_channel_parm->buf);
        hw_channel_parm->buf = NULL;
    }
    hw_channel_parm->ch_en = 0;
    hw_alink_parm->init_cnt--;
    alink_printf("alink_ch %d closed cnt %d\n", hw_channel_parm->ch_idx, hw_alink_parm->init_cnt);
}


int alink_start(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (!hw_alink_parm) {
        return -1;
    }
    ALINK_EN(hw_alink_parm->module, 1);
    return 0;
}

int alink_uninit_base(void *hw_alink, void *hw_channel)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (!hw_alink_parm) {
        return 0;
    }

    alink_printf("audio_link_exit\n");


    if (hw_alink_parm->init_cnt == 0) {
        ALINK_EN(hw_alink_parm->module, 0);
        if (hw_alink_parm->role == ALINK_ROLE_MASTER) {
            if (hw_alink_parm->mclk_io != ALINK_CLK_OUPUT_DISABLE) {
                gpio_disable_fun_output_port(hw_alink_parm->mclk_io);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->mclk_io), PORT_HIGHZ);
            }
            if ((hw_alink_parm->sclk_io != ALINK_CLK_OUPUT_DISABLE) && (hw_alink_parm->lrclk_io != ALINK_CLK_OUPUT_DISABLE)) {
                gpio_disable_fun_output_port(hw_alink_parm->sclk_io);
                gpio_disable_fun_output_port(hw_alink_parm->lrclk_io);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->sclk_io), PORT_HIGHZ);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->lrclk_io), PORT_HIGHZ);
            }
        } else {
            gpio_disable_fun_input_port(PFI_ALNK0_MCLK);
            gpio_disable_fun_input_port(PFI_ALNK0_SCLK);
            gpio_disable_fun_input_port(PFI_ALNK0_LRCK);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->mclk_io), PORT_HIGHZ);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->sclk_io), PORT_HIGHZ);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->lrclk_io), PORT_HIGHZ);
        }

        hw_alink_parm->init_cnt = 0;
        if (p_alink0_parm) {
            free(p_alink0_parm);
            p_alink0_parm = NULL;
        }
        alink_printf("audio_link_exit OK\n");
        return 1;
    }

    return 0;
}
int alink_uninit(void *hw_alink, void *hw_channel)
{
    alink_channel_close(hw_alink, hw_channel);
    return alink_uninit_base(hw_alink, hw_channel);
}

void alink_uninit_force(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;

    if (!hw_alink_parm) {
        return;
    }

    ALINK_EN(hw_alink_parm->module, 0);
    for (int i = 0; i < 4; i++) {
        alink_channel_close(hw_alink, &hw_alink_parm->ch_cfg[i]);
    }
    ALNK_CON_RESET(hw_alink_parm->module);
    hw_alink_parm->init_cnt = 0;

    if (p_alink0_parm) {
        free(p_alink0_parm);
        p_alink0_parm = NULL;
    }

    alink_printf("audio_link_force_exit OK\n");
}
int alink_get_sr(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (hw_alink_parm) {
        return hw_alink_parm->sample_rate;
    } else {
        return -1;
    }
}

int alink_set_sr(void *hw_alink, u32 sr)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;

    if (!hw_alink_parm) {
        return -1;
    }

    if (hw_alink_parm->sample_rate == sr) {
        return 0;
    }

    u8 is_alink_en = (ALINK_SEL(hw_alink_parm->module, CON0) & BIT(11));

    if (is_alink_en) {
        ALINK_EN(hw_alink_parm->module, 0);
    }

    alink_sr(hw_alink_parm, sr);

    if (is_alink_en) {
        ALINK_EN(hw_alink_parm->module, 1);
    }

    return 0;
}

u32 alink_get_dma_len(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (hw_alink_parm) {
        return ALINK_FIFO_LEN(hw_alink_parm->module);
    }
    return 0;
}

void alink_set_tx_pns(void *hw_alink, u32 len)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (hw_alink_parm) {
        ALINK_OPNS_SET(hw_alink_parm->module, len);
    }
}

void alink_set_rx_pns(void *hw_alink, u32 len)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (hw_alink_parm) {
        ALINK_IPNS_SET(hw_alink_parm->module, len);
    }
}



#endif


