#ifndef _GPIO_HW_H
#define _GPIO_HW_H

#include "typedef.h"

#define IO_GROUP_NUM 		16

#define IO_PORTA_00 				(IO_GROUP_NUM * 0 + 0)//A-A=0
#define IO_PORTA_01 				(IO_GROUP_NUM * 0 + 1)
#define IO_PORTA_02 				(IO_GROUP_NUM * 0 + 2)
#define IO_PORTA_03 				(IO_GROUP_NUM * 0 + 3)
#define IO_PORTA_04 				(IO_GROUP_NUM * 0 + 4)
#define IO_PORTA_05 				(IO_GROUP_NUM * 0 + 5)
#define IO_PORTA_06 				(IO_GROUP_NUM * 0 + 6)
#define IO_PORTA_07 				(IO_GROUP_NUM * 0 + 7)
#define IO_PORTA_08 				(IO_GROUP_NUM * 0 + 8)
#define IO_PORTA_09 				(IO_GROUP_NUM * 0 + 9)
#define IO_PORTA_10 				(IO_GROUP_NUM * 0 + 10)
#define IO_PORTA_11 				(IO_GROUP_NUM * 0 + 11)
#define IO_PORTA_12 				(IO_GROUP_NUM * 0 + 12)
#define IO_PORTA_13 				(IO_GROUP_NUM * 0 + 13)
#define IO_PORTA_14 				(IO_GROUP_NUM * 0 + 14)
#define IO_PORTA_15 				(IO_GROUP_NUM * 0 + 15)
#define IO_PORT_PA_MASK              0xffff

#define IO_PORTB_00 				(IO_GROUP_NUM * 1 + 0)//B-A=1
#define IO_PORTB_01 				(IO_GROUP_NUM * 1 + 1)
#define IO_PORTB_02 				(IO_GROUP_NUM * 1 + 2)
#define IO_PORTB_03 				(IO_GROUP_NUM * 1 + 3)
#define IO_PORTB_04 				(IO_GROUP_NUM * 1 + 4)
#define IO_PORTB_05 				(IO_GROUP_NUM * 1 + 5)
#define IO_PORTB_06 				(IO_GROUP_NUM * 1 + 6)
#define IO_PORTB_07 				(IO_GROUP_NUM * 1 + 7)
#define IO_PORTB_08 				(IO_GROUP_NUM * 1 + 8)
#define IO_PORTB_09 				(IO_GROUP_NUM * 1 + 9)
#define IO_PORTB_10 				(IO_GROUP_NUM * 1 + 10)
#define IO_PORTB_11 				(IO_GROUP_NUM * 1 + 11)
#define IO_PORT_PB_MASK              0x0fff

#define IO_PORTC_00 				(IO_GROUP_NUM * 2 + 0)//C-A=2
#define IO_PORTC_01 				(IO_GROUP_NUM * 2 + 1)
#define IO_PORTC_02 				(IO_GROUP_NUM * 2 + 2)
#define IO_PORTC_03 				(IO_GROUP_NUM * 2 + 3)
#define IO_PORTC_04 				(IO_GROUP_NUM * 2 + 4)
#define IO_PORTC_05 				(IO_GROUP_NUM * 2 + 5)
#define IO_PORTC_06 				(IO_GROUP_NUM * 2 + 6)
#define IO_PORTC_07 				(IO_GROUP_NUM * 2 + 7)
#define IO_PORTC_08 				(IO_GROUP_NUM * 2 + 8)
#define IO_PORT_PC_MASK              0x01ff

#define IO_PORTD_00 				(IO_GROUP_NUM * 3 + 0)//D-A=3
#define IO_PORTD_01 				(IO_GROUP_NUM * 3 + 1)
#define IO_PORTD_02 				(IO_GROUP_NUM * 3 + 2)
#define IO_PORTD_03 				(IO_GROUP_NUM * 3 + 3)
#define IO_PORTD_04 				(IO_GROUP_NUM * 3 + 4)
#define IO_PORTD_05 				(IO_GROUP_NUM * 3 + 5)
#define IO_PORTD_06 				(IO_GROUP_NUM * 3 + 6)
#define IO_PORT_PD_MASK              0x007f

#define IO_PORTE_00 				(IO_GROUP_NUM * 4 + 0)//E-A=4
#define IO_PORTE_01 				(IO_GROUP_NUM * 4 + 1)
#define IO_PORTE_02 				(IO_GROUP_NUM * 4 + 2)
#define IO_PORTE_03 				(IO_GROUP_NUM * 4 + 3)
#define IO_PORTE_04 				(IO_GROUP_NUM * 4 + 4)
#define IO_PORTE_05 				(IO_GROUP_NUM * 4 + 5)
#define IO_PORTE_06 				(IO_GROUP_NUM * 4 + 6)
#define IO_PORT_PE_MASK              0x007f

#define IO_PORTG_00 				(IO_GROUP_NUM * 6 + 0)//G-A=6
#define IO_PORTG_01 				(IO_GROUP_NUM * 6 + 1)
#define IO_PORTG_02 				(IO_GROUP_NUM * 6 + 2)
#define IO_PORTG_03 				(IO_GROUP_NUM * 6 + 3)
#define IO_PORTG_04 				(IO_GROUP_NUM * 6 + 4)
#define IO_PORTG_05 				(IO_GROUP_NUM * 6 + 5)
#define IO_PORTG_06 				(IO_GROUP_NUM * 6 + 6)
#define IO_PORTG_07 				(IO_GROUP_NUM * 6 + 7)
#define IO_PORTG_08 				(IO_GROUP_NUM * 6 + 8)
#define IO_PORT_PG_MASK              0x01ff


