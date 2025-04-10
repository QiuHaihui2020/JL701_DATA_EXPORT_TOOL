#ifndef JLSTREAM_H
#define JLSTREAM_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "node_uuid.h"
#include "media/audio_base.h"
#include "media/audio_def.h"
#include "media/media_config.h"
#include "system/task.h"
#include "system/spinlock.h"
#include "fs/resfile.h"

#define FADE_GAIN_MAX	16384

#define __STREAM_CACHE_CODE     __attribute__((section(".jlstream.text.cache.L2")))
#define __NODE_CACHE_CODE(name)     __attribute__((section(".node."#name".text.cache.L2")))

#define STREAM_NODE_RUN_TIMER_DEBUG_EN 0

typedef void *jlstream_breaker_t;

struct stream_oport;
struct stream_node;
struct stream_snode;
struct stream_note;
struct jlstream;

#ifndef SEEK_SET
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */
#endif


#define NODE_IOC_OPEN_IPORT         0x00010000
#define NODE_IOC_CLOSE_IPORT        0x00010001
#define NODE_IOC_OPEN_OPORT         0x00010002
#define NODE_IOC_CLOSE_OPORT        0x00010003

#define NODE_IOC_SET_FILE           0x00020000
#define NODE_IOC_OPEN_FILE          0x00020001
#define NODE_IOC_GET_FMT            0x00020002
#define NODE_IOC_SET_FMT            0x00020003
#define NODE_IOC_CLR_FMT            0x00020004
#define NODE_IOC_GET_DELAY          0x00020005      // 获取缓存数据的延时
#define NODE_IOC_SET_SCENE          0x00020006      // 设置数据流的场景
#define NODE_IOC_SET_CHANNEL        0x00020007
#define NODE_IOC_NEGOTIATE          0x00020008      // 各个节点参数协商
#define NODE_IOC_FLUSH_OUT          0x00020009
#define NODE_IOC_SET_TIME_STAMP     0x0002000a
#define NODE_IOC_SYNCTS             0x0002000b
#define NODE_IOC_NODE_CONFIG        0x0002000c
#define NODE_IOC_SET_PRIV_FMT 		0x0002000d		//设置节点私有参数
#define NODE_IOC_NAME_MATCH         0x0002000e
#define NODE_IOC_SET_PARAM          0x0002000f
#define NODE_IOC_GET_PARAM          0x00020010
#define NODE_IOC_DECODER_PP		    0x00020011		//解码暂停再重开
#define NODE_IOC_DECODER_FF	    	0x00020012		//快进
#define NODE_IOC_DECODER_FR    		0x00020013		//快退
#define NODE_IOC_DECODER_REPEAT     0x00020014      //无缝循环播放
#define NODE_IOC_DECODER_DEST_PLAY  0x00020015      //跳到指定位置播放
#define NODE_IOC_GET_CUR_TIME  		0x00020016      //获取音乐播放当前时间
#define NODE_IOC_GET_TOTAL_TIME  	0x00020017      //获取音乐播放总时间
#define NODE_IOC_GET_BP  			0x00020018      //获取解码断点信息
#define NODE_IOC_SET_BP         	0x00020019      //设置解码断点信息
#define NODE_IOC_SET_FILE_LEN       0x0002001a      //设置解码文件长度信息
#define NODE_IOC_SET_BP_A           0x0002001b      //设置复读A点
#define NODE_IOC_SET_BP_B           0x0002001c      //设置复读B点
#define NODE_IOC_SET_AB_REPEAT      0x0002001d      //设置AB点复读模式
#define NODE_IOC_GET_ODEV_CACHE     0x0002001e      //获取输出设备的缓存采样数
#define NODE_IOC_SET_BTADDR         0x0002001f
#define NODE_IOC_SET_ENC_FMT        0x00020020
#define NODE_IOC_GET_ENC_FMT        0x00020021
#define NODE_IOC_GET_HEAD_INFO      0x00020022      //获取编码的头文件信息
#define NODE_IOC_SET_TASK           0x00020023
#define NODE_IOC_FSEEK              0x00020024
#define NODE_IOC_SET_BIT_WIDE       0x00020025
#define NODE_IOC_FLOW_CTRL_ENABLE   0x00020026
#define NODE_IOC_GET_BTADDR         0x00020027
#define NODE_IOC_GET_FRAME_SIZE     0x00020028
#define NODE_IOC_SET_FRAME_SIZE     0x00020029
#define NODE_IOC_SET_TS_PARM        0x0002002a
#define NODE_IOC_SET_SLEEP          0x0002002b
#define NODE_IOC_SET_SEAMLESS       0x0002002c
#define NODE_IOC_ENC_RESET          0x0002002d
#define NODE_IOC_FORCE_DUMP_PACKET  0x0002002e
#define NODE_IOC_GET_MIXER_INFO     0x0002002f
#define NODE_IOC_TWS_TX_SWITCH      0x00020030
#define NODE_IOC_GET_ID3      		0x00020031

