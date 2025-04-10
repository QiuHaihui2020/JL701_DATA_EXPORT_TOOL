#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_device_feature.data.bss")
#pragma data_seg(".rcsp_device_feature.data")
#pragma const_seg(".rcsp_device_feature.text.const")
#pragma code_seg(".rcsp_device_feature.text")
#endif
#include "rcsp_device_feature.h"
#include "app_config.h"
#include "rcsp_config.h"
#include "btstack/avctp_user.h"
#include "rcsp_event.h"
#include "custom_cfg.h"
#include "JL_rcsp_packet.h"
#include "rcsp_extra_flash_opt.h"
#include "app_task.h"
#include "rcsp_vol_setting.h"
#include "rcsp_device_status.h"
#include "JL_rcsp_api.h"
#include "JL_rcsp_attr.h"

#if (RCSP_MODE)
#pragma pack(1)
struct _SYS_info {
    u8 bat_lev;
    u8 sys_vol;
    u8 max_vol;
    u8 vol_is_sync;
};

struct _EDR_info {
    u8 addr_buf[6];
    u8 profile;
    u8 state;
};
#pragma pack()

static u32 target_feature_attr_protocol_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 ver = get_rcsp_version();
    rlen = add_one_attr(buf, buf_size, offset, attr, &ver, 1);
    return rlen;
}
static u32 target_feature_attr_sys_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }
    struct _SYS_info sys_info = {0};
#if (RCSP_MODE != RCSP_MODE_EARPHONE)
    extern u8 get_vbat_percent(void);
    sys_info.bat_lev = get_vbat_percent(); //get_battery_level() / 10;
    rcsp_get_max_vol_info(&sys_info.max_vol);
    rcsp_get_cur_dev_vol_info(&sys_info.sys_vol);
#endif
#if BT_SUPPORT_MUSIC_VOL_SYNC || TCFG_BT_VOL_SYNC_ENABLE
    extern u8 avctp_get_remote_vol_sync(bd_addr_t addr);
    if (avctp_get_remote_vol_sync(bt_get_current_remote_addr()) || (0 == rcspModel->A_platform)) {
        sys_info.vol_is_sync |= BIT(0);
        rcspModel->dev_vol_sync = 1;
    }
#endif
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&sys_info, sizeof(sys_info));
    return rlen;
}
static u32 target_feature_attr_edr_addr(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    struct _EDR_info edr_info;
    extern const u8 *bt_get_mac_addr();
    u8 taddr_buf[6];
    memcpy(taddr_buf, bt_get_mac_addr(), 6);
    edr_info.addr_buf[0] =  taddr_buf[5];
    edr_info.addr_buf[1] =  taddr_buf[4];
    edr_info.addr_buf[2] =  taddr_buf[3];
    edr_info.addr_buf[3] =  taddr_buf[2];
    edr_info.addr_buf[4] =  taddr_buf[1];
    edr_info.addr_buf[5] =  taddr_buf[0];
    edr_info.profile = 0x0E;
    if (get_defalut_bt_channel_sel()) {
        edr_info.profile |= BIT(7);
    } else {
        edr_info.profile &= ~BIT(7);
    }
    if (bt_get_connect_status() ==  BT_STATUS_WAITINT_CONN) {
        edr_info.state = 0;
    } else {
        edr_info.state = 1;
    }
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&edr_info, sizeof(struct _EDR_info));
    return rlen;
}
static u32 target_feature_attr_platform(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    return rlen;
}
static u32 target_feature_function_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 tmp_buf[6] = {0};
    u32 func_mask = app_htonl(rcspModel->function_mask);
#if RCSP_DEVICE_STATUS_ENABLE
    u8 cur_fun = rcsp_get_cur_mode(app_get_curr_task());
#else
    u8 cur_fun = BT_FUNCTION;
