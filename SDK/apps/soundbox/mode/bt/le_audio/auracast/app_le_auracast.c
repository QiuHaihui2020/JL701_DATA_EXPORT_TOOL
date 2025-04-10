#include "system/includes.h"
#include "app_le_auracast.h"
#include "wireless_trans.h"
#include "le/auracast_source_api.h"
#include "le/auracast_sink_api.h"
#include "app_config.h"
#include "btstack/avctp_user.h"
#include "app_tone.h"
#include "app_main.h"
#include "audio_config.h"
#include "le_audio_player.h"
#include "a2dp_player.h"
#include "vol_sync.h"
#include "spdif_player.h"
#include "fm_api.h"
#include "linein.h"
#include "spdif_file.h"
#include "spdif.h"
#include "btstack/le/auracast_delegator_api.h"
#include "btstack/le/att.h"
#include "btstack/le/ble_api.h"

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
#include "ble_rcsp_server.h"
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define LOG_TAG             "[APP_LE_AURACAST]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/**************************************************************************************************
  测试专用
**************************************************************************************************/
/****
| sampling_frequency| variant  | 采样率 | 帧间隔(us) | 包长(字节) | 码率(kbps) | 重发次数 |
| ----------------- | -------- | ------ | ---------- | -----------| -----------| -------- |
|        0          |    0     | 8000   | 7500       | 26         | 27.732     | 2        |
|        0          |    1     | 8000   | 10000      | 30         | 24         | 2        |
|        1          |    0     | 16000  | 7500       | 30         | 32         | 2        |
|        1          |    1     | 16000  | 10000      | 40         | 32         | 2        |
|        2          |    0     | 24000  | 7500       | 45         | 48         | 2        |
|        2          |    1     | 24000  | 10000      | 60         | 48         | 2        |
|        3          |    0     | 32000  | 7500       | 60         | 64         | 2        |
|        3          |    1     | 32000  | 10000      | 80         | 64         | 2        |
|        4          |    0     | 44100  | 8163       | 97         | 95.06      | 4        |
|        4          |    1     | 44100  | 10884      | 130        | 95.55      | 4        |
|        5          |    0     | 48000  | 7500       | 75         | 80         | 4        |
|        5          |    1     | 48000  | 10000      | 100        | 80         | 4        |
****/
#define AURACAST_BIS_NUM                    (1)
#define AURACAST_BIS_SAMPLING_RATE          (5)
#define AURACAST_BIS_VARIANT                (1)
#define AURACAST_BIS_ENCRYPTION_ENABLE      (0)

auracast_user_config_t user_config = {
    .config_num_bis = AURACAST_BIS_NUM,
    .config_sampling_frequency = AURACAST_BIS_SAMPLING_RATE,
    .config_variant = AURACAST_BIS_VARIANT,
    .encryption = AURACAST_BIS_ENCRYPTION_ENABLE,
    .broadcast_id = 0x123456,
    .broadcast_name = "JL_wilson_auracast",
};
auracast_advanced_config_t user_advanced_config = {
    .iso_interval = 20000,  // ISO interval(uints:us).
    .rtn = 3,
};

static u8 auracast_switch_onoff = 0;
/**************************************************************************************************
  Data Types
**************************************************************************************************/
enum {
    ///0x1000起始为了不要跟提示音的IDEX_TONE_重叠了
    TONE_INDEX_AURACAST_OPEN = 0x1000,
    TONE_INDEX_AURACAST_CLOSE,
};

struct app_auracast_info_t {
    bool init_ok;
    u16 bis_hdl;
    void *recorder;
    struct le_audio_player_hdl rx_player;
};

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

struct auracast_adv_info {
    uint16_t length;
    uint8_t flag;
    uint8_t op;
    uint8_t sn;
    //uint8_t seq;
    uint8_t data[0];
    uint8_t crc[0];
} __attribute__((packed));

struct app_auracast_t {
    u8 status;
    u8 role;
    u8 bis_num;
    u8 big_hdl;
    u16 latch_bis_hdl;
    struct app_auracast_info_t bis_hdl_info[MAX_BIS_NUMS];
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

const struct _auracast_code_list_t {
    u32 sample_rate;
    u32 SDU_interval;     // (us)
    u16 max_SDU_octets;   // (bytes / ch)
    u32 bit_rate;         // (bps / ch)
} auracast_code_list[6][2] = {
    {
        { 8000, 7500, 26, 27732 },
        { 8000, 10000, 30, 24000 },
    },
    {
        { 16000, 7500, 30, 32000 },
        { 16000, 10000, 40, 32000 },
    },
    {
        { 24000, 7500, 45, 48000 },
        { 24000, 10000, 60, 48000 },
    },
    {
        { 32000, 7500, 60, 64000 },
        { 32000, 10000, 80, 64000 },
    },
    {
        { 44100, 8163, 97, 95060 },
        { 44100, 10884, 130, 95550 },
    },
    {
        { 48000, 7500, 75, 80000 },
        { 48000, 10000, 100, 80000 },
    },
};
/**************************************************************************************************
  Local Function Prototypes
**************************************************************************************************/
static bool is_auracast_as_source();
static int auracast_source_media_open(uint8_t index);
static int auracast_source_media_close(uint8_t index);
static int auracast_source_media_reset();
static void auracast_iso_rx_callback(uint8_t *packet, uint16_t size);
static int auracast_sink_media_open(uint8_t index, uint8_t *packet, uint16_t length);
static int auracast_sink_media_close();

/**************************************************************************************************
  Local Global Variables
**************************************************************************************************/
static OS_MUTEX mutex;
static u8 app_auracast_init_flag = 0;
static u8 auracast_app_mode_exit = 0;  /*!< 音源模式退出标志 */
static u8 config_auracast_as_master = 0;   /*!< 配置广播强制做主机 */
static int cur_deal_scene = -1; /*< 当前系统处于的运行场景 */
static struct app_auracast_t app_auracast;
static struct le_audio_mode_ops *le_audio_switch_ops = NULL; /*!< 广播音频和本地音频切换回调接口指针 */
static uint8_t match_auracast_num = 3;
uint8_t match_aurcast_name[4][28] = {
    [0] = "JBL Clip 5",
    [1] = "LE-H_54B7E5C85311",
    [2] = "MoerDuo_BLE",
    [3] = "JL_auracast",
};
static unsigned char errpacket[2] = {
    0x02, 0x00
};

#define TCFG_AURACAST_SINK_CONNECT_BY_APP 0

#if TCFG_AURACAST_SINK_CONNECT_BY_APP
static u8 add_source_state = 0;
static u8 add_source_mac[6];
static u8 no_past_broadcast_num = 0;
static u8 encry_lock = 0;
static u8 ccc[100];
static bass_no_past_source_t no_past_broadcast_sink_notify;
static const struct conn_update_param_t con_param = {
    .interval_min = 86,
    .interval_max = 86,
    .latency = 2,
    .timeout = 500,
};
#endif

u8 lea_cfg_support_ll_hci_cmd_in_lea_lib = 1;


#define AURACAST_SINK_MAX_RECORD_NUM  3
#define AURACAST_SINK_RECORDED_WIRTE_VM     0
#define AURACAST_SINK_FILTER_TIMEOUT  10*1000L

static u8 auracast_sink_start_record = 0;
static u8 auracast_sink_curr_connect_mac_addr[6];
static u8 auracast_sink_last_connect_mac_addr[6];
static u8 auracast_sink_record_connect_mac_addr[AURACAST_SINK_MAX_RECORD_NUM][6];
static u8 auarcast_sink_mac_addr_filter = 0;
static u32 auracast_sink_connect_timeout = 0;

int le_auracast_state = 0;
static u16 auracast_scan_time = 0;
extern void set_ext_scan_priority(u8 set_pr);
enum {
    BROADCAST_STATUS_SCAN_START = 1,

