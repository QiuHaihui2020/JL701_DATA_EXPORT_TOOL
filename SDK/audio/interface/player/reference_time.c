#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".reference_time.data.bss")
#pragma data_seg(".reference_time.data")
#pragma const_seg(".reference_time.text.const")
#pragma code_seg(".reference_time.text")
#endif
/*************************************************************************************************/
/*!
*  \file      reference_time.c
*
*  \brief     提供音频参考时钟选择的接口函数实体与管理
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/

#include "reference_time.h"
#include "system/includes.h"
/* #include "le_audio_stream.h" */

/*
 * 关于优先级选择：
 * 手机的网络时钟优先级最高，无手机网络的情况下选择TWS网络
 */
static LIST_HEAD(reference_head);
static u8 local_network = 0xff;
static u8 local_net_addr[6];
static u8 local_network_id = 1;
extern const int LE_AUDIO_TIME_ENABLE;

struct reference_clock {
    u8 id;
    u8 network;
    union {
        u8 net_addr[6];
        void *le_addr;
    };
    struct list_head entry;
};

extern void bt_audio_reference_clock_select(void *addr, u8 network);
extern u32 bt_audio_reference_clock_time(u8 network);
extern u32 bt_audio_reference_clock_remapping(void *src_addr, u8 src_network, void *dst_addr, u8 dst_network, u32 clock);
extern u8 bt_audio_reference_link_exist(void *addr, u8 network);

//le audio相关函数弱定义
__attribute__((weak))
int le_audio_stream_clock_select(void *le_audio)
{
    printf("empty fun %s\n", __FUNCTION__);
    return 0;
}
__attribute__((weak))
u32 le_audio_stream_current_time(void *le_audio)
{
    printf("empty fun %s\n", __FUNCTION__);
    return 0;
}
__attribute__((weak))
int le_audio_stream_get_latch_time(void *le_audio, u32 *time, u16 *us_1_12th, u32 *event)
{
    printf("empty fun %s\n", __FUNCTION__);
    return 0;
}
__attribute__((weak))
void le_audio_stream_latch_time_enable(void *le_audio)
{
    printf("empty fun %s\n", __FUNCTION__);
}


int audio_reference_clock_select(void *addr, u8 network)
{
    struct reference_clock *reference_clock = (struct reference_clock *)zalloc(sizeof(struct reference_clock));
    struct reference_clock *clk;
    u8 reset_clock = 1;

    local_irq_disable();

    if (network > 2) {
        local_irq_enable();
        free(reference_clock);
        return 0;
    }

    if (list_empty(&reference_head) || network == 0) {
        reference_clock->network = network;
        if (LE_AUDIO_TIME_ENABLE && network == 2) {
            reference_clock->le_addr = addr;
        } else {
            if (addr == NULL) {
                memset(reference_clock->net_addr, 0x0, sizeof(reference_clock->net_addr));
            } else {
                memcpy(reference_clock->net_addr, addr, sizeof(reference_clock->net_addr));
            }
        }
    } else {
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (clk->network == 0 && bt_audio_reference_link_exist(clk->net_addr, clk->network)) {
            reference_clock->network = clk->network;
            memcpy(reference_clock->net_addr, clk->net_addr, sizeof(reference_clock->net_addr));
            reset_clock = 0;
        } else {
            reference_clock->network = network;
            if (LE_AUDIO_TIME_ENABLE && network == 2) {
                reset_clock = (clk->network == 2 ? 0 : 1);//多路le_audio 参考时钟只需要设置一次;
                reference_clock->le_addr = addr;
            } else {
                if (addr == NULL) {
                    memset(reference_clock->net_addr, 0x0, sizeof(reference_clock->net_addr));
                } else {
                    memcpy(reference_clock->net_addr, addr, sizeof(reference_clock->net_addr));
                }
            }
        }
    }

    list_add(&reference_clock->entry, &reference_head);
    reference_clock->id = local_network_id;
    if (++local_network_id == 0) {
        local_network_id++;
    }
    local_irq_enable();

    if (reset_clock) {
        if (LE_AUDIO_TIME_ENABLE && reference_clock->network == 2) {
            le_audio_stream_clock_select(reference_clock->le_addr);
        } else {
            bt_audio_reference_clock_select(reference_clock->net_addr, reference_clock->network);
        }
    }

    return reference_clock->id;
}