#define NODE_IOC_START              (0x00040000 | NODE_STA_RUN)
#define NODE_IOC_PAUSE              (0x00040000 | NODE_STA_PAUSE)
#define NODE_IOC_STOP               (0x00040000 | NODE_STA_STOP)
#define NODE_IOC_SUSPEND            (0x00040000 | NODE_STA_SUSPEND)

#define TIMESTAMP_US_DENOMINATOR    32

enum stream_event {
    STREAM_EVENT_NONE,
    STREAM_EVENT_INIT,
    STREAM_EVENT_LOAD_DECODER,
    STREAM_EVENT_LOAD_ENCODER,
    STREAM_EVENT_UNLOAD_DECODER,
    STREAM_EVENT_UNLOAD_ENCODER,

    STREAM_EVENT_SUSPEND,
    STREAM_EVENT_READY,
    STREAM_EVENT_START,
    STREAM_EVENT_PAUSE,
    STREAM_EVENT_STOP,

    STREAM_EVENT_CLOSE_PLAYER,
    STREAM_EVENT_CLOSE_RECODER,
    STREAM_EVENT_PREEMPTED,
    STREAM_EVENT_NEGOTIATE_FAILD,
    STREAM_EVENT_GET_PIPELINE_UUID,

    STREAM_EVENT_GET_COEXIST_POLICY,
    STREAM_EVENT_INC_SYS_CLOCK,

    STREAM_EVENT_GET_NODE_PARM,
    STREAM_EVENT_GET_EFF_ONLINE_PARM,

    STREAM_EVENT_A2DP_ENERGY,

    STREAM_EVENT_GET_SWITCH_CALLBACK,
    STREAM_EVENT_GET_MERGER_CALLBACK,
};

enum stream_scene : u8 {
    STREAM_SCENE_A2DP,
    STREAM_SCENE_ESCO,
    STREAM_SCENE_TWS_MUSIC,     // TWS转发本地音频文件
    STREAM_SCENE_AI_VOICE,
    STREAM_SCENE_LINEIN,        //linein 模式
    STREAM_SCENE_FM,            //FM 模式
    STREAM_SCENE_TDM,            //TDM 模式
    STREAM_SCENE_MUSIC,         //本地音乐
    STREAM_SCENE_RECODER,       //录音
    STREAM_SCENE_SPDIF,
    STREAM_SCENE_PC_SPK,
    STREAM_SCENE_PC_MIC,
    STREAM_SCENE_IIS,
    STREAM_SCENE_MIC,			//mic 模式
    STREAM_SCENE_MIC_EFFECT,
    STREAM_SCENE_MIC_EFFECT2,
    STREAM_SCENE_HEARING_AID,
    STREAM_SCENE_DEV_FLOW,
    STREAM_SCENE_LE_AUDIO,//LE Audio Media
    STREAM_SCENE_LEA_CALL,//LE Audio CALL
    STREAM_SCENE_ADDA_LOOP,
    STREAM_SCENE_WIRELESS_MIC,  //16 wireless mic
    STREAM_SCENE_LOCAL_TWS,


    //最大32个场景，如果大于32个场景，需把tone、ring, key_tone场景号往后挪
    STREAM_SCENE_TONE = 0x20,
    STREAM_SCENE_RING = 0x60,
    STREAM_SCENE_KEY_TONE = 0xa0,
};

enum stream_coexist : u8 {
    STREAM_COEXIST_AUTO,
    STREAM_COEXIST_DISABLE,
};


enum stream_state : u8 {
    STREAM_STA_INIT,
    STREAM_STA_NEGOTIATE,
    STREAM_STA_READY,
    STREAM_STA_START,
    STREAM_STA_STOP,
    STREAM_STA_PLAY,

