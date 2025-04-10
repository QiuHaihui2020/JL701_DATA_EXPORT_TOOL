#ifndef APP_MSG_H
#define APP_MSG_H


#include "os/os_api.h"

enum {
    MSG_FROM_KEY =  Q_MSG + 1,

    MSG_FROM_TWS,
    MSG_FROM_BT_STACK,
    MSG_FROM_BT_HCI,

    MSG_FROM_EARTCH,

    MSG_FROM_BATTERY,
    MSG_FROM_CHARGE_STORE,
    MSG_FROM_TESTBOX,
    MSG_FROM_ANCBOX,

    MSG_FROM_PWM_LED,
    MSG_FROM_TONE,

    MSG_FROM_APP,

    MSG_FROM_AUDIO,

    MSG_FROM_CI_UART,
    MSG_FROM_CDC,
    MSG_FROM_CFGTOOL_TWS_SYNC,

    MSG_FROM_DEVICE,

    MSG_FROM_RCSP,
    MSG_FROM_RCSP_BT,
    MSG_FROM_TWS_UPDATE_NEW,

    MSG_FROM_LOCAL_TWS,

    MSG_FROM_BIG,
    MSG_FROM_CIG,

    MSG_FROM_RTC,
};


struct app_msg_handler {
    int owner;
    int from;
    int (*handler)(int *msg);
};


#define APP_MSG_PROB_HANDLER(msg_handler) \
    const struct app_msg_handler msg_handler sec(.app_msg_prob_handler)

extern const struct app_msg_handler app_msg_prob_handler_begin[];
extern const struct app_msg_handler app_msg_prob_handler_end[];

#define for_each_app_msg_prob_handler(p) \
    for (p = app_msg_prob_handler_begin; p < app_msg_prob_handler_end; p++)


#define APP_MSG_HANDLER(msg_handler) \
    const struct app_msg_handler msg_handler sec(.app_msg_handler)

extern const struct app_msg_handler app_msg_handler_begin[];
extern const struct app_msg_handler app_msg_handler_end[];

#define for_each_app_msg_handler(p) \
    for (p = app_msg_handler_begin; p < app_msg_handler_end; p++)



#define APP_KEY_MSG_FROM_TWS    1
#define APP_KEY_MSG_FROM_CIS    2

#define APP_MSG_KEY    0x010000

#define APP_MSG_FROM_KEY(msg)   (msg & APP_MSG_KEY)

#define APP_MSG_KEY_VALUE(msg)  ((msg >> 8) & 0xff)

#define APP_MSG_KEY_ACTION(msg)  (msg & 0xff)


#define APP_KEY_MSG_REMAP(key_value, key_action) \
            (APP_MSG_KEY | (key_value << 8) | key_action)


enum {
    APP_MSG_NULL = 0,
    APP_MSG_KEY_POWER_ON,
    APP_MSG_KEY_POWER_ON_HOLD,
    APP_MSG_KEY_POWER_OFF,
    APP_MSG_KEY_POWER_OFF_HOLD,
    APP_MSG_KEY_POWER_OFF_RELEASE,
    APP_MSG_KEY_POWER_OFF_INSTANTLY,

    APP_MSG_WRITE_RESFILE_START,
    APP_MSG_WRITE_RESFILE_STOP,