#define IO_PORTP_00 				(IO_GROUP_NUM * 13 + 0)//pp固定13
#define IO_PORT_PP_MASK              0x0001

#define IO_PORT_LDOIN   			IO_PORTP_00

#define IO_MAX_NUM 					(IO_PORTP_00 + 1)

#define IO_PORT_DP                  (IO_GROUP_NUM * 14 + 0)//usb固定14
#define IO_PORT_DM                  (IO_GROUP_NUM * 14 + 1)
#define IO_PORT_USB_MASK            0x03
#define IS_PORT_USB(x)              (x <= IO_PORT_DM)//无usb赋0

//无PR
// #define IO_PORT_PR_00               (IO_GROUP_NUM * 15 + 0)//pr固定15
// #define IO_PORT_PR_01               (IO_GROUP_NUM * 15 + 1)
// #define IO_PORT_PR_02               (IO_GROUP_NUM * 15 + 2)
// #define IO_PORT_PR_03               (IO_GROUP_NUM * 15 + 3)
// #define IO_PORT_PR_04               (IO_GROUP_NUM * 15 + 4)
// #define IO_PORT_PR_MASK              0x1f


#define IO_PORT_MAX					(IO_PORT_DM + 1)

#define P33_IO_OFFSET               0
#define IO_CHGFL_DET                (IO_PORT_MAX + P33_IO_OFFSET + 0)
#define IO_VBGOK_DET                (IO_PORT_MAX + P33_IO_OFFSET + 1)
#define IO_VBTCH_DET                (IO_PORT_MAX + P33_IO_OFFSET + 2)
#define IO_LDOIN_DET                (IO_PORT_MAX + P33_IO_OFFSET + 3)
#define IO_VBATDT_DET               (IO_PORT_MAX + P33_IO_OFFSET + 4)
// #define IO_SPDIF_RX_DET             (IO_PORT_MAX + P33_IO_OFFSET + 4)


#define GPIOA                       (IO_GROUP_NUM * 0)
#define GPIOB                       (IO_GROUP_NUM * 1)
#define GPIOC                       (IO_GROUP_NUM * 2)
#define GPIOD                       (IO_GROUP_NUM * 3)
#define GPIOE                       (IO_GROUP_NUM * 4)
#define GPIOG                       (IO_GROUP_NUM * 6)
#define GPIOP                       (IO_GROUP_NUM * 13)
#define GPIOUSB                     (IO_GROUP_NUM * 14)
// #define GPIOR                       (IO_GROUP_NUM * 15) //br28 no pr
#define GPIOP33                     (IO_PORT_MAX + P33_IO_OFFSET)

enum gpio_port {
    PORTA = 0,
    PORTB = 1,
    PORTC = 2,
    PORTD = 3,
    PORTE = 4,
    PORTG = 6,
    PORTP = 13,
    PORTUSB = 14,
    // PORTR = 15,  //br28 无pr
};
#define IS_PORT_ALL_PERIPH(PORT) (((PORT) == PORTA) || \
                                  ((PORT) == PORTB) || \
                                  ((PORT) == PORTC) || \
                                  ((PORT) == PORTD) || \
                                  ((PORT) == PORTE) || \
                                  ((PORT) == PORTG) || \
                                  ((PORT) == PORTP) || \
                                  ((PORT) == PORTUSB))

enum port_op_mode {
    PORT_SET = 1,
    PORT_AND,
    PORT_OR,
    PORT_XOR,
};

struct port_reg {
    volatile unsigned int out;
    volatile unsigned int in;
    volatile unsigned int dir;
    volatile unsigned int die;
    volatile unsigned int pu0;
    // volatile unsigned int pu1;
    volatile unsigned int pd0;
    // volatile unsigned int pd1;
    volatile unsigned int hd0;
    volatile unsigned int hd1;
    volatile unsigned int dieh;
};
#define GPIO_PX_PU_REG_NUM 1
#define GPIO_PX_PD_REG_NUM 1
#define GPIO_PX_HD_REG_NUM 2
#define GPIO_PX_DIEH_REG_NUM 1
#define GPIO_PX_SPL_REG_NUM  0
#define GPIO_PX_BSR_REG_NUM  0
struct usb_reg {
    volatile unsigned int out: 2; //0
        volatile unsigned int dir: 2; //2
        volatile unsigned int pu0: 2; //4
        // volatile unsigned int pu1:2;//6
        volatile unsigned int pd0: 2; //6
        volatile unsigned int die: 2; //8
        volatile unsigned int dieh: 2; //10
        volatile unsigned int : 4; //12
        volatile unsigned int in: 2; //16
        volatile unsigned int res: 14; //18
    };
#define GPIO_USB_PU_REG_NUM 1
#define GPIO_USB_PD_REG_NUM 1
#define GPIO_USB_HD_REG_NUM 0
#define GPIO_USB_DIEH_REG_NUM 1
#define GPIO_USB_SPL_REG_NUM  0
#define GPIO_USB_BSR_REG_NUM  0
//无PR
// struct port_pr_reg {
//     volatile unsigned int in;
//     volatile unsigned int out;
//     volatile unsigned int dir;
//     volatile unsigned int die;
//     volatile unsigned int pu0;
//     // volatile unsigned int pu1;
//     volatile unsigned int pd0;
//     // volatile unsigned int pd1;
//     volatile unsigned int hd0;
//     // volatile unsigned int hd1;
// };
// #define GPIO_PR_PU_REG_NUM 0
// #define GPIO_PR_PD_REG_NUM 0
// #define GPIO_PR_HD_REG_NUM 0
// #define GPIO_PR_DIEH_REG_NUM 0
// #define GPIO_PR_SPL_REG_NUM  0
//#define GPIO_PR_BSR_REG_NUM  0

