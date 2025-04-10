#ifndef AUDIO_BASE_H
#define AUDIO_BASE_H

#include "generic/typedef.h"
#include "audio_def.h"

#define AUDIO_INPUT_FILE          0x01
#define AUDIO_INPUT_FRAME         0x02


enum audio_channel {
    AUDIO_CH_L          = (1 << 4) | 1,           	//左声道（单声道）
    AUDIO_CH_R          = (1 << 4) | 2,           	//右声道（单声道）
    AUDIO_CH_DIFF       = (1 << 4) | 3,        	//差分（单声道）
    AUDIO_CH_MIX        = (1 << 4) | 4,           //左右声道混合(单声道)
    AUDIO_CH_LR         = (2 << 4) | 5,       	//立体声
    AUDIO_CH_DUAL_L     = (2 << 4) | 6,  		//双声道都为左
    AUDIO_CH_DUAL_R     = (2 << 4) | 7,  		//双声道都为右
    AUDIO_CH_DUAL_LR    = (2 << 4) | 8,  		//双声道为左右混合
    AUDIO_CH_TRIPLE     = (3 << 4) | 1,  		//3声道
    AUDIO_CH_QUAD       = (4 << 4) | 9,  		//四声道（LRLR）

    AUDIO_CH_MAX = 0xff,
};

#define AUDIO_CH_NUM(ch) ((ch) >> 4)

/*! \brief      音频处理结构 */
struct audio_fmt {
    u8  channel;        /*!<  */
    u16  frame_len;      /*!<  幁长度 (bytes)*/
    u16 sample_rate;    /*!<  采样率 e.g. 48kHz/44.1kHz/32kHz*/
    u32 coding_type;
    u32 bit_rate;       /*!<  比特率 (bps)*/
    u32 total_time;
    u32 quality;
    u32 complexity;
    void *priv;
};


enum audio_hw_device {
    AUDIO_HW_SBC_DEV,

};














#endif

