/*********************************************************************************************
    *   Filename        : sdfile.h

    *   Description     : 本文件存放内置flash文件系统(sdfile)的数据结构声明和接口声明

    *   Author          : MoZhiYe

    *   Email           : mozhiye@zh-jieli.com

    *   Last modifiled  : 2022-03-25 13:50

    *   Copyright:(c)JIELI  2022-2030  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef __SDFILE_H__
#define __SDFILE_H__

#include "typedef.h"

//*********************************************************************************//
//                                SDFILE数据结构                                   //
//*********************************************************************************//

#define SDFILE_NAME_LEN 			16
//================= 文件头:
typedef struct sdfile_file_head {
    u16 head_crc;
    u16 data_crc;
    u32 addr;
    u32 len;
    u8 attr;
    u8 res;
    u16 index;
    char name[SDFILE_NAME_LEN];
} SDFILE_FILE_HEAD;


//================= 目录头:
struct sdfile_dir {
    u16 file_num;
    struct sdfile_file_head head;
};

//================= 错误表:
enum SDFILE_ERR_TABLE {
    SDFILE_DIR_NOT_EXIST = -0xFF,
    SDFILE_FILE_NOT_EXIST,
    SDFILE_MALLOC_ERR,
    SDFILE_VM_NOT_FIND,
    SDFILE_DATA_CRC_ERR,
    SDFILE_WRITE_AREA_NEED_ERASE_ERR,
    SDFILE_SUSS = 0,
    SDFILE_END,
};



#define SDFILE_PART_APP 		0x55
#define SDFILE_PART_RES 		0xAA

#define SDFILE_FILE_ATTR_UBOOT 		0x00		//uboot代码文件
#define SDFILE_FILE_ATTR_APP 		0x01		//app代码文件
#define SDFILE_FILE_ATTR_REG 		0x02		//普通文件
#define SDFILE_FILE_ATTR_DIR 		0x03		//目录文件
#define SDFILE_FILE_ATTR_RESERVED 	0x10		//预留文件
#define SDFILE_FILE_ATTR_CPMPRESS 	0x40		//压缩文件
#define SDFILE_FILE_ATTR_ENC 		0x80 		//加密文件

#define SDFILE_FILE_ATTR_TYPE_MASK	0x0F

#if (VFS_ENABLE == 1)

//*********************************************************************************//
//                               Have VFS In System                                //
//*********************************************************************************//
#if SDFILE_STORAGE

#define SDFILE_MAX_DEEPTH 		2

#define SDFILE_NAME_LEN 			16

struct sdfile_folder {
    u32 saddr;
    u32 addr;
    u32 len;
};

struct sdfile_scn {
    u8 subpath;
    u8 cycle_mode;
    u8 attr;
    u8 deepth;
    u16 dirCounter;
    u16 fileCounter;
    u16	fileNumber;			// 当前文件序号
    u16	totalFileNumber;
    u16	last_file_number;
    u16 fileTotalInDir;     // 当前目录的根下有效文件的个数
    u16 fileTotalOutDir;	// 当前目录前的文件总数，目录循环模式下，需要用它来计算文件序号
    u32 sclust_id;
    u32 root_addr;  //分区根地址
    u8  part_index;  //标记分区，0为res 1：app 2:ext_reserve
    u8  scan_dir_flag; //按目录扫描标志
    const char *ftypes;
    struct sdfile_file_head head;
    struct sdfile_folder folder[SDFILE_MAX_DEEPTH];
};
#endif /* #if SDFILE_STORAGE */

//================= 文件指针:
typedef struct sdfile_file_t {
    u32 fptr;
    struct sdfile_file_head head;
#if SDFILE_STORAGE
    struct sdfile_scn *pscn;
#endif
} SDFILE;

#else /* #if (VFS_ENABLE == 1) */

//*********************************************************************************//
//                               None VFS In System                                //
//================= 文件指针:
typedef struct sdfile_file_t {
    u32 fptr;
    struct sdfile_file_head head;
} SDFILE;
//*********************************************************************************//

#endif  /* #if (VFS_ENABLE == 1) */

// int sdfile_delete_data(SDFILE *fp);
// #define fdel_data   sdfile_delete_data

//*********************************************************************************//
//                               SDFILE路径声明                               	   //
//*********************************************************************************//
#ifndef SDFILE_MOUNT_PATH
#define SDFILE_MOUNT_PATH     	"flash"
#endif /* #ifndef SDFILE_MOUNT_PATH */

#ifndef SDFILE_APP_ROOT_PATH
#define SDFILE_APP_ROOT_PATH       	SDFILE_MOUNT_PATH"/app/"  //app分区
#endif /* #ifndef SDFILE_APP_ROOT_PATH */

#ifndef SDFILE_RES_ROOT_PATH
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/res/"  //资源文件分区
#endif /* #ifndef SDFILE_RES_ROOT_PATH */

//*********************************************************************************//
//                               SDFILE接口声明                               	   //
//*********************************************************************************//

//获取flash物理大小, 单位: Byte
u32 sdfile_get_disk_capacity(void);
//flash addr  -> cpu addr
u32 sdfile_flash_addr2cpu_addr(u32 offset);
//cpu addr  -> flash addr
u32 sdfile_cpu_addr2flash_addr(u32 offset);

void sdfile_head_addr_get(char *name, u32 *addr, u32 *len);

int sdfile_init(void);

u8 *sdfile_get_burn_code(u8 *len);

#endif /* #ifndef __SDFILE_H__ */

