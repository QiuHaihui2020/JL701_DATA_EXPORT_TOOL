#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cfg_private.data.bss")
#pragma data_seg(".cfg_private.data")
#pragma const_seg(".cfg_private.text.const")
#pragma code_seg(".cfg_private.text")
#endif
#include "system/includes.h"
#include "boot.h"
#include "asm/sfc_norflash_api.h"
#include "cfg_private.h"
#include "sdfile.h"
/* #include "fs/resfile.h" */

typedef struct {
    u32 cfg_private_file_num;
    SDFILE_FILE_HEAD head;
    u8 cfg_private_create_flag;
    u32 cfg_private_part_flash_addr;
    u32 cfg_private_part_maxsize;
    u32 cfg_private_align_size;
    u32 cfg_private_file_maxsize;
} CFG_PRIVATE_HDL;

CFG_PRIVATE_HDL *cp_hdl = NULL;


static RESFILE *cfg_private_create(const char *path, const char *mode)
{
    int i = 0;
    RESFILE *file = NULL;
    char *buf_temp = NULL;
    int file_head_len = 0;

    char *str = strrchr(path, '/');

    buf_temp = (char *)malloc(cp_hdl->cfg_private_align_size);
    norflash_read(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, cp_hdl->cfg_private_part_flash_addr);
    for (; i < cp_hdl->cfg_private_file_num; i++) {
        int addr = cp_hdl->cfg_private_part_flash_addr + i * sizeof(SDFILE_FILE_HEAD);
        norflash_read(NULL, (void *)&cp_hdl->head, sizeof(SDFILE_FILE_HEAD), addr);
        if (CRC16((u8 *)&cp_hdl->head + 2, sizeof(SDFILE_FILE_HEAD) - 2) == cp_hdl->head.head_crc) {
            file_head_len += cp_hdl->head.len;
            continue;
        }
        memset(&cp_hdl->head, 0, sizeof(SDFILE_FILE_HEAD));
        cp_hdl->head.len = cp_hdl->cfg_private_part_maxsize - cp_hdl->cfg_private_file_num * sizeof(SDFILE_FILE_HEAD) - file_head_len; //分配剩余空间都给新文件，最后close后修改最终长度
        if (cp_hdl->cfg_private_file_maxsize) {
            if (cp_hdl->head.len < cp_hdl->cfg_private_file_maxsize) { //判断申请长度是否大于剩余容量
                r_printf(">>>[test]:file maxsize is over\n");
                goto __exit;
            }
            cp_hdl->head.len = cp_hdl->cfg_private_file_maxsize;
        }
        /* y_printf(">>>[test]:file len = %d\n", cp_hdl->head.len); */
        cp_hdl->head.addr = cp_hdl->cfg_private_file_num * sizeof(SDFILE_FILE_HEAD) + file_head_len; //使用相对part地址
        /* y_printf(">>>[test]:file flash addr = 0x%x\n", cp_hdl->head.addr); */
        memcpy(cp_hdl->head.name, (char *)(str + 1), strlen(str + 1));
        cp_hdl->head.attr = 0x02;
        cp_hdl->head.head_crc = CRC16((u8 *)&cp_hdl->head + 2, sizeof(SDFILE_FILE_HEAD) - 2);
        break;
    }
    if (i >= cp_hdl->cfg_private_file_num) {
        r_printf(">>>[test]:file head is over\n");
        goto __exit;
    }
    cfg_private_erase(cp_hdl->cfg_private_part_flash_addr, cp_hdl->cfg_private_align_size);
    memcpy(buf_temp + i * sizeof(SDFILE_FILE_HEAD), &cp_hdl->head, sizeof(SDFILE_FILE_HEAD));
    norflash_write(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, cp_hdl->cfg_private_part_flash_addr);
    /* put_buf((const u8 *)buf_temp, cp_hdl->cfg_private_align_size); */
    file = resfile_open(path);
    /* y_printf(">>>[test]:path = %s, file = 0x%x\n", path, file); */

__exit:
    if (buf_temp) {
        free(buf_temp);
    }
    return file;
}

RESFILE *cfg_private_open(const char *path, const char *mode)
{
    int res;
    RESFILE *file = resfile_open(path);
    if (!file) {
        if (mode[0] == 'w' && mode[1] == '+') {
            file = cfg_private_create(path, mode);
            if (file && !cp_hdl->cfg_private_file_maxsize) { //如果设置了最大长度则不需要配置标志
                //置上标志，close时修改最终实际写入长度
                cp_hdl->cfg_private_create_flag = 1;
            }
        }
    }
    cp_hdl->cfg_private_file_maxsize = 0;
    return file;
}