    STREAM_STA_PAUSE        = 0x10,
    STREAM_STA_PREEMPTED    = 0x20,
    STREAM_STA_SUSPEND      = 0x30,
};

enum stream_node_state : u16 {
    NODE_STA_RUN                    = 0x0001,
    NODE_STA_PAUSE                  = 0x0002,
    NODE_STA_SUSPEND                = 0x0004,
    NODE_STA_STOP                   = 0x0008,
    NODE_STA_PEND                   = 0x0010,
    NODE_STA_DEC_NO_DATA            = 0x0020,
    NODE_STA_DEC_END                = 0x0040,
    NODE_STA_DEC_PAUSE              = 0x0080,
    NODE_STA_DEC_ERR                = 0x00c0,
    NODE_STA_SOURCE_NO_DATA         = 0x0100,
    NODE_STA_DEV_ERR                = 0x0200,
    NODE_STA_ENC_END                = 0x0400,
    NODE_STA_OUTPUT_TO_FAST         = 0x0800,   //解码输出太多主动挂起
    NODE_STA_OUTPUT_BLOCKED         = 0x1000,   //终端节点缓存满,数据写不进去
    NODE_STA_OUTPUT_SPLIT           = 0x2000,
};

enum stream_node_type : u8 {
    NODE_TYPE_SYNC      = 0x01,
    NODE_TYPE_BYPASS    = 0x03,
    NODE_TYPE_FLOW_CTRL = 0x04,
    NODE_TYPE_ASYNC     = 0x10,
    NODE_TYPE_IRQ       = 0x20,
    NODE_TYPE_SWITCH    = 0x40,
};

enum negotiation_state : u8 {
    NEGO_STA_CONTINUE       = 0x01,
    NEGO_STA_ACCPTED        = 0x02,
    NEGO_STA_SUSPEND        = 0x04,
    NEGO_STA_DELAY          = 0x08,
    NEGO_STA_ABORT          = 0x10,
    NEGO_STA_SAMPLE_RATE_LOCK = 0x20,
};

enum pcm_24bit_data_type : u8 {
    PCM_24BIT_DATA_4BYTE = 0,
    PCM_24BIT_DATA_3BYTE,
};

struct stream_fmt {
    u8 Qval;
    u8 bit_wide;        //数据流中数据的位宽。
    u8 dec_bit_wide;    //解码需要配置的位宽。
    u8 pcm_24bit_type;      //用于判断3byte_24bit数据或4byte_24bit数据
    u8 channel_mode;
    u8 chconfig_id;    	//声道Id, LDAC解码需要配置的参数,通过这个解析出声道类型。
    u16 frame_dms;		//帧长时间，单位 deci-ms (ms/10)
    u16 codec_version;  //数据编码类型的版本，同一种coding_type,可能存在不同的版本,LHDC 解码需要配置的参数。
    u32 bit_rate;
    u32 sample_rate;
    u32 coding_type;
};

struct stream_enc_fmt {
    u8 channel;
    u8 bit_width;
    u16 frame_dms;		//帧长时间，单位 deci-ms (ms/10)
    u32 sample_rate;
    u32 bit_rate;
    u32 coding_type;
};

struct stream_file_ops {
    int (*read)(void *file, u8 *buf, int len);
    int (*seek)(void *file, int offset, int fromwhere);
    int (*write)(void *file, u8 *data, int len);
    int (*close)(void *file);
    int (*get_fmt)(void *file, struct stream_fmt *fmt);
    int (*get_name)(void *file, u8 *name, int len);
};

struct stream_file_info {
    void *file;
    const char *fname;
    const struct stream_file_ops *ops;
    struct audio_dec_breakpoint *dbp;
    enum stream_scene scene;
    u32 coding_type;
};

struct stream_dec_file {
    void *file;
    int (*fread)(void *file, u8 *buf, u32 fpos, int len, OS_SEM *sem);
};

struct stream_get_fmt {
    struct stream_file_info *file_info;
    struct stream_fmt *fmt;
};

struct stream_decoder_info {
    enum stream_scene scene;
    u8 frame_time;
    u32 coding_type;
    const char *task_name;
};

struct stream_encoder_info {
    enum stream_scene scene;
    u32 coding_type;
    const char *task_name;
};

/*
 * scene_a和scene_b指定的格式有冲突，scene_a抢占scene_b
 */
