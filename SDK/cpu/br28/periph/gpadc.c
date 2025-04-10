#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gpadc.data.bss")
#pragma data_seg(".gpadc.data")
#pragma const_seg(".gpadc.text.const")
#pragma code_seg(".gpadc.text")
#endif
#include "typedef.h"
#include "clock.h"
#include "gpadc.h"
#include "timer.h"
#include "init.h"
#include "asm/efuse.h"
#include "irq.h"
#include "asm/power_interface.h"
#include "jiffies.h"
#include "app_config.h"
#include "gpio.h"
#include "clock.h"
#include "asm/power_interface.h"

//br28

// static u16 adc_dma_buf[32] = {0};   //dma 模式存放数据的 buf
// static u16 adc_ch_date_buf[16]; //dma 模式经过处理后存数据的 buf
static u8 adc_ch_io_table[16] = {  //gpio->adc_ch 表
    IO_PORTA_00,
    IO_PORTA_05,
    IO_PORTA_06,
    IO_PORTA_08,
    IO_PORTC_04,
    IO_PORTC_05,
    IO_PORTB_01,
    IO_PORTB_02,
    IO_PORTB_05,
    IO_PORTB_09,
    IO_PORT_DP,
    IO_PORT_DM,
    IO_PORTG_00,
    IO_PORTG_01,
    IO_PORTG_05,
    IO_PORTG_07,
};

// enum {  //dma模式和cpu模式分时控制的标志位
//     ADC_CPU_SAMPLE_FORBID = 0,
//     ADC_CPU_SAMPLE_ALLOW,
//     ADC_CPU_SAMPLE_BUSY,
// };
// static u8 cpu_sample_flag = ADC_CPU_SAMPLE_FORBID;

static void clock_critical_enter(void)
{
}
static void adc_adjust_div(void)
{
    const u8 adc_div_table[] = {1, 6, 12, 24, 48, 72, 96, 128};
    const u32 lsb_clk = clk_get("lsb");
    adc_clk_div = 7;
    for (int i = 0; i < ARRAY_SIZE(adc_div_table); i++) {
        if (lsb_clk / adc_div_table[i] <= 1000000) {
            adc_clk_div = i;
            break;
        }
    }
}
CLOCK_CRITICAL_HANDLE_REG(saradc, clock_critical_enter, adc_adjust_div)


u32 adc_io2ch(int gpio)   //根据传入的GPIO，返回对应的ADC_CH
{
    for (u8 i = 0; i < ARRAY_SIZE(adc_ch_io_table); i++) {
        if (adc_ch_io_table[i] == gpio) {
            return (ADC_CH_TYPE_IO | i);
        }
    }
    printf("add_adc_ch io error!!! change other io_port!!!\n");
    return 0xffffffff; //未找到支持ADC的IO
}

void adc_io_ch_set(enum AD_CH ch, u32 mode) //adc io通道 模式设置
{

}

void adc_internal_signal_to_io(u32 analog_ch, u16 adc_io) //将内部通道信号，接到IO口上，输出
{
    gpio_set_mode(IO_PORT_SPILT(adc_io), PORT_HIGHZ);
    /* gpio_set_function(IO_PORT_SPILT(adc_io), PORT_FUNC_GPADC); */


    adc_sample(analog_ch, 0);
    u32 ch = adc_io2ch(adc_io);
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    SFR(JL_ADC->CON, 8, 4, adc_ch_sel);
    SFR(JL_ADC->CON, 21, 3, 0b111);
}

