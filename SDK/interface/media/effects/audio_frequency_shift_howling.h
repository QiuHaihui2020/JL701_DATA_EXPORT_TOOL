#ifndef _PITCH_SHIFT_HOWLING_API_H
#define _PITCH_SHIFT_HOWLING_API_H

#include "effects/howling_pitchshifter_api.h"
typedef struct _HOWLING_PITCHSHIFT_PARM_ {
    s16 ps_parm;
    s16 fe_parm;
    s16 use_ps;
    s16 use_fs;
} _HOWLING_PITCHSHIFT_PARM;

typedef struct _HowlingPs_PARM_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    _HOWLING_PITCHSHIFT_PARM parm;
} howling_pitchshift_param_tool_set;



struct audio_frequency_shift_howling_parm {
    HOWLING_PITCHSHIFT_PARM pitch_parm;
    int sample_rate;
    u16 channel;
    u8 bypass;
};

struct audio_frequency_shift_howling {
    void *workbuf;
    HOWLING_PITCHSHIFT_FUNC_API *ops;
    struct audio_frequency_shift_howling_parm parm;
    u8 status;
    u8 update;
};
/*
 *移频啸叫抑制打开
 * */
struct audio_frequency_shift_howling *audio_frequency_shift_howling_open(struct audio_frequency_shift_howling_parm *para);
/*
 *移频啸叫抑制运行处理
 * */
int audio_frequency_shift_howling_run(struct audio_frequency_shift_howling *hdl, short *in, short *out, int len);
/*
 *移频啸叫抑制关闭
 * */
void audio_frequency_shift_howling_close(struct audio_frequency_shift_howling *hdl);
/*
 *移频啸叫抑制参数更新
 * */
void audio_frequency_shift_howling_update_parm(struct audio_frequency_shift_howling *hdl, HOWLING_PITCHSHIFT_PARM *parm);
/*
 *移频啸叫抑制直通设置
 * */
void audio_frequency_shift_howling_bypass(struct audio_frequency_shift_howling *hdl, u8 bypass);

#endif
