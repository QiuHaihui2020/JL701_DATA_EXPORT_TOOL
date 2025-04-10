#ifndef _LC3_CODEC_h
#define _LC3_CODEC_h

#ifndef u16
#define  u32  unsigned int
#define  u16  unsigned short
#define  s16  short
#define  u8  unsigned char
#endif

#define  SET_DECODE_MODE      0x80
#define  SET_DEC_CH_MODE      0x81
#define  SET_FADE_IN_TMS      0x82    //for dec  plc
#define  SET_NEW_BITRATE      0x90    //enc.
#define  GET_EDFRAME_LEN      0x91    //omly for enc.
#define  SET_EDFRAME_LEN      0x92    //only for dec.

enum {
    LC3_ERROR_NO_DATA = 0x48,                  //读不到数据
    LC3_ERROR_FRLEN_MISMATCH = 0x60,           //读入一帧 帧长不匹配
};

enum {
    LC3_NORMAL_CH = 0,  //仅在此模式下,音源为双声道时输出双声道pcm数据.
    LC3_L_OUT,
    LC3_R_OUT,
    LC3_LR_MEAN
};

enum {
    LC3_2CH_L_OR_R = 0x80
};

// typedef struct _AUDIO_DECODE_PARA  {
// u32 mode;    [>通过codec_config,cmd=SET_DECODE_MODE设置解码输出模式<]
// }AUDIO_DECODE_PARA;

typedef struct _AUDIO_DECODE_VADD {
    int fi_tms;    /*通过codec_config,cmd=SET_FADE_IN_TMS设置解码丢包后音量恢复淡入时间毫秒数*/
    /*淡入持续时间:音量从0变到16384,丢包音量清零 */
} AUDIO_DECODE_VADD;


typedef struct _AUDIO_SET_BITRATE {
    u32 br;
} AUDIO_SET_BITRATE;

typedef struct _LC3_DEC_CH_OPUT_PARA {
    u32 mode;
    short pL;    //mode_3_channel_L_coefficient  Q13  8192:50%
    short pR;    //mode_3_channel_R_coefficient  Q13
} LC3_DEC_CH_OPUT_PARA;

typedef struct _LC3_CODEC_INFO {
    u32 sr;            ///< sample rate
    u32 br;            ///< bit rate
    u16 fr_dms;        ///< 每帧持续时间 ms*10. 可设置10*10/5*10/2.5*10.
    u16 ep_mode;       ///< 默认:0.
    u16 nch;
    u16 if_s24;        ///< 编码输入数据  或者解码输出数据位宽是否为24比特.
} LC3_CODEC_INFO;      ///< enc_para + dec_para

typedef struct _LC3_CODEC_IO_ {
    void *priv;
    u16(*input_data)(void *priv,  u8 *buf, u16 len);    //bytes
    u16(*output_data)(void *priv, u8 *buf, u16 len);    //bytes   /*解码器在AUDIO_DECODE_PARA参数mode=1时判断是否输出完整*/
} LC3_CODEC_IO;

typedef struct __LC3_CODEC_OPS {
    u32(*open)(void *work_buf, const LC3_CODEC_IO *audioIO, void *para);  ///<打开解码器
    u32(*run)(void *work_buf, void *scratch);		///<解码主循环(buffer 32bit对齐)
    u32(*need_dcbuf_size)(void *para);		///<获取编解码需要的空间work_buf
    u32(*need_stbuf_size)(void *para);               ///<获取编解码需要的堆栈scratch
    u32(*codec_config)(void *work_buf, u32 cmd, void *parm);
} LC3_CODEC_OPS;


extern LC3_CODEC_OPS *get_lc3_enc_ops();
extern LC3_CODEC_OPS *get_lc3_dec_ops();

#endif



