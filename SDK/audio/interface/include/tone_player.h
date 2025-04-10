#ifndef TONE_PLAYER_H
#define TONE_PLAYER_H

#include "generic/typedef.h"
#include "jlstream.h"
#include "audio_decoder.h"
#include "sdk_config.h"

#define MAX_FILE_NUM 20


typedef int (*tone_player_cb_t)(void *, enum stream_event);

struct tone_player {
    struct list_head entry;
    u8 ref;
    u8 index;
    u8 stream_index;
    u8 player_id;
    u8 channel_mode;
    enum stream_scene scene;
    enum stream_coexist coexist;
    u16 fname_uuid;
    u32 coding_type;
    u32 sample_rate;

    struct jlstream *stream;

    void *file;
    void *next_file;
    const char *file_name_list[MAX_FILE_NUM + 1];

    void *priv;
    tone_player_cb_t callback;

#if FILE_DEC_REPEAT_EN
    struct fixphase_repair_obj repair_buf;
#endif
};


int tone_player_init(struct tone_player *player, const char *file_name);

int tone_player_add(struct tone_player *player);

void tone_player_free(struct tone_player *player);



/* ---------------------提示音播放API-----------------------------
 *
 * 提示音和其它音频之间默认采用叠加方式播放,可能由于资源限制而采用打断的方式播放
 * 提示音和提示音之间默认不抢占,采用排队方式播放
 *
 * ---------------------------------------------------------*/

/*
 * 播放单个提示音文件
 */
int play_tone_file(const char *file_name);


/*
 * 播放单个提示音文件,带有播放状态回调参数
 */
int play_tone_file_callback(const char *file_name, void *priv,
                            tone_player_cb_t callback);

/*
 * 打断方式播放单个提示音文件
 */
int play_tone_file_alone(const char *file_name);


int play_tone_file_alone_callback(const char *file_name, void *priv,
                                  tone_player_cb_t callback);


/*
 * 播放文件列表,内部不会复制文件名,file_name[x]指向的文件名地址要求播放过程中有效
 */
int play_tone_files(const char *const file_name[], u8 file_num);

int play_tone_files_alone(const char *const file_name[], u8 file_num);

int play_tone_files_callback(const char *const file_name[], u8 file_num,
                             void *priv, tone_player_cb_t callback);

int play_tone_files_alone_callback(const char *const file_name[], u8 file_num,
                                   void *priv, tone_player_cb_t callback);
/*
 * 停止播放所有提示音
 */
void tone_player_stop();

/*
 * 返回是否有提示音在播放
 */
int tone_player_runing();


/*
 * 播放按键音
 */
int play_key_tone_file(const char *file_name);

/*
 * 按键音播放器是否运行中
 */
u8 key_tone_player_running(void);

/*
 * 提示音文件名转uuid, 全局的提示音播放结束事件中会有次参数，可用于识别是哪一个提示音
 */
u16 tone_player_get_fname_uuid(const char *fname);


int common_dec_repeat_set(struct jlstream *stream, struct fixphase_repair_obj *repair_buf); //设置对应数据流无缝循环播放

#endif

