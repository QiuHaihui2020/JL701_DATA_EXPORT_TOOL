#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_tool.data.bss")
#pragma data_seg(".lp_touch_key_tool.data")
#pragma const_seg(".lp_touch_key_tool.text.const")
#pragma code_seg(".lp_touch_key_tool.text")
#endif
#include "system/includes.h"
#include "asm/lp_touch_key_tool.h"
#include "asm/lp_touch_key_api.h"
#include "btstack/avctp_user.h"
#include "classic/tws_api.h"
#include "app_config.h"
#include "key_driver.h"
#include "online_db_deal.h"

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

//LP KEY在线调试工具版本号管理
const u8 lp_key_sdk_name[16] = "BR28_SDK";
const u8 lp_key_bt_ver[4]    = {0, 0, 1, 0};
struct lp_key_ver_info {
    char sdkname[16];
    u8 lp_key_ver[4];
};

//   通道号           按键事件个数
const char ch_content[] = {
    0x01,                                       //版本号
    'c', 'h', '0', '\0', 6, 0, 1, 2, 3, 4, 5,
    'c', 'h', '1', '\0', 6, 0, 1, 2, 3, 4, 5,
    'c', 'h', '2', '\0', 6, 0, 1, 2, 3, 4, 5,
    'c', 'h', '3', '\0', 6, 0, 1, 2, 3, 4, 5,
    'c', 'h', '4', '\0', 6, 0, 1, 2, 3, 4, 5,
    'c', 'h', '5', '\0', 6, 0, 1, 2, 3, 4, 5,
};
static u32 ch_content_size = 0;
static u32 ch_content_offset = 0;

enum {
    TOUCH_RECORD_GET_VERSION                = 0x05,
    TOUCH_RECORD_GET_CH_SIZE                = 0x0B,
    TOUCH_RECORD_GET_CH_CONTENT             = 0x0C,
    TOUCH_RECORD_CHANGE_MODE                = 0x0E,
    TOUCH_RECORD_COUNT                      = 0x200,
    TOUCH_RECORD_START,
    TOUCH_RECORD_STOP,
    ONLINE_OP_QUERY_RECORD_PACKAGE_LENGTH,
    TOUCH_CH_CFG_UPDATE                     = 0x3000,
    TOUCH_CH_VOL_CFG_UPDATE                 = 0x3001,
    TOUCH_CH_CFG_CONFIRM                    = 0x3100,
    TOUCH_CH_GET_VOL_CFG                    = 0x3101,
};

enum {
    LP_KEY_ONLINE_ST_INIT = 0,
    LP_KEY_ONLINE_ST_READY,
    LP_KEY_ONLINE_ST_CH_RES_DEBUG_START,
    LP_KEY_ONLINE_ST_CH_RES_DEBUG_STOP,
    LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START,
    LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM,
};

//小机接收命令包格式, 使用DB_PKT_TYPE_TOUCH通道接收
typedef struct {
    int cmd_id;
    int mode;
    int data[];
} touch_receive_cmd_t;

//小机发送按键消息格式, 使用DB_PKT_TYPE_TOUCH通道发送
typedef struct {
    u32 cmd_id;
    u32 mode;
    u32 key_event;
} lp_touch_online_key_event_t;


struct ch_ana_cfg {
    u8 isel;
    u8 vhsel;
    u8 vlsel;
};

struct ch_algo_cfg {
    u16 cfg0;
    u16 cfg1;
    u16 cfg2;
};

struct lp_touch_key_online {
    u8 state;
    u8 current_record_ch;
    u16 res_packet;
    lp_touch_online_key_event_t online_key_event;
};

static struct lp_touch_key_online lp_key_online;

