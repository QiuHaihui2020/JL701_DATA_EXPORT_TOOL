#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_device_status.data.bss")
#pragma data_seg(".rcsp_device_status.data")
#pragma const_seg(".rcsp_device_status.text.const")
#pragma code_seg(".rcsp_device_status.text")
#endif
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp.h"
#include "rcsp_event.h"
#include "rcsp_device_info_func_common.h"
#include "ble_rcsp_server.h"
#include "rcsp_setting_opt.h"
#include "dev_manager.h"
#include "btstack/avctp_user.h"
#include "app_task.h"
#include "rcsp_misc_setting.h"
#include "rcsp_extra_flash_opt.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_attr.h"
#include "rcsp_fm_func.h"
#include "rcsp_spdif_func.h"
#include "rcsp_pc_func.h"

#if RCSP_ADV_ANC_VOICE
#include "adv_anc_voice.h"
#endif

#include "adv_hearing_aid_setting.h"
#include "adv_ai_no_pick.h"
#include "adv_scene_noise_reduction.h"
#include "adv_wind_noise_detection.h"
#include "adv_voice_enhancement_mode.h"
#include "adv_1t2_setting.h"

#if RCSP_MODE && RCSP_DEVICE_STATUS_ENABLE

#if (RCSP_MODE == RCSP_MODE_EARPHONE)
#include "event.h"
#endif

#if TCFG_APP_LINEIN_EN
#include "linein_dev.h"
#endif

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif

#define ASSERT_SET_LEN(len, limit) 	\
									do{	\
										if(len >= limit){	\
										}else{				\
											return 0;   \
										}\
									}while(0);


#define ASSERT_SET_LEN_RETURN_NULL(len, limit) 	\
									do{	\
										if(len >= limit){	\
										}else{				\
											return;   \
										}\
									}while(0);

#define FUNCTION_UPDATE_MAX_LEN			(256)

#pragma pack(1)
struct _DEV_info {
    u8 status;
    u32 usb_handle;
    u32 sd0_handle;
    u32 sd1_handle;
    u32 flash_handle;
    u32 flash_handle_2;
};
#pragma pack()

typedef bool (*func_set)(void *priv, u8 *data, u16 len);
typedef u32(*func_get)(void *priv, u8 *buf, u16 buf_size, u32 mask);
static const func_set set_tab[FUNCTION_MASK_MAX] = {
    [BT_FUNCTION_MASK] = rcsp_bt_func_set,
#if (TCFG_APP_MUSIC_EN)
    [MUSIC_FUNCTION_MASK] = rcsp_music_func_set,
#endif
#if (TCFG_APP_RTC_EN && RCSP_APP_RTC_EN)
    [RTC_FUNCTION_MASK] = rcsp_rtc_func_set,
#endif
#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
    [LINEIN_FUNCTION_MASK] = rcsp_linein_func_set,
#endif
#if (TCFG_APP_FM_EN)
    [FM_FUNCTION_MASK] = rcsp_fm_func_set,
    /* [FMTX_FUNCTION_MASK] = NULL, */
#endif
#if (TCFG_APP_SPDIF_EN)
    [SPDIF_FUNCTION_MASK] = rcsp_spdif_func_set,
#endif
#if (TCFG_APP_PC_EN && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE)
    [PC_FUNCTION_MASK] = rcsp_pc_func_set,
#endif
};

static const func_get get_tab[FUNCTION_MASK_MAX] = {
    [BT_FUNCTION_MASK] = rcsp_bt_func_get,
#if (TCFG_APP_MUSIC_EN)
    [MUSIC_FUNCTION_MASK] = rcsp_music_func_get,
#endif
#if (TCFG_APP_RTC_EN && RCSP_APP_RTC_EN)
    [RTC_FUNCTION_MASK] = rcsp_rtc_func_get,
#endif
#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
    [LINEIN_FUNCTION_MASK] = rcsp_linein_func_get,
#endif
#if (TCFG_APP_FM_EN)
    [FM_FUNCTION_MASK] = rcsp_fm_func_get,
    /* [FMTX_FUNCTION_MASK] = NULL, */
#endif
#if (TCFG_APP_SPDIF_EN)
    [SPDIF_FUNCTION_MASK] = rcsp_spdif_func_get,
#endif
#if (TCFG_APP_PC_EN && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE)
    [PC_FUNCTION_MASK] = rcsp_pc_func_get,
#endif
};

static u8 _OpCode = 0;
static u8 _OpCode_SN = 0;

