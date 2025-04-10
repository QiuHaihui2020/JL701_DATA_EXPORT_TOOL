#ifndef TASK_PRIORITY_H
#define TASK_PRIORITY_H


#include "os/os_api.h"


struct task_info {
    const char *name;
    u8 prio;
    u8 core;
    u16 stack_size;
    u16 qsize;
};


typedef OS_SEM  	sem_t;
typedef OS_MUTEX  	mutex_t;


/* --------------------------------------------------------------------------*/
/**
 * @brief 创建任务
 *
 * @param task 服务函数
 * @param p 服务函数传参
 * @param name 任务名
 *
 * @return 0：成功，其他：错误码
 */
/* ----------------------------------------------------------------------------*/
int task_create(void (*task)(void *p), void *p, const char *name);

int __task_create(void (*task)(void *p), void *p, const char *name, u8 prio);
/* --------------------------------------------------------------------------*/
/**
 * @brief 删除任务
 *
 * @param name 任务名
 *
 * @return 0：成功，其他：错误码
 */
/* ----------------------------------------------------------------------------*/
int task_kill(const char *name);
int task_delete(const char *name, OS_SEM *sem);

void task_info_init(void);
void task_info_add(TaskHandle_t task);
void task_info_del(const char *name);
void task_info_update_runtime(TaskHandle_t old_task, TaskHandle_t new_task);
void task_info_reset(void);
void task_info_output(int size);
void task_info_output_debug(void);
u32 task_info_idle_percentage(void);
void *task_info_list_head_get(void);
int os_task_priority(const char *name);
int os_cpu_usage(const char *task_name, int *usage);

int os_hw_sem_pend(OS_SEM *sem, int timeout, const char *hw_name);
int os_hw_sem_post(OS_SEM *sem, const char *hw_name);
int os_get_hw_usage(const char *hw_name);

/**
 * @brief 输出指定任务的运行信息，指定的任务必须是在停止运行的过程中
 *
 * @param task_name
 *
 * @return
 */
int task_trace_info_dump(const char *task_name);

#define SIMPLE_TASK_INFO_NAME_MAX_LEN 16
typedef struct __simple_task_info {
    char task_name[SIMPLE_TASK_INFO_NAME_MAX_LEN];		// 任务名
    u32 stack_used;										// 已使用栈大小
    u32 stack_max;										// 最大栈
    u32 task_usage;										// CPU占用
} SIMPLE_TASK_INFO;
/* --------------------------------------------------------------------------*/
/**
 * @brief 获取记录的任务个数
 *
 * @result
 */
/* ----------------------------------------------------------------------------*/
u32 get_task_info_list_num(void);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取简易的任务信息
 *
 * @param task_info_list
 * @param task_info_list_len
 *
 * @result task_info_list的有效长度，小于等于task_info_list_len
 */
/* ----------------------------------------------------------------------------*/
u16 simple_task_info_list_get(SIMPLE_TASK_INFO *task_info_list, u32 task_info_list_len);

#endif


