#include "pwm_led.h"
#include "gpio.h"



// *INDENT-OFF*

//case 1：ALL OFF
//case 4：
//case 10：
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata1_4_10)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2,                     //控制周期2ms
    .pwm_cycle = 4,                     //pwm周期 1 * 10us
    .h_pwm_duty = 0,					//h_pwm 占空比 0~100
    .l_pwm_duty = 0,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 0,           //pwm输出的时间
PWM_LED_PLATFORM_DATA_END()


//case2 : ALL ON
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata2)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2,                     //控制周期2ms
    .pwm_cycle = 4,                     //pwm周期 4 * 10us
    .h_pwm_duty = 100,					//h_pwm 占空比 0~100
    .l_pwm_duty = 100,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 2,           //pwm输出的时间2ms
PWM_LED_PLATFORM_DATA_END()


//case3 : LED_L ON
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata3)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 20,                    //控制周期 20ms
    .pwm_cycle = 4,                     //pwm周期 4 * 10us
    .h_pwm_duty =   0,					//h_pwm 占空比 0~100
    .l_pwm_duty = 100,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 20,          //pwm输出的时间20ms
PWM_LED_PLATFORM_DATA_END()


//case9 : LED_H ON
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata9)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 20,                    //控制周期 20ms
    .pwm_cycle = 4,                     //pwm周期 4 * 10us
    .h_pwm_duty = 100,					//h_pwm 占空比 0~100
    .l_pwm_duty =   0,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 20,          //pwm输出的时间20ms
PWM_LED_PLATFORM_DATA_END()


//case 5: LED_L 2s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata5)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2000,                  //控制周期 2s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =   0,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 11: LED_H 2s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata11)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2000,                  //控制周期 2s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =   0,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 6: LED_L 0.5s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata6)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle = 500,                   //控制周期 5 * 100 * 10us = 0.5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =   0,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 12: LED_H 0.5s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata12)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 500,                   //控制周期 0.5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =   0,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 7: LED_L 5s闪两下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata7)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 5000,                  //控制周期 5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =   0,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 1,							//每控制周期内输出2次pwm
    .out_twice.first_pwm_out_time  = 100,  //第一次pwm输出的时间
    .out_twice.pwm_gap_time        = 200,  //第一次和第二次的间隔时间
    .out_twice.second_pwm_out_time = 100,  //第二次pwm输出的时间
PWM_LED_PLATFORM_DATA_END()


//case 7: LED_H 5s闪两下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata13)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 5000,                  //控制周期 5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =   0,					//l_pwm 占空比 0~100
    .out_mode = 1,							//每控制周期内输出2次pwm
    .out_twice.first_pwm_out_time  = 100,  //第一次pwm输出的时间
    .out_twice.second_pwm_out_time = 100,  //第二次pwm输出的时间
    .out_twice.pwm_gap_time        = 200,  //第一次和第二次的间隔时间
PWM_LED_PLATFORM_DATA_END()

//case 8: LED_L 5s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata8)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 5000,                  //控制周期 5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =   0,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()

//case 14: LED_H 5s闪一下0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata14)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 5000,                  //控制周期 5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =   0,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 15:  LED_L LED_H  0.5s交替闪烁
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata15)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 500,                   //控制周期 0.5s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 16:  LED_L LED_H  2s交替闪烁
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata16)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2000,                  //控制周期 2s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()


//case 17:  LED_L 呼吸模式
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata17)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2700,                  //控制周期 2.7s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 2,							//呼吸模式
    .out_breathe.pwm_duty_max_keep_time = 200,//pwm最大占空比至少维持的时间
    .out_breathe.pwm_out_time = 1000,        //pwm输出的时间 1s
PWM_LED_PLATFORM_DATA_END()

//case 18:  LED_H 呼吸模式
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata18)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2700,                  //控制周期 2.7s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 2,							//呼吸模式
    .out_breathe.pwm_duty_max_keep_time = 200,//pwm最大占空比至少维持的时间
    .out_breathe.pwm_out_time = 1000,        //pwm输出的时间
PWM_LED_PLATFORM_DATA_END()


//case 19:  LED_H LED_L 交替呼吸模式
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata19)
    .port0 = IO_PORTA_03,               //引脚
    .port1 = -1,		                //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2700,                  //控制周期 2.7s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 2,							//呼吸模式
    .out_breathe.pwm_duty_max_keep_time = 200,//pwm最大占空比至少维持的时间
    .out_breathe.pwm_out_time = 1000,        //pwm输出的时间
PWM_LED_PLATFORM_DATA_END()

//case 19:  LED_H LED_L 双IO交替呼吸模式
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata20)
    .port0 = IO_PORTA_02,               //引脚
    .port1 = IO_PORTA_04,               //引脚

    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2700,                  //控制周期 2.7s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 2,							//呼吸模式
    .out_breathe.pwm_duty_max_keep_time = 200,//pwm最大占空比至少维持的时间
    .out_breathe.pwm_out_time = 1000,        //pwm输出的时间
PWM_LED_PLATFORM_DATA_END()

//case 19:  LED_H LED_L 双IO同时呼吸
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata21)
    .port0 = IO_PORTA_02,               //引脚
    .port1 = IO_PORTA_04,               //引脚

    .first_logic = 1,                   //先输出l_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2700,                  //控制周期 2.7s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 2,							//呼吸模式
    .out_breathe.pwm_duty_max_keep_time = 200,//pwm最大占空比至少维持的时间
    .out_breathe.pwm_out_time = 1000,        //pwm输出的时间
PWM_LED_PLATFORM_DATA_END()


//case 16:  LED_L LED_H  双IO 2s交替闪烁0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata22)
    .port0 = IO_PORTA_02,               //引脚
    .port1 = IO_PORTA_04,               //引脚
    .first_logic = 0,                   //先输出h_pwm
    .alternate_out = 1,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2000,                  //控制周期 2s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()

//case 16:  LED_L LED_H  双IO 2s同时闪烁0.1s
PWM_LED_PLATFORM_DATA_BEGIN(test_pwm_led_pdata23)
    .port0 = IO_PORTA_02,               //引脚
    .port1 = IO_PORTA_04,               //引脚
    .first_logic = 1,                   //先输出h_pwm
    .alternate_out = 0,                 //h_pwm和l_pwm每过一个周期交替输出
    .ctl_cycle_num = 0,                       //无限循环
    .ctl_cycle = 2000,                  //控制周期 2s
    .pwm_cycle = 50,                    //pwm周期 50 * 10us
    .h_pwm_duty =  50,					//h_pwm 占空比 0~100
    .l_pwm_duty =  50,					//l_pwm 占空比 0~100
    .out_mode = 0,							//每控制周期内输出1次pwm
    .out_once.pwm_out_time = 100,         //pwm输出的时间100ms
PWM_LED_PLATFORM_DATA_END()




void pwm_led_test(void)
{
    printf("******** pwm led test *************\n");

    /* pwm_led_hw_init((void *)&test_pwm_led_pdata1_4_10); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata2); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata3); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata9); */
    pwm_led_hw_init((void *)&test_pwm_led_pdata5);
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata11); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata6); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata12); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata7); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata13); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata8); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata14); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata15); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata16); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata17); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata18); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata19); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata20); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata21); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata22); */
    /* pwm_led_hw_init((void *)&test_pwm_led_pdata23); */


#if 1
    extern void wdt_clear();
    while (1) {
        wdt_clear();
    }
#endif
}