static void common_function_attr_vol_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        rcspModel->err_code = -1;
        return;
    }
    ASSERT_SET_LEN_RETURN_NULL(len, 1);
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_VOL_SETTING);
    if (setting_opt_hdl) {
        set_rcsp_opt_setting(setting_opt_hdl, data);
        u32 mask = BIT(attr);
        rcsp_msg_post(USER_MSG_RCSP_SET_VOL, 2, (int)priv, mask);
    }
}
static void common_function_attr_eq_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        set_rcsp_opt_setting(setting_opt_hdl, data);
        u32 mask = BIT(attr);
        rcsp_msg_post(USER_MSG_RCSP_SET_EQ_PARAM, 2, (int)priv, mask);
    }
}
static void common_function_attr_fmtx_freq_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
    ASSERT_SET_LEN_RETURN_NULL(len, 2);
    u16 freq = READ_BIG_U16(data);
    rcsp_msg_post(USER_MSG_RCSP_SET_FMTX_FREQ, 2, (int)priv, (int)freq);
#endif
}
static void common_function_attr_bt_emitter_sw_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    ASSERT_SET_LEN_RETURN_NULL(len, 1);
    rcspModel->emitter_sw = data[0];
    rcsp_msg_post(USER_MSG_RCSP_SET_BT_EMITTER_SW, 1, (int)priv);
}

extern void emitter_search_stop(u8 result);
static void common_function_attr_bt_emitter_connect_state_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    ASSERT_SET_LEN_RETURN_NULL(len, 7);
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    memcpy(rcspModel->emitter_con_addr, data + 1, 6);
    /* rcsp_msg_post(USER_MSG_RCSP_SET_BT_EMITTER_CONNECT_STATES, 2, (int)priv, (int)data[0]); */
    printf("USER_MSG_RCSP_SET_BT_EMITTER_CONNECT_STATES, state = %d\n", data[0]);
    if (data[0]) {
#if TCFG_USER_EMITTER_ENABLE
        emitter_search_stop(0);
#endif
        put_buf(rcspModel->emitter_con_addr, 6);
        bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, rcspModel->emitter_con_addr);
    } else {
        if (bt_get_curr_channel_state() != 0) {
            bt_cmd_prepare(USER_CTRL_A2DP_CMD_CLOSE, 0, NULL);
            bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 6, spp_remote_addr);
        }
    }
}

static void common_function_attr_high_low_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        rcspModel->err_code = -1;
        return;
    }

    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_HIGH_LOW_VOL);
    if (!set_setting_extra_handle(setting_opt_hdl, data, NULL)) {
        set_rcsp_opt_setting(setting_opt_hdl, NULL);
    }
    u32 mask = BIT(attr);
    rcsp_msg_post(USER_MSG_RCSP_SET_HIGH_LOW_VOL_PARAM, 2, (int)priv, mask);
}

static void common_function_attr_anc_voice_info_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_ANC_VOICE
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    anc_voice_info_set(data, len);
#endif // RCSP_ADV_ANC_VOICE
}

static void common_function_attr_hearing_aid_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_ASSISTED_HEARING
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    deal_hearing_aid_setting(_OpCode, _OpCode_SN, data, 0, 1);
#endif // RCSP_ADV_ASSISTED_HEARING
}

static void common_function_attr_adaptive_noise_reduction_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION\n");
    u8 *switch_adaptive_noise_reduction = data;
    if (*switch_adaptive_noise_reduction) {
        u8 *is_adaptive_noise_reduction_check = data + 1;
        if (*is_adaptive_noise_reduction_check) {
            // 重新检测
            set_adaptive_noise_reduction_reset();
        } else {
            // 开启自适应
            set_adaptive_noise_reduction_on();
        }
    } else {
        // 关闭自适应
        set_adaptive_noise_reduction_off();
    }
#endif
}

static void common_function_attr_ai_no_pick_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_AI_NO_PICK
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_AI_NO_PICK\n");
    u8 *ai_no_pick_func = data + 1;
    u8 *ai_no_pick_val = data + 2;
    if ((*ai_no_pick_func) == 0x00) {				// 开关智能免摘
        set_ai_no_pick_switch((*ai_no_pick_val) ? true : false);
    } else if ((*ai_no_pick_func) == 0x01) {		// 设置智能免摘敏感度
        set_ai_no_pick_sensitivity((*ai_no_pick_val));
    } else if ((*ai_no_pick_func) == 0x02) {		// 设置智能免摘启动后自动关闭时间
        set_ai_no_pick_auto_close_time((*ai_no_pick_val));
    }

