#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_wind_noise_detection.data.bss")
#pragma data_seg(".adv_wind_noise_detection.data")
#pragma const_seg(".adv_wind_noise_detection.text.const")
#pragma code_seg(".adv_wind_noise_detection.text")
#endif
#include "adv_wind_noise_detection.h"
#include "app_config.h"
#include "rcsp_adv_bluetooth.h"

#if (RCSP_ADV_ANC_VOICE && RCSP_ADV_WIND_NOISE_DETECTION)


/**
 * @brief 获取风噪检测开关
 *
 * @result bool
 */
bool get_wind_noise_detection_switch()
{
    printf("===%s===", __FUNCTION__);
    return true;
}

/**
 * @brief 设置风噪检测开关
 */
void set_wind_noise_detection_switch(bool detection_switch)
{
    printf("%s, switch:%d\n", __FUNCTION__, detection_switch);
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_WIND_NOISE_DETECTION, NULL, 0);
}

#endif