struct stream_coexist_policy {
    enum stream_scene scene_a;
    enum stream_scene scene_b;
    u32 coding_a;
    u32 coding_b;
};

enum frame_flags : u16 {
    FRAME_FLAG_FLUSH_OUT                = 0x01,
    FRAME_FLAG_FILL_ZERO                = 0x03,     //补0包
    FRAME_FLAG_FILL_PACKET              = 0x05,     //补数据包
    FRAME_FLAG_TIMESTAMP_ENABLE         = 0x08,     //时间戳有效
    FRAME_FLAG_RESET_TIMESTAMP_BIT      = 0x10,
    FRAME_FLAG_RESET_TIMESTAMP          = 0x11,     //重设各节点的时间戳
    FRAME_FLAG_SET_DRIFT_BIT            = 0x20,
    FRAME_FLAG_SET_D_SAMPLE_RATE        = 0x21,     //设置sampte_rate的delta
    FRAME_FLAG_UPDATE_TIMESTAMP         = 0x40,     //更新时间戳，驱动节点重设
    FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE = 0x80,
    FRAME_FLAG_SYS_TIMESTAMP_ENABLE     = 0x100,    //数据帧使用系统时间戳
    FRAME_FLAG_PERIOD_SAMPLE            = 0x200,
    FRAME_FLAG_UPDATE_INFO              = FRAME_FLAG_UPDATE_TIMESTAMP | FRAME_FLAG_UPDATE_DRIFT_SAMPLE_RATE,

    FRAME_FLAG_PULL_AGAIN               = 0x1000    //frame被pull过之后被重新加回iport->frame
};

enum audio_Qval : u8 {
    AUDIO_QVAL_16BIT = 15,
    AUDIO_QVAL_24BIT = 23,
};

struct node_port_data_wide {
    u8 iport_data_wide;//DATA_BIT_WIDE_16BIT,DATA_BIT_WIDE_24BIT
    u8 oport_data_wide;//DATA_BIT_WIDE_16BIT,DATA_BIT_WIDE_24BIT
} __attribute__((packed));

struct stream_bit_width {
    u8 bit_width;
} __attribute__((packed));

struct stream_frame {
    struct list_head entry;
    u8 bit_wide;
    u16 delay;
    u16 offset;
    u16 len;
    u16 size;
    s16 d_sample_rate;
    enum frame_flags flags;
    u32 timestamp;
    u32 fpos;
    u8 data[0];
};


struct stream_thread {
    u8 id;
    u8 debug;
    u8 start;
    u32 start_usec;
    char name[16];
    OS_SEM sem;
    OS_MUTEX mutex;
    struct jlstream *stream;
    struct stream_thread *next;
};

struct stream_iport {
    struct list_head frames;

    u8 id;

    u16 max_len;

    enum stream_node_state state;

#if STREAM_NODE_RUN_TIMER_DEBUG_EN
    u32 run_time;       //usec
#endif

    struct stream_node *node;

    struct stream_oport *prev;

    struct stream_iport *sibling;

    void (*handle_frame)(struct stream_iport *, struct stream_note *note);
    int private_data[0];
};


struct stream_oport {

    s16 d_sample_rate;
    enum frame_flags flags;
    u8 id;
    u16 buffered_pcms;

    struct stream_fmt fmt;

    u32 offset;
    u32 timestamp;

    struct stream_node *node;

    struct stream_iport *next;

    struct stream_oport *sibling;
};


struct stream_node_adapter {
    const char *name;
    u16 hdl_size;
    u16 iport_size;

    u16 uuid;

    u8 ability_channel_in;
    u8 ability_channel_out;
    u8 ability_channel_convert;
    u8 ability_resample;
    u8 ability_bit_wide;
    u8 fixed_oport;

    int (*bind)(struct stream_node *, u16 uuid);

    int (*ioctl)(struct stream_iport *iport, int cmd, int arg);

    void (*release)(struct stream_node *node);
};

struct stream_node {
    u16 uuid;
    u16 pipeline;

    u8 subid;
    enum stream_node_type type;
    enum stream_node_state state;

    struct stream_iport *iport;

    struct stream_oport *oport;

    OS_MUTEX mutex;

    const struct stream_node_adapter *adapter;
    int private_data[0];
};


struct stream_snode {
    struct stream_node node;
    struct jlstream *stream;
    int private_data[0];
};