#endif
}

static void common_function_attr_scene_noise_reduction_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_SCENE_NOISE_REDUCTION
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_SCENE_NOISE_REDUCTION\n");
    u8 *scene_noise_reduction_type = data + 1;
    set_scene_noise_reduction_type((RCSP_SCENE_NOISE_REDUCTION_TYPE)(*scene_noise_reduction_type));
#endif
}

static void common_function_attr_wind_noise_detection_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_WIND_NOISE_DETECTION
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_WIND_NOISE_DETECTION\n");
    u8 *wind_noise_detection_switch = data + 1;
    set_wind_noise_detection_switch((*wind_noise_detection_switch) ? true : false);
#endif
}

static void common_function_attr_voice_enhancement_mode_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if RCSP_ADV_VOICE_ENHANCEMENT_MODE
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_VOICE_ENHANCEMENT_MODE\n");
    u8 *voice_enhancement_mode_switch = data + 1;
    set_voice_enhancement_mode_switch((*voice_enhancement_mode_switch) ? true : false);
#endif
}

static void common_function_attr_1t2_info_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
#if TCFG_RCSP_DUAL_CONN_ENABLE
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
#if TCFG_USER_TWS_ENABLE
    if (!get_bt_tws_connect_status()) {
        // 如果tws没有连接，则不允许设置
        rcspModel->err_code = 1;
        return;
    }
#endif
    printf("SET RCSP_DEVICE_STATUS_ATTR_TYPE_1T2\n");
    put_buf(data, len);
    u8 *switch_1t2 = data + 1;
    u8 mac_addr[6] = {0};
    if (len >= 8) {
        memcpy(mac_addr, data + 2, 6);
    }
    if (ble_con_handle) {
        rcsp_set_1t2_switch((*switch_1t2) ? true : false, ble_con_handle, mac_addr);
    } else {
        rcsp_set_1t2_switch((*switch_1t2) ? true : false, ble_con_handle, spp_remote_addr);
    }
#endif
}

static void common_function_attr_misc_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        rcspModel->err_code = -1;
        return;
    }
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_MISC_SETTING);
    if (setting_opt_hdl) {
        if (!set_setting_extra_handle(setting_opt_hdl, data, NULL)) {
            set_rcsp_opt_setting(setting_opt_hdl, data);
        } else {
            rcspModel->err_code = -1;
        }
    }
}

static void common_function_attr_color_led_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_COLOR_LED_SETTING);
    if (setting_opt_hdl) {
        set_rcsp_opt_setting(setting_opt_hdl, data);
    }
}

static void common_function_attr_karaoke_eq_setting_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return;
    }
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        set_rcsp_opt_setting(setting_opt_hdl, data);
        u32 mask = BIT(attr);
        rcsp_msg_post(USER_MSG_RCSP_SET_EQ_PARAM, 2, (int)priv, mask);
    }
}

static const attr_set_func common_function_set_tab[RCSP_DEVICE_STATUS_ATTR_TYPE_MAX] = {
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BATTERY 				          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_VOL 					          ] = common_function_attr_vol_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO 			          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ERROR_STATS			          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_EQ_INFO				          ] = common_function_attr_eq_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BS_FILE_TYPE			          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FUNCTION_MODE		          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_COLOR_LED_SETTING_INFO 		  ] = common_function_attr_color_led_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FMTX_FREQ			          ] = common_function_attr_fmtx_freq_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_SW		          ] = common_function_attr_bt_emitter_sw_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_CONNECT_STATES      ] = common_function_attr_bt_emitter_connect_state_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_HIGH_LOW_SET			          ] = common_function_attr_high_low_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO 	      ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ANC_VOICE	                  ] = common_function_attr_anc_voice_info_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FETCH_ALL_ANC_VOICE            ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PHONE_SCO_STATE_INFO 	      ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_MISC_SETTING_INFO 	      	  ] = common_function_attr_misc_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO	  ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO 		  ] = common_function_attr_karaoke_eq_setting_set,
    // 缺一个声卡功能19
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ASSISTED_HEARING               ] = common_function_attr_hearing_aid_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION       ] = common_function_attr_adaptive_noise_reduction_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_AI_NO_PICK                     ] = common_function_attr_ai_no_pick_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_SCENE_NOISE_REDUCTION          ] = common_function_attr_scene_noise_reduction_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_WIND_NOISE_DETECTION           ] = common_function_attr_wind_noise_detection_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_VOICE_ENHANCEMENT_MODE         ] = common_function_attr_voice_enhancement_mode_setting_set,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_1T2         					 ] = common_function_attr_1t2_info_set,
};


