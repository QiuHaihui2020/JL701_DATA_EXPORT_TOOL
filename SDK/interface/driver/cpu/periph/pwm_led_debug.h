#ifndef __PWM_LED_DEBUG_H__
#define __PWM_LED_DEBUG_H__

//发布模块注释这个宏
//测试的时候建议关了debug打印来测试，卡好时序
// #define PWM_LED_DEBUG_ENABLE

//0=不开demo
#define PWM_LED_TEST_MODE 0
//singer
// #define PWM_LED_TEST_MODE 1
// display name  搜索这个字段看每个灯效的行为
//tws
// #define PWM_LED_TEST_MODE 2

//测试灯效属性互相切换是否正常
// #define PWM_LED_TEST_MODE 3

// #define PWM_LED_TEST_MODE 4


// #define PWM_PAIRED_LED_UUID 0xb37e
// #define PWM_PAIRED_LED_UUID 0xc3eb
#define PWM_PAIRED_LED_UUID 0x63b8


#ifdef PWM_LED_DEBUG_ENABLE
//不开启局部打印
#if 1
#define PWMLED_LOG_DEBUG(fmt, ...) 	g_printf("[PWM_LED_DEBUG]\\(^o^)/" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#define PWMLED_LOG_BUF put_buf

#define PWMLED_HW_LOG_INFO(fmt, ...) 	b_printf("[PWM_HW_LED_INFO](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_HW_LOG_DEBUG(fmt, ...) 	y_printf("[PWM_HW_LED_DEBG](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_HW_LOG_SUCC(fmt, ...) 	r_printf("[PWM_HW_LED_SUCC](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_HW_LOG_ERR(fmt, ...) 	r_printf("[PWM_HW_LED_ERR](>_<)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#else

#define PWMLED_LOG_INFO printf
#define PWMLED_LOG_DEBUG(...)
#define PWMLED_LOG_SUCC(...)
#define PWMLED_LOG_ERR(...)


#define PWMLED_HW_LOG_INFO(...)
#define PWMLED_HW_LOG_DEBUG(...)
#define PWMLED_LOG_BUF(...)
#endif

//
#if 1
#define PWMLED_LOG_INFO(fmt, ...) 	y_printf("[PWM_LED_INFO] " fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)

#define PWMLED_LOG_SUCC(fmt, ...) 	r_printf("[PWM_LED_SUCC](^_^)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#define PWMLED_LOG_ERR(fmt, ...) 		r_printf("[PWM_LED_ERR](>_<)" fmt "\t|\tfun:%s: line:%d", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#endif //#if 0
//

#else
//发布模块
#define PWMLED_LOG_INFO       printf
#define PWMLED_LOG_DEBUG(...)
#define PWMLED_LOG_SUCC(...)
#define PWMLED_LOG_ERR(...)

#define PWMLED_LOG_BUF(...)

#define PWMLED_HW_LOG_INFO  printf
#define PWMLED_HW_LOG_DEBUG(...)
#define PWMLED_HW_LOG_SUCC(...)
#define PWMLED_HW_LOG_ERR(...)
#endif


//*********************************************************************************
//#define PWM_LED_IO_TEST

// #define PWM_LED_IO_TEST_DEMO
//#define PWM_LED_IO_TEST_SDK

#ifdef PWM_LED_IO_TEST
#define LED_TIMER_TEST_PORT (JL_PORTC)
#define LED_TIMER_TEST_PIN  (5)

#define LED_TIMER_TEST_PORT1 (JL_PORTC)
#define LED_TIMER_TEST_PIN1  (6)
#endif

struct led_mode {
    u16 uuid;
    char *name;
};
#endif /* #ifndef __PWM_LED_DEBUG_H__ */

