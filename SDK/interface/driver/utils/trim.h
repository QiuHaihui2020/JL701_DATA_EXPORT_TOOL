#ifndef __ARCH_TRIM__
#define __ARCH_TRIM__

#include "typedef.h"

struct volt_trim {
    volatile u32 *reg;
    volatile u32 p33_reg;  //注意p33寄存器间接还是直接访问
    u32 Start;
    u32 Length;
    u32 MinVal;
    u32 MaxVal;
    u32 ad_ch;
};

int voltage_trim_dichotomy(u32 vbg, struct volt_trim *trim, void (*pUDELAY)(u32 us));

void arch_trim();

u32 trim_get_sys_pll_vbg();

u32 trim_get_usb_pll_vbg();

u32 trim_get_usb_rx_ldo();

u32 trim_get_usb_tx_ldo();

#endif
