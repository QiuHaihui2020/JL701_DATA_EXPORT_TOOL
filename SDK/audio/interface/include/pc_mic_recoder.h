#ifndef __PC_MIC_RECODER
#define __PC_MIC_RECODER

#include "cpu.h"

int pc_mic_recoder_open(void);
void pc_mic_recoder_close(void);
int pc_mic_recoder_open_by_taskq(void);
int pc_mic_recoder_close_by_taskq(void);
int pc_mic_recoder_restart_by_taskq(void);
int pc_mic_set_volume_by_taskq(u32 mic_vol);
extern u8 pc_mic_get_node_state(void);
extern void pc_mic_set_fmt(u8 channel, u8 bit, u32 sample_rate);
extern u32 pc_mic_get_fmt_sample_rate(void);
bool pc_mic_recoder_runing();
void pcm_mic_recoder_dump(int force_dump);
#endif


