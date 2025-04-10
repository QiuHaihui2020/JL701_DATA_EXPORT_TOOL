#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_key_setting.data.bss")
#pragma data_seg(".adv_key_setting.data")
#pragma const_seg(".adv_key_setting.text.const")
#pragma code_seg(".adv_key_setting.text")
#endif
#include "app_config.h"
#include "syscfg_id.h"
#include "key_event_deal.h"
#include "ble_rcsp_server.h"

#include "rcsp_setting_sync.h"
#include "rcsp_setting_opt.h"
#include "adv_anc_voice_key.h"
#include "app_msg.h"
#include "key_driver.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if (RCSP_MODE && RCSP_ADV_EN)
#if RCSP_ADV_KEY_SET_ENABLE

extern int get_bt_tws_connect_status();
static u8 rcsp_adv_key_event_flag = 0x0;

#define ADV_POWER_ON_OFF	0
enum RCSP_KEY_TYPE {
    RCSP_KEY_TYPE_NULL = 0x00,
#if ADV_POWER_ON_OFF
    RCSP_KEY_TYPE_POWER_ON,
    RCSP_KEY_TYPE_POWER_OFF,
#endif
    RCSP_KEY_TYPE_PREV = 0x03,
    RCSP_KEY_TYPE_NEXT,
    RCSP_KEY_TYPE_PP,
    RCSP_KEY_TYPE_ANSWER_CALL,
    RCSP_KEY_TYPE_HANG_UP,
    RCSP_KEY_TYPE_CALL_BACK,
    RCSP_KEY_TYPE_INC_VOICE,
    RCSP_KEY_TYPE_DESC_VOICE,
    RCSP_KEY_TYPE_TAKE_PHOTO,
    RCSP_KEY_TYPE_ANC_VOICE = 0xFF,
};

enum RCSP_EAR_CHANNEL {
    RCSP_EAR_CHANNEL_LEFT = 0x01,
    RCSP_EAR_CHANNEL_RIGHT = 0x02,
};

enum RCSP_KEY_ACTION {
    RCSP_KEY_ACTION_CLICK = 0x01,
    RCSP_KEY_ACTION_DOUBLE_CLICK = 0x02,
};

static u8 g_key_setting[12] = {
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_CLICK,      RCSP_KEY_TYPE_PP, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_CLICK,      RCSP_KEY_TYPE_PP, \
    RCSP_EAR_CHANNEL_LEFT,  RCSP_KEY_ACTION_DOUBLE_CLICK, RCSP_KEY_TYPE_ANSWER_CALL, \
    RCSP_EAR_CHANNEL_RIGHT, RCSP_KEY_ACTION_DOUBLE_CLICK, RCSP_KEY_TYPE_ANSWER_CALL
};
// 按键缓存
// 第一列 | 第二列 | 第三列
// 01 左耳| 01 单击| 按键功能
// 02 右耳| 02 双击| 按键功能

static void enable_adv_key_event(void)
{
    rcsp_adv_key_event_flag = 1;
}

static u8 get_adv_key_event_status(void)
{
    return rcsp_adv_key_event_flag;
}

static void disable_adv_key_event(void)
{
    rcsp_adv_key_event_flag = 0;
}

static void set_key_setting(u8 *key_setting_info)
{
    // 设置左耳短按功能
    g_key_setting[3 * 0 + 2] = key_setting_info[0];
    // 设置左耳长按功能
    g_key_setting[3 * 2 + 2] = key_setting_info[1];
    // 设置右耳短按功能
    g_key_setting[3 * 1 + 2] = key_setting_info[2];
    // 设置右耳长按功能
    g_key_setting[3 * 3 + 2] = key_setting_info[3];
}

static int get_key_setting(u8 *key_setting_info)
{
    // 获取左耳短按功能
    key_setting_info[0] = g_key_setting[3 * 0 + 2];
    // 获取左耳长按功能
    key_setting_info[1] = g_key_setting[3 * 2 + 2];
    // 获取右耳短按功能
    key_setting_info[2] = g_key_setting[3 * 1 + 2];
    // 获取右耳长按功能
    key_setting_info[3] = g_key_setting[3 * 3 + 2];
    return 0;
}

static void update_key_setting_vm_value(u8 *key_setting_info)
{
    syscfg_write(CFG_RCSP_ADV_KEY_SETTING, key_setting_info, 4);
}

static void key_setting_sync(u8 *key_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_rcsp_setting(ATTR_TYPE_KEY_SETTING);
    }
#endif
}

static void deal_key_setting(u8 *key_setting_info, u8 write_vm, u8 tws_sync)
{
    u8 key_setting[4];
    if (!key_setting_info) {
        get_key_setting(key_setting);
    } else {
        memcpy(key_setting, key_setting_info, 4);
    }
    // 1、写入VM
    if (write_vm) {
        update_key_setting_vm_value(key_setting);
    }
    // 2、同步对端
    if (tws_sync) {
        key_setting_sync(key_setting);
    }
    // 3、更新状态
    enable_adv_key_event();
}


