/*********************************************************************************************
 *   Filename        : connected.c

 *   Description     :

 *   Author          : Weixin Liang

 *   Email           : liangweixin@zh-jieli.com

 *   Last modifiled  : 2022-12-15 14:26

 *   Copyright:(c)JIELI  2011-2022  @ , All Rights Reserved.
 *********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "btstack/avctp_user.h"
#include "app_msg.h"
#include "cig.h"
#include "le_connected.h"
#include "wireless_trans_manager.h"
#include "wireless_trans.h"
#include "clock_manager/clock_manager.h"
#include "le_audio_stream.h"
#include "bt_event_func.h"
#include "le_audio_player.h"

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)

/**************************************************************************************************
  Macros
 **************************************************************************************************/
#define LOG_TAG             "[LE_CONNECTED]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


/*! \brief CIS丢包修复 */
#define CIS_AUDIO_PLC_ENABLE    1

#define CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME   	 0  //将cis 的时间戳转化为本地时间，同步节点网络需要选择local

#define connected_get_cis_tick_time(cis_txsync)  wireless_trans_get_last_tx_clk((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP ? "cig_perip" : "cig_central", (void *)(cis_txsync))

/**************************************************************************************************
  Data Types
 **************************************************************************************************/

/*! \brief 广播状态枚举 */
enum {
    CONNECTED_STATUS_STOP,      /*!< 广播停止 */
    CONNECTED_STATUS_STOPPING,  /*!< 广播正在停止 */
    CONNECTED_STATUS_OPEN,      /*!< 广播开启 */
    CONNECTED_STATUS_START,     /*!< 广播启动 */
};

typedef struct {
    bool init_ok;
    bool cis_discon;
    u16 cis_hdl;
    u16 acl_hdl;
    void *recorder;
    struct le_audio_player_hdl rx_player;
} cis_hdl_info_t;

/*! \brief 时间戳类型转换结构体 */
#if CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME
struct cis_ts_convert_hdl {
    u32 usec_offset;
    u32 tick_usec;
    u32 prev_ts;
    u32 prev_local_ts;
    u8 latch_time_state;
};
#endif

/*! \brief 广播结构体 */
struct connected_hdl {
    struct list_head entry; /*!< cig链表项，用于多cig管理 */
    u8 del;
    u8 cig_hdl;
    u16 latch_cis_hdl;
    cis_hdl_info_t cis_hdl_info[CIS_MAX_CONNECTABLE_NUMS];
    u32 cig_sync_delay;
    const char *role_name;
#if CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME
    struct cis_ts_convert_hdl ts_convert;
#endif
};

/**************************************************************************************************
  Static Prototypes
 **************************************************************************************************/
static int connected_dec_data_receive_handler(void *_hdl, void *data, int len);
static void connected_central_event_callback(const CIG_EVENT event, void *priv);
static void connected_iso_callback(const void *const buf, size_t length, void *priv);
static void connected_perip_event_callback(const CIG_EVENT event, void *priv);

/**************************************************************************************************
  Global Variables
 **************************************************************************************************/
static DEFINE_SPINLOCK(connected_lock);
static OS_MUTEX connected_mutex;
static u8 connected_role;   /*!< 记录当前广播为接收端还是发送端 */
static u8 connected_init_flag;  /*!< 广播初始化标志 */
static u8 g_cig_hdl;        /*!< 用于cig_hdl获取 */
static u8 connected_num;    /*!< 记录当前开启了多少个cig广播 */
static u8 bredr_close_flag = 0;
static u8 *transmit_buf;    /*!< 用于发送端发数 */
static struct list_head connected_list_head = LIST_HEAD_INIT(connected_list_head);
static struct le_audio_mode_ops *le_audio_switch_ops = NULL; /*!< 广播音频和本地音频切换回调接口指针 */
u8 cig_peripheral_support_lea_profile  = 1;
const cig_callback_t cig_central_cb = {
    .receive_packet_cb      = connected_iso_callback,
    .event_cb               = connected_central_event_callback,
};
const cig_callback_t cig_perip_cb = {
    .receive_packet_cb      = connected_iso_callback,
    .event_cb               = connected_perip_event_callback,
};

#if CIS_AUDIO_PLC_ENABLE
//丢包修复补包 解码读到 这两个byte 才做丢包处理
static unsigned char errpacket[10] = {
    0x02, 0x00
};
#endif /*CIS_AUDIO_PLC_ENABLE*/

/**************************************************************************************************
  Function Declarations
 **************************************************************************************************/
static inline void connected_mutex_pend(OS_MUTEX *mutex, u32 line)
{
    int os_ret;
    os_ret = os_mutex_pend(mutex, 0);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}

static inline void connected_mutex_post(OS_MUTEX *mutex, u32 line)
{
    int os_ret;
    os_ret = os_mutex_post(mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s err, os_ret:0x%x", __FUNCTION__, os_ret);
        ASSERT(os_ret != OS_ERR_PEND_ISR, "line:%d err, os_ret:0x%x", line, os_ret);
    }
}

static int cig_event_to_user(int event, void *value, u32 len)
{
    int *evt = zalloc(sizeof(int) + len);
    ASSERT(evt);
    evt[0] = event;
    memcpy(&evt[1], value, len);
    app_send_message_from(MSG_FROM_CIG, sizeof(int) + len, (int *)evt);
    free(evt);
    return 0;
}