#endif
    memcpy(tmp_buf, (u8 *)&func_mask, 4);
    memcpy(tmp_buf + 4, &cur_fun, 1);
    memcpy(tmp_buf + 5, &rcspModel->music_icon_mask, 1);
    rlen = add_one_attr(buf, buf_size, offset, attr, tmp_buf, sizeof(tmp_buf));
    return rlen;
}
static u32 target_feature_dev_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 ver = get_vid_pid_ver_from_cfg_file(GET_VER_FROM_EX_CFG);
    ver = READ_BIG_U16(&ver);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&ver, sizeof(ver));
    return rlen;
}
static u32 target_feature_sdk_type(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 sdk_type = rcspModel->sdk_type;
    rlen = add_one_attr(buf, buf_size, offset, attr, &sdk_type, 1);
    return rlen;
}
static u32 target_feature_uboot_version(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if CONFIG_CPU_BR28 || CONFIG_CPU_BR36
    u8 *uboot_ver_flag = (u8 *)(0x1C000 - 0x8);
    u8 uboot_version[2] = {uboot_ver_flag[0], uboot_ver_flag[1]};
    rlen = add_one_attr(buf, buf_size, offset, attr, uboot_version, sizeof(uboot_version));
#endif
    return rlen;
}
static u32 target_feature_ota_double_parition(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }
    u32 rlen = 0;
    u8 double_partition_value;
    u8 ota_loader_need_download_flag;
    u8 update_channel_sel;

    if (rcspModel->ota_type) {
        double_partition_value = 0x1;
        ota_loader_need_download_flag = 0x00;
    } else {
        double_partition_value = 0x0;
        ota_loader_need_download_flag = 0x01;
    }
    update_channel_sel = 0x1;       //强制使用BLE升级
    u8 update_param[3] = {
        double_partition_value,
        ota_loader_need_download_flag,
        update_channel_sel,
    };

    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)update_param, sizeof(update_param));
    return rlen;
}
static u32 target_feature_ota_update_status(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u8 update_status_value[3] = {0x0};
#if JL_RCSP_EXTRA_FLASH_OPT
    update_status_value[2] = rcsp_eflash_update_flag_get();
#endif
    rlen = add_one_attr(buf, buf_size, offset, attr, update_status_value, sizeof(update_status_value));
    return rlen;
}
static u32 target_feature_pid_vid(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 pvid[2] = {0};
    pvid[0] =  get_vid_pid_ver_from_cfg_file(GET_VID_FROM_EX_CFG);
    pvid[1] =  get_vid_pid_ver_from_cfg_file(GET_PID_FROM_EX_CFG);
    pvid[0] = READ_BIG_U16(&pvid[0]);
    pvid[1] = READ_BIG_U16(&pvid[1]);
    rlen = add_one_attr(buf, buf_size, offset, attr, (u8 *)&pvid, sizeof(pvid));
    return rlen;
}
static u32 target_feature_authkey(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if VER_INFO_EXT_COUNT
    u8 authkey_len = 0;
    u8 *local_authkey_data = NULL;
    get_authkey_procode_from_cfg_file(&local_authkey_data, &authkey_len, GET_AUTH_KEY_FROM_EX_CFG);
    if (local_authkey_data && authkey_len) {
        printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        put_buf(local_authkey_data, authkey_len);
        rlen = add_one_attr(buf, buf_size, offset, attr, local_authkey_data, authkey_len);
    }
#endif
    return rlen;
}
static u32 target_feature_procode(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
#if VER_INFO_EXT_COUNT
    u8 procode_len = 0;
    u8 *local_procode_data = NULL;
    get_authkey_procode_from_cfg_file(&local_procode_data, &procode_len, GET_PRO_CODE_FROM_EX_CFG);
    if (local_procode_data && procode_len) {
        printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
        put_buf(local_procode_data, procode_len);
        rlen = add_one_attr(buf, buf_size, offset, attr, local_procode_data, procode_len);
    }
#endif
    return rlen;
}
static u32 target_feature_mtu(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;
    u16 rx_max_mtu = JL_packet_get_rx_max_mtu();
    u16 tx_max_mtu = JL_packet_get_tx_max_mtu();
    u8 t_buf[4];
    t_buf[0] = (tx_max_mtu >> 8) & 0xFF;
    t_buf[1] = tx_max_mtu & 0xFF;
    t_buf[2] = (rx_max_mtu >> 8) & 0xFF;
    t_buf[3] = rx_max_mtu & 0xFF;
    rlen = add_one_attr(buf, buf_size, offset,  attr, t_buf, 4);
    return rlen;
}
static u32 target_feature_ble_only(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    u32 rlen = 0;

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    u8 taddr_buf[8];
    taddr_buf[0] = 0;
    le_controller_get_mac(taddr_buf + 1);
    for (u8 i = 0; i < (6 / 2); i++) {
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
        taddr_buf[7 - i - 1] ^= taddr_buf[i + 1];
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
    }
    taddr_buf[7] |= BIT(1); // 是否支持Le Audio功能
    if (is_cig_phone_conn()) {
        swapX(bt_get_mac_addr(), taddr_buf + 1, 6);
        taddr_buf[7] |= BIT(2);  // Le Audio是否已连接
    }
#else
    u8 taddr_buf[7];
    taddr_buf[0] = 0;
    le_controller_get_mac(taddr_buf + 1);
    for (u8 i = 0; i < (6 / 2); i++) {
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
        taddr_buf[7 - i - 1] ^= taddr_buf[i + 1];
        taddr_buf[i + 1] ^= taddr_buf[7 - i - 1];
    }
#endif
    rlen = add_one_attr(buf, buf_size, offset,  attr, taddr_buf, sizeof(taddr_buf));
    return rlen;
}
static u32 target_feature_bt_emitter_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 val = 0;
    val |= rcspModel->emitter_en;
    val |= rcspModel->emitter_sw;
    printf("val = %d, emitter_en = %d, emitter_sw = %d\n", val, rcspModel->emitter_en, rcspModel->emitter_sw);
    rlen = add_one_attr(buf, buf_size, offset,  attr, &val, 1);
    return rlen;
}

