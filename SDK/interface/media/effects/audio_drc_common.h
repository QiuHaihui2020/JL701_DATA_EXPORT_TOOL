#ifndef _AUDIO_DRC_COMMON_H_
#define _AUDIO_DRC_COMMON_H_

enum {
    DATA_IN_SHORT = 0,    //输入数据类型，输入short时，输出不能为int
    DATA_IN_INT,
};
enum {
    DATA_OUT_SHORT = 0, //输出数据类型，输入short时，输出不能为int
    DATA_OUT_INT
};

enum {
    PEAK = 0,	 //算法类型
    RMS
};

enum {
    PERPOINT = 0, //mode 模式
    TWOPOINT
};

struct threshold_group {
    float in_threshold;
    float out_threshold;
};
struct mdrc_common_param {
    int is_bypass;
    int way_num;                                    //段数
    int order;                                      //阶数
    int low_freq;                                   //低分频点
    int high_freq;                                  //高分频点
};
#endif