static u16 get_available_cig_hdl(u8 id, struct list_head *head)
{
    struct connected_hdl *p;
    u8 hdl = id;
    if ((hdl == 0) || (hdl > 0xEF)) {
        hdl = 1;
        g_cig_hdl = 1;
    }

    connected_mutex_pend(&connected_mutex, __LINE__);
__again:
    list_for_each_entry(p, head, entry) {
        if (hdl == p->cig_hdl) {
            hdl++;
            goto __again;
        }
    }

    if (hdl > 0xEF) {
        hdl = 0;
    }

    if (hdl == 0) {
        hdl++;
        goto __again;
    }

    g_cig_hdl = hdl;
    connected_mutex_post(&connected_mutex, __LINE__);
    return hdl;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 初始化CIG所需的参数及流程
 *
 * @param role:CIG角色作为主机还是从机
 */
/* ----------------------------------------------------------------------------*/
void connected_init(u8 role)
{
    log_info("--func=%s", __FUNCTION__);
    int ret;

    if (connected_init_flag) {
        return;
    }

    int os_ret = os_mutex_create(&connected_mutex);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
        ASSERT(0);
    }

    connected_init_flag = 1;

    if (role == CONNECTED_ROLE_CENTRAL) {
        //初始化cis发送参数及注册回调
        ret = wireless_trans_init("cig_central", NULL);
        if (ret != 0) {
            log_error("wireless_trans_init fail:0x%x\n", ret);
            connected_init_flag = 0;
        }
    }

    if (role == CONNECTED_ROLE_PERIP) {
        //初始化cis接收参数及注册回调
        ret = wireless_trans_init("cig_perip", NULL);
        if (ret != 0) {
            log_error("wireless_trans_init fail:0x%x\n", ret);
            connected_init_flag = 0;
        }
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 复位CIG所需的参数及流程
 *
 * @param role:CIG角色作为主机还是从机
 */
/* ----------------------------------------------------------------------------*/
void connected_uninit(u8 role)
{
    log_info("--func=%s", __FUNCTION__);
    int ret;

    if (!connected_init_flag) {
        return;
    }

    int os_ret = os_mutex_del(&connected_mutex, OS_DEL_NO_PEND);
    if (os_ret != OS_NO_ERR) {
        log_error("%s %d err, os_ret:0x%x", __FUNCTION__, __LINE__, os_ret);
    }

    connected_init_flag = 0;

    if (role & CONNECTED_ROLE_CENTRAL) {
        ret = wireless_trans_uninit("cig_central", NULL);
        if (ret != 0) {
            log_error("wireless_trans_uninit fail:0x%x\n", ret);
        }
    }

    if (role & CONNECTED_ROLE_PERIP) {
        ret = wireless_trans_uninit("cig_perip", NULL);
        if (ret != 0) {
            log_error("wireless_trans_uninit fail:0x%x\n", ret);
        }
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG主机连接成功处理事件
 *
 * @param priv:连接成功附带的句柄参数
 *
 * @return 是否执行成功 -- 0为成功，其他为失败
 */
/* ----------------------------------------------------------------------------*/
int connected_central_connect_deal(void *priv)
{
    u8 i;
    u8 index = 0;
    u8 find = 0;
    struct connected_hdl *connected_hdl = 0;
    cig_hdl_t *hdl = (cig_hdl_t *)priv;
    struct le_audio_stream_params params;

    log_info("connected_central_connect_deal");
    log_info("hdl->cig_hdl:%d, hdl->cis_hdl:%d", hdl->cig_hdl, hdl->cis_hdl);

    //真正连上设备后，清除BIT(7)，使外部跑转发流程
    connected_role &= ~BIT(7);

    connected_mutex_pend(&connected_mutex, __LINE__);
    list_for_each_entry(connected_hdl, &connected_list_head, entry) {
        if (connected_hdl->cig_hdl == hdl->cig_hdl) {
            find = 1;
            break;
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);

    if (!find) {
        connected_hdl = (struct connected_hdl *)zalloc(sizeof(struct connected_hdl));
        ASSERT(connected_hdl, "connected_hdl is NULL");
    }

    for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
        if (!connected_hdl->cis_hdl_info[i].cis_hdl) {
            index = i;
            break;
        }
    }

    le_audio_switch_ops = get_connected_audio_sw_ops();

    //关闭本地音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->local_audio_close) {
        le_audio_switch_ops->local_audio_close();
    }

    params.fmt.nch = get_cig_audio_coding_nch();
    params.fmt.bit_rate = get_cig_audio_coding_bit_rate();
    params.fmt.coding_type = LE_AUDIO_CODEC_TYPE;
    params.fmt.frame_dms = get_cig_audio_coding_frame_duration();
    params.fmt.sdu_period = get_cig_sdu_period_us();
    params.fmt.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
    params.fmt.dec_ch_mode = LEA_TX_DEC_OUTPUT_CHANNEL;
    params.conn = connected_hdl->latch_cis_hdl;

    connected_hdl->role_name = "cig_central";
    connected_hdl->cig_hdl = hdl->cig_hdl;
    connected_hdl->cis_hdl_info[index].acl_hdl = hdl->acl_hdl;
    connected_hdl->cis_hdl_info[index].cis_hdl = hdl->cis_hdl;
    if (!connected_hdl->latch_cis_hdl) {
        connected_hdl->latch_cis_hdl = hdl->cis_hdl;
    }

#if (LEA_CIG_TRANS_MODE == 2)

    //TODO:打开播放器播放远端传来的音频
    if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
        le_audio_switch_ops->rx_le_audio_open(&connected_hdl->cis_hdl_info[index].rx_player, &params);
    }

    if (!connected_hdl->cis_hdl_info[index].recorder) {
        //TODO:打开recorder
        if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
            connected_hdl->cis_hdl_info[index].recorder = le_audio_switch_ops->tx_le_audio_open(&params);
        }
    }

    if (!connected_hdl->cig_sync_delay) {
        connected_hdl->cig_sync_delay = hdl->cig_sync_delay;
    }

#elif (LEA_CIG_TRANS_MODE == 1)

#if (LEA_CIG_CONNECT_MODE == 2)

    //两发一收central做接收，使用接收解码声道配置
    params.fmt.dec_ch_mode = LEA_RX_DEC_OUTPUT_CHANNEL;

    //TODO:打开播放器播放远端音频
    if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
        le_audio_switch_ops->rx_le_audio_open(&connected_hdl->cis_hdl_info[index].rx_player, &params);
    }
#else
    if (!connected_hdl->cis_hdl_info[index].recorder) {
        //TODO:打开recorder
        if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
            connected_hdl->cis_hdl_info[index].recorder = le_audio_switch_ops->tx_le_audio_open(&params);
        }
    }

    if (!connected_hdl->cig_sync_delay) {
        connected_hdl->cig_sync_delay = hdl->cig_sync_delay;
    }
#endif

#endif

    connected_hdl->cis_hdl_info[index].init_ok = 1;

    connected_mutex_pend(&connected_mutex, __LINE__);
    if (!find) {
        spin_lock(&connected_lock);
        list_add_tail(&connected_hdl->entry, &connected_list_head);
        spin_unlock(&connected_lock);
    }
    connected_mutex_post(&connected_mutex, __LINE__);

    return 0;
}

