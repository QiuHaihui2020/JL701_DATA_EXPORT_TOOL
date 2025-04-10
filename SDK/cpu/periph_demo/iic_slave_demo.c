#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iic_slave_demo.data.bss")
#pragma data_seg(".iic_slave_demo.data")
#pragma const_seg(".iic_slave_demo.text.const")
#pragma code_seg(".iic_slave_demo.text")
#endif
#include "iic_api.h"
#include "clock.h"
#include "asm/wdt.h"

#define LOG_TAG             "[iic_s_demo]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if 1
#define IIC_SCL_IO IO_PORTA_02
#define IIC_SDA_IO IO_PORTA_03
/******************************hw iic slave test*****************************/
u8 slave_tx_buf_test[250];
u8 slave_rx_buf_test[255];
//rx协议:start,addr write,data0,data1,,,,,,stop
//tx协议:start,addr read,data0,data1,,,,,nack,stop
AT(.iic.text.cache.L1)
void hw_iic_isr_callback()//slave test
{
    static int rx_state = 0, tx_cnt = 0, rx_cnt = 1;
    u8 iic = 0;
    if (rx_state == 0) {
        rx_state = hw_iic_slave_rx_byte(iic, &slave_rx_buf_test[0]);//addr
        if (rx_state == IIC_SLAVE_RX_ADDR_RX) { //rx
            hw_iic_slave_rx_prepare(iic, 1, 0);
        } else if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
            hw_iic_slave_tx_byte(iic, slave_tx_buf_test[tx_cnt++]);
        } else { //error
            log_error("iic slave rx addr error!\n");
            rx_state = 0;
        }
    }

    if (hw_iic_get_pnd(iic, I2C_PND_STOP)) {
        /* putchar('e'); */
        hw_iic_clr_all_pnd(iic);
        hw_iic_slave_rx_prepare(iic, 0, 0);
        if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
            log_info_hexdump(slave_rx_buf_test, rx_cnt);
        }
        rx_state = 0;
        tx_cnt = 0;
        rx_cnt = 1;
    }

    if (hw_iic_get_pnd(iic, I2C_PND_TASK_DONE)) {
        if (rx_state == IIC_SLAVE_RX_ADDR_TX) { //tx
            if (hw_iic_slave_tx_check_ack(iic) == 0) { //no ack
                return;
            }
            hw_iic_slave_tx_byte(iic, slave_tx_buf_test[tx_cnt++]);
        } else if (rx_state >= IIC_SLAVE_RX_ADDR_RX) { //rx
            hw_iic_slave_rx_byte(iic, &slave_rx_buf_test[rx_cnt++]);
            hw_iic_slave_rx_prepare(iic, 1, 0);
        }
    }
}
void hw_iic_slave_polling_test()
{
    struct hw_iic_slave_config hw_iic_config_test = {
        .config.role = IIC_SLAVE,
        .config.scl_io = IIC_SCL_IO,
        .config.sda_io = IIC_SDA_IO,
        .config.io_mode = PORT_INPUT_PULLUP_10K,//上拉或浮空
        .config.hdrive = PORT_DRIVE_STRENGT_2p4mA,   //enum GPIO_HDRIVE 0:2.4MA, 1:8MA, 2:26.4MA, 3:40MA
        .config.ie_en = 0,
        .config.io_filter = 1,
        .slave_addr = 0x68,
        .iic_slave_irq_callback = NULL,
    };
    for (u16 i = 0; i < sizeof(slave_rx_buf_test); i++) {
        slave_rx_buf_test[i] = 0;
    }
    for (u8 i = 0; i < sizeof(slave_tx_buf_test); i++) {
        slave_tx_buf_test[i] = i;
    }

    //ie
#if 0//ie test
    hw_iic_config_test.config.ie_en = 1;
    hw_iic_config_test.iic_slave_irq_callback = hw_iic_isr_callback;
    hw_iic_slave_init(0, &hw_iic_config_test);
    while (1) {
        wdt_clear();
    }
#endif

    //polling
    enum iic_state_enum iic_init_state = hw_iic_slave_init(0, &hw_iic_config_test);
    if (iic_init_state == IIC_OK) {
        log_info("iic_slave init ok");
    }
    /* JL_PORTB->DIR &= ~BIT(3);//test io */
    /* JL_PORTB->OUT &= ~BIT(3); */
#if 0//rx test

    while (1) {
        hw_iic_slave_polling_rx(0, slave_rx_buf_test);
        wdt_clear();
    }
#else//tx test

    while (1) {
        log_info("------------iic slave polling tx test------------\n");
        if (hw_iic_slave_polling_tx(0, slave_tx_buf_test)) {
            log_info("iic slave tx ok\n");
        } else {
            log_info("iic slave tx fail\n");
        }
        wdt_clear();
    }
#endif
}
#endif