static u32 common_function_attr_battery_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    extern u8 get_vbat_percent(void);
    u8 vbat = get_vbat_percent();
    rlen = add_one_attr(buf, buf_size, offset, attr, &vbat, sizeof(vbat));
    return rlen;
}
static u32 common_function_attr_vol_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        return 0;
    }
    u8 cur_vol;
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_VOL_SETTING);
    if (setting_opt_hdl) {
        get_rcsp_opt_setting(setting_opt_hdl, (u8 *)&cur_vol);
        rlen = add_one_attr(buf, buf_size, offset, attr, &cur_vol, sizeof(cur_vol));
    }
    return rlen;
}
static u32 common_function_attr_dev_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct RcspModel *rcspModel = (struct RcspModel *) priv;
    struct _DEV_info dev_info = {0};

#if RCSP_MODE != RCSP_MODE_EARPHONE

    if (dev_manager_get_root_path_by_logo("udisk0")) {
        printf("dev [udisk0] online\n");
        dev_info.status |= BIT(RCSPDevMapUDISK);
        dev_info.usb_handle |= app_htonl((u32)RCSPDevMapUDISK);
    }

    if (dev_manager_get_root_path_by_logo("sd0")) {
        printf("dev [sd0] online\n");
        dev_info.status |= BIT(RCSPDevMapSD0);
        dev_info.sd0_handle |= app_htonl((u32)RCSPDevMapSD0);
    }

    if (dev_manager_get_root_path_by_logo("sd1")) {
        printf("dev [sd1] online\n");
        dev_info.status |= BIT(RCSPDevMapSD1);
        dev_info.sd1_handle = app_htonl((u32)RCSPDevMapSD1);
    }

    if (rcspModel->file_transfer_mode && dev_manager_get_root_path_by_logo("virfat_flash")) {
        printf("dev [virfat_flash] online\n");
        dev_info.status |= BIT(RCSPDevMapFLASH);
        dev_info.flash_handle = app_htonl((u32)RCSPDevMapFLASH);
    }

    if (rcspModel->file_trans_nor_fat && dev_manager_get_root_path_by_logo("fat_nor")) {
        printf("dev [fat_nor] online\n");
        dev_info.status |= BIT(RCSPDevMapFLASH_2);
        dev_info.flash_handle_2 = app_htonl((u32)RCSPDevMapFLASH_2);
    }

#if TCFG_APP_LINEIN_EN
    if (linein_is_online()) {
        dev_info.status |= BIT(RCSPDevMapAUX);
    }
#endif

#endif

    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&dev_info, sizeof(dev_info));
    return rlen;
}
static u32 common_function_attr_error_states_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    return rlen;
}

static u32 common_function_attr_pre_fetch_all_eq_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 eq_pre_fetch_info[1  +  20  + (1 + 10) * 10] = {0}; // num + freq + all_gain_of_eq [max]
    u16 eq_per_fetch_size = sizeof(eq_pre_fetch_info);
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        eq_per_fetch_size = get_setting_extra_handle(setting_opt_hdl, eq_pre_fetch_info, &eq_per_fetch_size);
        if (eq_per_fetch_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, eq_pre_fetch_info, eq_per_fetch_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_anc_voice_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_ANC_VOICE
    u8 anc_info[9] = {0};
    anc_voice_info_get(anc_info, sizeof(anc_info));
    rlen = add_one_attr(buf, buf_size, offset, attr, anc_info, sizeof(anc_info));
#endif // RCSP_ADV_ANC_VOICE
    return rlen;
}

static u32 common_function_attr_anc_voice_info_fetch_all_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_ANC_VOICE
    u8 anc_fetch_all_info[28] = {0};
    anc_voice_info_fetch_all_get(anc_fetch_all_info, sizeof(anc_fetch_all_info));
    rlen = add_one_attr(buf, buf_size, offset, attr, anc_fetch_all_info,  sizeof(anc_fetch_all_info));
#endif // RCSP_ADV_ANC_VOICE
    return rlen;
}

static u32 common_function_attr_phone_sco_state_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 phone_state = 0;
    if (BT_CALL_HANGUP != bt_get_call_status()) {
        phone_state = 1;
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, &phone_state, sizeof(phone_state));
    return rlen;
}

