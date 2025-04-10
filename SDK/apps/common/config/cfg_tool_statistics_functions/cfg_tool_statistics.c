#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".cfg_tool_statistics.data.bss")
#pragma data_seg(".cfg_tool_statistics.data")
#pragma const_seg(".cfg_tool_statistics.text.const")
#pragma code_seg(".cfg_tool_statistics.text")
#endif
#include "cfg_tool.h"
#include "app_config.h"

#if CFG_TOOL_VER == CFG_TOOL_VER_VISUAL
#include "system/malloc.h"
#include "system/task.h"
#include "cpu.h"

/* #if (defined CONFIG_CPU_BR27) */
/* #include "asm/cache.h" */
/* #endif */

#endif

#define TEMP_BUF_SIZE								256
#define TASK_INFO_NUM_MAX_SIZE						5
#define MAX_TASKS_INFO_LIST_NUM						32

#if TCFG_CFG_TOOL_ENABLE && (CFG_TOOL_VER == CFG_TOOL_VER_VISUAL)

static SIMPLE_TASK_INFO *_simple_task_info_list = NULL;
static u16 _simple_task_info_list_valid_len;
static CACHE_INFO *_cache_info_list = NULL;	// (cpu's icache) + dcache

static void cfg_tool_statistics_callback(u8 *packet, u32 size)
{
    u32 send_len = 0;
    u8 *buf = NULL;

    u32 cmd = packet_combined(packet, 2);

    switch (cmd) {
    case ONLINE_SUB_OP_REFRESH_TASK_INFO_LIST:
        if (_simple_task_info_list == NULL) {
            _simple_task_info_list = malloc(sizeof(SIMPLE_TASK_INFO) * MAX_TASKS_INFO_LIST_NUM);
            if (!_simple_task_info_list) {
                printf("_simple_task_info_list malloc err!\n");
                send_len = 0;
                break;
            }
        }
        _simple_task_info_list_valid_len = simple_task_info_list_get(_simple_task_info_list, MAX_TASKS_INFO_LIST_NUM);
        send_len = sizeof(_simple_task_info_list_valid_len);
        buf = send_buf_malloc(send_len);
        memcpy(buf, (void *)&_simple_task_info_list_valid_len, send_len);
        break;
    case ONLINE_SUB_OP_GET_TASK_INFO_LIST:
        u32 index = packet_combined(packet, 6);
        g_printf("index:%d\n", index);
        if (index < _simple_task_info_list_valid_len) {
            u32 task_info_num = 0;
            u32 task_info_size = sizeof(SIMPLE_TASK_INFO);
            send_len += sizeof(u32);
            for (u32 _index = index; (_index < _simple_task_info_list_valid_len) && (task_info_num < TASK_INFO_NUM_MAX_SIZE); _index++, task_info_num++) {
                send_len += task_info_size;
            }
            task_info_num = 0;
            buf = send_buf_malloc(send_len);
            for (u32 _index = index; (_index < _simple_task_info_list_valid_len) && (task_info_num < TASK_INFO_NUM_MAX_SIZE); _index++, task_info_num++) {
                memcpy(buf + sizeof(u32) + task_info_size * task_info_num, &_simple_task_info_list[_index], task_info_size);
            }
            memcpy(buf, &task_info_num, sizeof(u32));
            g_printf("info_num:%d %d\n", task_info_num);
        } else {
            send_len = 0;
        }
        break;
    case ONLINE_SUB_OP_GET_CACHE_INFO_LIST:
#if (defined CONFIG_CPU_BR27 || defined CONFIG_CPU_BR28)
        u32 cache_list_len = (CPU_CORE_NUM + 1);
        u32 cache_info_size = sizeof(CACHE_INFO);
        if (_cache_info_list == NULL) {
            _cache_info_list = malloc(cache_info_size * cache_list_len);	// (cpu's icache) + dcache
            if (!_cache_info_list) {
                printf("_cache_info_list malloc err!\n");
                send_len = 0;
                break;
            }
        }
        cache_info_list_get(_cache_info_list, cache_list_len);
        send_len += sizeof(u32);
        send_len += cache_info_size * cache_list_len;
        buf = send_buf_malloc(send_len);
        for (u32 i = 0; i < cache_list_len; i++) {
            memcpy(buf + sizeof(u32) + cache_info_size * i, &_cache_info_list[i], cache_info_size);
        }
        memcpy(buf, &cache_list_len, sizeof(u32));
#else
        send_len = 0;
#endif
        break;
    default: // DEFAULT_ACTION
        free(buf);
        return;
        break;
    }
    all_assemble_package_send_to_pc(REPLY_STYLE, packet[1], buf, send_len);
    free(buf);
}

REGISTER_DETECT_TARGET(cfg_tool_statistics_target) = {
    .id         		= VISUAL_CFG_TOOL_CHANNEL_STYLE,
    .tool_message_deal  = cfg_tool_statistics_callback,
};

/**
 * @brief 清理配置工具的统计资源
 */
void cfg_tool_statistics_resource_release()
{
    if (_simple_task_info_list) {
        free(_simple_task_info_list);
    }
    if (_cache_info_list) {
        free(_cache_info_list);
    }
    _simple_task_info_list = NULL;
    _cache_info_list = NULL;
}

#endif


