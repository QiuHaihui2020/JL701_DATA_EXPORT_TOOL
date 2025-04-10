#ifndef _AUDIO_HW_CROSSOVER_H_
#define _AUDIO_HW_CROSSOVER_H_

#include "effects/eq_config.h"
#include "effects/audio_eq.h"

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

#if 0
void *user_eq_hdl;
float user_coeff[5] = {1, 0, 0, 0, 0}; //直通的系数
void audio_hw_eq_process_demo()
{
    u32 ch_num = 2;	//通道数
    u32 sample_rate = 44100;//采样率
    u32 in_bit_width = 1;//32bit
    u32 out_bit_width = 1;//32bit
    u8 nsection = 1;//滤波器个数
    //open
    user_eq_hdl = audio_eq_coeff_open(user_coeff, nsection, sample_rate, ch_num, in_bit_width, out_bit_width);

    //run
    s16 *in_data_addr = ?;
    s16 *out_data_addr = ?;
    u32 in_data_len = ?;
    audio_eq_coeff_run(user_eq_hdl, in_data_addr, out_data_addr, in_data_len);

    //update ceoff tab
    //set coeff to eq
    audio_eq_coeff_update(user_eq_hdl, user_coeff);

    //close
    audio_eq_coeff_close(user_eq_hdl);
    user_eq_hdl = NULL;
}
#endif

#endif

