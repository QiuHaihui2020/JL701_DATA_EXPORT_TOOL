#ifndef reverb_api_h__
#define reverb_api_h__

#include "audio_reverb_common.h"

typedef struct __PLATE_REVERB_FUNC_API_ {
    unsigned int (*need_buf)(Plate_reverb_parm *preverb_parm);
    int (*open)(unsigned int *ptr, Plate_reverb_parm *preverb_parm, EF_REVERB0_FIX_PARM *echo_fix_parm);
    int (*init)(unsigned int *ptr, Plate_reverb_parm *preverb_parm);
    int (*run)(unsigned int *ptr, short *inbuf, short *outdata, int len);
} PLATE_REVERB0_FUNC_API;

extern PLATE_REVERB0_FUNC_API *get_plate_reverb_func_api();
extern PLATE_REVERB0_FUNC_API *get_plate_reverb_func_api_mask();
extern PLATE_REVERB0_FUNC_API *get_plate_reverb_adv_func_api();
extern PLATE_REVERB0_FUNC_API *get_plate_reverb_adv24_16_func_api();  //24bit的时候，湿声精度只有16bit的 ，buf比实际24bit少

#endif // reverb_api_h__
