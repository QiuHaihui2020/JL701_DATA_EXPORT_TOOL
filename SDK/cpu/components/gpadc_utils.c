#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".gpadc_utils.data.bss")
#pragma data_seg(".gpadc_utils.data")
#pragma const_seg(".gpadc_utils.text.const")
#pragma code_seg(".gpadc_utils.text")
#endif
#include "typedef.h"
#include "gpadc.h"
#include "jiffies.h"
#include "clock.h"

u8 cur_ch = ADC_MAX_CH;  //adc采集队列当前编号
u8 adc_clk_div = 48; //adc时钟分频系数

struct adc_info_t adc_queue[ADC_MAX_CH + ENABLE_PREEMPTIVE_MODE];  //采集队列声明

float adc_value_update(enum AD_CH ch, float adc_value_old, float adc_value_new) //vbat, vbg 值更新
{
    float ret_value;
#if AD_CH_IO_VBAT_PORT
    if (ch == AD_CH_IO_VBAT_PORT) {
        return ret_value = (adc_value_old * 0.8f + adc_value_new * 0.2f); //这里可以调用自己的滤波算法
    }
#endif
    return ret_value = (adc_value_old * 0.8f + adc_value_new * 0.2f);
}


void adc_init(void) //adc初始化
{
    if (adc_queue[0].sample_period) { //避免重复初始化adc
        return;
    }
    adc_close();
    adc_hw_init();
}

u32 adc_get_next_ch()    //获取采集队列中下一个通道的队列编号
{
    if (cur_ch == ADC_MAX_CH) {
        cur_ch = 0;
    }
    for (int i = cur_ch; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch != -1) {
            if (adc_queue[i].sample_period) {
                if (time_before(adc_queue[i].jiffies, jiffies)) {
                    adc_queue[i].jiffies += adc_queue[i].sample_period;
                    /* printf("prd---> %d %d\n", jiffies, adc_queue[i].jiffies); */
                } else {
                    continue;
                }
            }

            return i;
        }
    }
    return ADC_MAX_CH;
}

u32 adc_set_sample_period(enum AD_CH ch, u32 ms) //设置一个指定通道的采样周期
{
    u32 i;
    for (i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            adc_queue[i].sample_period = msecs_to_jiffies(ms);
            adc_queue[i].jiffies = msecs_to_jiffies(ms) + jiffies;
            break;
        }
    }
    return i;
}

void adc_update_vbg_value_restart(u8 cur_miovdd_level, u8 new_miovdd_level)
{
    printf("cur = %d, new = %d\n", cur_miovdd_level, new_miovdd_level);
    u32 cur_miovdd_vol = 2100 + cur_miovdd_level * 100;
    u32 new_miovdd_vol = 2100 + new_miovdd_level * 100;
    adc_queue[0].v.value = adc_queue[0].v.value * cur_miovdd_vol / new_miovdd_vol;
}

u32 adc_get_vbg_voltage()
{
    if (ADC_VBG_DATA_WIDTH == 0) {
        return ADC_VBG_CENTER;
    }
    int data = efuse_get_gpadc_vbg_trim();
    int sign = (data >> ADC_VBG_DATA_WIDTH) & 0x01;
    data = data & ((1 << ADC_VBG_DATA_WIDTH) - 1);
    if (sign == 1) {
        return  ADC_VBG_CENTER - data * ADC_VBG_TRIM_STEP;
    } else {
        return  ADC_VBG_CENTER + data * ADC_VBG_TRIM_STEP;
    }
}
//adc_value-->voltage   用传入的 vbg 计算
u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_value)
{
    u32 tmp = adc_get_vbg_voltage();
    if (adc_vbg == 0) {
        adc_vbg = 1;        //防止div0异常
    }
    u32 adc_res = adc_value * tmp / adc_vbg;
    return adc_res;
}

//adc_value-->voltage   用 vbg_value_array 数组均值计算
u32 adc_value_to_voltage_filter(u32 adc_value)
{
    u32 adc_vbg = adc_get_value(AD_CH_LDOREF);
    return adc_value_to_voltage(adc_vbg, adc_value);
}

u32 adc_get_value(enum AD_CH ch)   //获取一个指定通道的原始值，从队列中获取
{
    for (int i = 0; i < ADC_MAX_CH; i++) {
        if (adc_queue[i].ch == ch) {
            /* printf("get ch %d, %d\n", adc_queue[i].v.value, i); */
            return adc_queue[i].v.value;
        }
    }
    /* printf("func:%s,line:%d\n", __func__, __LINE__); */
    return 1;
}

u32 adc_get_voltage(enum AD_CH ch) //获取一个指定通道的电压值，从队列中获取
{
    for (int i = 0; i < ADC_MAX_CH + ENABLE_PREEMPTIVE_MODE; i++) {
        if (ch == adc_queue[i].ch && adc_queue[i].adc_voltage_mode == 1) {
            return adc_queue[i].v.voltage;
        }
    }

    u32 adc_vbg = adc_get_value(AD_CH_LDOREF);
    u32 adc_res;
    if (ch == AD_CH_IOVDD) {
        adc_res = 1023;
    } else {
        adc_res = adc_get_value(ch);
    }
    return adc_value_to_voltage(adc_vbg, adc_res);
}