static u32 common_function_attr_eq_param_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 eq_info[11] = {0};
    u16 eq_info_size = sizeof(eq_info);
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_EQ_SETTING);
    if (setting_opt_hdl) {
        eq_info_size = get_setting_extra_handle(setting_opt_hdl, eq_info, &eq_info_size);
        if (eq_info_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, eq_info, eq_info_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_bs_file_type(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_FILE_OPT
    /* printf("%s\n", rcsp_browser_file_ext()); */
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)rcsp_browser_file_ext(), rcsp_browser_file_ext_size());
#endif
    return rlen;
}
static u32 common_function_attr_function_mode_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct RcspModel *rcspModel = (struct RcspModel *) priv;
    rlen = add_one_attr(buf, buf_size, offset, attr, &rcspModel->cur_app_mode, sizeof(rcspModel->cur_app_mode));
    return rlen;
}
static u32 common_function_attr_fmtx_freq_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if (TCFG_APP_FM_EMITTER_EN && TCFG_FM_EMITTER_INSIDE_ENABLE)
    u16 freq = fm_emitter_manage_get_fre();
    printf("freq %d\n", freq);
    freq = app_htons(freq);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&freq, sizeof(freq));
#endif
    return rlen;
}
static u32 common_function_attr_bt_emitter_sw_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 sw = rcspModel->emitter_sw;
    rlen = add_one_attr(buf, buf_size, offset,  attr, &sw, 1);
    return rlen;
}
static u32 common_function_attr_bt_emitter_connect_state_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 send_buf[7] = {0};
    /* if(rcspModel->emitter_sw) */
    {
        send_buf[0] = rcspModel->emitter_bt_state;
        memcpy(send_buf + 1, rcspModel->emitter_con_addr, 6);
    }
    printf("stata = %d\n", send_buf[0]);
    put_buf(send_buf + 1, 6);
    rlen = add_one_attr(buf, buf_size, offset,  attr, send_buf, sizeof(send_buf));
    return rlen;
}
static u32 common_function_attr_high_low_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 data[8] = {0};
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_HIGH_LOW_VOL);
    if (setting_opt_hdl) {
        u8 size = get_setting_extra_handle(setting_opt_hdl, data, NULL);
        if (size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, data, size);
        }
    }

    return rlen;
}

static u32 common_function_attr_misc_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u32 data_len = rcsp_get_misc_setting_data_len();
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_MISC_SETTING);
    if (data_len && setting_opt_hdl) {
        u8 *misc_data = zalloc(data_len);
        data_len = get_rcsp_opt_setting(setting_opt_hdl, misc_data);
        rlen = add_one_attr(buf, buf_size, offset, attr, misc_data, data_len);
        if (misc_data) {
            free(misc_data);
        }
    }
    return rlen;
}

static u32 common_function_attr_color_led_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if (RCSP_ADV_COLOR_LED_SET_ENABLE)
    u32 data_len = rcsp_get_color_led_setting_data_len();
    u8 color_led_data[data_len];

    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_COLOR_LED_SETTING);
    if (setting_opt_hdl) {
        get_rcsp_opt_setting(setting_opt_hdl, color_led_data);
        rlen = add_one_attr(buf, buf_size, offset, attr, color_led_data, data_len);
    }
#endif
    return rlen;
}

static u32 common_function_attr_karaoke_eq_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 karaoke_eq_info[9] = {0};
    u16 karaoke_eq_size = sizeof(karaoke_eq_info);
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        karaoke_eq_size = get_setting_extra_handle(setting_opt_hdl, karaoke_eq_info, &karaoke_eq_size);
        if (karaoke_eq_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, karaoke_eq_info, karaoke_eq_size);
        }
    }
    return rlen;
}

static u32 common_function_attr_dha_fitting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if (RCSP_ADV_ASSISTED_HEARING)
    u8 dha_fitting_data[3 + 2 + 2 * DHA_FITTING_CHANNEL_MAX] = {0};
    get_dha_fitting_info(dha_fitting_data);
    rlen += add_one_attr(buf, buf_size, offset, attr, dha_fitting_data, sizeof(dha_fitting_data));
#endif
    return rlen;
}

