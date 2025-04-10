#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".clock_manager.data.bss")
#pragma data_seg(".clock_manager.data")
#pragma const_seg(".clock_manager.text.const")
#pragma code_seg(".clock_manager.text")
#endif

#include "clock_manager.h"
#include "system/init.h"
#include "system/timer.h"
#include "system/task.h"
#include "os/os_api.h"
#include "clock.h"
#include "list.h"
#include "app_config.h"
#include "jlstream.h"

#define LOG_TAG             "[clock-manager]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define CLOCK_DETECT_PERIOD      (4 * 1000)         //4s
#define CLOCK_DETECT_COUNTER     (7)                //7 times

#define CLOCK_MINIMUM_FREQ       (24  * 1000000)    //24MHz
#define CLOCK_MAXIMUM_FREQ       clk_get_max_frequency()



typedef struct clock_manager_st {
    struct list_head entry;
    u32 freq;
    u32 name;
} clock_manager_item;


static u8 ref_cnt;
static u8 clk_adjust_step = 0;
static u16 clk_ref_timer;
static clock_manager_item clk_locker;
static struct list_head clk_mgr_head = LIST_HEAD_INIT(clk_mgr_head);

static const int clock_table[] = {
    24 * MHz, 48 * MHz, 60 * MHz, 64 * MHz, 76 * MHz,
    80 * MHz, 96 * MHz, 120 * MHz, 128 * MHz, 140 * MHz, 160 * MHz,
    200 * MHz, 240 * MHz, 280 * MHz, 320 * MHz,
};

static u32 __get_clock(u32 dest_clk)
{
    for (int i = 0; i < ARRAY_SIZE(clock_table); i++) {
        if (dest_clk <= clock_table[i]) {
            return clock_table[i];
        }
    }
    return clock_table[ARRAY_SIZE(clock_table) - 1];
}

#ifdef CONFIG_EARPHONE_CASE_ENABLE
static u8 cpu_usage_limit[] = {75, 80, 85};
#else
static u8 cpu_usage_limit[] = {50, 55, 60};
#endif


#if 0
#define CLOCK_MANAGER_INIT_CRITICAL()
#define CLOCK_MANAGER_ENTER_CRITICAL()      local_irq_disable()
#define CLOCK_MANAGER_EXIT_CRITICAL()       local_irq_enable()
#else
static OS_MUTEX clk_mutex;
#define CLOCK_MANAGER_INIT_CRITICAL()       os_mutex_create(&clk_mutex)
#define CLOCK_MANAGER_ENTER_CRITICAL()      os_mutex_pend(&clk_mutex, 0)
#define CLOCK_MANAGER_EXIT_CRITICAL()       os_mutex_post(&clk_mutex)
#endif

