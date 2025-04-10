#ifndef __SCALE_H__
#define __SCALE_H__

#include "system/includes.h"
#include "asm/imb.h"


struct block_info {
    int block_buffer_size;
    int block_stride;
    int block_lines;
    u8 *block_buffer;
    u8 *block_buffer_p[4];
    struct imb_task_head *root_0;
    struct imb_task_head *root_1;
    struct imb_task_head *root_scale;
    int lcd_width;
    int lcd_height;
    u8 *lcd_buffer;
    int lcd_buffer_size;
    int lcd_buffer_num;
    int lcd_buffer_stride;
};

/*----------------------------------------------------------------------------*/
/**@brief    初始化缩放相关参数
   @param    root_0 : 第一个imb任务链(左半屏)
   @param    root_1 : 第二个imb任务链(右半屏)
   @param    lcd_width : 屏宽度
   @param    lcd_height : 屏高度
   @param    buffer[4] : 分块缓存(需要4块)
   @param    lcd_buffer_size : 每个分块缓存大小
   @param    lcd_buffer_stride : 分块缓存每行像素的字节数
   @return   缩放相关参数，提供给后续接口(imb_scale_move/imb_scale_jump/imb_scale_uninit)使用
   @note
*/
/*----------------------------------------------------------------------------*/
struct block_info *imb_scale_init(struct imb_task_head *root_0, struct imb_task_head *root_1, int lcd_width, int lcd_height, u8 *buffer[4], int lcd_buffer_size, int lcd_buffer_stride);
/*----------------------------------------------------------------------------*/
/**@brief    整屏左右缩放效果
   @param    block_info : 缩放信息
   @param    x_offset : 左边页面的偏移值(-lcd_width <= x_offset < 0)
   @param    ratio_l : 左边页面的缩放比率(ratio_l <= 1.0)
   @param    ratio_r : 右边页面的缩放比率(ratio_r <= 1.0)
   @param    scale_method : 缩放方式 0:软件缩放(速度快) 1:硬件缩放(效果好)
   @return
   @note     软件缩放速度快，效果差些，硬件缩放效果好，性能差些，建议软件缩放和硬件缩放结合使用，手指快速划动时采用软件缩放，手指慢速划动或者手指不动时采用硬件缩放, 可以兼顾切线以及效果
*/
/*----------------------------------------------------------------------------*/
void imb_scale_move(struct block_info *block_info, int x_offset, float ratio_l, float ratio_r, int scale_method);
/*----------------------------------------------------------------------------*/
/**@brief   整屏跳转缩放效果
   @param   ratio : 跳转页面的缩放比率(ratio <= 1.0)
   @param   scale_method : 缩放方式 0:软件缩放(速度快) 1:硬件缩放(效果好)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void imb_scale_jump(struct block_info *block_info, float ratio, int scale_method);
/*----------------------------------------------------------------------------*/
/**@brief   释放内存
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void imb_scale_uninit(struct block_info *block_info);

#endif