static u32 common_function_attr_adaptive_noise_reduction_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
    u8 adaptive_noise_reduction_info[3] = {0};
    // 获取自适应降噪开关
    if (get_adaptive_noise_reduction_switch() != 0) {
        // 开启
        adaptive_noise_reduction_info[0] = 0x01;
    }
    // 获取设置自适应降噪状态
    if (get_adaptive_noise_reduction_reset_status() != 0) {
        // 设置中
        adaptive_noise_reduction_info[1] = 0x01;
    }
    // 获取设置自适应降噪结果
    if (get_adaptive_noise_reduction_reset_result() != 0) {
        // 设置中
        adaptive_noise_reduction_info[2] = 0x01;
    }
    rlen += add_one_attr(buf, buf_size, offset, attr, adaptive_noise_reduction_info, sizeof(adaptive_noise_reduction_info));
#endif
    return rlen;
}

static u32 common_function_attr_ai_no_pick_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_AI_NO_PICK
    u8 ai_no_pick_info[5] = {0};
    ai_no_pick_info[1] = 0xff;	// 获取智能免摘全部数据
    ai_no_pick_info[2] = get_ai_no_pick_switch() ? 0x01 : 0x00;
    ai_no_pick_info[3] = get_ai_no_pick_sensitivity();
    ai_no_pick_info[4] = get_ai_no_pick_auto_close_time();
    rlen += add_one_attr(buf, buf_size, offset, attr, ai_no_pick_info, sizeof(ai_no_pick_info));
#endif
    return rlen;
}

static u32 common_function_attr_scene_noise_reduction_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_SCENE_NOISE_REDUCTION
    u8 scene_noise_reduction_info[2] = {0};
    scene_noise_reduction_info[1] = get_scene_noise_reduction_type();
    rlen += add_one_attr(buf, buf_size, offset, attr, scene_noise_reduction_info, sizeof(scene_noise_reduction_info));
#endif
    return rlen;
}

static u32 common_function_attr_wind_noise_detection_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_WIND_NOISE_DETECTION
    u8 wind_noise_detection_info[2] = {0};
    wind_noise_detection_info[1] = get_wind_noise_detection_switch() ? 0x01 : 0x00;
    rlen += add_one_attr(buf, buf_size, offset, attr, wind_noise_detection_info, sizeof(wind_noise_detection_info));
#endif
    return rlen;
}

static u32 common_function_attr_voice_enhancement_mode_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if RCSP_ADV_VOICE_ENHANCEMENT_MODE
    u8 voice_enhancement_mode_info[2] = {0};
    voice_enhancement_mode_info[1] = get_voice_enhancement_mode_switch() ? 0x01 : 0x00;
    rlen += add_one_attr(buf, buf_size, offset, attr, voice_enhancement_mode_info, sizeof(voice_enhancement_mode_info));
#endif
    return rlen;
}

static u32 common_function_attr_1t2_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if TCFG_RCSP_DUAL_CONN_ENABLE
    u8 info_1t2[2] = {0};
    info_1t2[1] = rcsp_get_1t2_switch() ? 0x01 : 0x00;
    rlen += add_one_attr(buf, buf_size, offset, attr, info_1t2, sizeof(info_1t2));
#endif
    return rlen;
}

static u32 common_function_attr_karaoke_pre_fetch_eq_setting_info_get(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 karaoke_eq_pre_fetch_info[1 + 8 * 2] = {0}; // num + freq * 2
    u16 karaoke_eq_pre_fetch_size = sizeof(karaoke_eq_pre_fetch_info);
    RCSP_SETTING_OPT *setting_opt_hdl = get_rcsp_setting_opt_hdl(ATTR_TYPE_KARAOKE_EQ_SETTING);
    if (setting_opt_hdl) {
        karaoke_eq_pre_fetch_size = get_setting_extra_handle(setting_opt_hdl, karaoke_eq_pre_fetch_info, &karaoke_eq_pre_fetch_size);
        if (karaoke_eq_pre_fetch_size) {
            rlen = add_one_attr(buf, buf_size, offset, attr, karaoke_eq_pre_fetch_info, karaoke_eq_pre_fetch_size);
        }
    }
    return rlen;
}