static void connected_central_rebuild_cis_latch(struct connected_hdl *connected_hdl)
{
    int i;

    for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
        if (!connected_hdl->cis_hdl_info[i].cis_hdl) {
            continue;
        }
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG主机连接断开成功处理接口口
 *
 * @param priv:断链成功附带的句柄参数
 *
 * @return 是否执行成功 -- 0为成功，其他为失败
 */
/* ----------------------------------------------------------------------------*/
int connected_central_disconnect_deal(void *priv)
{
    u8 i, index;
    u8 cis_connected_num = 0;
    int player_status = 0;
    struct connected_hdl *p, *connected_hdl;
    cig_hdl_t *hdl = (cig_hdl_t *)priv;
    void *recorder = 0;
    struct le_audio_player_hdl player;

    player.le_audio = 0;
    player.rx_stream = 0;

    log_info("connected_central_disconnect_deal");
    log_info("%s, cig_hdl:%d", __FUNCTION__, hdl->cig_hdl);

    //TODO:获取播放状态
    if (le_audio_switch_ops && le_audio_switch_ops->play_status) {
        player_status = le_audio_switch_ops->play_status();
    }

    connected_mutex_pend(&connected_mutex, __LINE__);
    spin_lock(&connected_lock);
    list_for_each_entry(p, &connected_list_head, entry) {
        if (p->cig_hdl == hdl->cig_hdl) {

            for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
                if (p->cis_hdl_info[i].init_ok && (p->cis_hdl_info[i].cis_hdl == hdl->cis_hdl)) {
                    p->cis_hdl_info[i].init_ok = 0;
                    p->cis_hdl_info[i].cis_hdl = 0;
                    if (p->cis_hdl_info[i].recorder) {
                        recorder = p->cis_hdl_info[i].recorder;
                        p->cis_hdl_info[i].recorder = NULL;
                    }

                    if (p->cis_hdl_info[i].rx_player.le_audio) {
                        player.le_audio = p->cis_hdl_info[i].rx_player.le_audio;
                        p->cis_hdl_info[i].rx_player.le_audio = NULL;
                    }

                    if (p->cis_hdl_info[i].rx_player.rx_stream) {
                        player.rx_stream = p->cis_hdl_info[i].rx_player.rx_stream;
                        p->cis_hdl_info[i].rx_player.rx_stream = NULL;
                    }
                    index = i;
                } else if (p->cis_hdl_info[i].cis_hdl) {
                    cis_connected_num++;
                }
            }

            for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
                if (p->cis_hdl_info[i].cis_hdl) {
                    p->latch_cis_hdl = p->cis_hdl_info[i].cis_hdl;
                    break;
                }
            }

            spin_unlock(&connected_lock);

            if (recorder) {
                //TODO:关闭recorder
                if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                    le_audio_switch_ops->tx_le_audio_close(recorder);
                }
            }

            if (player.le_audio && player.rx_stream) {
                //TODO:关闭player
                if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_close) {
                    le_audio_switch_ops->rx_le_audio_close(&player);
                }
            }

            spin_lock(&connected_lock);

            memset(&p->cis_hdl_info[index], 0, sizeof(cis_hdl_info_t));

            connected_hdl = p;
            break;
        }
    }
    spin_unlock(&connected_lock);
    connected_mutex_post(&connected_mutex, __LINE__);

    if (!cis_connected_num) {

        connected_role |= BIT(7);   //断开连接后，或上BIT(7)，防止外部流程判断错误

        connected_hdl->cig_sync_delay = 0;

        connected_hdl->latch_cis_hdl = 0;

#if CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME
        memset(&connected_hdl->ts_convert, 0, sizeof(struct cis_ts_convert_hdl));
#endif
        if (player_status == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
            //TODO:打开本地音频
            if (le_audio_switch_ops && le_audio_switch_ops->local_audio_open) {
                le_audio_switch_ops->local_audio_open();
            }
        }
    }

    return 0;
}

static void channel_separation(void *data, void *lch_buf, void *rch_buf, u32 len, u8 packet_num)
{
    u16 single_channel_data_len = (get_cig_enc_output_frame_len() - 2) / 2;
    u8 *read_ptr = (u8 *)data;
    u8 *lch_write_ptr = (u8 *)lch_buf;
    u8 *rch_write_ptr = (u8 *)rch_buf;
    u16 cpy_data_len = 0;
    for (u8 i = 0; i < packet_num; i++) {
        //拷贝数据长度信息
        memcpy(lch_write_ptr, &single_channel_data_len, sizeof(single_channel_data_len));
        lch_write_ptr += sizeof(single_channel_data_len);
        memcpy(rch_write_ptr, &single_channel_data_len, sizeof(single_channel_data_len));
        rch_write_ptr += sizeof(single_channel_data_len);
        cpy_data_len += sizeof(single_channel_data_len);

        //拷贝实际数据
        read_ptr += sizeof(single_channel_data_len);
        memcpy(lch_write_ptr, read_ptr, single_channel_data_len);
        read_ptr += single_channel_data_len;
        lch_write_ptr += single_channel_data_len;
        memcpy(rch_write_ptr, read_ptr, single_channel_data_len);
        read_ptr += single_channel_data_len;
        rch_write_ptr += single_channel_data_len;
        cpy_data_len += single_channel_data_len;
    }
    if (cpy_data_len != len) {
        putchar('-');
        /* printf("%d  %d  %d",cpy_data_len,len,single_channel_data_len); */
    }
}