    APP_MSG_MUSIC_PP,
    APP_MSG_MUSIC_PREV,
    APP_MSG_MUSIC_NEXT,
    APP_MSG_MUSIC_CHANGE_DEV,
    APP_MSG_MUSIC_AUTO_NEXT_DEV,
    APP_MSG_MUSIC_PLAYE_NEXT_FOLDER,
    APP_MSG_MUSIC_PLAYE_PREV_FOLDER,
    APP_MSG_MUSIC_PLAY_FIRST,
    APP_MSG_MUSIC_PLAY_REC_FOLDER_SWITCH,		//播放录音文件
    APP_MSG_MUSIC_DELETE_CUR_FILE,				//删除当前文件
    APP_MSG_MUSIC_MOUNT_PLAY_START,
    APP_MSG_MUSIC_PLAY_SUCCESS,
    APP_MSG_MUSIC_PLAY_START,
    APP_MSG_MUSIC_PLAY_START_BY_SCLUST,
    APP_MSG_MUSIC_PLAY_START_BY_DEV,
    APP_MSG_MUSIC_FR,
    APP_MSG_MUSIC_FF,
    APP_MSG_MUSIC_DEC_ERR,
    APP_MSG_MUSIC_PLAYER_END,
    APP_MSG_MUSIC_CHANGE_REPEAT,
    APP_MSG_MUSIC_SPEED_UP,
    APP_MSG_MUSIC_SPEED_DOWN,
    APP_MSG_MUSIC_PLAYER_AB_REPEAT_SWITCH,
    APP_MSG_MUSIC_MUTE,
    APP_MSG_MUSIC_CHANGE_EQ,
    APP_MSG_MUSIC_PLAY_BY_NUM,
    APP_MSG_MUSIC_PLAY_START_AT_DEST_TIME,
    APP_MSG_CALL_HANGUP,
    APP_MSG_CALL_LAST_NO,
    APP_MSG_CALL_THREE_WAY_ANSWER1,
    APP_MSG_CALL_THREE_WAY_ANSWER2,
    APP_MSG_CALL_SWITCH,
    APP_MSG_HID_CONTROL,
    APP_MSG_OPEN_SIRI,
    APP_MSG_VOL_UP,
    APP_MSG_VOL_DOWN,
    APP_MSG_MAX_VOL,
    APP_MSG_MIN_VOL,
    APP_MSG_LOW_LANTECY,
    APP_MSG_ADD_LINEIN_STREAM,		//叠加linein数据流
    APP_MSG_POWER_KEY_LONG,
    APP_MSG_POWER_KEY_HOLD,
    APP_MSG_SYS_MUTE,
    APP_MSG_CEC_MUTE,
    APP_MSG_CEC_VOL_UP,
    APP_MSG_CEC_VOL_DOWN,
    APP_MSG_PITCH_UP,
    APP_MSG_PITCH_DOWN,

    APP_MSG_SYS_TIMER,

    APP_MSG_POWER_ON,
    APP_MSG_POWER_OFF,
    APP_MSG_GOTO_MODE,
    APP_MSG_GOTO_NEXT_MODE,
    APP_MSG_ENTER_MODE,
    APP_MSG_EXIT_MODE,

    APP_MSG_BT_GET_CONNECT_ADDR,
    APP_MSG_BT_OPEN_PAGE_SCAN,
    APP_MSG_BT_CLOSE_PAGE_SCAN,
    APP_MSG_BT_ENTER_SNIFF,
    APP_MSG_BT_EXIT_SNIFF,
    APP_MSG_BT_A2DP_PAUSE,
    APP_MSG_BT_A2DP_PLAY,
    APP_MSG_BT_A2DP_START,
    APP_MSG_BT_PAGE_DEVICE,
    APP_MSG_BT_IN_PAIRING_MODE,

    APP_MSG_TWS_PAIRED,
    APP_MSG_TWS_UNPAIRED,
    APP_MSG_TWS_PAIR_SUSS,
    APP_MSG_TWS_CONNECTED,
    APP_MSG_TWS_WAIT_PAIR,                      // 等待配对
    APP_MSG_TWS_START_PAIR,                     // 手动发起配对
    APP_MSG_TWS_START_CONN,                     // 开始回连TWS
    APP_MSG_TWS_REMOVE_PAIR,                    // 取消配对
    APP_MSG_TWS_START_REMOVE_PAIR,              // 如果没有配对则发起配对，如果配对了则取消配对
    APP_MSG_TWS_WAIT_CONN,                      // 等待TWS连接
    APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS,    // 执行配对和连接的默认流程
    APP_MSG_TWS_POWERON_PAIR_TIMEOUT,           // TWS开机配对超时
    APP_MSG_TWS_POWERON_CONN_TIMEOUT,           // TWS开机回连超时
    APP_MSG_TWS_START_PAIR_TIMEOUT,
    APP_MSG_TWS_START_CONN_TIMEOUT,

    APP_MSG_FM_SCAN_ALL,
    APP_MSG_FM_SCAN_ALL_DOWN,
    APP_MSG_FM_SCAN_ALL_UP,
    APP_MSG_FM_SCAN_DOWN,
    APP_MSG_FM_SCAN_UP,
    APP_MSG_FM_PREV_STATION,
    APP_MSG_FM_NEXT_STATION,
    APP_MSG_FM_PREV_FREQ,
    APP_MSG_FM_NEXT_FREQ,

