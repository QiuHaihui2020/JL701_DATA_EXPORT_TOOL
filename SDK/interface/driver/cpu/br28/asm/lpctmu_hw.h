#ifndef _LPCTMU_HW_H_
#define _LPCTMU_HW_H_

#include "typedef.h"


#define LPCTMU_CHANNEL_SIZE         5

#define LPCTMU_ANA_CFG_ADAPTIVE     0


enum {
    LPCTMU_VH_065V,
    LPCTMU_VH_070V,
    LPCTMU_VH_075V,
    LPCTMU_VH_080V,
};

enum {
    LPCTMU_VL_020V,
    LPCTMU_VL_025V,
    LPCTMU_VL_030V,
    LPCTMU_VL_035V,
};

enum {
    LPCTMU_ISEL_036UA,
    LPCTMU_ISEL_072UA,
    LPCTMU_ISEL_108UA,
    LPCTMU_ISEL_144UA,
    LPCTMU_ISEL_180UA,
    LPCTMU_ISEL_216UA,
    LPCTMU_ISEL_252UA,
    LPCTMU_ISEL_288UA
};

enum {
    LPCTMU_CH0_PB0,
    LPCTMU_CH1_PB1,
    LPCTMU_CH2_PB2,
    LPCTMU_CH3_PB4,
    LPCTMU_CH4_PB5,
};

enum CTMU_M2P_CMD {
    REQUEST_LPCTMU_IRQ = 0x50,
    REQUEST_LPTMR_IRQ,
    RESET_IDENTIFY_ALGO,
};

struct lpctmu_platform_data {
    u8 hv_level;
    u8 lv_level;
    u8 cur_level;
    u8 sample_window_time;
    u8 sample_scan_time;
    u8 lowpower_sample_scan_time;
};

struct lpctmu_config_data {
    u8 ch_idx;
    u8 ch_num;
    u8 ch_list[LPCTMU_CHANNEL_SIZE];
    u8 ch_wkp_en;
    u8 softoff_keep_work;
    const struct lpctmu_platform_data *pdata;
};



#define LPCTMU_PLATFORM_DATA_BEGIN(data) \
    const struct lpctmu_platform_data data = {

#define LPCTMU_PLATFORM_DATA_END() \
    .sample_window_time = 4, \
    .sample_scan_time = 20, \
    .lowpower_sample_scan_time = 100, \
}


void lpctmu_send_m2p_cmd(enum CTMU_M2P_CMD cmd);

u32 lpctmu_get_cur_ch_by_idx(u32 ch_idx);

u32 lpctmu_get_idx_by_cur_ch(u32 cur_ch);

void lpctmu_set_ana_hv_level(u32 level);

u32 lpctmu_get_ana_hv_level(void);

void lpctmu_set_ana_lv_level(u32 level);

u32 lpctmu_get_ana_lv_level(void);

void lpctmu_set_ana_cur_level(u32 ch, u32 cur_level);

u32 lpctmu_get_ana_cur_level(u32 ch);

void lpctmu_init(struct lpctmu_config_data *cfg_data);

void lpctmu_disable(void);

void lpctmu_enable(void);

u32 lpctmu_is_sf_keep(void);


#endif



