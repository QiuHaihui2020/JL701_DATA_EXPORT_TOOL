#ifndef _AUDIO_SURROUND_API_H_
#define _AUDIO_SURROUND_API_H_

#include "system/includes.h"
#include "media/includes.h"
#include "effects/effect_sur_api.h"


typedef struct _surround_update_parm {//与结构SurEFECT_PARM_SET 关联
    int effectflag;                  //音效类型
    int rotatestep;                     //旋转速度
    int damping;	                    //高频衰减速度
    int feedback;                       //整体衰减速度
    int roomsize;                       //空间大小
} surround_update_parm;

typedef struct _SurroundEffect_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    surround_update_parm parm;
} surround_effect_param_tool_set;

typedef struct _surround_open_parm {
    SurEFECT_PARM_SET param;
    u8 channel;                         //通道数立体声配2，  不支持单声道
    u8 bypass;
} surround_open_parm;


typedef struct _surround_hdl {
    SUR_FUNC_API *ops;                  //环绕音效底层io
    void *work_buf;                     //底层模块运行的句柄及buf
    surround_open_parm parm;            //打开模块传入的参数
    u8 status;                           //内部运行状态机
    u8 update;                           //设置参数更新标志
} surround_hdl;

/*----------------------------------------------------------------------------*/
/**@brief   audio_surround_open  环绕音效打开
   @param    *parm: 环绕音效始化参数，详见结构体surround_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
surround_hdl *audio_surround_open(surround_open_parm *parm);

/*----------------------------------------------------------------------------*/
/**@brief   audio_surround_update_parm 环绕音效参数更新
   @param    *parm:传入自定义参数
   @return   0：成功  -1: 失败
   @note    对耳时，左右声道效果，须设置保持一致
*/
/*----------------------------------------------------------------------------*/
int audio_surround_update_parm(surround_hdl *hdl, surround_update_parm *parm);

/*----------------------------------------------------------------------------*/
/**@brief   audio_surround_close 环绕音效关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_surround_close(surround_hdl *_hdl);

/*----------------------------------------------------------------------------*/
/**@brief   audio_surround_run 环绕音效处理
   @param    _hdl:句柄
   @param    data:需要处理的数据
   @param    len:数据长度
   @return  0:成功  -1：失败
   @note    无数据流节点时，直接使用改接口进行环绕音效的处理
*/
/*----------------------------------------------------------------------------*/
int audio_surround_run(surround_hdl *hdl, void *indata, void *outdata, u32 len);

void audio_surround_bypass(surround_hdl *hdl, u8 bypass);

#ifndef RUN_NORMAL
#define RUN_NORMAL  0
#endif

#ifndef RUN_BYPASS
#define RUN_BYPASS  1
#endif



#endif