static int connected_tx_align_data_handler(u8 cig_hdl)
{
    struct connected_hdl *connected_hdl = 0;
    cis_hdl_info_t *cis_hdl_info;
    u32 timestamp;
    cis_txsync_t txsync;
    int rlen = 0, i;
    int err;
    u8 capture_send_update = 0;
    u8 packet_num;
    u16 single_ch_trans_data_len;
    void *L_buffer, *R_buffer;
    void *last_recorder;
    cig_stream_param_t param = {0};

#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
    packet_num = get_cig_transmit_data_len() / get_cig_enc_output_frame_len();
    single_ch_trans_data_len = (get_cig_transmit_data_len() - packet_num * 2) / 2 + packet_num * 2;
    L_buffer = malloc(single_ch_trans_data_len);
    ASSERT(L_buffer);
    R_buffer = malloc(single_ch_trans_data_len);
    ASSERT(R_buffer);
#endif

    spin_lock(&connected_lock);
    list_for_each_entry(connected_hdl, &connected_list_head, entry) {
        if (connected_hdl->cig_hdl != cig_hdl) {
            continue;
        }

        if (connected_hdl->del) {
            continue;
        }

        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            cis_hdl_info = &connected_hdl->cis_hdl_info[i];
            if (connected_hdl->cis_hdl_info[i].cis_hdl) {

                if (cis_hdl_info->recorder && (cis_hdl_info->recorder != last_recorder)) {
                    last_recorder = cis_hdl_info->recorder;
                    txsync.cis_hdl = cis_hdl_info->cis_hdl;
                    connected_get_cis_tick_time(&txsync);
                    timestamp = (txsync.tx_ts + connected_hdl->cig_sync_delay +
                                 get_cig_mtl_time() * 1000L + get_cig_sdu_period_us()) & 0xfffffff;
                    rlen = le_audio_stream_tx_data_handler(cis_hdl_info->recorder, transmit_buf,
                                                           get_cig_transmit_data_len(), timestamp, get_cig_play_latency());
                }

                if (!rlen) {
                    putchar('^');
                    continue;
                }

                param.cis_hdl = cis_hdl_info->cis_hdl;
#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
                channel_separation(transmit_buf, L_buffer, R_buffer, get_cig_transmit_data_len(), packet_num);
                if (!(i % 2)) {
                    err = wireless_trans_transmit((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP ? "cig_perip" : "cig_central", L_buffer, single_ch_trans_data_len, &param);
                } else {
                    err = wireless_trans_transmit((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP ? "cig_perip" : "cig_central", R_buffer, single_ch_trans_data_len, &param);
                }
#else
                err = wireless_trans_transmit((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP ? "cig_perip" : "cig_central", transmit_buf, get_cig_transmit_data_len(), &param);
#endif
                if (err != 0) {
                    log_error("wireless_trans_transmit fail\n");
                }
            }
        }
    }
    spin_unlock(&connected_lock);

#if (LEA_TX_CHANNEL_SEPARATION && (LE_AUDIO_CODEC_CHANNEL == 2))
    if (L_buffer) {
        free(L_buffer);
    }

    if (R_buffer) {
        free(R_buffer);
    }
#endif

    return 0;
}
static int cis_channel_disconnect_handler(cig_hdl_t *hdl)
{
    struct connected_hdl *p;
    int i = 0;

    spin_lock(&connected_lock);
    list_for_each_entry(p, &connected_list_head, entry) {
        if (p->cig_hdl != hdl->cig_hdl) {
            continue;
        }
        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            if (p->cis_hdl_info[i].cis_hdl != hdl->cis_hdl) {
                continue;
            }

            if (p->cis_hdl_info[i].rx_player.rx_stream) {
                le_audio_stream_rx_disconnect(p->cis_hdl_info[i].rx_player.rx_stream);
            }
        }
    }
    spin_unlock(&connected_lock);

    return 0;
}

static void connected_central_event_callback(const CIG_EVENT event, void *priv)
{
    /* log_info("--func=%s, %d", __FUNCTION__, event); */
    struct connected_hdl *hdl;
    cig_hdl_t cig_handler_t;

    switch (event) {
    //cis发射端开启成功后回调事件
    case CIG_EVENT_CIS_CONNECT:
        log_info("CIG_EVENT_CIS_CONNECT0\n");
        cig_event_to_user(CIG_EVENT_CENTRAL_CONNECT, priv, sizeof(cig_hdl_t));
        break;

    case CIG_EVENT_CIS_DISCONNECT:
        memcpy(&cig_handler_t, priv, sizeof(cig_hdl_t));
        connected_mutex_pend(&connected_mutex, __LINE__);
        spin_lock(&connected_lock);
        list_for_each_entry(hdl, &connected_list_head, entry) {
            for (u8 i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
                if (hdl->cis_hdl_info[i].cis_hdl == cig_handler_t.cis_hdl) {
                    //提前拦截蓝牙中断来数，避免音频堆积过多数据
                    hdl->cis_hdl_info[i].cis_discon = TRUE;
                    break;
                }
            }
        }
        cis_channel_disconnect_handler((cig_hdl_t *)priv);
        spin_unlock(&connected_lock);
        connected_mutex_post(&connected_mutex, __LINE__);
        log_info("CIG_EVENT_CIS_DISCONNECT0\n");
        cig_event_to_user(CIG_EVENT_CENTRAL_DISCONNECT, priv, sizeof(cig_hdl_t));
        break;

    case CIG_EVENT_ACL_CONNECT:
        log_info("CIG_EVENT_ACL_CONNECT0\n");
        cig_event_to_user(event, priv, sizeof(cis_acl_info_t));
        break;

    case CIG_EVENT_ACL_DISCONNECT:
        log_info("CIG_EVENT_ACL_DISCONNECT0\n");
        cig_event_to_user(event, priv, sizeof(cis_acl_info_t));
        break;

    //蓝牙取数发射回调事件
    case CIG_EVENT_TRANSMITTER_ALIGN:
        /* WARNING:该事件为中断函数回调, 不要添加过多打印 */
        u8 cig_hdl = *((u8 *)priv);
        connected_tx_align_data_handler(cig_hdl);
        break;

    default:
        break;
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG主机启动接口
 *
 * @param params:CIG主机启动所需参数
 *
 * @return 分配的cig_hdl
 */
/* ----------------------------------------------------------------------------*/
int connected_central_open(cig_parameter_t *params)
{
    int ret;

    connected_init(CONNECTED_ROLE_CENTRAL);

    if (!connected_init_flag) {
        return -2;
    }

    if (connected_num >= CIG_MAX_NUMS) {
        log_error("connected_num overflow");
        return -1;
    }

    if ((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        log_error("connected_role err");
        return -1;
    }

    log_info("--func=%s", __FUNCTION__);
    u8 available_cig_hdl = get_available_cig_hdl(++g_cig_hdl, &connected_list_head);

    set_cig_hdl(CONNECTED_ROLE_CENTRAL, available_cig_hdl);
    ret = wireless_trans_open("cig_central", (void *)params);
    if (ret != 0) {
        log_error("wireless_trans_open fail:0x%x\n", ret);
        if (connected_num == 0) {
            connected_role = CONNECTED_ROLE_UNKNOW;
        }
        return -1;
    }

    if (transmit_buf) {
        free(transmit_buf);
    }
    transmit_buf = zalloc(get_cig_transmit_data_len());
    ASSERT(transmit_buf, "transmit_buf is NULL");

    connected_role = CONNECTED_ROLE_CENTRAL | BIT(7);	//或上BIT(7)，防止外部流程判断错误

    //开启CIS后关闭经典蓝牙可发现可连接
#if LEA_CIG_CENTRAL_CLOSE_EDR_CONN
    bt_close_discoverable_and_connectable();
    bredr_close_flag = 1;;
#endif

    connected_num++;

    //TODO:修改时钟
    clock_alloc("le_connected", 12 * 1000000UL);

    return available_cig_hdl;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG从机连接成功处理事件
 *
 * @param priv:连接成功附带的句柄参数
 *
 * @return 是否执行成功 -- 0为成功，其他为失败
 */
/* ----------------------------------------------------------------------------*/
int connected_perip_connect_deal(void *priv)
{
    u8 i;
    u8 index = 0;
    u8 find = 0;
    struct connected_hdl *connected_hdl = 0;
    cig_hdl_t *hdl = (cig_hdl_t *)priv;
    struct le_audio_stream_params params;


    log_info("connected_perip_connect_deal");
    log_info("hdl->cig_hdl:%d, hdl->cis_hdl:%dMax_PDU_C_To_P:%d,Max_PDU_P_To_C:%d,", hdl->cig_hdl, hdl->cis_hdl, hdl->Max_PDU_C_To_P, hdl->Max_PDU_P_To_C);

    //真正连上设备后，清除BIT(7)，使外部跑转发流程
    connected_role &= ~BIT(7);

    connected_mutex_pend(&connected_mutex, __LINE__);
    list_for_each_entry(connected_hdl, &connected_list_head, entry) {
        if (connected_hdl->cig_hdl == hdl->cig_hdl) {
            find = 1;
            break;
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);

    if (!find) {
        connected_hdl = (struct connected_hdl *)zalloc(sizeof(struct connected_hdl));
        ASSERT(connected_hdl, "connected_hdl is NULL");
    }

    for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
        if (!connected_hdl->cis_hdl_info[i].cis_hdl) {
            index = i;
            break;
        }
    }

    le_audio_switch_ops = get_connected_audio_sw_ops();

    //关闭本地音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->local_audio_close) {
        le_audio_switch_ops->local_audio_close();
    }

    if (!connected_hdl->latch_cis_hdl) {
        connected_hdl->latch_cis_hdl = hdl->cis_hdl;
    }

    params.fmt.nch = get_cig_audio_coding_nch();
    params.fmt.bit_rate = get_cig_audio_coding_bit_rate();
    params.fmt.coding_type = LE_AUDIO_CODEC_TYPE;
    params.fmt.frame_dms = get_cig_audio_coding_frame_duration();
    params.fmt.sdu_period = get_cig_sdu_period_us();
    params.fmt.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
    params.fmt.dec_ch_mode = LEA_RX_DEC_OUTPUT_CHANNEL;
    params.latency = get_cig_tx_latency();
    params.conn = connected_hdl->latch_cis_hdl;

    connected_hdl->role_name = "cig_perip";
    connected_hdl->cig_hdl = hdl->cig_hdl;
    connected_hdl->cis_hdl_info[index].acl_hdl = hdl->acl_hdl;
    connected_hdl->cis_hdl_info[index].cis_hdl = hdl->cis_hdl;

    printf("le_audio  fmt: %d %d %d %d %d %d\n", params.fmt.coding_type, params.fmt.frame_dms, params.fmt.bit_rate,
           params.fmt.sample_rate, params.fmt.sdu_period, params.fmt.nch);

#if (LEA_CIG_TRANS_MODE == 2)

    //TODO:开启播放器播放远端传来的音频
    if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
        le_audio_switch_ops->rx_le_audio_open(&connected_hdl->cis_hdl_info[index].rx_player, &params);
    }

    if (!connected_hdl->cis_hdl_info[index].recorder) {
        //TODO:打开recorder
        if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
            connected_hdl->cis_hdl_info[index].recorder = le_audio_switch_ops->tx_le_audio_open(&params);
        }
    }

    if (!connected_hdl->cig_sync_delay) {
        connected_hdl->cig_sync_delay = hdl->cig_sync_delay;
    }

#elif (LEA_CIG_TRANS_MODE == 1)

#if (LEA_CIG_CONNECT_MODE == 2)

    //两发一收perip做发送，使用发射解码声道配置
    params.fmt.dec_ch_mode = LEA_TX_DEC_OUTPUT_CHANNEL;

    if (!connected_hdl->cis_hdl_info[index].recorder) {
        //TODO:打开recorder
        if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
            connected_hdl->cis_hdl_info[index].recorder = le_audio_switch_ops->tx_le_audio_open(&params);
        }
    }

    if (!connected_hdl->cig_sync_delay) {
        connected_hdl->cig_sync_delay = hdl->cig_sync_delay;
    }
#else
    //TODO:开启播放器播放远端传来的音频
    if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
        le_audio_switch_ops->rx_le_audio_open(&connected_hdl->cis_hdl_info[index].rx_player, &params);
    }
#endif
#endif

    connected_hdl->cis_hdl_info[index].init_ok = 1;

    connected_mutex_pend(&connected_mutex, __LINE__);
    if (!find) {
        spin_lock(&connected_lock);
        list_add_tail(&connected_hdl->entry, &connected_list_head);
        spin_unlock(&connected_lock);
    }
    connected_mutex_post(&connected_mutex, __LINE__);

    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG从机连接断开成功处理接口口
 *
 * @param priv:断链成功附带的句柄参数
 *
 * @return 是否执行成功 -- 0为成功，其他为失败
 */
/* ----------------------------------------------------------------------------*/
int connected_perip_disconnect_deal(void *priv)
{
    u8 i, index;
    u8 cis_connected_num = 0;
    int player_status = 0;
    struct connected_hdl *p, *connected_hdl;
    cig_hdl_t *hdl = (cig_hdl_t *)priv;
    void *recorder = 0;
    struct le_audio_player_hdl player;

    player.le_audio = 0;
    player.rx_stream = 0;

    log_info("connected_perip_disconnect_deal");
    log_info("%s, cig_hdl:%d", __FUNCTION__, hdl->cig_hdl);

    //TODO:获取播放状态
    if (le_audio_switch_ops && le_audio_switch_ops->play_status) {
        player_status = le_audio_switch_ops->play_status();
    }

    connected_mutex_pend(&connected_mutex, __LINE__);
    spin_lock(&connected_lock);
    list_for_each_entry(p, &connected_list_head, entry) {
        if (p->cig_hdl == hdl->cig_hdl) {

            for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
                if (p->cis_hdl_info[i].init_ok && (p->cis_hdl_info[i].cis_hdl == hdl->cis_hdl)) {
                    p->cis_hdl_info[i].init_ok = 0;
                    p->cis_hdl_info[i].cis_hdl = 0;
                    if (p->cis_hdl_info[i].recorder) {
                        recorder = p->cis_hdl_info[i].recorder;
                        p->cis_hdl_info[i].recorder = NULL;
                    }

                    if (p->cis_hdl_info[i].rx_player.le_audio) {
                        player.le_audio = p->cis_hdl_info[i].rx_player.le_audio;
                        p->cis_hdl_info[i].rx_player.le_audio = NULL;
                    }

                    if (p->cis_hdl_info[i].rx_player.rx_stream) {
                        player.rx_stream = p->cis_hdl_info[i].rx_player.rx_stream;
                        p->cis_hdl_info[i].rx_player.rx_stream = NULL;
                    }
                    index = i;
                } else if (p->cis_hdl_info[i].cis_hdl) {
                    cis_connected_num++;
                }
            }

            for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
                if (p->cis_hdl_info[i].cis_hdl) {
                    p->latch_cis_hdl = p->cis_hdl_info[i].cis_hdl;
                    break;
                }
            }

            spin_unlock(&connected_lock);

            if (recorder) {
                //TODO:关闭recorder
                if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                    le_audio_switch_ops->tx_le_audio_close(recorder);
                }
            }
            if (player.le_audio && player.rx_stream) {
                //TODO:关闭player
                if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_close) {
                    le_audio_switch_ops->rx_le_audio_close(&player);
                }
            }

            spin_lock(&connected_lock);

            memset(&p->cis_hdl_info[index], 0, sizeof(cis_hdl_info_t));

            connected_hdl = p;
            break;
        }
    }
    spin_unlock(&connected_lock);
    connected_mutex_post(&connected_mutex, __LINE__);

    if (!cis_connected_num) {

        connected_role |= BIT(7);   //断开连接后，或上BIT(7)，防止外部流程判断错误

        connected_hdl->cig_sync_delay = 0;

        connected_hdl->latch_cis_hdl = 0;

        if (player_status == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
            //TODO:打开本地音频
            if (le_audio_switch_ops && le_audio_switch_ops->local_audio_open) {
                le_audio_switch_ops->local_audio_open();
            }
        }
    }

    return 0;
}

