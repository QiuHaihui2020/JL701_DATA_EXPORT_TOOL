#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_command.data.bss")
#pragma data_seg(".rcsp_command.data")
#pragma const_seg(".rcsp_command.text.const")
#pragma code_seg(".rcsp_command.text")
#endif
#include "app_config.h"
#include "rcsp_config.h"
#include "rcsp_command.h"
#include "btstack/avctp_user.h"
#include "rcsp_event.h"
#include "system/timer.h"
#include "ble_rcsp_server.h"
#include "rcsp_manage.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_tws.h"
#include "rcsp_bt_manage.h"
#include "JL_rcsp_protocol.h"

#if RCSP_MODE == RCSP_MODE_EARPHONE
#include "app_tone.h"
#include "tone_player.h"
#include "asm/dac.h"
#endif

#if (RCSP_MODE)

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

#if RCSP_BT_CONTROL_ENABLE
void rcsp_command_send_bt_scan_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi)
{
    u8 send_len = sizeof(dev_class) + 6 + sizeof(rssi) + sizeof(name_len) + name_len;
    u8 *send_buf = (u8 *)zalloc(send_len);
    if (send_buf == NULL) {
        return ;
    }
    WRITE_BIG_U32(send_buf, dev_class);
    memcpy(send_buf + sizeof(dev_class), addr, 6);
    memcpy(send_buf + sizeof(dev_class) + 6, &rssi, 1);
    memcpy(send_buf + sizeof(dev_class) + 6 + sizeof(rssi), &name_len, 1);
    memcpy(send_buf + sizeof(dev_class) + 6 + sizeof(rssi) + sizeof(name_len), name, name_len);
    JL_CMD_send(JL_OPCODE_SYS_UPDATE_BT_STATUS, send_buf, send_len, JL_NOT_NEED_RESPOND, 0, NULL);
    free(send_buf);
    rcsp_printf("bt name = %s\n", name);
}

void rcsp_command_send_conncecting_bt_status(u8 *addr, u8 status)
{
    u8 send_buf[7] = {0};
    send_buf[0] = status;
    memcpy(send_buf, addr, 6);
    JL_CMD_send(JL_OPCODE_SYS_EMITTER_BT_CONNECT_STATUS, send_buf, sizeof(send_buf), JL_NOT_NEED_RESPOND, 0, NULL);
}
#endif

#if RCSP_ADV_FIND_DEVICE_ENABLE

static u16 find_device_timer = 0;
static u8 find_device_key_flag = 0;

static void rcsp_command_send_find_device(void *priv)
{
    struct RcspModel *rcspModel = (struct RcspModel *) priv;
    if (rcspModel == NULL) {
        return ;
    }
    if (!rcspModel->find_dev_en) {
        return;
    }
    //				 查找手机	播放铃声	超时时间(默认10s)
    u8 send_buf[4] = {0x00, 	0x01, 		0x00, 0x0A};
    JL_CMD_send(JL_OPCODE_SYS_FIND_DEVICE, send_buf, sizeof(send_buf), JL_NOT_NEED_RESPOND, 0, NULL);
}

u8 rcsp_find_device_key_flag_get(void)
{
    return find_device_key_flag;
}

void rcsp_send_find_device_stop(void)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL || 0 == get_rcsp_connect_status()) {
        return ;
    }
    if (!rcspModel->find_dev_en) {
        return ;
    }
    //				 查找手机	关闭铃声	超时时间(不限制)
    u8 send_buf[4] = {0x00, 	0x00, 		0x00, 0x00};
    JL_CMD_send(JL_OPCODE_SYS_FIND_DEVICE, send_buf, sizeof(send_buf), JL_NOT_NEED_RESPOND, 0, NULL);
}

extern void reset_find_device_buf(void);
void rcsp_find_device_reset(void)
{
    /* printf("rcsp_find %s, %s, %d, find_device_key_flag=0\n", __FILE__, __FUNCTION__, __LINE__); */
    reset_find_device_buf();
    find_device_key_flag = 0;
    if (find_device_timer) {
        sys_timeout_del(find_device_timer);
        find_device_timer = 0;
    }
}

