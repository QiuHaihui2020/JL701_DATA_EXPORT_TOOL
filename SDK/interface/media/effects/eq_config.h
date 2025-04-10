#ifndef _EQ_CONFIG_H_
#define _EQ_CONFIG_H_

#include "spinlock.h"
#include "math.h"
#include "effects/audio_eq.h"
#include "effects/effects_adj.h"



/*----------------------------------------------------------------------------*/
/**@brief   用默认eq系数表的eq效果模式设置(设置模式,更新系数)
  @param   mode:EQ_MODE_NORMAL, EQ_MODE_ROCK,EQ_MODE_POP,EQ_MODE_CLASSIC,EQ_MODE_JAZZ,EQ_MODE_COUNTRY, EQ_MODE_CUSTOM
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/

int eq_mode_set(EQ_MODE mode);
/*----------------------------------------------------------------------------*/
/**@brief   eq模式切换
  @param
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_sw(void);

/*----------------------------------------------------------------------------*/
/**@brief   设置custom系数表的某一段系数
  @param   seg->index：第几段(从0开始)
  @param   seg->iir_type:滤波器类型(EQ_IIR_TYPE_HIGH_PASS, EQ_IIR_TYPE_LOW_PASS, EQ_IIR_TYPE_BAND_PASS, EQ_IIR_TYPE_HIGH_SHELF,EQ_IIR_TYPE_LOW_SHELF)
  @param   seg->freq:中心截止频率(20~22kHz)
  @param   seg->gain:总增益(-18~18)
  @param   seg->q : q值（0.3~30）
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_seg(struct eq_seg_info *seg);
/*----------------------------------------------------------------------------*/
/**@brief   获取custom系数表的增益、频率
  @param   index:哪一段
  @param   freq:中心截止频率
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_info(u16 index, int freq, float gain);
/*----------------------------------------------------------------------------*/
/**@brief   获取某eq系数表一段eq的增益
  @param   mode:哪个模式
  @param   index:哪一段
  @return  增益
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
s8 eq_mode_get_gain(EQ_MODE mode, u16 index);
/*----------------------------------------------------------------------------*/
/**@brief   获取某eq系数表一段eq的中心截止频率
  @param   mode:EQ_MODE_NORMAL, EQ_MODE_ROCK,EQ_MODE_POP,EQ_MODE_CLASSIC,EQ_MODE_JAZZ,EQ_MODE_COUNTRY, EQ_MODE_CUSTOM
  @param   index:哪一段
  @return  中心截止频率
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_get_freq(EQ_MODE mode, u16 index);

/*----------------------------------------------------------------------------*/
/**@brief   设置用custom系数表一段eq的增益
  @param   index:哪一段
  @param   gain:增益
  @return
  @note    外部使用
  */
/*----------------------------------------------------------------------------*/
int eq_mode_set_custom_param(u16 index, int gain);

/*----------------------------------------------------------------------------*/
/**@brief    设置播歌用的eq系数表的总增益
   @param   global_gain:总增益
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void set_global_gain(EQ_MODE mode, float global_gain);


/*
 *获取当前默认系数表地址
 * */
int get_cur_eq_tab(float *global_gain, struct eq_seg_info **seg);


/*
 *调用本接口，可切换到eq节点内指定的配置项
 *name:eq节点定义的名字
 *cfg_index:eq配置项的序号
 * */
void eq_file_cfg_switch(char *name, char cfg_index);

// 使用过eq_file_cfg_switch接口时，本接口可获取musicEq的当前配置项序号
int get_cur_eq_num(char *tar_cfg_eq);


//返回某个eq效果模式标号
EQ_MODE eq_mode_get_cur(void);

/*
 *mode:枚举型EQ_MODE
 *return 返回对应系数表的段数
 * */
u8 eq_get_table_nsection(EQ_MODE mode);


extern const int AUDIO_EQ_MAX_SECTION;

#endif
