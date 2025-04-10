#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".device_config.data.bss")
#pragma data_seg(".device_config.data")
#pragma const_seg(".device_config.text.const")
#pragma code_seg(".device_config.text")
#endif
#include "system/includes.h"
#include "app_main.h"
#include "app_config.h"
#include "rtc.h"
#include "asm/sdmmc.h"
#include "linein_dev.h"
#include "usb/device/usb_stack.h"
#include "usb/host/usb_storage.h"
#include "ui/ui_api.h"
#include "ui_manage.h"
#include "iic_api.h"
#include "hdmi_cec_api.h"

#if TCFG_SD0_ENABLE
SD0_PLATFORM_DATA_BEGIN(sd0_data) = {
    .port = {
        TCFG_SD0_PORT_CMD,
        TCFG_SD0_PORT_CLK,
        TCFG_SD0_PORT_DA0,
        TCFG_SD0_PORT_DA1,
        TCFG_SD0_PORT_DA2,
        TCFG_SD0_PORT_DA3,
    },
    .data_width             = TCFG_SD0_DAT_MODE,
    .speed                  = TCFG_SD0_CLK,
    .detect_mode            = TCFG_SD0_DET_MODE,
    .priority				= 3,

#if (TCFG_SD0_DET_MODE == SD_IO_DECT)
    .detect_io              = TCFG_SD0_DET_IO,
    .detect_io_level        = TCFG_SD0_DET_IO_LEVEL,
    .detect_func            = sdmmc_0_io_detect,
#elif (TCFG_SD0_DET_MODE == SD_CLK_DECT)
    .detect_io_level        = TCFG_SD0_DET_IO_LEVEL,
    .detect_func            = sdmmc_0_clk_detect,
#else
    .detect_func            = sdmmc_cmd_detect,
#endif

#if (TCFG_SD0_POWER_SEL == SD_PWR_SDPG)
    .power                  = sd_set_power,
#else
    .power                  = NULL,
#endif

    SD0_PLATFORM_DATA_END()
};
#endif

/************************** linein KEY ****************************/
#if TCFG_APP_LINEIN_EN
struct linein_dev_data linein_data = {
    .enable = TCFG_APP_LINEIN_EN,
    .port   = NO_CONFIG_PORT,
    .up     = 1,
    .down   = 0,
    .ad_channel = NO_CONFIG_PORT,
    .ad_vol = 0,
};
#endif