void rcsp_stop_find_device(void *priv)
{
    if (!find_device_key_flag) {
        return ;
    }
    /* printf("rcsp_find %s, %s, %d, find_device_key_flag=0\n", __FILE__, __FUNCTION__, __LINE__); */
    find_device_key_flag = 0;
    rcsp_find_device_reset();
    rcsp_send_find_device_stop();

#if RCSP_MODE == RCSP_MODE_EARPHONE
    // 0 - way, 1 - player, 2 - find_device_key_flag
    u8 other_opt[3] = {0};
#if TCFG_USER_TWS_ENABLE && RCSP_ADV_EN
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        other_opt[2] = 2;
        extern void find_device_sync(u8 * param, u32 msec);
        find_device_sync(other_opt, 300);
        return;
    }
#endif
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_FIND_DEVICE_RESUME, other_opt, sizeof(other_opt));

#else

    extern void rcsp_set_vol_for_find_device(u8 vol_flag);
    rcsp_set_vol_for_find_device(find_device_key_flag);

#if (RCSP_MODE == RCSP_MODE_WATCH)
    if (!(bt_get_curr_channel_state() & A2DP_CH)) {
        puts("start to connect a2dp ");
        bt_cmd_prepare(USER_CTRL_CONN_A2DP, 0, NULL);
    }
#endif

#endif
}

void find_device_timeout_handle(u32 sec)
{
    /* printf("rcsp_find %s, %s, %d, find_device_key_flag=1\n", __FILE__, __FUNCTION__, __LINE__); */
    find_device_key_flag = 1;
    if (sec && (0 == find_device_timer)) {
        find_device_timer = sys_timeout_add(NULL, rcsp_stop_find_device, sec * 1000);
    }

#if 1//RCSP_MODE != RCSP_MODE_EARPHONE
    extern void rcsp_set_vol_for_find_device(u8 vol_flag);
    rcsp_set_vol_for_find_device(find_device_key_flag);
#endif
}

#if RCSP_MODE == RCSP_MODE_EARPHONE
/* extern void audio_dac_ch_mute(struct audio_dac_hdl *dac, u8 ch, u8 mute); */
// channel : L - 左耳，R - 右耳
// mute : 1 - 静音，0 - 有声音
static void earphone_mute(u8 channel, u8 mute)
{
    if ('L' == channel) {
        channel = 1;
    } else if ('R' == channel) {
        channel = 2;
    } else {
        return;
    }
    printf("rcsp_find %s, channel:%d, mute:%d\n", __FUNCTION__, channel, mute);
    audio_dac_ch_mute(NULL, channel, mute);
#if CONFIG_CPU_BR36
    extern void audio_dac_set_unmute_disable_ch(u8 ch, u8 disable_en);
    audio_dac_set_unmute_disable_ch(channel, mute);
#endif
}

void earphone_mute_handler(u8 *other_opt, u32 msec);
static void earphone_mute_timer_func(void *param)
{
    u8 way = ((u8 *)param)[0];
    u8 player = ((u8 *)param)[1];
    earphone_mute('L', 0);
    earphone_mute('R', 0);

    printf("rcsp_find %s, %s, %d, way:%d, player:%d\n", __FILE__, __FUNCTION__, __LINE__, way, player);
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        switch (way) {
        case 1: // 左侧播放
            earphone_mute('R', 1);
            break;
        case 2: // 右侧播放
            earphone_mute('L', 1);
            break;
        }
    }
#endif
    switch (player) {
    case 0:
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        // app端播放，设备端不播放
        tone_player_stop();
        break;
    case 1:
        /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        // 设备端播放
        play_tone_file(get_tone_files()->normal);
        earphone_mute_handler(param, 300);
        break;
    }
}