#define GPIO_PU_REG_NUM 1 //max_num
#define GPIO_PD_REG_NUM 1 //max_num
#define GPIO_HD_REG_NUM 2 //max_num


// ----------------------------------------
// enum gpio_hd_mode {//no use
//     GPIO_HD_2p4mA,		#<{(| 最大驱动电流  2.4mA |)}>#
//     GPIO_HD_8p0mA,		#<{(| 最大驱动电流  8.0mA |)}>#
//     GPIO_HD_26p4mA,		#<{(| 最大驱动电流  26.4mA  |)}>#
//     GPIO_HD_50p0mA,		#<{(| 最大驱动电流 50.0mA |)}>#
// };

//===================================================//
// Crossbar ENUM
//===================================================//
    enum PFI_TABLE {
    PFI_NULL = 0,
    PFI_GP_ICH0 = ((u32)(&(JL_IMAP->FI_GP_ICH0))),
    PFI_GP_ICH1 = ((u32)(&(JL_IMAP->FI_GP_ICH1))),
    PFI_GP_ICH2 = ((u32)(&(JL_IMAP->FI_GP_ICH2))),
    PFI_GP_ICH3 = ((u32)(&(JL_IMAP->FI_GP_ICH3))),
    PFI_GP_ICH4 = ((u32)(&(JL_IMAP->FI_GP_ICH4))),
    PFI_GP_ICH5 = ((u32)(&(JL_IMAP->FI_GP_ICH5))),
    PFI_GP_ICH6 = ((u32)(&(JL_IMAP->FI_GP_ICH6))),
    PFI_GP_ICH7 = ((u32)(&(JL_IMAP->FI_GP_ICH7))),
    PFI_GP_ICH8 = ((u32)(&(JL_IMAP->FI_GP_ICH8))),
    PFI_GP_ICH9 = ((u32)(&(JL_IMAP->FI_GP_ICH9))),
    PFI_GP_ICH10 = ((u32)(&(JL_IMAP->FI_GP_ICH10))),
    PFI_GP_ICH11 = ((u32)(&(JL_IMAP->FI_GP_ICH11))),
    PFI_GP_ICH12 = ((u32)(&(JL_IMAP->FI_GP_ICH12))),
    PFI_GP_ICH13 = ((u32)(&(JL_IMAP->FI_GP_ICH13))),
    PFI_SPI0_CLK = ((u32)(&(JL_IMAP->FI_SPI0_CLK))),
    PFI_SPI0_DA0 = ((u32)(&(JL_IMAP->FI_SPI0_DA0))),
    PFI_SPI0_DA1 = ((u32)(&(JL_IMAP->FI_SPI0_DA1))),
    PFI_SPI0_DA2 = ((u32)(&(JL_IMAP->FI_SPI0_DA2))),
    PFI_SPI0_DA3 = ((u32)(&(JL_IMAP->FI_SPI0_DA3))),
    PFI_SPI1_CLK = ((u32)(&(JL_IMAP->FI_SPI1_CLK))),
    PFI_SPI1_DA0 = ((u32)(&(JL_IMAP->FI_SPI1_DA0))),
    PFI_SPI1_DA1 = ((u32)(&(JL_IMAP->FI_SPI1_DA1))),
    PFI_SPI1_DA2 = ((u32)(&(JL_IMAP->FI_SPI1_DA2))),
    PFI_SPI1_DA3 = ((u32)(&(JL_IMAP->FI_SPI1_DA3))),
    PFI_SPI2_CLK = ((u32)(&(JL_IMAP->FI_SPI2_CLK))),
    PFI_SPI2_DA0 = ((u32)(&(JL_IMAP->FI_SPI2_DA0))),
    PFI_SPI2_DA1 = ((u32)(&(JL_IMAP->FI_SPI2_DA1))),
    PFI_SPI2_DA2 = ((u32)(&(JL_IMAP->FI_SPI2_DA2))),
    PFI_SPI2_DA3 = ((u32)(&(JL_IMAP->FI_SPI2_DA3))),
    PFI_SD0_CMD = ((u32)(&(JL_IMAP->FI_SD0_CMD))),
    PFI_SD0_DA0 = ((u32)(&(JL_IMAP->FI_SD0_DA0))),
    PFI_SD0_DA1 = ((u32)(&(JL_IMAP->FI_SD0_DA1))),
    PFI_SD0_DA2 = ((u32)(&(JL_IMAP->FI_SD0_DA2))),
    PFI_SD0_DA3 = ((u32)(&(JL_IMAP->FI_SD0_DA3))),
    PFI_IIC_SCL = ((u32)(&(JL_IMAP->FI_IIC_SCL))),
    PFI_IIC_SDA = ((u32)(&(JL_IMAP->FI_IIC_SDA))),
    PFI_UART0_RX = ((u32)(&(JL_IMAP->FI_UART0_RX))),
    PFI_UART1_RX = ((u32)(&(JL_IMAP->FI_UART1_RX))),
    PFI_UART1_CTS = ((u32)(&(JL_IMAP->FI_UART1_CTS))),
    PFI_UART2_RX = ((u32)(&(JL_IMAP->FI_UART2_RX))),
    PFI_TDM_S_WCK  = ((u32)(&(JL_IMAP->FI_TDM_S_WCK))),
    PFI_TDM_S_BCK  = ((u32)(&(JL_IMAP->FI_TDM_S_BCK))),
    PFI_TDM_M_DA  = ((u32)(&(JL_IMAP->FI_TDM_M_DA))),
    PFI_RDEC0_DAT0 = ((u32)(&(JL_IMAP->FI_RDEC0_DAT0))),
    PFI_RDEC0_DAT1 = ((u32)(&(JL_IMAP->FI_RDEC0_DAT1))),
    PFI_RDEC1_DAT0 = ((u32)(&(JL_IMAP->FI_RDEC1_DAT0))),
    PFI_RDEC1_DAT1 = ((u32)(&(JL_IMAP->FI_RDEC1_DAT1))),
    PFI_RDEC2_DAT0 = ((u32)(&(JL_IMAP->FI_RDEC2_DAT0))),
    PFI_RDEC2_DAT1 = ((u32)(&(JL_IMAP->FI_RDEC2_DAT1))),
    PFI_ALNK0_MCLK = ((u32)(&(JL_IMAP->FI_ALNK0_MCLK))),
    PFI_ALNK0_LRCK = ((u32)(&(JL_IMAP->FI_ALNK0_LRCK))),
    PFI_ALNK0_SCLK = ((u32)(&(JL_IMAP->FI_ALNK0_SCLK))),
    PFI_ALNK0_DAT0 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT0))),
    PFI_ALNK0_DAT1 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT1))),
    PFI_ALNK0_DAT2 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT2))),
    PFI_ALNK0_DAT3 = ((u32)(&(JL_IMAP->FI_ALNK0_DAT3))),
    PFI_PLNK_DAT0 = ((u32)(&(JL_IMAP->FI_PLNK_DAT0))),
    PFI_PLNK_DAT1 = ((u32)(&(JL_IMAP->FI_PLNK_DAT1))),
    PFI_CHAIN_IN0 = ((u32)(&(JL_IMAP->FI_CHAIN_IN0))),
    PFI_CHAIN_IN1 = ((u32)(&(JL_IMAP->FI_CHAIN_IN1))),
    PFI_CHAIN_IN2 = ((u32)(&(JL_IMAP->FI_CHAIN_IN2))),
    PFI_CHAIN_IN3 = ((u32)(&(JL_IMAP->FI_CHAIN_IN3))),
    PFI_CHAIN_RST = ((u32)(&(JL_IMAP->FI_CHAIN_RST))),
    PFI_TOTAl = ((u32)(&(JL_IMAP->FI_TOTAL))),
};

