
#ifndef _AUDIO_SPLICING_H_
#define _AUDIO_SPLICING_H_

#include "generic/typedef.h"


/* note: 16bit位位宽的单声道转双声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_single_to_dual(void *out, void *in, u16 len);

/* note: 32bit位位宽的单声道转双声道
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_single_to_dual_32bit(void *out, void *in, u16 len);

/* note: 16bit位位宽的单声道转双声道(新增的声道可选择是复制输入声道数据或者清零0处理)
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * slience:1:新增的声道做清零处理， 0：新增的声道复制左声道数据
 * */
void pcm_single_to_dual_with_slience(void *out, void *in, u16 len, u8 slience);

/* note: 32bit位位宽的单声道转双声道(新增的声道可选择是复制输入声道数据或者清零0处理)
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * slience:1:新增的声道做清零处理， 0：新增的声道复制输入声道数据
 * */
void pcm_single_to_dual_with_slience_32bit(void *out, void *in, u16 len, u8 slience);

/* note: 16bit位位宽的单声道转四声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_single_to_qual(void *out, void *in, u16 len);

/* note: 32bit位位宽的单声道转四声道
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_single_to_qual_32bit(void *out, void *in, u16 len);

/* note: 16bit位位宽的双声道转四声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_dual_to_qual(void *out, void *in, u16 len);

/* note: 16bit位位宽的双声道转四声道(新增的声道可选择是复制输入声道数据或者清零0处理)
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * slience:1:新增的声道做清零处理， 0：新增的声道复制输入声道数据
 * */
void pcm_dual_to_qual_with_slience(void *out, void *in, u16 len, u8 slience);

/* note: 32bit位位宽的双声道转四声道(新增的声道可选择是复制输入声道数据或者清零0处理)
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * slience:1:新增的声道做清零处理， 0：新增的声道复制输入声道数据
 * */
void pcm_dual_to_qual_with_slience_32bit(void *out, void *in, u16 len, u8 slience);

/* note: 16bit位位宽的双声道混合后转双声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_dual_mix_to_dual(void *out, void *in, u16 len);

/* note: 16bit位位宽的双声道转单声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_dual_to_single(void *out, void *in, u16 len);


/* note: 32bit位位宽的双声道转单声道
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_dual_to_single_32bit(void *out, void *in, u16 len);

/* note: 16bit位位宽的双声道转单声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * channel_mode:输出左：AUDIO_CH_L、输出右：AUDIO_CH_R 输出混合后的数据：AUDIO_CH_MIX
 * */
void pcm_dual_to_single_optional(void *out, void *in, u16 len, u8 channel_mode);

/* note: 16bit位位宽的双声道转双声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * channel_mode:输出双左：AUDIO_CH_L、         输出双右：AUDIO_CH_R
 	 输出左右混合后的双声道数据：AUDIO_CH_MIX  其他：输出原始声道数据
 * */
void pcm_dual_to_dual_optional(void *out, void *in, u16 len, u8 channel_mode);

/* note: 32bit位位宽的双声道转双声道
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * channel_mode:输出双左：AUDIO_CH_L、          输出双右：AUDIO_CH_R
 	 输出左右混合后的双声道数据：AUDIO_CH_MIX   其他：输出原始声道数据
 * */
void pcm_dual_to_dual_32bit_optional(void *out, void *in, u16 len, u8 channel_mode);

/* note: 32bit位位宽的双声道转单声道
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * channel_mode:输出左：AUDIO_CH_L、输出右：AUDIO_CH_R 输出混合后的数据：AUDIO_CH_MIX
 * */
void pcm_dual_to_single_32bit_optional(void *out, void *in, u16 len, u8 channel_mode);

/* note: 16bit位位宽的四声道转单声道
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_qual_to_single(void *out, void *in, u16 len);

/* note: 32bit位位宽的四声道转单声道
 * *out:32bit位宽输入地址
 * *in:32bit位宽输出地址
 * len:总的输入长度(byte)
 * */
void pcm_qual_to_single_32bit(void *out, void *in, u16 len);

/* note: 32bit位位宽的四声道转双声道(q0+q2,q1+q3)
 * *out:32bit位宽输出地址
 * *in:32bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_qual_to_dual_32bit(void *out, void *in, u16 len);

/* note: 16bit位位宽的四声道转双声道(q0+q2,q1+q3)
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * len:总的输入长度(byte)
 * */
void pcm_qual_to_dual(void *out, void *in, u16 len);

/* note: 16bit位位宽的两个单声道组合成四声道
 * *out:16bit位宽输出地址
 * *in_l:左16bit位宽输入地址
 * *in_r:右16bit位宽输入地址
 * len:一个声道的输入长度(byte)
 * */
void pcm_single_l_r_2_dual(void *out, void *in_l, void *in_r, u16 in_len);

/* note: 16bit位位宽的四个单声道组合成四声道
 * *out:16bit位宽输出地址
 * *in_fl:左16bit位宽输入地址
 * *in_fr:右16bit位宽输入地址
 * *in_rl:左16bit位宽输入地址
 * *in_rr:右16bit位宽输入地址
 * len:一个声道的输入长度(byte)
 * */
void pcm_fl_fr_rl_rr_2_qual(void *out, void *in_fl, void *in_fr, void *in_rl, void *in_rr, u16 in_len);

/* note: 16bit将四声道中其中一个声道的数据替换
 * *out:16bit位宽输出地址
 * *in:16bit位宽输入地址
 * in_len:一个声道的输入长度(byte)
 * idx:0、1、2、3表示dac通道
 * */
void pcm_fill_single_2_qual(void *out, void *in, u16 in_len, u8 idx);

/* note: 16bit位位宽的两个立体声组合成四声道
 * *out:16bit位宽输出地址
 * *in_flfr:flfr 16bit位宽输入地址
 * *in_rlfr:rlrr 16bit位宽输入地址
 * int_len:输入长度(byte)
 * */
void pcm_flfr_rlrr_2_qual(void *out, void *in_flfr, void *in_rlrr, u16 in_len);

/* note: 16bit将四声道中其中flfr声道的数据替换
 * *out:16bit位宽输出地址
 * *in_flfr:16bit位宽输入地址
 * in_len:一个声道的输入长度(byte)
 * */
void pcm_fill_flfr_2_qual(void *out, void *in_flfr, u16 in_len);

/* note: 16bit将四声道中其中rlrr声道的数据替换
 * *out:16bit位宽输出地址
 * *in_rlrr:16bit位宽输入地址
 * in_len:一个声道的输入长度(byte)
 * */
void pcm_fill_rlrr_2_qual(void *out, void *in_rlrr, u16 in_len);


#endif/*_AUDIO_SPLICING_H_*/