    APP_MSG_LINEIN_START,
    APP_MSG_IIS_START,
    APP_MSG_SPDIF_START,
    APP_MSG_SPDIF_SWITCH_SOURCE,
    APP_MSG_SPDIF_SET_SOURCE,
    APP_MSG_SPDIF_SOURCE_UPDATE,
    APP_MSG_SPDIF_STATUS_UPDATE,
    APP_MSG_RTC_UP,
    APP_MSG_RTC_DOWN,
    APP_MSG_RTC_SW,
    APP_MSG_RTC_SW_POS,

    APP_MSG_REC_PP,
    APP_MSG_ENC_START,
    APP_MSG_REVERB_OPNE,
    APP_MSG_MIC_VOL_UP,
    APP_MSG_MIC_VOL_DOWN,
    APP_MSG_MIC_BASS_UP,
    APP_MSG_MIC_BASS_DOWN,
    APP_MSG_MIC_TREBLE_UP,
    APP_MSG_MIC_TREBLE_DOWN,

    APP_MSG_CHANGE_MODE,
    APP_MSG_SWITCH_SOUND_EFFECT,
    APP_MSG_MIC_EFFECT_ON_OFF,
    APP_MSG_SWITCH_MIC_EFFECT,
    APP_MSG_VOCAL_REMOVE,
    //IR_NUM中间不允许插入msg
    APP_MSG_IR_NUM0,
    APP_MSG_IR_NUM1,
    APP_MSG_IR_NUM2,
    APP_MSG_IR_NUM3,
    APP_MSG_IR_NUM4,
    APP_MSG_IR_NUM5,
    APP_MSG_IR_NUM6,
    APP_MSG_IR_NUM7,
    APP_MSG_IR_NUM8,
    APP_MSG_IR_NUM9,
    //IR_NUM中间不允许插入msg

    APP_MSG_VOL_CHANGED,
    APP_MSG_MUSIC_FILE_NUM_CHANGED,
    APP_MSG_REPEAT_MODE_CHANGED,
    APP_MSG_FM_INIT_OK,
    APP_MSG_FM_REFLASH,
    APP_MSG_FM_STATION,
    APP_MSG_MUSIC_PLAY_STATUS,
    APP_MSG_LINEIN_PLAY_STATUS,
    APP_MSG_INPUT_FILE_NUM,
    APP_MSG_RTC_SET,
    APP_MSG_EQ_CHANGED,
    APP_MSG_MUTE_CHANGED,

    APP_MSG_LCD_OK,
    APP_MSG_LCD_MENU,
    APP_MSG_LCD_UP,
    APP_MSG_LCD_DOWN,
    APP_MSG_LCD_VOL_INC,
    APP_MSG_LCD_VOL_DEC,
    APP_MSG_LCD_MODE,
    APP_MSG_LCD_POWER,
    APP_MSG_LCD_POWER_START,

    APP_MSG_SMART_VOICE_EVENT,

    APP_MSG_TWS_OPEN_LOCAL_DEC,
    APP_MSG_TWS_CLOSE_LOCAL_DEC,

    APP_MSG_LE_BROADCAST_SW,
    APP_MSG_LE_CONNECTED_SW,
    APP_MSG_LE_AURACAST_SW_DEIVCE,
    APP_MSG_LE_AUDIO_ENTER_PAIR,
    APP_MSG_LE_AUDIO_EXIT_PAIR,
    APP_MSG_PC_START,

    APP_MSG_PC_AUDIO_PLAY_OPEN,
    APP_MSG_PC_AUDIO_PLAY_CLOSE,
    APP_MSG_PC_AUDIO_MIC_OPEN,
    APP_MSG_PC_AUDIO_MIC_CLOSE,

    APP_MSG_BT_WORK_MODE_CHANGE,

    APP_MSG_SW_WIRED_MIC_OR_WIRELESS_MIC,
    APP_MSG_WIRELESS_MIC_OPEN,
    APP_MSG_WIRELESS_MIC_CLOSE,
    APP_MSG_WIRELESS_MIC0_VOL_UP,
    APP_MSG_WIRELESS_MIC0_VOL_DOWN,
    APP_MSG_WIRELESS_MIC1_VOL_UP,
    APP_MSG_WIRELESS_MIC1_VOL_DOWN,
};


struct key_remap_table {
    u8 key_value;
    const int *remap_table;
    int (*remap_func)(int *event);
};

void app_send_message(int msg, int arg);

void app_send_message2(int _msg, int arg1, int arg2);

void app_send_message_from(int from, int argc, int *msg);

int app_key_event_remap(const struct key_remap_table *table, int *_event);

int app_get_message(int *msg, int max_num, const struct key_remap_table *key_table);


#endif