#define  INPUT_GP_ICH_MAX  14
#define  OUTPUT_GP_OCH_MAX 16

enum OUTPUT_CH_SIGNAL {
    OUTPUT_CH_SIGNAL_MC_PWM0_H = 0,
    OUTPUT_CH_SIGNAL_MC_PWM0_L,
    OUTPUT_CH_SIGNAL_MC_PWM1_H,
    OUTPUT_CH_SIGNAL_MC_PWM1_L,
    OUTPUT_CH_SIGNAL_MC_PWM2_H,
    OUTPUT_CH_SIGNAL_MC_PWM2_L,
    OUTPUT_CH_SIGNAL_MC_PWM3_H,
    OUTPUT_CH_SIGNAL_MC_PWM3_L,
    OUTPUT_CH_SIGNAL_TIMER0_PWM,
    OUTPUT_CH_SIGNAL_TIMER1_PWM,
    OUTPUT_CH_SIGNAL_TIMER2_PWM,
    OUTPUT_CH_SIGNAL_TIMER3_PWM,
    OUTPUT_CH_SIGNAL_TIMER4_PWM,
    OUTPUT_CH_SIGNAL_TIMER5_PWM,
    OUTPUT_CH_SIGNAL_CLOCK_OUT0,
    OUTPUT_CH_SIGNAL_CLOCK_OUT1,
    OUTPUT_CH_SIGNAL_CLOCK_OUT2,
    OUTPUT_CH_SIGNAL_CLOCK_OUT3,
    OUTPUT_CH_SIGNAL_PI_GP_ICH12,
    OUTPUT_CH_SIGNAL_PI_GP_ICH13,
    OUTPUT_CH_SIGNAL_WL_AMPE,
    OUTPUT_CH_SIGNAL_WL_LNAE,
    OUTPUT_CH_SIGNAL_WLC_INT_ACTIVE,
    OUTPUT_CH_SIGNAL_WLC_INT_STATUS,
    OUTPUT_CH_SIGNAL_WLC_INT_FREQ,
    OUTPUT_CH_SIGNAL_P11_DBG_OUT,
    OUTPUT_CH_SIGNAL_P33_DBG_OUT,
};

enum INPUT_CH_TYPE {
    INPUT_CH_TYPE_GP_ICH = 0,
    INPUT_CH_TYPE_TIME0_PWM = 14,
    INPUT_CH_TYPE_TIME1_PWM,
};
enum INPUT_CH_SIGNAL {
    //ICH_CON0
    INPUT_CH_SIGNAL_TIMER0_CIN = 0,
    INPUT_CH_SIGNAL_TIMER0_CAPTURE,
    INPUT_CH_SIGNAL_TIMER1_CIN,
    INPUT_CH_SIGNAL_TIMER1_CAPTURE,
    INPUT_CH_SIGNAL_TIMER2_CIN,
    INPUT_CH_SIGNAL_TIMER2_CAPTURE,
    INPUT_CH_SIGNAL_TIMER3_CIN,
    INPUT_CH_SIGNAL_TIMER3_CAPTURE,

