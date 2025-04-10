#ifndef AUDIO_ENCODE_COMMON_API_H
#define AUDIO_ENCODE_COMMON_API_H

#include "cpu.h"

/*********************** ENC *************************/

typedef struct _EN_FILE_IO_ {
    void *priv;
    u16(*input_data)(void *priv, s16 *buf, u8 channel, u16 len);
    u32(*output_data)(void *priv, u8 *buf, u16 len);
} EN_FILE_IO;



typedef  struct   _ENC_DATA_INFO_ {
    u16 sr;            ///<sample rate
    u16 br;            ///<mp2的时候它是bitrate，但是adpcm的时候，它是blockSize,一般配成256/512/1024/2048，超过2048会被限制成2048
    u32 nch;
    u8 bit_width;
} ENC_DATA_INFO;


typedef struct _ENC_OPS {
    u32(*need_buf)(u32 channel);
    void (*open)(void *ptr, EN_FILE_IO *audioIO);
    void (*set_info)(void *ptr, ENC_DATA_INFO *data_info);
    u32(*init)(void *ptr);
    u32(*run)(void *ptr);
    u8 *(*write_head)(void *ptr, u16 *len);
    u32(*get_time)(void *ptr);
} ENC_OPS;


#endif


