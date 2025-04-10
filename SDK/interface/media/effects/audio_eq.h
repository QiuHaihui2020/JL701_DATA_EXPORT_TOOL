
#ifndef _EQ_API_H_
#define _EQ_API_H_

#include "typedef.h"
#include "asm/hw_eq.h"
#include "media/audio_stream.h"
#include "system/timer.h"
#include "system/init.h"
#include "effects/effects_adj.h"

#define EQ_COEFF_NUM_PER_SECTION    5

/*eq type*/
typedef enum {
    EQ_MODE_NORMAL = 0,
    EQ_MODE_ROCK,
    EQ_MODE_POP,
    EQ_MODE_CLASSIC,
    EQ_MODE_JAZZ,
    EQ_MODE_COUNTRY,
    EQ_MODE_CUSTOM,//自定义
    EQ_MODE_MAX,
} EQ_MODE;

/*eq type*/
typedef enum {
    EQ_TYPE_FILE = 0x01,
    EQ_TYPE_ONLINE,
    EQ_TYPE_MODE_TAB,
} EQ_TYPE;

typedef enum {
    HW_EQ_CORE_AUTO_SEL  = 0,
    HW_EQ_CORE_0         = 1,
    HW_EQ_CORE_1         = 2,
} HW_EQ_CORE_SEL;

#define audio_eq_filter_info 	eq_coeff_info

typedef int (*audio_eq_filter_cb)(void *eq, int sr, struct audio_eq_filter_info *info);

//64位的位或处理
#define     MASK_BIT(n)              (u64)((u64)1 << (n))
struct audio_eq;

struct eq_parm {
    u8 in_mode;         //输入数据位宽 0:short  1:int  2:float
    u8 out_mode;        //输出数据位宽 0:short  1:int  2:float
    u8 run_mode;        //运行模式     0:normal 1:mono 2:stereo
    u8 data_in_mode;    //输入数据排放模式 0:block_in  1:sequence_in
    u8 data_out_mode;   //输出数据排放模式 0:block_out  1:sequenceo_out
};
struct audio_eq_param {
    u8 no_wait;         //异步eq处理使能, 1:使能异步eq(非阻塞)  0：使用同步阻塞方式eq处理
    u8 max_nsection;    //最大的eq段数,根据使用填写，要小于等于EQ_SECTION_MAX
    u8 nsection;        //实际需要的eq段数，需小于等于max_nsection
    u8 out_32bit;       //输入16bit时,输出位宽控制 0:16bit  1:32bit(如需输入其他位宽，请看结构体 struct eq_parm)
    u8 channels;        //通道数,1：单声道  2：立体声
    u8 core;            //多硬件支持选配，需芯片支持  0:自动选配， 1:使用eq core1  2：使用eq core2 (HW_EQ_CORE_SEL)
    audio_eq_filter_cb cb;//获取eq系数的回调函数
    u32 sr;              //采样率，更根据当前数据实际采样率填写

    float global_gain;
    struct eq_seg_info *seg;
    float global_gain_r;
    struct eq_seg_info *seg_r;
    void *parm;              //struct eq_parm

    void *priv;          //私有指针
    int (*output)(void *priv, void *data, u32 len);//异步eq输出回调
};


struct eq_fade_parm {
    u8 fade_time;       //系数更新是否需要淡入，淡入timer循环时间建议值(10~100ms)
    u8 f_fade_step;     //滤波器中心截止频率淡入步进（10~100Hz）
    float fade_step;    //滤波器增益淡入步进（0.01f~1.0f）
    float q_fade_step;  //滤波器q值淡入步进（0.01f~1.0f）
    float g_fade_step;  //总增益淡入步进（0.01f~1.0f）
    void *priv;
    int (*callback)(struct audio_eq *eq, void *priv);//淡出结束时的回调函数

};
struct audio_eq_fade {
    u8 type_en;
    u8 nsection;
    u16 timer;
    u32 cur_seg_size;
    float cur_global_gain;
    float use_global_gain;
    struct eq_seg_info *cur_seg;
    struct eq_seg_info *use_seg;
    u8 *gain_flag;
    void *priv;
    int (*callback)(struct audio_eq *eq, void *priv);

} ;