int cfg_private_read(RESFILE *file, void *buf, u32 len)
{
    return resfile_read(file, buf, len);
}

void cfg_private_erase(u32 addr, u32 len)
{
    /* r_printf(">>>[test]:addr = 0x%x, len = %d\n", addr, len); */
    u32 erase_total_size = len;
    u32 erase_addr = addr;
    u32 erase_size = 4096;
    u32 erase_cmd = IOCTL_ERASE_SECTOR;
    //flash不同支持的最小擦除单位不同(page/sector)
    //boot_info.vm.align == 1: 最小擦除单位page;
    //boot_info.vm.align != 1: 最小擦除单位sector;
    if (boot_info.vm.align == 1) {
        erase_size = 256;
        erase_cmd = IOCTL_ERASE_PAGE;
    }
    while (erase_total_size) {
        //擦除区域操作
        norflash_ioctl(NULL, erase_cmd, erase_addr);
        erase_addr += erase_size;
        erase_total_size -= erase_size;
    }
}

static int cfg_private_check(char *buf, int len)
{
    for (int i = 0; i < len; i++) {
        if ((u8)buf[i] != 0xff) {
            return 0;
        }
    }
    return 1;
}

int cfg_private_write(RESFILE *file, void *buf, u32 len)
{
    struct resfile_attrs attrs = {0};
    resfile_get_attrs(file, &attrs);
    /* r_printf(">>>[test]:attrs.sclust = 0x%x\n", attrs.sclust); */
    attrs.sclust = sdfile_cpu_addr2flash_addr(attrs.sclust);
    u32 fptr = resfile_get_pos(file);
    /* r_printf(">>>[test]:addr = 0x%x, fsize = %d, fptr = %d, w_len = %d\n", attrs.sclust, attrs.fsize, fptr, len); */
    if (len + fptr > attrs.fsize) {
        r_printf(">>>[test]:error, write over!!!!!!!!\n");
        return -1;
    }
    char *buf_temp = (char *)malloc(cp_hdl->cfg_private_align_size);
    int res = len;

    for (int wlen = 0; len > 0;) {
        u32 align_addr = (attrs.sclust + fptr) / cp_hdl->cfg_private_align_size * cp_hdl->cfg_private_align_size;
        u32 w_pos = attrs.sclust + fptr - align_addr;
        wlen = cp_hdl->cfg_private_align_size - w_pos;
        /* y_printf(">>>[test]:wpos = %d, wlen = %d\n", w_pos, wlen); */
        if (wlen > len) {
            wlen = len;
        }
        norflash_read(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, align_addr);
        if (0 == cfg_private_check(buf_temp, cp_hdl->cfg_private_align_size)) {
            cfg_private_erase(align_addr, cp_hdl->cfg_private_align_size);
        }
        memcpy(buf_temp + w_pos, buf, wlen);
        /* put_buf(buf_temp, cp_hdl->cfg_private_align_size); */
        norflash_write(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, align_addr);
        fptr += wlen;
        len -= wlen;
    }
__exit:
    free(buf_temp);
    resfile_seek(file, fptr, SEEK_SET);
    return res;
}

int cfg_private_erase_file(RESFILE *file)
{
    struct resfile_attrs attrs = {0};
    resfile_get_attrs(file, &attrs);
    attrs.sclust = sdfile_cpu_addr2flash_addr(attrs.sclust);
    int len = attrs.fsize;
    char *buf_temp = (char *)malloc(cp_hdl->cfg_private_align_size);
    u32 fptr = 0;

    for (int wlen = 0; len > 0;) {
        u32 align_addr = (attrs.sclust + fptr) / cp_hdl->cfg_private_align_size * cp_hdl->cfg_private_align_size;
        u32 w_pos = attrs.sclust + fptr - align_addr;
        wlen = cp_hdl->cfg_private_align_size - w_pos;
        if (wlen > len) {
            wlen = len;
        }
        norflash_read(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, align_addr);
        if (0 == cfg_private_check(buf_temp, cp_hdl->cfg_private_align_size)) {
            cfg_private_erase(align_addr, cp_hdl->cfg_private_align_size);
        }
        memset(buf_temp + w_pos, 0xff, wlen);
        norflash_write(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, align_addr);
        fptr += wlen;
        len -= wlen;
    }
    free(buf_temp);
    resfile_close(file);
    return 0;
}

