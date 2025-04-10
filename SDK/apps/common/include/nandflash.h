#ifndef _NANDFLASH_H
#define _NANDFLASH_H

#include "device/device.h"
#include "ioctl_cmds.h"
#include "spi.h"
#include "printf.h"
#include "gpio.h"
#include "device_drive.h"
#include "malloc.h"


#define GD_WRITE_ENABLE	        0x06
#define GD_WRITE_DISABLE	    0x04
#define GD_BLOCK_ERASE		    0xD8
#define GD_READ_ID		        0x9F
#define GD_READ_UID		        0x4B//only 1Gb
#define GD_RESET_DEVICE		    0xFF
/*for features*/
#define GD_GET_FEATURES         0x0F
#define GD_SET_FEATURES         0x1F
//<features addr
#define GD_FEATURES_PROTECT     0xA0
#define GD_FEATURES             0xB0
#define GD_GET_STATUS           0xC0
#define GD_DRIVE_STRENGTH       0xD0
/*for write*/
#define GD_PROGRAM_LOAD	        0x02
#define GD_PROGRAM_EXECUTE      0x10
#define GD_PROGRAM_LOAD_RANDOM_DATA     0x84
//for write x4
#define GD_PROGRAM_LOAD_X4	                  0x32
#define GD_PROGRAM_LOAD_RANDOM_DATA_X4        0x34//0x34 (1Gb:or 0xc4)
#define GD_PROGRAM_LOAD_RANDOM_DATA_QUAD_IO   0x72//only 1Gb
/*for read*/
#define GD_PAGE_READ_CACHE		0x13
#define GD_READ_FROM_CACHE		0x03//0x03/0x0b(from cache)
#define GD_READ_PAGE_CACHE_RANDOM		0x30//only 2Gb cache read(status reg bit7)
#define GD_READ_PAGE_CACHE_LAST		    0x3f//only 2Gb ending of cache read
//for read x2
#define GD_READ_FROM_CACHE_X2			0x3B
#define GD_READ_FROM_CACHE_DUAL_IO		0xBB
//for read x4
#define GD_READ_FROM_CACHE_X4			0x6B
#define GD_READ_FROM_CACHE_QUAD_IO		0xEB
//for protect
#define GD_PERMANENT_BLOCK_LOCK_PROTECTION 0x2c//only 2Gb. Permanently protect a specific group of blocks

#define NAND_OTP_PRT    BIT(7)
#define NAND_OTP_EN     BIT(6)
#define NAND_ECC_EN     BIT(4)
#define NAND_X4_EN      BIT(0)

#define NAND_STATUS_OIP      BIT(0)
#define NAND_STATUS_WEL      BIT(1)
#define NAND_STATUS_E_FAIL      BIT(2)
#define NAND_STATUS_P_FAIL      BIT(3)
#define NAND_STATUS_ECCS        (0xf0)

struct nandflash_dev_platform_data {
    int spi_hw_num;         //只支持SPI1或SPI2
    u32 spi_cs_port;        //cs的引脚
    u32 spi_read_width;     //flash读数据的线宽
    const struct spi_platform_data *spi_pdata;
    u32 start_addr;         //分区起始地址
    u32 size;               //分区大小，若只有1个分区，则这个参数可以忽略
};

#define NANDFLASH_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct nandflash_dev_platform_data data

#define NANDFLASH_DEV_PLATFORM_DATA_END()  \
};

void nandflash_test_demo(void);
extern const struct device_operations nandflash_dev_ops;
extern const struct device_operations ftl_dev_ops;


int nand_flash_erase(u32 address);

int nand_flash_read_page(u16 block, u8 page, u16 offset, u8 *buf, u16 len);

int nand_flash_read(u32 offset, u8 *buf,  u32 len);

int nand_flash_write(u32 offset, u8 *buf,  u32 len);

int nand_flash_write_page(u16 block, u8 page, u8 *buf, u16 len);

#endif

