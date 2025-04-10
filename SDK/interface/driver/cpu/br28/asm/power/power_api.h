#ifndef __POWER_API_H__
#define __POWER_API_H__

#define PMU_NEW_FLOW 0
#define TRIM_WVDD  		 BIT(0)
#define TRIM_PVDD  		 BIT(1)
#define TRIM_IOVDD       BIT(2)

#include "power/low_power.h"

//=========================电源参数配置==================================
struct low_power_param {
    u8 config;				//低功耗使能，蓝牙&&系统空闲可进入低功耗
    u8 osc_type;			//低功耗晶振类型，btosc/lrc
    u32 btosc_hz;			//蓝牙晶振频率

    //vddiow_lev不需要配置，sleep、softoff模式会保持电压，除非配置使用nkeep_vddio(功耗相差不大)
    u8 vddiom_lev;			//vddiom
    u8 vddiow_lev;			//vddiow
    u8 nkeep_vddio;			//softoff模式下不保持vddio

    u32 osc_delay_us;		//低功耗晶振起振延时，为预留配置。
    u8 rtc_clk;				//rtc时钟源，softoff模式根据此配置是否保持住相应时钟
    u8 lpctmu_en;			//低功耗触摸，softoff模式根据此配置是否保持住该模块

    u8 mem_init_con;		//初始化ram电源
    u8 mem_lowpower_con;	//低功耗ram电源
    u8 rvdd2pvdd;			//低功耗外接dcdc
    u8 pvdd_dcdc_port;

    u8 lptmr_flow;			//低功耗参数由用户配置
    u32 t1;
    u32 t2;
    u32 t3_lrc;
    u32 t4_lrc;
    u32 t3_btosc;
    u32 t4_btosc;

    u8 flash_pg;			//iomc: 外置flash power gate

    u8 btosc_disable;
    u8 delay_us;

    u8 vddio_keep_type_pd;	//sleep keep vddio使用类型
    u8 vddio_keep_type_sf;  //soff keep vddio使用类型
};

//config
#define SLEEP_EN                            BIT(0)
#define DEEP_SLEEP_EN                       BIT(1)

enum {
    VDDIO_KEEP_TYPE_TRIM = 0,//vddio keep使用trim值：默认
    VDDIO_KEEP_TYPE_PG,		 //vddio keep使用 mvvdio：功耗代价100ua
};

//osc_type
enum {
    OSC_TYPE_LRC = 0,
    OSC_TYPE_BT_OSC,
};

//DCVDD电源模式
enum {
    PWR_LDO15,
    PWR_DCDC15,
};

//PVDD电源模式
enum PVDD_MODE {
    PWR_PVDD_LDO,
    PWR_PVDD_DCDC,
};

//==============================软关机参数配置============================
//软关机会复位寄存器，该参数为传给rom配置的参数。
struct soft_flag0_t {
    u8 wdt_dis: 1;
    u8 poweroff: 1;
    u8 is_port_b: 1;
    u8 lvd_en: 1;
    u8 pmu_en: 1;
    u8 iov2_ldomode: 1;
    u8 res: 2;
};

struct soft_flag1_t {
    u8 usbdp: 2;
    u8 usbdm: 2;
    u8 uart_key_port: 1;
    u8 ldoin: 3;
};

struct soft_flag2_t {
    u8 pg2: 4;
    u8 pg3: 4;
};

struct soft_flag3_t {
    u8 pg4: 4;
    u8 res: 4;
};

struct soft_flag4_t {
    u8 fast_boot: 1;
    u8 flash_stable_delay_sel: 2;
    u8 res: 5;
};

struct soft_flag5_t {
    u8 mvddio: 3;
    u8 wvbg: 4;
    u8 res: 1;
};

struct boot_soft_flag_t {
    union {
        struct soft_flag0_t boot_ctrl;
        u8 value;
    } flag0;
    union {
        struct soft_flag1_t misc;
        u8 value;
    } flag1;
    union {
        struct soft_flag2_t pg2_pg3;
        u8 value;
    } flag2;
    union {
        struct soft_flag3_t pg4_res;
        u8 value;
    } flag3;
    union {
        struct soft_flag4_t fast_boot_ctrl;
        u8 value;
    } flag4;
    union {
        struct soft_flag5_t level;
        u8 value;
    } flag5;
};

enum soft_flag_io_stage {
    SOFTFLAG_HIGH_RESISTANCE,
    SOFTFLAG_PU,
    SOFTFLAG_PD,

    SOFTFLAG_OUT0,
    SOFTFLAG_OUT0_HD0,
    SOFTFLAG_OUT0_HD,
    SOFTFLAG_OUT0_HD0_HD,

    SOFTFLAG_OUT1,
    SOFTFLAG_OUT1_HD0,
    SOFTFLAG_OUT1_HD,
    SOFTFLAG_OUT1_HD0_HD,
};

//==============================电源接口============================

#define AT_VOLATILE_RAM_CODE_POWER        AT(.power_driver.text.cache.L1)
#define AT_VOLATILE_CACHE_CODE_POWER      AT(.power_driver.text.cache.L2)

void power_set_mode(u8 mode);

void power_set_pvdd_mode(enum PVDD_MODE mode);

u8 get_pvdd_dcdc_cfg();

void power_init(const struct low_power_param *param);

void power_keep_dacvdd_en(u8 en);

void sdpg_config(int enable);

void p11_init(void);

u8 get_wvdd_trim_level();

u8 get_pvdd_level();

u8 get_pvdd_trim_level();

u8 get_miovdd_trim_level();

u8 get_wiovdd_trim_level();

void store_pmu_trim_value_to_vm(u8 wvdd_level, u8 pvdd_level, u8 miovdd_lev, u8 wiovdd_lev);

u8 load_pmu_trim_value_from_vm();

void volatage_trim_init();

void power_early_init(u32 arg);

void power_later_init(u32 arg);

//==============================sleep接口============================
u32 get_rch_hz();

void cap_rch_enable();

void cap_rch_disable();

//==============================soft接口============================

void mask_softflag_config(struct boot_soft_flag_t *softflag);

void power_set_callback(u8 mode, void (*powerdown_enter)(u8 step), void (*powerdown_exit)(u32), void (*soft_poweroff_enter)(void));

//==============================system接口============================
u32 get_lptmr_usec(void);

#endif
