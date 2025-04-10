#ifndef _AUDIO_STEREO_widener_API_H_
#define _AUDIO_STEREO_widener_API_H_
#include "system/includes.h"
#include "media/audio_stream.h"
#include "effects/stereo_widen_api.h"

typedef struct _StereowidenerUdateParam {//与stewiden_parm_context关联
    int widenerval;      //widener: 0 to 100
    int intensity;     //EhanceStere： 0 to 100
} StereoWidenerUdateParam;


typedef struct _Stereowidener_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    StereoWidenerUdateParam parm;
} stereo_widener_param_tool_set;

struct stereo_widener_param {
    stewiden_parm_context param;
    u32 samplerate;
    u32 ch_num;
    u8 bypass;
};

//立体声增强
struct audio_stereo_widener {
    void *workbuf;           //stereo_widener 运行句柄及buf
    STE_WIDEN_FUNC_API *ops;
    struct stereo_widener_param parm;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};


int audio_stereo_widener_run(struct audio_stereo_widener *hdl, void *datain, void *dataout, u32 len);
struct audio_stereo_widener *audio_stereo_widener_open(struct stereo_widener_param *parm);
int audio_stereo_widener_close(struct audio_stereo_widener *hdl);
void audio_stereo_widener_set_bit_wide(struct audio_stereo_widener *hdl, af_DataType dataTypeobj);

int audio_stereo_widener_update_parm(struct audio_stereo_widener *hdl, StereoWidenerUdateParam *parm);
void audio_stereo_widener_bypass(struct audio_stereo_widener *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

