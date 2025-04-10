#ifndef _AUDIO_EFFECTS_API_H_
#define _AUDIO_EFFECTS_API_H_
#include "system/includes.h"
#include "effects/audio_phaser.h"
#include "effects/audio_flanger.h"
#include "effects/audio_chorus_advance.h"
#include "effects/audio_pingpong_echo.h"
#include "effects/audio_stereo_spatial_wider.h"
#include "effects/audio_distortion_clipping.h"
#include "effects/convert_data.h"

struct effects_func_api {
    u32(*need_buf)(void *param);
    u32(*tmp_buf_size)(void *param);
    int (*init)(void *ptr, void *param, void *tmpbuf);
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);
    int (*run)(void *ptr, void *indata, void *outdata, int PointsPerChannel);
    int (*update)(void *ptr, void *param);
};


struct effects_param {
    u32 sample_rate;
    u8 in_ch_num;
    u8 out_ch_num;
    u8 in_bit_width;
    u8 out_bit_width;
    u8 qval;
    u8 bypass;
    u8 aud_module_type;
    u8 aud_module_tmpbuf_type;
    u8 set_tmpbuf_before_init;//EFFECTS_SET_TMPBUF_BEF_INIT:根据算法特性在init前,设置tmpbuf EFFECTS_SET_TMPBUF_BEF_RUN:run前设置tmpbuf
    int (*private_update)(void *ptr, void *param); //参数改变需要重新初始化算法,需实现该接口
    int (*tmpbuf_size_update)(void *ptr, void *param, int len); //需要使用run的长度更新tmpbuf_size
    void *param;//算法参数结构
    struct effects_func_api *ops;
};

struct audio_effects {
    void *workbuf;           //effects 运行句柄及buf
    void *tmp_buf;
    struct effects_param parm;
    u16  tmpbuf_size;
    u8 status;                           //内部运行状态机
};


struct audio_effects *audio_effects_open(struct effects_param *parm);
int audio_effects_close(struct audio_effects *hdl);
int audio_effects_run(struct audio_effects *hdl, void *datain, void *dataout, u32 len);
int audio_effects_det_run(struct audio_effects *hdl, void *datain, void *det_datain, void *dataout, u32 len);
int audio_effects_update_parm(struct audio_effects *hdl, void *param, int param_len);
int audio_effects_bypass_run(struct audio_effects *hdl, void *datain, void *dataout, u32 len);
int audio_effects_out_len(struct audio_effects *hdl, int inlen);
int audio_effects_handle_frame(struct audio_effects *hdl, struct stream_iport *iport, struct stream_note *note);
int audio_effects_update_bypass(struct audio_effects *hdl, int bypass);

#define EFFECTS_RUN_NORMAL 0
#define EFFECTS_RUN_BYPASS BIT(0)
#define EFFECTS_RUN_REINIT BIT(1)
#define EFFECTS_RUN_UPDATE BIT(2)

#define EFFECTS_SET_TMPBUF_BEF_INIT           BIT(0)
#define EFFECTS_SET_TMPBUF_BEF_RUN            BIT(1)
#define EFFECTS_SET_TMPBUF_BEF_INIT_AND_RUN   (BIT(0)|BIT(1))

#endif

