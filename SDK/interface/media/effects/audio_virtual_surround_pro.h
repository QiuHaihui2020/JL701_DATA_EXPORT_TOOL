#ifndef _AUDIO_virtual_surround_pro_API_H_
#define _AUDIO_virtual_surround_pro_API_H_
#include "system/includes.h"
#include "effects/UpmixSystem.h"
#include "effects/audio_stereo_widener.h"
#include "effects/audio_gain_process.h"
#include "effects/audio_eq.h"
#include "effects/audio_wdrc.h"
#include "effects/audio_noisegate.h"
#include "effects/multi_ch_mix.h"
#include "audio_splicing.h"

struct virtual_surround_pro_update_param {//与upmixsystem_config关联
    int process_maxfrequency;   //所需处理频率最大值
    int process_minfrequency;   //所需处理频率最小值
    int af_length;              //滤波器长度
    float adaptiverate;         //滤波器跟踪步长
    float disconverge_erle_thr; //滤波器发散控制阈值
    int use_engctr;             //左/右环绕声能量动态占比控制开关
    float icc2_beta;            //环绕声道能量动态占比控制时的平滑度
    float icc2_gamma;           //环绕声道能量动态占比控制时的归一化中心点
    float diffuse_depth;        //环绕声强度控制阈值
    int sasoutmode;
    float gloabalminsuppress;
    int resever[6];             //保留位
};


struct virtual_surround_pro_tool_set {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct virtual_surround_pro_update_param parm;
};

struct virtual_surround_pro_param {
    struct virtual_surround_pro_tool_set param;
    int channel;
    int samplerate;
    af_DataType pcm_info;
};
struct lr_process {
    struct audio_eq *cross_eq[3];
    struct audio_stereo_widener *widener;
    struct aud_gain_process *gain;
};
struct center_process {
    struct audio_wdrc *drc;
    struct audio_eq *eq;
    struct aud_gain_process *gain;
};
struct ls_rs_process {
    NOISEGATE_API_STRUCT *ns_gate;
    struct audio_wdrc *drc;
    struct audio_eq *eq;
    struct aud_gain_process *gain;
};
//立体声增强
struct audio_virtual_surround_pro {
    void *workbuf;                       //virtual_surround_pro 运行句柄及buf
    void *tmp_buf;
    struct lr_process lr_hdl;
    struct center_process c_hdl;
    struct ls_rs_process lrs_hdl;
    struct upmixsystem_config parm;
    s16 *out_buf[3];
    s16 out_len;
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
};


struct audio_virtual_surround_pro *audio_virtual_surround_pro_open(struct virtual_surround_pro_param *parm);
//virtual surround pro run
int audio_virtual_surround_pro_run(struct audio_virtual_surround_pro *hdl, void *datain, void *dataout, u32 len);
//upmix 2to5 run
int audio_virtual_surround_pro_run_base(struct audio_virtual_surround_pro *hdl, void *datain, void *dataout, u32 len);
int audio_virtual_surround_pro_close(struct audio_virtual_surround_pro *hdl);
int audio_virtual_surround_pro_update_parm(struct audio_virtual_surround_pro *hdl, struct virtual_surround_pro_tool_set *update_parm);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif

#endif

