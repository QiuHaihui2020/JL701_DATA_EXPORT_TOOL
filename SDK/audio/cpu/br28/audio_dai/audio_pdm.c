#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_pdm.data.bss")
#pragma data_seg(".audio_pdm.data")
#pragma const_seg(".audio_pdm.text.const")
#pragma code_seg(".audio_pdm.text")
#endif
#include "audio_pdm.h"
#include "includes.h"
#include "asm/includes.h"
#include "asm/dac.h"
#include "media/includes.h"

/* #define PLNK_DEBUG_ENABLE */

#ifdef PLNK_DEBUG_ENABLE
#define plnk_printf 		y_printf
#else
#define plnk_printf(...)
#endif

static PLNK_PARM *__this = NULL;
#define PLNK_CLK	24000000

/*00:3order,01:2order,1x:1order*/
const u8 plnk_cic_order_reg[3] = {2, 1, 0};
#define PLNK_CIC_ORDER	3

static void plnk_set_shift_scale(u8 order, u8 m)
{
    u32 cic_r = __this->sclk_fre / __this->sr;
    cic_r = cic_r * m;
    u64 shift_sum = 1;
    u32	scale_sum;
    u8 shift_dir = 0;
    u8 shift_sel = 0;
    u8 scale_sel = 0;

    /* calculate shift_dir */
    for (u8 cnt = 0; cnt < order; cnt++) {
        shift_sum *= cic_r;
    }

    plnk_printf("shift_sum = %d", shift_sum);
    if (shift_sum < 4096) {
        shift_dir = 1;
    } else {
        shift_dir = 0;
    }

    /* calculate shift_sel */
    shift_sum = shift_sum * 15;
    for (shift_sel = 0; shift_sel <= 31; shift_sel++) {
        plnk_printf("shift_sum = %d, shift_sel = %d", shift_sum, shift_sel);
        if (shift_sum < 32768 && shift_sum > 16384) {
            break;
        }
        if (shift_dir == 1) {
            shift_sum = shift_sum * 2;
        } else if (shift_dir == 0) {
            shift_sum = shift_sum / 2;
        }
    }

    /* calculate scale_sel */
    for (scale_sel = 0; scale_sel <= 7; scale_sel++) {
        scale_sum = shift_sum * (8 + scale_sel) / 8;
        plnk_printf("scale_sum = %d, scale_sel = %d", scale_sum, scale_sel);
        if (scale_sum > 32768) {
            break;
        }
    }
    scale_sel = scale_sel - 1;

    plnk_printf("order = %d, m = %d, shift_dir = %d, shift_sel = %d, scale_sel = %d\n", order, m, shift_dir, shift_sel, scale_sel);

    PLNK_CH0_SHDIR(shift_dir);
    PLNK_CH1_SHDIR(shift_dir);

    PLNK_CH0_SHIFT(shift_sel);
    PLNK_CH1_SHIFT(shift_sel);

    PLNK_CH0_SCALE(scale_sel);
    PLNK_CH1_SCALE(scale_sel);
}


___interrupt
static void plnk_isr(void)
{
    u8 buf_flag;
    u32 *buf = (u32 *)__this->buf;
    u32 len = __this->dma_len / 4;
    u32 *data_buf;
    if (PLNK_PND_IS()) {
        PLNK_PND_CLR();
        buf_flag = PLNK_USING_BUF() ? 0 : 1;
        data_buf = (u32 *)buf;
        data_buf += buf_flag * len;
        if (__this && __this->isr_cb) {
            __this->isr_cb(__this->private_data, data_buf, __this->dma_len);
        }
    }
}

static void plnk_info_dump()
{
    plnk_printf("PLNK_CON = 0x%x", JL_PLNK->CON);
    plnk_printf("PLNK_CON1 = 0x%x", JL_PLNK->CON1);
    plnk_printf("PLNK_LEN = 0x%x", JL_PLNK->LEN);
    plnk_printf("PLNK_ADR = 0x%x", JL_PLNK->ADR);
    plnk_printf("PLNK_DOR = 0x%x", JL_PLNK->DOR);
    plnk_printf("ASS_CLK_CON = 0x%x", JL_ASS->CLK_CON);
}

