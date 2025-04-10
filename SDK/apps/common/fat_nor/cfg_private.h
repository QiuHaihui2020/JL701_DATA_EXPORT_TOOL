#ifndef _CFG_PRIVATE_H
#define _CFG_PRIVATE_H

#include "fs/resfile.h"

typedef enum {
    CFG_PRIVATE_OK = 0, // 0, 成功返回值
    CFG_PRIVATE_INIT_NO_PART, // 1, 初始化找不到此预留区域
    CFG_PRIVATE_INIT_ADDR_NO_ALIGN, // 2, 初始化发现地址不对齐
    CFG_PRIVATE_INIT_FILE_NUM_OVER, // 3, 初始化发现文件数量超过最大限制(4K 对齐：16个， 256对齐：8个)
    CFG_PRIVATE_INIT_MALLOC_ERR, // 4, 初始化申请内失败
    CFG_PRIVATE_UNKNOWN
} CFG_PRIVATE_R;

int cfg_private_init(int file_num, const char *part_path);
int cfg_private_uninit();
int cfg_private_erase_file(RESFILE *file);
int cfg_private_write(RESFILE *file, void *buf, u32 len);
int cfg_private_read(RESFILE *file, void *buf, u32 len);
int cfg_private_close(RESFILE *file);
int cfg_private_seek(RESFILE *file, int offset, int fromwhere);
void cfg_private_erase(u32 addr, u32 len);
RESFILE *cfg_private_open_by_maxsize(const char *path, const char *mode, int file_maxsize);

#endif
