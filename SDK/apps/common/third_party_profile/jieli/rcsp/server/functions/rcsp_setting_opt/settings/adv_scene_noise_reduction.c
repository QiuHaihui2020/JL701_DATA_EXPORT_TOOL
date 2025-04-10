#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_scene_noise_reduction.data.bss")
#pragma data_seg(".adv_scene_noise_reduction.data")
#pragma const_seg(".adv_scene_noise_reduction.text.const")
#pragma code_seg(".adv_scene_noise_reduction.text")
#endif
#include "adv_scene_noise_reduction.h"
#include "app_config.h"
#include "rcsp_adv_bluetooth.h"

#if (RCSP_ADV_ANC_VOICE && RCSP_ADV_SCENE_NOISE_REDUCTION)

/**
 * @brief 获取场景降噪类型
 *
 * @result RCSP_SCENE_NOISE_REDUCTION_TYPE
 */
RCSP_SCENE_NOISE_REDUCTION_TYPE get_scene_noise_reduction_type()
{
    printf("===%s===", __FUNCTION__);
    return RCSP_SCENE_NOISE_REDUCTION_TYPE_INTELLIGENT;
}

/**
 * @brief 设置场景降噪类型
 */
void set_scene_noise_reduction_type(RCSP_SCENE_NOISE_REDUCTION_TYPE type)
{
    switch (type) {
    case RCSP_SCENE_NOISE_REDUCTION_TYPE_INTELLIGENT:
    case RCSP_SCENE_NOISE_REDUCTION_TYPE_MILD:
    case RCSP_SCENE_NOISE_REDUCTION_TYPE_BALANCE:
    case RCSP_SCENE_NOISE_REDUCTION_TYPE_DEEPNESS:
    default:
        printf("%s, type:%d", __FUNCTION__, type);
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_SCENE_NOISE_REDUCTION, NULL, 0);
        break;
    }

}

#endif
