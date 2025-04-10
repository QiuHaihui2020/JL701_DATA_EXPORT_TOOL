#ifndef _MEM_HEAP_H_
#define _MEM_HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "generic/typedef.h"

typedef enum {
    P_MEMORY_TOTAL,       // ram0
    P_MEMORY_UNUSED,      // ram0
    P_MEMORY_USED,        // ram0

    P_VLT_MEMORY_TOTAL,   // ram1
    P_VLT_MEMORY_UNUSED,  // ram1
    P_VLT_MEMORY_USED,    // ram1

    P_HEAP_SIZE,          // ram0 + ram1
} MEMORY_TYPE;

void memory_init(void);

void *malloc(size_t size);
void *zalloc(size_t size);
void free(void *mem);
void *realloc(void *ptr, size_t size);
void *calloc(unsigned long count, unsigned long size);

void mem_stats(void);
/* ---------------------------------------------------------------------------- */
/**
 * @brief :获取物理内存的大小
 *
 * @param type: 需要获取的内存类型;
 *
 * @return : 对应类型物理内存大小
 */
/* ---------------------------------------------------------------------------- */
size_t memory_get_size(MEMORY_TYPE type);

/* ---------------------------------------------------------------------------- */
/**
 * @brief :申请连续空间的物理内存
 *
 * @param size: 申请内存大小, 单位byte;
 *
 * @return : 物理内存地址
 */
/* ---------------------------------------------------------------------------- */
void *phy_malloc(size_t size);

/* ---------------------------------------------------------------------------- */
/**
 * @brief :释放连续空间的物理内存
 *
 * @param :物理内存地址
 */
/* ---------------------------------------------------------------------------- */
void phy_free(void *pv);


/* --------------------------------------------------------------------------*/
/**
 * @brief dma_malloc
 *
 * @param size: 申请内存大小, 单位byte;
 *
 * @return : dma内存地址
 */
/* ----------------------------------------------------------------------------*/
void *dma_malloc(size_t size);

/* ---------------------------------------------------------------------------- */
/**
 * @brief :释放连续空间的dma内存
 *
 * @param :dma内存地址
 */
/* ---------------------------------------------------------------------------- */
void dma_free(void *pv);

/* --------------------------------------------------------------------------*/
/**
 * @brief :检查mem是否物理内存地址
 *
 * @param mem :检查对象
 *
 * @return :true - 物理内存，false - 非物理内存
 */
/* ----------------------------------------------------------------------------*/
int memory_in_phy(const void *mem);

/* --------------------------------------------------------------------------*/
/**
 * @brief :检查mem是否虚拟内存地址
 *
 * @param mem :检查对象
 *
 * @return :true - 虚拟内存，false - 非虚拟内存
 */
/* ----------------------------------------------------------------------------*/
int memory_in_vtl(const void *mem);

/* --------------------------------------------------------------------------*/
/**
 * @brief :获取空闲可掉电内存的电源控制位
 *
 * @param mem :
 *
 * @return :返回值对应P11_SYSTEM->MEM_PWR_CON0寄存器的每一bit
 */
/* ----------------------------------------------------------------------------*/
u32 pmalloc_get_idle_ram(void);

#ifdef __cplusplus
}
#endif

#endif /* _MEM_HEAP_H_ */
