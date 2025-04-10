
#include "system/includes.h"
#include "audio_config.h"
#include "effects/audio_hw_crossover.h"
#include "effects/audio_eq.h"

/* ### EQ加速模块 */
/* （1）直接填系数形式硬件EQ接口**<br> */
/*eq 打开
 *coeff:nsection个二阶IIR滤波器。每5个系数对应一个二阶的IIR滤波器。
        每个系数都是float类型,左右声道使用同一分系数.
        b0 b1 b2 a1 a2对应coeff排列如下：
                                        coeff[0]:b0
                                        coeff[1]:-a2
                                        coeff[2]:b2 / b0
                                        coeff[3]:-a1
                                        coeff[4]:b1 / b0
 *nsection    :系数表的段数(二阶滤波器个数)
 *sample_rate :采样率
 *ch_num      :通道数（1声道还是2声道)
 *in_mode     :eq输入数据位宽，2:float, 1：输入32bit数据， 0：输入16bit是数据
 *out_mode    :eq输出数据位宽，2:float, 1：输出32bit数据， 0：输出16bit是数据
 *return      :返回句柄
 * */
void *audio_eq_coeff_open(void *coeff, u8 nsection, u32 sample_rate, u32 ch_num, u32 in_mode, u32 out_mode);

/*eq 处理
 *hdl        :audio_eq_coeff_open返回的句柄
 *indata     :输入buf
 *outdata    :eq处理后输出数据存放的buf
 *indata_len :输入数据的字节长度,单位byte
 * */
void audio_eq_coeff_run(void *hdl, void *indata, void *outdata, u32 indata_len);

/*eq 更新滤波器系数
 *hdl   :audio_eq_coeff_open返回的句柄
 *coeff :需要更新的滤波器系数（段数不可变）
 * */
void audio_eq_coeff_update(void *hdl, void *coeff);

/*eq 关闭
 *hdl  :audio_eq_coeff_open返回的句柄
 * */
void audio_eq_coeff_close(void *hdl);



void *user_eq_hdl;
float user_coeff[5] = {1, 0, 0, 0, 0}; //直通的系数
s32 user_pcm_data0[256];
void audio_hw_eq_process_demo()
{
    u32 ch_num         = 2;	//通道数
    u32 sample_rate    = 44100;//采样率
    u32 in_bit_width   = 1;//32bit
    u32 out_bit_width  = 1;//32bit
    u8 nsection        = 1;//滤波器个数

    //open
    user_eq_hdl = audio_eq_coeff_open(user_coeff, nsection, sample_rate, ch_num, in_bit_width, out_bit_width);

    //run
    s16 *in_data_addr = (s16 *)user_pcm_data0;
    s16 *out_data_addr = (s16 *)user_pcm_data0;
    u32 in_data_len = sizeof(user_pcm_data0);
    audio_eq_coeff_run(user_eq_hdl, in_data_addr, out_data_addr, in_data_len);

    //update ceoff tab
    //set coeff to eq
    audio_eq_coeff_update(user_eq_hdl, user_coeff);

    //close
    audio_eq_coeff_close(user_eq_hdl);
    user_eq_hdl = NULL;

}


/* - **（2）直接填FGQ形式硬件EQ接口**<br> */

/*
 * eq打开处理
 **/
/*----------------------------------------------------------------------------*/
struct audio_eq *audio_dec_eq_open(struct audio_eq_param *parm);
/*
 *eq 关闭打开处理
 * */
void audio_dec_eq_close(struct audio_eq *eq);
/*
 *eq处理数据
 * */
int audio_dec_eq_run(struct audio_eq *eq, s16 *in,  s16 *out, u16 len);

/*
 *更新总增益
 * */
void audio_eq_update_global_gain(struct audio_eq *eq, struct eq_fade_parm fade_parm, float g_Gain);

/*
 *更新某一段eq系数seg
 *nsection:总的段数（与open时的滤波器个数一致）
 * */
void audio_eq_update_seg_info(struct audio_eq *eq, struct eq_fade_parm fade_parm, struct eq_seg_info *seg, u32 nsection);

struct audio_eq *fgq_eq_hdl;               //用户EQ指针
static float user_global_gain = 0;         //用户EQ总增益
static struct eq_seg_info user_eq_tab[] = {//用户EQ滤波器系数表
    {0, EQ_IIR_TYPE_BAND_PASS, 100,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 500,   0, 0.7f},
    {2, EQ_IIR_TYPE_BAND_PASS, 1000,  0, 0.7f},
    {3, EQ_IIR_TYPE_BAND_PASS, 5000,  0, 0.7f},
    {4, EQ_IIR_TYPE_BAND_PASS, 10000,  0, 0.7f},
};

s16 user_pcm_data1[256];
void user_eq_demo()
{
    //打开模块
    struct audio_eq_param eq_param = {0};
    struct eq_parm eparm = {0};
    eparm.in_mode  = 0;      //1：输入32bit数据， 0：输入16bit是数据
    eparm.out_mode = 0;      //1：输出32bit数据， 0：输出16bit是数据
    eparm.run_mode = 0;      //normal,固定使用
    eparm.data_in_mode = 1;  //序列进LRLRLR,固定使用
    eparm.data_out_mode = 1; //序列出LRLRLR,固定使用
    eq_param.parm = &eparm;
    eq_param.channels = 2;                        //通道数
    eq_param.sr = 44100;                          //采样率
    eq_param.cb = eq_get_filter_info;             //系数回调，固定使用
    eq_param.global_gain = user_global_gain;      //总增益
    eq_param.seg = user_eq_tab;                   //用户滤波器系数表
    eq_param.max_nsection = eq_param.nsection = ARRAY_SIZE(user_eq_tab);//滤波器个数
    fgq_eq_hdl = audio_dec_eq_open(&eq_param);


    //处理数据
    if (fgq_eq_hdl) {
        s16 *in_data = user_pcm_data1;    //输入数据地址
        s16 *out_data = user_pcm_data1;   //输出数据地址,输入输出位宽一致时，输入输出数据地址可复用
        s16 in_data_len = sizeof(user_pcm_data1); //输入数据长度，单位byte
        audio_dec_eq_run(fgq_eq_hdl, (s16 *)in_data, (s16 *)out_data, in_data_len);
    }


    //运行数据过程,如需要更新总增益
    struct eq_fade_parm fparm = {0};
    float global_gain = -2;        //总增益，单位dB
    user_global_gain = global_gain;//记录总增益
    if (fgq_eq_hdl) {
        audio_eq_update_global_gain(fgq_eq_hdl, fparm, global_gain);
    }


    //运行数据过程,如需要更新某一段滤波器
    struct eq_seg_info seg_tab = {0};
    seg_tab.index = 4 ;                        //更新第5段系数
    seg_tab.iir_type =  EQ_IIR_TYPE_BAND_PASS; //滤波器类型
    seg_tab.freq = 10000;                     //中心截止频率，单位Hz
    seg_tab.gain = -1;                         //增益值，单位dB
    seg_tab.q = 0.7f;                          //0.7f~30
    if (fgq_eq_hdl) {
        audio_eq_update_seg_info(fgq_eq_hdl, fparm, &seg_tab, ARRAY_SIZE(user_eq_tab));
    } else {
        memcpy(&user_eq_tab[seg_tab.index], &seg_tab, sizeof(struct eq_seg_info));
    }


    //模块使用完成，关闭模块
    if (fgq_eq_hdl) {
        audio_dec_eq_close(fgq_eq_hdl);
        fgq_eq_hdl = NULL;
    }
}


