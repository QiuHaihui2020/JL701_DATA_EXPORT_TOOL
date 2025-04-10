#ifndef _BASS_TREBLE_EQ_CONFIG_H_
#define _BASS_TREBLE_EQ_CONFIG_H_

#include "spinlock.h"
#include "math.h"
#include "effects/audio_eq.h"

struct bass_treble_gain_range {
    int iir_type;
    int center_frequency;
    float Q;
    float min_gain;
    float max_gain;
    float cur_gain;//在线调试时用于与工具交互显示，并控制当前增益。但存储效果文件内的值不用做当前gain
};

typedef struct bass_treble_tool_parm { //工具界面调音结构
    u32 is_bypass;
    u32 type;//默认0xaa:标识工具参数,
    u32 fade_time;
    float fade_step;
    float global_gain;
    struct bass_treble_gain_range range[3];
} bass_treble_param_tool_set;

struct seg_gain {
    int index;
    float gain;
};
struct bass_treble_parm { //旋钮调音结构
    u32 is_bypass;
    u32 type;//默认0xaa:标识工具参数
    u32 fade_time;
    float fade_step;
    float global_gain;
    struct seg_gain gain;
} ;

struct bass_treble_default_parm {
    char name[16];
    u32 is_bypass;
    u32 type;
    float global_gain;
    float gain[3];//[0]：低音增益 [1]：中音增益  [2]:高音增益
    char cfg_index;//如果使用配置项的序号，指定默认配置项
    char mode_index;//节点与多模式关联时，该变量用于获取相应模式下的节点参数.模式序号（如，蓝牙模式下，无多子模式，mode_index 是0）
};

#define BASS_TREBLE_PARM_INIT            (1UL<<0)
#define BASS_TREBLE_PARM_GET             (1UL<<2)
#define BASS_TREBLE_PARM_SET             (1UL<<4)
#define BASS_TREBLE_PARM_SET_GLOBAL_GAIN (1UL<<6)
#define BASS_TREBLE_PARM_SET_BYPASS      (1UL<<8)
#define BASS_TREBLE_PARM_TOO_SET 0xaa

enum bass_treble_eff {
    BASS_TREBLE_LOW  =  0,
    BASS_TREBLE_MID  =  1,
    BASS_TREBLE_HIGH =  2,
};



/*******************************************************************/
// 兼容旧接口
#define AUDIO_EQ_HIGH          2
#define AUDIO_EQ_BASS          3
struct high_bass {
    int freq;
    int gain;    //增益范围 -48 ~ 48
};
void mix_out_high_bass(u32 cmd, struct high_bass *hb);
/*******************************************************************/


#endif
