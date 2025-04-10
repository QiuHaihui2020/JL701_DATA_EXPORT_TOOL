#ifndef _AUDIOEFFECT_DATATYPE_
#define _AUDIOEFFECT_DATATYPE_

enum PCMDataType {
    DATA_INT_16BIT = 0,
    DATA_INT_32BIT,
    DATA_FLOAT_32BIT
};

typedef struct _af_DataType_ {
    unsigned char IndataBit;//输入数据bit类型
    unsigned char OutdataBit;//输出数据bit类型
    char IndataInc;//输入数据相同通道的下一个点间隔
    char OutdataInc;//输出数据相同通道的下一个点间隔
    char Qval;//位宽
} af_DataType;


enum {
    af_DATABIT_NOTSUPPORT = 0x404   //不支持的PCM数据类型返回错误
};


#endif
