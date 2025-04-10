#ifndef __TWO_IO_LED_H__
#define __TWO_IO_LED_H__


#include "typedef.h"


#ifdef CONFIG_CPU_BR29
#define LED_IO_SUPPORT_MUX  1
#else
#define LED_IO_SUPPORT_MUX  0
#endif


// *INDENT-OFF*
typedef struct two_io_platform_data {
    u8 io0;             //送出电平信号的IO
    u8 io1;             //送出电平信号的IO
    u8 com_pole_is_io;  //公共端是否为第三IO
    u8 com_pole_io;     //输出低电平的第三IO
    u8 soft_pwm_cycle;  //软件pwm周期,单位ms
    u8 io0_pwm_duty;    //io0 pwm 高电平占空比 0~100
    u8 io1_pwm_duty;    //io1 pwm 高电平占空比 0~100
    u8 ctl_option;      //0:只控IO0, 1:只控IO1，2:交替输出 3:同步输出
    u8 ctl_mode;        //0:一周期输出一次，1:一周期输出两次，2:周期渐变输出，3:常输出
    u8 ctl_cycle_num;   //控制周期的个数，值为0时，则控制周期无限循环, 值为n时，则第n次控制周期之后，模块自动关闭
    union {
        struct {
            u16 idle_time;  //控制周期内idle的时间
            u16 out_time;   //控制周期内输出的时间
        } cycle_once;       //周期1次
        struct {
            u16 idle_time;  //控制周期内idle的时间
            u16 out_time;   //控制周期内第一次输出的时间
            u16 gap_time;   //控制周期内第一次输出与第二次输出的间隔时间
            u16 out_time1;  //控制周期内第二次输出的时间
        } cycle_twice;
        struct {
            u16 idle_time;  //控制周期内idle的时间
            u16 duty_step;  //占空比每次递增或递减的步进
            u16 duty_step_time;//占空比从 0增至最大 或者 最大减至0 花的时间
            u16 duty_keep_time;//占空比增至最大时，至少维持的时间
        } cycle_breathe;
    };
    void (*ctl_cycle_cbfunc)(u32);//第ctl_cycle_num次控制周期结束时的回调函数
    u32 ctl_cycle_cbpriv;
} two_io_pdata_t;


#define TWO_IO_CTL_PLATFORM_DATA_BEGIN(data) \
    struct two_io_platform_data data = {

#define TWO_IO_CTL_PLATFORM_DATA_END()  \
    };


void two_io_ctl_init(two_io_pdata_t *pdata);

void two_io_ctl_close(void);

#endif