    //ICH_CON1
    INPUT_CH_SIGNAL_TIMER4_CIN,
    INPUT_CH_SIGNAL_TIMER4_CAPTURE,
    INPUT_CH_SIGNAL_TIMER5_CIN,
    INPUT_CH_SIGNAL_TIMER5_CAPTURE,
    INPUT_CH_SIGNAL_WAKEUP,
    INPUT_CH_SIGNAL_IRFLT,
    INPUT_CH_SIGNAL_CAP,
    INPUT_CH_SIGNAL_CLK_PIN,

    //ICH_CON2
    INPUT_CH_SIGNAL_WLC_EXT_ACT,
    INPUT_CH_SIGNAL_LCD_TE,
    INPUT_CH_SIGNAL_MC_CKPIN0,
    INPUT_CH_SIGNAL_MC_CKPIN1,
    INPUT_CH_SIGNAL_MC_CKPIN2,
    INPUT_CH_SIGNAL_MC_CKPIN3,
    INPUT_CH_SIGNAL_MC_FPIN0,
    INPUT_CH_SIGNAL_MC_FPIN1,

    //ICH_CON3
    INPUT_CH_SIGNAL_MC_FPIN2,
    INPUT_CH_SIGNAL_MC_FPIN3,
    INPUT_CH_SIGNAL_RESERVE0,
    INPUT_CH_SIGNAL_RESERVE1,
    INPUT_CH_SIGNAL_RESERVE2,
    INPUT_CH_SIGNAL_RESERVE3,
    INPUT_CH_SIGNAL_RESERVE4,
    INPUT_CH_SIGNAL_RESERVE5,
};

enum gpio_function {
    PORT_FUNC_NULL,    //null
    PORT_FUNC_UART0_TX, //out
    PORT_FUNC_UART0_RX,//in
    PORT_FUNC_UART1_TX, //out
    PORT_FUNC_UART1_RX,//in
    PORT_FUNC_UART1_RTS,//out
    PORT_FUNC_UART1_CTS,//in
    PORT_FUNC_UART2_TX, //out
    PORT_FUNC_UART2_RX,//in

    PORT_FUNC_SPI0_CLK,
    PORT_FUNC_SPI0_DA0,
    PORT_FUNC_SPI0_DA1,
    PORT_FUNC_SPI0_DA2,
    PORT_FUNC_SPI0_DA3,
    PORT_FUNC_SPI1_CLK,
    PORT_FUNC_SPI1_DA0,
    PORT_FUNC_SPI1_DA1,
    PORT_FUNC_SPI1_DA2,
    PORT_FUNC_SPI1_DA3,
    PORT_FUNC_SPI2_CLK,
    PORT_FUNC_SPI2_DA0,
    PORT_FUNC_SPI2_DA1,
    PORT_FUNC_SPI2_DA2,
    PORT_FUNC_SPI2_DA3,

    PORT_FUNC_IIC_SCL,
    PORT_FUNC_IIC_SDA,

    PORT_FUNC_SD0_CLK,//out
    PORT_FUNC_SD0_CMD,
    PORT_FUNC_SD0_DA0,
    PORT_FUNC_SD0_DA1,
    PORT_FUNC_SD0_DA2,
    PORT_FUNC_SD0_DA3,

    PORT_FUNC_GPADC,    //in
    PORT_FUNC_PWM_LED,

    PORT_FUNC_PLNK_SCLK,//out
    PORT_FUNC_PLNK_DAT0,//in
    PORT_FUNC_PLNK_DAT1,//in

    PORT_FUNC_OCH_CLOCK_OUT0,
    PORT_FUNC_OCH_CLOCK_OUT1,
    PORT_FUNC_OCH_CLOCK_OUT2,
    PORT_FUNC_OCH_CLOCK_OUT3,

    PORT_FUNC_TIMER0_PWM,
    PORT_FUNC_TIMER1_PWM,
    PORT_FUNC_TIMER2_PWM,
    PORT_FUNC_TIMER3_PWM,
    PORT_FUNC_TIMER4_PWM,
    PORT_FUNC_TIMER5_PWM,
    PORT_FUNC_TIMER0_CAPTURE,
    PORT_FUNC_TIMER1_CAPTURE,
    PORT_FUNC_TIMER2_CAPTURE,
    PORT_FUNC_TIMER3_CAPTURE,
    PORT_FUNC_TIMER4_CAPTURE,
    PORT_FUNC_TIMER5_CAPTURE,
    PORT_FUNC_ICH_WKUP,
    PORT_FUNC_IRFLT_0, //实际只有1个IRFLT
    PORT_FUNC_IRFLT_1,
    PORT_FUNC_IRFLT_2,
    PORT_FUNC_IRFLT_3,
    PORT_FUNC_IRFLT_4,
    PORT_FUNC_IRFLT_5,

