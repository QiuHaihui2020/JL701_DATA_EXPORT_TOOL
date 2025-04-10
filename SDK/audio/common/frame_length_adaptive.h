#ifndef _FRAME_LENGTH_ADAPTIVE_H_
#define _FRAME_LENGTH_ADAPTIVE_H_

#include "system/includes.h"
#include "circular_buf.h"

/* 当输入数据大小和需要处理的帧长不一致时，
 * 使用该接口可以按照设定的帧长处理数据输出
 */
struct fixed_frame_len_handle {
    u8 start;
    int frame_len;
    u8 *process_buf;
    void *priv;
    int (*output_handle)(void *priv, u8 *in, u8 *out, int len);
    int remain_len;
};

/*设置每次处理的帧长大小 和 处理回调*/
struct fixed_frame_len_handle *audio_fixed_frame_len_init(int frame_len, int (*output_handle)(void *priv, u8 *in, u8 *out, int len), void *priv);
/*根据输入数据长度，获取需要输出数据大小*/
int get_fixed_frame_len_output_len(struct fixed_frame_len_handle *hdl, int in_len);
/*按照设置的帧长处理数据输出*/
int audio_fixed_frame_len_run(struct fixed_frame_len_handle *hdl, u8 *in, u8 *out, int len);
void audio_fixed_frame_len_exit(struct fixed_frame_len_handle *hdl);


struct frame_length_adaptive_hdl {

    u16 adj_len;
    u8 *sw_buf;  //长度转换buf
    cbuffer_t sw_cbuf;
};

/* adj_len : 需要输出的数据长度
 * in_len : 输入的数据长度
 * */
struct frame_length_adaptive_hdl *frame_length_adaptive_open(int adj_len, int in_len);

int frame_length_adaptive_run(void *_hdl, void *in_data, void *out_data, u16 in_len);

void frame_length_adaptive_close(void *_hdl);


#endif/*FRAME_LENGTH_ADAPTIVE_H_*/

