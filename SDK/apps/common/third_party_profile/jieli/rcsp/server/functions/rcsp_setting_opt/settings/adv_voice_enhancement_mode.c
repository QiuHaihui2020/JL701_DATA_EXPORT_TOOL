#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_voice_enhancement_mode.data.bss")
#pragma data_seg(".adv_voice_enhancement_mode.data")
#pragma const_seg(".adv_voice_enhancement_mode.text.const")
#pragma code_seg(".adv_voice_enhancement_mode.text")
#endif
#include "adv_voice_enhancement_mode.h"
#include "app_config.h"
#include "rcsp_adv_bluetooth.h"

#if (RCSP_ADV_ANC_VOICE && RCSP_ADV_WIND_NOISE_DETECTION)

/**
 * @brief 获取人声增强模式开关
 *
 * @result bool
 */
bool get_voice_enhancement_mode_switch()
{
    printf("===%s===", __FUNCTION__);
    return true;
}

/**
 * @brief 设置人声增强模式开关
 */
void set_voice_enhancement_mode_switch(bool mode_switch)
{
    printf("%s, switch:%d\n", __FUNCTION__, mode_switch);
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_VOICE_ENHANCEMENT_MODE, NULL, 0);
}

#endif
