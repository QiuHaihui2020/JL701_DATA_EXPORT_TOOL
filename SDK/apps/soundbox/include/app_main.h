#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "app_msg.h"
#include "app_mode_manager/app_mode_manager.h"
#include "app_mode.h"
#include "poweroff.h"
#include "app_config.h"
#include "app_music.h"
#include "app_default_msg_handler.h"
#include "bt_background.h"
#include "soundbox.h"

enum {
    SYS_POWERON_BY_KEY = 1,
    SYS_POWERON_BY_OUT_BOX,
};

enum {
    SYS_POWEROFF_BY_KEY = 1,
    SYS_POWEROFF_BY_IN_BOX,
    SYS_POWEROFF_BY_TIMEOUT,
};

enum {
    BT_MODE_SIGLE_BOX = 1,
    BT_MODE_TWS,
    BT_MODE_BROADCAST,
    BT_MODE_AURACAST,
    BT_MODE_CIG,
};

typedef struct _APP_VAR {
    u8 volume_def_state;
    s16 bt_volume;
    s16 dev_volume;
    s16 music_volume;
    s16 call_volume;
    s16 wtone_volume;
    s16 ktone_volume;
    s16 ring_volume;
    u8 opid_play_vol_sync;
    u8 aec_dac_gain;
    u8 aec_mic_gain;
    u8 aec_mic1_gain;
    u8 aec_mic2_gain;
    u8 aec_mic3_gain;
    u8 rf_power;
    u8 goto_poweroff_flag;
    u16 goto_poweroff_cnt;
    u8 poweroff_sametime_flag;
    u8 play_poweron_tone;
    u8 remote_dev_company;
    u8 siri_stu;
    u8 cycle_mode;
    u8 have_mass_storage;
    u8 poweron_reason;
    u8 poweroff_reason;
    int auto_stop_page_scan_timer;     //用于1拖2时，有一台连接上后，超过三分钟自动关闭Page Scan
    u16 auto_off_time;
    u16 warning_tone_v;
    u16 poweroff_tone_v;
    u16 phone_dly_discon_time;
    u8 usb_mic_gain;
    int wait_timer_do;
    u32 start_time;
    u8 pitch_mode;
    s16 mic_eff_volume;
} APP_VAR;

struct bt_mode_var {
    //phone
    u8 phone_ring_flag; //来电响铃标志
    u8 phone_num_flag;  //通话号码获取标志
    u8 phone_income_flag; //来电标志
    u8 phone_call_dec_begin; //来电esco解码标志
    u8 phone_ring_sync_tws; //tws来电铃声同步标志
    u8 phone_ring_addr[6]; //响铃的手机地址

    u8 inband_ringtone; //带内铃声,表示支不支持手机端铃声
    u8 phone_vol; //手机音量
    u16 phone_timer_id; //查询手机号码定时器
    u8 last_call_type;
    u8 income_phone_num[30]; //通话手机号码
    u8 income_phone_len; //通话手机号码长度
    s32 auto_connection_counter; //回链设备的总时间
    int auto_connection_timer; //回连定时器
    u8 auto_connection_addr[6]; //回连地址
    int tws_con_timer;
    u8 tws_start_con_cnt;
    u8 tws_conn_state; //tws连接状态
    bool search_tws_ing;
    int sniff_timer; //进入sniff模式定时器
    bool fast_test_mode;
    u16 exit_check_timer;
    u8 init_start; //蓝牙协议栈已经开始初始化标志位
    u8 init_ok; //蓝牙初始化完成标志
    u8 initializing; //蓝牙正在初始化标志
    u8 exiting; //蓝牙正在退出
    u8 wait_exit; //蓝牙等待退出
    u32 get_music_player_timer;
    u8 ignore_discon_tone;  // 1-退出蓝牙模式， 不响应discon提示音
    background_var background;  //蓝牙后台相关变量
    volatile u8 work_mode;
    volatile u8 last_work_mode;
    u8 bt_dual_conn_config;
};

typedef struct _BT_USER_COMM_VAR {
} BT_USER_COMM_VAR;

extern APP_VAR app_var;
extern struct bt_mode_var g_bt_hdl;

enum app_mode_t {
    APP_MODE_IDLE,
    APP_MODE_UPDATE,
    APP_MODE_POWERON,
    APP_MODE_BT,
    APP_MODE_MUSIC,
    APP_MODE_FM,
    APP_MODE_RECORD,
    APP_MODE_LINEIN,
    APP_MODE_RTC,
    APP_MODE_PC,
    APP_MODE_SPDIF,
    APP_MODE_IIS,
    APP_MODE_MIC,
    APP_MODE_SINK,
    APP_MODE_NULL,
};

enum app_mode_index {
    APP_MODE_BT_INDEX,
    APP_MODE_MUSIC_INDEX,
    APP_MODE_FM_INDEX,
    APP_MODE_RECORD_INDEX,
    APP_MODE_LINEIN_INDEX,
    APP_MODE_RTC_INDEX,
    APP_MODE_PC_INDEX,
    APP_MODE_SPDIF_INDEX,
    APP_MODE_IIS_INDEX,
};

#define earphone (&bt_user_priv_var)

void app_power_off(void *priv);
void bt_bredr_enter_dut_mode(u8 mode, u8 inquiry_scan_en);
void bt_bredr_exit_dut_mode();
u8 check_local_not_accept_sniff_by_remote();

struct app_mode *app_mode_switch_handler(int *msg);

#endif
