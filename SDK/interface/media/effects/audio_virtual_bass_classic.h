#ifndef _AUDIO_virtual_bass_classic_API_H_
#define _AUDIO_virtual_bass_classic_API_H_
#include "system/includes.h"
#include "media/audio_stream.h"
#include "effects/virtual_bass_classic_api.h"

typedef struct _VirtualBassClassicUpateParam {
    int attackTime;
    int releaseTime;
    int attack_hold_time;
    float threshold;
    int resever[10];
} virtual_bass_classic_update_param;

//虚拟低音
typedef struct _VirtualBassClassic_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    virtual_bass_classic_update_param parm;
} virtual_bass_classic_param_tool_set;

struct virtual_bass_classic_open_param {
    struct virtual_bass_class_param parm;
    u8 bypass;
};
typedef struct _virtual_bass_classic_hdl {
    void *workbuf;           //virtual_bass_classic 运行句柄及buf
    struct virtual_bass_class_param parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
    u8 out32bit;
} virtual_bass_classic_hdl;


int audio_virtual_bass_classic_run(virtual_bass_classic_hdl *hdl, void *datain, void *dataout, u32 len);
virtual_bass_classic_hdl *audio_virtual_bass_classic_open(struct virtual_bass_classic_open_param *parm);
int audio_virtual_bass_classic_close(virtual_bass_classic_hdl *hdl);
void audio_virtual_bass_classic_set_bit_wide(virtual_bass_classic_hdl *hdl, u32 out32bit);

int audio_virtual_bass_classic_update_parm(virtual_bass_classic_hdl *hdl, virtual_bass_classic_update_param *parm);
void audio_virtual_bass_classic_bypass(virtual_bass_classic_hdl *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

