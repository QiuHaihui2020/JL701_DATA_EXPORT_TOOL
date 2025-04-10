#ifndef CONFIG_APP_MUSIC_H
#define CONFIG_APP_MUSIC_H

#include "system/includes.h"
#include "app_main.h"
#include "music/breakpoint.h"
#include "effect/effects_default_param.h"

#if (TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0)
#define MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN			0//如果是u盘和SD卡复用， 这里必须为0， 保证usb枚举的时候解码是停止的
#else
#define MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN			1
#endif

///模式参数结构体
struct __music_task_parm {
    u8 type;
    int val;
};

///music模式控制结构体
struct __music {
    struct __music_task_parm task_parm;
    u16 file_err_counter;//错误文件统计
    u8 file_play_direct;//0:下一曲， 1：上一曲
    u8 scandisk_break;//扫描设备打断标志
    char device_tone_dev[16];
    struct __breakpoint *breakpoint;
    struct music_player *player_hd;
    enum _speed_level speed_mode;
    enum _pitch_level pitch_mode;
    u8 music_busy;
    u8 scandisk_mark;//扫描设备标志
    u16 timer;
    //固定为接收端时，打开广播接收后，如果连接上了会关闭本地的音频，当关闭广播后，需要恢复本地的音频播放
    u8 close_broadcast_need_resume_local_music_flag;
    //下面这个变量为了解决问题：固定为接收端，暂停中打开广播再关闭，本地音乐需是暂停状态
    u8 music_local_audio_resume_onoff;
};
enum {
    MUSIC_TASK_START_BY_NORMAL = 0x0,
    MUSIC_TASK_START_BY_BREAKPOINT,
    MUSIC_TASK_START_BY_SCLUST,
    MUSIC_TASK_START_BY_NUMBER,
    MUSIC_TASK_START_BY_PATH,
};

void music_task_dev_online_start(char *in_logo);
void music_task_set_parm(u8 type, int val);
void music_player_err_deal(int err);
//获取music app当前播放的设备
char *music_app_get_dev_cur(void);

/*获取music app当前的断点地址*/
struct audio_dec_breakpoint *music_app_get_dbp_addr(void);

u8 get_music_le_audio_flag(void);
int music_app_msg_handler(int *msg);
struct app_mode *app_enter_music_mode(int arg);
struct music_player *music_app_get_cur_hdl(void);
u8 music_app_set_btaddr(void *le_audio, struct stream_enc_fmt *fmt);
int music_device_msg_handler(int *msg);

const char *get_music_tone_name_by_logo(const char *logo);

extern int music_device_tone_play(char *logo);

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==2))
//当固定为接收端时，其它模式下开广播切进music模式，关闭广播后music模式不会自动播放
extern void music_set_broadcast_local_open_flag(u8 en);
#endif

#endif

