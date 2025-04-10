#include "two_io_led.h"
#include "gpio.h"


// *INDENT-OFF*

//ALL OFF
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .ctl_option = 4,
TWO_IO_CTL_PLATFORM_DATA_END()

//ALL ON
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data0)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 100,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data1)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 50,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 ON
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data2)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data3)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED1 ON
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data4)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data5)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 3,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 2s闪一下0.1s
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data6)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data7)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED1 2s闪一下0.1s
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data8)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data9)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 5s闪两下0.1s
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data10)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 4700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data11)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 4700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED1 5s闪两下0.1s
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data12)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 4700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data13)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 4700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1  2s交替闪烁1次
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data14)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 100,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 2,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data15)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 50,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 2,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1  2s交替闪烁2次
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data16)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 100,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 2,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 1700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data17)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 50,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 2,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 1700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1  2s同时闪烁1次
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data18)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 100,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data19)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 50,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 0,
    .cycle_once.idle_time = 1900,
    .cycle_once.out_time = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1  2s同时闪烁2次
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data20)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 100,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 1700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data21)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 50,
    .io1_pwm_duty = 50,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 1,
    .cycle_twice.idle_time = 1700,
    .cycle_twice.out_time = 100,
    .cycle_twice.gap_time = 100,
    .cycle_twice.out_time1 = 100,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 呼吸模式
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data22)
    .io0 = IO_PORTC_04,
    .io0_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 0,
    .ctl_mode = 2,
    .cycle_breathe.idle_time = 2000,
    .cycle_breathe.duty_step = 10,
    .cycle_breathe.duty_step_time = 1000,
    .cycle_breathe.duty_keep_time = 1000,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED1 呼吸模式
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data23)
    .io1 = IO_PORTC_05,
    .io1_pwm_duty = 100,
    .soft_pwm_cycle = 10,
    .ctl_option = 1,
    .ctl_mode = 2,
    .cycle_breathe.idle_time = 2000,
    .cycle_breathe.duty_step = 10,
    .cycle_breathe.duty_step_time = 1000,
    .cycle_breathe.duty_keep_time = 1000,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1 交替呼吸模式
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data24)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 80,
    .io1_pwm_duty = 80,
    .soft_pwm_cycle = 10,
    .ctl_option = 2,
    .ctl_mode = 2,
    .cycle_breathe.idle_time = 2000,
    .cycle_breathe.duty_step = 10,
    .cycle_breathe.duty_step_time = 1000,
    .cycle_breathe.duty_keep_time = 1000,
TWO_IO_CTL_PLATFORM_DATA_END()

//LED0 LED1 同时呼吸
TWO_IO_CTL_PLATFORM_DATA_BEGIN(two_io_data25)
    .io0 = IO_PORTC_04,
    .io1 = IO_PORTC_05,
    .io0_pwm_duty = 80,
    .io1_pwm_duty = 80,
    .soft_pwm_cycle = 10,
    .ctl_option = 3,
    .ctl_mode = 2,
    .cycle_breathe.idle_time = 2000,
    .cycle_breathe.duty_step = 10,
    .cycle_breathe.duty_step_time = 1000,
    .cycle_breathe.duty_keep_time = 1000,
TWO_IO_CTL_PLATFORM_DATA_END()


void two_io_ctl_test(void)
{
    printf("******** two_io_ctl test *************\n");

    /* two_io_ctl_init(&two_io_data0); */
    /* two_io_ctl_init(&two_io_data1); */
    /* two_io_ctl_init(&two_io_data2); */
    /* two_io_ctl_init(&two_io_data3); */
    /* two_io_ctl_init(&two_io_data4); */
    /* two_io_ctl_init(&two_io_data5); */
    two_io_ctl_init(&two_io_data6);
    /* two_io_ctl_init(&two_io_data7); */
    /* two_io_ctl_init(&two_io_data8); */
    /* two_io_ctl_init(&two_io_data9); */
    /* two_io_ctl_init(&two_io_data10); */
    /* two_io_ctl_init(&two_io_data11); */
    /* two_io_ctl_init(&two_io_data12); */
    /* two_io_ctl_init(&two_io_data13); */
    /* two_io_ctl_init(&two_io_data14); */
    /* two_io_ctl_init(&two_io_data15); */
    /* two_io_ctl_init(&two_io_data16); */
    /* two_io_ctl_init(&two_io_data17); */
    /* two_io_ctl_init(&two_io_data18); */
    /* two_io_ctl_init(&two_io_data19); */
    /* two_io_ctl_init(&two_io_data20); */
    /* two_io_ctl_init(&two_io_data21); */
    /* two_io_ctl_init(&two_io_data22); */
    /* two_io_ctl_init(&two_io_data23); */
    /* two_io_ctl_init(&two_io_data24); */
    /* two_io_ctl_init(&two_io_data25); */
}