#ifdef CONFIG_EQ_SUPPORT_ASYNC
struct audio_eq_async {
    u16 ptr;
    u16 len;
    u16 buf_len;
    u8 clear;
    u8 out_stu;
    char *buf;
};
#endif

struct audio_eq {
    void *eq_ch;                          //硬件eq句柄
    u8 updata;                            //系数是否需要更新
    u8 start;                             //eq start标识
    u8 max_nsection;                      //eq最大段数
    u8 cur_nsection;                      //当前eq段数
    u8 check_hw_running;                  //检测到硬件正在运行时不等待其完成1:设置检查  0：不检查
    u8 async_en;
    u8 out_32bit;
    u8 ch_num;
    u8 core;                              //多硬件支持选配，需芯片支持
    u8 run_status;
    u8 dma_ram;                           //是否使用外部配置的dma_ram 0:否， 1：是
    u8 ext_seg_num[2];
    u8 ext_coeff_lp_num[2];
    u8 ext_coeff_hp_num[2];
    u32 sr;                                    //采样率
    u64 mask[2];
    float global_gain;
    float global_gain_r;
    s16 *eq_out_buf;//同步方式，32bit输出，当out为NULL时，内部申请eq_out_buf
    int out_buf_size;

#ifdef CONFIG_EQ_SUPPORT_ASYNC
    void *run_buf;
    void *run_out_buf;
    int run_len;
    struct audio_eq_async async;                     //异步eq 输出管理
#endif
    u32 mute_cnt_l;                                  //左声道eq mem清除计数
    u32 mute_cnt_r;                                  //右声道eq mem清除计数
    u32 mute_cnt_max;                                //记录最大点数，超过该点数，清eq mem

    audio_eq_filter_cb cb;                           //系数回调
    void *output_priv;                               //私有指针
    int (*output)(void *priv, void *data, u32 len);  //输出回调

    struct eq_seg_info *eq_seg_tab;                 //运算前系数表,特殊应用时，可使用
    struct eq_seg_info *seg;                        //运算前系数表,由audio_eq_param初始化时指定
    struct eq_seg_info *seg_r;                      //运算前系数表,由audio_eq_param初始化时指定
    int *eq_coeff_tab;                              //运算后系数表
    struct audio_eq_fade *fade;
    struct audio_eq_fade *fade_r;
    struct eq_fade_parm fade_parm;
    void (*resume)(void *priv);                     //非数据流节点方式，激活解码
    void *resume_priv;
    spinlock_t lock;
};

/*获取默认系数表EQ_MODE*/
struct eq_default_seg_tab {
    float global_gain;
    int seg_num;
    struct eq_seg_info *seg;
};

struct eq_default_parm {    //eq默认参数配置
    char name[16];       //节点名字
    char mode_index;//节点与多模式关联时，该变量用于获取相应模式下的节点参数.模式序号（如，蓝牙模式下，无多子模式，mode_index 是0）
    char cfg_index;//默认配置项序号(指定配置文件内的目标配置项)
    char core;
    struct eq_default_seg_tab   default_tab;//发生过调用默认系数表切换(eq_mode_set, eq_mode_sw)的动作时，此处的获取才会有效，否则使用文件内cfg_name对应的参数
};


struct eq_tool {//配置项存储结构
    float global_gain;           //总增益
    int is_bypass: 8;                //0 关闭， 1：打开bypass
    int seg_num: 8;                //eq效果文件存储的段数 +2
    int reserve: 16;
    struct eq_seg_info seg[0];   //eq系数存储地址,高阶高通低通滤波器追加存储在尾部
};

