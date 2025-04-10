#ifndef _AUDIO_CLK_SYNC_H_
#define _AUDIO_CLK_SYNC_H_

#include "typedef.h"

typedef struct {
    u32 timestamp;				// 收到数据的时间戳
    u32 deal_timestamp;			// 处理数据的时间戳
    u32 dev_cache_samples;      // 输出设备的缓存
    int sw_cache_samples;       // 后级软件模块缓存, 当该值为负数说明DAC拿到的数据比同步节点的多, 当同步没有作用时debug可以打印出这个值。如果同步节点正常工作而该值为负，那么说明有其它数据流没有经过同步节点往dac写数
    u32 sample_rate;			// 数据的采样率
    u8  *data;					// 数据的地址
    u32 len;					// 数据的总字节长度
    u8  bit_wide;				// 数据位宽
    u8  channel_num;			// 数据通道数
} audio_clk_sync_frame;

typedef struct {
    u32 input_sample_rate;			// 模块输入采样率
    u32 output_sample_rate;			// 模块输出采样率
    u32 delay_ms;					// 通路延时设置
    u32 channel_num;				// 数据通道数
    u32 cal_cycle_time_us;          // 采样率调整周期
    void (*output_handler)(void *, audio_clk_sync_frame *);	// 运算后的数据回调
    void *output_handler_priv;                              // 回调函数的传参
    u8  bit_wide;					// 数据位宽
    u8  block_type;					// 阻塞模式
} audio_clk_sync_param;

extern void *audio_clk_sync_open(audio_clk_sync_param *param);
extern int  audio_clk_sync_close(void *_hdl);
extern int  audio_clk_sync_run(void *_hdl, audio_clk_sync_frame *frame);
extern int  audio_clk_sync_update_output_points(void *_hdl, u32 points);
extern int  audio_clk_sync_update_dev_output_points(u32 timestamp, u32 points);
extern int  audio_clk_sync_update_points_pend(void);
extern int  audio_clk_sync_update_points_post(void);
extern int  audio_clk_sync_sw_cache_samples_get(void *_hdl);
extern u32  core_tick_time_get_us(void);
extern u8   audio_clk_sync_get_hw_src_id(void *_hdl);
#endif
