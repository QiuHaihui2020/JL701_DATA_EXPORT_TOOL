//*********************************************************************************//
// Module name : cache.h                                                           //
// Description : q32DSP cache control head file                                    //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __Q32DSP_CACHE__
#define __Q32DSP_CACHE__

//#include "generic/typedef.h"
#if 0
//------------------------------------------------------//
// icache function
//------------------------------------------------------//

void IcuEnable(void);
void IcuDisable(void);
void IcuInitial(void);

//void IcuFlushAll(void);
void IcuFlushinvAll(void);
//void IcuFlushRegion(int *beg, int len);                // note len!=0
void IcuFlushinvRegion(int *beg, int len);             // note len!=0

void IcuUnlockAll(void);
void IcuUnlockRegion(int *beg, int len);               // note len!=0
void IcuPfetchRegion(int *beg, int len);               // note len!=0
void IcuLockRegion(int *beg, int len);                 // note len!=0

void IcuReportEnable(void);
void IcuReportDisable(void);
void IcuReportPrintf(void);
void IcuReportClear(void);

void IcuEmuEnable(void);
void IcuEmuDisable(void);
void IcuEmuMessage(void);

#define WAIT_ICACHE_IDLE    do{asm volatile("csync"); \
                                 while(!(q32DSP_icu(core_num())->CON & BIT(31)));}  while(0);

//------------------------------------------------------//
// dcache function
//------------------------------------------------------//
void DcuEnable(void);
void DcuDisable(void);
void DcuInitial(void);

//void DcuFlushAll(void);
void DcuFlushinvAll(void);
//void DcuFlushRegion(int *beg, int len);                // note len!=0
void DcuFlushinvRegion(int *beg, int len);             // note len!=0

void DcuUnlockAll(void);
void DcuUnlockRegion(int *beg, int len);               // note len!=0
void DcuPfetchRegion(int *beg, int len);               // note len!=0
void DcuLockRegion(int *beg, int len);                 // note len!=0

void DcuReportEnable(void);
void DcuReportDisable(void);
void DcuReportPrintf(void);
void DcuReportClear(void);

void DcuEmuEnable(void);
void DcuEmuDisable(void);
void DcuEmuMessage(void);

#define WAIT_DCACHE_IDLE    do{asm volatile("csync"); while(!(JL_DCU->CON & BIT(31)));} while(0);
#endif

typedef struct __cache_info {
    unsigned int cache_type;		// 0:icache; 1:dcache
    unsigned int cpu_id;
    unsigned int efficiency;
} CACHE_INFO;

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取cache信息
 *
 * @param cache_info_list
 * @param cache_info_list_len
 */
/* ----------------------------------------------------------------------------*/
void cache_info_list_get(CACHE_INFO *cache_info_list, unsigned int cache_info_list_len);

void flush_dcache(void *ptr, int len);
void flushinv_dcache(void *ptr, int len);
void IcuEnable(void);
void DcuEnable(void);
void IcuWaitIdle(void);
void DcuWaitIdle(void);
void IcuDisable(void);
void DcuDisable(void);
void IcuFlushinvAll(void);
void IcuUnlockAll(void);
void IcuFlushinvRegion(int *beg, int len);
void IcuUnlockRegion(int *beg, int len);
void IcuLockRegion(int *beg, int len);
void IcuPfetchRegion(unsigned int *beg, unsigned int len);
void DcuFlushinvAll(void);
void DcuFlushinvRegion(int *beg, int len);
void DcuPfetchRegion(unsigned int *beg, unsigned int len);
void IcuInitial(void);
void DcuInitial(void);

#define WAIT_DCACHE_IDLE    do {DcuWaitIdle();} while(0);
#define WAIT_ICACHE_IDLE    do {IcuWaitIdle();} while(0);
//#define WAIT_DCACHE_IDLE    do{asm volatile("csync"); while(!(JL_DCU->CON & BIT(31)));} while(0);

#endif /* #ifndef __Q32DSP_CACHE__ */
//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
