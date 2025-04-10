#ifndef  _STEREO_WINDEN_API_H_
#define  _STEREO_WINDEN_API_H_

#include "AudioEffect_DataType.h"

typedef struct _stewiden_parm_context_ {
    int widenval;      //Widen: 0 to 100
    int intensity;     //EhanceStere： 0 to 100
    af_DataType dataTypeobj;
} stewiden_parm_context;


typedef struct __STE_WIDEN_FUNC_API_ {
    unsigned int (*need_buf)(stewiden_parm_context *parm);
    unsigned int (*open)(unsigned int *ptr, int sr, int nch, stewiden_parm_context *parm);
    unsigned int (*init)(unsigned int *ptr, stewiden_parm_context *parm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);            // len是datapontsPerChannel
} STE_WIDEN_FUNC_API;


extern STE_WIDEN_FUNC_API *get_stewiden_func_api();

#endif
