#ifndef __PC_RX_FILE_H
#define __PC_RX_FILE_H

#include "cpu.h"



u8 is_pc_spk_file_start(void);
u8 is_pc_spk_file_run(void);
u32 pc_spk_get_fmt_sample_rate(void);
void pc_spk_set_fmt(u8 channel, u8 bit, u32 sample_rate);

void pc_spk_data_isr_cb(void *buf, u32 len);	//spk 中断往数据流推数据函数
int pc_mode_broadcast_deal_by_taskq(int broadcast_music_status);

#endif