static const attr_get_func target_common_function_get_tab[RCSP_DEVICE_STATUS_ATTR_TYPE_MAX] = {
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BATTERY 				          ] = common_function_attr_battery_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_VOL 					          ] = common_function_attr_vol_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO 			          ] = common_function_attr_dev_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ERROR_STATS			          ] = NULL,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_EQ_INFO				          ] = common_function_attr_eq_param_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BS_FILE_TYPE			          ] = common_function_attr_bs_file_type,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FUNCTION_MODE		          ] = common_function_attr_function_mode_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_COLOR_LED_SETTING_INFO 		  ] = common_function_attr_color_led_setting_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FMTX_FREQ			          ] = common_function_attr_fmtx_freq_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_SW		          ] = common_function_attr_bt_emitter_sw_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_CONNECT_STATES      ] = common_function_attr_bt_emitter_connect_state_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_HIGH_LOW_SET			          ] = common_function_attr_high_low_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_ALL_EQ_INFO 	      ] = common_function_attr_pre_fetch_all_eq_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ANC_VOICE                      ] = common_function_attr_anc_voice_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_FETCH_ALL_ANC_VOICE            ] = common_function_attr_anc_voice_info_fetch_all_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PHONE_SCO_STATE_INFO 	      ] = common_function_attr_phone_sco_state_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_MISC_SETTING_INFO 	      	  ] = common_function_attr_misc_setting_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_PRE_FETCH_KARAOKE_EQ_INFO	  ] = common_function_attr_karaoke_pre_fetch_eq_setting_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_KARAOKE_EQ_SETTING_INFO 		  ] = common_function_attr_karaoke_eq_setting_info_get,
    // 缺一个声卡功能19
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ASSISTED_HEARING               ] = common_function_attr_dha_fitting_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_ADAPTIVE_NOISE_REDUCTION       ] = common_function_attr_adaptive_noise_reduction_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_AI_NO_PICK                     ] = common_function_attr_ai_no_pick_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_SCENE_NOISE_REDUCTION          ] = common_function_attr_scene_noise_reduction_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_WIND_NOISE_DETECTION           ] = common_function_attr_wind_noise_detection_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_VOICE_ENHANCEMENT_MODE         ] = common_function_attr_voice_enhancement_mode_info_get,
    [RCSP_DEVICE_STATUS_ATTR_TYPE_1T2         					 ] = common_function_attr_1t2_info_get,
};



static bool rcsp_common_function_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    printf("rcsp_common_function_set\n");
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return false;
    }
    _OpCode = OpCode;
    _OpCode_SN = OpCode_SN;
    put_buf(data, len);
    attr_set(priv, data, len, common_function_set_tab, RCSP_DEVICE_STATUS_ATTR_TYPE_MAX, ble_con_handle, spp_remote_addr);
    if (rcspModel->err_code) {
        rcspModel->err_code = 0;
        return false;
    }
    return true;
}

static u32 rcsp_common_function_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    printf("rcsp_common_function_get, mask = %x\n", mask);
    return attr_get(priv, buf, buf_size, target_common_function_get_tab, RCSP_DEVICE_STATUS_ATTR_TYPE_MAX, mask);
}

// 设备状态更改
bool rcsp_device_status_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 function, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            return rcsp_common_function_set(priv, OpCode, OpCode_SN, data, len, ble_con_handle, spp_remote_addr);
        } else {
            return false;
        }
    }

    func_set func = set_tab[function];
    if (func) {
        return func(priv, data, len);
    }

    return false;
}

// 获取设备状态
u32 rcsp_device_status_get(void *priv, u8 function, u8 *data, u16 len, u8 *buf, u16 buf_size)
{
    ASSERT_SET_LEN(len, 4);
    u32 mask = READ_BIG_U32(data);
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            return rcsp_common_function_get(priv, buf, buf_size, mask);
        } else {
            return 0;
        }
    }

    func_get func = get_tab[function];
    if (func) {
        return func(priv, buf, buf_size, mask);
    }
    return 0;
}

// 设备状态更改
bool rcsp_device_status_cmd_set(void *priv, u8 OpCode, u8 OpCode_SN, u8 function, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
    if (COMMON_FUNCTION == function) {
        // 模式切换
        rcsp_msg_post(USER_MSG_RCSP_MODE_SWITCH, 2, (int)priv, (int)data[0]);
        return true;
    }
    return rcsp_device_status_set(priv, OpCode, OpCode_SN, function, data, len, ble_con_handle, spp_remote_addr);
}

