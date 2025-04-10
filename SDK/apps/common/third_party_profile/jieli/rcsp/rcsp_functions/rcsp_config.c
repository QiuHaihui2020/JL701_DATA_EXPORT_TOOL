#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_config.data.bss")
#pragma data_seg(".rcsp_config.data")
#pragma const_seg(".rcsp_config.text.const")
#pragma code_seg(".rcsp_config.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "le_common.h"
#include "rcsp_event.h"
#include "ble_rcsp_adv.h"
#include "ble_rcsp_server.h"
#include "custom_cfg.h"
#include "rcsp_adv_bluetooth.h"

#if (RCSP_MODE)

// rcsp认证lib开关
#if (!BT_CONNECTION_VERIFY)
u8 rcsp_auth_support = 1;
#else
u8 rcsp_auth_support = 0;
#endif

// rcsp一拖二lib开关
#if (TCFG_RCSP_DUAL_CONN_ENABLE)
u8 rcsp_dual_support = 1;
#else
u8 rcsp_dual_support = 0;
#endif

/* #define RCSP_DEBUG_EN */
#ifdef RCSP_DEBUG_EN
#define rcsp_putchar(x)                	putchar(x)
#define rcsp_printf                    	printf
#define rcsp_put_buf(x,len)				put_buf(x,len)
#else
#define rcsp_putchar(...)
#define rcsp_printf(...)
#define rcsp_put_buf(...)
#endif

void rcsp_set_auth_support_en(u8 en)
{
    if (en) {
        rcsp_auth_support = 1;
    } else {
        rcsp_auth_support = 0;
    }
}

u8 rcsp_get_auth_support()
{
    return rcsp_auth_support;
}

u8 get_defalut_bt_channel_sel(void)
{
#if (RCSP_CHANNEL_SEL)
    return RCSP_CHANNEL_SEL;
#else
    return 0;
#endif
}

u8 get_curr_platform(void)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel) {
        return rcspModel->A_platform;
    }
    return 0;
}

void rcsp_config(struct RcspModel *rcspModel)
{
    if (rcspModel == NULL) {
        return ;
    }

    ///配置app支持功能展示, 跟进具体方案进行配置
#if !TCFG_RCSP_DUAL_CONN_ENABLE // 一拖二

#if (TCFG_APP_BT_EN && RCSP_ADV_MUSIC_INFO_ENABLE)
    rcspModel->function_mask |= BIT(BT_FUNCTION_MASK);
#endif
#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    rcspModel->function_mask |= BIT(MUSIC_FUNCTION_MASK);
#endif
#if (TCFG_APP_RTC_EN && RCSP_APP_RTC_EN)
    rcspModel->function_mask |= BIT(RTC_FUNCTION_MASK);
#endif
#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
    rcspModel->function_mask |= BIT(LINEIN_FUNCTION_MASK);
#endif
#if TCFG_APP_FM_EN
    rcspModel->function_mask |= BIT(FM_FUNCTION_MASK);
#endif
#if TCFG_COLORLED_ENABLE
    rcspModel->function_mask |= BIT(COLOR_LED_MASK);
#endif

#if TCFG_APP_SPDIF_EN
    rcspModel->function_mask |= BIT(SPDIF_FUNCTION_MASK);
#endif

#if 0//(TCFG_APP_PC_EN && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE)
    rcspModel->function_mask |= BIT(PC_FUNCTION_MASK);
#endif

#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    rcspModel->music_icon_mask = DEV_ICON_ALL_DISPLAY
                                 | BIT(USB_ICON_DISPLAY)
                                 | BIT(SD0_ICON_DISPLAY)
                                 | BIT(SD1_ICON_DISPLAY)
                                 ;
#endif

#endif

#if (RCSP_SDK_TYPE)
    ///sdk类型
    rcspModel->sdk_type = RCSP_SDK_TYPE;
#endif

    ///OTA升级类型
    extern const int support_dual_bank_update_en;
    rcspModel->ota_type = support_dual_bank_update_en;

#if (BT_CONNECTION_VERIFY)
    ///是否需要握手配置	0-校验，1-不校验
    rcspModel->auth_check = BT_CONNECTION_VERIFY;
    JL_rcsp_set_auth_flag(rcspModel->auth_check);
#endif

    ///是否使能发射器功能配置
    rcspModel->emitter_en = 0;

#if (RCSP_ADV_FIND_DEVICE_ENABLE)
    ///是否支持查找设备
    rcspModel->find_dev_en = RCSP_ADV_FIND_DEVICE_ENABLE;
#endif
    ///是否支持游戏模式
    rcspModel->game_mode_en = 0;
    ///是否支持md5升级
    rcspModel->md5_support = UPDATE_MD5_ENABLE;
    ///是否支持卡拉ok功能
    rcspModel->karaoke_en = RCSP_ADV_KARAOKE_SET_ENABLE;

#ifdef RCSP_SOUND_EFFECT_FUNC_DISABLE
    ///是否屏蔽声效
    rcspModel->sound_effects_disable = RCSP_SOUND_EFFECT_FUNC_DISABLE;
#endif

#if (JL_RCSP_EXTRA_FLASH_OPT)
    rcspModel->extra_flash_en = JL_RCSP_EXTRA_FLASH_OPT;
#endif

#if (WATCH_FILE_TO_FLASH && TCFG_DEV_MANAGER_ENABLE)
    rcspModel->file_trans_back_mode = 1;
    rcspModel->file_transfer_mode = 1;
#if (TCFG_NOR_FAT)
    rcspModel->file_trans_nor_fat = 1;
#endif
#endif

#if JL_RCSP_SIMPLE_TRANSFER
    rcspModel->file_simple_trans_mode = 1;
#endif

}


#endif//RCSP_MODE