    BROADCAST_STATUS_SCAN_STOP,
    BROADCAST_STATUS_START,

    BROADCAST_STATUS_STOP,
};

static int auracast_sink_record_mac_addr(u8 *mac_addr);
static int auracast_sink_get_mac_addr_is_recorded(u8 *mac_addr);
static int auracast_sink_get_recorded_addr_num(void);
static void auracast_sink_connect_filter_timeout(void *priv);
/* --------------------------------------------------------------------------*/
/**
 * @brief 广播资源初始化
 */
/* ----------------------------------------------------------------------------*/
void app_auracast_init(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (app_auracast_init_flag) {
        return;
    }

    int os_ret = os_mutex_create(&mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
        ASSERT(0);
    }

    app_auracast_init_flag = 1;
}
/* --------------------------------------------------------------------------*/
/**
 * @brief 广播资源初始化
 */
/* ----------------------------------------------------------------------------*/
void app_auracast_uninit(void)
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!app_auracast_init_flag) {
        return;
    }

    app_auracast_init_flag = 0;
    int os_ret = os_mutex_del(&mutex, OS_DEL_NO_PEND);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
    }

}
/* --------------------------------------------------------------------------*/
/**
 * @brief 申请互斥量，用于保护临界区代码，与app_broadcast_mutex_post成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_auracast_mutex_pend(OS_MUTEX *mutex, u32 line)
{
    if (!app_auracast_init_flag) {
        log_error("%s err, mutex uninit", __FUNCTION__);
        return;
    }

    int os_ret;
    os_ret = os_mutex_pend(mutex, 0);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 释放互斥量，用于保护临界区代码，与app_broadcast_mutex_pend成对使用
 *
 * @param mutex:已创建的互斥量指针变量
 */
