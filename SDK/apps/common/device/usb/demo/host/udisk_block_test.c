#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".udisk_block_test.data.bss")
#pragma data_seg(".udisk_block_test.data")
#pragma const_seg(".udisk_block_test.text.const")
#pragma code_seg(".udisk_block_test.text")
#endif
#if 0
#include "cpu/includes.h"
#include "system/timer.h"
#include "system/event.h"
#include "device/ioctl_cmds.h"
#include "device_drive.h"
#include "app_config.h"
#include "app_msg.h"

#include "usb_config.h"
#include "usb/host/usb_host.h"
//#include "usb_ctrl_transfer.h"
//#include "usb_bulk_transfer.h"
#include "usb_storage.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static u8 msd_test_buf[32 * 1024];

#define UDISK_SPEED_TEST_EN     0   //速度测试总控使能
#define UDISK_W_TIME_EN         1   //写入速度测试使能
#define UDISK_R_TIME_EN         1   //读取速度测试使能
#define BLOCKS_TO_KICKSTARTS    64  //多少块统计一次时间
static JL_TIMER_TypeDef *pTimer = JL_TIMER3;
static u32 timer_irq_idx = IRQ_TIME3_IDX;
static u32 t_counter;
static u32 wr_blocks;
static u32 rd_blocks;
static u32 wr_time;
static u32 rd_time;
static u32 wr_KBps_min;
static u32 rd_KBps_min;
static u32 wr_KBps_max;
static u32 rd_KBps_max;
static u32 wr_KBps_avg;
static u32 rd_KBps_avg;
___interrupt
void udisk_speed_test_irq()
{
    pTimer->CON |= BIT(14);
    t_counter++;
}
void udisk_speed_test_init()
{
    wr_blocks = 0;
    rd_blocks = 0;
    wr_time = 0;
    rd_time = 0;
    t_counter = 0;
    wr_KBps_min = 0xffffffff;
    rd_KBps_min = 0xffffffff;
    wr_KBps_max = 0;
    rd_KBps_max = 0;
    wr_KBps_avg = 0;
    rd_KBps_avg = 0;
    printf("lsb clock %d\n", clk_get("lsb"));
    request_irq(timer_irq_idx, 7, udisk_speed_test_irq, 0);
    pTimer->CNT = 0;
    pTimer->PRD = clk_get("lsb") / 4 * 10 / 1000; //10ms
    pTimer->CON = (1 << 4) | (1 << 10) | (1 << 14);  //sel lsb_clk, div4
}
void udisk_speed_test_uninit()
{
    if (pTimer) {
        unrequest_irq(timer_irq_idx);
        pTimer->CON = BIT(14);
        //pTimer = NULL;
    }
}
void udisk_speed_test_probe()
{
    if (pTimer) {
        t_counter = 0;
        pTimer->CNT = 0;
        pTimer->CON |= BIT(0) | BIT(14);
    }
}
u32 udisk_speed_test_capture()
{
    u32 time = 0;
    if (pTimer) {
        pTimer->CON &= ~BIT(0);
        time = (t_counter * pTimer->PRD + pTimer->CNT) / (clk_get("lsb") / 4000000);  //unit in us
    }
    return time;
}
void udisk_write_speed_dump(u32 cur_time, u32 numOflba)
{
    u32 KBps = 0;
    if (pTimer) {
        wr_time += cur_time;
        wr_blocks += numOflba;
        if (wr_blocks >= BLOCKS_TO_KICKSTARTS) {
            // Bytes/us = (KB/1024)/(s/1000000) = KB/s * (1000000/1024) =
            // KB/s * (16075 / 16) = (15625 / 16) KB/s
            KBps = (wr_blocks * 512) * 15625 / 16 / wr_time;
            printf("w: %d blocks, t %d us, %d KB/s\n", wr_blocks, wr_time, KBps);
            if (KBps < wr_KBps_min) {
                wr_KBps_min = KBps;
            }
            if (KBps > wr_KBps_max) {
                wr_KBps_max = KBps;
            }
            wr_KBps_avg = (wr_KBps_avg + KBps) / 2;
            wr_time = 0;
            wr_blocks = 0;
        }
    }
}
void udisk_read_speed_dump(u32 cur_time, u32 numOflba)
{
    u32 KBps = 0;
    if (pTimer) {
        rd_time += cur_time;
        rd_blocks += numOflba;
        if (rd_blocks >= BLOCKS_TO_KICKSTARTS) {
            // Bytes/us = (KB/1024)/(s/1000000) = KB/s * (1000000/1024) =
            // KB/s * (16075 / 16) = (15625 / 16) KB/s
            KBps = (rd_blocks * 512) * 15625 / 16 / rd_time;
            printf("r: %d blocks, t %d us, %d KB/s\n", rd_blocks, rd_time, KBps);
            if (KBps < rd_KBps_min) {
                rd_KBps_min = KBps;
            }
            if (KBps > rd_KBps_max) {
                rd_KBps_max = KBps;
            }
            rd_KBps_avg = (rd_KBps_avg + KBps) / 2;
            rd_time = 0;
            rd_blocks = 0;
        }
    }
}
void udisk_speed_summary_dump()
{
    printf("w: min %d, max %d, avg %d\n", wr_KBps_min, wr_KBps_max, wr_KBps_avg);
    printf("r: min %d, max %d, avg %d\n", rd_KBps_min, rd_KBps_max, rd_KBps_avg);
}
void usb_host_test()
{
___usb_test:
    //usb_host_mount(0, 3, 20, 200);
    struct device *device;
    device = dev_open("udisk0", NULL);
    log_info("%s---%d\n", __func__, __LINE__);
    u32 lba = 0;
    while (1) {

        u32 *p = (u32 *)msd_test_buf;
        u32 numOflba = sizeof(msd_test_buf) / 512;

        if ((dev_bulk_read(device, p, lba, numOflba)) != numOflba) {
            printf("read error %d", lba);
            while (1);
        }
#if 1
        if (dev_bulk_write(device, p, lba, numOflba) != numOflba) {

            printf("write error %d", lba);
            while (1);
        }

        if ((dev_bulk_read(device, p, lba, numOflba)) != numOflba) {
            printf("read error %d", lba);
            while (1);
        }
#endif
        for (int i = 0; i < numOflba * 512 / 4; i++) {
            if (p[i] != (i + lba * 0x80)) {
                printf("@lba:%x offset %x   %x != %x ", lba, i, p[i], i + lba * 0x80);
                /* put_buf(p, numOflba * 512); */
                /* while (1); */
            }
        }
        wdt_clear();

        lba += numOflba;
    }
}
void usb_host_test2()
{
    int err = 0;
    struct device *device;

    printf("udisk test start\n");
    device = dev_open("udisk0", NULL);
    if (!device) {
        return;
    }
    //测试起始地址
    u32 lba = 0;
    //每一次读写大小(block)
    u32 numOflba = 4 * 1024 / 512;
    ASSERT(numOflba * 512 * 2 <= sizeof(msd_test_buf));
    u32 *p1 = (u32 *)((u32)msd_test_buf);
    u32 *p2 = (u32 *)((u32)msd_test_buf + numOflba * 512);
    u32 seed;
    u32 block_num = 0;
    u32 block_size = 0;
    //总测试数据量(byte)，设一个较大的值可用于煲机测试
    u32 test_size = 20 * 1024 * 1024;
    u32 xus = 0;
    extern void wdt_clr();

    dev_ioctl(device, IOCTL_GET_BLOCK_NUMBER, (u32)&block_num);
    dev_ioctl(device, IOCTL_GET_BLOCK_SIZE, (u32)&block_size);
    printf("block_num %d, block_size %d\n", block_num, block_size);
#if UDISK_SPEED_TEST_EN
    udisk_speed_test_init();
#endif
    test_size /= 512;
    while (test_size) {
        if (numOflba > test_size) {
            numOflba = test_size;
        }
        seed = rand32();
        for (int i = 0; i < numOflba * 512 / 4; i++) {
            p1[i] = seed + i;
        }
#if UDISK_SPEED_TEST_EN && UDISK_W_TIME_EN
        udisk_speed_test_probe();
#endif
        err = dev_bulk_write(device, (u8 *)p1, lba, numOflba);
#if UDISK_SPEED_TEST_EN && UDISK_W_TIME_EN
        xus = udisk_speed_test_capture();
        udisk_write_speed_dump(xus, numOflba);
#endif
        if (err != numOflba) {
            printf("udisk write fail, lba %x, err %d\n", lba, err);
            /* goto __exit; */
            while (1) {
                wdt_clr();
            }
        }
        wdt_clr();
        memset((u8 *)p2, 0, numOflba * 512);
#if UDISK_SPEED_TEST_EN && UDISK_R_TIME_EN
        udisk_speed_test_probe();
#endif
        err = dev_bulk_read(device, (u8 *)p2, lba, numOflba);
#if UDISK_SPEED_TEST_EN && UDISK_R_TIME_EN
        xus = udisk_speed_test_capture();
        udisk_read_speed_dump(xus, numOflba);
#endif
        if (err != numOflba) {
            printf("udisk read fail, lba %x, err %d\n", lba, err);
            /* goto __exit; */
            while (1) {
                wdt_clr();
            }
        }
        wdt_clr();

        for (int i = 0; i < numOflba * 512 / 4; i++) {
            if (p1[i] != p2[i]) {
                printf("udisk read/write data different, lba %x, offset %x, offset %% 16 = %x\n", lba, i * 4, i * 4 % 16);
                printf_buf(&((u8 *)p1)[i * 4 / 16 * 16 - 32], 64);
                printf("-----------------------------\n");
                printf_buf(&((u8 *)p2)[i * 4 / 16 * 16 - 32], 64);
                /* goto __exit; */
                while (1) {
                    wdt_clr();
                }
            }
        }
        printf("lba %d pass\n", lba);

        lba += numOflba;
        test_size -= numOflba;
    }
    printf("udisk test pass\n");

__exit:
#if UDISK_SPEED_TEST_EN
    udisk_speed_summary_dump();
    udisk_speed_test_uninit();
#endif
    dev_close(device);
}

static int udisk_test_event_handler(int *msg)
{
    char *usb_msg;
    int ret = 0;
    if (msg[0] == DEVICE_EVENT_FROM_USB_HOST) {
        usb_msg = (char *)msg[2];
        if (!strncmp(usb_msg, "udisk", 5)) {
            if (msg[1] == DEVICE_EVENT_IN) {
                usb_host_test2();
            } else if (msg[1] == DEVICE_EVENT_OUT) {
            }
            ret = 1;
        }
    }
    return ret;
}
APP_MSG_PROB_HANDLER(udisk_test_msg_entry) = {
    .owner = 0xff,
    .from = MSG_FROM_DEVICE,
    .handler = udisk_test_event_handler,
};

#endif
