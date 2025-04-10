#ifndef  __CLOCK_H__
#define  __CLOCK_H__

#include "asm/clock_hal.h"

#define KHz_UNIT	(1000L)
#define MHz	        (1000000L)

enum pll_ref_source : u8 {
    PLL_REF_XOSC,       //外部晶振，单端模式
    PLL_REF_XOSC_DIFF,  //外部晶振，差分模式
    PLL_REF_LRC,
    PLL_REF_HRC,
    PLL_REF_RTC_OSC,
    PLL_REF_XCLK,
};

///VAD时钟源
#define 		VAD_CLOCK_USE_CLOSE					0 //VAD关闭
#define         VAD_CLOCK_USE_BTOSC                 1 //DVAD、ANALOG使用BTOSC
#define         VAD_CLOCK_USE_RC_AND_BTOSC          2 //DVAD使用RC、BTOSC直连ANALOG
#define         VAD_CLOCK_USE_PMU_STD12M            3 //DVAD使用BTOSC通过PMU配置的STD12M
#define         VAD_CLOCK_USE_LRC                   4 //DVAD使用LRC

//ANC时钟源
#define			ANC_CLOCK_USE_CLOSE					0 //ANC关闭，无需保持相关时钟
#define			ANC_CLOCK_USE_BTOSC					1 //ANC使用BTOSCX2时钟
#define			ANC_CLOCK_USE_PLL					2 //ANC使用PLL时钟



#define     SYS_24M     (24 * MHz)
#define     SYS_48M     (48 * MHz)
#define     SYS_60M     (60 * MHz)
#define     SYS_64M     (64 * MHz)
#define     SYS_96M     (96 * MHz)
#define     SYS_128M    (128 * MHz)
#define     SYS_160M    (160 * MHz)

void clock_dump(void);
void clock_core_voltage_dump();

//返回芯片支持的最高时钟频率
u32 clk_get_max_frequency();
int clk_early_init(enum pll_ref_source pll_ref, u32 ref_frequency, u32 pll_frequency);

/**
 * @brief clk_set 设置对应name时钟节点的时钟频率，实际频率会大于等于配置频率
 *
 * @param name
 * @param frequency 单位为Hz
 *
 * @return 小于0，标识对应时钟节点频率设置失败,整数为实际频率,
 */
int clk_set_api(const char *name, u32 frequency);

/**
 * @brief clk_get  获取对应name的时钟节点时钟频率
 *
 * @param name
 *
 * @return 返回频率，单位为Hz
 */
u32 clk_get(const char *name);


/**
 * @brief clock_set_sfc_max_freq
 * 使用前需要保证所使用的flash支持4bit 100Mhz 模式
 *
 * @param dual_max_freq for cmd 3BH BBH
 * @param quad_max_freq for cmd 6BH EBH
 */
void clk_set_sfc_max_freq(u32 dual_max_freq, u32 quad_max_freq);


enum clk_mode {
    CLOCK_MODE_ADAPTIVE = 0,
    CLOCK_MODE_USR,
};
/* ***************************************************************************/
/**
 * \Brief :         频率电压适配模式接口，支持动态配置频率电压为自适应或用户设置
 *
 * \Param :         mode    : CLOCK_MODE_ADAPTIVE 频率电压自适应使能 / CLOCK_MODE_USR 频率电压用户控制
 * \Param :         sys_dvdd: 用户设置值
 */
/* *****************************************************************************/
void clk_voltage_init(u8 mode, u8 sys_dvdd);
void clk_voltage_mode(u8 mode, u8 sys_dvdd);


/**
 * @brief clk_set_lowest_voltage 设置dvdd工作的最低 工作电压
 *
 * @param dvdd_lev  mic 工作时候 建议 SYSVDD_VOL_SEL_105V，关闭的时候设置为 SYSVDD_VOL_SEL_084V
 */
void clk_set_lowest_voltage(u32 dvdd_lev);



/*
 * system enter critical and exit critical handle
 * */
struct clock_critical_handler {
    void (*enter)();
    void (*exit)();
};

#define CLOCK_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler clock_##name \
		 SEC_USED(.clock_critical_txt) = {enter, exit};

extern struct clock_critical_handler clock_critical_handler_begin[];
extern struct clock_critical_handler clock_critical_handler_end[];

#define list_for_each_loop_clock_critical(h) \
	for (h=clock_critical_handler_begin; h<clock_critical_handler_end; h++)

extern struct clock_critical_handler hsb_critical_handler_begin[];
extern struct clock_critical_handler hsb_critical_handler_end[];

#define HSB_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler hsb_##name \
		 SEC(.hsb_critical_txt) = {enter, exit};

extern struct clock_critical_handler hsb_critical_handler_begin[];
extern struct clock_critical_handler hsb_critical_handler_end[];

#define list_for_each_loop_hsb_critical(h) \
	for (h=hsb_critical_handler_begin; h<hsb_critical_handler_end; h++)

#define LSB_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler lsb_##name \
		 SEC(.lsb_critical_txt) = {enter, exit};

extern struct clock_critical_handler lsb_critical_handler_begin[];
extern struct clock_critical_handler lsb_critical_handler_end[];

#define list_for_each_loop_lsb_critical(h) \
	for (h=lsb_critical_handler_begin; h<lsb_critical_handler_end; h++)





// 阻塞式延时
void delay_nops(u32 nops);
//delay function does not guarantee accuracy. The application may sleep longer than msecs under heavy load conditions.
void udelay(u32 us);
void mdelay(u32 ms);


u32 __get_lrc_hz();


/**
 * @brief 不需要输出时钟时需调clk_out_close关闭,否则一直占用och
 * @param gpio:参见枚举,固定io带入固定io; 有chx的可以选择任意io; 都有选择任意io
 * @param clk_src:详见枚举,备注多通道的可同时输出多路clk
 * @param div:详见枚举各通道div注释
 * return: 0:error(通道被占用,无法输出)
 */
int clk_out(u32 gpio, enum CLK_OUT_SOURCE clk_src, u32 div);
/**
 * @brief clk_out_close将关闭所有正在输出的clk_src时钟
 * @param gpio:不检查gpio,当clk_src正确直接配置高祖
 * @param clk_src:详见枚举,关闭该时钟输出
 *return: 0:error(时钟未曾输出)*/
int clk_out_close(u32 gpio, enum CLK_OUT_SOURCE clk_src);

/**
 * @brief 设置电压频率电压表最大挡位
 */
void clock_voltage_rising_step_max();
/**
 * @brief 设置电压频率电压表最小挡位
 */
void clock_voltage_falling_step_min();

void update_vdd_table(u8 val);
#endif  /*CLOCK_H*/
