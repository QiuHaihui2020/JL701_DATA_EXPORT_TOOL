#ifndef __RESAMPLE_API_H__
#define __RESAMPLE_API_H__

// #include "br18_dsp_Ecommon.h"
#include "effects/AudioEffect_DataType.h"

typedef struct _RS_IO_CONTEXT_ {
    void *priv;
    int(*output)(void *priv, void *data, int len);
} RS_IO_CONTEXT;

typedef struct _RS_PARA_STRUCT_ {
    unsigned int new_insample;
    unsigned int new_outsample;
    int nch;
    af_DataType dataTypeobj;
} RS_PARA_STRUCT;

typedef struct  _RS_STUCT_API_ {
    unsigned int(*need_buf)();
    void (*open)(unsigned int *ptr, RS_PARA_STRUCT *rs_para);
    void (*set_sr)(unsigned int *ptr, int newsrI);
    int(*run)(unsigned int *ptr, short *inbuf, int len, short *obuf);  //len是n个点，返回n个点
} RS_STUCT_API;

RS_STUCT_API *get_rsfast_context();//lib_resample_fast_cal.a里面的如果设置const  int RS_FAST_MODE_QUALITY = 8;  它的效果就跟lib_resample_cal完全一样的了
#define get_rs16_context  get_rsfast_context

#endif