/************************** rtc ****************************/
#if TCFG_APP_RTC_EN
//初始一下当前时间
const struct sys_time def_sys_time = {
    .year = 2020,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

//初始一下目标时间，即闹钟时间
const struct sys_time def_alarm = {
    .year = 2050,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

/* extern void alarm_isr_user_cbfun(u8 index); */
RTC_DEV_PLATFORM_DATA_BEGIN(rtc_data)
.default_sys_time = &def_sys_time,
 .default_alarm = &def_alarm,
#if defined(CONFIG_CPU_BR28)
  .rtc_clk = RTC_CLK_RES_SEL,
#endif
#if defined(CONFIG_CPU_BR27)
   .rtc_clk = RTC_CLK_RES_SEL,
    .rtc_sel = HW_RTC,
#endif
     //闹钟中断的回调函数,用户自行定义
     .cbfun = NULL,
      /* .cbfun = alarm_isr_user_cbfun, */
      RTC_DEV_PLATFORM_DATA_END()

#endif


#if TCFG_UI_ENABLE
#if TCFG_UI_LED7_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data) = {
    .pin_type = LED7_PIN7,
    .pin_cfg.pin7.pin[0] = TCFG_LED7_PIN0,
    .pin_cfg.pin7.pin[1] = TCFG_LED7_PIN1,
    .pin_cfg.pin7.pin[2] = TCFG_LED7_PIN2,
    .pin_cfg.pin7.pin[3] = TCFG_LED7_PIN3,
    .pin_cfg.pin7.pin[4] = TCFG_LED7_PIN4,
    .pin_cfg.pin7.pin[5] = TCFG_LED7_PIN5,
    .pin_cfg.pin7.pin[6] = TCFG_LED7_PIN6,
    LED7_PLATFORM_DATA_END()
};

const struct ui_devices_cfg ui_cfg_data = {
    .type = LED_7,
    .private_data = (void *) &led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */
#endif /*TCFG_UI_ENABLE*/

const struct iic_master_config soft_iic_cfg_const[MAX_SOFT_IIC_NUM] = {
    //soft iic0
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_SW_I2C0_CLK_PORT,
        .sda_io = TCFG_SW_I2C0_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_10K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_SW_I2C0_DELAY_CNT,  //IIC通讯波特率 未使用
        .io_filter = 0,                        //软件iic无滤波器
    },
#if 0
    //soft iic1
    {
        .role = IIC_MASTER,
        .scl_io = TCFG_SW_I2C1_CLK_PORT,
        .sda_io = TCFG_SW_I2C1_DAT_PORT,
        .io_mode = PORT_INPUT_PULLUP_10K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_SW_I2C1_DELAY_CNT,  //IIC通讯波特率 未使用
        .io_filter = 0,                        //软件iic无滤波器
    },
#endif
};

const struct iic_master_config hw_iic_cfg_const[MAX_HW_IIC_NUM] = {
    {
        .role = IIC_MASTER,
#if (defined CONFIG_CPU_BR36 || defined CONFIG_CPU_BR27 || defined CONFIG_CPU_BR28)
        .scl_io = TCFG_HW_I2C0_CLK_PORT,
        .sda_io = TCFG_HW_I2C0_DAT_PORT,
#else
#endif
        .io_mode = PORT_INPUT_PULLUP_10K,      //上拉或浮空，如果外部电路没有焊接上拉电阻需要置上拉
        .hdrive = PORT_DRIVE_STRENGT_2p4mA,    //IO口强驱
        .master_frequency = TCFG_HW_I2C0_CLK,  //IIC通讯波特率
        .io_filter = 1,                        //是否打开滤波器（去纹波）
    },
};

#if (TCFG_HW_SPI1_ENABLE || TCFG_HW_SPI2_ENABLE)
const struct spi_platform_data spix_p_data[HW_SPI_MAX_NUM] = {
    {
        //spi0
    },
    {
        //spi1
        .port = {
            TCFG_HW_SPI1_PORT_CLK, //clk any io
            TCFG_HW_SPI1_PORT_DO, //do any io
            TCFG_HW_SPI1_PORT_DI, //di any io
            0xff, //d2 any io
            0xff, //d3 any io
            0xff, //cs any io(主机不操作cs)
        },
        .role = TCFG_HW_SPI1_ROLE,//SPI_ROLE_MASTER,
        .clk  = TCFG_HW_SPI1_BAUD,
        .mode = TCFG_HW_SPI1_MODE,//SPI_MODE_BIDIR_1BIT,//SPI_MODE_UNIDIR_2BIT,
        .bit_mode = SPI_FIRST_BIT_MSB,
        .cpol = 0,//clk level in idle state:0:low,  1:high
        .cpha = 0,//sampling edge:0:first,  1:second
        .ie_en = 0, //ie enbale:0:disable,  1:enable
        .irq_priority = 3,
        .spi_isr_callback = NULL,  //spi isr callback
    },
#if SUPPORT_SPI2
    {
        //spi2
        .port = {
            TCFG_HW_SPI2_PORT_CLK, //clk any io
            TCFG_HW_SPI2_PORT_DO, //do any io
            TCFG_HW_SPI2_PORT_DI, //di any io
            0xff, //d2 any io
            0xff, //d3 any io
            0xff, //cs any io(主机不操作cs)
        },
        .role = TCFG_HW_SPI2_ROLE,//SPI_ROLE_MASTER,
        .clk  = TCFG_HW_SPI2_BAUD,
        .mode = TCFG_HW_SPI2_MODE,//SPI_MODE_BIDIR_1BIT,//SPI_MODE_UNIDIR_2BIT,
        .bit_mode = SPI_FIRST_BIT_MSB,
        .cpol = 0,//clk level in idle state:0:low,  1:high
        .cpha = 0,//sampling edge:0:first,  1:second
        .ie_en = 0, //ie enbale:0:disable,  1:enable
        .irq_priority = 3,
        .spi_isr_callback = NULL,  //spi isr callback
    },
#endif
};
#endif

#if TCFG_NANDFLASH_DEV_ENABLE
#include "nandflash.h"
NANDFLASH_DEV_PLATFORM_DATA_BEGIN(nandflash_dev_data) = {
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
    .spi_read_width = TCFG_FLASH_DEV_FLASH_READ_WIDTH,//flash读数据的线宽
    .start_addr     = 0,
    .size           = 256 * 1024 * 1024,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
};


#endif



REGISTER_DEVICES(device_table) = {
#if TCFG_SD0_ENABLE
    { "sd0", 	&sd_dev_ops,	(void *) &sd0_data},
#endif
#if TCFG_APP_LINEIN_EN
    { "linein",  &linein_dev_ops, (void *) &linein_data},
#endif
#if TCFG_APP_SPDIF_EN
    { "spdif",  &hdmi_dev_ops, NULL},
#endif
#if TCFG_APP_RTC_EN
#if TCFG_USE_VIRTUAL_RTC
    { "rtc",   &rtc_simulate_ops, (void *) &rtc_data},
#else
    { "rtc",   &rtc_dev_ops, (void *) &rtc_data},
#endif
#endif
#if TCFG_UDISK_ENABLE
    { "udisk0",   &mass_storage_ops, NULL},
#endif
#if TCFG_NANDFLASH_DEV_ENABLE
    {"nand_flash",   &nandflash_dev_ops, (void *) &nandflash_dev_data},
    {"nandflash_ftl",   &ftl_dev_ops, NULL },
#endif

};
