#ifndef _AUDIO_HOWLING_GATE_H_
#define _AUDIO_HOWLING_GATE_H_

#include "effects/HowlingGate_api.h"

struct howling_gate_update_parm {
    int acitive_energyThresold;  //0到-90， 超过这个能量阈值才会计算更新啸叫状态
    int ExSensitive_energyThresold;   //0到-90,超过这个能量阈值灵敏度提高
    int Release_energyThreshold;      //0到-90，超过这个能量阈值后才开始恢复
    int suphold_ms;      //0到10000，进入啸叫压制保持多少ms
    int attack_speed_ms;   //0到10000，启动啸叫处理后压制到目标的时间
    int release_speed_ms;   //0到10000，释放啸叫处理后恢复到目标的时间
    int sensitivity;     //0到14, 灵敏度越高，越容易触发啸叫
    int howling_gain;    //啸叫的时候目标音量，0到100， 百分比
    int howling_ms;    //0到20000，检测到啸叫后持续一段确认时长后才启动处理
    int overflowcheck_en;  //0到1
};

struct howling_gate_open_parm {
    HOWLING_GATE_PARM parm;
    u32 sample_rate;
    u8 ch_num;
    u8 bypass;
};

typedef struct _howling_gate_param_tool_set {
    int is_bypass;
    struct howling_gate_update_parm parm;
} howling_gate_param_tool_set;

typedef struct _howling_gate_hdl {
    HOWLING_GATE_FUNC_API *ops;
    void *workbuf;
    HOWLING_GATE_PARM parm;
    u32 sample_rate;
    u8 ch_num;
    u8 update;
    u8 status;
} howling_gate_hdl;

//打开
howling_gate_hdl *audio_howling_gate_open(struct howling_gate_open_parm *param);

//关闭
void audio_howling_gate_close(howling_gate_hdl *hdl);

//参数更新
void audio_howling_gate_update_parm(howling_gate_hdl *hdl, struct howling_gate_update_parm *parm);

//数据处理
int audio_howling_gate_run(howling_gate_hdl *hdl, s16 *indata, s16 *outdata, int len);

//bypass
void audio_howling_gate_bypass(howling_gate_hdl *hdl, u8 bypass);

#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif
