#ifndef __AUDIO_NOISEGATE_API_H
#define __AUDIO_NOISEGATE_API_H

#include "media/audio_stream.h"
#include "effects/noisegate_api.h"


typedef struct __NoiseGate_update_Param {
    int attackTime;  //启动时间
    int releaseTime; //释放时间
    float threshold;	  //阈值 界面值
    float low_th_gain;	//低于阈值增益 界面值
} _noisegate_update_param;


typedef struct _NoiseGateParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    _noisegate_update_param parm;
} noisegate_param_tool_set;

struct noisegate_open_parm {
    NoiseGateParam 	parm;  //参数
    u8 bypass;
};

typedef struct _NOISEGATE_API_STRUCT_ {
    void				*workbuf;    //运算buf指针
    NoiseGateParam 	parm;  //参数
    u8 status;
    u8 update;
} NOISEGATE_API_STRUCT;


NOISEGATE_API_STRUCT *audio_noisegate_open(struct noisegate_open_parm *noisegate_para);
void audio_noisegate_close(NOISEGATE_API_STRUCT *hdl);
void audio_noisegate_update_parm(NOISEGATE_API_STRUCT *hdl, noisegate_update_param *parm);
void audio_noisegate_bypass(NOISEGATE_API_STRUCT *hdl, u8 bypass);
int audio_noisegate_run(NOISEGATE_API_STRUCT *hdl, s16 *data, u16 len);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif
#endif

