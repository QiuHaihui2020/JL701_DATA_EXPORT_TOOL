#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_ai_no_pick.data.bss")
#pragma data_seg(".adv_ai_no_pick.data")
#pragma const_seg(".adv_ai_no_pick.text.const")
#pragma code_seg(".adv_ai_no_pick.text")
#endif
#include "adv_ai_no_pick.h"
#include "app_config.h"
#include "rcsp_adv_bluetooth.h"

#if RCSP_ADV_AI_NO_PICK

/**
 * @brief 获取智能免摘开关
 *
 * @result bool
 */
bool get_ai_no_pick_switch()
{
    printf("===%s===", __FUNCTION__);
    return true;
}

/**
 * @brief 开启智能免摘
 */
void set_ai_no_pick_switch(bool p_switch)
{
    printf("%s, switch:%d", __FUNCTION__, p_switch);
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_AI_NO_PICK, NULL, 0);
}

/**
 * @brief 获取智能免摘敏感度
 *
 * @result RCSP_AI_NO_PICK_SENSITIVITY
 */
RCSP_AI_NO_PICK_SENSITIVITY get_ai_no_pick_sensitivity()
{
    printf("===%s===", __FUNCTION__);
    return RCSP_AI_NO_PICK_SENSITIVITY_HIGH;
}

/**
 * @brief 设置智能免摘敏感度
 */
void set_ai_no_pick_sensitivity(RCSP_AI_NO_PICK_SENSITIVITY sensitivity)
{
    switch (sensitivity) {
    case RCSP_AI_NO_PICK_SENSITIVITY_HIGH:
    case RCSP_AI_NO_PICK_SENSITIVITY_LOW:
    default:
        printf("%s, sensitivity:%d", __FUNCTION__, sensitivity);
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_AI_NO_PICK, NULL, 0);
    }
}

/**
 * @brief 获取智能免摘自动关闭时间
 *
 * @result RCSP_AI_NO_PICK_AUTO_CLOSE_TIME
 */
RCSP_AI_NO_PICK_AUTO_CLOSE_TIME get_ai_no_pick_auto_close_time()
{
    printf("===%s===", __FUNCTION__);
    return RCSP_AI_NO_PICK_AUTO_CLOSE_TIME_NONE;
}

/**
 * @brief 设置智能免摘自动关闭时间
 */
void set_ai_no_pick_auto_close_time(RCSP_AI_NO_PICK_AUTO_CLOSE_TIME time_type)
{
    switch (time_type) {
    case RCSP_AI_NO_PICK_AUTO_CLOSE_TIME_NONE:
    case RCSP_AI_NO_PICK_AUTO_CLOSE_TIME_5s:
    case RCSP_AI_NO_PICK_AUTO_CLOSE_TIME_15s:
    case RCSP_AI_NO_PICK_AUTO_CLOSE_TIME_30s:
    default:
        printf("%s, type:%d", __FUNCTION__, time_type);
        JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_AI_NO_PICK, NULL, 0);
    }
}

#endif
