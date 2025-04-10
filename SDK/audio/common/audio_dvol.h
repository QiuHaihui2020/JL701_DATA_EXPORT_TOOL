#ifndef _AUDIO_DVOL_H_
#define _AUDIO_DVOL_H_

#include "generic/typedef.h"
#include "os/os_type.h"
#include "os/os_api.h"
#include "generic/list.h"
#include "asm/math_fast_function.h"

#define DVOL_RESOLUTION		14
#define DVOL_MAX			16384
#define DVOL_MAX_FLOAT		16384.0f

#define BG_DVOL_FADE_ENABLE		0	/*多路声音叠加，背景声音自动淡出小声*/

#define DIGITAL_VOLUME_LEVEL_MAX  200  //默认的音量等级限制设成200

/*Digital Volume Channel*/
#define MUSIC_DVOL		    0b00001
#define CALL_DVOL			0b00010
#define TONE_DVOL			0b00100
#define RING_DVOL			0b01000
#define KEY_TONE_DVOL		0b10000

/*Digital Volume Fade Step*/
#define MUSIC_DVOL_FS		2
#define CALL_DVOL_FS		4
#define TONE_DVOL_FS		30

/*Digital Volume Max Level*/
#define MUSIC_DVOL_MAX		16
#define CALL_DVOL_MAX		15
#define TONE_DVOL_MAX		16

struct audio_vol_params {
    u16 vol;
    u16 vol_max;
    u16 fade_step;
    s16 vol_limit;
    u8 bit_wide;
};

typedef struct {
    u8 toggle;					/*数字音量开关*/
    u8 fade;					/*淡入淡出标志*/
    u16 vol;						/*淡入淡出当前音量(level)*/
    u16 vol_max;					/*淡入淡出最大音量(level)*/
    s16 vol_limit;				/*最大数字音量限制*/
    s16 vol_fade;				/*淡入淡出对应的起始音量*/
#if BG_DVOL_FADE_ENABLE
    s16 vol_bk;					/*后台自动淡出前音量值*/
    struct list_head entry;
#endif/*BG_DVOL_FADE_ENABLE*/
    volatile s16 vol_target;	/*淡入淡出对应的目标音量*/
    volatile u16 fade_step;		/*淡入淡出的步进*/
    float  cfg_vol_min;         /*最小音量的分贝数*/
    float  cfg_vol_max;         /*最小音量的分贝数*/
    u16    cfg_level_max;       /*最大音量等级*/
    u8     vol_table_custom;    /*是否使用外部工具读取的音量表*/
    u8     vol_table_default;    /*是否使用默认的音量表*/
    u8     mute_en;              /*是否将数据设成0*/
    u8     bit_wide;            /*数据位宽*/
    float   *vol_table;            /*自定义音量表*/
} dvol_handle;

int audio_digital_vol_init(u16 *vol_table, u16 vol_max);
void audio_digital_vol_bg_fade(u8 fade_out);
dvol_handle *audio_digital_vol_open(struct audio_vol_params params);
void audio_digital_vol_close(dvol_handle  *dvol);
void audio_digital_vol_set(dvol_handle *dvol, u16 vol);
void audio_digital_vol_mute_set(dvol_handle *dvol, u8 mute_en);
int audio_digital_vol_run(dvol_handle *dvol, void *data, u32 len);
void audio_digital_vol_reset_fade(dvol_handle *dvol);

#endif/*_AUDIO_DVOL_H_*/
