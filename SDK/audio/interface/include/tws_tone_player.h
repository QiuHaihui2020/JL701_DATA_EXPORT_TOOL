#ifndef TWS_TONE_PLAYER_H
#define TWS_TONE_PLAYER_H

#include "tone_player.h"
#include "ring_player.h"


typedef void (*tws_tone_cb_func_t)(int priv, enum stream_event event);

struct tws_tone_callback {
    int func_uuid;
    tws_tone_cb_func_t callback;
    const char *task_name;
};

extern const struct tws_tone_callback  tws_tone_cb_begin[];
extern const struct tws_tone_callback  tws_tone_cb_end[];

/*
 * 注册TWS回调函数, 播放结束时主从都会调用
 */
#define REGISTER_TWS_TONE_CALLBACK(cb_stub) \
    static const struct tws_tone_callback __tws_tone_##cb_stub sec(.tws_tone_callback)



/*---------------TWS同步播放提示音接口-----------------------
 *
 * delay_msec: 表示多少msec以后tws同时开始出声音
 * 由于TWS每次通信有一定的时间间隔,环境无线干扰可能导致通信失败
 * 建议delay_msec设置在300ms以上,以增加抗干扰的能力
 *
 * ---------------------------------------------------------*/

/*
 * 默认叠加方式同步播放单个文件, 可能由于受资源限制而采样打断的方式播放
 */
int tws_play_tone_file(const char *file_name, int delay_msec);

int tws_play_tone_file_callback(const char *file_name, int delay_msec, u32 func_uuid);


/*
 * 打断方式同步播放单个文件
 */
int tws_play_tone_file_alone(const char *file_name, int delay_msec);

int tws_play_tone_file_alone_callback(const char *file_name, int delay_msec, u32 func_uuid);

/*
 * 同步播放文件列表
 */
int tws_play_tone_files(const char *const file_name[], u8 file_num, int delay_msec);

int tws_play_tone_files_callback(const char *const file_name[], u8 file_num,
                                 int delay_msec, u32 func_uuid);

int tws_play_tone_files_alone_callback(const char *const file_name[], u8 file_num,
                                       int delay_msec, u32 func_uuid);
/*
 * 同步播放铃声
 */
int tws_play_ring_file(const char *file_name, int delay_msec);

int tws_play_ring_file_callback(const char *file_name, int delay_msec, u32 func_uuid);

int tws_play_ring_file_alone(const char *file_name, int delay_msec);

int tws_play_ring_file_alone_callback(const char *file_name, int delay_msec, u32 func_uuid);



#endif

