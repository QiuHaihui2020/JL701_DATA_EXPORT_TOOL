#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sd_test.data.bss")
#pragma data_seg(".sd_test.data")
#pragma const_seg(".sd_test.text.const")
#pragma code_seg(".sd_test.text")
#endif

#if 0

#include "system/includes.h"
#include "fs/fs.h"


extern void srand(unsigned int __seed) __THROW;
#include "generic/ascii.h"
#define CONFIG_ROOT_PATH "storage/sd0/C/sdtest"
extern void force_set_sd_online(char *sdx);
void sd_rw_test(u8 *buffer)
{
    void *fd = dev_open("sd0", NULL);
    u32 lba = 0;
    while (fd) {
        int r = dev_bulk_read(fd, buffer, lba++, 1);
        if (r != 1) {
            goto __sd_error;
        }

        put_buf(buffer, 512);
    }

__sd_error:
    puts(" ===sd rw err===\n");
    while (1) {};

}
static void test_main(void *p)
{
    force_set_sd_online("sd0");
    u8 *buffer = dma_malloc(512);

    void *mnt = mount("sd0", "storage/sd0", "fat", 3, NULL);
    if (!mnt) {
        puts(" ===sd mount err===\n");
        goto __fs_error;
    }
#ifndef CONFIG_CPU_BR52
    FILE *fp = NULL;
    char file_name[128];
    u32 seed = JL_RAND->R64L;
    snprintf(file_name, sizeof(file_name), CONFIG_ROOT_PATH"%d.bin", seed);
    srand(seed);
    fp = fopen(file_name, "w+");
    if (!fp) {
        puts("fopen err\n");
        dma_free(buffer);
        goto __fs_error;
    }
    while (1) {
        fwrite(buffer, 512, 1, fp);
    }
#endif
__fs_error:
    sd_rw_test(buffer);
}

void sd_demo()
{
    int err = task_create(test_main, NULL, "periph_demo");
    if (err != OS_NO_ERR) {
        r_printf("creat fail %x\n", err);
    }
}
#endif