int lp_touch_key_online_debug_key_event_handle(u32 ch, void *e)
{
    struct key_event *event = (struct key_event *)e;
    if ((lp_key_online.state == LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START) && (lp_key_online.current_record_ch == ch)) {
        lp_key_online.online_key_event.cmd_id = 0x3100;
        lp_key_online.online_key_event.mode = 0;
        lp_key_online.online_key_event.key_event = event->event;
        log_debug("send %d event to PC", lp_key_online.online_key_event.key_event);
        app_online_db_send(DB_PKT_TYPE_TOUCH, (u8 *)(&(lp_key_online.online_key_event)), sizeof(lp_touch_online_key_event_t));
    }

    if ((lp_key_online.state == LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM) ||
        (lp_key_online.state <= LP_KEY_ONLINE_ST_READY)) {
        return 0;
    }

    return 1;
}

int lp_touch_key_online_debug_send(u32 ch, u16 val)
{
    int err = 0;
    putchar('s');
    if ((lp_key_online.state == LP_KEY_ONLINE_ST_CH_RES_DEBUG_START) && ((lp_key_online.current_record_ch == ch) || (lp_key_online.current_record_ch == LPCTMU_CHANNEL_SIZE))) {
        lp_key_online.res_packet = val;
        err = app_online_db_send(DB_PKT_TYPE_DAT_CH0, (u8 *)(&(lp_key_online.res_packet)), 2);
    }
    return err;
}

