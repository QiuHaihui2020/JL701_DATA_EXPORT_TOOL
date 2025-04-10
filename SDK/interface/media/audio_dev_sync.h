#ifndef _AUDIO_DEV_SYNC_H
#define _AUDIO_DEV_SYNC_H

#include "media/audio_base.h"
#include "sync/audio_syncts.h"
#include "effects/effects_adj.h"
#include "media/audio_general.h"
#include "asm/audio_src.h"


struct dev_sync_params {
    u32 in_sample_rate;
    int out_sample_rate;
    u8  channel;
    u8 bit_width;
    void *priv;
    int (*handle)(void *priv, void *data, int len);
};
struct sample_rate_and_probabilty {
    int sample_rate;
    u8 probabilty;
};

#define DEV_SYNC_MATCHED_OUTSR_NUM                       10
struct dev_sync {
    struct audio_src_handle hw_src;
    struct dev_sync_params params;
    void *priv;
    int (*handler)(void *, void *, int);
    struct sample_rate_and_probabilty outsr_store_table[DEV_SYNC_MATCHED_OUTSR_NUM];
    u32 run_timeout;
    u32 dev_input_frames;
    u32 start_time;
    u32 start_frames;
    u32 next_latch_period;
    u32 real_out_sample_rate;
    u32 offset;
    u8 match_outsr_count;
    u8 start;
    u8 dev_buf_full;

};

struct sync_rate_params {
    char *name;//for debug
    u32 buffered_frames;
    u32 current_time;
    u32 timestamp;
    s16 d_sample_rate;

};

void *dev_sync_open(struct dev_sync_params *params);
int dev_sync_write(void *_hdl, void *data, int len);
int dev_sync_bufferd_frames(void *_hdl);
int dev_sync_calculate_sample_rate(void *_hdl, struct stream_frame *frame);//追踪实际采样率
int dev_sync_calculate_output_samplerate(void *_hdl, int frames, int buffered_frames);
void dev_sync_update_rate(void *_hdl, struct sync_rate_params *params);
void dev_sync_close(void *_hdl);

/*
 *网络时钟对应的当前微秒时间
 * */
u32 dev_sync_latch_time(u32 reference_network);

#define SELF_DELAY     0
#define MASTER_DELAY   1
#define DELAY_PROVIDER 2

#define NORMAL_DEV  0
#define MASTER_DEV  1
#define SYNC_TO_MASTER_DEV  2
extern const int config_dev_sync_enable;
#endif
