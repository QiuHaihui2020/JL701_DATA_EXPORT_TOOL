#ifndef __LOCAL_TWS_H__
#define __LOCAL_TWS_H__

#include "app_main.h"

#define TWS_FUNC_ID_LOCAL_TWS	TWS_FUNC_ID('L', 'O', 'T', 'S')

enum {
    LOCAL_TWS_ROLE_NULL,
    LOCAL_TWS_ROLE_SOURCE,
    LOCAL_TWS_ROLE_SINK,
};

enum {
    CMD_TWS_OPEN_LOCAL_DEC_REQ = 0x1,
    CMD_TWS_OPEN_LOCAL_DEC_RSP,
    CMD_TWS_CLOSE_LOCAL_DEC_REQ,
    CMD_TWS_CLOSE_LOCAL_DEC_RSP,
    CMD_TWS_ENTER_SINK_MODE_REQ,
    CMD_TWS_ENTER_SINK_MODE_RSP,
    CMD_TWS_BACK_TO_BT_MODE_REQ,
    CMD_TWS_BACK_TO_BT_MODE_RSP,
    CMD_TWS_CONNECT_MODE_REPORT,
    CMD_TWS_PLAYER_STATUS_REPORT,
    CMD_TWS_ENTER_NO_SOURCE_MODE_REPORT,
    CMD_TWS_VOL_UP,
    CMD_TWS_VOL_DOWN,
    CMD_TWS_VOL_REPORT,
    CMD_TWS_MUSIC_PP,
    CMD_TWS_MUSIC_NEXT,
    CMD_TWS_MUSIC_PREV,
};

typedef struct _local_tws_info {
    u8 role;
    u8 sync_goto_bt_mode;
    u8 cmd_record;                       //用于记录上一条命令
    volatile u32 cmd_timestamp;          //用于记录命令的时间戳
    const char *sync_tone_name;          //记录同步播放的提示音
    u32 timer;                           //用于音量同步
    void *priv;                          //回调给local_audio_open的参数
    volatile u8 remote_dec_status;       //对方是否开启local_tws_dec
} local_tws_info;

struct local_tws_mode_ops {
    enum app_mode_t name;
    void (*local_audio_open)(void *priv);
    bool (*get_play_status)(void);
};

#define REGISTER_LOCAL_TWS_OPS(local_tws_ops) \
    struct local_tws_mode_ops  __##local_tws_ops sec(.local_tws)

extern struct local_tws_mode_ops   local_tws_ops_begin[];
extern struct local_tws_mode_ops   local_tws_ops_end[];

#define list_for_each_local_tws_ops(ops) \
    for (ops = local_tws_ops_begin; ops < local_tws_ops_end; ops++)

void local_tws_init(void);
int local_tws_enter_mode(const char *file_name, void *priv);
void local_tws_exit_mode(void);
void local_tws_connect_mode_report(void);
void local_tws_disconnect_deal(void);
void local_tws_enter_sink_mode_rsp(enum app_mode_t mode);
void local_tws_close_dec_rsp(void);
u8 local_tws_get_remote_dec_status(void);
void local_tws_vol_operate(u8 operate);
void local_tws_music_operate(u8 operate, void *arg);
void local_tws_sync_vol(void);
u8 local_tws_get_role(void);

#endif

