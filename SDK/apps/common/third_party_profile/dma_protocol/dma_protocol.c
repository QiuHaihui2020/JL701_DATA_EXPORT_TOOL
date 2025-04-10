#include "system/includes.h"
#include "media/includes.h"
#include "sdk_config.h"
#include "app_msg.h"
/* #include "earphone.h" */
#include "bt_tws.h"
#include "app_main.h"
#include "battery_manager.h"
#include "app_power_manage.h"
#include "btstack/avctp_user.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "clock_manager/clock_manager.h"
#include "btstack/third_party/app_protocol_event.h"
#include "multi_protocol_main.h"
#include "app_ble_spp_api.h"
#include "asm/charge.h"
#include "dma_platform_api.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & DMA_EN)

#if 1
#define APP_DMA_LOG       printf
#define APP_DMA_DUMP      put_buf
#else
#define APP_DMA_LOG(...)
#define APP_DMA_DUMP(...)
#endif

//u8 dma_tws_support = 1;

bool dma_is_tws_master_role()
{
#if TCFG_USER_TWS_ENABLE
    return (tws_api_get_role() == TWS_ROLE_MASTER);
#endif
    return 1;
}


static int dma_get_tws_side()
{
    // CHECK_STATUS_TWS_SIDE:     /*0是单耳，1是左耳，2是右耳*/
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_local_channel() == 'R') {
        return 2;
    } else if (tws_api_get_local_channel() == 'L') {
        return 1;
    }
#endif
    return 0;
}

//*********************************************************************************//
//                                 DMA认证信息                                     //
//*********************************************************************************//
#define DMA_PRODUCT_INFO_TEST       1

#if DMA_PRODUCT_INFO_TEST
static const char *dma_product_id  = "asoYwYghv0fy6HFexl6bTIZUHjyZGnEH";
static const char *dma_triad_id    = "000AGDdk0000000400000001";
static const char *dma_secret      = "c994d7c408b7a61a";
#endif
static const char *dma_product_key = "Yg5Xb2NOK01bgB9csSUHAAgG4lUjMXXZ";

#define DMA_PRODUCT_ID_LEN      65
#define DMA_PRODUCT_KEY_LEN     65
#define DMA_TRIAD_ID_LEN        32
#define DMA_SECRET_LEN          32

#define DMA_LEGAL_CHAR(c)       ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))

static u16 dma_get_one_info(const u8 *in, u8 *out)
{
    int read_len = 0;
    const u8 *p = in;

    while (DMA_LEGAL_CHAR(*p) && *p != ',') { //read Product ID
        *out++ = *p++;
        read_len++;
    }
    return read_len;
}

struct _DmaUserInformation dma_device_info = {
    .name                          = "JL_Dueros",
    .device_type                   = "SPEAKER",
    .manufacturer                  = "JL",
    .firmware_version              = "B.1.0",
    .software_version              = "J.1.0",
    .initiator_type                = 1,
    .support_sota                  = false,
    .is_earphone                   = true,
    .support_dual_record           = false,
    .support_local_voice_wake      = true,
    .support_local_audio_file      = false,
    .support_media                 = true,
    .support_model_beamforming_asr = true,
    .disable_heart_beat            = true,
    .support_log                   = false,
    .battery_structure             = 0,
    .touch_setting_type            = 0,
    .support_local_wakeup          = false,
    .support_find_earphone         = false,
};

u8 read_dma_product_info_from_flash(u8 *read_buf, u16 buflen)
{
    u8 *rp = read_buf;
    //const u8 *dma_ptr = (u8 *)app_protocal_get_license_ptr();
    const u8 *dma_ptr = NULL;

    if (dma_ptr == NULL) {
        return FALSE;
    }

    if (dma_get_one_info(dma_ptr, rp) != 32) {
        return FALSE;
    }
    dma_ptr += 33;

    rp = read_buf + DMA_PRODUCT_ID_LEN;
    memcpy(rp, dma_product_key, strlen(dma_product_key));

    rp = read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN;
    if (dma_get_one_info(dma_ptr, rp) != 24) {
        return FALSE;
    }
    dma_ptr += 25;

    rp = read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN + DMA_TRIAD_ID_LEN;
    if (dma_get_one_info(dma_ptr, rp) != 16) {
        return FALSE;
    }

    return TRUE;
}

__attribute__((weak))
void bt_update_testbox_addr(u8 *addr)
{

}

void dueros_dma_manufacturer_info_init()
{
    u8 read_buf[DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN + DMA_TRIAD_ID_LEN + DMA_SECRET_LEN + 1] = {0};
    bool ret = FALSE;

    APP_DMA_LOG("dueros_dma_manufacturer_info_init\n");

#if DMA_PRODUCT_INFO_TEST
    memcpy(read_buf, dma_product_id, strlen(dma_product_id));
    memcpy(read_buf + DMA_PRODUCT_ID_LEN, dma_product_key, strlen(dma_product_key));
    memcpy(read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN, dma_triad_id, strlen(dma_triad_id));
    memcpy(read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN + DMA_TRIAD_ID_LEN, dma_secret, strlen(dma_secret));
    ret = TRUE;
#else
    //  ret = read_dma_product_info_from_flash(read_buf, sizeof(read_buf));
#endif

    if (ret == TRUE) {
        APP_DMA_LOG("read license success\n");
        APP_DMA_LOG("product id: %s\n", read_buf);
        APP_DMA_LOG("product key: %s\n", read_buf + DMA_PRODUCT_ID_LEN);
        APP_DMA_LOG("triad id: %s\n", read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN);
        APP_DMA_LOG("secret: %s\n", read_buf + DMA_PRODUCT_ID_LEN + DMA_PRODUCT_KEY_LEN + DMA_TRIAD_ID_LEN);
        dma_set_product_id_key(read_buf);
    } else {
        dma_set_product_id_key(NULL);
    }

#if 1
    u8 mac[] = {0xF4, 0x43, 0x8D, 0x29, 0x17, 0x02};
    u8 ble_mac[6];
    void bt_update_mac_addr(u8 * addr);
    void lmp_hci_write_local_address(const u8 * addr);
    void bt_update_testbox_addr(u8 * addr);
    // extern int le_controller_set_mac(void *addr);
    //extern void bt_make_ble_address(u8 * ble_address, u8 * edr_address);
    bt_update_mac_addr(mac);
    lmp_hci_write_local_address(mac);
    bt_update_testbox_addr(mac);
    // bt_make_ble_address(ble_mac, mac);
    // le_controller_set_mac(mac); //修改BLE地址
    dma_ble_set_mac_addr(mac);
    APP_DMA_DUMP(mac, 6);
    APP_DMA_DUMP(ble_mac, 6);
#endif
}

static int dma_protocol_bt_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        break;
    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_MOBILE_DISCONNECTED);
        break;
    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        dma_app_speech_stop();
        dma_ble_adv_enable(0);
        break;
    case BT_STATUS_PHONE_HANGUP:
        dma_ble_adv_enable(1);
        break;

    case BT_STATUS_PHONE_MANUFACTURER:
        if (!dma_pair_state()) {
            if (bt_get_call_status() != BT_CALL_HANGUP) {
                dma_ble_adv_enable(0);
            }
        }
        break;
    case BT_STATUS_SIRI_OPEN:
    case BT_STATUS_SIRI_CLOSE:
    case BT_STATUS_SIRI_GET_STATE:
        /* case BT_STATUS_VOICE_RECOGNITION: */
        if ((app_var.siri_stu == 1) || (app_var.siri_stu == 2)) {
            dma_app_speech_stop();
            dma_ble_adv_enable(0);
        } else if (app_var.siri_stu == 0) {
            dma_ble_adv_enable(1);
        }
        break;
    case BT_STATUS_CONN_A2DP_CH:
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_MOBILE_CONNECTED);
        break;
    }
    return 0;
}

static int dma_app_status_event_handler(int *msg)
{
    struct bt_event *bt = (struct bt_event *)msg;
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
    dma_protocol_bt_status_event_handler(bt);
    return 0;
}
APP_MSG_HANDLER(dma_protocol_msg_handler) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = dma_app_status_event_handler,
};

static void __tws_rx_from_sibling(u8 *data)
{
    /* u16 opcode = (data[1] << 8) | data[0]; */
    /* u16 len = (data[3] << 8) | data[2]; */
    /* u8 *rx_data = data + 4; */
    /* switch (opcode) { */
    /* case APP_PROTOCOL_TWS_FOR_LIB: */
    /* APP_PROTOCOL_LOG("APP_PROTOCOL_TWS_FOR_LIB"); */
    /* dma_tws_data_deal(rx_data, len); */
    /* break; */
    /* } */
    /* free(data); */
}

static void dma_app_protocol_rx_from_sibling(void *_data, u16 len, bool rx)
{
    int err = 0;
    if (rx) {
        printf(">>>%s \n", __func__);
        printf("len :%d\n", len);
        put_buf(_data, len);
        u8 *rx_data = malloc(len);
        memcpy(rx_data, _data, len);

        int msg[4];
        msg[0] = (int)__tws_rx_from_sibling;
        msg[1] = 1;
        msg[2] = (int)rx_data;
        err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (err) {
            printf("tws rx post fail\n");
        }
    }
}

//发送给对耳
REGISTER_TWS_FUNC_STUB(dma_app_tws_rx_from_sibling) = {
    .func_id = 0xFF5432DA,
    .func    = dma_app_protocol_rx_from_sibling,
};

static void dma_protocol_bt_tws_event_handler(struct tws_event *tws)
{
    int role = tws->args[0];
    int phone_link_connection = tws->args[1];
    int reason = tws->args[2];
    u8 phone_addr[6];
    u8 btaddr[6];

    switch (tws->event) {
    case TWS_EVENT_CONNECTED:
        if (!dma_is_tws_master_role()) {
            dma_all_disconnecd();
            dma_ble_adv_enable(0);
        } else {
            if (ai_mic_is_busy()) { //mic在解码过程中，从机连接成功，挂起音频
                /* ai_mic_tws_sync_suspend(); */
            }
        }
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_TWS_CONNECT);
        break;
    case TWS_EVENT_CONNECTION_DETACH:
        if (!dma_pair_state()) {    // //APP未连接，开启广播
            if (0 == bt_get_esco_coder_busy_flag()) {
                //esco在用的时候开广播会影响质量
                dma_ble_adv_enable(1);
            }
        }
        if (!ai_mic_is_busy()) {
            dma_app_speech_stop();
        }
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_BATTERY_LEVEL_UPDATE); //主动上报电量
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_TWS_DISCONNECT);
        break;
    case TWS_EVENT_ROLE_SWITCH:
        dma_update_tws_state_to_lib(USER_NOTIFY_STATE_ROLE_SWITCH_FINISH);
        dma_all_disconnecd();
        if (role == TWS_ROLE_MASTER && (0 == bt_get_esco_coder_busy_flag())) {
            dma_ble_adv_enable(1);
        } else {
            dma_ble_adv_enable(0);
        }
        break;
    }
}

static int bt_dma_tws_status_event_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    dma_protocol_bt_tws_event_handler(evt);
    return 0;
}
APP_MSG_HANDLER(bt_dma_tws_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = bt_dma_tws_status_event_handler,
};


static int dma_app_special_message(int id, int opcode, u8 *data, u32 len)
{
    int ret = 0;
    switch (opcode) {
    case APP_PROTOCOL_DMA_SAVE_RAND:
        APP_DMA_LOG("APP_PROTOCOL_DMA_SAVE_RAND");
        //APP_DMA_DUMP(data, len);
        ret = syscfg_write(VM_DMA_RAND, data, len);
        //app_protocol_tws_send_to_sibling(DMA_TWS_ALL_INFO_SYNC, data, len);
        break;
    case APP_PROTOCOL_DMA_READ_RAND:
        APP_DMA_LOG("APP_PROTOCOL_DMA_READ_RAND");
        ret = syscfg_read(VM_DMA_RAND, data, len);
        //APP_DMA_DUMP(data, len);
        break;
    case APP_PROTOCOL_DMA_SAVE_OTA_INFO:
        APP_DMA_LOG("APP_PROTOCOL_DMA_SAVE_OTA_INFO");
        APP_DMA_DUMP(data, len);
        //不支持断点续传
        //ret = syscfg_write(VM_TME_AUTH_COOKIE, data, len);
        //app_protocol_tws_send_to_sibling(DMA_TWS_OTA_INFO_SYNC, data, len);
        break;
    case APP_PROTOCOL_DMA_READ_OTA_INFO:
        APP_DMA_LOG("APP_PROTOCOL_DMA_READ_OTA_INFO");
        //ret = syscfg_read(VM_TME_AUTH_COOKIE, data, len);
        //不支持断点续传
        memset(data, 0, len);
        APP_DMA_DUMP(data, len);
        break;
    case APP_PROTOCOL_DMA_TTS_TYPE:
        printf("app_protocol_dma_tone_play %d\n", data[0]);
        //app_protocol_dma_tone_play(data[0], 1);
        break;
    case APP_PROTOCOL_SPEECH_START:
        APP_DMA_LOG("APP_PROTOCOL_SPEECH_START");
        dma_app_speech_start();
        break;
    case APP_PROTOCOL_SPEECH_STOP:
        APP_DMA_LOG("APP_PROTOCOL_SPEECH_STOP");
        dma_app_speech_stop();
        break;
    case APP_PROTOCOL_LIB_TWS_DATA_SYNC:
        //app_protocol_tws_send_to_sibling(APP_PROTOCOL_TWS_FOR_LIB, data, len);
        break;
    }
    return ret;
}

int dma_app_check_status(int status_flag)
{
#if TCFG_USER_TWS_ENABLE
    switch (status_flag) {
    case CHECK_STATUS_TWS_MASTER:
        return dma_is_tws_master_role();
    case CHECK_STATUS_TWS_SLAVE:
        return !dma_is_tws_master_role();
    case CHECK_STATUS_TWS_PAIR_STA: /*1是tws已经配对了，0是未配对*/
        if (get_bt_tws_connect_status()) {
            return true;
        } else {
            return false;
        }
    case CHECK_STATUS_TWS_SIDE:     /*0是单耳，1是左耳，2是右耳*/
        return dma_get_tws_side();
    }
#endif
    return 0;
}

bool dma_get_battery(u8 type, u8 *value)
{
    for (int i = 0; i < APP_PROTOCOL_BAT_T_MAX; i++) {
        if (type & BIT(i)) {
            /* value[i] = app_protocal_get_bat_by_type(i); */
        }
    }
    return 0;
}

int dma_app_speech_start(void)
{
    if (!dma_is_tws_master_role()) {
        return -1;
    }
    if (BT_STATUS_TAKEING_PHONE == bt_get_connect_status()) {
        APP_DMA_LOG("phone ing...\n");
        return -1;
    }
    if (ai_mic_is_busy()) {
        APP_DMA_LOG("mic activing...\n");
        return -1;
    }

    ai_mic_rec_start();

    return 0;
}

void dma_app_speech_stop(void)
{
    if (ai_mic_is_busy()) {
        APP_DMA_LOG("app_protocol_speech_stop\n");
        ai_mic_rec_close();
        // send stop_speech_cmd;
    }
}

u16 dma_app_speech_data_send(u8 *buf, u16 len)
{
    return dma_speech_data_send(buf, len);
}

int dma_protocol_init(void)
{
    printf(">>>>>>>>>>>>>>>>>>> dma_protocol_init");
    dma_message_callback_register(dma_app_special_message);
    dma_check_status_callback_register(dma_app_check_status);
    dma_get_battery_callback_register(dma_get_battery);
    dma_user_set_dev_info(&dma_device_info);
    dma_all_init();
    dueros_dma_manufacturer_info_init();

    mic_rec_pram_init(AUDIO_CODING_OPUS, 0, dma_app_speech_data_send, 4, 1024);

    dma_ble_adv_enable(1);
    return 0;
}

int dma_protocol_exit(void)
{
    dma_all_exit();
    return 0;
}

#endif