int cfg_private_close(RESFILE *file)
{
    if (cp_hdl->cfg_private_create_flag) {
        int i = 0;
        char name[SDFILE_NAME_LEN];
        resfile_get_name(file, name, SDFILE_NAME_LEN);
        char *buf_temp = (char *)malloc(cp_hdl->cfg_private_align_size);
        norflash_read(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, cp_hdl->cfg_private_part_flash_addr);
        for (; i < cp_hdl->cfg_private_file_num; i++) {
            int addr = cp_hdl->cfg_private_part_flash_addr + i * sizeof(SDFILE_FILE_HEAD);
            norflash_read(NULL, (void *)&cp_hdl->head, sizeof(SDFILE_FILE_HEAD), addr);
            if (CRC16((u8 *)&cp_hdl->head + 2, sizeof(SDFILE_FILE_HEAD) - 2) != cp_hdl->head.head_crc) {
                continue;
            }
            if (0 == memcmp(cp_hdl->head.name, name, SDFILE_NAME_LEN)) {
                cp_hdl->head.len = resfile_get_pos(file);
                y_printf(">>>[test]:real file len = %d\n", cp_hdl->head.len);
                cp_hdl->head.head_crc = CRC16((u8 *)&cp_hdl->head + 2, sizeof(SDFILE_FILE_HEAD) - 2);
                break;
            }
        }
        cp_hdl->cfg_private_create_flag = 0;
        if (i < cp_hdl->cfg_private_file_num) {
            cfg_private_erase(cp_hdl->cfg_private_part_flash_addr, cp_hdl->cfg_private_align_size);
            memcpy(buf_temp + i * sizeof(SDFILE_FILE_HEAD), &cp_hdl->head, sizeof(SDFILE_FILE_HEAD));
            norflash_write(NULL, (void *)buf_temp, cp_hdl->cfg_private_align_size, cp_hdl->cfg_private_part_flash_addr);
        }
        if (buf_temp) {
            free(buf_temp);
        }
    }
    return resfile_close(file);
}

int cfg_private_seek(RESFILE *file, int offset, int fromwhere)
{
    return resfile_seek(file, offset, fromwhere);
}

int cfg_private_init(int file_num, const char *part_path)
{
    int ret = CFG_PRIVATE_OK;
    RESFILE *file = NULL;

    cp_hdl = malloc(sizeof(CFG_PRIVATE_HDL));
    if (!cp_hdl) {
        ret = CFG_PRIVATE_INIT_MALLOC_ERR;
        goto __exit;
    }
    memset(cp_hdl, 0, sizeof(CFG_PRIVATE_HDL));
    //给与初值
    cp_hdl->cfg_private_file_num = 5;
    cp_hdl->cfg_private_align_size = 4096;
    if (boot_info.vm.align == 1) {
        cp_hdl->cfg_private_align_size = 256;
    }


    u32 max_file_num = cp_hdl->cfg_private_align_size / sizeof(SDFILE_FILE_HEAD);
    cp_hdl->cfg_private_file_num = file_num;
    if (cp_hdl->cfg_private_file_num > max_file_num) {
        r_printf(">>>[test]:file num %d is over max_num:%d\n", file_num, max_file_num);
        ret = CFG_PRIVATE_INIT_FILE_NUM_OVER;
        goto __exit;
    }

    r_printf(">>>[test]:part_path = %s\n", part_path);
    file = resfile_open(part_path);
    if (!file) {
        r_printf(">>>[test]:no apply part!!!!!!!\n");
        ret = CFG_PRIVATE_INIT_NO_PART;
        goto __exit;
    }
    struct resfile_attrs attrs = {0};
    resfile_get_attrs(file, &attrs);
    r_printf(">>>[test]:init logic attrs.sclust = 0x%x\n", attrs.sclust);
    attrs.sclust = sdfile_cpu_addr2flash_addr(attrs.sclust);
    r_printf(">>>[test]:init flash attrs.sclust = 0x%x\n", attrs.sclust);
    if (attrs.sclust % cp_hdl->cfg_private_align_size) {
        r_printf(">>>[test]:part start addr is not alignment, sclust = %d, cp_hdl->cfg_private_align_size = %d\n", attrs.sclust, cp_hdl->cfg_private_align_size);
        ret = CFG_PRIVATE_INIT_ADDR_NO_ALIGN;
        goto __exit;
    }
    cp_hdl->cfg_private_part_flash_addr = attrs.sclust;
    cp_hdl->cfg_private_part_maxsize = attrs.fsize;
__exit:
    if (file) {
        resfile_close(file);
    }
    return ret;
}