static void adc_audio_ch_select(u32 ch_sel) //Audio通道切换
{
    SFR(JL_ADDA->ADDA_CON1, 1, 1, 1);
    SFR(JL_ADDA->ADDA_CON0, 0, 12, 0);

    switch (ch_sel) {
    case AD_CH_AUDIO_VADADC:
        SFR(JL_ADDA->ADDA_CON0, 12, 1, 1);
        break;
    case AD_CH_AUDIO_VCM:
        SFR(JL_ADDA->ADDA_CON0, 11, 1, 1);
        break;
    case AD_CH_AUDIO_VBGLDO:
        SFR(JL_ADDA->ADDA_CON0, 10, 1, 1);
        break;
    case AD_CH_AUDIO_HPVDD:
        SFR(JL_ADDA->ADDA_CON0, 9, 1, 1);
        break;
    case AD_CH_AUDIO_RN:
        SFR(JL_ADDA->ADDA_CON0, 4, 1, 1);
        break;
    case AD_CH_AUDIO_RP:
        SFR(JL_ADDA->ADDA_CON0, 3, 1, 1);
        break;
    case AD_CH_AUDIO_LN:
        SFR(JL_ADDA->ADDA_CON0, 2, 1, 1);
        break;
    case AD_CH_AUDIO_LP:
        SFR(JL_ADDA->ADDA_CON0, 1, 1, 1);
        break;
    case AD_CH_AUDIO_DACVDD:
        SFR(JL_ADDA->ADDA_CON0, 0, 1, 1);
        break;
#if 0
    case AD_AUDIO_MICIN0:
        SFR(JL_ADDA->ADDA_CON0, 5, 4, 0);
        break;
    case AD_AUDIO_MICIN1:
        SFR(JL_ADDA->ADDA_CON0, 5, 4, 1);
        break;
    case AD_AUDIO_MICIN2:
        SFR(JL_ADDA->ADDA_CON0, 5, 4, 2);
        break;
    case AD_AUDIO_MICIN3:
        SFR(JL_ADDA->ADDA_CON0, 5, 4, 3);
        break;
#endif
    default:
        break;
    }
}

u32 adc_add_sample_ch(enum AD_CH ch)   //添加一个指定的通道到采集队列
{
    u32 adc_type_sel = ch & ADC_CH_MASK_TYPE_SEL;
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    printf("type = %x,ch = %x\n", adc_type_sel, adc_ch_sel);
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            break;
        } else if (adc_queue[i].ch == -1) {
            adc_queue[i].ch = ch;
            adc_queue[i].v.value = 1;
            adc_queue[i].sample_period = 0;

            switch (ch) {
            case AD_CH_PMU_VBAT:
                adc_queue[i].adc_voltage_mode = 1;
                break;
            case AD_CH_PMU_VTEMP:
                adc_queue[i].adc_voltage_mode = 1;
                break;
            default:
                adc_queue[i].adc_voltage_mode = 0;
                break;
            }
            // if (adc_type_sel == ADC_CH_TYPE_IO) {
            //     printf("IO dma sample\n");
            //     JL_ADC->CON &= ~BIT(4);
            //     SFR(JL_ADC->DMA_CON0, 16, 16, (JL_ADC->DMA_CON0 >> 16) + 1);//DMA长度加1
            //     JL_ADC->DMA_CON1 |= BIT(adc_ch_sel);//使能对应DMA通道
            // } else {
            //     /* JL_ADC->CON |= BIT(1); */
            // }
            printf("add sample ch 0x%x, i = %d\n", ch, i);
            break;
        }
    }
    return i;
}

u32 adc_delete_ch(enum AD_CH ch)    //将一个指定的通道从采集队列中删除
{
    u32 i = 0;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            adc_queue[i].ch = -1;
            break;
        }
    }
    return i;
}

