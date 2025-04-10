#ifndef _AUDIO_NOISEGATE_PRO_H_
#define _AUDIO_NOISEGATE_PRO_H_

#include "media/audio_stream.h"
#include "effects/noisegate_pro_api.h"

struct noisegate_pro_tool_update_parm {
    int attack_time;
    int release_time;
    int attack_hold_time;
    int reserve; //预留位
    float threshold;
    float low_th_gain;
    int lp_fc;
    int hp_fc;
    int lp_order;
    int hp_order;
    int lp_bypass;
    int hp_bypass;
};

struct noisegate_pro_param_tool_set {
    int is_bypass;
    struct noisegate_pro_tool_update_parm parm;
};

struct noisegate_pro_open_parm {
    int is_bypass;
    struct noisegate_pro_param parm;
};

struct noisegate_pro_hdl {
    void *work_buf;
    struct noisegate_pro_param parm;
    u8 status;
    u8 update;
};

struct noisegate_pro_hdl *audio_noisegate_pro_open(struct noisegate_pro_open_parm *open_parm);
int audio_noisegate_pro_close(struct noisegate_pro_hdl *hdl);
int audio_noisegate_pro_run(struct noisegate_pro_hdl *hdl, void *det_data, void *in_data, void *out_data, int len);
int audio_noisegate_pro_update_parm(struct noisegate_pro_hdl *hdl, struct noisegate_pro_update_param *parm);
int audio_noisegate_pro_bypass(struct noisegate_pro_hdl *hdl, int bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif
#endif
