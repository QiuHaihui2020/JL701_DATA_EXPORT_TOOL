#ifndef AUDIO_DECODE_COMMON_API_H
#define AUDIO_DECODE_COMMON_API_H

#include "cpu.h"
/********************* DEC *********************/

//通用解码器配置信息
#define PLAY_MOD_NORMAL   0x00
#define PLAY_MOD_FF       0x01
#define PLAY_MOD_FB       0x02
#define SET_DECODE_MODE   0x80

typedef struct _AUDIO_DECODE_PARA {
    u32 mode;
} AUDIO_DECODE_PARA;

typedef struct _deccfg_inf {
    u32 bitwidth;   //16_24(s32)
    u32 channels;   //最多支持的声道数
    u32 sr;         //最大支持的采样率.
} deccfg_inf;

typedef struct decoder_inf {
    u32 sr ;            ///< sample rate
    u32 br ;            ///< bit rate
    u32 nch ;           ///<声道
    u32 total_time;     ///<总时间
} dec_inf_t ;

enum { //format_check 格式检查返回值, FORMAT_OK可继续调run,
    //sdk根据获取的解码信息位宽、采样率、声道数控制是否支持解码.
    FORMAT_OK = 0,
    FORMAT_OK_BUT_NO_SUPPORT,   //格式正确不支持的其他参数.
    FORMAT_ERR,
    FORMAT_NOT_DETECTED,
    FORMAT_OK_UNSUPPORTED_BPS,  //不支持的位宽.
    FORMAT_OK_UNSUPPORTED_NSR,  //不支持的采样率.
    FORMAT_OK_UNSUPPORTED_NCH,  //不支持的声道数.
    FORMAT_OK_UNSUPPORTED_BLK,  //不支持的块长度.
    M4A_MEMORY_NOT_ENOUGH = 0x10,
};

//编解码器错误的返回值
enum {
    MAD_ERROR_FILE_END = 0x40,                        //文件结束
    MAD_ERROR_FILESYSTEM_ERR = 0x41,
    MAD_ERROR_DISK_ERR = 0x42,
    MAD_ERROR_SYNC_LIMIT = 0x43,
    MAD_ERROR_FF_FR_FILE_END = 0x44,           //快进到文件结束了
    MAD_ERROR_FF_FR_END = 0x45,
    MAD_ERROR_FF_FR_FILE_START = 0x46,         //快退到头
    MAD_ERROR_LIMIT = 0x47,
    MAD_ERROR_NODATA = 0x48,                      //读不到数了
    MAD_ERROR_UNKNOWN_REQ_INDEX_CODEBOOK = 0x49,
    MAD_ERROR_BLOCK_CODE_LOOKUP_FAILED = 0x4a,
    MAD_ERROR_SUBFRAME_COUNTER = 0x4b,
    MAD_ERROR_BITALLOC = 0x4c,
    MAD_ERROR_HEADER_CHECK = 0x4d,
    MAD_ERROR_FOOTER_CHECK = 0x4e,
    MAD_ERROR_SYNC = 0x4f,

    MAD_ERROR_DSPBUSY = 0x50,

    MAD_ERROR_FRLEN_OVFLOW = 0x60,
    MAD_ERROR_DEODE_ELSE_ERR = 0x61,
    BT_AAC_NODATA_OR_FRLEN_ERR = 0x62,

    DECODER_ERR_OUT_SPACE = 0x80,
    DECODER_ERR_UNFINISH,
};


//通用解码器api
struct if_decoder_io {
    void *priv ;
    int (*input)(void *priv, u32 addr, void *buf, int len, u8 type);
    /*
    priv -- 私有结构体，由初始化函数提供。
    addr -- 文件位置
    buf  -- 读入地址
    len  -- 读入长度 512 的整数倍
    type -- 0 --同步读（等到数据读回来，函数才返回） ，1 -- 异步读（不等数据读回来，函数就放回）

    */
    int (*check_buf)(void *priv, u32 addr, void *buf);
    int (*output)(void *priv, void *data, int len);
    u32(*get_lslen)(void *priv);
    u32(*store_rev_data)(void *priv, u32 addr, int len);
    int (*input_fr)(void *priv, void **buf);
};

typedef struct if_decoder_io IF_DECODER_IO;

typedef struct __audio_decoder_ops {
    char *name;                                                            ///< 解码器名称
    u32(*open)(void *work_buf, void *stk_buf, const struct if_decoder_io *decoder_io, u8 *bk_point_ptr, void *dci);  ///<打开解码器

    u32(*format_check)(void *work_buf, void *stk_buf);					///<格式检查

    u32(*run)(void *work_buf, u32 type, void *stk_buf);					///<主循环

    dec_inf_t *(*get_dec_inf)(void *work_buf);				///<获取解码信息
    u32(*get_playtime)(void *work_buf);					///<获取播放时间
    u32(*get_bp_inf)(void *work_buf);						///<获取断点信息;返回断点信息存放的地址

    //u32 (*need_workbuf_size)() ;							///<获取整个解码所需的buffer
    u32(*need_dcbuf_size)(void *dci);						///<获取解码需要的buffer
    u32(*need_skbuf_size)(void *dci); 						///<获取解码过程中stack_buf的大小
    u32(*need_bpbuf_size)(void *dci);						///<获取保存断点信息需要的buffer的长度

    //void (*set_dcbuf)(void* ptr);			                ///<设置解码buffer
    //void (*set_bpbuf)(void *work_buf,void* ptr);			///<设置断点保存buffer

    void (*set_step)(void *work_buf, u32 step);				///<设置快进快进步长。
    void (*set_err_info)(void *work_buf, u32 cmd, u8 *ptr, u32 size);		///<设置解码的错误条件
    u32(*dec_config)(void *work_buf, u32 cmd, void *parm);
} audio_decoder_ops, decoder_ops_t;






#endif

