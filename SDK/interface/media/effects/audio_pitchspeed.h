#ifndef _AUDIO_PITCHSPEED_API_H_
#define _AUDIO_PITCHSPEED_API_H_

#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"

#include "effects/ps_cal_api.h"

typedef struct _pitch_speed_tool_set {
    float pitch; //-12~12 半音阶
    float speed; //0.5~3  速度倍数
} pitch_speed_param_tool_set;

struct pitch_speed_update_parm {
    char name[16];
    u16 speedV;//>80变快,<80 变慢，建议范围36-130
    u16 pitchV;//32768 是原始音调  >32768是音调变高，<32768 音调变低，建议范围20000 - 50000
};

struct audio_pitchspeed_open_parm {
    PS69_CONTEXT_CONF param;
    int chn;
    int sr;
};

struct pitch_speed {
    PS69_API_CONTEXT *ops;
    void *workbuf;           	//运算buf指针
    struct audio_pitchspeed_open_parm param;
    u8 updata;
    u8 status;
};



struct pitch_speed *audio_pitchspeed_open(struct audio_pitchspeed_open_parm *param);
int audio_pitchspeed_run(struct pitch_speed *hdl, s16 *in_data, s16 *out_data, int len);
void audio_pitchspeed_close(struct pitch_speed *hdl);
void audio_pitchspeed_update_parm(struct pitch_speed *hdl, PS69_CONTEXT_CONF *param);

#endif //_AUDIO_PITCHSPEED_API_H_
