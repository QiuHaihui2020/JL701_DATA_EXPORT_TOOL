#ifndef GENERIC_CPU_H
#define GENERIC_CPU_H

#include <asm/cpu.h>

__attribute__((always_inline))
static inline u32 ucPortCountLeadingZeros(u32 ulBitmap)
{
    u32 ucReturn;
    __asm__ volatile("%0 = clz(%1)" : "=r"(ucReturn) : "r"(ulBitmap));
    return ucReturn;
}

extern const int config_asser;
extern void cpu_assert(char *file, int line, bool condition, char *cond_str);
extern void cpu_assert_debug();

/* extern u32 rets_log[100];                               */
/* extern u32 g_rets_idx;                                  */
/* #define assert_d(a,...)   \                             */
/*         do { \                                          */
/*             if(config_asser){\                          */
/*                 if(!(a)){ \                             */
/*                     printf("ASSERT : "__VA_ARGS__); \   */
/*                     {\                                  */
/*                         u32 rets;\                      */
/*                         asm("%0 = rets":"=r"(rets));\   */
/*                         rets_log[g_rets_idx++] = rets;\ */
/*                     }\                                  */
/*                 } \                                     */
/*             } \                                         */
/*         }while(0);                                      */


//release 会被优化掉，不生成任何代码
#define assert_d(a,...)   \
        do { \
            if(config_asser){\
                if(!(a)){ \
					printf("cpu %d file:%s, line:%d",current_cpu_id(), __FILE__, __LINE__); \
					printf("ASSERT-FAILD: "#a" "__VA_ARGS__); \
                    {\
                        while(1);\
                    }\
                } \
            } \
        }while(0);



#define ASSERT(a,...)   \
		do { \
			if(config_asser){\
				if(!(a)){ \
					printf("cpu %d file:%s, line:%d",current_cpu_id(), __FILE__, __LINE__); \
					printf("ASSERT-FAILD: "#a" "__VA_ARGS__); \
                    cpu_assert(__FILE__, __LINE__, a ? true : false, #a); \
				} \
			}else {\
				if(!(a)){ \
                    cpu_assert(NULL, __LINE__, a ? true : false, #a); \
				}\
			}\
		}while(0);

#define ASSERT_RESET(a) \
		do { \
			if(config_asser){\
				if(!(a)){ \
					printf("cpu %d file:%s, line:%d",current_cpu_id(), __FILE__, __LINE__); \
					printf("ASSERT-FAILD: "#a" "__VA_ARGS__); \
                    cpu_assert(__FILE__, __LINE__, a ? true : false, #a); \
				} \
			}else {\
				if(!(a)){ \
                    cpu_assert(NULL, __LINE__, a ? true : false, #a); \
                    // cpu_assert(__FILE__, __LINE__, a ? true : false, #a); \
}\
}\
} while (0);


#if defined(CONFIG_256K_FLASH) && defined(CONFIG_RELEASE_ENABLE)
#undef ASSERT
#define ASSERT(a,...)
#endif

#endif

