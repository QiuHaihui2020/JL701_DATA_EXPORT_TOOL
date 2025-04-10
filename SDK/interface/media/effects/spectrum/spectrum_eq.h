#ifndef SPECTRUM_EQ_API_H
#define SPECTRUM_EQ_API_H


#include "effects/audio_eq.h"
#include "effects/spectrum/spectrum_api.h"

#define SET_SPECTRUM_ADVANCE_HANDLER 0xaa
typedef struct spectrum_advance_tool {
    int is_bypass;
    int nSections;
    visualp tool_para[0];//参数组
} spectrum_advance_param_tool_set;

//获取声道的能量值
struct spectrum_advance_parm {
    int section;//有效的段数,最大32段
    float db_data[2][32];//[0][i]:左声道, [1][i]:右声道
};
struct spectrum_advance_set_handler {
    char name[16];
    u16 type;	//SET_SPECTRUM_ADVANCE_HANDLER
    u16 fps;    //单位时间出输出的最大帧数
    u16 cbuf_len;//缓存数据的长度,未配置时，默认16k
    u16 read_len;//单次处理的长度,未配置时，默认4k
    void (*handler)(void *prive, void *param); //db值输出回调
};

struct spectrum_open_parm {
    int SampleRate;
    u8 channel;
    u16 section;
    af_DataType pcm_info;
    specp param[0];//section个
};

struct spectrum_base_hdl {
    void *workbuf;
    struct audio_eq *eq;
};

struct spectrum_advance_hdl {
    struct spectrum_base_hdl *base;
    struct spectrum_open_parm *parm;
    struct spectrum_advance_parm db_parm;//模块内缓存的一帧dB值
    OS_MUTEX mutex;
    s16 *tmp;
    s16 len;
    u8 update[32];
    u8 new_section;
    u8 status;
};


struct spectrum_advance_hdl *audio_spectrum_advance_open(struct spectrum_open_parm *parm);
int audio_stpectrum_advance_run(struct spectrum_advance_hdl *hdl, s16 *data, u16 len);
int audio_spectrum_advance_close(struct  spectrum_advance_hdl *hdl);
/*
 *获取目标频点的能量值
 **hdl:audio_spectrum_open接口的返回值
 *db_data:一帧频谱值（当前最新一次所有频点的频谱值）归一化处理后的值
 *return 0:返回正常，非0：返回失败
 * */
int audio_spectrum_advance_get_dB(struct spectrum_advance_hdl *hdl, struct spectrum_advance_parm *db_data);//获取一帧能量值
void audio_spectrum_advance_update_parm(struct spectrum_advance_hdl *hdl, spectrum_advance_param_tool_set *parm);
void audio_spectrum_advance_bypass(struct spectrum_advance_hdl *hdl, int bypass);

#endif