#define hdl_node(hdl) container_of(hdl, struct stream_node, private_data)
#define hdl_snode(hdl) container_of(hdl, struct stream_snode, private_data)

enum {
    THREAD_TYPE_SOURCE  = 0x01,
    THREAD_TYPE_DEC     = 0x02,
    THREAD_TYPE_ENC     = 0x04,
    THREAD_TYPE_OUTPUT  = 0x08,
};


struct stream_note {

    u8 output_time;
    u8 output_start;
    enum stream_node_state state;

    int delay;
    int sleep;
    int min_sleep;
    u32 jiffies;

    struct jlstream *stream;
    struct stream_thread *thread;
};

struct stream_ctrl {
    enum stream_state state;
    u16 fade_msec;
    u32 fade_timeout;
    OS_SEM sem;
};

struct jlstream {
    struct list_head entry;

    u8 id;
    u8 ref;
    u8 run_cnt;
    u8 delay;
    u8 usage;
    u8 incr_sys_clk;
    u8 thread_run;
    u8 thread_num;
    u16 output_time;
    u8 thread_policy_step;
    enum stream_state state;
    enum stream_state pp_state;
    enum stream_coexist coexist;

    u16 max_delay;
    u16 dest_delay;         // 目标缓存大小
    u16 timer;
    u16 thread_timer;
    enum stream_scene scene;

    u32 end_jiffies;
    u32 coding_type;
#if STREAM_NODE_RUN_TIMER_DEBUG_EN
    u32 run_usec;
#endif

    struct stream_snode *snode;

    struct stream_ctrl ctrl;

    struct stream_thread *thread;
    struct stream_thread *last_thread;

    jlstream_breaker_t breaker;

    void *callback_arg;
    const char *callback_task;
    void (*callback_func)(void *, int);
};


#define REGISTER_STREAM_NODE_ADAPTER(adapter) \
    const struct stream_node_adapter adapter sec(.stream_node_adapter)

#define PCM_SAMPLE_ONE_SECOND   (1000000 * TIMESTAMP_US_DENOMINATOR)
#define PCM_SAMPLE_TO_TIMESTAMP(frames, sample_rate) \
    ((u64)(frames) * PCM_SAMPLE_ONE_SECOND / ((u32)sample_rate))

#define TIME_TO_PCM_SAMPLES(time, sample_rate) \
    (((u64)time * sample_rate / PCM_SAMPLE_ONE_SECOND) + (((u64)time * sample_rate) % PCM_SAMPLE_ONE_SECOND == 0 ? 0 : 1))


void jlstream_lock();

void jlstream_unlock();

int jlstream_event_notify(enum stream_event, int arg);



/*
 * 申请一个指定长度的frame
 */
struct stream_frame *jlstream_get_frame(struct stream_oport *, u16 len);

/*
 * 将frame推送给后级节点
 */
void jlstream_push_frame(struct stream_oport *, struct stream_frame *);

/*
 * 获取一个前级推送的frame
 */
struct stream_frame *jlstream_pull_frame(struct stream_iport *, struct stream_note *);

/*
 * 从前级推送的frame中读取指定长度数据
 */
int jlstream_pull_data(struct stream_iport *iport, u32 fpos, u8 *buf, int len);


/*
 * 释放已经处理完数据的frame
 */
void jlstream_free_frame(struct stream_frame *frame);

/*
 * 释放所有缓存的frame
 */
void jlstream_free_iport_frames(struct stream_iport *iport);



void jlstream_frame_bypass(struct stream_iport *iport, struct stream_note *note);


int jlstream_get_iport_data_len(struct stream_iport *iport);

int jlstream_get_iport_delay(struct stream_iport *iport);

int jlstream_get_iport_frame_num(struct stream_iport *iport);

void jlstream_wakeup_thread(struct jlstream *stream, struct stream_node *node, struct stream_thread *thread);

int stream_node_ioctl(struct stream_node *node, u16 uuid, int cmd, int arg);

int jlstream_node_ioctl(struct jlstream *stream, u16 uuid, int cmd, int arg);

int jlstream_ioctl(struct jlstream *jlstream, int cmd, int arg);

int jlstream_iport_ioctl(struct stream_iport *iport, int cmd, int arg);


struct jlstream *jlstream_for_node(struct stream_node *node);

int jlstream_read_node_data(u16 uuid, u8 subid, u8 *buf);
int jlstream_read_node_port_data_wide(u16 uuid, u8 subid, u8 *buf);

