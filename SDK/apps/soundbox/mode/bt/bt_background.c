#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_background.data.bss")
#pragma data_seg(".bt_background.data")
#pragma const_seg(".bt_background.text.const")
#pragma code_seg(".bt_background.text")
#endif
#include "system/includes.h"
#include "btstack/avctp_user.h"
#include "btstack/bluetooth.h"
#include "btctrler/btctrler_task.h"
#include "app_config.h"
#include "bt_background.h"
#include "bt_slience_detect.h"
#include "app_main.h"
#include "app_msg.h"
#include "a2dp_player.h"
#include "app_tone.h"
#include "classic/tws_api.h"
#include "dual_conn.h"
#include "bt_tws.h"

#if (TCFG_BT_BACKGROUND_ENABLE)

#define LOG_TAG             "[BACKGROUND]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define TWS_FUNC_ID_BACKGROUND_SYNC    TWS_FUNC_ID('B', 'A', 'C', 'K')

#define TWS_MSG_BACKGROUND_GO_BACK_TO_BT         0x55

extern void clr_page_mode_active(void);
extern int bt_tws_connction_status_event_handler(int *msg);
/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台模式初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_background_init(int (*hci_handler)(struct bt_event *), int (*status_handler)(struct bt_event *))
{
    g_bt_hdl.background.background_working = 0;
    g_bt_hdl.background.goback_timer = 0xff;
    g_bt_hdl.background.original_hci_handler = hci_handler;
    g_bt_hdl.background.original_status_handler = status_handler;
    INIT_LIST_HEAD(&g_bt_hdl.background.forward_msg_head);
}

static void background_add_forward_msg(int msg_from, int *msg)
{
    struct forward_msg *_forward_msg = zalloc(sizeof(struct forward_msg));
    if (_forward_msg) {
        _forward_msg->msg_from = msg_from;
        memcpy(_forward_msg->msg, msg, sizeof(struct bt_event));
        list_add_tail(&_forward_msg->entry, &g_bt_hdl.background.forward_msg_head);
    }
}
static void background_wait_phone_end(void *priv)
{

    if ((app_var.siri_stu) && (app_var.siri_stu != 3)) {
        // siri不退出
        return;
    }

    if ((bt_get_call_status() == BT_CALL_OUTGOING)
        || (bt_get_call_status() == BT_CALL_ALERT)
        || (bt_get_call_status() == BT_CALL_INCOMING)
        || (bt_get_call_status() == BT_CALL_ACTIVE)
       ) {
        // 通话不退出
        return;
    }

    if (bt_get_esco_coder_busy_flag()) {
        return;
    }

    if (g_bt_hdl.background.goback_timer != 0xff) {
        sys_timer_del(g_bt_hdl.background.goback_timer);
    }
    g_bt_hdl.background.goback_timer = 0xff;

    app_send_message(APP_MSG_GOTO_MODE, g_bt_hdl.background.goback_mode);
}