void le_audio_phone_connect_profile_begin(uint16_t con_handle)
{
    r_printf("le_audio_phone_connect_profile_begin");
    cig_event_to_user(CIG_EVENT_PHONE_CONNECT, (void *)&con_handle, 2);
}
void le_audio_phone_connect_profile_disconnect(uint16_t con_handle)
{
    r_printf("le_audio_phone_connect_profile_disconnect");
    cig_event_to_user(CIG_EVENT_PHONE_DISCONNECT, (void *)&con_handle, 2);
}

static void connected_perip_event_callback(const CIG_EVENT event, void *priv)
{
    /* log_info("--func=%s, %d", __FUNCTION__, event); */

    switch (event) {
    //cis接收端连接成功后回调事件
    case CIG_EVENT_CIS_CONNECT:
        log_info("%s, CIG_EVENT_CIS_CONNECT\n", __FUNCTION__);
        cig_event_to_user(CIG_EVENT_PERIP_CONNECT, priv, sizeof(cig_hdl_t));
        break;

    //cis接收端断开成功后回调事件
    case CIG_EVENT_CIS_DISCONNECT:
        log_info("%s, CIG_EVENT_CIS_DISCONNECT\n", __FUNCTION__);
        cig_event_to_user(CIG_EVENT_PERIP_DISCONNECT, priv, sizeof(cig_hdl_t));
        break;

    case CIG_EVENT_ACL_CONNECT:
        log_info("%s, CIG_EVENT_ACL_CONNECT\n", __FUNCTION__);
        cig_event_to_user(event, priv, sizeof(cis_acl_info_t));
        break;

    case CIG_EVENT_ACL_DISCONNECT:
        log_info("%s, CIG_EVENT_ACL_DISCONNECT\n", __FUNCTION__);
        cig_event_to_user(event, priv, sizeof(cis_acl_info_t));
        break;

    //蓝牙取数发射回调事件
    case CIG_EVENT_TRANSMITTER_ALIGN:
        /* WARNING:该事件为中断函数回调, 不要添加过多打印 */
        u8 cig_hdl = *((u8 *)priv);
        connected_tx_align_data_handler(cig_hdl);
        break;

    default:
        break;
    }
}