    PORT_FUNC_MCPWM0_H,
    PORT_FUNC_MCPWM0_L,
    PORT_FUNC_MCPWM1_H,
    PORT_FUNC_MCPWM1_L,
    PORT_FUNC_MCPWM2_H,
    PORT_FUNC_MCPWM2_L,
    PORT_FUNC_MCPWM3_H,
    PORT_FUNC_MCPWM3_L,
    PORT_FUNC_MCPWM0_FP,
    PORT_FUNC_MCPWM1_FP,
    PORT_FUNC_MCPWM2_FP,
    PORT_FUNC_MCPWM3_FP,

    PORT_FUNC_RDEC0_PORT0,
    PORT_FUNC_RDEC0_PORT1,
    PORT_FUNC_RDEC1_PORT0,
    PORT_FUNC_RDEC1_PORT1,
    PORT_FUNC_RDEC2_PORT0,
    PORT_FUNC_RDEC2_PORT1,
};

/**************************************************/
#define __struct(x) (struct x##_reg *)
#define _struct(x) __struct(x)
#ifdef GPIOA
#define __PORTPA ((struct port_reg *)JL_PORTA)
#endif
#ifdef GPIOB
#define __PORTPB ((struct port_reg *)JL_PORTB)
#endif
#ifdef GPIOC
#define __PORTPC ((struct port_reg *)JL_PORTC)
#endif
#ifdef GPIOD
#define __PORTPD ((struct port_reg *)JL_PORTD)
#endif
#ifdef GPIOE
#define __PORTPE ((struct port_reg *)JL_PORTE)
#endif
#ifdef GPIOF
#define __PORTPF ((struct port_reg *)JL_PORTF)
#endif
#ifdef GPIOG
#define __PORTPG ((struct port_reg *)JL_PORTG)
#endif
#ifdef GPIOH
#define __PORTPH ((struct port_reg *)JL_PORTH)
#endif
#ifdef GPIOP
#define __PORTPP ((struct port_reg *)JL_PORTP)
#endif
#ifdef GPIOR
#define __PORTPR ((struct port_pr_reg *)R3_PR_IO_P)
#endif
#ifdef GPIOUSB
#define __PORTPU ((struct usb_reg *)JL_USB_IO)
#endif
#define __portx(x,y) __PORT##x->y
#define _portx(x,y)  __portx(x,y)
#define __toggle_port(x,y) __PORT##x->out ^= y;
#define _toggle_port(port,pin) __toggle_port(port,pin)

//log:
#define GPIO_LOG_FORMAT "0x%04x  0x%04x  0x%04x  0x%04x  0x%04x,%s  0x%04x,%s  0x%04x,0x%04x  %s"
#define GPIO_NO_SUPPORT_FUN "------"
#define GPIO_LOG_PORT(x,y) JL_PORT##x->OUT&y,JL_PORT##x->DIR&y,JL_PORT##x->DIE&y,JL_PORT##x->DIEH&y,JL_PORT##x->PU&y,GPIO_NO_SUPPORT_FUN,JL_PORT##x->PD&y,GPIO_NO_SUPPORT_FUN,JL_PORT##x->HD0&y,JL_PORT##x->HD&y,GPIO_NO_SUPPORT_FUN
#ifdef GPIOP
#define GPIO_LOG_PORTP JL_PORTP->OUT,JL_PORTP->DIR,JL_PORTP->DIE,JL_PORTP->DIEH,JL_PORTP->PU0,JL_PORTP->PU1,JL_PORTP->PD0,JL_PORTP->PD1,JL_PORTP->HD0,JL_PORTP->HD1
#endif
#ifdef GPIOR
#define GPIO_LOG_FORMAT_R "0x%04x  0x%04x  0x%04x  %s  0x%04x,0x%04x  0x%04x,0x%04x  0x%04x,0x%04x  %s"
#define GPIO_LOG_PORTR R3_PR_OUT,R3_PR_DIR,R3_PR_DIE,GPIO_NO_SUPPORT_FUN,R3_PR_PU0,R3_PR_PU1,R3_PR_PD0,R3_PR_PD1,R3_PR_HD0,R3_PR_HD1,GPIO_NO_SUPPORT_FUN
#endif
#ifdef GPIOUSB
#define GPIO_LOG_FORMAT_U "0x%04x  0x%04x  0x%04x  0x%04x  0x%04x,%s  0x%04x,%s  %s,%s  %s"
#define GPIO_LOG_PORTU _portx(PU,out),_portx(PU,dir),_portx(PU,die),_portx(PU,dieh),_portx(PU,pu0),GPIO_NO_SUPPORT_FUN,_portx(PU,pd0),GPIO_NO_SUPPORT_FUN,GPIO_NO_SUPPORT_FUN,GPIO_NO_SUPPORT_FUN,GPIO_NO_SUPPORT_FUN
#endif
/*************************function*************************/

/**
 * @brief usb_iomode
 *
 * @param enable 1，使能；0，关闭
 */
void usb_iomode(const u32 enable);
/**
 * @brief gpio_write
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1；0，输出0
 *
 * @return
 */
int gpio_hw_write(const u32 gpio, const u32 value);//return <0:error
/**
 * @brief gpio_read
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
int gpio_hw_read(const u32 gpio);//return <0:error

int get_gpio(const char *p);//return <0:error

/**************************************************************/
/*********************multi pin interface***************************/
int gpio_hw_port_pin_judge(const enum gpio_port port, u32 pin);
/**
 * @brief port_set_direction
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，输入；0，输出
 *
 * @return <0 :error
 */
int gpio_hw_set_direction(const enum gpio_port port, u32 pin, const u32 value);
/**
 * @brief port_direction_input
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 *
 * @return <0 :error
 */
