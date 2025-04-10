/*********************************************************************************************
    *   Filename        : wireless_params.h

    *   Description     :

    *   Author          : Weixin Liang

    *   Email           : liangweixin@zh-jieli.com

    *   Last modifiled  : 2022-12-15 14:26

    *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef __WIRELESS_TRANS_H
#define __WIRELESS_TRANS_H

/*  Include header */
#include "big.h"
#include "cig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/
#ifndef CONFIG_LE_AUDIO_PARAMS_MAX_NUM
#define CONFIG_LE_AUDIO_PARAMS_MAX_NUM  10
#endif

#define WIRELESS_DATA_CALLBACK_FUNC_REGISTER(channel) \
    const struct wireless_data_callback_func channel SEC(.wireless_call_sync)

#define list_for_each_wireless_data_callback(p) \
    for (p = wireless_data_callback_func_begin; p < wireless_data_callback_func_end; p++)

// 设备标识，用于区分不同设备，与实际声道没有直接的关联
#define WIRELESS_TX_DEV1  BIT(0)   //发送设备1
#define WIRELESS_TX_DEV2  BIT(1)   //发送设备2
#define WIRELESS_RX_DEV1  BIT(2)   //接收设备1
#define WIRELESS_MASTER_DEV  BIT(0)   //主机
#define WIRELESS_SLAVE_DEV1  BIT(1)   //从机1
#define WIRELESS_SLAVE_DEV2  BIT(2)   //从机2

#define WIRELESS_SYNC_CALL_REGISTER(sync_call) \
    static const struct wireless_sync_call sync_call sec(.wireless_sync_call_func)

#define list_for_each_wireless_sync_call(p) \
    for (p = wireless_sync_call_func_begin; p < wireless_sync_call_func_end; p++)

#define WIRELESS_CUSTOM_DATA_STUB_REGISTER(stub) \
    static const struct wireless_custom_data_trans_stub stub sec(.wireless_custom_data_trans_stub)

#define list_for_each_wireless_data_trans(p) \
    for (p = wireless_custom_data_trans_stub_begin; p < wireless_custom_data_trans_stub_end; p++)

#define WIRELESS_SYNC_CALL_TX                1
#define WIRELESS_SYNC_CALL_RX                2

/**************************************************************************************************
  Data Types
**************************************************************************************************/
struct broadcast_cfg_args {
    u8 len;
    u32 sdu_interval;
    u8 rtn;
    u8 mtl;
    u32 bitrate;
    u32 tx_latency;
    u32 play_latency;
} __attribute__((packed));

struct broadcast_platform_data {
    struct broadcast_cfg_args args[CONFIG_LE_AUDIO_PARAMS_MAX_NUM];

    //非可视化工具的参数反正下面
    u8 nch;
    u8 frame_len;
    u32 coding_type;
    u32 sample_rate;
};

struct connected_cfg_args {
    u8 len;
    u32 sdu_interval;
    u8 rtnCToP;
    u8 rtnPToC;
    u8 mtlCToP;
    u8 mtlPToC;
    u32 bitrate;
    u32 tx_latency;
    u32 play_latency;
} __attribute__((packed));

struct connected_platform_data {
    struct connected_cfg_args args[CONFIG_LE_AUDIO_PARAMS_MAX_NUM];

    //非可视化工具的参数反正下面
    u8 nch;
    u8 frame_len;
    u32 coding_type;
    u32 sample_rate;
};

struct le_audio_mode_ops {
    int (*local_audio_open)(void);
    int (*local_audio_close)(void);
    void *(*tx_le_audio_open)(void *fmt);
    int (*tx_le_audio_close)(void *le_audio);
    int (*rx_le_audio_open)(void *rx_audio, void *args);
    int (*rx_le_audio_close)(void *rx_audio);
    int (*play_status)(void);                 //获取当前音频播放状态
};

/*! \brief 自定义数据转发和事件同步回调函数接头体 */
struct wireless_data_callback_func {
    void(*event_handler)(const CIG_EVENT event, void *arg);
    int(*tx_events_suss)(void *data, int len);
    int(*rx_events_suss)(u16 acl_hdl, const void *const data, int len);
};

struct wireless_sync_call {
    int uuid;
    void (*func)(int priv, int sync_call_role);
    const char *task_name;
};

struct wireless_custom_data_trans_stub {
    int uuid;
    void (*func)(u16 acl_hdl, u8 arg_num, int *argv);
    const char *task_name;
};

/*! \brief 本地音源播放器状态 */
enum {
    LOCAL_AUDIO_PLAYER_STATUS_ERR,
    LOCAL_AUDIO_PLAYER_STATUS_PLAY,
    LOCAL_AUDIO_PLAYER_STATUS_STOP,
};

enum le_audio_mode_t {
    LE_AUDIO_1T3_MODE = 100,
};

enum {
    WIRELESS_TRANS_32K_SR = 1,
    WIRELESS_TRANS_48K_SR = 2,
};

enum {
    WIRELESS_TRANS_CODEC_JLA = 0,
    WIRELESS_TRANS_CODEC_JLA_LL,
    WIRELESS_TRANS_CODEC_JLA_LW,
};

enum {
    WIRELESS_KEY_SYNC_TYPE = 0xA2,
    WIRELESS_CUSTOM_DATA_TRANSMIT_TYPE = 0xA3,
};

