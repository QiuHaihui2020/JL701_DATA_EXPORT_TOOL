
/*
 ****************************************************************
 *							AUDIO EVENT HANDLER
 * File  : audio_event_handlr.c
 * Notes : 处理AUDIO_LIB 关联APP的事件
 *
 ****************************************************************
 */

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_event_handler.data.bss")
#pragma data_seg(".audio_event_handler.data")
#pragma const_seg(".audio_event_handler.text.const")
#pragma code_seg(".audio_event_handler.text")
#endif

#include "audio_event_manager.h"
#include "generic/typedef.h"
#include "syscfg_id.h"
#include "user_cfg_id.h"
#include "power/power_manage.h"

int audio_event_notify(enum audio_lib_event event, void *priv, int arg)
{
    switch (event) {
    case AUDIO_LIB_EVENT_DAC_LOWPOWER:
        /* printf(">>> dac off check %d %d!\n", low_power_sys_not_idle_cnt(), lower_power_bt_group_query()); */
        //当系统剩余1个没有进入低功耗，且蓝牙已经进入低功耗，则可以关闭DAC
        return ((low_power_sys_not_idle_cnt() == 1) && \
                (lower_power_bt_group_query() > 0));
    case AUDIO_LIB_EVENT_VBG_TRIM_WTITE:
        return syscfg_write(CFG_VBG_TRIM, priv, arg);
    case AUDIO_LIB_EVENT_VBG_TRIM_READ:
        return syscfg_read(CFG_VBG_TRIM, priv, arg);
    default:
        break;
    }
    return 0;
}
