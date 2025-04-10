#ifndef DECODER_NODE_MGR_H
#define DECODER_NODE_MGR_H

#include "jlstream.h"
#include "media/audio_base.h"
#include "media/audio_splicing.h"
#include "system/task.h"
#include "effects/effects_adj.h"

struct decoder_hdl;

struct decoder_file_ops {
    void *file;
    int (*fread)(void *file, u8 *buf, int len);
    int (*fseek)(void *file, u32 fpos);
};

struct decoder_get_fmt {
    struct decoder_file_ops fops;
    struct stream_fmt *fmt;
};


struct decoder_plug_ops {
    int coding_type;
    void *(*init)(struct decoder_hdl *dec_hdl);
    int (*run)(void *);
    int (*ioctl)(void *, int, int);
    void (*release)(void *);
};
struct convert_bit_wide {
    u8 *buf;//解码器位宽转换buf
    u16 len;
    u16 offset;
};

struct decoder_flow_ctrl {
    u16 max_sleep;
    u16 frame_min_len;
    u16 frame_max_len;
    struct list_head frames;
    struct stream_frame *frame;
};

struct decoder_dec_task {
    u8 frame_cnt;
    struct stream_thread *wakup_thread;
    OS_SEM dec_sem;
    const char *task_name;
    struct list_head file_frames;
};

struct decoder_hdl {
    void *decoder;
    int (*run)(void *);
    enum stream_scene scene;
    u8 start;
    u8 output_frame_cnt;
    u8 timestamp_flag;
    u8 frame_time;
    u8 pause;
    u8 dec_end;
    u8 no_data;
    u8 stop_dec;
    u8 channel_mode;
    u8 decoder_out_bit_wide;//0:16bit, 1:24bit 解码器实际输出位宽,如与decoder_node的oport_data_wide不一致时，decoder_node需做位宽转换
    u16 coding_type;

    u32 cur_time;
    u32 timestamp;
    u32 file_len;  //解码文件的长度
    u32 fpos;
    struct decoder_file_ops fops;

    OS_MUTEX mutex;
    OS_MUTEX file_mutex;
    OS_SEM file_sem;
    struct jlstream_fade fade;
    struct audio_dec_breakpoint *breakpoint;
    struct decoder_flow_ctrl *flow_ctrl;
    struct decoder_dec_task *task;
    const struct decoder_plug_ops *plug;

    struct node_port_data_wide data_wide;
    struct convert_bit_wide convert;
};

#define REGISTER_DECODER_PLUG(plug) \
    const struct decoder_plug_ops plug sec(.decoder_plug)

int decoder_plug_output_data(void *_hdl, u8 *data, u16 len, u8 channel_mode, void *priv);

int decoder_plug_read_data(void *_hdl, u32 fpos, u8 *data, u16 len);

int decoder_plug_get_data_len(void *_hdl);

struct stream_frame *decoder_plug_pull_frame(void *_hdl);

void decoder_plug_free_frame(void *_hdl, struct stream_frame *frame);

#endif

