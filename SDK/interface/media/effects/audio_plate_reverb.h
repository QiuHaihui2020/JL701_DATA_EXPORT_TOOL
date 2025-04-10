


#ifndef _audio_reverb_h__
#define _audio_reverb_h__

#include "effects/reverb_api.h"

struct plate_reverb_update_parm { /*与Plate_reverb_parm结构关联*/
    int wet;                      //0-300%
    int dry;                      //0-200%
    int pre_delay;                 //0-40ms
    int highcutoff;                //0-20k 高频截止
    int diffusion;                  //0-100%
    int decayfactor;                //0-100%
    int highfrequencydamping;       //0-100%
    int modulate;                  // 0或1
    int roomsize;                   //20%-100%
    int wet_bit_wide;             //0：16bit 1:24bit  24bit使用多70k ram
};

typedef  struct  _Plate_reverb_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct plate_reverb_update_parm parm;
} plate_reverb_param_tool_set;

struct audio_plate_reverb_parm {
    Plate_reverb_parm plate_parm;
    EF_REVERB0_FIX_PARM fixparm;
    u16 advance;
    u16 wet_bit_wide;
    u8 bypass;
};


struct audio_plate_reverb {
    void *workbuf;
    PLATE_REVERB0_FUNC_API *ops;           //函数指针
    struct audio_plate_reverb_parm parm;
    u8 status;
    u8 update;
};



/*
 * 打开plate声卡混响模块
 */
struct audio_plate_reverb *audio_plate_reverb_open(struct audio_plate_reverb_parm *parm);
/*
 * plate声卡混响处理
 */
int audio_plate_reverb_run(struct audio_plate_reverb *hdl, short *in, short *out, int len);
/*
 * 关闭plate声卡混响模块
 */
void  audio_plate_reverb_close(struct audio_plate_reverb *hdl);

/*
 *  plate声卡混响参数更新
 */
void audio_plate_reverb_update_parm(struct audio_plate_reverb *hdl, struct plate_reverb_update_parm *parm);

void audio_plate_reverb_bypass(struct audio_plate_reverb *hdl, u8 bypass);


#endif // reverb_echo_h__
