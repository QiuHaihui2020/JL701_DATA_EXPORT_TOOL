#ifndef _USB_HW_H_
#define _USB_HW_H_
#include "typedef.h"
#include "generic/ioctl.h"


#define USB_MAX_HW_EPNUM    5

/* #define ep_regs JL_USB_EP_TypeDef */
typedef struct {
    volatile u32 TXMAXP;
    volatile u32 TXCSR1;
    volatile u32 TXCSR2;
    volatile u32 RXMAXP;
    volatile u32 RXCSR1;
    volatile u32 RXCSR2;
    volatile const u32 RXCOUNT1;
    volatile const u32 RXCOUNT2;
    volatile u32 TXTYPE;
    volatile u32 TXINTERVAL;
    volatile u32 RXTYPE;
    volatile u32 RXINTERVAL;
    u32 RESERVED[0xd0 / 4];
} ep_regs;



#define PHY_ON          0
#define LOW_SPEED       1
#define USB_NRST        2
#define TM1             3
#define CID             4
#define VBUS            5
#define USB_TEST        6
#define PDCHKDP         9
#define SOFIE           10
#define SIEIE           11
#define CLR_SOF_PND     12
#define SOF_PND         13
#define SIE_PND         14
#define CHKDPO          15
#define DM_SE           16
#define DP_SE           17

#define MC_RNW          14
#define MACK            15

#define DPOUT           0
#define DMOUT           1
#define DPIE            2
#define DMIE            3
#define DPPU            4
#define DMPU            5
#define DPPD            6
#define DMPD            7
#define DPDIE           8
#define DMDIE           9
#define DPDIEH          10
#define DMDIEH          11
#define IO_MODE         12
#define SR              13
#define IO_PU_MODE      14



enum {
    USB0,
};
#define USB_MAX_HW_NUM      1



#endif