struct eq_online { //在线调试临时存储的系数结构
    int uuid;
    char name[16];
    int  is_bypass;              //1: 直通
    float global_gain;           //总增益
    int seg_num;                 //eq效果文件存储的段数
    struct eq_seg_info seg[0];   //eq系数存储地址
};

#define EQ_NAME_CMD        0x1
#define EQ_IS_BYPASS_CMD   0x2
#define EQ_SEG_NUM_CMD     0x3

#define EQ_SEG_CMD         0x7
#define EQ_GLOBAL_GAIN_CMD 0x8
/*
 *特殊类型，用于工具同步所有节点参数下来时，提速使用
 * */
#define EQ_SPECIAL_CMD     0x9//bypass 总增益 段数
#define EQ_SPECIAL_SEG_CMD 0xa//多滤波器同时更新

#define EQ_TAB_CMD         0xb//整个系数表更新，包括global_gain，seg_num，seg

/*调音结构、eq节点参数更新结构*/
struct eq_adj {
    /*****************************/
    /*工具调音结构*/
    int type;
    union {
        int  is_bypass;//1: 直通
        float global_gain;
        int seg_num;                 //eq效果文件存储的段数
        struct eq_seg_info seg;
        struct eq_default_seg_tab tab;
    } param;
    /*****************************/
    /*参数更新附带淡入配置结构*/
    struct eq_fade_parm fade_parm;
};

struct eq_core_check {
    struct list_head entry;
    void *stream;
    int core;
};
/*
 *eq core自动选配,并添加到eq core链表
 * */
int eq_core_auto_sel(struct eq_core_check *check, void *stream);
/*
 * 从eq core链表内删除
 * */
void eq_core_auto_del(struct eq_core_check *check);

/*----------------------------------------------------------------------------*/
/**@brief    eq模块初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_init(int eq_section_num);
/*----------------------------------------------------------------------------*/
/**@brief    eq 打开
   @param    *param: eq参数句柄,参数详见结构体struct audio_eq_param
   @return   eq句柄
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_open(struct audio_eq *eq, struct audio_eq_param *param);
/*----------------------------------------------------------------------------*/
/**@brief    异步eq设置输出回调
   @param    eq:句柄
   @param    *output:回调函数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_output_handle(struct audio_eq *eq, int (*output)(void *priv, void *data, u32 len), void *output_priv);
/*----------------------------------------------------------------------------*/
/**@brief    同步eq设置输出buf
   @param    eq:句柄
   @param    *output:回调函数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_output_buf(struct audio_eq *eq, s16 *buf, u32 len);
/*----------------------------------------------------------------------------*/
/**@brief    eq设置采样率
   @param    eq:句柄
   @param    sr:采样率
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_samplerate(struct audio_eq *eq, int sr);
/*----------------------------------------------------------------------------*/
/**@brief    eq设置输入输出通道数
   @param    eq:句柄
   @param    channel:通道数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_channel(struct audio_eq *eq, u8 channel);

/*----------------------------------------------------------------------------*/
/**@brief    设置检查eq是否正在运行
   @param    eq:句柄
   @param    check_hw_running: 1:设置检查  0：不检查
   @return
   @note     检测到硬件正在运行时不等待其完成，直接返回, 仅异步EQ有效
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_check_running(struct audio_eq *eq, u8 check_hw_running);
/*----------------------------------------------------------------------------*/
/**@brief    设置eq信息
   @param    eq:句柄
   @param    channels:设置输入输出通道数
   @param    out_32bit:使能eq输出32bit数据，1：输出32bit数据， 0：输出16bit是数据
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_info(struct audio_eq *eq, u8 channels, u8 out_32bit);

/*----------------------------------------------------------------------------*/
/**@brief    设置eq信息
   @param    eq:句柄
   @param    channels:设置输出通道数
   @param    in_mode:使能eq输入32bit数据，2:float, 1：输入32bit数据， 0：输入16bit是数据
   @param    out_mode:使能eq输出32bit数据，2:float, 1：输出32bit数据， 0：输出16bit是数据
   @param    run_mode:运行模式，0：normal, 1:mono, 2:stero
   @param    data_in_mode:输入数据存放方式 0：块模式  1：序列模式
   @param    data_out_mode:输入数据存放方式 0：块模式  1：序列模式
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_info_new(struct audio_eq *eq, struct hw_eq_info *info);

/*----------------------------------------------------------------------------*/
/**@brief    eq启动
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_start(struct audio_eq *eq);
/*----------------------------------------------------------------------------*/
/**@brief    eq的数据处理
   @param    eq:句柄
   @param    *data:输入数据地址
   @param    len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_run(struct audio_eq *eq, s16 *data, u32 len);
/*----------------------------------------------------------------------------*/
/**@brief    eq关闭
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_close(struct audio_eq *eq);


#ifdef CONFIG_EQ_SUPPORT_ASYNC
/*----------------------------------------------------------------------------*/
/**@brief    清除异步eq剩余的数据
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_async_data_clear(struct audio_eq *eq);
#endif
/*----------------------------------------------------------------------------*/
/**@brief    获取异步eq剩余的数据
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_data_len(struct audio_eq *eq);


/*
 * audio_eq_open重新封装，简化使用
 **/
