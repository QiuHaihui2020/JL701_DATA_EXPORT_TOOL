
#ifndef _AUDIO_VOICE_CHANGER_API_H_
#define _AUDIO_VOICE_CHANGER_API_H_

#include "effects/voiceChanger_api.h"
#include "media/audio_stream.h"

enum {
    V_CHANGER_ORINGIN_TYPE,
    V_CHANGER_ADVANCE_TYPE,
};

struct voice_changer_update_parm { //与结构VOICECHANGER_PARM关联
    u32 effect_v;                    //
    u32 shiftv;                      //pitch rate:  40-250
    u32 formant_shift;               // 40-250
};

typedef struct _VoiceChangerParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct voice_changer_update_parm parm;
} voice_changer_param_tool_set;

struct voicechanger_open_parm {
    VOICECHANGER_PARM param;
    u32 sample_rate;
    u8 type; //0 -> VoiceChanger，1 -> VoiceChanger_adv
    u8 bypass;
};

typedef struct _voice_changer_hdl {
    VOICECHANGER_FUNC_API *ops;
    void *workbuf;
    struct voicechanger_open_parm parm;
    u8 update;
    u8 status;
} voice_changer_hdl;
/*
 * 获取变声模块默认参数；open时不传默认参数会使用内部默认参数
 * 仅用于获取初值。实时参数存放在open的返回句柄parm中
 */
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api();
extern VOICECHANGER_FUNC_API *get_voiceChanger_adv_func_api();
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_pitchshift(); //不管effectv传什么，都认为是EFFECT_VOICECHANGE_PITCHSHIFT
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_cartoon();  //
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_spectrum();//不管effectv传什么，都认为是EFFECT_VOICECHANGE_SPECTRUM
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_robort();
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_melody();
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_whisper();
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_f0_domain();

extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_f0_td();   //对应get_voiceChanger_func_api的EFFECT_VOICECHANGE_F0_TD模式
extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_f0_h_td(); //对应get_voiceChanger_adv_func_api的EFFECT_VOICECHANGE_F0_TD模式

extern VOICECHANGER_FUNC_API *get_voiceChanger_func_api_feedback();
/*
 * 变声模块打开
 */
voice_changer_hdl *audio_voice_changer_open(struct voicechanger_open_parm *param);
/*
 * 变声模块关闭
 */
void audio_voice_changer_close(voice_changer_hdl *hdl);
/*
 * 变声模块参数更新
 */
void audio_voice_changer_update_parm(voice_changer_hdl *hdl, struct voice_changer_update_parm *parm);
/*
 * 变声模块数据处理
 */
int audio_voice_changer_run(voice_changer_hdl *hdl, s16 *indata, s16 *outdata, int len, u8 ch_num);
/*
 * 变声模块暂停处理
 */
void audio_voice_changer_bypass(voice_changer_hdl *hdl, u32 bypass);
#endif

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

