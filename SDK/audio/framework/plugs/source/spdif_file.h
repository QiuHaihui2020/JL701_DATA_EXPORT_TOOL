#ifndef __SPDIF_FILE_H
#define __SPDIF_FILE_H



#define SPDIF_RX_CLK		    320 * 1000000UL
/* #define SPDIF_DEC_DATA_BUF_LEN  1024*4 */
// SPDIF 一个包是192个，设置为768则是4个包的数据
//16bit:x*2*2*2, 24bit:192*4*2*2
#define SPDIF_DATA_DMA_LEN     (192 * 2 * 2 * 2)
#define SPDIF_INF_DMA_LEN	   (192 * 2 * 2 / 2)
enum hdmi_det_mode_enum {
    HDMI_DET_UNUSED,
    HDMI_DET_BY_IO,
    HDMI_DET_BY_CEC,
};
enum audio_spdif_state_enum {
    AUDIO_SPDIF_STATE_CLOSE,
    AUDIO_SPDIF_STATE_INIT,
    AUDIO_SPDIF_STATE_START,
    AUDIO_SPDIF_STATE_STOP,
};
//stream.bin spdif参数结构
struct spdif_file_cfg {
    //HDMI ARC配置
    u16 hdmi_port[2];
    u8  hdmi_det_mode;
    u16 hdmi_det_port;
    u16 cec_io_port;
    u16 iic_scl_port;
    u16 iic_sda_port;
    u8  iic_num;
    //OPT/COAL
    u16 opt_port[2];
    u16 coal_port[2];

} __attribute__((packed));
//
void audio_spdif_file_init(void);
//获取spdif 节点配置参数
struct spdif_file_cfg *audio_spdif_file_get_cfg(void);

//获取spdif 节点配置的CEC IO
int get_hdmi_cec_io(void);
/* spdif_init
 * @description: 初始化spdif
 * @return：返回 struct spdif_file_hdl 结构体类型指针
 * @note: 无
 */
void *spdif_init(void);


/* spdif_release
 * @description: 释放掉spdif_init 申请的内存
 * @param: 参数传入 spdif_file_hdl 结构体指针类型的参数
 * @return：无
 * @note: 无
 */
void spdif_release(void *_hdl);


/* spdif_start
 * @description: 打开spdif，此时spdif 数据中断开始有数据输入
 * @return：返回0表示成功
 * @note:
 */
int spdif_start(void);


/* spdif_stop
 * @description: 关闭spdif，停止spdif 数据中断数据输入
 * @return：返回0表示成功
 * @note:
 */
int spdif_stop(void);


//提供给外部用于设置默认启动的spdif io 的接口,spdif_init前设置
void set_spdif_default_io(u8 port, u8 port_mode);
/* 提供给外部用于切换 spdif io 的接口
 * port:      比如 IO_PORTA_06
 * port_mode: 比如 SPDIF_IN_ANALOG_PORT_A 或者 SPDIF_IN_DIGITAL_PORT_A(详见 SPDIF_SLAVE_CH 枚举)
 */
void spdif_io_port_switch(u8 port, u8 port_mode);

/*循环切换配置文件里配置的输入IO*/
void spdif_io_loop_switch(void);


/* 获取当前输入源是否是HDMI */
int spdif_is_hdmi_source(void);

/* 如果当前为HDMI数据源时，需等CEC通信后数据稳定了，由外部控制是否往后面推数*/
void spdif_hdmi_set_push_data_en(u8 en);

/* 获取 spdif 输入源的IO */
u8 get_spdif_source_io(void);

/* 当前HDMI是否插入(HDMI检测脚检测), 500ms更新值 */
extern u8 hdmi_is_online(void);

/* 通过消息队列重启 spdif 数据流 */
extern int spdif_restart_by_taskq(void);

/* 通过消息队列重启 spdif 数据流 */
extern int spdif_open_player_by_taskq(void);

/* 获取 spdif 数据流是否处于打开状态 */
extern bool spdif_player_runing(void);

//spdif 切源的时候解mute
extern void spdif_switch_source_unmute();

//设置spdif source
extern void spdif_set_port_by_index(u8 index);

//获取当前spdif source
extern u8 spdif_get_cur_port_index();

#endif

