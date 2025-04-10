#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ftl_device.data.bss")
#pragma data_seg(".ftl_device.data")
#pragma const_seg(".ftl_device.text.const")
#pragma code_seg(".ftl_device.text")
#endif
#include "device/ioctl_cmds.h"
#include "device/device.h"
#include "nandflash.h"
#include "ftl_api.h"
#include "app_config.h"

#if TCFG_NANDFLASH_DEV_ENABLE

static u32 g_capacity = 0;
static struct device ftl_device;

static u32 get_first_one(u32 n)
{
    u32 pos = 0;
    for (pos = 0; pos < 32; pos++) {
        if (n & BIT(pos)) {
            return pos;
        }
    }
    return 0xff;
}

static enum ftl_error_t ftl_port_page_read(u16 block, u8 page, u16 offset, u8 *buf, int len)
{
    int ret = nand_flash_read_page(block, page, offset, buf, len);
    if (ret == 0) {
        return FTL_ERR_NO;
    }
    if (ret == 1) {
        return FTL_ERR_1BIT_ECC;
    }
    return FTL_ERR_READ;
}

static enum ftl_error_t ftl_port_page_write(u16 block, u8 page, u8 *buf, int len)
{
    int ret = nand_flash_write_page(block, page, buf, len);
    if (ret == 0) {
        return FTL_ERR_NO;
    }
    if (ret == 1) {
        return FTL_ERR_1BIT_ECC;
    }
    return FTL_ERR_WRITE;
}

static enum ftl_error_t ftl_port_erase_block(u32 addr)
{
    int ret = nand_flash_erase(addr);
    if (ret == 0) {
        return FTL_ERR_NO;
    }
    return FTL_ERR_WRITE;
}

static int ftl_dev_init(const struct dev_node *node, void *arg)
{
    return 0;
}

static int ftl_dev_open(const char *name, struct device **device, void *arg)
{
    dev_open("nand_flash", NULL);

    struct ftl_nand_flash flash;
    flash.block_begin = 0;
    flash.block_end = 1024;
    flash.logic_block_num = 1000;
    flash.page_num = 64;
    flash.page_size = 2 * 1024;
    flash.oob_offset = 0;
    flash.oob_size = 64;
    flash.max_erase_cnt = 10 * 10000;
    flash.page_size_shift = get_first_one(flash.page_size);
    flash.block_size_shift = get_first_one(flash.page_size * flash.page_num);
    flash.page_read = ftl_port_page_read;
    flash.page_write = ftl_port_page_write;
    flash.erase_block = ftl_port_erase_block;

    g_capacity = flash.logic_block_num << (flash.block_size_shift - 9);

    struct ftl_config config = {
        .page_buf_num = 4,
        .delayed_write_msec = 100,
    };
    ftl_init(&flash, &config);
    *device = &ftl_device;
    return 0;
}

static int ftl_dev_close(struct device *device)
{
    ftl_uninit();
    return 0;
}

static int ftl_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    enum ftl_error_t error;
    int rlen = ftl_read_bytes(offset * 512, buf, len * 512, &error);
    if (rlen < 0) {
        return 0;
    }
    return rlen / 512;
}

static int ftl_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    enum ftl_error_t error;
    int wlen = ftl_write_bytes(offset * 512, buf, len * 512, &error);
    if (wlen < 0) {
        return 0;
    }
    return wlen / 512;
}

static bool ftl_dev_online(const struct dev_node *node)
{
    return 1;
}

static int ftl_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    int reg = 0;

    switch (cmd) {
    case IOCTL_GET_STATUS:
        *(u32 *)arg = 1;
        break;
    case IOCTL_GET_ID:
        *((u32 *)arg) = 0xcd71;
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = 512;//usb fat
        break;
    case IOCTL_ERASE_BLOCK:
        break;
    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = g_capacity;
        break;
    case IOCTL_FLUSH:
        break;
    case IOCTL_CMD_RESUME:
        break;
    case IOCTL_CMD_SUSPEND:
        break;
    case IOCTL_CHECK_WRITE_PROTECT:
        *(u32 *)arg = 0;
        break;
    default:
        reg = -EINVAL;
        break;
    }
    return reg;
}

const struct device_operations ftl_dev_ops = {
    .init   = ftl_dev_init,
    .online = ftl_dev_online,
    .open   = ftl_dev_open,
    .read   = ftl_dev_read,
    .write  = ftl_dev_write,
    .ioctl  = ftl_dev_ioctl,
    .close  = ftl_dev_close,
};

#endif

