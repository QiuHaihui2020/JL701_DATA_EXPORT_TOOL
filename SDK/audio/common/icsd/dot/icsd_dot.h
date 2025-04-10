#ifndef _ICSD_DOT_H
#define _ICSD_DOT_H

#include "generic/typedef.h"
#include "os/os_type.h"
#include "os/os_api.h"
#include "timer.h"
#include "math.h"
#include "icsd_common.h"

#if 1
#define _dot_printf printf               //打开智能免摘库打印信息
#else
extern void dot_printf_off(char *format, ...);
#define _dot_printf dot_printf_off
#endif/*log_en*/

#define DOT_ANC_DMA_SR_SEL       2//46.9k

#define FFTLEN   		1024

extern u16	DOT_FUNCTION_DEBUG;
#define DOT_DEBUG_DATA				BIT(0)
#define DOT_DEBUG_LINE				BIT(1)

struct icsd_dot_infmt {
    void *alloc_ptr;     //外部申请的ram地址
    u16	tone_delay;      //提示音延长多长时间开始获取sz数据
    void (*dot_dma_on)(u8 out_sel, int *buf, int len);    //算法结果输出
};

struct icsd_dot_libfmt {
    int lib_alloc_size;  //算法ram需求大小
    u8  alogm;           //anc采样率
};

struct icsd_dot_parm {
    u8 dot_len;
    u8 start_point;
    float norm_thr;
    float loose_thr;
};
extern struct icsd_dot_parm DOT_PARM;


extern int (*_printf)(char *format, ...);
extern int (*dot_printf)(const char *format, ...);


extern void icsd_dot_post_anc_task_cmd(u8 cmd);
// extern void anc_dma_on(u8 out_sel, int *buf, int irq_point);
// extern void anc_core_dma_stop(void);
//DOT调用的APP函数
//LIB调用的AEQ函数
extern void icsd_dot_output(int dot_db);
//DOT调用的LIB函数
extern void icsd_dot_start(u32 slience_frames, u16 sample_rate);
extern void icsd_dot_version();
//APP调用的LIB函数
extern void icsd_dot_get_libfmt(struct icsd_dot_libfmt *libfmt);
extern void icsd_dot_set_infmt(struct icsd_dot_infmt *fmt);
extern void icsd_dot_anctask_handle(u8 cmd);
extern void icsd_dot_anc_dma_done();

void icsd_dot_parm_init();
void icsd_dot_tone_start(u32 slience_frames, u16 sample_rate);
void icsd_dot_data_debug_en(u8 en);
void icsd_dot_line_debug_en(u8 en);

#endif
