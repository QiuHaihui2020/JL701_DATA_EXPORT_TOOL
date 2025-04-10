#ifndef _AUDIO_chorus_API_H_
#define _AUDIO_chorus_API_H_
#include "system/includes.h"
#include "media/audio_stream.h"
#include "effects/chorus_api.h"

typedef struct _ChorusUdateParam {//与CHORUS_PARM关联
    int wetgain;               //0-100
    int drygain;
    int mod_depth;
    int mod_rate;
    int feedback;
    int delay_length;
} ChorusUdateParam;


typedef struct _Chorus_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    ChorusUdateParam parm;
} chorus_param_tool_set;

struct chorus_param {
    CHORUS_PARM param;
    u32 samplerate;
    u32 ch_num;
    u8 bypass;
};

struct audio_chorus {
    void *workbuf;           //chorus 运行句柄及buf
    CHORUS_FUNC_API *ops;
    struct chorus_param parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};


int audio_chorus_run(struct audio_chorus *hdl, void *datain, void *dataout, u32 len);
struct audio_chorus *audio_chorus_open(struct chorus_param *parm);
int audio_chorus_close(struct audio_chorus *hdl);
void audio_chorus_set_bit_wide(struct audio_chorus *hdl, af_DataType dataTypeobj);

int audio_chorus_update_parm(struct audio_chorus *hdl, ChorusUdateParam *parm);
void audio_chorus_bypass(struct audio_chorus *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