void earphone_mute_handler(u8 *other_opt, u32 msec)
{
    /* printf("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    /* put_buf(other_opt, 3); */
    static u16 earphone_mute_timer = 0;
    static u8 tmp_param[3] = {0};
    int size_other_opt = sizeof(tmp_param);
    memcpy(tmp_param, other_opt, size_other_opt);
#if TCFG_USER_TWS_ENABLE
    if ((tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) && (2 == tmp_param[2])) {
        /* printf("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        rcsp_find_device_reset();
        earphone_mute_timer_func(tmp_param);
    }
#endif
    if (earphone_mute_timer) {
        /* printf("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        sys_timeout_del(earphone_mute_timer);
        earphone_mute_timer = 0;
    }

    if (find_device_key_flag) {
        /* printf("rcsp_find %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
        earphone_mute_timer = sys_timeout_add(tmp_param, earphone_mute_timer_func, msec);
    }
}

#endif

void find_decice_tws_connect_handle(u8 flag, u8 *param)
{
#if TCFG_USER_TWS_ENABLE
    static u8 sync_param[4] = {0};
    u8 other_opt[3] = {0};
    /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
    /* put_buf(param, 2); */
    /* put_buf(param, 4); */
    /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
    /* put_buf(sync_param, 4); */
    switch (flag) {
    case 0:
        if (find_device_key_flag) {
            memcpy(other_opt, sync_param, 2);
            /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
            /* put_buf(other_opt, 3); */
            extern void find_device_stop_timer(u8 * param, u32 msec);
            find_device_stop_timer(other_opt, 300);
            memcpy(other_opt, sync_param + 2, 2);
            /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
            /* put_buf(other_opt, 3); */
#if 1//RCSP_ADV_EN
            extern void find_device_sync(u8 * param, u32 msec);
            find_device_sync(other_opt, 300);
#endif
        }
        break;
    case 1:
        memcpy(sync_param, param, 2);
        /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
        /* put_buf(sync_param, 4); */
        break;
    case 2:
        memcpy(sync_param + 2, param, 2);
        /* printf("rcsp_find %s, %s, %d, flag:%d\n", __FILE__, __FUNCTION__, __LINE__, flag); */
        /* put_buf(sync_param, 4); */
        break;
    }
#endif // TCFG_USER_TWS_ENABLE

}


void rcsp_find_device(void *priv)
{
    struct RcspModel *rcspModel = rcsp_handle_get();
    if (rcspModel == NULL || 0 == get_rcsp_connect_status()) {
        return ;
    }
    if (!rcspModel->find_dev_en) {
        return ;
    }

    if (find_device_key_flag) {
        rcsp_stop_find_device(NULL);
    } else {
        /* printf("rcsp_find %s, %s, %d, find_device_key_flag=1\n", __FILE__, __FUNCTION__, __LINE__); */
        find_device_key_flag = 1;
#if ((RCSP_MODE == RCSP_MODE_EARPHONE) || (RCSP_MODE == RCSP_MODE_SOUNDBOX))
        if ((1 == rcspModel->A_platform) && (0 != bt_get_curr_channel_state())) {
            // iOS平台
            bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
        }
#else
        if (bt_get_curr_channel_state() & A2DP_CH) {
            puts("start to disconnect a2dp ");
            bt_cmd_prepare(USER_CTRL_DISCONN_A2DP, 0, NULL);
        }
#endif
        rcsp_command_send_find_device(rcspModel);
    }
#if (RCSP_MODE == RCSP_MODE_WATCH)
    extern void rcsp_adv_find_phone_handler(void);
    rcsp_adv_find_phone_handler();
#endif
}


#if (RCSP_MODE == RCSP_MODE_WATCH)
int find_phone_timer_flag = 0;

void rcsp_find_phone_reset(void)
{
    r_printf("%s\n", __func__);
    if (find_phone_timer_flag) {
        sys_timeout_del(find_phone_timer_flag);
        find_phone_timer_flag = 0;
    }
}

void rcsp_adv_find_phone_handler(void)
{
    r_printf("%s\n", __func__);
    rcsp_find_phone_reset();
    if (!find_phone_timer_flag) {
        r_printf("time scan play find phone music\n");
        find_phone_timer_flag = sys_timeout_add(NULL, rcsp_find_device, 10 * 1000);
    }

}
#endif // (RCSP_MODE == RCSP_MODE_WATCH)

#else // RCSP_ADV_FIND_DEVICE_ENABLE

u8 rcsp_find_device_key_flag_get(void)
{
    return 0;
}

#endif // RCSP_ADV_FIND_DEVICE_ENABLE


#endif//RCSP_MODE