extern void core_voltage_dump();        //debug info api

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_alloc，此函数会触发时钟频率设置
 *
 * @param name
 * @param clk
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_alloc(const char *name, u32 clk)
{
    clock_manager_item *p;
    u32 hash = JBHash((u8 *)name, strlen(name));

    CLOCK_MANAGER_ENTER_CRITICAL();

    list_for_each_entry(p, &clk_mgr_head, entry) {
        if (p->name == hash) {
            CLOCK_MANAGER_EXIT_CRITICAL();
            return -1;
        }
    }

    clock_manager_item *it = malloc(sizeof(clock_manager_item));
    if (it == NULL) {
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    it->name = hash;
    it->freq = clk;

    list_add_tail(&it->entry, &clk_mgr_head);

    clock_refurbish();

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_free，此函数会触发时钟频率设置
 *
 * @param name
 *
 * @return 0:succ
 */
/* ----------------------------------------------------------------------------*/
int clock_free(const char *name)
{
    clock_manager_item *p, *n;
    u32 hash = JBHash((u8 *)name, strlen(name));

    CLOCK_MANAGER_ENTER_CRITICAL();

    list_for_each_entry_safe(p, n, &clk_mgr_head, entry) {
        if (p->name == hash) {
            __list_del_entry(&p->entry);
            free(p);
            continue;
        }
    }

    clock_refurbish();

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_dump
 */
/* ----------------------------------------------------------------------------*/
static u32 clock_list_sum(void)
{
    clock_manager_item *p;
    u32 total = 0;

#ifdef CONFIG_EARPHONE_CASE_ENABLE
    list_for_each_entry(p, &clk_mgr_head, entry) {
        if (total < p->freq) {
            total = p->freq;
        }
    }
#else
    list_for_each_entry(p, &clk_mgr_head, entry) {
        /*log_info("%s : %dHz", p->name, p->freq);*/
        total += p->freq;
    }
#endif
    if (total < CLOCK_MINIMUM_FREQ) {
        total = CLOCK_MINIMUM_FREQ;
    }
    /*log_info("%s : %dHz", "total", total);*/

    return total;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_test
 */
/* ----------------------------------------------------------------------------*/
void clock_manager_test(void)
{
    clock_alloc("clk_mgr_1", 2 * 1000000U);
    clock_manager_dump();

    clock_alloc("clk_mgr_2", 2 * 1000000U);
    clock_manager_dump();

    clock_free("clk_mgr_1");
    clock_manager_dump();

    clock_free("clk_mgr_2");
    clock_manager_dump();

    log_info("test over");
    while (1);
}


void clock_lock_dump(void)
{
    log_info("clock_lock owner : %x", clk_locker.name);
    log_info("clock_lock freq  : %d", clk_locker.freq);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_lock
 *
 * @param name: owner name
 * @param clk: lock_freq
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_lock(const char *name, u32 clk)
{
    u32 hash = JBHash((u8 *)name, strlen(name));

    CLOCK_MANAGER_ENTER_CRITICAL();

    if (clk_locker.freq) {
        //has been locked, lock fail
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    clk_locker.freq = clk;
    clk_locker.name = hash;

#if (defined TCFG_FIX_CLOCK_FREQ  && TCFG_FIX_CLOCK_FREQ)
    clk_set_api("sys", TCFG_FIX_CLOCK_FREQ);
#else
    clk_set_api("sys", clk);
#endif

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief clock_unlock
 *
 * @param name: owner name
 *
 * @return 0:succ, other:err code
 */
/* ----------------------------------------------------------------------------*/
int clock_unlock(char *name)
{
    u32 hash = JBHash((u8 *)name, strlen(name));

    log_info("%s", __func__);

    CLOCK_MANAGER_ENTER_CRITICAL();

    clock_lock_dump();

    if (clk_locker.freq == 0) {
        //has been locked
        /* ASSERT(0, "please lock it befor unlock"); */
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -1;
    }

    if (clk_locker.name == hash) {
        /* ASSERT(0, "locker owner is %s", clk_locker.name); */
        CLOCK_MANAGER_EXIT_CRITICAL();
        return -2;
    }

    clk_locker.freq = 0;

    //refurbish clock after unlock succ
    clock_refurbish();

    CLOCK_MANAGER_EXIT_CRITICAL();

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_reflash
 */
/* ----------------------------------------------------------------------------*/

/* --------------------------------------------------------------------------*/
/**
 * @brief clk_ref_cal
 */
/* ----------------------------------------------------------------------------*/
static void clk_ref_cal(void)
{
#if 0   //效率统计输出
    task_info_output(0);
#endif

    int usage[3] = { 0, 0, 0 };
    int a = os_cpu_usage(NULL, usage);
    task_info_reset();

    if (a < 0) {
        return;
    }
    int usage_max = MAX(usage[0], usage[1]);
#ifdef CONFIG_EARPHONE_CASE_ENABLE
    usage[2] = jlstream_get_cpu_usage();
    if (usage_max < usage[2]) {
        usage_max = usage[2];
    }
#endif

    int curr_clk = clk_get("sys");
    int dest_clk = curr_clk;

#if (defined TCFG_FIX_CLOCK_FREQ  && TCFG_FIX_CLOCK_FREQ)
    //固定时钟只进行效率统计
    r_printf("cpu: %d %d  clk:%d\n", usage[0], usage[1], curr_clk);
    return;
#endif

__again:
    switch (clk_adjust_step) {
    case 0:
        dest_clk = __get_clock(curr_clk / 50 * usage_max);
        clk_adjust_step = 1;
        break;
    case 1:
        if (usage_max < cpu_usage_limit[0]) {
            dest_clk = __get_clock(curr_clk / cpu_usage_limit[1] * usage_max);
        } else if (usage_max > cpu_usage_limit[2]) {
            dest_clk = __get_clock(curr_clk / (cpu_usage_limit[1] - 10) * usage_max);
        } else {
            clk_adjust_step = 2;
        }
        break;
    case 2:
        if (usage_max >= cpu_usage_limit[0] && usage_max <= cpu_usage_limit[2]) {
            break;
        }
        clk_adjust_step = 1;
        goto __again;
    }

    int min_clk = clock_list_sum();
    if (dest_clk < min_clk) {
        dest_clk = min_clk;
    }

    log_info("cpu0: %d%% cpu1: %d%% jlstream: %d%% curr_clk:%d  min_clk:%d dest_clk:%d, %d\n",
             usage[0], usage[1], usage[2], curr_clk, min_clk, dest_clk, clk_adjust_step);

    //clock lock
    if (clk_locker.freq) {
        dest_clk = clk_locker.freq;
    } else if (dest_clk < min_clk) {
        dest_clk = min_clk;
    }
    if (dest_clk > CLOCK_MAXIMUM_FREQ) {
        dest_clk = CLOCK_MAXIMUM_FREQ;
    }

    clk_set_api("sys", dest_clk);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clk_ref_fun
 *
 * @param p
 */
/* ----------------------------------------------------------------------------*/
static void clk_ref_fun(void *p)
{
    CLOCK_MANAGER_ENTER_CRITICAL();
    if (ref_cnt < CLOCK_DETECT_COUNTER) {
        /* log_info("CLOCK_DETECT_COUNTER:%d\n", ref_cnt); */
        ref_cnt++;
        clk_ref_cal();
    } else {
        sys_timer_del(clk_ref_timer);
        clk_ref_timer = 0;
    }
    CLOCK_MANAGER_EXIT_CRITICAL();
}

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_refurbish，MIPS变化时调用此函数刷新。
 */
/* ----------------------------------------------------------------------------*/
void clock_refurbish(void)
{
    if (cpu_in_irq() || cpu_irq_disabled()) {
        //以上情况，需要改为在APP任务上设置
        int msg[3];
        msg[0] = (int)clock_refurbish;
        msg[1] = 1;
        msg[2] = 0;
        os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        /* sys_timeout_add(NULL, (void *)clock_refurbish, 1); */
        return;
    }

    CLOCK_MANAGER_ENTER_CRITICAL();

    //clk driver 需要提供每个芯片可以设置的最高挡位
#if (defined TCFG_FIX_CLOCK_FREQ  && TCFG_FIX_CLOCK_FREQ)
    clk_set_api("sys", TCFG_FIX_CLOCK_FREQ);
#else
    clk_set_api("sys", CLOCK_MAXIMUM_FREQ);
#endif
    ref_cnt = 0;
    clk_adjust_step = 0;

    if (clk_ref_timer) {
        sys_timer_modify(clk_ref_timer, CLOCK_DETECT_PERIOD);
    } else {
        clk_ref_timer = sys_timer_add(NULL, clk_ref_fun, CLOCK_DETECT_PERIOD);
    }

    ASSERT(clk_ref_timer);

    task_info_reset();

    CLOCK_MANAGER_EXIT_CRITICAL();
}

_NOINLINE_
int clk_set_unused(const char *name, int clk)
{
    return 0;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief init
 */
/* ----------------------------------------------------------------------------*/
static int clock_manager_init(void)
{
    CLOCK_MANAGER_INIT_CRITICAL();
    return 0;
}
early_initcall(clock_manager_init);

/* --------------------------------------------------------------------------*/
/**
 * @brief clock_manager_test
 */
/* ----------------------------------------------------------------------------*/
#if 0   //test code
static void clk_test2_tmr_fun(void *p)
{
    clk_set_api("sys", CLOCK_MAXIMUM_FREQ);

    clock_refurbish();
}
void clock_manager_test2(void)
{
    sys_timer_add(NULL, clk_test2_tmr_fun, 60 * 1000);
}
static void clk_test3_tmr_fun(void *p)
{
    /* int ret; */
    clock_unlock("test");
    /* ASSERT(ret == 0); */
}
void clock_manager_test3(void)
{
    int ret;
    ret = clock_lock("test", 128 * 1000000U);
    ASSERT(ret == 0);
    sys_timeout_add(NULL, clk_test3_tmr_fun, 30 * 1000);

    /* void mpu_set(int idx, u32 begin, u32 end, u32 inv, const char *format, ...); */
    /* u32 addr = (u32)(&clk_locker.freq); */
    /* mpu_set(2, addr, addr+3, 0, "Cr"); */
}
#endif