void adc_sample(enum AD_CH ch, u32 ie) //启动一次cpu模式的adc采样
{
    u32 adc_con = 0;
    SFR(adc_con, 0, 3, adc_clk_div);//adc_clk 分频
    adc_con |= (0x1 << 12); //启动延时控制，实际启动延时为此数值*8个ADC时钟
    adc_con |= BIT(3) | BIT(4); //ana en
    adc_con |= BIT(6); //clr pend
    if (ie) {
        adc_con |= BIT(5);//ie
    }
    adc_con |= BIT(17);//clk en

    u32 adc_type_sel = ch & ADC_CH_MASK_TYPE_SEL;
    u16 adc_ch_sel = ch & ADC_CH_MASK_CH_SEL;
    SFR(adc_con, 21, 3, 0b010);//cpu adc test sel en
    SFR(adc_con, 18, 3, adc_type_sel >> 16);    //test sel
    switch (adc_type_sel) {
    case ADC_CH_TYPE_BT:
        break;
    case ADC_CH_TYPE_PMU:
        adc_pmu_ch_select(adc_ch_sel);
        break;
    case ADC_CH_TYPE_AUDIO:
        adc_audio_ch_select(ch);
        break;
    case ADC_CH_TYPE_LPCTM:
        break;
    case ADC_CH_TYPE_X32K:
        break;
    case ADC_CH_TYPE_PLL1:
        break;
    default:
        SFR(adc_con, 21, 3, 0b001); //cpu adc io sel en
        SFR(adc_con, 8, 4, adc_ch_sel);
        break;
    }
    JL_ADC->CON = adc_con;
    JL_ADC->CON |= BIT(6);//kistart
}
static void adc_wait_idle_timeout()
{
    u32 time_out_us = 1000;
    while (time_out_us--) {
        asm("csync");
        if (JL_ADC->CON & BIT(7)) {
            break;
        }
        if ((JL_ADC->CON & BIT(4)) == 0) {
            break;
        }
        udelay(1);
    }
}
void adc_wait_enter_idle() //等待adc进入空闲状态，才可进行阻塞式采集
{
    if (JL_ADC->CON & BIT(4)) {
        adc_wait_idle_timeout();
        adc_close();
    } else {
        return ;
    }
}
void adc_set_enter_idle() //设置 adc cpu模式为空闲状态
{
    /* JL_ADC->CON &= ~BIT(4);//en */
}
u16 adc_wait_pnd()   //cpu采集等待pnd
{
    adc_wait_idle_timeout();
    u32 adc_res = JL_ADC->RES;
    adc_close();
    return adc_res;
}

void adc_dma_sample(enum AD_CH ch, u16 *buf, u32 sample_times)
{
    /* JL_PORTA->DIR &= ~BIT(7); */
    /* JL_PORTA->OUT |= BIT(7); */

    adc_queue[ADC_MAX_CH].ch = ch;

    adc_wait_enter_idle();

    local_irq_disable();

    adc_sample(ch, 0);

    for (u32 i = 0; i < sample_times; i++) {
        adc_wait_idle_timeout();
        buf[i] = JL_ADC->RES;
        /* printf("res %d\n", buf[i]); */
        JL_ADC->CON |= BIT(6);
    }
    adc_close();

    adc_queue[ADC_MAX_CH].ch = -1;

    adc_set_enter_idle();

    local_irq_enable();

    /* JL_PORTA->OUT &= ~BIT(7); */
}

void adc_close()     //adc close
{
    JL_ADC->CON = BIT(17);//clock_en
    JL_ADC->CON = BIT(17) | BIT(6);
    JL_ADC->CON = BIT(6);
}

___interrupt
static void adc_isr()   //中断函数
{
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        adc_close();
        return;
    }

    if (JL_ADC->CON & BIT(7)) {
        u32 adc_value = JL_ADC->RES;

        if (adc_cpu_mode_process(adc_value)) {
            return;
        }
        cur_ch++;
        cur_ch = adc_get_next_ch();
        if (cur_ch == ADC_MAX_CH) {
            adc_close();
        } else {
            adc_sample(adc_queue[cur_ch].ch, 1);
        }
    }

}

static void adc_scan()  //定时函数，每 x ms启动一轮cpu模式采集
{
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        return;
    }
    if (JL_ADC->CON & BIT(4)) {
        return;
    }
    cur_ch = adc_get_next_ch();
    if (cur_ch == ADC_MAX_CH) {
        return;
    }
    adc_sample(adc_queue[cur_ch].ch, 1);
}