u32 audio_reference_clock_time(void)
{
    if (!list_empty(&reference_head)) {
        struct reference_clock *clk;
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (LE_AUDIO_TIME_ENABLE && clk->network == 2) {
#if 1
            extern u32 bb_le_clk_get_time_us(void);
            return bb_le_clk_get_time_us();
#else
            return le_audio_stream_current_time(clk->le_addr);
#endif
        }
        return bt_audio_reference_clock_time(clk->network);
    }

    return bt_audio_reference_clock_time(local_network);
}

u32 audio_reference_network_clock_time(u8 network)
{
    return bt_audio_reference_clock_time(network);
}

u8 audio_reference_network_exist(u8 id)
{
    struct reference_clock *clk;
    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (clk->id == id) {
            goto exist_detect;
        }
    }

    local_irq_enable();
    return 0;

exist_detect:
    local_irq_enable();
    return bt_audio_reference_link_exist(clk->net_addr, clk->network);
}

int le_audio_get_reference_latch_time(u32 *time, u16 *us_1_12th, u32 *event)
{
    if (!LE_AUDIO_TIME_ENABLE) {
        return 0;
    }
    if (!list_empty(&reference_head)) {
        struct reference_clock *clk;
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (clk->network == 2) {
            le_audio_stream_get_latch_time(clk->le_addr, time, us_1_12th, event);
        }
    }
    return 0;
}

void le_audio_reference_time_latch_enable(void)
{
    if (!LE_AUDIO_TIME_ENABLE) {
        return;
    }

    if (!list_empty(&reference_head)) {
        struct reference_clock *clk;
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (clk->network == 2) {
            le_audio_stream_latch_time_enable(clk->le_addr);
        }
    }
}

u8 is_audio_reference_clock_enable(void)
{
    return 0;
    /*return local_network == 0xff ? 0 : 1;*/
}

u8 audio_reference_clock_network(void *addr)
{
    struct reference_clock *clk;

    if (list_empty(&reference_head)) {
        return -1;
    }

    clk = list_first_entry(&reference_head, struct reference_clock, entry);
    if (clk->network != 2 && addr) {
        memcpy(addr, clk->net_addr, 6);
    }

    return clk->network;
}

u8 audio_reference_clock_match(void *addr, u8 network)
{
    struct reference_clock *clk;

    if (list_empty(&reference_head)) {
        return 0;
    }
    clk = list_first_entry(&reference_head, struct reference_clock, entry);
    if (clk->network == network) {
        if (network == 0) {
            if (addr && memcmp(clk->net_addr, addr, 6) == 0) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

u32 audio_reference_clock_remapping(u8 now_network, u8 dst_network, u32 clock)
{
    void *now_addr = NULL;
    void *dst_addr = NULL;
    struct reference_clock *clk;

    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (!now_addr && clk->network == now_network) {
            now_addr = clk->net_addr;
        }
        if (!dst_addr && clk->network == dst_network) {
            dst_addr = clk->net_addr;
        }
    }
    local_irq_enable();

    return bt_audio_reference_clock_remapping(now_addr, now_network, dst_addr, dst_network, clock);
}

void audio_reference_clock_exit(u8 id)
{
    struct reference_clock *clk;
    local_irq_disable();
    list_for_each_entry(clk, &reference_head, entry) {
        if (clk->id == id) {
            goto delete;
        }
    }

    local_irq_enable();
    return;
delete:
    list_del(&clk->entry);
    free(clk);
    if (!list_empty(&reference_head)) {
        clk = list_first_entry(&reference_head, struct reference_clock, entry);
        if (LE_AUDIO_TIME_ENABLE && clk->network == 2) {
            le_audio_stream_clock_select(clk->le_addr);
        } else {
            bt_audio_reference_clock_select(clk->net_addr, clk->network);
        }
    }
    local_irq_enable();
}