static u32 target_feature_md5_game_support(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }

    u32 rlen = 0;
    u8 ext_function_flag[2] = {0};
    u8 md5_support = 0;
#if RCSP_MODE == RCSP_MODE_EARPHONE
    md5_support = UPDATE_MD5_ENABLE;
#endif
#if RCSP_MODE != RCSP_MODE_EARPHONE
    if (rcspModel->game_mode_en) {
#else
    {
#endif
        md5_support |= BIT(1);
    }
    if (rcspModel->find_dev_en) {
        md5_support |= BIT(2);
    }
    if (rcspModel->karaoke_en) {
        md5_support |= BIT(3);
    }
    if (rcspModel->sound_effects_disable) {
        md5_support |= BIT(4);
    }
    if (rcspModel->extra_flash_en) {
        md5_support |= BIT(5);
    }
#if (RCSP_ADV_ANC_VOICE)
    md5_support |= BIT(6);
#endif
#if ((defined CONFIG_DEBUG_RECORD_ENABLE) && CONFIG_DEBUG_RECORD_ENABLE)
    md5_support |= BIT(7);
#endif
    ext_function_flag[0] = md5_support;
    u8 ext_function_flag_byte1 = 0;
#if (RCSP_ADV_ASSISTED_HEARING)
    ext_function_flag_byte1 |= BIT(0);
#endif
#if RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
    ext_function_flag_byte1 |= BIT(1);
#endif
#if RCSP_ADV_AI_NO_PICK
    ext_function_flag_byte1 |= BIT(3);
#endif
#if RCSP_ADV_SCENE_NOISE_REDUCTION
    ext_function_flag_byte1 |= BIT(4);
#endif
#if RCSP_ADV_WIND_NOISE_DETECTION
    ext_function_flag_byte1 |= BIT(5);
#endif
#if RCSP_ADV_VOICE_ENHANCEMENT_MODE
    ext_function_flag_byte1 |= BIT(6);
#endif
#if TCFG_RCSP_DUAL_CONN_ENABLE
    // 是否一拖二
    ext_function_flag_byte1 |= BIT(7);
#endif
    ext_function_flag[1] = ext_function_flag_byte1;

    rlen = add_one_attr(buf, buf_size, offset,  attr, ext_function_flag, 2);

    return rlen;
}

