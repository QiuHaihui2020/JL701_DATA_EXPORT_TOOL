#ifndef PLATFORM_ADC_API_H
#define PLATFORM_ADC_API_H

#include "asm/gpadc_hw.h"
#include "asm/efuse.h"

#define AD_CH_IO_VBAT_PORT        0//IO_PORTA_02   //选择一个有ADC功能IO口采集vbat电压，电压不能超过 vddio
#define ENABLE_PREEMPTIVE_MODE 1    //阻塞式采集使能
#define AD_CH_PMU_VBG_TRIM_NUM  32  //初始化时，VBG通道校准的采样次数
#define PMU_CH_SAMPLE_PERIOD    500 //CPU模式采样周期默认值 单位：ms
#define ADC_MAX_CH  10  //采集队列支持的最大通道数

struct adc_info_t { //adc采集队列信息结构体
    u32 jiffies;
    u32 ch;
    union {
        u16 value;
        float voltage;
    } v;
    u16 adc_voltage_mode;
    u16 sample_period;
};
extern u8 cur_ch;  //adc采集队列当前编号
extern u8 adc_clk_div; //adc时钟分频系数
// extern u16 vbat_voltage; //电池电压值，默认一直刷新
// extern u16 vbg_value; //vbg adc原始值，默认一直刷新
extern struct adc_info_t adc_queue[ADC_MAX_CH + ENABLE_PREEMPTIVE_MODE];  //采集队列声明

float adc_value_update(enum AD_CH ch, float adc_value_old, float adc_value_new); //vbat, vbg 值更新
void adc_init(void); //adc初始化
u32 adc_get_next_ch();    //获取采集队列中下一个通道的队列编号
u32 adc_set_sample_period(enum AD_CH ch, u32 ms); //设置一个指定通道的采样周期
u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_value);   //adc_value-->voltage   用传入的 vbg 计算
u32 adc_value_to_voltage_filter(u32 adc_value);   //adc_value-->voltage   用 vbg_value_array 数组均值计算

u32 adc_get_value(enum AD_CH ch);   //获取一个指定通道的原始值，从队列中获取
u32 adc_get_voltage(enum AD_CH ch); //获取一个指定通道的电压值，从队列中获取
u32 adc_get_value_blocking(enum AD_CH ch);    //阻塞式采集一个指定通道的adc原始值
u32 adc_get_value_blocking_filter(enum AD_CH ch, u32 sample_times);    //阻塞式采集一个指定通道的adc原始值,均值滤波
u32 adc_get_voltage_blocking(enum AD_CH ch);  //阻塞式采集一个指定通道的电压值（经过均值滤波处理）
u32 adc_cpu_mode_process(u32 adc_value);    //adc_isr 中断中，cpu模式的公共处理函数

void adc_update_vbg_value_restart(u8 cur_miovdd_level, u8 new_miovdd_level);//iovdd变化之后，重新计算vbg_value
u32 adc_get_vbg_voltage();
u32 adc_io2ch(int gpio);   //根据传入的GPIO，返回对应的ADC_CH
void adc_io_ch_set(enum AD_CH ch, u32 mode); //adc io通道 模式设置
void adc_internal_signal_to_io(u32 analog_ch, u16 adc_io); //将内部通道信号，接到IO口上，输出
u32 adc_add_sample_ch(enum AD_CH ch);   //添加一个指定的通道到采集队列
u32 adc_delete_ch(enum AD_CH ch);    //将一个指定的通道从采集队列中删除
void adc_sample(enum AD_CH ch, u32 ie); //启动一次cpu模式的adc采样
void adc_close();     //adc close
void adc_wait_enter_idle(); //等待adc进入空闲状态，才可进行阻塞式采集
void adc_set_enter_idle(); //设置 adc cpu模式为空闲状态
u16 adc_wait_pnd();   //cpu采集等待pnd
void adc_dma_sample(enum AD_CH ch, u16 *buf, u32 sample_times); //dma模式连续采集n次, 没有dma模块的软件方式连续采集
void adc_hw_init(void);    //adc初始化子函数

u32 adc_check_vbat_lowpower();  //兼容旧芯片，直接return 0
void adc_reset(); //复位adc内部队列的缓冲数据
#endif