int gpio_hw_direction_input(const enum gpio_port port, u32 pin);
/**
 * @brief port_direction_output/port_write_port/port_set_output_value
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，输出1；0，输出0
 * @return <0 :error
 */
int gpio_hw_direction_output(const enum gpio_port port, u32 pin, const int value);/////////
int gpio_hw_write_port(const enum gpio_port port, u32 pin, const u32 value);
int gpio_hw_set_output_value(const enum gpio_port port, u32 pin, const u32 value);
/**
 * @brief port_set_pull_up/port_set_pull_down/port_set_hd/
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 参考枚举gpio_pullup_mode/gpio_pulldown_mode/gpio_drive_strength
 * @return <0 :error
 */
int gpio_hw_set_pull_up(const enum gpio_port port, u32 pin, const enum gpio_pullup_mode value);
int gpio_hw_set_pull_down(const enum gpio_port port, u32 pin, const enum gpio_pulldown_mode value);//portabcdpr:pd0,pd1,usb:pd0
int gpio_hw_set_drive_strength(const enum gpio_port port, u32 pin, const enum gpio_drive_strength value);
/**
 * @brief port_set_die/port_set_dieh
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 * @param value 1，设置1；0，设置0
 * @return <0 :error
 */
int gpio_hw_set_die(const enum gpio_port port, u32 pin, const int value);
int gpio_hw_set_dieh(const enum gpio_port port, u32 pin, const u32 value);

//iic专用
int gpio_hw_set_spl(const enum gpio_port port, u32 pin, const u32 value);
/**
 * @brief gpio_hw_read_port/gpio_hw_read_out_level/gpio_hw_read_drive_strength
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1
 *
 * @return <0 :error ；other：ok
 */
int gpio_hw_read_port(const enum gpio_port port, u32 pin);
int gpio_hw_read_out_level(const enum gpio_port port, u32 pin);
u32 gpio_hw_read_drive_strength(const enum gpio_port port, u32 pin);//return hd1:高16位,  hd0:低16位

/**
 * @brief:同组多个io配置不同状态接口
 *
 * @param port 参考枚举gpio_port：PORTA，PORTB
 * @param pin 参考宏：PORT_PIN_0，PORT_PIN_1,可以多个io或|
 * @param value：对应pin的bit有效
 * @param op：参考枚举port_op_mode ：PORT_SET，PORT_AND,,,
 *
 * @return <0 :error ；other：ok
 */
