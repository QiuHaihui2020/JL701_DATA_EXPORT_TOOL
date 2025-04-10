/*********************************************************************************************
    *   Filename        : resfile.h

    *   Description     : 本文件存放访问系统资源文件的函数接口和数据结构声明

    *   Author          : MoZhiYe

    *   Email           : mozhiye@zh-jieli.com

    *   Last modifiled  : 2022-04-15 09:30

    *   Copyright:(c)JIELI  2022-2030  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef __RESFILE_H__
#define __RESFILE_H__

#include "typedef.h"


#define RESFILE_SEEK_SET	0	/* Seek from beginning of file.  */
#define RESFILE_SEEK_CUR	1	/* Seek from current position.  */
#define RESFILE_SEEK_END	2	/* Seek from end of file.  */

//内置flash根路径声明:
#define FLASH_ROOT			"flash"
#define FLASH_APP_PATH		FLASH_ROOT"/app/"
#define FLASH_RES_PATH		FLASH_ROOT"/res/"

//外置flash根路径声明:
//#define EXFLASH_ROOT		"exflash"
#if CONFIG_RESFILE_SUPPORT_MULT_DEVICE
typedef struct __resfile {
    void *ops;
    void *priv_data;
} RESFILE;
#else
typedef struct __resfile {
    int priv_data[0];
} RESFILE;
#endif

struct resfile_attrs {
    u8 attr;   	//文件属性
    u32 fsize; 	//文件长度
    u32 sclust; //文件地址
};


enum resfile_err_code {
    RESFILE_ERR_NONE = 0,
    RESFILE_ERR_PATH = -1,
    RESFILE_ERR_NO_MEM = -2,
    RESFILE_ERR_OPS_NO_SUPPORT = -3,
    RESFILE_ERR_HANDLE = -4,
};


/* ---------------------------------------------------------------------------- */
/**
 * @brief 资源文件打开接口
 *
 * @param path: 文件路径名, 如内置flash路径, FLASH_RES_PATH"file.txt"
 *
 * @return: 1) 非NULL, 文件打开成功, 返回文件句柄;
 			2) NULL, 文件打开失败
 */
/* ---------------------------------------------------------------------------- */
RESFILE *resfile_open(const char *path);

/* ---------------------------------------------------------------------------- */
/**
 * @brief 资源文件读接口
 *
 * @param fp: 文件句柄
 * @param buf: 用于读取数据的buf
 * @param len: 读取数据长度
 *
 * @return: 1) >= 0: 实际成功读取的长度
  			2) < 0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_read(RESFILE *fp, void *buf, u32 len);

/* ---------------------------------------------------------------------------- */
/**
 * @brief 特殊资源文件写接口, 该接口一般只用来写由用户分配的自定义
 		预留区域的文件, 请误用来写只读的资源文件.
 *
 * @param fp: 文件句柄
 * @param buf: 用于写入数据的buf
 * @param len: 写入数据长度
 *
 * @return: 1) >= 0: 实际成功写入的长度
  			2) < 0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_write(RESFILE *fp, void *buf, u32 len);


/* ---------------------------------------------------------------------------- */
/**
 * @brief 设置资源文件读写指针偏移
 *
 * @param fp: 文件句柄
 * @param offset: 偏移量
 * @param fromwhere: 偏移方式:
 						1)RESFILE_SEEK_SET: 从文件开头偏移;
 						2)RESFILE_SEEK_CUR: 从文件当前读写指针偏移;
 						3)RESFILE_SEEK_END: 从文件结束偏移;
 *
 * @return: 1) >=0: 当前文件读写指针位置
  			2) < 0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_seek(RESFILE *fp, u32 offset, u32 fromwhere);


/* ---------------------------------------------------------------------------- */
/**
 * @brief 获取资源文件名称
 *
 * @param fp: 文件句柄
 * @param name: 获取获取文件名的buf
 * @param len: buf长度
 *
 * @return: 1) >=0: 获取成功
  			2) < 0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_get_name(RESFILE *fp, void *name, u32 len);



/* ---------------------------------------------------------------------------- */
/**
 * @brief 获取资源文件长度
 *
 * @param fp: 文件句柄
 *
 * @return: 1) >=0: 当前文件实际长度
   			2)返回<0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_get_len(RESFILE *fp);



/* ---------------------------------------------------------------------------- */
/**
 * @brief 获取当前资源文件读写指针位置
 *
 * @param fp: 文件句柄
 *
 * @return: 1) >=0: 当前文件读写指针位置
  			2) < 0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_get_pos(RESFILE *fp);


/* ---------------------------------------------------------------------------- */
/**
 * @brief 获取当前资源文件的属性信息
 *
 * @param fp: 文件句柄
 * @param attrs: 用于获取属性信息的buf
 *
 * @return : 1)返回0: 获取成功
   			 2)返回<0: 错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_get_attrs(RESFILE *fp, struct resfile_attrs *attrs);


/* ---------------------------------------------------------------------------- */
/**
 * @brief  关闭当前资源文件
 *
 * @param fp: 文件句柄
 *
 * @return : 1)返回0: 成功关闭
   			 2)返回<0: 关闭错误, 请查看本文件enum resfile_err_code
 */
/* ---------------------------------------------------------------------------- */
int resfile_close(RESFILE *fp);

#endif /* #ifndef __RESFILE_H__ */
