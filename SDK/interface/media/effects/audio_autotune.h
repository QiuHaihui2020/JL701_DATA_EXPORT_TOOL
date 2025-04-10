
#ifndef _AUDIO_AUTOTUNE_API_H_
#define _AUDIO_AUTOTUNE_API_H_

#include "effects/voiceChanger_api.h"
// #include "media/audio_stream.h"
struct autotune_update_parm {
    u32 mode;                        //调式
    u32 speedv;                      //2到100
};


typedef  struct _AUTOTUNE_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct autotune_update_parm parm;
} autotune_param_tool_set;


struct autotune_open_parm {
    AUTOTUNE_PARM param;
    u32 sample_rate;
};
typedef struct _autotune_hdl {
    AUTOTUNE_FUNC_API *ops;
    void *workbuf;
    struct autotune_open_parm parm;
    u8 update;
    u8 status;
} autotune_hdl;

extern AUTOTUNE_FUNC_API *get_autotune_func_api();
/*
 * 电子音打开
 */
autotune_hdl *audio_autotune_open(struct autotune_open_parm *param);
/*
 *  电子音关闭
 */
void audio_autotune_close(autotune_hdl *hdl);
/*
 *  电子音参数更新
 */
void audio_autotune_update_parm(autotune_hdl *hdl, struct autotune_update_parm *parm);
/*
 *  电子音数据处理
 */
int audio_autotune_run(autotune_hdl *hdl, s16 *indata, s16 *outdata, int len, u8 ch_num);
/*
 *  电子音暂停处理
 */

void audio_autotune_bypass(autotune_hdl *hdl, u8 bypass);
#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