static u32 target_feature_file_transfer_info(void *priv, u8 attr, u8 *buf, u16 buf_size, u32 offset)
{
    struct RcspModel *rcspModel = (struct RcspModel *)priv;
    if (rcspModel == NULL) {
        return 0;
    }
    u32 config = 0;
    u8 send_buf[4] = {0};

    if (rcspModel->file_transfer_mode) {
        config |= BIT(0);//文件传输是支持配置分包crc校验使能, 这里默认支持
    }

    if (rcspModel->file_trans_back_mode) {
        config |= BIT(1); // 文件回传支持手机选择外挂flash还是sd卡
    }

    if (rcspModel->file_simple_trans_mode) {
        config |= BIT(2);
    }

    WRITE_BIG_U32(send_buf, config);

    u32 rlen = add_one_attr(buf, buf_size, offset,  attr, send_buf, sizeof(config));
    return rlen;
}

static const attr_get_func target_feature_mask_get_tab[RCSP_DEVICE_FEATURE_ATTR_TYPE_MAX] = {
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_PROTOCOL_VERSION  ] = target_feature_attr_protocol_version,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_SYS_INFO          ] = target_feature_attr_sys_info,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_EDR_ADDR          ] = target_feature_attr_edr_addr,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_PLATFORM          ] = target_feature_attr_platform,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_FUNCTION_INFO     ] = target_feature_function_info,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_VERSION       ] = target_feature_dev_version,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_SDK_TYPE          ] = target_feature_sdk_type,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_UBOOT_VERSION     ] = target_feature_uboot_version,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DOUBLE_PARITION   ] = target_feature_ota_double_parition,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_UPDATE_STATUS     ] = target_feature_ota_update_status,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_VID_PID       ] = target_feature_pid_vid,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_AUTHKEY       ] = target_feature_authkey,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_PROCODE       ] = target_feature_procode,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_DEV_MAX_MTU       ] = target_feature_mtu,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_CONNECT_BLE_ONLY  ] = target_feature_ble_only,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_BT_EMITTER_INFO   ] = target_feature_bt_emitter_info,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_MD5_GAME_SUPPORT  ] = target_feature_md5_game_support,
    [RCSP_DEVICE_FEATURE_ATTR_TYPE_FILE_TRANSFER_INFO] = target_feature_file_transfer_info,
};

// 解析设备特征相关的rcsp数据
u32 rcsp_target_feature_parse_packet(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    printf("rcsp_target_feature_parse_packet, mask = %x\n", mask);
    return attr_get(priv, buf, buf_size, target_feature_mask_get_tab, RCSP_DEVICE_FEATURE_ATTR_TYPE_MAX, mask);
}

#if TCFG_CONNECTED_ENABLE || TCFG_BROADCAST_ENABLE
static RCSP_LeAudioMode _leAudioMode = RCSP_LeAudioModeNone;
/**
 * @brief 设置leaudio的状态
 *
 * #param RCSP_LeAudioMode
 */
void rcsp_set_LeAudio_mode(RCSP_LeAudioMode mode)
{
    _leAudioMode = mode;
}
/**
 * @brief 获取leaudio的状态
 *
 * @result RCSP_LeAudioMode
 */
RCSP_LeAudioMode rcsp_get_LeAudio_mode()
{
    return _leAudioMode;
}
#endif

#endif//RCSP_MODE


