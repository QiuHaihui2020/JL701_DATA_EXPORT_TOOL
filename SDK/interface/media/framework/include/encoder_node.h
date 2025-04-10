#ifndef ENCODER_NODE_MGR_H
#define ENCODER_NODE_MGR_H

#include "jlstream.h"
#include "media/audio_base.h"

struct encoder_plug_ops {
    int coding_type;
    void *(*init)(void *priv);
    int (*run)(void *);
    int (*ioctl)(void *, int, int);
    void (*release)(void *);
};

struct encoder_fmt {
    u8 quality;
    u8 complexity;
    u8 sw_hw_option;
    u8 ch_num;
    u8 bit_width;
    u16 frame_dms;
    u32 bit_rate;
    u32 sample_rate;
};


enum change_file_step {
    SEAMLESS_OPEN_FILE,
    SEAMLESS_CHANGE_FILE,
};

/*
 * 无缝录音配置, 支持配置录制多长时间(秒)后切换文件
 * advance_time: 提前多少秒调用change_file(priv, SEAMLESS_OPEN_FILE), 用于提前创建新文件
 * time: 单个文件录音时长(秒)
 */
struct seamless_recording {
    u8 advance_time;
    u16 time;
    void *priv;
    /*
     * 此回调函数在音频流任务中调用,不能执行耗时长的操作,否则可能导致音频播放卡顿
     */
    int (*change_file)(void *priv, enum change_file_step step);
};

#define REGISTER_ENCODER_PLUG(plug) \
    const struct encoder_plug_ops plug sec(.encoder_plug)

int encoder_plug_output_data(void *_hdl, u8 *data, u16 len);

int encoder_plug_read_data(void *_hdl, u8 *data, u16 len);

struct stream_frame *encoder_plug_pull_frame(void *_hdl);

struct stream_frame *encoder_plug_get_output_frame(void *_hdl, u16);

void encoder_plug_put_output_frame(void *_hdl, struct stream_frame *);

void encoder_plug_free_frame(void *_hdl, struct stream_frame *);
int encoder_plug_get_bit_width(void *_hdl);

int encoder_plug_get_bit_width(void *_hdl);
#endif