enum {
    CMD_MUSIC_PP = 0x41,
    CMD_MUSIC_PREV,
    CMD_MUSIC_NEXT,
    CMD_VOCAL_REMOVE,
    CMD_MIC_SWITCH,
    CMD_MIC_VOL_CHANGE,
    CMD_MIC_PITCH_CHANGE,
    CMD_ECHO_DELAY_CHANGE,
    CMD_EFFECT_MODE_CHANGE,
    CMD_EFFECT_MODE_KTV,
    CMD_EFFECT_MODE_CLUB,
    CMD_EFFECT_MODE_COMPERE,
    CMD_EFFECT_MODE_MONSTER,
    CMD_EFFECT_MODE_ORIGINAL,
    CMD_NOISEGATE_SW,
};

enum {
    LE_AUDIO_APP_MODE_ENTER,
    LE_AUDIO_APP_MODE_EXIT,
    LE_AUDIO_A2DP_START,
    LE_AUDIO_A2DP_STOP,
    LE_AUDIO_PHONE_START,
    LE_AUDIO_PHONE_STOP,
    LE_AUDIO_EDR_DISCONN,
    LE_AUDIO_MUSIC_START,
    LE_AUDIO_MUSIC_STOP,
    LE_AUDIO_APP_OPEN,
    LE_AUDIO_APP_CLOSE,
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
extern struct wireless_data_callback_func  wireless_data_callback_func_begin[];
extern struct wireless_data_callback_func  wireless_data_callback_func_end[];
extern const struct wireless_sync_call  wireless_sync_call_func_begin[];
extern const struct wireless_sync_call  wireless_sync_call_func_end[];
extern const struct wireless_custom_data_trans_stub wireless_custom_data_trans_stub_begin[];
extern const struct wireless_custom_data_trans_stub wireless_custom_data_trans_stub_end[];

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/* --------------------------------------------------------------------------*/
/**
 * @brief 任务同步调用接口，约定事件双方同时执行对应的事件任务
 *
 * @param uuid:注册回调时配置的ID
 * @param priv:需要同步执行的事件
 * @param delay_ms:约定的延时时间
 * @param device_channel:需要跟本机同步执行的远端设备的标识
 *
 * @return len:实际发出去的数据长度
 */
/* ----------------------------------------------------------------------------*/
int wireless_api_sync_call_by_uuid(u8 uuid, int priv, int delay_ms, u8 device_channel);

/* --------------------------------------------------------------------------*/
/**
 * @brief 发送数据给远端设备
 *
 * @param uuid:注册回调时配置的ID
 * @param data:需要发送的数据
 * @param len:发送数据的长度
 * @param device_channel:远端设备的标识
 *
 * @return 实际发送的数据长度
 */
/* ----------------------------------------------------------------------------*/
int wireless_custom_data_send_to_sibling(u8 uuid, void *data, u16 len, u8 device_channel);

void read_le_audio_product_name(void);
void read_le_audio_pair_name(void);
const char *get_le_audio_product_name(void);
const char *get_le_audio_pair_name(void);
struct le_audio_mode_ops *get_broadcast_audio_sw_ops();
struct le_audio_mode_ops *get_connected_audio_sw_ops();
int le_audio_ops_register(u8 mode);
int le_audio_ops_unregister(void);
void wireless_trans_auto_test1_init(void);
void wireless_trans_auto_test2_init(void);
void wireless_trans_auto_test3_init(void);
void wireless_trans_auto_test4_init(void);

u32 get_big_enc_output_frame_len(void);
u16 get_big_transmit_data_len(void);
u32 get_big_enc_output_buf_len(void);
u32 get_big_dec_input_buf_len(void);
u32 get_big_sdu_period_us(void);
u32 get_big_tx_latency(void);
u32 get_big_play_latency(void);
u32 get_big_mtl_time(void);
u8 get_bis_num(u8 role);
void set_big_hdl(u8 role, u8 big_hdl);
void update_receiver_big_codec_params(void *sync_data);
void update_receiver_big_params(uint16_t Max_PDU, uint16_t iso_Interval);
big_parameter_t *set_big_params(u8 app_task, u8 role, u8 big_hdl);
int get_big_audio_coding_nch(void);
int get_big_audio_coding_bit_rate(void);
int get_big_audio_coding_frame_duration(void);
int get_big_tx_rtn(void);
int get_big_tx_delay(void);

u32 get_cig_enc_output_frame_len(void);
u16 get_cig_transmit_data_len(void);
u32 get_cig_enc_output_buf_len(void);
u32 get_cig_dec_input_buf_len(void);
u32 get_cig_sdu_period_us(void);
u32 get_cig_mtl_time(void);
void set_cig_hdl(u8 role, u8 cig_hdl);
cig_parameter_t *set_cig_params(u8 app_task, u8 role, u8 cig_hdl);
int get_cig_audio_coding_nch(void);
int get_cig_audio_coding_bit_rate(void);
int get_cig_audio_coding_frame_duration(void);
int get_cig_tx_rtn(void);
int get_cig_tx_delay(void);
u32 get_cig_tx_latency(void);
u32 get_cig_play_latency(void);
u8 get_cis_num(u8 role);
void reset_cig_params(void);

void le_audio_working_status_switch(u8 en);
int le_audio_scene_deal(int scene);
int update_le_audio_deal_scene(int scene);
u8 get_le_audio_app_mode_exit_flag();
u8 get_le_audio_curr_role();
#ifdef __cplusplus
};
#endif

#endif


