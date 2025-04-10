#include "audio_debug.h"
#include "system/timer.h"
#include "audio_config.h"
#include "system/task.h"
#include "jlstream.h"
#include "classic/hci_lmp.h"


#define AUD_CFG_DUMP_ENABLE				1	//音频配置跟踪使能
#define AUD_REG_DUMP_ENABLE				0	//音频寄存器跟踪使能
#define AUD_CACHE_INFO_DUMP_ENABLE		0 	//cache信息跟踪使能
#define AUD_TASK_INFO_DUMP_ENABLE		0	//任务运行信息跟踪使能
#define AUD_JLSTREAM_MEM_DUMP_ENABLE	0	//jlstream内存跟踪
#define AUD_BT_INFO_DUMP_ENABLE			0	//蓝牙音频流跟踪

static void audio_config_trace(void *priv)
{
    printf(">>Audio_Config_Trace:\n");
#if AUD_CFG_DUMP_ENABLE
    audio_config_dump();
#endif

#if AUD_REG_DUMP_ENABLE
    audio_adda_dump();
#endif

#if AUD_CACHE_INFO_DUMP_ENABLE
    extern void CacheReport(void);
    CacheReport();
#endif

#if AUD_TASK_INFO_DUMP_ENABLE
    task_info_output(0);
#endif

#if AUD_JLSTREAM_MEM_DUMP_ENABLE
    stream_mem_unfree_dump();
#endif

#if AUD_BT_INFO_DUMP_ENABLE
    printf("ESCO Tx Packet Num:%d", lmp_private_get_esco_tx_packet_num());
#endif
}

void audio_config_trace_setup(int interval)
{
    sys_timer_add(NULL, audio_config_trace, interval);
}