static u8 get_adv_key_opt(u8 key_action, u8 channel)
{
    u8 opt;
    for (opt = 0; opt < sizeof(g_key_setting); opt += 3) {
        if (g_key_setting[opt] == channel &&
            g_key_setting[opt + 1] == key_action) {
            break;
        }
    }
    if (sizeof(g_key_setting) == opt) {
        return -1;
    }

    switch (g_key_setting[opt + 2]) {
    case RCSP_KEY_TYPE_NULL:
        opt = APP_MSG_NULL;
        break;
#if ADV_POWER_ON_OFF
    case RCSP_KEY_TYPE_POWER_ON:
        opt = APP_MSG_POWER_ON;
        break;
    case RCSP_KEY_TYPE_POWER_OFF:
        opt = APP_MSG_POWER_OFF;
        break;
#endif
    case RCSP_KEY_TYPE_PREV:
        opt = APP_MSG_MUSIC_PREV;
        break;
    case RCSP_KEY_TYPE_NEXT:
        opt = APP_MSG_MUSIC_NEXT;
        break;
    case RCSP_KEY_TYPE_PP:
        opt = APP_MSG_MUSIC_PP;
        break;
    case RCSP_KEY_TYPE_ANSWER_CALL:
        opt = APP_MSG_CALL_ANSWER;
        break;
    case RCSP_KEY_TYPE_HANG_UP:
        opt = APP_MSG_CALL_HANGUP;
        break;
    case RCSP_KEY_TYPE_CALL_BACK:
        opt = APP_MSG_CALL_LAST_NO;
        break;
    case RCSP_KEY_TYPE_INC_VOICE:
        opt = APP_MSG_VOL_UP;
        break;
    case RCSP_KEY_TYPE_DESC_VOICE:
        opt = APP_MSG_VOL_DOWN;
        break;
    case RCSP_KEY_TYPE_TAKE_PHOTO:
        opt = APP_MSG_HID_CONTROL;
        break;
    case RCSP_KEY_TYPE_ANC_VOICE:
        opt = APP_MSG_NULL;
#if (RCSP_ADV_EN && RCSP_ADV_ANC_VOICE)
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            break;
        }
#endif
        update_anc_voice_key_opt();
#endif
        break;
    }
    return opt;
}

/**
 * rcsp按键配置转换
 *
 * @param value 按键功能
 * @param msg 按键消息
 *
 * @return 是否拦截消息
 */
int rcsp_key_event_remap(int *msg)
{
    if (0 == get_adv_key_event_status()) {
        return -1;
    }
    int key_value = APP_MSG_KEY_VALUE(msg[0]);
    if (key_value != KEY_POWER) {
        return -1;
    }
    int key_action = APP_MSG_KEY_ACTION(msg[0]);

    switch (key_action) {
    case KEY_ACTION_CLICK:
        // 单击
        key_action = RCSP_KEY_ACTION_CLICK;
        break;
    case KEY_ACTION_DOUBLE_CLICK:
        // 双击
        key_action = RCSP_KEY_ACTION_DOUBLE_CLICK;
        break;
    default:
        return -1;
    }

#if (TCFG_USER_TWS_ENABLE)
    u8 channel = tws_api_get_local_channel();
#else
    u8 channel = 'U';
#endif

    switch (channel) {
    case 'U':
    case 'L':
        channel = (msg[1] == APP_KEY_MSG_FROM_TWS) ? RCSP_EAR_CHANNEL_RIGHT : RCSP_EAR_CHANNEL_LEFT;
        break;
    case 'R':
        channel = (msg[1] == APP_KEY_MSG_FROM_TWS) ? RCSP_EAR_CHANNEL_LEFT : RCSP_EAR_CHANNEL_RIGHT;
        break;
    default:
        return -1;
    }

    return get_adv_key_opt(key_action, channel);
}

static int key_set_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    u8 dlen = *((u8 *)setting_data_len);
    u8 *key_setting_data = (u8 *)setting_data;
    while (dlen >= 3) {
        if (key_setting_data[0] == RCSP_EAR_CHANNEL_LEFT) {
            if (key_setting_data[1] == RCSP_KEY_ACTION_CLICK) {
                g_key_setting[2] = key_setting_data[2];
            } else if (key_setting_data[1] == RCSP_KEY_ACTION_DOUBLE_CLICK) {
                g_key_setting[8] = key_setting_data[2];
            }
        } else if (key_setting_data[0] == RCSP_EAR_CHANNEL_RIGHT) {
            if (key_setting_data[1] == RCSP_KEY_ACTION_CLICK) {
                g_key_setting[5] = key_setting_data[2];
            } else if (key_setting_data[1] == RCSP_KEY_ACTION_DOUBLE_CLICK) {
                g_key_setting[11] = key_setting_data[2];
            }
        }
        dlen -= 3;
        key_setting_data += 3;
    }
    return 0;
}

static int key_get_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = (int *)g_key_setting;
    return sizeof(g_key_setting);
}

static RCSP_SETTING_OPT adv_key_opt = {
    .data_len = 4,
    .setting_type = ATTR_TYPE_KEY_SETTING,
    .syscfg_id = CFG_RCSP_ADV_KEY_SETTING,
    .deal_opt_setting = deal_key_setting,
    .set_setting = set_key_setting,
    .get_setting = get_key_setting,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = key_set_setting_extra_handle,
    .get_setting_extra_handle = key_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(adv_key_opt);

#endif
#endif