// 设备状态更改
void rcsp_device_status_update(u8 function, u32 mask)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL || 0 == JL_rcsp_get_auth_flag()) {
        return;
    }
    u32 rlen = 0;
    u8 *buf = zalloc(FUNCTION_UPDATE_MAX_LEN);
    if (buf == NULL) {
        printf("no ram err!!\n");
        return;
    }
    buf[0] = function;
    if (function >= FUNCTION_MASK_MAX) {
        if (function == COMMON_FUNCTION) {
            rlen = attr_get((void *)rcspModel, buf + 1, FUNCTION_UPDATE_MAX_LEN - 1, target_common_function_get_tab, RCSP_DEVICE_STATUS_ATTR_TYPE_MAX, mask);
        }
    } else {
        func_get func = get_tab[function];
        if (func) {
            rlen = func(rcspModel, buf + 1, FUNCTION_UPDATE_MAX_LEN - 1, mask);
        }
    }
    if (rlen) {
        JL_CMD_send(JL_OPCODE_SYS_INFO_AUTO_UPDATE, buf, rlen + 1, JL_NOT_NEED_RESPOND, 0, NULL);
    }
    free(buf);
}

void rcsp_update_bt_emitter_connect_state(u8 state, u8 *addr)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return;
    }
    rcspModel->emitter_bt_state = state;
    memcpy(rcspModel->emitter_con_addr, addr, 6);
    rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_BT_EMITTER_CONNECT_STATES));
}

void rcsp_update_dev_state(u32 event, void *param)
{
#if JL_RCSP_EXTRA_FLASH_OPT
    if (rcsp_exflash_flash_opt_check(param)) {
        return;
    }
#endif
    switch (event) {
#if (TCFG_APP_RTC_EN && RCSP_APP_RTC_EN)
    case DEVICE_EVENT_FROM_ALM:
        printf("DEVICE_EVENT_FROM_ALM\n");
        rcsp_rtc_msg_deal(-1);
        break;
#endif
    case DEVICE_EVENT_FROM_LINEIN:
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO));
        break;
    }
}

// 获取当前模式
u8 rcsp_get_cur_mode(u8 app_mode)
{
    u8 current_mode;
    switch (app_mode) {
    case APP_MUSIC_TASK:
        current_mode = MUSIC_FUNCTION_MASK;
        break;
    case APP_RTC_TASK:
        current_mode = RTC_FUNCTION_MASK;
        break;
    case APP_LINEIN_TASK:
        current_mode = LINEIN_FUNCTION_MASK;
        break;
    case APP_FM_TASK:
        current_mode = FM_FUNCTION_MASK;
        break;
    case APP_SPDIF_TASK:
        current_mode = SPDIF_FUNCTION_MASK;
        break;
    case APP_PC_TASK:
        current_mode = PC_FUNCTION_MASK;
        break;
    case APP_BT_TASK:
    default:
        current_mode = BT_FUNCTION_MASK;
        break;
    }
    return current_mode;
}

void function_change_inform(u8 app_mode, u8 ret)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL) {
        return;
    }
    if (LINEIN_FUNCTION == rcspModel->cur_app_mode) {
        rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_DEV_INFO));
    }
    rcspModel->cur_app_mode = rcsp_get_cur_mode(app_mode);
    rcsp_device_status_update(COMMON_FUNCTION, BIT(RCSP_DEVICE_STATUS_ATTR_TYPE_FUNCTION_MODE));
}

// rcsp功能设置关闭
void rcsp_device_status_setting_stop(void)
{
    struct RcspModel *rcsp_model = rcsp_handle_get();
    if (rcsp_model == NULL) {
        return ;
    }
    switch (rcsp_model->cur_app_mode) {
    case BT_FUNCTION:
        printf("BT_FUNCTION STOP\n");
        break;
    case MUSIC_FUNCTION:
        printf("MUSIC_FUNCTION STOP\n");
#if TCFG_APP_MUSIC_EN
        // 音乐模式是否播放
        rcsp_music_func_stop();
#endif
        break;
    case RTC_FUNCTION:
        printf("RTC_FUNCTION STOP\n");
#if TCFG_APP_RTC_EN
        // RTC模式是否播放
        /* rtc_func_stop(); */
#endif
        break;
    case LINEIN_FUNCTION:
        printf("LINEIN_FUNCTION STOP\n");
#if (TCFG_APP_LINEIN_EN && !SOUNDCARD_ENABLE)
        rcsp_linein_func_stop();
#endif
        break;
    case FM_FUNCTION:
        printf("FM_FUNCTION STOP\n");
#if TCFG_APP_FM_EN
        rcsp_fm_func_stop();
#endif
        break;
        /* case FMTX_FUNCTION: */
        /*     printf("FMTX_FUNCTION STOP\n"); */
        /*     break; */
    }
    // 调用各种设置的release函数
    rcsp_opt_release();
}

#else

void rcsp_device_status_update(u8 function, u32 mask)
{
}

#endif
