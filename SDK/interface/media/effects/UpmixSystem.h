#ifndef UpmixSystem_H
#define UpmixSystem_H

#include "AudioEffect_DataType.h"

#define UPMIXSYSTEM_CONFIG_SETENABLEBIT 0
#define UPMIXSYSTEM_CONFIG_SETDIFFUSEENGCONTROL 2
#define UPMIXSYSTEM_CONFIG_SETDEC_ICC2BETA 3
#define UPMIXSYSTEM_CONFIG_SETDEC_ICC2GAMMA 4
#define UPMIXSYSTEM_CONFIG_SETDIFFUSEOUTLEV 5
#define SAS_L_EN 1
#define SAS_R_EN 2
#define SAS_C_EN 4
#define SAS_LS_EN 8
#define SAS_RS_EN 16

/*********Note: 5.1 Surround sound channel arrangement**********
 * L、R、C、LFE、LS、RS
 * But this algorithm omits the calculation of LFE
 * Therefore, The final output is arranged in order:
 * L、R、C、LS、RS
 *
 * OutCh_1_**：Single-channel output configuration
 *  Such as 'OUTCH_1_C'
 * OutCh_2_**：Dual-channel output configuration
 *  Such as 'OUTCH_2_L_R'
 * OutCh_3_**：Three-channel output configuration
 *  Such as 'OUTCH_3_L_R_C'
 * * OutCh_4_**：Four-channel output configuration
 *  Such as 'OUTCH_4_L_R_LS_RS'
 * OutCh_5_**：Five-channel output configuration
 *  Such as 'OUTCH_5_L_R_C_LS_RS'
 *
 * The currently supported output configurations are described
 * in the enumeration structure below.
 * It also can be increased according to actual demand
 * *****************************************************************/
enum sasoutmode_set {
    OUTCH_1_L = 0,
    OUTCH_1_R,
    OUTCH_1_C,
    OUTCH_1_LS,
    OUTCH_1_RS,
    OUTCH_2_L_R,
    OUTCH_2_L_LS,
    OUTCH_2_R_RS,
    OUTCH_2_LS_RS,
    OUTCH_3_L_R_C,
    OUTCH_4_L_R_LS_RS,
    OUTCH_5_L_R_C_LS_RS,

};

struct upmixsystem_config {
    int channel;                // 通道数
    int samplerate;             //采样率
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
    af_DataType pcm_info;
};
#ifdef __cplusplus
extern "C"
{
#endif
// 获取处理帧长大小
int sas_getminiframesize();
// 获取算法处理时延点数
int sas_getprocessdelay();
// 获取算法运行时所需固定空间大小
int sas_querybufsize(struct upmixsystem_config *af_cfg);

// 获取算法运行时所需临时空间大小
int sas_querytempbufsize(struct upmixsystem_config *af_cfg);

//算法初始化接口
int sas_init(void *runbuf,
             void *tmpbuf,
             struct upmixsystem_config *af_cfg);
//算法处理接口
void sas_stereo_process(void *runbuf,
                        void *tempbuf,
                        void *input,
                        void *output,
                        int nminiframe);
//算法额外预留配置接口（可用于初始化后或者p运行时动态调整相关参数）
void sas_config(void *runbuf, int configtype, int *configpara);
#ifdef __cplusplus
}
#endif
#endif