int gpio_hw_op_dir(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_out(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_die(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_dieh(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_pu0(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_pu1(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_pd0(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);
int gpio_hw_op_pd1(const enum gpio_port port, u32 pin, u32 value, const enum port_op_mode op);

//=================================================================================//
//@brief: CrossBar 获取某IO的输出映射寄存器
//@input:
// 		gpio: 需要输出外设信号的IO口; 如IO_PORTA_00
//@return:
// 		输出映射寄存器地址; 如&(JL_OMAP->PA0_OUT)
//=================================================================================//
u32 *gpio2crossbar_outreg(u32 gpio);

//=================================================================================//
//@brief: CrossBar 获取某IO的输入映射序号
//@input:
// 		gpio: 需要输出外设信号的IO口; 如IO_PORTA_00
//@return:
// 		输出映射序号; 如PA0_IN
//=================================================================================//
u32 gpio2crossbar_inport(u32 gpio);

//=================================================================================//
//@brief: CrossBar 输出设置 API, 将指定IO口设置为某个外设的输出
//@input:
// 		gpio: 需要输出外设信号的IO口;
// 		fun_index: 需要输出到指定IO口的外设信号, 可以输出外设信号列表请查看io_omap.h文件;
// 		dir_ctl: IO口方向由外设控制使能, 常设为1;
// 		data_ctl: IO口电平状态由外设控制使能, 常设为1;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note: 所映射的IO需要在设置IO状态为输出配置;
//@example: 将UART0的Tx信号输出到IO_PORTA_05口:
// 			gpio_direction_output(IO_PORTA_05, 1); //设置IO为输出状态
//			gpio_set_fun_output_port(IO_PORTA_05, FO_UART0_TX, 1, 1); //将UART0的Tx信号输出到IO_PORTA_05口
//=================================================================================//
int gpio_set_fun_output_port(u32 gpio, u32 fun_index, u8 dir_ctl, u8 data_ctl);

//=================================================================================//
//@brief: CrossBar 输出设置 API, 将指定IO释放外设控制, 变为普通IO;
//@input:
// 		gpio: 需要释放外设控制IO口, 释放后变为普通IO模式;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note:
//@example: 将IO_PORTA_05口被某一外设控制状态释放:
// 			gpio_disable_fun_output_port(IO_PORTA_05);
//=================================================================================//
int gpio_disable_fun_output_port(u32 gpio);

//=================================================================================//
//@brief: CrossBar 输入设置 API, 将某个外设的输入设置为从某个IO输入
//@input:
// 		gpio: 需要输入外设信号的IO口;
// 		pfun: 需要从指定IO输入的外设信号, 可以输入的外设信号列表请查看gpio.h文件enum PFI_TABLE枚举项;
//@return:
// 		1)0: 执行正确;
//		2)-EINVAL: 传参出错;
//@note: 所映射的IO需要在设置IO状态为输入配置;
//@example: 将UART0的Rx信号设置为IO_PORTA_05口输入:
//			gpio_set_die(IO_PORTA_05, 1); 		//数字输入使能
//			gpio_set_pull_up(IO_PORTA_05, 1);  //上拉输入使能
//			gpio_direction_input(IO_PORTA_05);  //设置IO为输入状态
//			gpio_set_fun_input_port(IO_PORTA_05, PFI_UART0_RX); //将UART0的Rx信号设置为IO_PORTA_05口输入
//=================================================================================//
int gpio_set_fun_input_port(u32 gpio, enum PFI_TABLE pfun);

//=================================================================================//
//@brief: CrossBar 输入设置 API, 将某个外设信号释放IO口控制, 变为普通IO;
//@input:
// 		pfun: 需要释放由某个IO口输入的外设信号, 外设信号列表请查看gpio.h文件enum PFI_TABLE枚举项;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将外设信号PFI_UART0_RX释放由某个IO输入:
// 			gpio_disable_fun_input_port(PFI_UART0_RX);
//=================================================================================//
int gpio_disable_fun_input_port(enum PFI_TABLE pfun);

//=================================================================================//
//@brief: Output Channel输出设置 API, 将指定IO口设置为某个外设的输出
//@input:
// 		gpio: 需要输出外设信号的IO口;
// 		signal: 将enum OUTPUT_CH_SIGNAL列表中需要输出到指定IO口的外设信号, 可以输出的外设信号列表请查看gpio.h文件的enum OUTPUT_CH_SIGNAL枚举项;
//@return:  默认为0, 出错内部触发ASSERT;
//@note: 所映射的IO需要在设置IO状态为输出配置;
//@example: 将OUTPUT_CH_SIGNAL_MC_PWM0_H的Tx信号输出到IO_PORTA_05口:
// 			gpio_direction_output(IO_PORTA_05, 1); //设置IO为输出状态
//			gpio_och_sel_output_signal(IO_PORTA_05, OUTPUT_CH_SIGNAL_MC_PWM0_H); //将OUTPUT_CH_SIGNAL_MC_PWM0_H信号输出到IO_PORTA_05口
//=================================================================================//
int gpio_och_sel_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);

//=================================================================================//
//@brief: Output Channel 输出设置 API, 将指定IO释放外设控制, 变为普通IO;
//@input:
// 		gpio: 需要释放外设控制IO口, 释放后变为普通IO模式;
// 		signal: 将enum OUTPUT_CH_SIGNAL列表中需要取消输出的外设信号, 外设信号列表请查看gpio.h文件的enum OUTPUT_CH_SIGNAL枚举项;;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将OUTPUT_CH_SIGNAL_MC_PWM0_H取消输出IO_PORTA_05:
// 			gpio_och_disable_output_signal(IO_PORTA_05, OUTPUT_CH_SIGNAL_MC_PWM0_H);
//=================================================================================//
int gpio_och_disable_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);

//=================================================================================//
//@brief: Input Channel 输入设置 API, 将某个外设的输入设置为从某个IO输入
//@input:
// 		gpio: 需要输入外设信号的IO口;
// 		signal: 需要从指定IO输入的外设信号, 可以输入的外设信号列表请查看gpio.h文件enum INPUT_CH_SIGNAL枚举项;
//      type: INPUT_CH 类型, 常设为INPUT_CH_TYPE_GP_ICH;
//@return:  默认为0, 出错内部触发ASSERT;
//@note: 所映射的IO需要在设置IO状态为输入配置;
//@example: 将INPUT_CH_SIGNAL_TIMER0_CIN信号设置为IO_PORTA_05口输入:
//			gpio_set_die(IO_PORTA_05, 1); 		//数字输入使能
//			gpio_set_pull_up(IO_PORTA_05, 1);  //上拉输入使能
//			gpio_direction_input(IO_PORTA_05);  //设置IO为输入状态
//			gpio_ich_sel_input_signal(IO_PORTA_05, INPUT_CH_SIGNAL_TIMER0_CIN, INPUT_CH_TYPE_GP_ICH); //将INPUT_CH_SIGNAL_TIMER0_CIN信号设置为IO_PORTA_05口输入
//=================================================================================//
int gpio_ich_sel_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);

//=================================================================================//
//@brief: Input Channel 输入设置 API, 将某个外设信号释放IO口控制, 变为普通IO;
//@input:
// 		gpio: 需要取消输入外设信号的IO口;
// 		signal: 需要取消输入的外设信号, 外设信号列表请查看gpio.h文件enum INPUT_CH_SIGNAL枚举项;
//      type: INPUT_CH 类型, 常设为INPUT_CH_TYPE_GP_ICH;
//@return:  默认为0, 无出错处理;
//@note:
//@example: 将外设信号INPUT_CH_SIGNAL_TIMER0_CIN释放由某个IO输入:
// 			gpio_ich_disable_input_signal(IO_PORTA_05, INPUT_CH_SIGNAL_TIMER0_CIN, INPUT_CH_TYPE_GP_ICH);
//=================================================================================//
int gpio_ich_disable_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);

u32 gpio_get_ich_use_flag();
//获取空闲的gp_ich
//return: 0xff:error
u8 gpio_get_unoccupied_gp_ich();
//value:gp_ich序号
void gpio_release_gp_ich(u8 value);

//打印指定组别指定pin的crossbar信息
void gpio_crossbar_fo_dump(char px_name[], u8 max_px_out_num, u16 px_mask, u32 *omap_ptr);
void gpio_crossbar_fi_dump(char px_name[], u8 max_px_in_num, u16 px_mask, u8 px_in);
#endif //_GPIO_HW_H
