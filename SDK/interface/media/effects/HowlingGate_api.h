#ifndef _HOWLING_GATE_API_H_
#define _HOWLING_GATE_API_H_

#include "AudioEffect_DataType.h"


enum {
    HOWLINGGATE_STATUS_NORM = 0,
    HOWLINGGATE_STATUS_COMFIRMING,
    HOWLINGGATE_STATUS_GATED,
    HOWLINGGATE_STATUS_GATEKEEPING
};

#ifndef HAVE_TYPE_F
#define HAVE_TYPE_F    1
#endif

typedef  struct _HOWLING_GATE_PARM_ {
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
    int reserved0;
    int reserved1;
    af_DataType dataTypeobj;
} HOWLING_GATE_PARM;


typedef struct _HOWLING_GATE_FUNC_API_ {
    unsigned int (*need_buf)(HOWLING_GATE_PARM *parm_obj);
    int (*open)(void *ptr, unsigned int sr, unsigned int nchin, unsigned int  nchout, HOWLING_GATE_PARM *parm_obj);
    void (*run)(void *ptr, short *detect_data, short *vol_data, short *outdata, int len);    //len是多少个点
    void (*get_statue)(void *ptr, int *statuesNow, int *statuesKeep);
} HOWLING_GATE_FUNC_API;

extern HOWLING_GATE_FUNC_API *get_howling_gate_func_api();


#endif
