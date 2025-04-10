#ifndef __AUDIO_NOTCH_HOWLING_API_H
#define __AUDIO_NOTCH_HOWLING_API_H

#include "notch_howling_api.h"

//啸叫抑制 NotchHowling:
typedef struct _NotchHowlingUpdateParam {
    float Q;    	//Q值
    float gain;		//增益
    int fade_n;		//启动释放时间
    float threshold;
} NotchHowlingUpdateParam;

typedef struct _NotchHowlingParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    NotchHowlingUpdateParam parm;
} notch_howling_param_tool_set;


struct audio_notch_howling_parm {
    NotchHowlingParam notch_parm;
    u16 channel;
    u8 bypass;
};

struct audio_notch_howling {
    void *workbuf;
    struct audio_notch_howling_parm parm;
    u8 status;
    u8 update;
};


/*
 *陷波啸叫抑制打开
 * */
struct audio_notch_howling *audio_notch_howling_open(struct audio_notch_howling_parm *para);
/*
 *陷波啸叫抑制运行处理
 * */
int audio_notch_howling_run(struct audio_notch_howling *hdl, short *in, short *out, int len);
/*
 *陷波啸叫抑制关闭
 * */
void audio_notch_howling_close(struct audio_notch_howling *hdl);
/*
 *陷波啸叫抑制参数更新
 * */
void audio_notch_howling_update_parm(struct audio_notch_howling *hdl, NotchHowlingUpdateParam *parm);
/*
 *陷波啸叫抑制直通设置
 * */
void audio_notch_howling_bypass(struct audio_notch_howling *hdl, u8 bypass);

#endif
