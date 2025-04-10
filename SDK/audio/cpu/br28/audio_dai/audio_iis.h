#ifndef _AUDIO_IIS_H_
#define _AUDIO_IIS_H_

#include "audio_link.h"
#include "media/audio_cfifo.h"

struct alink_param {
    u8 module_idx;         // 0代表模块0, 1代表模块1
    u8 bit_width;          //位宽配置 0：16bit 1:24bit
    u32 dma_size;          //决定iis buf大小，dma_len / 2 的值为中断起一次的长度len的值
    u32 sr;                // 采样率配置
    float fixed_pns;            // 固定pns,单位ms
};

struct ch_config {
    u8 len;//分组的长度
    u8  en;	//0:关闭 1：使能
    u8  dir;//0输入：1：输出
    u16 data_io_uuid;//io配置
} __attribute__((packed));

/* 保存配置文件信息的结构体 */
struct iis_file_cfg {
    u8 module_idx; 	// 0代表模块0, 1代表模块1
    u8 role;			// 0代表主机，1代表从机
    u16 io_mclk_uuid;	//mclk io 的 uuid
    u16 io_sclk_uuid;
    u16 io_lrclk_uuid;
    struct ch_config hwcfg[4];//通道0~3的配置
} __attribute__((packed));


struct iis_io_cfg {
    u8 mclk;
    u8 sclk;
    u8 lrclk;
    u8 data_io[4];
};

struct _iis_hdl {
    ALINK_PARM alink_parm;
    struct alnk_hw_ch ch_cfg[4];
    struct iis_file_cfg cfg;
    struct iis_io_cfg io_cfg;
    u8 *iis_buf;
    u8 start;   //硬件模块的state
    u8 module_idx;
    u32 protect_pns;
    void *hw_alink;
    void *hw_alink_ch[4];
    struct list_head tx_irq_list[4];
    struct list_head rx_irq_list[4];

    /* -------------------------------iis tx 接入audio cfifo----------------------------------- */
    u8  state[4];              /*硬件通道的状态*/
    OS_MUTEX mutex[4];
    struct list_head sync_list[4];
    u8 channel;
    u8 fifo_state[4];
    int unread_samples[4];             /*未读样点个数*/
    struct audio_cfifo cfifo[4];        /*DAC cfifo结构管理*/
    float fixed_pns;            // 固定pns,单位ms
};
struct audio_iis_rx_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, void *, int);
    u8 ch_idx;
    u8 enable;
};
/*
 * audio_iis_init : 	申请iis需要的资源和初始化iis配置
 * param: module_idx:   br27 有两个ALINK模块，module可传入值为0 或 1
 *        sr:           采样率
 *        bit_width:    位宽配置 0：16bit 1:24bit
 *        dma_size:     决定iis buf大小，dma_len / 2 的值为中断起一次的长度len的值
 */
void *audio_iis_init(struct alink_param params);

/*
 *iis 硬件初始化，通道配
 *ch_idx:配置与蓝牙同步关联的目标通道,不需要关联时直接配0xff
 * */
void audio_iis_open(void *_hdl, u8 ch_idx);

/*
 *iis硬件启动
 * */
int audio_iis_start(void *_hdl);
/*
 *iis 通道关闭
 * */
int audio_iis_stop(void *_hdl, u8 ch_idx);
/*
 *iis硬件关闭
 * */
int audio_iis_close(void *_hdl);
/*
 *iis资源释放
 * */
int audio_iis_uninit(void *_hdl);

/* 提供给 rx 中断调用的函数, len: rx中断接收到的数据量，单位byte */
void audio_iis_set_shn(void *_hdl, u8 ch_idx, int len);

/*
 * 设置rx 起中断的点数
 */
void audio_iis_set_rx_irq_points(void *_hdl, int irq_points);

/*
 *通道是否初始化
 * */
int audio_iis_ch_alive(void *_hdl, u8 ch_idx);

/*
 *硬件是否启动
 * */
u8 audio_iis_is_working(void *_hdl);
/*
 *采样率获取
 * */
int audio_iis_get_sample_rate(void *_hdl);

/*
 *设置目标通道的中断回调函数
 * */
void audio_iis_set_irq_handler(void *_hdl, u8 ch_idx, void *priv, void (*handle)(void *priv, void *addr, int len));

struct audio_iis_sync_node {
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};


struct audio_iis_channel_attr {
    u16 ch_idx;                /*IIS通道*/
    u16 delay_time;            /*IIS通道延时*/
    u16 protect_time;          /*IIS延时保护时间*/
    u16 write_mode;            /*IIS写入模式*/
    u8 dev_properties;         /*关联蓝牙同步的主设备使能，只能有一个节点是主设备*/
} __attribute__((packed));

struct audio_iis_channel {
    struct list_head entry;
    u8 state;                              /*IIS状态*/
    u8 fade_out;
    struct audio_iis_channel_attr attr;     /*IIS通道属性*/
    void *iis;                              /*IIS设备*/
    struct audio_cfifo_channel fifo;        /*IIS cfifo通道管理*/
};
struct multi_ch {
    struct audio_iis_channel *ch[4];
};

int audio_iis_new_channel(void *_hdl, void *private_data);
void audio_iis_channel_start(void *iis_ch);
int audio_iis_channel_close(void *iis_ch);
int audio_iis_channel_write(void *iis_ch, void *buf, int len);
int audio_iis_fill_slience_frames(void *iis_ch,  int frames);
void audio_iis_channel_set_attr(void *iis_ch, void *attr);
int audio_iis_data_len(void *iis_ch);
void audio_iis_force_use_syncts_frames(void *iis_ch, int frames, u32 timestamp);
void audio_iis_syncts_trigger_with_timestamp(void *iis_ch, u32 timestamp);
int audio_iis_add_syncts_with_timestamp(void *iis_ch, void *syncts, u32 timestamp);
void audio_iis_remove_syncts_handle(void *iis_ch, void *syncts);
int audio_iis_link_to_syncts_check(void *_hdl, void *syncts);
int audio_iis_channel_latch_time(struct audio_iis_channel *ch, u32 *latch_time, u32(*get_time)(u32 reference_network), u32 reference_network);
int audio_iis_get_buffered_frames(void *hdl, u8 ch_idx);
int audio_iis_get_hwptr(void *_hdl, u8 ch_idx);
int audio_iis_get_swptr(void *_hdl, u8 ch_idx);
u32 audio_iis_fix_dma_len(u32 module_idx, u32 tx_dma_buf_time_ms, u16 rx_irq_points, u8 bit_width, u8 ch_num);
void audio_iis_add_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *output);
void audio_iis_del_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *output);
void audio_iis_add_multi_channel_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *_output);
void audio_iis_del_multi_channel_rx_output_handler(struct _iis_hdl *hdl, struct audio_iis_rx_output_hdl *_output);
void audio_iis_multi_channel_start(struct multi_ch *mch);
int audio_iis_multi_channel_write(struct multi_ch *ch, int *data, int *len, int *wlen);
int audio_iis_multi_channel_fill_slience_frames(struct multi_ch *ch, int frames, int *wlen);
int audio_iis_check_hw_cfg_status(u8 module_idx, u8 ch_idx, u8 tar_dir);
extern struct _iis_hdl *iis_hdl[2];
#endif