#if CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME
extern uint32_t bb_le_clk_get_time_us(void);
//ble时间转化为本地时间
static u32 le_audio_usec_to_local_usec(struct cis_ts_convert_hdl *ts_convert, u32 usec)
{
    u32 time = bb_le_clk_get_time_us();//le_audio_stream_tx_time(hdl->stream);
    u32 local_usec = audio_jiffies_usec();
    int diff = 0;

    if (ts_convert->latch_time_state == 0) {
        ts_convert->latch_time_state = 1;
        ts_convert->usec_offset = time - local_usec;
    } else {
        /*校准*/
        u32 expect_time = (local_usec + ts_convert->usec_offset) & 0xfffffff;
        if (time != expect_time) {
            if (time > expect_time) {
                if (time > 0x7ffffff && expect_time < 0x7ffffff) {
                    diff = -(int)((u32)(expect_time - time) & 0x7ffffff);
                } else {
                    diff = time - expect_time;
                }
            } else {
                if (time < 0x7ffffff && expect_time > 0x7ffffff) {
                    diff = (int)((u32)(time - expect_time) & 0x7ffffff);
                } else {
                    diff = time - expect_time;
                }
            }
            ts_convert->usec_offset += diff;
        }

        if (usec < ts_convert->tick_usec) {
            ts_convert->usec_offset -= 0x10000000;
        }
    }

    ts_convert->tick_usec = usec;

    local_usec = usec - ts_convert->usec_offset;

    /* printf("<%u, %u, %u, %lu, %d>\n", usec, time, local_usec, audio_jiffies_usec(),diff); */
    return local_usec;
}
#endif