int cfg_private_uninit()
{
    if (cp_hdl) {
        free(cp_hdl);
    }
    return CFG_PRIVATE_OK;
}

RESFILE *cfg_private_open_by_maxsize(const char *path, const char *mode, int file_maxsize)
{
    cp_hdl->cfg_private_file_maxsize = file_maxsize; //设置单个文件最大长度
    return cfg_private_open(path, mode);
}

void cfg_private_test_demo(void)
{
#if 0
#define N 512
    char test_buf[N] = {0};
    char r_buf[512] = {0};
    for (int i = 0; i < N; i++) {
        test_buf[i] = i & 0xff;
    }

#if 0
    char path[64] = "mnt/sdfile/app/FATFSI/";
    char path1[64] = "mnt/sdfile/app/FATFSI/eq_cfg_hw.bin";
    char path2[64] = "mnt/sdfile/app/FATFSI/cfg_tool.bin";
#else
    char path[64] = "flash/APP/FATFSI/";
    char path1[64] = "flash/APP/FATFSI/eq_cfg_hw.bin";
    char path2[64] = "flash/APP/FATFSI/cfg_tool.bin";
#endif
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);

    ////////////初始化//////////
    y_printf(">>>[test]:init-------------\n");
    int ret = cfg_private_init(10, path);
    if (ret) {
        r_printf(">>>[test]:err init \n");
        goto _exit;
    }
    //////////////////////

    ///////////创建文件//////////
    y_printf(">>>[test]:创建第一个文件-------------\n");
    RESFILE *fp = cfg_private_open_by_maxsize(path1, "w+", 2 * 1024);
    if (!fp) {
        r_printf(">>>[test]:open fail!!!!!\n");
        goto _exit;
    }
    /* cfg_private_erase_file(fp); */
    /* fp = cfg_private_open(path, "w+"); */
    /* cfg_private_read(fp, r_buf, 512); */
    /* put_buf(r_buf, 512); */
    /* cfg_private_seek(fp, 0, SEEK_SET); */

    y_printf(">>>[test]:写第一个文件-------------\n");
    cfg_private_write(fp, test_buf, N);

    cfg_private_close(fp);
    y_printf(">>>[test]:读第一个文件-------------\n");
    fp = cfg_private_open_by_maxsize(path1, "r", 2 * 1024);
    cfg_private_read(fp, r_buf, 512);
    put_buf((const u8 *)r_buf, 512);
    cfg_private_close(fp);
    //////////////////////

    ///////////创建文件//////////
    y_printf(">>>[test]:创建第二个文件-------------\n");
    fp = cfg_private_open_by_maxsize(path2, "w+", 1024);
    if (!fp) {
        r_printf(">>>[test]:open fail!!!!!\n");
        goto _exit;
    }
    for (int i = 0; i < N; i++) {
        test_buf[i] = (N - 1 - i) & 0xff;
    }
    y_printf(">>>[test]:写第二个文件-------------\n");
    cfg_private_write(fp, test_buf, N);
    cfg_private_close(fp);
    y_printf(">>>[test]:读第二个文件-------------\n");
    fp = cfg_private_open_by_maxsize(path2, "r", 1024);
    cfg_private_read(fp, r_buf, 512);
    put_buf((const u8 *)r_buf, 512);
    cfg_private_close(fp);
    //////////////////////

    cfg_private_uninit();

#if 0
    RESFILE *file = cfg_private_open("mnt/sdfile/app/FATFSI", "w+");
    if (!file) {
        r_printf(">>>[test]:open fail!!!!!\n");
        goto _exit;
    }
    struct resfile_attrs attrs = {0};
    resfile_get_attrs(file, &attrs);
    y_printf(">>>[test]:in part addr = %d, fsize = %d\n", attrs.sclust, attrs.fsize);
    char *part = zalloc(attrs.fsize);
    cfg_private_erase_file(file);
    file = cfg_private_open("mnt/sdfile/app/FATFSI", "w+");
    cfg_private_read(file, part, attrs.fsize);
    put_buf(part, attrs.fsize);
    free(part);
#endif
_exit:
    while (1);
#endif
}