static void background_goback_with_phone(void)
{
    if (g_bt_hdl.background.goback_timer == 0xff) {
        g_bt_hdl.background.goback_timer = sys_timer_add(NULL, background_wait_phone_end, 1000);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台模式退出蓝牙
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_background_suspend()
{
    u8 suepend_rx_bulk = 0;
    g_bt_hdl.exiting = 0;
    g_bt_hdl.background.background_working = 1;
    g_bt_hdl.background.backmode = BACKGROUND_GOBACK_WITH_MODE_SWITCH;

#if (TCFG_USER_TWS_ENABLE && TCFG_LOCAL_TWS_ENABLE == 0)
    if (tws_api_is_connect()) {
        tws_api_detach(TWS_DETACH_BY_POWEROFF, 5000);
    }
    tws_api_cancle_create_connection();
    g_bt_hdl.ignore_discon_tone ++;
#endif

#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
    if (bt_get_total_connect_dev() > 0) {
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
    } else {
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    }
#endif

    sys_auto_shut_down_disable();

    btctrler_suspend(suepend_rx_bulk);
    btstack_suspend();

    /*关掉解码, 打开能量检测，处理暂停之后又播歌的情况，能跳回蓝牙模式*/
    u8 addr[6];
    if (a2dp_player_get_btaddr(addr)) {
        a2dp_player_close(addr);
        bt_start_a2dp_slience_detect(addr, 50);     //这里处理能跳回蓝牙模式外也处理后台丢包功能，如果手机一直没有发stop过来，这里会一直丢静音数据
    } else {
        u8 *get_g_play_addr(void);
        bt_start_a2dp_slience_detect(get_g_play_addr(), 50);        //广播打开的情况，这里已经把player关闭了
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙后台模式返回蓝牙
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_background_resume(void)
{
    g_bt_hdl.background.background_working = 0;
    g_bt_hdl.background.goback_fitler = 0;

    btstack_resume();
    btctrler_resume();

#if (TCFG_USER_TWS_ENABLE && TCFG_LOCAL_TWS_ENABLE == 0)
    if (bt_tws_is_paired()) {
        app_send_message(APP_MSG_TWS_PAIRED, 0);
    } else {
        app_send_message(APP_MSG_TWS_UNPAIRED, 0);
    }
#elif TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
    app_send_message(APP_MSG_BT_PAGE_DEVICE, 0);
#endif
    void *devices[2];
    if (btstack_get_conn_devices(devices, 2) < 1) {          //无设备连接才打开自动关机
        sys_auto_shut_down_enable();
    }

    log_info("bt_background_resume\n");
    if (g_bt_hdl.background.backmode == BACKGROUND_GOBACK_WITH_PHONE && g_bt_hdl.background.goback_mode != APP_MODE_SINK) {
        log_info("bt_background_goback_with_phone\n");
        background_goback_with_phone();
    }
    /*除切模式触发的后台返回的消息需要重新处理*/
    if (g_bt_hdl.background.backmode != BACKGROUND_GOBACK_WITH_MODE_SWITCH) {
        struct forward_msg *p, *n;
        list_for_each_entry_safe(p, n, &g_bt_hdl.background.forward_msg_head, entry) {
            os_taskq_post_type("app_core", p->msg_from, \
                               sizeof(struct bt_event) / 4, p->msg);
            __list_del_entry(&(p->entry));
            free(p);
        }
        r_printf("list_emtry:%d\n",  list_empty(&g_bt_hdl.background.forward_msg_head));
    }
}

int bt_background_check_if_can_enter()
{
    int esco_state;

    if (app_var.siri_stu && app_var.siri_stu != 3 && bt_get_esco_coder_busy_flag()) {
        // siri不退出
        return -EINVAL;
    }

    esco_state = bt_get_call_status();
    if (esco_state == BT_CALL_OUTGOING  ||
        esco_state == BT_CALL_ALERT     ||
        esco_state == BT_CALL_INCOMING  ||
        esco_state == BT_CALL_ACTIVE) {
        // 通话不退出
        return -EINVAL;
    }

    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台HCI事件过滤处理
   @param    event: 事件
   @return   0:不需要切换模式  1:需要切换模式  2:通话导致需要切换  3:需要走原来的消息处理流程
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_background_hci_event_filter(struct bt_event *event)
{
    log_info("bt hci event: %d \n", event->event);
    int ret = BACKGROUND_EVENT_NO_MACTH;
    switch (event->event) {
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        /* clock_add_set(BT_CONN_CLK); To Do?*/
#if TCFG_BT_BACKGROUND_GOBACK  && !USER_SUPPORT_DUAL_A2DP_SOURCE
        ret = BACKGROUND_SWITCH_TO_BT;
#endif
        break;
    default:
        ret = BACKGROUND_EVENT_ORIGINAL_DEAL;
        break;
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台BTSTACK事件过滤处理
   @param    event: 事件
   @return   0:不需要切换模式  1:需要切换模式  2:通话导致需要切换  3:需要走原来的消息处理流程
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_background_btstack_event_filter(struct bt_event *event)
{
    u8 ret = BACKGROUND_EVENT_NO_MACTH;
    log_info("btstack event: %d  %d %d\n", event->event, event->value, tws_api_get_role());
    switch (event->event) {
    // 需要切换蓝牙的命令
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        //关机导致的断开不可以回去蓝牙，否则后台关机会有问题
        if (app_var.goto_poweroff_flag) {
            break;
        }

        app_set_a2dp_play_status(0);
        if (g_bt_hdl.background.close_bt_hw_in_background) {
            //需要后台关闭蓝牙硬件的就不返回蓝牙了
            printf("close_bt_hw_in_background not go back\n");
            break;
        }
#if TCFG_BT_BACKGROUND_GOBACK && (TCFG_BACKGROUND_WITHOUT_EDR_CONNECT == 0)
#if !USER_SUPPORT_DUAL_A2DP_SOURCE
        ret = BACKGROUND_SWITCH_TO_BT;
#endif
#else
        ret = BACKGROUND_EVENT_ORIGINAL_DEAL;

#endif
        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
#if TCFG_BT_BACKGROUND_GOBACK
#if !USER_SUPPORT_DUAL_A2DP_SOURCE
        if (!check_page_mode_active()) {        //如果是回连过程中不返回
            ret = BACKGROUND_SWITCH_TO_BT;
        } else {
            ret = BACKGROUND_EVENT_ORIGINAL_DEAL;
        }
#endif
#else
        ret = BACKGROUND_EVENT_ORIGINAL_DEAL;
        /* bt_status_connect_background(&event->u.bt); */
#endif
#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
        //在蓝牙模式回连过程切到别的模式后台连接上要主动断开
        bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
#endif
        clr_page_mode_active();
        break;

    case BT_STATUS_START_CONNECTED:
#if TCFG_BT_BACKGROUND_GOBACK && !USER_SUPPORT_DUAL_A2DP_SOURCE
        ret = BACKGROUND_SWITCH_TO_BT;
#endif
        break;

    case  BT_STATUS_ENCRY_COMPLETE:
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        ret = BACKGROUND_SWITCH_TO_BT;
        break;
    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
    /* bt_status_last_call_type_change(&event->u.bt); */
    case BT_STATUS_VOICE_RECOGNITION:
    case BT_STATUS_PHONE_INCOME:
    case BT_STATUS_PHONE_NUMBER:
    /* case BT_STATUS_PHONE_MANUFACTURER: */
    case BT_STATUS_PHONE_OUT:
    case BT_STATUS_PHONE_ACTIVE:
        /* case BT_STATUS_PHONE_HANGUP: */
        ret = BACKGROUND_PHONE_CALL_SWITCH_TO_BT;
        break;
    // 不需要处理的命令
    /* case BT_STATUS_INIT_OK: 这里目前看了暂时不需要区分？*/
    /* 	bt_status_init_ok_background(&event->u.bt); */
    /* 	break; */
    case BT_STATUS_A2DP_MEDIA_START:
        log_info("BT_STATUS_A2DP_MEDIA_START start slience detect\n");
        app_set_a2dp_play_status(1);
        bt_start_a2dp_slience_detect(event->args, 50);      //丢掉50包(约1s)之后才开始能量检测,过滤掉提示音，避免提示音引起抢占
        ret = BACKGROUND_A2DP_SLIENCE_DETECT;
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        app_set_a2dp_play_status(0);
        bt_stop_a2dp_slience_detect(event->args);
#if TCFG_USER_TWS_ENABLE
        void tws_a2dp_player_close(u8 * bt_addr);
        tws_a2dp_player_close(event->args);
#else
        a2dp_play_close(event->args);
#endif
        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        break;
    // 按原方式处理的命令
    default:
        ret = BACKGROUND_EVENT_ORIGINAL_DEAL;
        break;
    }
    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    判断是否处于蓝牙后台
   @param    void
   @return   TURE:处于蓝牙后台   FALSE:不处于蓝牙后台
   @note
*/
/*----------------------------------------------------------------------------*/
bool bt_background_active(void)
{
    return (g_bt_hdl.background.background_working) ? TRUE : FALSE;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台返回蓝牙模式
   @param    *msg_type:返回蓝牙模式的消息类型
             *msg:返回蓝牙模式的消息内容
             *mode:返回蓝牙模式的类型
   @return   void
   @note
*/
/*----------------------------------------------------------------------------*/
static void bt_background_goback(int msg_type, int *msg, BACKGROUND_GOBACK_MODE mode)
{
    //返回蓝牙模式消息还需要走原本的消息处理函数
    background_add_forward_msg(msg_type, msg);
    if (g_bt_hdl.background.goback_fitler == 0) {       //处理通话时可能触发多次goback导致重复切换任务
        g_bt_hdl.background.goback_fitler = 1;
        g_bt_hdl.background.backmode = mode;
        if (g_bt_hdl.background.backmode == BACKGROUND_GOBACK_WITH_PHONE) {		//由通话返回蓝牙模式需要记录当前模式，在通话结束之后返回
            g_bt_hdl.background.goback_mode = app_get_current_mode()->name;
        }
        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
#if TCFG_USER_TWS_ENABLE
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            u8 data[2] = {TWS_MSG_BACKGROUND_GO_BACK_TO_BT, mode};
            tws_api_send_data_to_slave(&data, sizeof(data), TWS_FUNC_ID_BACKGROUND_SYNC);
        }
#endif
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台消息分发
   @param    *msg_from:消息类型
             *msg:消息内容
   @return   void
   @note
*/
/*----------------------------------------------------------------------------*/
static void background_msg_forward(int msg_from, int *msg)
{

    const struct app_msg_handler *handler;

    for_each_app_msg_handler(handler) {
        if (handler->from != msg_from) {
            continue;
        }

        handler->handler(msg);
    }
}

static int background_btstack_event_handler(int *event)
{
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }
    u8 ret = bt_background_btstack_event_filter((struct bt_event *)event);
    switch (ret) {
    case BACKGROUND_A2DP_SLIENCE_DETECT:
        /* background_add_forward_msg(MSG_FROM_BT_STACK, event); */
        break;
    case BACKGROUND_PHONE_CALL_SWITCH_TO_BT:
        g_printf("BACKGROUND_SWITCH_TO_BT_WITH_PHONE\n");
        bt_background_goback(MSG_FROM_BT_STACK, event, BACKGROUND_GOBACK_WITH_PHONE);
        break;
    case BACKGROUND_SWITCH_TO_BT:
        if (!check_page_mode_active()) {        //如果是回连过程中不返回
            g_printf("BACKGROUND_SWITCH_TO_BT\n");
            bt_background_goback(MSG_FROM_BT_STACK, event, BACKGROUND_GOBACK_WITH_OTHER);
        }
        break;
    case BACKGROUND_EVENT_ORIGINAL_DEAL:
        g_printf("BACKGROUND_ORIGINAL_DEAL\n");
        if (g_bt_hdl.background.original_status_handler) {
            g_bt_hdl.background.original_status_handler((struct bt_event *)event);
        }
        background_msg_forward(MSG_FROM_BT_STACK, event);
        break;
    }
    return 0;
}

static int background_hci_event_handler(int *event)
{
    u8 ret = bt_background_hci_event_filter((struct bt_event *)event);
    switch (ret) {
    case BACKGROUND_PHONE_CALL_SWITCH_TO_BT:
        g_printf("BACKGROUND_SWITCH_TO_BT_WITH_PHONE\n");
        bt_background_goback(MSG_FROM_BT_HCI, event, BACKGROUND_GOBACK_WITH_PHONE);
        break;
    case BACKGROUND_SWITCH_TO_BT:
        g_printf("BACKGROUND_SWITCH_TO_BT\n");
        bt_background_goback(MSG_FROM_BT_HCI, event, BACKGROUND_GOBACK_WITH_OTHER);
        break;
    case BACKGROUND_EVENT_ORIGINAL_DEAL:
        g_printf("BACKGROUND_ORIGINAL_DEAL\n");
        if (g_bt_hdl.background.original_hci_handler) {
            g_bt_hdl.background.original_hci_handler((struct bt_event *)event);
        }
        background_msg_forward(MSG_FROM_BT_HCI, event);
        break;
    }
    return 0;
}

static int background_app_msg_handler(int *msg)
{
    int ret = TRUE;
    switch (msg[0]) {
    case APP_MSG_BT_A2DP_START:
        g_printf("BACKGROUND_SWITCH_TO_BT A2DP_START\n");
        struct bt_event event;
        event.event = BT_STATUS_A2DP_MEDIA_START;           //这里需要触发流程去打开解码
        memcpy(event.args, msg + 1, 6);
        background_add_forward_msg(MSG_FROM_BT_STACK, (int *)&event);
        g_bt_hdl.background.backmode = BACKGROUND_GOBACK_WITH_MUSIC;
        app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
#if TCFG_USER_TWS_ENABLE
        u8 data[2] = {TWS_MSG_BACKGROUND_GO_BACK_TO_BT, BACKGROUND_GOBACK_WITH_MUSIC};
        tws_api_send_data_to_slave(&data, sizeof(data), TWS_FUNC_ID_BACKGROUND_SYNC);
#endif
        break;
    default:
        ret = FALSE;        //APP_MSG没有match上需要走消息转发流程
        break;
    }
    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    后台过滤消息,如果处于后台模式且消息类型属于后台处理的，由后台来处理
   @param    msg: 需要处理的消息内容
   @return   TRUE: 消息由后台处理，无需再转发   FALSE:消息需要转发
   @note
*/
/*----------------------------------------------------------------------------*/
bool bt_background_msg_forward_filter(int *msg)
{
    bool ret = TRUE;
    if (g_bt_hdl.background.background_working) {
        switch (msg[0]) {
#if TCFG_USER_TWS_ENABLE
        case MSG_FROM_TWS:
            bt_tws_connction_status_event_handler(msg + 1);
            ret = FALSE;
            break;
#endif
        case MSG_FROM_BT_HCI:
            background_hci_event_handler(msg + 1);
            break;
        case MSG_FROM_BT_STACK:
            background_btstack_event_handler(msg + 1);
            break;
        case MSG_FROM_APP:
            ret = background_app_msg_handler(msg + 1);
            break;
        default:
            ret = FALSE;
            break;
        }
    } else {
        ret = FALSE;
    }
    return ret;
}

void bt_background_set_switch_mode(u8 mode)
{
    g_printf("bt_background_set_switch_mode:%d\n", mode);
    g_bt_hdl.background.poweron_need_switch_mode = TRUE;
    g_bt_hdl.background.poweron_mode = mode;
}

bool bt_background_switch_mode_check(void)
{
    return g_bt_hdl.background.poweron_need_switch_mode;
}

void bt_background_switch_mode_after_initializes(void)
{
    if (g_bt_hdl.background.poweron_need_switch_mode) {
        g_printf(" bt_background_switch_mode_after_initializes:%d\n", g_bt_hdl.background.poweron_need_switch_mode);
        app_send_message(APP_MSG_GOTO_MODE, g_bt_hdl.background.poweron_mode);
        g_bt_hdl.background.poweron_need_switch_mode = FALSE;
    }
}

#if TCFG_USER_TWS_ENABLE

static void bt_background_sync(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        if (data[0] == TWS_MSG_BACKGROUND_GO_BACK_TO_BT) {
            g_bt_hdl.background.backmode  = data[1];
            g_bt_hdl.background.goback_mode = app_get_current_mode()->name;
            app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
        }
    }
}

REGISTER_TWS_FUNC_STUB(background_sync_stub) = {
    .func_id = TWS_FUNC_ID_BACKGROUND_SYNC,
    .func    = bt_background_sync,
};
#endif

#endif
