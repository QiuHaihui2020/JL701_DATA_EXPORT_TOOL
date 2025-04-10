#ifndef _AUDIO_VOCAL_REMOVE_API_H_
#define _AUDIO_VOCAL_REMOVE_API_H_

#include "system/includes.h"
#include "media/includes.h"
#include "effects/audio_eq.h"

enum {
    BYPASS_OFF = 0,
    BYPASS_ON,
    FADE_BYPASS_OFF,
    FADE_BYPASS_ON,
};

typedef struct _VOCAL_REMOVE_PARM {
    u32 low_cut_en;
    u32 low_cut_freq;
    u32 high_cut_en;
    u32 high_cut_freq;
    u8 remove_ratio; //人声消除比例
} __attribute__((packed)) VOCAL_REMOVE_PARM;

typedef  struct _vocal_remover_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    VOCAL_REMOVE_PARM parm;
} __attribute__((packed)) vocal_remover_param_tool_set;

typedef struct _vocal_remove_open_parm {
    u8 channel;//输入音频声道数
    u8 bit_wide;//0:16bit 1:32bit
} vocal_remove_open_parm;


typedef struct _vocal_remove_hdl {
    u8 vocal_remove_dis;
    u8 vocal_remove_ratio;
    vocal_remove_open_parm o_parm;
} vocal_remove_hdl;
/*----------------------------------------------------------------------------*/
/**@brief   audio_vocal_remove_open  人声消除打开
   @param    *_parm: 始化参数，详见结构体vocal_remove_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
vocal_remove_hdl *audio_vocal_remove_open(vocal_remove_open_parm *_parm);

/*----------------------------------------------------------------------------*/
/**@brief    audio_vocal_remove_close 人声关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_remove_close(vocal_remove_hdl *_hdl);


/*----------------------------------------------------------------------------*/
/**@brief    audio_vocal_remove_sw 运行过程开关处理
   @param    _hdl:句柄
   @param    dis: 0：人声消除有效  1:人声消除无效
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_remove_sw(vocal_remove_hdl *_hdl, u32 dis);

int audio_vocal_remove_run(vocal_remove_hdl *_hdl, void *data, u32 len);

int audio_vocal_remove_ratio(vocal_remove_hdl *_hdl, u8 ratio);
int audio_vocal_remove_eq_run(struct audio_eq *vocal_remove_eq, s16 *in, s16 *out, u16 len, int is_bypass);
#endif

