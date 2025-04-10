#ifndef effectSUR_api_h__
#define effectSUR_api_h__


#include  "AudioEffect_DataType.h"



/* enum */
// {
// EFFECT_CH_L = 0x10,                 //单声道输入，输出左声道
// EFFECT_CH_R = 0x20,                  //单声道输入，输出右声道
// EFFECT_CH2_L= 0x30,                  //双声道输入，输出2个左声道
// EFFECT_CH2_R = 0x40,                  //双声道输入，输出2个右声道
/* }; */

enum {
    EFFECT_3D_TYPE0 = 0x01,
    EFFECT_3D_TYPE1 = 0x02,            //这2个2选1  ：如果都置上，优先用EFFECT_3D_TYPE1
    EFFECT_3D_LRDRIFT = 0x04,
    EFFECT_3D_ROTATE = 0X08,           //这2个2选1 : 如果都置上，优先用EFFECT_3D_ROTATE
    EFFECT_3D_TYPE2 = 0x10,
    EFFECT_3D_LRDRIFT2 = 0x20
};

typedef struct SurEFECT_PARM_SET {
    int effectflag;            //下拉框:可选项上面的enum
    int rotatestep;            //范围0到1000，文本框填值
    int damping;               //0到4096，文本框填值
    int feedback;              //0到128，文本框填值
    int roomsize;              //0到128 ，文本框填值
    af_DataType dataTypeobj;
} SurEFECT_PARM_SET;


typedef struct __SUR_FUNC_API_ {
    unsigned int (*need_buf)(SurEFECT_PARM_SET *surparm);
    unsigned int (*open)(unsigned int *ptr, int nch, SurEFECT_PARM_SET *surparm);
    unsigned int (*init)(unsigned int *ptr, SurEFECT_PARM_SET *surparm);
    unsigned int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);             // len是输入数据的总长byte
} SUR_FUNC_API;


extern SUR_FUNC_API *get_sur_func_api();




#endif // reverb_api_h__
