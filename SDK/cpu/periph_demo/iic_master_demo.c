#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".iic_master_demo.data.bss")
#pragma data_seg(".iic_master_demo.data")
#pragma const_seg(".iic_master_demo.text.const")
#pragma code_seg(".iic_master_demo.text")
#endif
#include "clock.h"
#include "asm/wdt.h"

#define LOG_TAG             "[iic_m_demo]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if 1 //0:软件iic,  1:硬件iic
#define _IIC_USE_HW
#endif
#include "iic_api.h"

/******************************iic master test*****************************/
#if 1
#define IIC_SCL_IO IO_PORTA_01
#define IIC_SDA_IO IO_PORTA_02
void iic_master_polling_test()
{
    struct iic_master_config iic_config_test = {
        .role = IIC_MASTER,
        .scl_io = IIC_SCL_IO,
        .sda_io = IIC_SDA_IO,
        .io_mode = PORT_INPUT_PULLUP_10K,//上拉或浮空
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,   //enum GPIO_HDRIVE 0:2.4MA, 1:8MA, 2:26.4MA, 3:40MA
        .master_frequency = 100000, //软件iic频率不准(hz)
        .io_filter = 1,  //软件无效
    };
    //eeprom

#ifdef _IIC_USE_HW
    log_info("**********************hw iic test*************************\n");
    u8 eeprom_reg_addr = 66;
#else
    log_info("**********************soft iic test*************************\n");
    u8 eeprom_reg_addr = 64;
#endif
    enum iic_state_enum iic_init_state = iic_init(0, &iic_config_test);
    if (iic_init_state == IIC_OK) {
        log_info("iic master init ok");
    } else {
        log_error("iic master init fail");
        return;
    }

    u8 eeprom_wbuf[32], eeprom_rbuf[32];
    u8 eeprom_dev_addr = 0xa0;
    u8 eeprom_retry_cnt = 10;
    int eeprom_ret_len = 0;
    for (u8 i = 0; i < sizeof(eeprom_rbuf); i++) {
        eeprom_wbuf[i] = i % 26 + 'a';
        eeprom_rbuf[i] = 0;
    }
    eeprom_ret_len = i2c_master_read_nbytes_from_device_reg(0, eeprom_dev_addr, &eeprom_reg_addr, 1, eeprom_rbuf, 31);
    log_info("%s", eeprom_rbuf);
    memset(eeprom_rbuf, 0, 32);
    mdelay(20);
    //cppcheck-suppress redundantAssignment
    eeprom_ret_len = 0;
    puts(">>>> write in\n");
    while ((eeprom_ret_len != 8) && (--eeprom_retry_cnt)) {
        eeprom_ret_len = i2c_master_write_nbytes_to_device_reg(0, eeprom_dev_addr, &eeprom_reg_addr, 1, eeprom_wbuf, 8);
    }
    puts("<<<< write out\n");
    eeprom_retry_cnt = 10;
    mdelay(20);
    eeprom_ret_len = 0;
    puts(">>>> read in\n");
    while ((eeprom_ret_len != 31) && (--eeprom_retry_cnt)) {
        eeprom_ret_len = i2c_master_read_nbytes_from_device_reg(0, eeprom_dev_addr, &eeprom_reg_addr, 1, eeprom_rbuf, 31);
    }
    puts("<<<< read out\n");
    iic_init_state = iic_uninit(0);
    if (iic_init_state == IIC_OK) {
        log_info("iic master uninit ok");
    } else {
        log_error("iic master uninit fail");
    }

    log_info_hexdump(eeprom_rbuf, 32);
    log_info("%s", eeprom_rbuf);
}
#endif

