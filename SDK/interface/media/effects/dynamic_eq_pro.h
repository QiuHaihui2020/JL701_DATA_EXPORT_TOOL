#ifndef __DYNAMIC_EQ_PRO_H__
#define __DYNAMIC_EQ_PRO_H__

#include "system/includes.h"
#include "effects/DynamicEQPro_api.h"
#include "effects/drc_api.h"
#include "effects/convert_data.h"

#define RUN_NORMAL  0
#define RUN_BYPASS  1

//动态EQ DynamicEQPro:
typedef struct _dynamic_eq_pro_tool_set {
    int is_bypass;          // 1-> byass 0 -> no bypass
    int nSection;					//段数
    DynamicEQProEffectParam  effectParam[4];
} dynamic_eq_pro_param_tool_set ; //实际发送这个结构体


struct dynamic_eq_pro {
    struct audio_stream_entry entry;	//音频流入口
    void *workbuf;                      //算法运行buf
    DynamicEQProEffectParam *effectParam;
    DynamicEQProParam parm;                //算法相关配置参数
    u8 status;                          //内部运行状态机
    u8 update;                          //设置参数更新标志
};
/*
*********************************************************************
*            dynamic_eq_pro_open
* Description: 动态eq打开
* Arguments  :*parm 检测模块相关参数、若有2段，则parm[0], parm[1],参数连续存方
*             nesection:动态eq检测模块支持的段数
*             channel:输入数据通道数
*             sample_rate:输入数据采样率
* Return	 : 模块句柄.
* Note(s)    : None.
*********************************************************************
*/
struct dynamic_eq_pro *dynamic_eq_pro_open(DynamicEQProEffectParam *effectParam, DynamicEQProParam *parm);


/*
*********************************************************************
*            dynamic_eq_pro_run
* Description: 动态eq模块数据处理
* Arguments  :*hdl:模块句柄
* 			  ext_det_data:外部检测数据源，配NULL时，使用输入数据
*             data:输入数据地址，32bit位宽
*             len:输入数据长度，byte
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int dynamic_eq_pro_run(struct dynamic_eq_pro *hdl, int *ext_det_data, int *data, int len);

/*
*********************************************************************
*            dynamic_eq_pro_bypass
* Description: 动态eq模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_pro_bypass(struct dynamic_eq_pro *hdl, u8 bypass);

/*
*********************************************************************
*            dynamic_eq_bypass
* Description: 动态eq模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_pro_update(struct dynamic_eq_pro *hdl, dynamic_eq_pro_param_tool_set *dparam);


/*
*********************************************************************
*            dynamic_eq_close
* Description: 动态eq检块关闭
* Arguments  :*hdl:模块句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_pro_close(struct dynamic_eq_pro *hdl);





#endif/*__DYNAMIC_EQ_H__*/