static void connected_iso_callback(const void *const buf, size_t length, void *priv)
{
    u8 err_flag = 0;
    cig_stream_param_t *param;
    struct connected_hdl *hdl;
    struct wireless_data_callback_func *p;
    u32 timestamp;

    param = (cig_stream_param_t *)priv;

    if (param->acl_hdl) {
        //收取同步数据
        list_for_each_wireless_data_callback(p) {
            if (p->rx_events_suss) {
                if (p->rx_events_suss(param->acl_hdl, buf, length)) {
                    break;
                }
            }
        }
        return;
    }

    /* log_info("<<- cis Data Out <<- TS:%d,%d", param->ts, length); */
    /* put_buf(buf, 2); */
    spin_lock(&connected_lock);
    list_for_each_entry(hdl, &connected_list_head, entry) {
        if (hdl->del) {
            continue;
        }
#if CIS_TIMSTAMP_CONVERT_TO_LOCOAL_TIME
        //cis 时间戳转换为本地时间
        struct cis_ts_convert_hdl *ts_convert = &hdl->ts_convert;
        if (ts_convert->prev_ts == param->ts) {
            param->ts = ts_convert->prev_local_ts;
        } else {
            ts_convert->prev_ts = param->ts;
            param->ts = le_audio_usec_to_local_usec(ts_convert, param->ts);
            ts_convert->prev_local_ts = param->ts;
        }
#endif
        for (u8 i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            if (hdl->cis_hdl_info[i].init_ok && (!hdl->cis_hdl_info[i].cis_discon) && (hdl->cis_hdl_info[i].cis_hdl == param->cis_hdl)) {
                //收取音频数据
#if CIS_AUDIO_PLC_ENABLE
                if (length == 0) {
                    /*get_cig_sdu_period_us返回是us单位，get_cig_audio_coding_frame_duration是ms*10单位 */
                    u8 frame_num = get_cig_sdu_period_us() / 100 / get_cig_audio_coding_frame_duration();
                    for (u8 j = 0; j < frame_num; j++) {
                        memcpy((u8 *)errpacket + length, errpacket, 2);
                        length += 2;
                    }
                    err_flag = 1;
                }
#endif//CIS_AUDIO_PLC_ENABLE

                timestamp = (param->ts + get_cig_play_latency()) & 0xfffffff;
                if (err_flag) {
                    le_audio_stream_rx_frame(hdl->cis_hdl_info[i].rx_player.rx_stream, (void *)errpacket, length, timestamp);
                } else {
                    le_audio_stream_rx_frame(hdl->cis_hdl_info[i].rx_player.rx_stream, (void *)buf, length, timestamp);
                }
            }
        }
    }
    spin_unlock(&connected_lock);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief CIG从机启动接口
 *
 * @param params:CIG从机启动所需参数
 *
 * @return 分配的cig_hdl
 */
/* ----------------------------------------------------------------------------*/
int connected_perip_open(cig_parameter_t *params)
{
    int ret;

    connected_init(CONNECTED_ROLE_PERIP);

    if (!connected_init_flag) {
        return -2;
    }

    if (connected_num >= CIG_MAX_NUMS) {
        log_error("connected_num overflow");
        return -1;
    }

    if ((connected_role & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        log_error("connected_role err");
        return -1;
    }

    log_info("--func=%s", __FUNCTION__);
    u8 available_cig_hdl = 0xFF;

    set_cig_hdl(CONNECTED_ROLE_PERIP, available_cig_hdl);
    ret = wireless_trans_open("cig_perip", (void *)params);
    if (ret != 0) {
        log_error("wireless_trans_open fail:0x%x\n", ret);
        if (connected_num == 0) {
            connected_role = CONNECTED_ROLE_UNKNOW;
        }
        return -1;
    }

#if ((LEA_CIG_TRANS_MODE == 2) || (LEA_CIG_CONNECT_MODE == 2))
    if (transmit_buf) {
        free(transmit_buf);
    }
    transmit_buf = zalloc(get_cig_transmit_data_len());
    ASSERT(transmit_buf, "transmit_buf is NULL");
#endif
    //开启CIS后关闭经典蓝牙可发现可连接
#if LEA_CIG_PERIPHERAL_CLOSE_EDR_CONN
    bt_close_discoverable_and_connectable();
    bredr_close_flag = 1;
#endif

    connected_role = CONNECTED_ROLE_PERIP | BIT(7);	//或上BIT(7)，防止外部流程判断错误

    connected_num++;

    //TODO:修改时钟
    clock_alloc("le_connected", 12 * 1000000UL);

    return available_cig_hdl;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 关闭对应cig_hdl的CIG连接
 *
 * @param cig_hdl:需要关闭的CIG连接的cig_hdl
 */
/* ----------------------------------------------------------------------------*/
void connected_close(u8 cig_hdl)
{
    int player_status = 0;
    u8 i;
    int ret;
    void *recorder = 0;
    struct le_audio_player_hdl player;

    player.le_audio = 0;
    player.rx_stream = 0;

    if (!connected_init_flag) {
        return;
    }

    log_info("--func=%s", __FUNCTION__);

    //TODO:获取播放状态
    if (le_audio_switch_ops && le_audio_switch_ops->play_status) {
        player_status = le_audio_switch_ops->play_status();
    }

    struct connected_hdl *hdl;
    connected_mutex_pend(&connected_mutex, __LINE__);
    spin_lock(&connected_lock);
    list_for_each_entry(hdl, &connected_list_head, entry) {
        if (hdl->cig_hdl != cig_hdl) {
            continue;
        }
        hdl->del = 1;
    }
    spin_unlock(&connected_lock);
    connected_mutex_post(&connected_mutex, __LINE__);

    //关闭CIG
    if ((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        ret = wireless_trans_close("cig_perip", &cig_hdl);
        if (ret != 0) {
            log_error("wireless_trans_close fail:0x%x\n", ret);
        }
    } else if ((connected_role & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        ret = wireless_trans_close("cig_central", &cig_hdl);
        if (ret != 0) {
            log_error("wireless_trans_close fail:0x%x\n", ret);
        }
    }

    reset_cig_params();

    //释放链表
    struct connected_hdl *p, *n;
    connected_mutex_pend(&connected_mutex, __LINE__);
    spin_lock(&connected_lock);
    list_for_each_entry_safe(p, n, &connected_list_head, entry) {
        if (p->cig_hdl != cig_hdl) {
            continue;
        }

        list_del(&p->entry);

        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            p->cis_hdl_info[i].init_ok = 0;
            p->cis_hdl_info[i].cis_hdl = 0;
            if (p->cis_hdl_info[i].recorder) {
                recorder = p->cis_hdl_info[i].recorder;
                p->cis_hdl_info[i].recorder = NULL;
            }

            if (p->cis_hdl_info[i].rx_player.le_audio) {
                player.le_audio = p->cis_hdl_info[i].rx_player.le_audio;
                p->cis_hdl_info[i].rx_player.le_audio = NULL;
            }

            if (p->cis_hdl_info[i].rx_player.rx_stream) {
                player.rx_stream = p->cis_hdl_info[i].rx_player.rx_stream;
                p->cis_hdl_info[i].rx_player.rx_stream = NULL;
            }

            spin_unlock(&connected_lock);

            if (recorder) {
                //TODO:关闭recorder
                if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                    le_audio_switch_ops->tx_le_audio_close(recorder);
                    recorder = NULL;
                }
            }
            if (player.le_audio && player.rx_stream) {
                //TODO:关闭player
                if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_close) {
                    le_audio_switch_ops->rx_le_audio_close(&player);
                    player.le_audio = NULL;
                    player.rx_stream = NULL;
                }
            }

            spin_lock(&connected_lock);
        }

        free(p);
    }
    spin_unlock(&connected_lock);
    connected_mutex_post(&connected_mutex, __LINE__);

    if (bredr_close_flag) {
        //恢复经典蓝牙可发现可连接
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        bredr_close_flag = 0;
    }
    connected_num--;
    if (connected_num == 0) {
        connected_uninit(connected_role);
        connected_role = CONNECTED_ROLE_UNKNOW;
        clock_free("le_connected");
    }

    if (player_status == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //TODO:打开本地音频
        if (le_audio_switch_ops && le_audio_switch_ops->local_audio_open) {
            le_audio_switch_ops->local_audio_open();
        }
    }
}
/* --------------------------------------------------------------------------*/
/**
 * @brief 获取当前CIG是主机还是从机
 *
 * @return CIG角色
 */
/* ----------------------------------------------------------------------------*/
u8 get_connected_role(void)
{
    return connected_role;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief acl数据发送接口
 *
 * @param acl_hdl:acl数据通道句柄
 * @param data:需要发送的数据
 * @param length:数据长度
 *
 * @return 实际发送出去的数据长度
 */
/* ----------------------------------------------------------------------------*/
int connected_send_custom_data(u16 acl_hdl, void *data, size_t length)
{
    int err = -1;
    cig_stream_param_t param = {0};
    param.acl_hdl = acl_hdl;

    if ((connected_role & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        err = wireless_trans_transmit("cig_central", data, length, &param);
    } else if ((connected_role & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        err = wireless_trans_transmit("cig_perip", data, length, &param);
    }
    if (err != 0) {
        log_error("acl wireless_trans_transmit fail:0x%x\n", err);
        err = 0;
    } else {
        err = length;
    }
    return err;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 供外部手动初始化recorder模块
 *
 * @param cis_hdl:recorder模块所对应的cis_hdl
 */
/* ----------------------------------------------------------------------------*/
void cis_audio_recorder_reset(u16 cis_hdl)
{
    u8 i;
    struct connected_hdl *p;
    void *recorder = 0;
    struct le_audio_stream_params params = {0};

    connected_mutex_pend(&connected_mutex, __LINE__);
    list_for_each_entry(p, &connected_list_head, entry) {
        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            if (p->cis_hdl_info[i].init_ok && (p->cis_hdl_info[i].cis_hdl == cis_hdl)) {
                spin_lock(&connected_lock);
                if (p->cis_hdl_info[i].recorder) {
                    recorder = p->cis_hdl_info[i].recorder;
                    p->cis_hdl_info[i].recorder = NULL;
                }
                spin_unlock(&connected_lock);

                //关闭旧的recorder
                if (recorder) {
                    if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                        g_printf("%s %d", __FUNCTION__, __LINE__);
                        le_audio_switch_ops->tx_le_audio_close(recorder);
                    }
                }

                params.fmt.nch = get_cig_audio_coding_nch();
                params.fmt.bit_rate = get_cig_audio_coding_bit_rate();
                params.fmt.coding_type = LE_AUDIO_CODEC_TYPE;
                params.fmt.frame_dms = get_cig_audio_coding_frame_duration();
                params.fmt.sdu_period = get_cig_sdu_period_us();
                params.fmt.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
#if ((LEA_CIG_TRANS_MODE == 1) && (LEA_CIG_CONNECT_MODE == 2))
                //两发一收central做接收，使用接收解码声道配置
                params.fmt.dec_ch_mode = LEA_RX_DEC_OUTPUT_CHANNEL;
#else
                params.fmt.dec_ch_mode = LEA_TX_DEC_OUTPUT_CHANNEL;
#endif
                params.conn = p->latch_cis_hdl;

                //重新打开新的recorder
                if (!p->cis_hdl_info[i].recorder) {
                    if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_open) {
                        recorder = le_audio_switch_ops->tx_le_audio_open(&params);
                        spin_lock(&connected_lock);
                        p->cis_hdl_info[i].recorder = recorder;
                        spin_unlock(&connected_lock);
                    }
                }
            }
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 供外部手动关闭recorder模块
 *
 * @param cis_hdl:recorder模块所对应的cis_hdl
 */
/* ----------------------------------------------------------------------------*/
void cis_audio_recorder_close(u16 cis_hdl)
{
    u8 i;
    struct connected_hdl *p;
    void *recorder = 0;

    connected_mutex_pend(&connected_mutex, __LINE__);
    list_for_each_entry(p, &connected_list_head, entry) {
        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            if (p->cis_hdl_info[i].init_ok && (p->cis_hdl_info[i].cis_hdl == cis_hdl)) {
                spin_lock(&connected_lock);
                if (p->cis_hdl_info[i].recorder) {
                    recorder = p->cis_hdl_info[i].recorder;
                    p->cis_hdl_info[i].recorder = NULL;
                }
                spin_unlock(&connected_lock);

                //关闭旧的recorder
                if (recorder) {
                    if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                        le_audio_switch_ops->tx_le_audio_close(recorder);
                    }
                }
            }
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);
}



int le_connected_audio_all_close(u16 cig_hdl)
{
    u8 i;
    int ret;
    void *recorder = 0;
    int player_status = 0;
    struct le_audio_player_hdl player;

    player.le_audio = 0;
    player.rx_stream = 0;

    if (!connected_init_flag) {
        return -1;
    }
    log_info("--func=%s", __FUNCTION__);
    //TODO:获取播放状态
    struct connected_hdl *p, *n;
    connected_mutex_pend(&connected_mutex, __LINE__);
    spin_lock(&connected_lock);
    list_for_each_entry_safe(p, n, &connected_list_head, entry) {
        if (p->cig_hdl != cig_hdl) {
            continue;
        }
        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            p->cis_hdl_info[i].init_ok = 0;
            if (p->cis_hdl_info[i].recorder) {
                recorder = p->cis_hdl_info[i].recorder;
                p->cis_hdl_info[i].recorder = NULL;
            }
            if (p->cis_hdl_info[i].rx_player.le_audio) {
                player.le_audio = p->cis_hdl_info[i].rx_player.le_audio;
                p->cis_hdl_info[i].rx_player.le_audio = NULL;
            }
            if (p->cis_hdl_info[i].rx_player.rx_stream) {
                player.rx_stream = p->cis_hdl_info[i].rx_player.rx_stream;
                p->cis_hdl_info[i].rx_player.rx_stream = NULL;
            }
            spin_unlock(&connected_lock);
            if (recorder) {
                //TODO:关闭recorder
                if (le_audio_switch_ops && le_audio_switch_ops->tx_le_audio_close) {
                    le_audio_switch_ops->tx_le_audio_close(recorder);
                    recorder = NULL;
                }
            }
            if (player.le_audio && player.rx_stream) {
                //TODO:关闭player
                if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_close) {
                    le_audio_switch_ops->rx_le_audio_close(&player);
                    player.le_audio = NULL;
                    player.rx_stream = NULL;
                }
            }
            spin_lock(&connected_lock);
        }
    }
    spin_unlock(&connected_lock);
    connected_mutex_post(&connected_mutex, __LINE__);

    return 0;
}

int le_connected_audio_all_open(u16 cig_hdl)
{

    u8 find = 0;
    struct connected_hdl *connected_hdl, *n;
    struct le_audio_stream_params params = {0};

    u8 i = 0;
    void *recorder = 0;

    connected_mutex_pend(&connected_mutex, __LINE__);
    list_for_each_entry_safe(connected_hdl, n, &connected_list_head, entry) {
        if (connected_hdl->cig_hdl == cig_hdl) {
            find = 1;
            break;
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);

    le_audio_switch_ops = get_connected_audio_sw_ops();

    //关闭本地音频播放
    if (le_audio_switch_ops && le_audio_switch_ops->local_audio_close) {
        le_audio_switch_ops->local_audio_close();
    }

    params.fmt.nch = get_cig_audio_coding_nch();
    params.fmt.bit_rate = get_cig_audio_coding_bit_rate();
    params.fmt.coding_type = LE_AUDIO_CODEC_TYPE;
    params.fmt.frame_dms = get_cig_audio_coding_frame_duration();
    params.fmt.sdu_period = get_cig_sdu_period_us();
    params.fmt.sample_rate = LE_AUDIO_CODEC_SAMPLERATE;
    params.fmt.dec_ch_mode = LEA_TX_DEC_OUTPUT_CHANNEL;
    params.conn = connected_hdl->latch_cis_hdl;
    connected_hdl->role_name = "cig_central";

    connected_mutex_pend(&connected_mutex, __LINE__);
    if (find) {
        for (i = 0; i < CIS_MAX_CONNECTABLE_NUMS; i++) {
            if (!connected_hdl->cis_hdl_info[i].cis_hdl) {
                continue;
            }
            if ((!connected_hdl->cis_hdl_info[i].rx_player.le_audio) &&
                (!connected_hdl->cis_hdl_info[i].rx_player.rx_stream)) {
                if (le_audio_switch_ops && le_audio_switch_ops->rx_le_audio_open) {
                    le_audio_switch_ops->rx_le_audio_open(&connected_hdl->cis_hdl_info[i].rx_player, &params);
                    spin_lock(&connected_lock);
                    connected_hdl->cis_hdl_info[i].init_ok = 1;
                    spin_unlock(&connected_lock);
                }
            }
        }
    }
    connected_mutex_post(&connected_mutex, __LINE__);
    return 0;
}

#endif