void adc_hw_init(void)    //adc初始化子函数
{
    memset(adc_queue, 0xff, sizeof(adc_queue));

    adc_close();

    adc_adjust_div();

    adc_add_sample_ch(AD_CH_LDOREF);
    adc_set_sample_period(AD_CH_LDOREF, PMU_CH_SAMPLE_PERIOD);

    adc_add_sample_ch(AD_CH_PMU_VBAT);
    adc_set_sample_period(AD_CH_PMU_VBAT, PMU_CH_SAMPLE_PERIOD);

    adc_add_sample_ch(AD_CH_PMU_VTEMP);
    adc_set_sample_period(AD_CH_PMU_VTEMP, PMU_CH_SAMPLE_PERIOD);

    u32 vbg_adc_value = 0;
    u32 vbg_min_value = -1;
    u32 vbg_max_value = 0;

    for (int i = 0; i < AD_CH_PMU_VBG_TRIM_NUM; i++) {
        u32 adc_value = adc_get_value_blocking(AD_CH_LDOREF);
        if (adc_value > vbg_max_value) {
            vbg_max_value = adc_value;
        } else if (adc_value < vbg_min_value) {
            vbg_min_value = adc_value;
        }
        vbg_adc_value += adc_value;
    }
    vbg_adc_value -= vbg_max_value;
    vbg_adc_value -= vbg_min_value;

    vbg_adc_value /= (AD_CH_PMU_VBG_TRIM_NUM - 2);
    adc_queue[0].v.value = vbg_adc_value;
    printf("LDOREF = %d\n", vbg_adc_value);

    u32 voltage = adc_get_voltage_blocking(AD_CH_PMU_VBAT);
    adc_queue[1].v.voltage = voltage;
    printf("vbat = %d mv\n", adc_get_voltage(AD_CH_PMU_VBAT) * 4);

    voltage = adc_get_voltage_blocking(AD_CH_PMU_VTEMP);
    adc_queue[2].v.voltage = voltage;
    printf("dtemp = %d mv\n", voltage);

    /* read_hpvdd_voltage(); */
    //切换到vbg通道

    request_irq(IRQ_GPADC_IDX, 1, adc_isr, 0);
    usr_timer_add(NULL, adc_scan,  5, 0);

    /* adc_add_sample_ch(adc_io2ch(IO_PORTA_06)); */
    /* adc_set_sample_period(adc_io2ch(IO_PORTA_06), PMU_CH_SAMPLE_PERIOD); */
    /* void adc_test_demo(); */
    /* usr_timer_add(NULL, adc_test_demo, 1000, 0); //打印信息 */
    /* u16 buf[32]; */
    /* adc_dma_sample(AD_CH_IO_PA6, buf, 32); */
    /* put_buf((u8 *)buf, 64); */
}

static void adc_test_demo()  //adc测试函数，根据需求搭建
{
    /* printf("\n\n%s() CHIP_ID :%x\n", __func__, get_chip_version()); */
    /* printf("%s() VBG:%d\n", __func__, adc_get_value(AD_CH_LDOREF)); */
    /* printf("%s() VBAT:%d mv\n", __func__, adc_get_voltage(AD_CH_PMU_VBAT) * 4); */
    /* printf("%s() DTEMP:%d\n", __func__, adc_get_voltage(AD_CH_PMU_VTEMP)); */
    /* printf("%s() PA6:%d\n", __func__, adc_get_value(adc_io2ch(IO_PORTA_06))); */
    /* printf("%s() PA6:%dmv\n", __func__, adc_get_voltage_blocking(adc_io2ch(IO_PORTA_06))); */
}

static u8 gpadc_idle_query(void)
{
    if (JL_ADC->CON & BIT(4)) {
        return 0; //不可以进入休眠
    }
    return 1; //可以进入休眠
}
REGISTER_LP_TARGET(gpadc_driver_target) = {
    .name = "gpadc",
    .is_idle = gpadc_idle_query,
};
