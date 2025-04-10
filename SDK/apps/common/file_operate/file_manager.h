#ifndef __FILE_MANAGER_H__
#define __FILE_MANAGER_H__

#include "system/includes.h"
#include "typedef.h"
#include "fs/fs.h"
#include "dev_manager.h"

#define FILE_MANAGER_SCAN_DISK_MAX_DEEPTH				9

struct __scan_callback {
    void (*enter)(struct __dev *dev);
    void (*exit)(struct __dev *dev);
    int (*scan_break)(void);
};

FILE *file_manager_select(struct __dev *dev, struct vfscan *fs, int sel_mode, int arg, struct __scan_callback *callback);
struct vfscan *file_manager_scan_disk(struct __dev *dev, const char *path, const char *parm, u8 cycle_mode, struct __scan_callback *callback);
#endif// __FILE_MANAGER_H__