/*----------------------------------------------------------------------------*/
struct audio_eq *audio_dec_eq_open(struct audio_eq_param *parm);
/*
 *audio_eq_close重新封装，简化使用
 * */
void audio_dec_eq_close(struct audio_eq *eq);
/*
 *audio_eq_run重新疯转，支持输入输出不同地址，如使用异步处理，out配NULL
 * */
int audio_dec_eq_run(struct audio_eq *eq, s16 *in,  s16 *out, u16 len);


int audio_eq_flush_out(struct audio_eq *eq);

int check_eq_core(struct eq_core_check *check);
/*
 *更新左声道总增益
 * */
void audio_eq_update_global_gain(struct audio_eq *eq, struct eq_fade_parm fade_parm, float g_Gain);
/*
 *更新右声道总增益
 * */
void audio_eq_update_right_ch_global_gain(struct audio_eq *eq, struct eq_fade_parm fade_parm, float g_Gain);


/*
 *更新左声某一段eq系数seg
 *nsection:总的段数
 * */
void audio_eq_update_seg_info(struct audio_eq *eq, struct eq_fade_parm fade_parm, struct eq_seg_info *seg, u32 nsection);
/*
 *更新右声道某一段eq系数
 *nsection:总的段数
 * */
void audio_eq_update_right_ch_seg_info(struct audio_eq *eq, struct eq_fade_parm fade_parm, struct eq_seg_info *seg, u32 nsection);

/*
 * 更新eq系数表
 * */
void audio_eq_update_tab(struct audio_eq *eq, struct eq_default_seg_tab *tab);

/*
 * 更新右声道eq系数表
 * */
void audio_eq_update_right_ch_tab(struct audio_eq *eq, struct eq_default_seg_tab *tab);

/*
 *段数发生改变，更新系数表地址
 * */
void audio_eq_update_seg_ptr(struct audio_eq *eq, struct eq_seg_info *seg);
/*
 *eq滤波器系数回调
 * */
int eq_get_filter_info(void *_eq, int sr, struct audio_eq_filter_info *info);

/*
 *初始化为直通系数
 *nsection:总的段数
 *coeff:目标系数首地址
 * */
void eq_coeff_default_init(u32 nsection, void *coeff);

/*----------------------------------------------------------------------------*/
/**@brief    滤波器计算管理函数
   @param    *seg:提供给滤波器的基本信息
   @param    sample_rate:采样率
   @param    *coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int eq_seg_design(struct eq_seg_info *seg, int sample_rate, void *coeff);

int audio_eq_tab_check(struct eq_seg_info *seg, u16 seg_num);
int audio_eq_cal_hp_lp_filter_addr(struct audio_eq *eq, struct eq_seg_info *seg_tab, u8 i);
#endif

