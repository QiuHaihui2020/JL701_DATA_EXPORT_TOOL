#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_mic_setting.data.bss")
#pragma data_seg(".adv_mic_setting.data")
#pragma const_seg(".adv_mic_setting.text.const")
#pragma code_seg(".adv_mic_setting.text")
#endif
#include "app_config.h"
#include "syscfg_id.h"
#include "ble_rcsp_server.h"

#include "adv_mic_setting.h"
#include "rcsp_setting_sync.h"
#include "rcsp_setting_opt.h"
#include "classic/tws_api.h"
#include "bt_tws.h"

#if (RCSP_MODE && RCSP_ADV_EN)
#if RCSP_ADV_MIC_SET_ENABLE

enum RCSP_MIC_MODE {
    RCSP_MIC_MODE_AUTO = 1,		// 自动切换
    RCSP_MIC_MODE_LEFT = 2,		// 始终左耳
    RCSP_MIC_MODE_RIGHT = 3,	// 始终右耳
};

static u8 g_mic_mode = RCSP_MIC_MODE_LEFT;

static void mic_setting_info_deal(u8 *mic_info_data)
{
#if TCFG_USER_TWS_ENABLE
    u8 channel = tws_api_get_local_channel();
    printf("%s, channel:%c\n", __FUNCTION__, channel);

    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        printf("-----master-----\n");
    } else {
        printf("-----role-----\n");
    }
    switch (*mic_info_data) {
    case RCSP_MIC_MODE_AUTO:
        if (get_bt_tws_connect_status()) {
            printf("-------auto--------\n");
            tws_api_auto_role_switch_enable();
        }
        break;
    case RCSP_MIC_MODE_LEFT:
        if (get_bt_tws_connect_status()) {
            if (channel != 'L') {
                printf("-------!L--------\n");
                tws_api_auto_role_switch_disable();
                tws_api_role_switch();
            }
        }
        break;
    case RCSP_MIC_MODE_RIGHT:
        if (get_bt_tws_connect_status()) {
            if (channel != 'R') {
                printf("-------!R--------\n");
                tws_api_auto_role_switch_disable();
                tws_api_role_switch();
            }
        }
        break;
    default:
        break;
    }
#endif
}

static void set_mic_setting(u8 *mic_setting_info)
{
    g_mic_mode = *mic_setting_info;
}

static int get_mic_setting(u8 *mic_setting_info)
{
    *mic_setting_info = g_mic_mode;
    return 0;
}

static void update_mic_setting_vm_value(u8 *mic_setting_info)
{
    syscfg_write(CFG_RCSP_ADV_MIC_SETTING, mic_setting_info, 1);
}

static void adv_mic_setting_sync(u8 *mic_setting_info)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_rcsp_setting(ATTR_TYPE_MIC_SETTING);
    }
#endif
}

static void deal_mic_setting(u8 *mic_setting_info, u8 write_vm, u8 tws_sync)
{
    if (!mic_setting_info) {
    } else {
        set_mic_setting(mic_setting_info);
    }
    if (write_vm) {
        update_mic_setting_vm_value(&g_mic_mode);
    }
    if (tws_sync) {
        adv_mic_setting_sync(&g_mic_mode);
    }
    mic_setting_info_deal(&g_mic_mode);
}


#if TCFG_USER_TWS_ENABLE
//通话时,根据esco state,固定mic的位置
void rcsp_user_mic_fixed_deal()
{
    u8 channel = tws_api_get_local_channel();

    if (!get_bt_tws_connect_status()) {
        return;
    }

    u8 adv_mic_setting = 0;
    get_mic_setting(&adv_mic_setting);
    if ((tws_api_get_role() == TWS_ROLE_MASTER)
        && (tws_api_get_tws_state() | TWS_STA_ESCO_OPEN)) {

        switch (adv_mic_setting) {
        case RCSP_MIC_MODE_LEFT:
            if (get_bt_tws_connect_status()) {
                if (channel != 'L') {
                    printf("mic_sw_l\n");
                    tws_api_role_switch();
                }
            }
            break;

        case RCSP_MIC_MODE_RIGHT:
            if (get_bt_tws_connect_status()) {
                if (channel != 'R') {
                    printf("mic_sw_r\n");
                    tws_api_role_switch();
                }
            }

            break;

        default:
            printf("mic_sw_auto\n");
            break;
        }
    }
}
#endif

static int mic_set_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    u8 dlen = *((u8 *)setting_data_len);
    u8 *mic_setting_data = (u8 *)setting_data;
    memcpy(&g_mic_mode, mic_setting_data, dlen);
    printf("mic_set_setting_extra_handle mode:%d", g_mic_mode);
    return 0;
}

static int mic_get_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    int **setting_data_ptr = (int **)setting_data;
    *setting_data_ptr = (int *)&g_mic_mode;
    return sizeof(g_mic_mode);
}

static RCSP_SETTING_OPT adv_mic_opt = {
    .data_len = 1,
    .setting_type =	ATTR_TYPE_MIC_SETTING,
    .syscfg_id = CFG_RCSP_ADV_MIC_SETTING,
    .deal_opt_setting = deal_mic_setting,
    .set_setting = set_mic_setting,
    .get_setting = get_mic_setting,
    .custom_setting_init = NULL,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = mic_set_setting_extra_handle,
    .get_setting_extra_handle = mic_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(adv_mic_opt);

#endif

#endif
