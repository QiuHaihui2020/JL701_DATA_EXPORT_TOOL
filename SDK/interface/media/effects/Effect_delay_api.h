#ifndef  _EFFECT_DEALY_API_H_
#define  _EFFECT_DEALY_API_H_

#include "AudioEffect_DataType.h"

typedef struct _delay_parm_context_ {
    float maxdelay_ms;
    float *delaydest_ms_array;
    int speedv;
    int quality;
    af_DataType dataTypeobj;
} delay_parm_context;


typedef struct __EFFECT_DELAY_FUNC_API_ {
    unsigned int (*need_buf)(int sr, int nch, delay_parm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, delay_parm_context *parm);
    unsigned int (*init)(unsigned int *ptr, delay_parm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);             // len是多少对点
} EFFECT_DELAY_FUNC_API;


extern EFFECT_DELAY_FUNC_API *get_modDelay_func_api();



#endif