u32 adc_get_value_blocking(enum AD_CH ch)    //阻塞式采集一个指定通道的adc原始值
{
    adc_queue[ADC_MAX_CH].ch = ch;

    adc_wait_enter_idle();

    local_irq_disable();

    adc_sample(adc_queue[ADC_MAX_CH].ch, 0);

    adc_queue[ADC_MAX_CH].v.value = adc_wait_pnd();

    adc_queue[ADC_MAX_CH].ch = -1;

    adc_set_enter_idle();

    local_irq_enable();

    return adc_queue[ADC_MAX_CH].v.value;
}

u32 adc_get_value_blocking_filter(enum AD_CH ch, u32 sample_times)
{
    u32 ch_adc_value = 0;
    u32 ch_min_value = -1;
    u32 ch_max_value = 0;

    if (sample_times <= 2) {
        sample_times = 3;
    }
    for (int i = 0; i < sample_times; i++) {
        if (ch == AD_CH_IOVDD) {
            break;
        }

        u32 adc_value = adc_get_value_blocking(ch);

        if (adc_value > ch_max_value) {
            ch_max_value = adc_value;
        }
        if (adc_value < ch_min_value) {
            ch_min_value = adc_value;
        }
        ch_adc_value += adc_value;
    }

    if (ch == AD_CH_IOVDD) {
        ch_adc_value = 1023;
    } else {
        ch_adc_value -= ch_max_value;
        ch_adc_value -= ch_min_value;

        ch_adc_value /= sample_times - 2;
    }

    /* printf("%s() %d ch: %d min: %d max: %d  ", __func__, __LINE__, ch_adc_value, ch_min_value, ch_max_value); */
    return ch_adc_value;
}

u32 adc_get_voltage_blocking(enum AD_CH ch)  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
{
    u32 vbg_adc_value = 0;
    u32 vbg_min_value = -1;
    u32 vbg_max_value = 0;

    u32 ch_adc_value = 0;
    u32 ch_min_value = -1;
    u32 ch_max_value = 0;

    for (int i = 0; i < 12; i++) {

        u32 adc_value = adc_get_value_blocking(AD_CH_LDOREF);
        if (adc_value > vbg_max_value) {
            vbg_max_value = adc_value;
        }
        if (adc_value < vbg_min_value) {
            vbg_min_value = adc_value;
        }

        vbg_adc_value += adc_value;

        if (ch == AD_CH_IOVDD) {
            continue;
        }

        adc_value = adc_get_value_blocking(ch);

        if (adc_value > ch_max_value) {
            ch_max_value = adc_value;
        }
        if (adc_value < ch_min_value) {
            ch_min_value = adc_value;
        }
        ch_adc_value += adc_value;
    }

    vbg_adc_value -= vbg_max_value;
    vbg_adc_value -= vbg_min_value;

    vbg_adc_value /= 10;
    /* printf("%s() %d vbg: %d min: %d max: %d  ", __func__, __LINE__, vbg_adc_value, vbg_min_value, vbg_max_value); */


    if (ch == AD_CH_IOVDD) {
        ch_adc_value = 1023;
    } else {
        ch_adc_value -= ch_max_value;
        ch_adc_value -= ch_min_value;

        ch_adc_value /= 10;
    }

    /* printf("%s() %d ch: %d min: %d max: %d  ", __func__, __LINE__, ch_adc_value, ch_min_value, ch_max_value); */
    return adc_value_to_voltage(vbg_adc_value, ch_adc_value);
}

u32 adc_cpu_mode_process(u32 adc_value)    //adc_isr 中断中，cpu模式的公共处理函数
{
    if (adc_queue[cur_ch].adc_voltage_mode == 1) {
        u32 vbg_value = adc_queue[0].v.value;
        u32 voltage = adc_value_to_voltage(vbg_value, adc_value);
        adc_queue[cur_ch].v.voltage = adc_value_update(adc_queue[cur_ch].ch, adc_queue[cur_ch].v.voltage, voltage);
        /* printf("%d ad[%x]: %d %d", cur_ch, adc_queue[cur_ch].ch, adc_value, adc_queue[cur_ch].v.voltage); */
    } else  if (adc_queue[cur_ch].ch == AD_CH_LDOREF) {
        adc_queue[0].v.value = adc_value_update(AD_CH_LDOREF, adc_queue[0].v.value, adc_value);
    } else {
        adc_queue[cur_ch].v.value = adc_value;
    }
    /* printf("%d ad[%x]: %d %d", cur_ch, adc_queue[cur_ch].ch, adc_value, adc_queue[cur_ch].v.voltage); */
    if (adc_queue[ADC_MAX_CH].ch != -1) {
        adc_close();
        return 1;
    }
    return 0;
}

u32 adc_check_vbat_lowpower()
{
    return 0;
}
void adc_reset()
{
    for (u8 ch = 0; ch < ADC_MAX_CH; ch++) {
        if (adc_queue[ch].adc_voltage_mode == 1) {
            u32 voltage = adc_get_voltage_blocking(adc_queue[ch].ch);
            adc_queue[ch].v.voltage = voltage;
            printf("ch:%d, voltage:%dmv\n", ch, voltage);
        } else if (adc_queue[ch].ch == AD_CH_LDOREF) {
            u32 adc_value = adc_get_value_blocking_filter(AD_CH_LDOREF, 12);
            adc_queue[0].v.value = adc_value;
            printf("LDOREF = %d\n", adc_value);
        }
    }
}
