#ifndef __EFFECTS_DEV__H
#define __EFFECTS_DEV__H

#include "system/includes.h"

struct packet_ctrl {
    void *node_hdl;
    s16 *remain_buf;
    u8 *pack_frame;
    struct stream_frame *out_frame;
    void (*effect_run)(void *, s16 *, s16 *, u32);
    u32 sample_rate;    //采样率
    u16 remain_len;     //记录算法消耗的剩余的数据长度
    u16 frame_len;
    u8 in_ch_num;       //输入声道数,单声道:1，立体声:2, 四声道:4
    u8 out_ch_num;      //输出声道数,单声道:1，立体声:2, 四声道:4
    u8 bit_width;       //位宽, 0:16bit  1:32bit
    u8 qval;            //q, 15:16bit, 23:24bit
};

void effect_dev_init(struct packet_ctrl *hdl, u32 process_points_per_ch);
void effect_dev_process(struct packet_ctrl *hdl, struct stream_iport *iport,  struct stream_note *note);
void effect_dev_close(struct packet_ctrl *hdl);

#endif