int jlstream_read_stream_crc();

int jlstream_get_delay(struct jlstream *stream);


/*
 * 数据流创建和控制接口
 */
struct jlstream *jlstream_pipeline_parse(u16 pipeline, u16 source_uuid);
struct jlstream *jlstream_pipeline_parse_by_node_name(u16 pipeline, const char *node_name);

void jlstream_set_scene(struct jlstream *stream, enum stream_scene scene);

void jlstream_set_coexist(struct jlstream *stream, enum stream_coexist coexist);

void jlstream_set_callback(struct jlstream *stream, void *arg,
                           void (*callback)(void *, int));


/*
 * 设置文件句柄和文件操作接口
 */
int jlstream_set_dec_file(struct jlstream *stream, void *file_hdl,
                          const struct stream_file_ops *ops);

int jlstream_set_enc_file(struct jlstream *stream, void *file_hdl,
                          const struct stream_file_ops *ops);


int jlstream_add_thread(struct jlstream *stream, const char *thread_name);

/*
 * 启动数据流
 */
int jlstream_start(struct jlstream *stream);


/*
 * 播放/暂停切换接口,fade_msec 为淡入/淡出时间
 */
int jlstream_pp_toggle(struct jlstream *stream, u16 fade_msec);


/*
 *停止数据流,fade_msec 为淡出时间
 */
void jlstream_stop(struct jlstream *stream, u16 fade_msec);


/*
 *释放数据流
 */
void jlstream_release(struct jlstream *stream);


int jlstream_fade_out_32bit(int value, s16 step, s32 *data, int len, u8 channel);
int jlstream_fade_out(int value, s16 step, s16 *data, int len, u8 channel);

int jlstream_fade_in(int value, s16 step, s16 *data, int len, u8 channel);
int jlstream_fade_in_32bit(int value, s16 step, s32 *data, int len, u8 channel);

/*
 * 获取节点和设置节点参数接口
 */
void *jlstream_get_node(u16 node_uuid, const char *name, u16 max_param_len);

int jlstream_set_node_param_s(void *node, void *param, u16 param_len);

int jlstream_get_node_param_s(void *node, void *param, u16 param_len);

void jlstream_put_node(void *);

int jlstream_set_node_param(u16 node_uuid, const char *name, void *param, u16 param_len);

int jlstream_get_node_param(u16 node_uuid, const char *name, void *param, u16 param_len);

int jlstream_read_bit_width(u16 uuid, u8 *buf);

int jlstream_is_contains_node_from(struct stream_node *node, u16 node_uuid);

int jlstream_read_indicator_node_data(u16 uuid, u8 subid, u8 *param);
/*
 * 数据淡入淡出接口
 *
 */
struct jlstream_fade {
    u8 in;
    u8 out;
    u8 channel;
    u8 bit_wide;
    s16 step;
    s16 value;
};

/*
 * dir 0:淡出,  1:淡入
 */
void jlstream_fade_init(struct jlstream_fade *fade, int dir, u32 sample_rate,
                        u8 channel, u16 msec);

enum stream_fade_result {
    STREAM_FADE_IN,
    STREAM_FADE_OUT,
    STREAM_FADE_IN_END,
    STREAM_FADE_OUT_END,
    STREAM_FADE_END,
};

enum stream_fade_result jlstream_fade_data(struct jlstream_fade *fade, u8 *data, int len);

void jlstream_cross_fade(s16 *fadein_addr, s16 *fadeout_addr, s16 *output_addr, int len, int ch);

void jlstream_cross_fade_32bit(s32 *fadein_addr, s32 *fadeout_addr, s32 *output_addr, int len, int ch);

void jlstream_del_node_from_thread(struct stream_node *node);

int jlstream_add_node_2_thread(struct stream_node *node, const char *task_name);

void jlstream_global_lock();
void jlstream_global_unlock();
bool jlstream_global_locked();

int jlstream_global_pause();
int jlstream_global_resume();

jlstream_breaker_t jlstream_insert_breaker(struct jlstream *stream,
        u16 uuid_a, const char *name_a,
        u16 uuid_b, const char *name_b);

int jlstream_delete_breaker(jlstream_breaker_t breaker, bool restore);

void jlstream_return_frame(struct stream_iport *iport, struct stream_frame *frame);

int jlstream_get_cpu_usage();

void stream_mem_unfree_dump();
#endif