/* ----------------------------------------------------------------------------*/
static inline void app_auracast_mutex_post(OS_MUTEX *mutex, u32 line)
{
    if (!app_auracast_init_flag) {
        log_error("%s err, mutex uninit", __FUNCTION__);
        return;
    }

    int os_ret;
    os_ret = os_mutex_post(mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}
void auracast_scan_switch_priority(void *_sw)
{
    int sw = (int)_sw;
    auracast_scan_time = 0;
    u8 bt_addr[6];
    int timeout = 150;
    u8 a2dp_play = 0;
    if (a2dp_player_get_btaddr(bt_addr)) {
        a2dp_play = 1;
    }
    /* r_printf("auracast_scan_switch=%d,%d\n",sw,timeout ); */
    //edr classic acl priority 30-11=19
    if (sw) {
        /* putchar('S'); */
        set_ext_scan_priority(12);//30-12=18
    } else {
        /* putchar('s'); */
        set_ext_scan_priority(8);//30-8=22
        timeout = a2dp_play ? 300 : 400;
    }
    sw = !sw;
    auracast_scan_time = sys_timeout_add((void *)sw, auracast_scan_switch_priority, timeout);
}



/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

u8 get_auracast_role(void)
{
    return app_auracast.role;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief BIG开关提示音结束回调接口
 *
 * @param priv:传递的参数
 * @param event:提示音回调事件
 *
 * @return
 */
/* ----------------------------------------------------------------------------*/
static int auracast_tone_play_end_callback(void *priv, enum stream_event event)
{
    u32 index = (u32)priv;

    g_printf("%s, event:0x%x", __FUNCTION__, event);

    switch (event) {
    case STREAM_EVENT_NONE:
    case STREAM_EVENT_STOP:
        switch (index) {
        case TONE_INDEX_AURACAST_OPEN:
            g_printf("TONE_INDEX_AURACAST_OPEN");
            break;
        case TONE_INDEX_AURACAST_CLOSE:
            g_printf("TONE_INDEX_AURACAST_CLOSE");
            break;
        default:
            break;
        }
    default:
        break;
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取接收方连接状态
 *
 * @return 接收方连接状态
 */
/* ----------------------------------------------------------------------------*/
u8 get_auracast_status(void)
{
    return app_auracast.status;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前是否在退出模式的状态
 *
 * @return 1；是，0：否
 */
/* ----------------------------------------------------------------------------*/
u8 get_auracast_app_mode_exit_flag(void)
{
    return auracast_app_mode_exit;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 判断当前设备作为广播发送设备还是广播接收设备
 *
 * @return true:发送设备，false:接收设备
 */
/* ----------------------------------------------------------------------------*/
static bool is_auracast_as_source()
{
#if (LEA_BIG_FIX_ROLE == 1)
    return true;
#elif (LEA_BIG_FIX_ROLE == 2)
    return false;
#endif

    struct app_mode *cur_mode = app_get_current_mode();

    //当前处于蓝牙模式并且已连接手机设备时，
    //(1)播歌作为广播发送设备；
    //(2)暂停作为广播接收设备。
    if ((cur_mode->name == APP_MODE_BT) &&
        (bt_get_connect_status() != BT_STATUS_WAITINT_CONN)) {
        if ((bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING) ||
            get_a2dp_decoder_status() ||
            a2dp_player_runing()) {
            return true;
        } else {
            return false;
        }
    }

#if TCFG_APP_LINEIN_EN
    if (cur_mode->name == APP_MODE_LINEIN)  {
        if (linein_get_status() || config_auracast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_IIS_EN
    if (cur_mode->name == APP_MODE_IIS)  {
        if (iis_get_status() || config_auracast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_MIC_EN
    if (cur_mode->name == APP_MODE_MIC)  {
        if (mic_get_status() || config_auracast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_MUSIC_EN
    if (cur_mode->name == APP_MODE_MUSIC) {
        if ((music_file_get_player_status(get_music_file_player()) == FILE_PLAYER_START) || config_auracast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_FM_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_FM)  {
        if (fm_get_fm_dev_mute() == 0 || config_auracast_as_master) {
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_SPDIF_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_SPDIF) {
        //由于spdif 是先打开数据源然后再打开数据流的顺序，具有一定的滞后性，所以不能用 函数 spdif_player_runing 函数作为判断依据
        /* if (spdif_player_runing() ||  config_auracast_as_master) { */
        if (!get_spdif_mute_state()) {
            y_printf("spdif_player_runing?\n");
            return true;
        } else {
            return false;
        }
    }
#endif

#if TCFG_APP_PC_EN
    //当处于下面几种模式时，作为广播发送设备
    if (cur_mode->name == APP_MODE_PC) {
#if defined(TCFG_USB_SLAVE_AUDIO_SPK_ENABLE) && TCFG_USB_SLAVE_AUDIO_SPK_ENABLE
        return true;
#else
        return false;
#endif
    }
#endif

    return false;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 检测广播当前是否处于挂起状态
 *
 * @return true:处于挂起状态，false:处于非挂起状态
 */
/* ----------------------------------------------------------------------------*/
static bool is_need_resume_auracast()
{
    if (app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        return true;
    } else {
        return false;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播从挂起状态恢复
 */
/* ----------------------------------------------------------------------------*/
static void app_auracast_resume()
{
    if (!g_bt_hdl.init_ok) {
        return;
    }

    if (!is_need_resume_auracast()) {
        return;
    }

    if (is_auracast_as_source()) {
        app_auracast_source_open();
    } else {
        app_auracast_sink_open();
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播进入挂起状态
 */
/* ----------------------------------------------------------------------------*/
static void app_auracast_suspend()
{
    if (app_auracast.role == APP_AURACAST_AS_SOURCE) {
        app_auracast_source_close(APP_AURACAST_STATUS_SUSPEND);
    } else if (app_auracast.role == APP_AURACAST_AS_SINK) {
        app_auracast_sink_close(APP_AURACAST_STATUS_SUSPEND);
    }
}
/* --------------------------------------------------------------------------*/
/**
 * @brief 开启广播
 *
 * @return >=0:success
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_open()
{
    u8 i;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    log_info("auracast_open");

    app_auracast_mutex_pend(&mutex, __LINE__);
#if TCFG_KBOX_1T3_MODE_EN
    le_audio_ops_register(APP_MODE_NULL);
#endif

    if (is_auracast_as_source()) {
        //初始化广播发送端参数
        app_auracast_source_open();
    } else {
        //初始化广播接收端参数
        app_auracast_sink_open();
    }

    app_auracast_mutex_post(&mutex, __LINE__);
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

#if TCFG_AURACAST_SINK_CONNECT_BY_APP
static void auracast_sync_start(uint8_t *packet, uint16_t length)
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
#if 0
        for (u8 i = 0; i < NO_PAST_MAX_BASS_NUM_SOURCES; i++) {
            status = memcmp(no_past_broadcast_sink_notify.save_auracast_addr[i], config->source_mac_addr, 6);
            if (!status) {
                return;
            }
        }
#endif
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

static void auracast_big_info(uint8_t *packet, uint16_t length)
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
        //if (auracast_delegator_event_notify(DELEGATOR_ATT_CHECK_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t))) {
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
        //}
        auracast_sink_set_scan_filter(1, no_past_broadcast_num, config->source_mac_addr);
        auracast_sink_rescan();
    }
}

static void auracast_att_init(uint8_t *packet, uint16_t length)
{
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("att init\n");
    auracast_delegator_notify_t notify;
    notify.con_handle = config->con_handle;
    auracast_delegator_event_notify(DELEGATOR_ATT_PROFILE_START_NOTIFY, (void *)&notify, sizeof(auracast_delegator_notify_t));
    ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, config->con_handle, &con_param);
}

static void auracast_key_add(uint8_t *packet, uint16_t length)
{
    auracast_delegator_info_t *data = (auracast_delegator_info_t *)packet;
    ASSERT(data, "data is NULL");
    auracast_sink_set_broadcast_code(data->broadcast_code);
    put_buf(data->broadcast_code, 16);
    encry_lock = 0;
    auracast_sink_rescan();
}

static void auracast_device_add(uint8_t *packet, uint16_t length)
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

static void auracast_delegator_event_callback(uint16_t event, uint8_t *packet, uint16_t length)
{
    switch (event) {
    case DELEGATOR_SCAN_START_EVENT:
        g_printf("scan start\n");
        bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
        auracast_delegator_adv_enable(0);
        mdelay(2);
        auracast_sink_rescan();
        le_auracast_state = BROADCAST_STATUS_SCAN_START;
        /* if (auracast_scan_time == 0) { */
        /* auracast_scan_time = sys_timeout_add((void *)1, auracast_scan_switch_priority, 150); */
        /* }	 */
        break;
    case DELEGATOR_SCAN_STOP_EVENT:
        app_auracast_mutex_pend(&mutex, __LINE__);
        g_printf("scan stop\n");
        //app_auracast_sink_close(APP_AURACAST_STATUS_STOP);
        le_auracast_state = BROADCAST_STATUS_SCAN_STOP;
        auracast_sink_set_audio_state(0);
        auracast_sink_big_sync_terminate();
        auracast_sink_media_close();
        auracast_sink_stop_scan();
        no_past_broadcast_num = 0;
        for (u8 i = 0; i < NO_PAST_MAX_BASS_NUM_SOURCES; i++) {
            memset(no_past_broadcast_sink_notify.save_auracast_addr[i], 0, 6);
            auracast_sink_set_source_filter(i, no_past_broadcast_sink_notify.save_auracast_addr[i]);
        }
        auracast_sink_set_source_filter(0, 0);
        auracast_sink_set_scan_filter(0, 0, 0);
        add_source_state = 0;
        app_auracast_mutex_post(&mutex, __LINE__);
        break;
    case DELEGATOR_DEVICE_ADD_EVENT:
        app_auracast_mutex_pend(&mutex, __LINE__);
        g_printf("device add\n");
        auracast_device_add(packet, length);
        app_auracast_mutex_post(&mutex, __LINE__);
        break;
    case DELEGATOR_DEVICE_KEY_ADD_EVENT:
        g_printf("device key add\n");
        auracast_key_add(packet, length);
        break;
    case DELEGATOR_DEVICE_MODIFY_EVENT:
        app_auracast_mutex_pend(&mutex, __LINE__);
        g_printf("device modify\n");
        //app_auracast_sink_close(APP_AURACAST_STATUS_STOP);
        auracast_sink_set_audio_state(0);
        auracast_sink_big_sync_terminate();
        auracast_sink_media_close();
        auracast_sink_stop_scan();
        app_auracast_mutex_post(&mutex, __LINE__);
        break;
    default:
        break;
    }
}
#endif

static void auracast_sync_info_report(uint8_t *packet, uint16_t length)
{
    if (!app_auracast_init_flag) {
        return;
    }
    app_auracast_mutex_pend(&mutex, __LINE__);
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        app_auracast_mutex_post(&mutex, __LINE__);
        return ;
    }
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;
    ASSERT(config, "config is NULL");
    printf("sync create\n");
    printf("Advertising_SID[%d]Address_Type[%d]ADDR:\n", config->Advertising_SID, config->Address_Type);
    put_buf(config->source_mac_addr, 6);
    printf("auracast name:%s\n", config->broadcast_name);
#if 1
    //不匹配设备名，搜到直接同步，如需匹配设备名，请#if 0
    auracast_sink_big_sync_create(config);
    app_auracast_mutex_post(&mutex, __LINE__);
    return ;
#endif

#if 0
    printf("last_connect_addr:\n");
    put_buf(auracast_sink_last_connect_mac_addr, 6);

    if ((!memcmp(auracast_sink_last_connect_mac_addr, config->source_mac_addr, 6)) && auracast_sink_start_record) {
#if AURACAST_SINK_FILTER_TIMEOUT
        if (!auracast_sink_connect_timeout) {
            auracast_sink_connect_timeout = sys_timeout_add(NULL, auracast_sink_connect_filter_timeout, AURACAST_SINK_FILTER_TIMEOUT);
        }
#endif
        app_auracast_mutex_post(&mutex, __LINE__);
        auracast_sink_rescan();
        return ;
    }

    if (auarcast_sink_mac_addr_filter) {
        /* for(int i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++){ */
        /*     printf("recorded_addr[%d]:", i); */
        /*     put_buf(auracast_sink_record_connect_mac_addr[i],6); */
        /* } */
        if ((auracast_sink_get_mac_addr_is_recorded(config->source_mac_addr))) {
#if AURACAST_SINK_FILTER_TIMEOUT
            if (auracast_sink_connect_timeout) {
                sys_timeout_del(auracast_sink_connect_timeout);
                auracast_sink_connect_timeout =  0;
            }
#endif
            auracast_sink_big_sync_create(config);
        } else {
            app_auracast_mutex_post(&mutex, __LINE__);
            auracast_sink_rescan();
            return ;
        }
    } else {
#if AURACAST_SINK_FILTER_TIMEOUT
        if (auracast_sink_connect_timeout) {
            sys_timeout_del(auracast_sink_connect_timeout);
            auracast_sink_connect_timeout =  0;
        }
#endif
        auracast_sink_big_sync_create(config);
    }
    ;
    if (auracast_sink_start_record) {
        auracast_sink_record_mac_addr(config->source_mac_addr);
    }

    memcpy(auracast_sink_curr_connect_mac_addr, config->source_mac_addr, 6);
    app_auracast_mutex_post(&mutex, __LINE__);
    return;

#endif


    printf("match auracast name:%s[%d]\n", match_aurcast_name[match_auracast_num], (int)strlen((void *)match_aurcast_name[match_auracast_num]));
    if (match_name((void *)config->broadcast_name, (void *)match_aurcast_name[match_auracast_num], strlen((void *)match_aurcast_name[match_auracast_num]))) {
        printf("auracast name match\n");
        auracast_sink_big_sync_create(config);
        app_auracast_mutex_post(&mutex, __LINE__);
        return;
    } else {
        printf("auracast name no match\n");
        app_auracast_mutex_post(&mutex, __LINE__);
        auracast_sink_rescan();
        return;
    }

}

static int auracast_sink_sync_create(uint8_t *packet, uint16_t length)
{
    u8 i;
    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;

    if (!app_auracast_init_flag) {
        return -1;
    }
    app_auracast_mutex_pend(&mutex, __LINE__);
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        app_auracast_mutex_post(&mutex, __LINE__);
        return -1;
    }

    if (config->Num_BIS > SINK_MAX_BIS_MUMS) {
        app_auracast.bis_num = SINK_MAX_BIS_MUMS;
    } else {
        app_auracast.bis_num = config->Num_BIS;
    }

    app_auracast.big_hdl = config->BIG_Handle;
    app_auracast.status = APP_AURACAST_STATUS_SYNC;

    for (i = 0; i < app_auracast.bis_num; i++) {
        app_auracast.bis_hdl_info[i].bis_hdl = config->Connection_Handle[i];
        if (!app_auracast.latch_bis_hdl) {
            app_auracast.latch_bis_hdl = app_auracast.bis_hdl_info[i].bis_hdl;
        }
        auracast_sink_media_open(i, packet, length);
        app_auracast.bis_hdl_info[i].init_ok = 1;
    }

    app_auracast_mutex_post(&mutex, __LINE__);
    return 0;
}

static int auracast_sink_sync_terminate(uint8_t *packet, uint16_t length)
{
    u8 i;

    if (!app_auracast_init_flag) {
        return -1;
    }
    app_auracast_mutex_pend(&mutex, __LINE__);
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        app_auracast_mutex_post(&mutex, __LINE__);
        return -1;
    }
    for (i = 0; i < app_auracast.bis_num; i++) {
        app_auracast.bis_hdl_info[i].init_ok = 0;
    }

    auracast_sink_media_close();

    app_auracast.bis_num = 0;
    app_auracast.big_hdl = 0;
    app_auracast.latch_bis_hdl = 0;
    app_auracast.status = APP_AURACAST_STATUS_SCAN;

    auracast_sink_big_sync_terminate();
    auracast_sink_rescan();

    app_auracast_mutex_post(&mutex, __LINE__);
    return 0;
}

static void auracast_sink_event_callback(uint16_t event, uint8_t *packet, uint16_t length)
{

    if (!app_auracast_init_flag) {
        return ;
    }
    switch (event) {
    case AURACAST_SINK_BIG_SYNC_CREATE_EVENT:
        //建立同步
        g_printf("sink BIG_SYNC_CREATE");
        auracast_sink_sync_create(packet, length);
        break;
    case AURACAST_SINK_BIG_SYNC_TERMINATE_EVENT:
        //主动解除同步
        g_printf("sink BIG_SYNC_TERMINATE");
        break;
    case AURACAST_SINK_ISO_RX_CALLBACK_EVENT:
        //获取音频数据
        auracast_iso_rx_callback(packet, length);
        break;
    case AURACAST_SINK_SOURCE_INFO_REPORT_EVENT:
        //获取远端设备信息
#if TCFG_AURACAST_SINK_CONNECT_BY_APP
        app_auracast_mutex_pend(&mutex, __LINE__);
        auracast_sync_start(packet, length);
        app_auracast_mutex_post(&mutex, __LINE__);
#else
        auracast_sync_info_report(packet, length);
#endif
        break;
#if 0
    case AURACAST_SINK_DISCONNECT_EVENT:
        printf("disconnect\n");
        auracast_sink_stop_scan();
        auracast_delegator_adv_enable(1);
        break;
#endif
    case AURACAST_SINK_BIG_INFO_REPORT_EVENT:
        app_auracast_mutex_pend(&mutex, __LINE__);
        if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
            app_auracast_mutex_post(&mutex, __LINE__);
            return ;
        }
#if TCFG_AURACAST_SINK_CONNECT_BY_APP
        auracast_big_info(packet, length);
#else
        auracast_sink_big_create();
#endif
        app_auracast_mutex_post(&mutex, __LINE__);
        break;
    case AURACAST_SINK_PERIODIC_ADVERTISING_SYNC_LOST_EVENT:
        printf("periodic adv sync lost\n");
#if TCFG_AURACAST_SINK_CONNECT_BY_APP
        memset(no_past_broadcast_sink_notify.save_auracast_addr[no_past_broadcast_num], 0, 6);
#endif
        break;
    case AURACAST_SINK_BIG_SYNC_FAIL_EVENT:
    case AURACAST_SINK_BIG_SYNC_LOST_EVENT:
        app_auracast_mutex_pend(&mutex, __LINE__);
        //被动解除同步
        g_printf("sink BIG_SYNC_LOST");
        auracast_sink_sync_terminate(packet, length);
        app_auracast_mutex_post(&mutex, __LINE__);
        break;
#if TCFG_AURACAST_SINK_CONNECT_BY_APP
    case AURACAST_SINK_BLE_CONNECT_EVENT:
        g_printf("sink BLE_CONNECT_EVENT");
        auracast_att_init(packet, length);
        break;
#if 0
    case AURACAST_SINK_DISCONNECT_EVENT:
        g_printf("sink DISCONNECT_EVENT\n");
        auracast_sink_stop_scan();
        auracast_delegator_adv_enable(1);
        break;
#endif
#endif
    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 开启广播
 *
 * @return >=0:success
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_sink_open()
{
    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }

    if (app_auracast.status != APP_AURACAST_STATUS_STOP && app_auracast.status != APP_AURACAST_STATUS_SUSPEND) {
        return -EPERM;
    }

    struct app_mode *mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    auracast_switch_onoff = 1;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    ble_module_enable(0);
#endif
    app_auracast_mutex_pend(&mutex, __LINE__);
    log_info("auracast_sink_open");

    le_auracast_state = 0;
    auracast_sink_init();
    auracast_sink_event_callback_register(auracast_sink_event_callback);
#if TCFG_AURACAST_SINK_CONNECT_BY_APP
    auracast_delegator_user_config_t auracast_delegator_user_parm;
    auracast_delegator_user_parm.adv_edr = 1;
    auracast_delegator_user_parm.adv_interval = 40;
    memcpy(&auracast_delegator_user_parm.device_name, (u8 *)bt_get_local_name(), LOCAL_NAME_LEN);
    auracast_delegator_user_parm.device_name_len = strlen(auracast_delegator_user_parm.device_name);
    /* auracast_delegator_user_config_t param; */
    auracast_delegator_config(&auracast_delegator_user_parm);
    auracast_delegator_event_callback_register(auracast_delegator_event_callback);
    auracast_delegator_adv_enable(1);
#else
    auracast_sink_scan_start();
#endif

    app_auracast.role = APP_AURACAST_AS_SINK;
    app_auracast.status = APP_AURACAST_STATUS_SCAN;

    app_auracast_mutex_post(&mutex, __LINE__);
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭广播
 *
 * @param status:挂起还是停止
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_sink_close(u8 status)
{
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }
    u8 i;

    app_auracast_mutex_pend(&mutex, __LINE__);
    log_info("auracast_sink_close");

    auracast_sink_set_audio_state(0);
    if (app_auracast.status == APP_AURACAST_STATUS_SYNC) {
        auracast_sink_big_sync_terminate();
    }
    auracast_sink_stop_scan();
    os_time_dly(10);
    auracast_sink_uninit();
    auracast_sink_media_close();

    app_auracast.status = status;
    app_auracast.bis_num = 0;
    app_auracast.role = 0;
    app_auracast.big_hdl = 0;
    app_auracast.latch_bis_hdl = 0;

    for (i = 0; i < MAX_BIS_NUMS; i++) {
        memset(&app_auracast.bis_hdl_info[i], 0, sizeof(struct app_auracast_info_t));
    }

    app_auracast_mutex_post(&mutex, __LINE__);

    auracast_switch_onoff = 0;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if (status != APP_AURACAST_STATUS_SUSPEND) {
        ble_module_enable(1);
    }
#endif
    return 0;
}

static void auracast_source_app_send_callback(uint8_t *buff, uint16_t length)
{
    u8 i;
    int rlen = 0;
    u32 timestamp;
    auracast_event_send_t *send_packet = (auracast_event_send_t *)buff;
    u32 sdu_interval_us = auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].SDU_interval;
    timestamp = (auracast_source_read_iso_tx_sync(send_packet->bis_index) \
                 + auracast_source_get_sync_delay()) & 0xfffffff;
    timestamp += ((send_packet->bis_sub_event_counter - 1) * sdu_interval_us);
    if (app_auracast.bis_hdl_info[send_packet->bis_index].recorder) {
        rlen = le_audio_stream_tx_data_handler(app_auracast.bis_hdl_info[send_packet->bis_index].recorder, send_packet->buffer, length, timestamp, TCFG_LE_AUDIO_PLAY_LATENCY);
        if (!rlen) {
            putchar('^');
        }
    }
}

static void auracast_source_create(uint8_t *packet, uint16_t length)
{

    if (!app_auracast_init_flag) {
        return ;
    }
    app_auracast_mutex_pend(&mutex, __LINE__);
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        app_auracast_mutex_post(&mutex, __LINE__);
        return ;
    }
    app_auracast.role = APP_AURACAST_AS_SOURCE;
    app_auracast.bis_num = AURACAST_BIS_NUM;
    app_auracast.latch_bis_hdl = auracast_source_get_bis_hdl(0);

    for (u8 i = 0; i < app_auracast.bis_num; i++) {
        auracast_source_media_open(i);
        app_auracast.bis_hdl_info[i].init_ok = 1;
    }

    app_auracast_mutex_post(&mutex, __LINE__);
}

static void auracast_source_app_event_callback(uint16_t event, uint8_t *packet, uint16_t length)
{
    switch (event) {
    case AURACAST_SOURCE_BIG_CREATED:
        g_printf("AURACAST_SOURCE_BIG_CREATED\n");
        auracast_source_create(packet, length);
        break;
    case AURACAST_SOURCE_BIG_TERMINATED:
        g_printf("AURACAST_SOURCE_BIG_TERMINATED\n");
        break;
    case AURACAST_SOURCE_SEND_CALLBACK:
        auracast_source_app_send_callback(packet, length);
        break;
    default:
        printf("auracast source unknow event %x\n", event);
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 开启广播
 *
 * @return >=0:success
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_source_open()
{
    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }

    if (app_auracast.status != APP_AURACAST_STATUS_STOP && app_auracast.status != APP_AURACAST_STATUS_SUSPEND) {
        return -EPERM;
    }

    struct app_mode *mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    auracast_switch_onoff = 1;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    ble_module_enable(0);
#endif
    app_auracast_mutex_pend(&mutex, __LINE__);
    log_info("auracast_source_open");

    auracast_source_init();
    auracast_source_config(&user_config);
    auracast_source_advanced_config(&user_advanced_config);
    auracast_source_event_callback_register(auracast_source_app_event_callback);
    auracast_source_start();

    app_auracast.status = APP_AURACAST_STATUS_BROADCAST;

    app_auracast_mutex_post(&mutex, __LINE__);
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭广播
 *
 * @param status:挂起还是停止
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_source_close(u8 status)
{
    if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
        return -EPERM;
    }

    app_auracast_mutex_pend(&mutex, __LINE__);
    log_info("auracast_source_close");

    auracast_source_stop();
    os_time_dly(10);
    auracast_source_uninit();
    auracast_source_media_close(0xff);

    app_auracast.bis_num = 0;
    app_auracast.role = 0;
    app_auracast.big_hdl = 0;
    app_auracast.latch_bis_hdl = 0;
    app_auracast.status = status;

    for (u8 i = 0; i < MAX_BIS_NUMS; i++) {
        memset(&app_auracast.bis_hdl_info[i], 0, sizeof(struct app_auracast_info_t));
    }

    app_auracast_mutex_post(&mutex, __LINE__);

    auracast_switch_onoff = 0;
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
    if (status != APP_AURACAST_STATUS_SUSPEND) {
        ble_module_enable(1);
    }
#endif
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播开关切换
 *
 * @return 0：操作成功
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_switch(void)
{
    u8 i;
    u8 find = 0;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    if (!tone_player_runing()) {
        if (app_auracast.status == APP_AURACAST_STATUS_STOP || app_auracast.status == APP_AURACAST_STATUS_SUSPEND) {
            bt_work_mode_select(BT_MODE_AURACAST);
            play_tone_file_alone_callback(get_tone_files()->le_broadcast_open,
                                          (void *)TONE_INDEX_AURACAST_OPEN,
                                          auracast_tone_play_end_callback);
        } else {
            bt_work_mode_select(g_bt_hdl.last_work_mode);
            play_tone_file_alone_callback(get_tone_files()->le_broadcast_close,
                                          (void *)TONE_INDEX_AURACAST_CLOSE,
                                          auracast_tone_play_end_callback);
        }
    }

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 更新系统当前处于的场景
 *
 * @param scene:当前系统状态
 *
 * @return 0:success
 */
/* ----------------------------------------------------------------------------*/
int update_app_auracast_deal_scene(int scene)
{
    cur_deal_scene = scene;
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 广播开启情况下，不同场景的处理流程
 *
 * @param scene:当前系统状态
 *
 * @return ret < 0:无需处理，ret == 0:处理事件但不拦截后续流程，ret > 0:处理事件并拦截后续流程
 */
/* ----------------------------------------------------------------------------*/
int app_auracast_deal(int scene)
{
    u32 rets_addr;
    __asm__ volatile("%0 = rets ;" : "=r"(rets_addr));

    if (g_bt_hdl.work_mode != BT_MODE_AURACAST) {
        return -EPERM;
    }
    u8 i;
    int ret = 0;
    static u8 phone_start_cnt = 0;
    struct app_mode *mode;

    if (!g_bt_hdl.init_ok) {
        return -EPERM;
    }

    if ((cur_deal_scene == scene) &&
        (scene != LE_AUDIO_PHONE_START) &&
        (scene != LE_AUDIO_PHONE_STOP)) {
        log_error("app_auracast_deal,scene not be modified:%d", scene);
        return -EPERM;
    }

    g_printf("app_auracast_deal, rets_addr:0x%x", rets_addr);

    cur_deal_scene = scene;

    switch (scene) {
    case LE_AUDIO_APP_MODE_ENTER:
        log_info("LE_AUDIO_APP_MODE_ENTER");
        //进入当前模式
        auracast_app_mode_exit = 0;
    case LE_AUDIO_APP_OPEN:
        config_auracast_as_master = 1;
        mode = app_get_current_mode();
        if (mode) {
            le_audio_ops_register(mode->name);
        }
        if (is_need_resume_auracast()) {
            if (mode->name == APP_MODE_BT) {
                //处于其他模式时，手机后台播歌使设备跳回蓝牙模式，此时获取蓝牙底层a2dp状态为正在播放，
                //但BT_STATUS_A2DP_MEDIA_START事件还没到来，无法获取设备信息，导致直接开关tx_le_audio_open使用了空设备地址引起死机
                app_auracast_sink_open();
            } else {
                app_auracast_resume();
            }
            ret = 1;
        }
        config_auracast_as_master = 0;
        break;

    case LE_AUDIO_APP_MODE_EXIT:
        log_info("auracast_app_mode_exit");
        //退出当前模式
        auracast_app_mode_exit = 1;
    case LE_AUDIO_APP_CLOSE:
        phone_start_cnt = 0;
        app_auracast_suspend();
        le_audio_ops_unregister();
        break;

    case LE_AUDIO_MUSIC_START:
    case LE_AUDIO_A2DP_START:
        log_info("LE_AUDIO_MUSIC_START");
        //启动a2dp播放
        if (auracast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现auracast_app_mode_exit，再出现LE_AUDIO_A2DP_START，导致广播状态发生改变
            break;
        }
#if (LEA_BIG_FIX_ROLE == 0)
        if (app_auracast.status != APP_AURACAST_STATUS_STOP && app_auracast.status != APP_AURACAST_STATUS_SUSPEND) {
            //(1)当处于广播开启并且作为接收设备时，挂起广播，播放当前手机音乐；
            //(2)当前广播处于挂起状态时，恢复广播并作为发送设备。
            if (app_auracast.role == APP_AURACAST_AS_SINK) {
                app_auracast_suspend();
            } else if (app_auracast.role == APP_AURACAST_AS_SOURCE) {
                ret = 1;
            }
        }
#else
        if (app_auracast.role == APP_AURACAST_AS_SOURCE) {
            //固定收发角色重启广播数据流
            auracast_source_media_reset();
            ret = 1;
            break;
        }
#endif

        if (is_need_resume_auracast()) {
            /* app_auracast_resume(); */
            app_auracast_source_open();
            ret = 1;
        }
        break;

    case LE_AUDIO_MUSIC_STOP:
    case LE_AUDIO_A2DP_STOP:
        log_info("LE_AUDIO_MUSIC_STOP");
        //停止a2dp播放
        if (auracast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现auracast_app_mode_exit，再出现LE_AUDIO_A2DP_STOP，导致广播状态发生改变
            break;
        }
#if (LEA_BIG_FIX_ROLE == 0)
        //当前处于广播挂起状态时，停止手机播放，恢复广播并接收其他设备的音频数据
        app_auracast_suspend();
#else
        if (app_auracast.role == APP_AURACAST_AS_SOURCE) {
            //固定收发角色暂停播放时关闭广播数据流
            auracast_source_media_close(0xff);
            ret = 1;
            break;
        }
#endif
        if (is_need_resume_auracast()) {
            /* app_auracast_resume(); */
            app_auracast_sink_open();
            ret = 1;
        }
        break;

    case LE_AUDIO_PHONE_START:
        log_info("LE_AUDIO_PHONE_START");
        //通话时，挂起广播
        phone_start_cnt++;
        printf("===phone_start_cnt:%d===\n", phone_start_cnt);
        app_auracast_suspend();
        break;

    case LE_AUDIO_PHONE_STOP:
        log_info("LE_AUDIO_PHONE_STOP");
        //通话结束恢复广播
        phone_start_cnt--;
        printf("===phone_start_cnt:%d===\n", phone_start_cnt);
        if (phone_start_cnt) {
            log_info("phone_start_cnt:%d", phone_start_cnt);
            break;
        }
        //当前处于蓝牙模式并且挂起前广播，恢复广播并作为接收设备
        if (is_need_resume_auracast()) {
            /* app_auracast_resume(); */
            app_auracast_sink_open();
        }
        break;

    case LE_AUDIO_EDR_DISCONN:
        log_info("LE_AUDIO_EDR_DISCONN");
        if (auracast_app_mode_exit) {
            //防止蓝牙非后台情况下退出蓝牙模式时，会先出现auracast_app_mode_exit，再出现LE_AUDIO_EDR_DISCONN，导致广播状态发生改变
            break;
        }
        //当经典蓝牙断开后，作为发送端的广播设备挂起广播
        if (app_auracast.role == APP_AURACAST_AS_SOURCE) {
            app_auracast_suspend();
        }
        if (is_need_resume_auracast()) {
            /* app_auracast_resume(); */
            app_auracast_sink_open();
        }
        break;

    default:
        log_error("%s invalid operation\n", __FUNCTION__);
        ret = -ESRCH;
        break;
    }

    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 非蓝牙后台情况下，在其他音源模式开启BIG，前提要先开蓝牙协议栈
 */
/* ----------------------------------------------------------------------------*/
void app_auracast_open_in_other_mode()
{
    app_auracast_init();
    if (is_need_resume_auracast()) {
        struct app_mode *mode = app_get_current_mode();
        if (mode) {
            le_audio_ops_register(mode->name);
        }
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 非蓝牙后台情况下，在其他音源模式关闭BIG
 */
/* ----------------------------------------------------------------------------*/
void app_auracast_close_in_other_mode()
{
    app_auracast_suspend();
}

static int auracast_source_media_open(uint8_t index)
{
    g_printf("auracast_source_media_open");

    le_audio_switch_ops = get_broadcast_audio_sw_ops();
    //关闭本地音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->local_audio_close) {
        le_audio_switch_ops->local_audio_close();
    }

    u16 frame_dms;
    if (auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].SDU_interval >= 10000) {
        frame_dms = 100;
    } else {
        frame_dms = 75;
    }

    struct le_audio_stream_params params = {0};
    params.fmt.nch = 1;
    params.fmt.coding_type = AUDIO_CODING_LC3;
    params.fmt.frame_dms = frame_dms;
    params.fmt.bit_rate = auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].bit_rate;
    params.fmt.sdu_period = auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].SDU_interval;
    params.fmt.isoIntervalUs = auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].SDU_interval;
    params.fmt.sample_rate = auracast_code_list[user_config.config_sampling_frequency][user_config.config_variant].sample_rate;
    params.fmt.dec_ch_mode = LEA_TX_DEC_OUTPUT_CHANNEL;
    params.latency = TCFG_LE_AUDIO_PLAY_LATENCY;
    params.conn = app_auracast.latch_bis_hdl;

    //打开广播音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
        g_printf("auracast_source_tx_le_audio_open");
        app_auracast.bis_hdl_info[index].recorder = le_audio_switch_ops->tx_le_audio_open(&params);
    }
    return 0;
}

static int auracast_source_media_close(uint8_t index)
{
    u8 i;
    void *recorder = 0;
    u8 player_status = 0;

    //获取当前播放器状态
    if (le_audio_switch_ops && le_audio_switch_ops->play_status) {
        player_status = le_audio_switch_ops->play_status();
    }

    for (i = 0; i < app_auracast.bis_num; i++) {
        if (0xff != index && i != index) {
            continue;
        }

        if (app_auracast.bis_hdl_info[i].recorder) {
            recorder = app_auracast.bis_hdl_info[i].recorder;
            app_auracast.bis_hdl_info[i].recorder = NULL;
        }

        if (recorder) {
            if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                le_audio_switch_ops->tx_le_audio_close(recorder);
                recorder = NULL;
                g_printf("auracast_source_media_close");
            }
        }
    }

    //当前处于播放状态，关闭le audio音频流后恢复本地播放
    if (player_status == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        if (le_audio_switch_ops && le_audio_switch_ops->local_audio_open) {
            le_audio_switch_ops->local_audio_open();
        }
    }

    return 0;
}

int auracast_source_media_reset()
{
    auracast_source_media_close(0xff);
    auracast_source_media_open(0);
    return 0;
}

static int auracast_sink_media_open(uint8_t index, uint8_t *packet, uint16_t length)
{
    g_printf("auracast_sink_media_open");

    auracast_sink_source_info_t *config = (auracast_sink_source_info_t *)packet;

    le_audio_switch_ops = get_broadcast_audio_sw_ops();
    //关闭本地音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->local_audio_close) {
        le_audio_switch_ops->local_audio_close();
    }

    struct le_audio_stream_params params = {0};
    params.fmt.nch = 1;
    params.fmt.coding_type = AUDIO_CODING_LC3;
    params.fmt.dec_ch_mode = LEA_RX_DEC_OUTPUT_CHANNEL;

    g_printf("nch:%d, coding_type:0x%x, dec_ch_mode:%d",
             params.fmt.nch, params.fmt.coding_type, params.fmt.dec_ch_mode);

    if (config->frame_duration == FRAME_DURATION_7_5) {
        params.fmt.frame_dms = 75;
    } else if (config->frame_duration == FRAME_DURATION_10) {
        params.fmt.frame_dms = 100;
    } else {
        ASSERT(0, "frame_dms err:%d", config->frame_duration);
    }
    params.fmt.sdu_period = config->sdu_period;
    params.fmt.isoIntervalUs = config->sdu_period;
    params.fmt.sample_rate = config->sample_rate;
    params.fmt.bit_rate = config->bit_rate;
    params.conn = app_auracast.latch_bis_hdl;

    g_printf("frame_dms:%d, sdu_period:%d, sample_rate:%d, bit_rate:%d,latch_bis_hdl:%d",
             params.fmt.frame_dms, config->sdu_period, config->sample_rate, config->bit_rate, params.conn);

    //打开广播音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
        g_printf("auracast_sink_rx_le_audio_open");
        le_audio_switch_ops->rx_le_audio_open(&app_auracast.bis_hdl_info[index].rx_player, &params);
    }

    return 0;
}

static int auracast_sink_media_close()
{
    u8 i;
    u8 player_status = 0;
    struct le_audio_player_hdl player;
    player.le_audio = 0;
    player.rx_stream = 0;

    //获取当前播放器状态
    if (le_audio_switch_ops && le_audio_switch_ops->play_status) {
        player_status = le_audio_switch_ops->play_status();
    }

    for (i = 0; i < app_auracast.bis_num; i++) {
        if (app_auracast.bis_hdl_info[i].rx_player.le_audio) {
            player.le_audio = app_auracast.bis_hdl_info[i].rx_player.le_audio;
            app_auracast.bis_hdl_info[i].rx_player.le_audio = NULL;
        }

        if (app_auracast.bis_hdl_info[i].rx_player.rx_stream) {
            player.rx_stream = app_auracast.bis_hdl_info[i].rx_player.rx_stream;
            app_auracast.bis_hdl_info[i].rx_player.rx_stream = NULL;
        }

        if (player.le_audio && player.rx_stream) {
            if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_close) {
                le_audio_switch_ops->rx_le_audio_close(&player);
                player.le_audio = 0;
                player.rx_stream = 0;
                g_printf("auracast_sink_media_close");
            }
        }
    }

    //当前处于播放状态，关闭le audio音频流后恢复本地播放
    if (player_status == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        if (le_audio_switch_ops && le_audio_switch_ops->local_audio_open) {
            le_audio_switch_ops->local_audio_open();
        }
    }

    return 0;
}

static void auracast_iso_rx_callback(uint8_t *packet, uint16_t size)
{
    //putchar('o');
    bool plc_flag = 0;
    hci_iso_hdr_t hdr = {0};
    ll_iso_unpack_hdr(packet, &hdr);
    if ((hdr.pb_flag == 0b10) && (hdr.iso_sdu_length == 0)) {
        if (hdr.packet_status_flag == 0b00) {
            /* log_error("SDU empty"); */
            putchar('m');
            return;
            plc_flag = 1;
        } else {
            /* log_error("SDU lost"); */
            putchar('s');
            return;
            plc_flag = 1;
        }
    }
    if (((hdr.pb_flag == 0b10) || (hdr.pb_flag == 0b00)) && (hdr.packet_status_flag == 0b01)) {
        //log_error("SDU invalid, len=%d", hdr.iso_sdu_length);
        putchar('p');
        return;
        plc_flag = 1;
    }
    for (u8 i = 0; i < app_auracast.bis_num; i++) {
        //printf("[%d][%d][%x]\n",app_auracast.bis_hdl_info[i].bis_hdl,hdr.handle,(u32)app_auracast.bis_hdl_info[i].rx_player.rx_stream);
        if (app_auracast.bis_hdl_info[i].bis_hdl == hdr.handle && app_auracast.bis_hdl_info[i].rx_player.rx_stream) {
            if (plc_flag) {
                le_audio_stream_rx_frame(app_auracast.bis_hdl_info[i].rx_player.rx_stream, (void *)errpacket, 2, hdr.time_stamp + TCFG_LE_AUDIO_PLAY_LATENCY);
            } else {
                le_audio_stream_rx_frame(app_auracast.bis_hdl_info[i].rx_player.rx_stream, (void *)hdr.iso_sdu, hdr.iso_sdu_length, hdr.time_stamp + TCFG_LE_AUDIO_PLAY_LATENCY);
            }
        }
    }
}


int app_auracast_sink_start_record(void)
{

    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }


    if (app_auracast.role != APP_AURACAST_AS_SINK) {
        return -EPERM;
    }

    struct app_mode *mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    auracast_sink_start_record = 1;


#if AURACAST_SINK_RECORDED_WIRTE_VM
    for (int i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++) {
        syscfg_read(VM_WIRELESS_RECORDED_ADDR0 + i, auracast_sink_record_connect_mac_addr[i], 6);
    }
#endif

    u8 temp[6] = {0};
    if (memcmp(auracast_sink_curr_connect_mac_addr, temp, 6)) {
        auracast_sink_record_mac_addr(auracast_sink_curr_connect_mac_addr);
    }

    return 0;
}

int app_auracast_sink_stop_record(void)
{
    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }


    if (app_auracast.role != APP_AURACAST_AS_SINK) {
        return -EPERM;
    }

    struct app_mode *mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    app_auracast_mutex_pend(&mutex, __LINE__);
    if (auracast_sink_start_record) {
        auracast_sink_start_record = 0;
    }

    if (auarcast_sink_mac_addr_filter) {
        auarcast_sink_mac_addr_filter = 0;
    }

    memset(auracast_sink_last_connect_mac_addr, 0, 6);

    app_auracast_mutex_post(&mutex, __LINE__);

    return 0;

}


static int auracast_sink_get_mac_addr_is_recorded(u8 *mac_addr)
{

    int ret = 0;
    app_auracast_mutex_pend(&mutex, __LINE__);
    for (int i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++) {
        if (!(memcmp(auracast_sink_record_connect_mac_addr[i], mac_addr, 6))) {
            ret = 1;
        }
    }

    app_auracast_mutex_post(&mutex, __LINE__);
    return ret;

}

static int auracast_sink_get_recorded_addr_num(void)
{

    int num = 0;
    u8 temp[6] = {0};

    app_auracast_mutex_pend(&mutex, __LINE__);

    for (int i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++) {
        if ((memcmp(auracast_sink_record_connect_mac_addr[i], temp, 6))) {
            num++;
        }
    }
    app_auracast_mutex_post(&mutex, __LINE__);

    return num;
}


int app_auracast_sink_switch_source_device(u8 switch_mode) //0:切换设备后不过滤设备；1：切换设备后过滤处理只连接记录的设备
{
    if (!g_bt_hdl.init_ok || app_var.goto_poweroff_flag) {
        return -EPERM;
    }

    if (!app_auracast_init_flag) {
        return -EPERM;
    }

    if (app_auracast.role != APP_AURACAST_AS_SINK) {
        return -EPERM;
    }

    struct app_mode *mode = app_get_current_mode();
    if (mode && (mode->name == APP_MODE_BT) &&
        (bt_get_call_status() != BT_CALL_HANGUP)) {
        return -EPERM;
    }

    u8 temp[6] = {0};
    u8 recorded_num = 0;

    if (!auracast_sink_start_record) {
        app_auracast_sink_start_record();
    }

    recorded_num = auracast_sink_get_recorded_addr_num();
    if (!recorded_num) {
        return -EPERM;
    }

    app_auracast_mutex_pend(&mutex, __LINE__);
    if (recorded_num >= 2 && switch_mode) {
        auarcast_sink_mac_addr_filter = switch_mode;
    } else if (switch_mode) {
        printf("[error]The current number of records is insufficient");
        auarcast_sink_mac_addr_filter = 0;
    } else {
        auarcast_sink_mac_addr_filter = 0;
    }

    memcpy(auracast_sink_last_connect_mac_addr, auracast_sink_curr_connect_mac_addr, 6);

    app_auracast_mutex_post(&mutex, __LINE__);

    app_auracast_sink_close(APP_AURACAST_STATUS_STOP);
    app_auracast_sink_open();

    return 0;
}
u8 check_local_not_accept_sniff_by_remote()
{
    /* printf("le_auracast_state=%d\n",le_auracast_state ); */
    if (le_audio_player_is_playing() || le_auracast_state == BROADCAST_STATUS_SCAN_START) {
        return TRUE;
    }

    return FALSE;
}


static int auracast_sink_record_mac_addr(u8 *mac_addr)
{

    int ret = 0;
    u8 temp[6] = {0};
    static u8 discard_cnt = 0;
    int i = 0;
    if (!mac_addr) {
        return -EPERM;
    }

    if (!memcmp(mac_addr, temp, 6)) {
        return -EPERM;
    }

    app_auracast_mutex_pend(&mutex, __LINE__);

    for (i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++) {
        if (!memcmp(mac_addr, auracast_sink_record_connect_mac_addr[i], 6)) {
            ret = 1;
            break;
        }
        if ((memcmp(auracast_sink_record_connect_mac_addr[i], temp, 6))) {
            memcpy(auracast_sink_record_connect_mac_addr[i], mac_addr, 6);
            ret = 2;
            break;
        }

    }

    if (i == AURACAST_SINK_MAX_RECORD_NUM) {
        printf("reached maxixmum number of records, discard [%d] addr:", discard_cnt);
        put_buf(auracast_sink_record_connect_mac_addr[discard_cnt], 6);
        memcpy(auracast_sink_record_connect_mac_addr[discard_cnt], mac_addr, 6);
        i =  discard_cnt;
        discard_cnt++;
        discard_cnt = (discard_cnt >= 3) ? 0 : discard_cnt;
    }
#if AURACAST_SINK_RECORDED_WIRTE_VM
    syscfg_write(VM_WIRELESS_RECORDED_ADDR0 + i, auracast_sink_record_connect_mac_addr[i], 6);
#endif

    app_auracast_mutex_post(&mutex, __LINE__);

    return ret;
}

void app_auracast_sink_discard_record(void)
{

    app_auracast_mutex_pend(&mutex, __LINE__);

    for (int i = 0; i < AURACAST_SINK_MAX_RECORD_NUM; i++) {
        memset(auracast_sink_record_connect_mac_addr[i], 0, 6);
#if AURACAST_SINK_RECORDED_WIRTE_VM
        syscfg_write(VM_WIRELESS_RECORDED_ADDR0 + i, auracast_sink_record_connect_mac_addr[i], 6);
#endif
    }

    auarcast_sink_mac_addr_filter = 0;

    memset(auracast_sink_last_connect_mac_addr, 0, 6);

    app_auracast_mutex_post(&mutex, __LINE__);

}


static void auracast_sink_connect_filter_timeout(void *priv)
{
    memset(auracast_sink_last_connect_mac_addr, 0, 6);

    auarcast_sink_mac_addr_filter = 0;
}


u8 get_auracast_switch_onoff(void)
{
    return auracast_switch_onoff;
}

#endif