static int lp_touch_key_online_debug_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    u8 ch = 0;
    touch_receive_cmd_t *touch_cmd;
    u8 parse_seq = ext_data[1];

    struct ch_algo_cfg *algo_cfg;
    struct ch_ana_cfg *ana_cfg;

    struct lp_key_ver_info ver_info = {0};


    log_debug("%s", __func__);
    put_buf(packet, size);
    put_buf(ext_data, ext_size);
    touch_cmd = (touch_receive_cmd_t *)packet;

    switch (touch_cmd->cmd_id) {

    case TOUCH_RECORD_GET_VERSION://第一步
        log_debug("TOUCH_RECORD_GET_VERSION");
        memcpy(ver_info.sdkname, lp_key_sdk_name, sizeof(lp_key_sdk_name));
        memcpy(ver_info.lp_key_ver, lp_key_bt_ver, sizeof(lp_key_bt_ver));
        app_online_db_ack(parse_seq, (u8 *)(&ver_info), sizeof(ver_info)); //回复版本号数据结构
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_RECORD_GET_CH_SIZE:
        log_debug("TOUCH_RECORD_GET_CH_SIZE");//第二步
        ch_content_size = sizeof(ch_content);
        ch_content_offset = 0;
        app_online_db_ack(parse_seq, (u8 *)&ch_content_size, 4); //回复对应的通道数据长度
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_RECORD_GET_CH_CONTENT:
        log_debug("TOUCH_RECORD_GET_CH_CONTENT");//第三步
        u32 ack_len = ch_content_size - ch_content_offset;
        ack_len = ack_len > 32 ? 32 : ack_len;
        app_online_db_ack(parse_seq, (u8 *)(&ch_content[ch_content_offset]), ack_len);
        ch_content_offset += ack_len;
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_RECORD_CHANGE_MODE://第四步
        log_debug("TOUCH_RECORD_CHANGE_MODE, cmd_mode = %d", touch_cmd->mode);
        lp_key_online.current_record_ch = touch_cmd->mode;
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_CH_GET_VOL_CFG:
        log_debug("TOUCH_CH_GET_VOL_CFG");//界面按钮触发，在第4步之后
        ch = lp_key_online.current_record_ch;
        u8 tmp_ana_cfg[6];
        tmp_ana_cfg[0] = 3;//最大值
        tmp_ana_cfg[1] = lpctmu_get_ana_hv_level();
        tmp_ana_cfg[2] = 3;//最大值
        tmp_ana_cfg[3] = lpctmu_get_ana_lv_level();
        tmp_ana_cfg[4] = 7;//最大值
        tmp_ana_cfg[5] = lpctmu_get_ana_cur_level(ch);
        log_debug("hv_max:%d hv:%d lv_max:%d lv:%d ic_max:%d ic:%d", tmp_ana_cfg[0],
                  tmp_ana_cfg[1],
                  tmp_ana_cfg[2],
                  tmp_ana_cfg[3],
                  tmp_ana_cfg[4],
                  tmp_ana_cfg[5]);
        app_online_db_ack(parse_seq, tmp_ana_cfg, 6); //回数据
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_CH_VOL_CFG_UPDATE://第五步
        log_debug("TOUCH_CH_VOL_CFG_UPDATE");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        ana_cfg = (struct ch_ana_cfg *)(touch_cmd->data);
        log_debug("update, isel = %d, vhsel = %d, vlsel = %d", ana_cfg->isel, ana_cfg->vhsel, ana_cfg->vlsel);
        ch = lp_key_online.current_record_ch;
        if (ch < LPCTMU_CHANNEL_SIZE) {
            lpctmu_send_m2p_cmd(RESET_IDENTIFY_ALGO);
            lpctmu_set_ana_cur_level(lp_key_online.current_record_ch, ana_cfg->isel);
            lpctmu_set_ana_hv_level(ana_cfg->vhsel);
        }
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_RECORD_START://第六步
        log_debug("TOUCH_RECORD_START");
        app_online_db_ack(parse_seq, (u8 *)"OK", 1); //该命令随便ack一个byte即可
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_RES_DEBUG_START;
        ch = lp_key_online.current_record_ch;
        if (ch < LPCTMU_CHANNEL_SIZE) {
            M2P_CTMU_CH_DEBUG |= BIT(ch);
        } else {
            M2P_CTMU_CH_DEBUG = 0xff;
        }
        break;

    case TOUCH_CH_CFG_UPDATE://第七步
        log_debug("TOUCH_CH_CFG_UPDATE");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START;
        algo_cfg = (struct ch_algo_cfg *)(touch_cmd->data);
        log_debug("update, cfg0 = %d, cfg1 = %d, cfg2 = %d", algo_cfg->cfg0, algo_cfg->cfg1, algo_cfg->cfg2);
        ch = lp_key_online.current_record_ch;
        if (ch < LPCTMU_CHANNEL_SIZE) {
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG0L + ch * 8) = (algo_cfg->cfg0 >> 0) & 0xff;
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG0H + ch * 8) = (algo_cfg->cfg0 >> 8) & 0xff;
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG1L + ch * 8) = ((algo_cfg->cfg0  + 5) >> 0) & 0xff;
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG1H + ch * 8) = ((algo_cfg->cfg0  + 5) >> 8) & 0xff;
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG2L + ch * 8) = (algo_cfg->cfg2 >> 0) & 0xff;
            M2P_MESSAGE_ACCESS(M2P_MASSAGE_CTMU_CH0_CFG2H + ch * 8) = (algo_cfg->cfg2 >> 8) & 0xff;
            lpctmu_send_m2p_cmd(RESET_IDENTIFY_ALGO);
        }
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_CH_CFG_CONFIRM://第八步
        log_debug("TOUCH_CH_CFG_CONFIRM");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM;
        M2P_CTMU_CH_DEBUG = 0;
        break;

    case TOUCH_RECORD_STOP://第九步
        log_debug("TOUCH_RECORD_STOP");
        app_online_db_ack(parse_seq, (u8 *)"OK", 1); //该命令随便ack一个byte即可
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_RES_DEBUG_STOP;
        M2P_CTMU_CH_DEBUG = 0;
        break;

    default:
        break;
    }

    return 0;
}

int lp_touch_key_online_debug_init(void)
{
    log_debug("%s", __func__);

    memset((u8 *)&lp_key_online, 0, sizeof(struct lp_touch_key_online));

    app_online_db_register_handle(DB_PKT_TYPE_TOUCH, lp_touch_key_online_debug_parse);
    lp_key_online.state = LP_KEY_ONLINE_ST_READY;

    M2P_CTMU_CH_DEBUG = 0;

    return 0;
}

int lp_touch_key_online_debug_exit(void)
{
    return 0;
}