static void plnk_sr_set(u32 sr)
{
    PLNK_CLK_EN(0);
    u16 plnk_dor = __this->sclk_fre / sr - 1;
    u16 sclk_div = PLNK_CLK / __this->sclk_fre - 1;
    plnk_printf("PLNK_SR = %d, dor = %d, sclk_dov = %d\n", sr, plnk_dor, sclk_div);
    PLNK_DOR_SET(plnk_dor);
    PLNK_SCLK_DIV(sclk_div);
    PLNK_CLK_EN(1);
}

static void plnk_sclk_io_init(u8 port)
{
    gpio_set_mode(IO_PORT_SPILT(port), PORT_OUTPUT_HIGH);
    gpio_set_fun_output_port(port, FO_PLNK_SCLK, 0, 1);
}

static void plnk_data_io_init(u8 ch_index, u8 port)
{
    gpio_set_mode(IO_PORT_SPILT(port), PORT_INPUT_FLOATING);
    if (ch_index) {
        gpio_set_fun_input_port(port, PFI_PLNK_DAT1);
    } else {
        gpio_set_fun_input_port(port, PFI_PLNK_DAT0);
    }
}

void *plnk_init(void *hw_plink)
{
    if (hw_plink == NULL) {
        return NULL;
    }
    __this = (PLNK_PARM *)hw_plink;

    u8 m = 2;
    __this->ch_num = __this->ch_cfg[0].en + __this->ch_cfg[1].en;

    PLNK_RESET();

    PLNK_DMA_LEN(__this->dma_len / 2);
    __this->buf = dma_malloc(__this->dma_len  * 2 * __this->ch_num);
    memset(__this->buf, 0x00, __this->dma_len  * 2 * __this->ch_num);
    ASSERT(__this->buf);
    PLNK_BUF_SET((u32)__this->buf);

    PLNK_CH0_DFDLY(m - 1);
    PLNK_CH1_DFDLY(m - 1);
    PLNK_CH0_ORDER(plnk_cic_order_reg[PLNK_CIC_ORDER - 1]);
    PLNK_CH1_ORDER(plnk_cic_order_reg[PLNK_CIC_ORDER - 1]);

    plnk_set_shift_scale(PLNK_CIC_ORDER, m);

    PLNK_CH0_MD(__this->ch_cfg[0].mode);
    PLNK_CH1_MD(__this->ch_cfg[1].mode);

    PLNK_CH0_MIC_SEL(__this->ch_cfg[0].mic_type);
    PLNK_CH1_MIC_SEL(__this->ch_cfg[1].mic_type);

    plnk_sr_set(__this->sr);

    plnk_sclk_io_init(__this->sclk_io);

    if (__this->data_cfg[0].en) {
        plnk_data_io_init(0, __this->data_cfg[0].io);
    }

    if (__this->data_cfg[1].en) {
        plnk_data_io_init(1, __this->data_cfg[1].io);
    }

    PLNK_PND_CLR();

    request_irq(IRQ_PDM_LINK_IDX, 3, plnk_isr, 0);

    return __this;
}

void plnk_start(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_CH0_EN(__this->ch_cfg[0].en);
    PLNK_CH1_EN(__this->ch_cfg[1].en);
    PLNK_PND_CLR();
    PLNK_IE(1);
    plnk_info_dump();
}

void plnk_stop(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_PND_CLR();
    PLNK_IE(0);
    PLNK_CH0_EN(0);
    PLNK_CH1_EN(0);
    plnk_info_dump();
}

void plnk_uninit(void *hw_plink)
{
    if (!hw_plink) {
        return;
    }

    PLNK_PND_CLR();
    PLNK_RESET();

    if (__this->buf) {
        dma_free(__this->buf);
        __this->buf = NULL;
    }
    __this = NULL;
}



