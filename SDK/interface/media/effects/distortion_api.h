#ifndef _DISTORTION_API_H
#define _DISTORTION_API_H

#include "AudioEffect_DataType.h"

#ifdef WIN32

#define AT_DISTORTION(x)
#define AT_DISTORTION_CODE
#define AT_DISTORTION_CONST
#define AT_DISTORTION_SPARSE_CODE
#define AT_DISTORTION_SPARSE_CONST

#else
#define AT_DISTORTION(x)           __attribute__((section(#x)))
#define AT_DISTORTION_CODE         AT_DISTORTION(.distortion.text.cache.L2.code)
#define AT_DISTORTION_CONST        AT_DISTORTION(.distortion.text.cache.L2.const)
#define AT_DISTORTION_SPARSE_CODE  AT_DISTORTION(.distortion.text)
#define AT_DISTORTION_SPARSE_CONST AT_DISTORTION(.distortion.text.const)
#endif

// 以下控制输入输出位宽的宏，用于前期的算法测试；
// 若工具在实际运行时候已根据实际的输入输出位宽已经定义disortortion_run_mode变量，则可删除下面的宏
// 算法支持下面的全部4种输入输出格式，运行时根据输入输出位宽定义并初始化distortion_run_mode变量为其中一种输入输出格式，如下
// const int distortion_run_mode = EFx_BW_16t16 | EFx_BW_16t32 | EFx_BW_32t16 | EFx_BW_32t32;
// #define BIT(x) (int)(1<<x)
// #define EFx_BW_16t16 BIT(0)
// #define EFx_BW_16t32 BIT(1)
// #define EFx_BW_32t16 BIT(2)
// #define EFx_BW_32t32 BIT(3)

extern const int distortion_run_mode;
enum DistortionType {
    Hardclip = 0,
    Softclip1,
    Softclip2,
};

typedef struct _distortion_param {
    af_DataType pcm_info;

    int distortion_type;          // 失真处理类型：Hardclip / Softclip1 /Softclip2
    int channel;
    float driver_dB;              // 输入信号在进行失真削波之前的放大倍数(dB), 建议范围0.0f ~ 80.0f
    float clip_threshold;
    float softclip1_kneeWidth;    // Softclip1方式下的knee宽度设置，范围在0.05f - 1.0f之间
    float volume;                 // 输出音量大小控制（dB）, 建议范围-30.0f ~ 30.0f
    float dry_wet;                // 干湿比，0.0f表示全为湿，1.0f表示全为干, 建议范围0.0f ~ 1.0f

} distortion_api_t;

struct distortion_func_api {
    unsigned int (*need_buf)(void *parm);
    unsigned int (*tmp_buf_size)(void *parm);
    int (*init)(void *ptr, void *pram, void *tmpbuf);
    int (*set_tmpbuf)(void *ptr, void *tmpbuf);
    int (*run)(void *ptr, void *indata, void *outdata, int PointPerChannel);
    int (*update)(void *ptr, void *parm);
};
unsigned int distortion_need_buf(void *param);
unsigned int distortion_tmp_buf_size(void *param);
int distortion_init(void *workbuf, void *param, void *tmpbuf);
int distortion_set_tmpbuf(void *workbuf, void *tmpbuf);
int distortion_run(void *workbuf, void *indata, void *outdata, int per_channel_npoint);
int distortion_update(void *workbuf, void *param);

struct distortion_func_api *get_distortion_func_api();
int libdistortion_version_1_0_0();

#endif // _DISTORTION_API_H
