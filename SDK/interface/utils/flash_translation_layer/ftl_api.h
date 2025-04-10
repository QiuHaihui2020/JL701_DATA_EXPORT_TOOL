#ifndef _FTL_API_H
#define _FTL_API_H

#include "generic/typedef.h"


enum ftl_error_t {
    FTL_ERR_NO          = 0x00,
    FTL_ERR_1BIT_ECC    = 0x01,
    FTL_ERR_READ        = 0x02,
    FTL_ERR_WRITE       = 0x04,
    FTL_ERR_NO_MEM      = 0x08,
    FTL_ERR_NO_BLOCK    = 0x10,
    FTL_ERR_INVALID     = 0x20,
};

struct ftl_nand_flash {
    u8 block_size_shift;
    u8 page_size_shift;
    u8 page_num;            //每个block的page数量
    u16 block_begin;        //需要管理的起始block
    u16 block_end;          //需要管理的结束block(不包含)
    u16 logic_block_num;    //虚拟出的逻辑block数量, 需要预留一定比例的坏块,
    //要比物理block数量少
    u16 page_size;          //page大小(byte)
    u16 oob_offset;         //可以使用的oob地址偏移(相对于oob起始地址)
    u16 oob_size;           //oob区域大小(byte)
    u32 max_erase_cnt;      //block的最大擦除次数
    enum ftl_error_t (*page_read)(u16 block, u8 page, u16 offset, u8 *buf, int len);
    enum ftl_error_t (*page_write)(u16 block, u8 page, u8 *buf, int len);
    enum ftl_error_t (*erase_block)(u32 addr);
};

struct ftl_config {
    u8 page_buf_num;            //page缓存数量,最少1个,影响读取和写入速度
    u8 delayed_write_msec;      //写入数据没满一个page时,延时写入flash的时间,
    //0表示每次都直接写入
};

int ftl_init(struct ftl_nand_flash *flash, struct ftl_config *config);


int ftl_read_bytes(u32 addr, u8 *buf, u16 len, enum ftl_error_t *error);


int ftl_write_bytes(u32 addr, u8 *buf, u16 len, enum ftl_error_t *error);

/*
 * 返回flash block size, 单位KByte
 */
int ftl_get_block_size();

/*
 * 返回flash容量, 单位KByte
 */
int ftl_get_capacity();


/*
 * 擦除逻辑地址, 地址要按block大小对齐
 */
int ftl_erase_lgc_addr(u32 addr);


int ftl_format();


/*
 * 释放掉ftl管理申请的memory和任务
 */
void ftl_uninit();





#endif
