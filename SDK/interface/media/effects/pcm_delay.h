#ifndef _PCM_DELAY_H_
#define _PCM_DELAY_H_

#include "effects/Effect_delay_api.h"
#include "media/audio_stream.h"

struct pcm_delay_update_parm {
    int link;
    int speedv;
    int quality;
    float ch0_delay; //ms
    float ch1_delay;
    float ch2_delay;
    float ch3_delay;
    float max_delay;
};

struct pcm_delay_open_parm {
    delay_parm_context parm;
    u32 sample_rate;
    u8 ch_num;
    u8 bypass;
};
typedef struct _PCM_DELAY_TOOL_SET {
    int is_bypass;
    int channel_num;
    struct pcm_delay_update_parm parm;
} pcm_delay_param_tool_set;

typedef struct _pcm_delay_hdl {
    EFFECT_DELAY_FUNC_API *ops;
    void *workbuf;
    delay_parm_context parm;
    u32 sample_rate;
    float ch_delay[4];
    u8 ch_num;
    u8 update;
    u8 status;
} pcm_delay_hdl;

//打开
pcm_delay_hdl *audio_pcm_delay_open(struct pcm_delay_open_parm *param);

//关闭
void audio_pcm_delay_close(pcm_delay_hdl *hdl);

//参数更新
void audio_pcm_delay_update_parm(pcm_delay_hdl *hdl, struct pcm_delay_update_parm *parm);

//数据处理
int audio_pcm_delay_run(pcm_delay_hdl *hdl, s16 *indata, s16 *outdata, int len);

//bypass
void audio_pcm_delay_bypass(pcm_delay_hdl *hdl, u8 bypass);

#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif
