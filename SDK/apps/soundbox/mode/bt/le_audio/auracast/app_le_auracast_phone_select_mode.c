#include "system/includes.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "app_tone.h"
#include "app_main.h"
#include "linein.h"
#include "wireless_trans.h"
#include "audio_config.h"
#include "a2dp_player.h"
#include "vol_sync.h"
#include "ble_rcsp_server.h"
#include "spdif_file.h"
#include "btstack/le/auracast_sink_api.h"
#include "btstack/le/auracast_delegator_api.h"
#include "le_audio_player.h"
#include "btstack/le/att.h"
#include "btstack/le/ble_api.h"


#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG             "[APP_AURACAST_SINK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define APP_MSG_KEY_AURACAST 4
#define MAX_AURACAST_NUM 3

#define LEA_DEC_OUTPUT_CHANNEL 0x25 //接收端解码输出
static struct broadcast_hdl *broadcast_hdl = NULL;
struct auracast_audio  g_rx_audio;
static u8 add_source_state = 0;
static u8 add_source_mac[6];


enum {
    BROADCAST_STATUS_STOP,
    BROADCAST_STATUS_START,
};

struct broadcast_rx_audio_hdl {
    void *le_audio;
    void *rx_stream;
};

typedef struct {
    u16 bis_hdl;
    void *recorder;
    struct broadcast_rx_audio_hdl rx_player;
} bis_hdl_info_t ;

struct auracast_audio {
    uint8_t bis_num;
    bis_hdl_info_t audio_hdl[MAX_NUM_BIS];
};

struct broadcast_hdl {
    struct list_head entry;
    u8 del;
    u8 big_hdl;
    u8 status;
    bis_hdl_info_t bis_hdl_info[MAX_NUM_BIS];
    u32 big_sync_delay;
    const char *role_name;
};

struct broadcast_featrue_notify {
    uint8_t feature;
    uint8_t metadata_len;
    uint8_t metadata[20];
} __attribute__((packed));

struct broadcast_base_info_notify {
    uint8_t address_type;
    uint8_t address[6];
    uint8_t adv_sid;
    uint16_t pa_interval;
} __attribute__((packed));

struct broadcast_codec_info {
    uint8_t nch;
    u32 coding_type;
    s16 frame_len;
    s16 sdu_period;
    int sample_rate;
    int bit_rate;
} __attribute__((packed));

struct broadcast_source_endpoint_notify {
    uint8_t prd_delay[3];
    uint8_t num_subgroups;
    uint8_t num_bis;
    uint8_t codec_id[5];
    uint8_t codec_spec_length;
    uint8_t codec_spec_data[0];
    uint8_t metadata_length;
    uint8_t bis_data[0];
} __attribute__((packed));

typedef struct {
    uint8_t  save_auracast_addr[NO_PAST_MAX_BASS_NUM_SOURCES][6];
    uint8_t encryp_addr[NO_PAST_MAX_BASS_NUM_SOURCES][6];
    uint8_t  broadcast_name[28];
    uint32_t  broadcast_id;
    uint8_t enc;
    struct  broadcast_featrue_notify fea_data;
    struct broadcast_base_info_notify base_data;
    struct broadcast_codec_info codec_data;
} bass_no_past_source_t;

struct auracast_adv_info {
    uint16_t length;
    uint8_t flag;
    uint8_t op;
    uint8_t sn;
    //uint8_t seq;
    uint8_t data[0];
    uint8_t crc[0];
} __attribute__((packed));

static const struct conn_update_param_t con_param = {
    .interval_min = 166,
    .interval_max = 166,
    .latency = 2,
    .timeout = 500,
};

bass_no_past_source_t no_past_broadcast_sink_notify;
static u8 no_past_broadcast_num = 0;
static u8 encry_lock = 0;

void auracast_sink_event_callback(uint16_t event, uint8_t *packet, uint16_t length);
void auracast_delegator_event_callback(uint16_t event, uint8_t *packet, uint16_t length);

static u32 broadcast_audio_reference_time(void *priv, u8 cmd, void *arg)
{
    return 0;
}

static bool match_name(char *target_name, char *source_name, size_t target_len)
{
    if (strlen(target_name) == 0 || strlen(source_name) == 0) {
        return FALSE;
    }

    if (0 == memcmp(target_name, source_name, target_len)) {
        return TRUE;
    }

    return FALSE;
}

static void le_auracast_sync_start(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("sync create\n");

    put_buf(add_source_mac, 6);
    put_buf(config->source_mac_addr, 6);
    u8 status;
    if (add_source_state == DELEGATOR_SYNCHRONIZED_TO_PA && !encry_lock) {
        status = memcmp(config->source_mac_addr, add_source_mac, 6);
        if (!status) {
            auracast_sink_big_sync_create(config);
        }
    } else {
        for (u8 i = 0; i < NO_PAST_MAX_BASS_NUM_SOURCES; i++) {
            status = memcmp(no_past_broadcast_sink_notify.save_auracast_addr[i], config->source_mac_addr, 6);
            if (!status) {
                return;
            }
        }
        no_past_broadcast_num += 1;
        if (no_past_broadcast_num >= NO_PAST_MAX_BASS_NUM_SOURCES) {
            no_past_broadcast_num = 0;
        }
        memset(no_past_broadcast_sink_notify.broadcast_name, 0, sizeof(no_past_broadcast_sink_notify.broadcast_name));
        memcpy(no_past_broadcast_sink_notify.broadcast_name, config->broadcast_name, strlen((void *)config->broadcast_name));
        no_past_broadcast_sink_notify.broadcast_id = config->broadcast_id;
        no_past_broadcast_sink_notify.base_data.address_type = config->Address_Type;
        memcpy(no_past_broadcast_sink_notify.base_data.address, config->source_mac_addr, 6);
        memcpy(no_past_broadcast_sink_notify.save_auracast_addr[no_past_broadcast_num], config->source_mac_addr, 6);
        no_past_broadcast_sink_notify.base_data.adv_sid = config->Advertising_SID;
        no_past_broadcast_sink_notify.fea_data.feature = config->feature;
        printf("Advertising_SID[%d]Address_Type[%d]ADDR:\n", config->Advertising_SID, config->Address_Type);
        put_buf(config->source_mac_addr, 6);
        printf("auracast name:%s\n", config->broadcast_name);
        auracast_sink_big_sync_create(config);
    }
}

static u8 make_auracast_ltv_data(u8 *buf, u8 data_type, u8 *data, u8 data_len)
{
    buf[0] = data_len + 1;
    buf[1] = data_type;
    memcpy(buf + 2, data, data_len);
    return data_len + 2;
}

static void le_auracast_big_info(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    if (add_source_state == DELEGATOR_SYNCHRONIZED_TO_PA) {
        auracast_sink_big_create();
    } else {

        no_past_broadcast_sink_notify.enc = config->enc;
        printf("%s\n", no_past_broadcast_sink_notify.broadcast_name);
        printf("%d\n", no_past_broadcast_sink_notify.base_data.address_type);
        put_buf(no_past_broadcast_sink_notify.base_data.address, 6);
        printf("%d\n", no_past_broadcast_sink_notify.base_data.adv_sid);
        printf("%d\n", no_past_broadcast_sink_notify.broadcast_id);
        printf("%d\n", no_past_broadcast_sink_notify.enc);
        auracast_delegator_notify_t notify;
        notify.att_send_len = 100;
        if (auracast_delegator_event_notify(DELEGATOR_ATT_CHECK_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t))) {
            u8 build_notify_data[200];
            u8 offset = 0;
            struct auracast_adv_info *info = (struct auracast_adv_info *)build_notify_data;
            info->flag = 1;
            info->op = 3;
            info->sn = 2;
            offset += 5;

            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x1, no_past_broadcast_sink_notify.broadcast_name, strlen((void *)no_past_broadcast_sink_notify.broadcast_name));
            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x2, (u8 *)&no_past_broadcast_sink_notify.broadcast_id, 3);
            struct broadcast_featrue_notify data;
            if (no_past_broadcast_sink_notify.enc) {
                data.feature = 0x7;
                memcpy(no_past_broadcast_sink_notify.encryp_addr[no_past_broadcast_num], no_past_broadcast_sink_notify.base_data.address, 6);
            } else {
                data.feature = 0x6;
            }
            data.metadata_len = 0;
            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x3, (u8 *)&data, 2);
#if 1
            u8 ccc[100];
            struct broadcast_source_endpoint_notify *codec_data = (struct broadcast_source_endpoint_notify *)ccc;
            u8 codec_offset = 0;
            codec_data->prd_delay[0] = 0;
            codec_data->prd_delay[1] = 0;
            codec_data->prd_delay[2] = 0;
            codec_data->num_subgroups = 1;
            codec_data->num_bis = 1;
            u8 codec_id[5] = {0x6, 0x0, 0x0, 0x0, 0x0};
            memcpy(codec_data->codec_id, codec_id, 5);

            codec_offset += 11;
            u8 save_offset = codec_offset;
            u8 frequency = 0x8;
            codec_offset += make_auracast_ltv_data(&ccc[codec_offset], 0x1, &frequency, 1);
            u8 frame_duration = 0x1;
            codec_offset += make_auracast_ltv_data(&ccc[codec_offset], 0x2, &frame_duration, 1);
            u16 octets_frame = 0x0064;
            codec_offset += make_auracast_ltv_data(&ccc[codec_offset], 0x4, (u8 *)&octets_frame, 2);

            codec_data->codec_spec_length = codec_offset - save_offset;

            u8 meta_len = 0;
            ccc[codec_offset] = meta_len;
            codec_offset += 1;
            u8 bis_index = 1;
            ccc[codec_offset] = bis_index;
            codec_offset += 1;
            u8 bis_codec_len = 6;
            ccc[codec_offset] = bis_codec_len;
            codec_offset += 1;

            u8 bis_codec[4] = {0x1, 0x0, 0x0, 0x0};
            codec_offset += make_auracast_ltv_data(&ccc[codec_offset], 0x3, bis_codec, sizeof(bis_codec));

            put_buf(ccc, codec_offset);

            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x4, ccc, codec_offset);
#else
            biginfo_notify_data[biginfo_size - 1] = 0;
            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x4, biginfo_notify_data, biginfo_size);

            if (biginfo_notify_data) {
                free(biginfo_notify_data);
            }
#endif
            no_past_broadcast_sink_notify.base_data.pa_interval = 0xffff;
            offset += make_auracast_ltv_data(&build_notify_data[offset], 0x5, (u8 *)&no_past_broadcast_sink_notify.base_data, sizeof(struct broadcast_base_info_notify));
            info->length = offset;
            u16 crc = CRC16(build_notify_data, offset);
            memcpy(&build_notify_data[offset], &crc, 2);
            offset += 2;
            put_buf(build_notify_data, offset);

            for (u8 i = 0; i < 2; i++) {
                auracast_delegator_notify_t notify;
                notify.big_len = offset;
                notify.big_data = build_notify_data;
                auracast_delegator_event_notify(DELEGATOR_ATT_SEND_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t));
                mdelay(100);
            }
        }
        auracast_sink_set_scan_filter(1, no_past_broadcast_num, config->source_mac_addr);
        auracast_sink_rescan();
    }
}

static void le_auracast_att_init(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("att init\n");
    auracast_delegator_notify_t notify;
    notify.con_handle = config->con_handle;
    auracast_delegator_event_notify(DELEGATOR_ATT_PROFILE_START_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t));
    ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, config->con_handle, &con_param);
}

static void le_auracast_audio_open(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("audio open\n");
#if 0
    //audio open
    if (config->Num_BIS > MAX_NUM_BIS) {
        g_rx_audio.bis_num = MAX_NUM_BIS;
    } else {
        g_rx_audio.bis_num = config->Num_BIS;
    }
    for (u8 i = 0; i < g_rx_audio.bis_num; i++) {
        g_rx_audio.audio_hdl[i].bis_hdl = config->Connection_Handle[i];
    }
    struct le_audio_stream_params params;
    params.fmt.nch = LE_AUDIO_CODEC_CHANNEL;
    params.fmt.coding_type = LE_AUDIO_CODEC_TYPE;
    params.fmt.frame_dms = LE_AUDIO_CODEC_FRAME_LEN;
    params.fmt.sdu_period = config->sdu_period;
    params.fmt.isoIntervalUs = config->sdu_period;
    params.fmt.sample_rate = config->sample_rate;
    params.fmt.dec_ch_mode = LEA_DEC_OUTPUT_CHANNEL;
    params.fmt.bit_rate = config->bit_rate;
    params.reference_time = broadcast_audio_reference_time;
    broadcast_hdl = (struct broadcast_hdl *)malloc(sizeof(struct broadcast_hdl));
    ASSERT(broadcast_hdl, "broadcast_hdl is NULL");
    broadcast_hdl->role_name = "big_rx";
    broadcast_hdl->status = BROADCAST_STATUS_START;
    broadcast_hdl->big_hdl = config->BIG_Handle;
    params.conn = broadcast_hdl;
    int err;

    for (u8 i = 0; i < g_rx_audio.bis_num; i++) {
        broadcast_hdl->bis_hdl_info[i].bis_hdl = g_rx_audio.audio_hdl[i].bis_hdl;
        g_rx_audio.audio_hdl[i].rx_player.le_audio = le_audio_stream_create(params.conn, &params.fmt, params.reference_time);
        g_rx_audio.audio_hdl[i].rx_player.rx_stream = le_audio_stream_rx_open(g_rx_audio.audio_hdl[i].rx_player.le_audio, params.fmt.coding_type);
        ll_config_ctrler_clk(g_rx_audio.audio_hdl[i].bis_hdl, 0);
        err = le_audio_player_open(g_rx_audio.audio_hdl[i].rx_player.le_audio, &params);
        if (err != 0) {
            ASSERT(0, "player open fail");
        }
    }
#endif

}

static void le_auracast_source_info_report(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("sample_rate[%d]sdu_period[%d]bit_rate[%d]\n", config->sample_rate, config->sdu_period, config->bit_rate);
}

static void le_auracast_sync_terminate(void)
{
    printf("sync terminate\n");
    auracast_sink_big_sync_terminate();
}

static void le_auracast_iso_rx_callback(uint8_t *packet, uint16_t size)
{
    hci_iso_hdr_t hdr = {0};
    ll_iso_unpack_hdr(packet, &hdr);
    if ((hdr.pb_flag == 0b10) && (hdr.iso_sdu_length == 0)) {
        if (hdr.packet_status_flag == 0b00) {
            putchar('m');
            return;
            /* log_error("SDU empty"); */
        } else {
            putchar('s');
            return;
            /* log_error("SDU lost"); */
        }
    }
    if (((hdr.pb_flag == 0b10) || (hdr.pb_flag == 0b00)) && (hdr.packet_status_flag == 0b01)) {
        putchar('p');
        return;
        //log_error("SDU invalid, len=%d", hdr.iso_sdu_length);
    }
#if 0
    for (u8 i = 0; i < g_rx_audio.bis_num; i++) {
        //printf("[%d][%d]",hdr.handle,g_rx_audio.audio_hdl[i].bis_hdl);
        //if (g_rx_audio.audio_hdl[i].bis_hdl == hdr.handle) {
        le_audio_stream_rx_frame(g_rx_audio.audio_hdl[i].rx_player.rx_stream, (void *)hdr.iso_sdu, hdr.iso_sdu_length, hdr.time_stamp);
        //}
    }
#endif

}

static void le_auracast_audio_close(void)
{
#if 0
    printf("audio close\n");
    for (u8 i = 0; i < g_rx_audio.bis_num; i++) {
        if (g_rx_audio.audio_hdl[i].rx_player.le_audio && g_rx_audio.audio_hdl[i].rx_player.rx_stream) {
            le_audio_player_close(g_rx_audio.audio_hdl[i].rx_player.le_audio);
            le_audio_stream_rx_close(g_rx_audio.audio_hdl[i].rx_player.rx_stream);
            le_audio_stream_free(g_rx_audio.audio_hdl[i].rx_player.le_audio);
            g_rx_audio.audio_hdl[i].rx_player.le_audio = NULL;
            g_rx_audio.audio_hdl[i].rx_player.rx_stream = NULL;
        }
    }
    if (broadcast_hdl) {
        free(broadcast_hdl);
        broadcast_hdl = NULL;
    }
#endif

}

static void le_auracast_stop(void)
{
    auracast_sink_set_audio_state(0);
    le_auracast_sync_terminate();
    le_auracast_audio_close();
    auracast_sink_scan_stop();
    auracast_sink_uninit();
}

static void le_auracast_sync_lost(uint8_t *packet, uint16_t length)
{
    le_auracast_sync_terminate();
    le_auracast_audio_close();
    auracast_sink_rescan();
}

static void le_auracast_key_add(uint8_t *packet, uint16_t length)
{
    auracast_delegator_info_t *data = (auracast_delegator_info_t *)packet;
    ASSERT(data, "data is NULL");
    auracast_sink_set_broadcast_code(data->broadcast_code);
    put_buf(data->broadcast_code, 16);
    encry_lock = 0;
    auracast_sink_rescan();
}

static void le_auracast_device_add(uint8_t *packet, uint16_t length)
{
    auracast_delegator_info_t *data = (auracast_delegator_info_t *)packet;
    ASSERT(data, "data is NULL");
    u8 encry;
    for (u8 i = 0; i < NO_PAST_MAX_BASS_NUM_SOURCES; i++) {
        encry = memcmp(no_past_broadcast_sink_notify.encryp_addr[i], data->source_addr, 6);
        if (!encry) {
            break;
        }
    }
    if (!encry) {
        printf("auracast is encryption!!\n");
        encry_lock = 1;
    } else {
        printf("auracast is not encry!!\n");
        encry_lock = 0;
        auracast_sink_rescan();
    }

    auracast_delegator_notify_t notify;
    notify.encry = encry;
    notify.bass_source_id = data->bass_source_id;
    auracast_delegator_event_notify(DELEGATOR_BASS_ADD_SOURCE_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t));

    add_source_state = DELEGATOR_SYNCHRONIZED_TO_PA;
    memcpy(add_source_mac, data->source_addr, 6);
    auracast_sink_set_source_filter(1, data->source_addr);
    auracast_sink_set_scan_filter(0, 0, 0);
}

#if 0
static int le_auracast_app_msg_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_STATUS_INIT_OK:
        log_info("APP_MSG_STATUS_INIT_OK");
        printf("--earphone auracast mode\n");
        auracast_sink_init();
        auracast_sink_event_callback_register(auracast_sink_event_callback);
        auracast_delegator_init();
        auracast_delegator_event_callback_register(auracast_delegator_event_callback);
        auracast_delegator_adv_enable(1);
        break;
    case APP_MSG_ENTER_MODE://1
        log_info("APP_MSG_ENTER_MODE");
        break;
    case APP_MSG_BT_GET_CONNECT_ADDR://1
        log_info("APP_MSG_BT_GET_CONNECT_ADDR");
        break;
    case APP_MSG_BT_OPEN_PAGE_SCAN://1
        log_info("APP_MSG_BT_OPEN_PAGE_SCAN");
        break;
    case APP_MSG_BT_CLOSE_PAGE_SCAN://1
        log_info("APP_MSG_BT_CLOSE_PAGE_SCAN");
        break;
    case APP_MSG_BT_ENTER_SNIFF:
        break;
    case APP_MSG_BT_EXIT_SNIFF:
        break;
    case APP_MSG_TWS_PAIRED://1
        log_info("APP_MSG_TWS_PAIRED");
        break;
    case APP_MSG_TWS_UNPAIRED://1
        log_info("APP_MSG_TWS_UNPAIRED");
        break;
    case APP_MSG_TWS_PAIR_SUSS://1
        log_info("APP_MSG_TWS_PAIR_SUSS");
    case APP_MSG_TWS_CONNECTED://1
        log_info("APP_MSG_TWS_CONNECTED");
        break;
    case APP_MSG_POWER_OFF://1
        log_info("APP_MSG_POWER_OFF");
        break;
    case APP_MSG_LE_AUDIO_MODE:
        r_printf("APP_MSG_LE_AUDIO_MODE=%d\n", msg[1]);
        break;
    case APP_MSG_KEY|APP_MSG_KEY_AURACAST:
        printf("auracast_switch\n");
        le_auracast_stop();
        break;
    default:
        break;
    }
    return 0;
}
#endif

void auracast_sink_event_callback(uint16_t event, uint8_t *packet, uint16_t length)
{
    switch (event) {
    case AURACAST_SINK_SOURCE_INFO_REPORT_EVENT:
        le_auracast_sync_start(packet, length);
        break;
    case AURACAST_SINK_BLE_CONNECT_EVENT:
        le_auracast_att_init(packet, length);
        break;
    case AURACAST_SINK_BIG_SYNC_CREATE_EVENT:
        le_auracast_audio_open(packet, length);
        break;
    case AURACAST_SINK_BIG_SYNC_TERMINATE_EVENT:
        printf("big terminate\n");
        break;
    case AURACAST_SINK_BIG_SYNC_LOST_EVENT:
        printf("big lost\n");
        le_auracast_sync_lost(packet, length);
        break;
    case AURACAST_SINK_BIG_INFO_REPORT_EVENT:
        printf("big info report\n");
        le_auracast_big_info(packet, length);
        break;
    case AURACAST_SINK_ISO_RX_CALLBACK_EVENT:
        le_auracast_iso_rx_callback(packet, length);
        break;
    default:
        break;
    }
}

void auracast_delegator_event_callback(uint16_t event, uint8_t *packet, uint16_t length)
{
    switch (event) {
    case DELEGATOR_SCAN_START_EVENT:
        printf("scan start\n");
        auracast_sink_rescan();
        break;
    case DELEGATOR_SCAN_STOP_EVENT:
        printf("scan stop\n");
        le_auracast_stop();
        no_past_broadcast_num = 0;
        for (u8 i = 0; i < NO_PAST_MAX_BASS_NUM_SOURCES; i++) {
            memset(no_past_broadcast_sink_notify.save_auracast_addr[i], 0, 6);
            auracast_sink_set_source_filter(i, no_past_broadcast_sink_notify.save_auracast_addr[i]);
        }
        auracast_sink_set_source_filter(0, 0);
        auracast_sink_set_scan_filter(0, 0, 0);
        add_source_state = 0;
        break;
    case DELEGATOR_DEVICE_ADD_EVENT:
        printf("device add\n");
        le_auracast_device_add(packet, length);
        break;
    case DELEGATOR_DEVICE_KEY_ADD_EVENT:
        printf("device key add\n");
        le_auracast_key_add(packet, length);
        break;
    case DELEGATOR_DEVICE_MODIFY_EVENT:
        printf("device modify\n");
        auracast_sink_set_audio_state(0);
        le_auracast_sync_terminate();
        le_auracast_audio_close();
        auracast_sink_scan_stop();
        break;
    default:
        break;
    }
}

void auracast_phone_select_mode(void)
{
    printf("--auracast phone select mode\n");
    auracast_sink_init();
    auracast_sink_event_callback_register(auracast_sink_event_callback);
    auracast_delegator_init();
    auracast_delegator_event_callback_register(auracast_delegator_event_callback);
    auracast_delegator_adv_enable(1);
}

#if 0
APP_MSG_HANDLER(le_auracast_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = le_auracast_app_msg_handler,
};
#endif
#endif
