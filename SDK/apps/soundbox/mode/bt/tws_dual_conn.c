#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tws_dual_conn.data.bss")
#pragma data_seg(".tws_dual_conn.data")
#pragma const_seg(".tws_dual_conn.text.const")
#pragma code_seg(".tws_dual_conn.text")
#endif
#include "btstack/avctp_user.h"
#include "classic/tws_api.h"
#include "app_main.h"
#include "soundbox.h"
#include "app_config.h"
#include "bt_tws.h"
#include "user_cfg.h"
#include "bt_common.h"
#include "le_broadcast.h"
#include "dual_conn.h"

#if(TCFG_USER_TWS_ENABLE && TCFG_APP_BT_EN)

#define MAX_PAGE_DEVICE_NUM 2

#define TIMEOUT_CONN_DEVICE_OPEN_PAGE  1 //第二台设备超时断开回连一直开启page

struct page_device_info {
    struct list_head entry;
    u32 timeout;
    u16 timer;
    u8 mac_addr[6];
};

struct dual_conn_handle {
    u16 timer;
    u16 page_scan_timer;
    u16 close_inquiry_scan_timer;
    u8 device_num_recorded;
    u8 remote_addr[3][6];
    u8 page_head_inited;
    u8 page_scan_auto_disable;
    u8 inquiry_scan_disable;
    struct list_head page_head;
};

static struct dual_conn_handle g_dual_conn;
static u8 page_mode_active = 0;

void clr_page_mode_active(void)
{
    page_mode_active = 0;
}

static bool page_list_empty()
{
    return list_empty(&g_dual_conn.page_head);
}

static void auto_close_page_scan(void *p)
{
    puts("auto_close_page_scan\n");
    g_dual_conn.page_scan_timer = 0;
    g_dual_conn.page_scan_auto_disable = 1;
    lmp_hci_write_scan_enable((0 << 1) | 0);

    u8 data[2] = {0, 0};
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    tws_api_send_data_to_slave(data, 2, 0xF730EBC7);
}

static void write_scan_conn_enable(bool scan_enable, bool conn_enable)
{
    if (g_dual_conn.page_scan_auto_disable) {
        if (!scan_enable && conn_enable) {
            return;
        }
    }

#if (TCFG_BT_DUAL_CONN_ENABLE == 0)
    if (bt_get_total_connect_dev()) {
        return;
    }
#endif

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && LEA_BIG_RX_CLOSE_EDR_EN)
    if (get_broadcast_role() == BROADCAST_ROLE_RECEIVER) {
        return;
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)

#if LEA_CIG_CENTRAL_CLOSE_EDR_CONN
    if ((get_connected_role() & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        return;
    }
#endif

#if LEA_CIG_PERIPHERAL_CLOSE_EDR_CONN
    if ((get_connected_role() & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        return;
    }
#endif

#endif

    lmp_hci_write_scan_enable((conn_enable << 1) | scan_enable);

    if ((scan_enable || conn_enable) && page_list_empty()) {
        int connect_device = bt_get_total_connect_dev();
        app_send_message(APP_MSG_BT_IN_PAIRING_MODE, connect_device);
    }

#if TCFG_DUAL_CONN_PAGE_SCAN_TIME
    if (tws_api_get_role() == TWS_ROLE_SLAVE || (conn_enable && !scan_enable)) {
        if (g_dual_conn.page_scan_timer) {
            sys_timer_modify(g_dual_conn.page_scan_timer,
                             TCFG_DUAL_CONN_PAGE_SCAN_TIME * 1000);
        } else {
            g_dual_conn.page_scan_timer = sys_timeout_add(NULL, auto_close_page_scan,
                                          TCFG_DUAL_CONN_PAGE_SCAN_TIME * 1000);
        }
    } else {
        if (g_dual_conn.page_scan_timer) {
            sys_timeout_del(g_dual_conn.page_scan_timer);
            g_dual_conn.page_scan_timer = 0;
        }
    }
#endif
}

static void close_inquiry_scan(void *p)
{
    g_dual_conn.inquiry_scan_disable = 1;
    if (g_dual_conn.device_num_recorded == 1 && bt_get_total_connect_dev() == 1) {
        write_scan_conn_enable(0, 0);
    }
    u8 data[2] = {0, 0};
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    tws_api_send_data_to_slave(data, 2, 0xF730EBC7);
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    if (g_dual_conn.close_inquiry_scan_timer) {
        sys_timeout_del(g_dual_conn.close_inquiry_scan_timer);
        g_dual_conn.close_inquiry_scan_timer = 0;
    }
#endif
}

static int dual_conn_try_open_inquiry_scan()
{
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    if (g_dual_conn.inquiry_scan_disable) {
        return 0;
    }
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        write_scan_conn_enable(1, 1);
        return 1;
    }
    return 0;
#endif
}

static int add_device_2_page_list(u8 *mac_addr, u32 timeout)
{
    struct page_device_info *info;

    printf("add_device_2_page_list: %d\n", timeout);
    put_buf(mac_addr, 6);

    if (!g_dual_conn.page_head_inited) {
        return 0;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (memcmp(info->mac_addr, mac_addr, 6) == 0) {
            if (info->timer) {
                sys_timeout_del(info->timer);
                info->timer = 0;
            }
            info->timeout = jiffies + msecs_to_jiffies(timeout);

            __list_del_entry(&info->entry);
            list_add_tail(&info->entry, &g_dual_conn.page_head);
            return 1;
        }
    }

    info = malloc(sizeof(*info));
    info->timer = 0;
    info->timeout = jiffies + msecs_to_jiffies(timeout);
    memcpy(info->mac_addr, mac_addr, 6);
    list_add_tail(&info->entry, &g_dual_conn.page_head);
    return 0;
}

static void del_device_from_page_list(u8 *mac_addr)
{
    struct page_device_info *info;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (memcmp(info->mac_addr, mac_addr, 6) == 0) {
            puts("del_device\n");
            put_buf(mac_addr, 6);
            __list_del_entry(&info->entry);
            if (info->timer) {
                sys_timeout_del(info->timer);
            }
            free(info);
            return;
        }
    }

}

static void clr_device_in_page_list()
{
    struct page_device_info *info, *n;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        __list_del_entry(&info->entry);
        if (info->timer) {
            sys_timeout_del(info->timer);
        }
        free(info);
    }
}

static u8 *get_device_addr_in_page_list()
{
    struct page_device_info *info, *n;
    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        return info->mac_addr;
    }
    return NULL;
}


static void tws_wait_pair_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_cancle_wait_pair();
    dual_conn_page_device();
}

static void tws_auto_pair_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_cancle_wait_pair();
    tws_api_cancle_create_connection();
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}

static void tws_pair_new_tws(void *p)
{
    u8 tws_active = 1;

#if (TCFG_LOCAL_TWS_ENABLE == 0 && TCFG_BT_BACKGROUND_ENABLE)
    if (bt_background_active()) {
        tws_active = 0;
    }
#endif
    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }

    tws_api_cancle_create_connection();
    if (tws_active) {
        if (bt_get_total_connect_dev() == 0) {
#if TCFG_BT_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
            tws_api_auto_pair(0);
#else
            tws_api_wait_pair_by_code(0, NULL, 0);
#endif
            g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout, 3000);
        } else {
#if CONFIG_TWS_USE_COMMMON_ADDR == 0
            tws_api_search_sibling_by_code();
            g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout, 3000);
#endif
        }
    }
}

static void tws_wait_conn_timeout(void *p)
{
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}


void tws_dual_conn_state_handler()
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    int state               = tws_api_get_tws_state();
    int connect_device      = bt_get_total_connect_dev();
    int have_page_device    = page_list_empty() ? false : true;
    u8 tws_active = 1;
    u8 edr_background_active = 1;

#if(TCFG_BT_BACKGROUND_ENABLE == 0)
    if (g_bt_hdl.wait_exit) { //非后台正在推出就不处理消息了，防止退出之后资源释放了，流程又打开可发现可连接导致异常
        return;
    }
#endif
    printf("page_state: %d, %x, %d %d %x\n", connect_device, state, have_page_device, g_dual_conn.device_num_recorded, rets);

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }

#if TCFG_BT_BACKGROUND_ENABLE
    if (bt_background_active()) {
#if (TCFG_LOCAL_TWS_ENABLE == 0)
        tws_active = 0;
#endif
#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
        edr_background_active = 0;
#endif
    }
#endif

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }

    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }

    if (state & TWS_STA_SIBLING_CONNECTED) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            write_scan_conn_enable(0, 0);
            return;
        }
        if (connect_device == 0) {
            if (edr_background_active) {
                write_scan_conn_enable(1, 1);
            }
        } else if (connect_device == 1) {
            if (g_dual_conn.device_num_recorded > 1) {
#if TCFG_BT_DUAL_CONN_ENABLE
#if TIMEOUT_CONN_DEVICE_OPEN_PAGE
                if (have_page_device) {
                    r_printf("page_and_page_sacn");
                    if (get_device_addr_in_page_list()) {
                        bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6,
                                       get_device_addr_in_page_list());
                    }
                }
#endif
                write_scan_conn_enable(0, 1);
#endif
            }
        }
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_wait_conn_timeout, 6000);
            tws_api_auto_role_switch_disable();
        } else {
            if (g_dual_conn.device_num_recorded == 1) {
#if TCFG_BT_DUAL_CONN_ENABLE
                dual_conn_try_open_inquiry_scan();
#endif
            }
            tws_api_auto_role_switch_enable();
        }
    } else if (state & TWS_STA_TWS_PAIRED) {
        if (connect_device == 0) {
            if (tws_active) {
                tws_api_create_connection(0);
            }
            if (edr_background_active) {
                write_scan_conn_enable(1, 1);
            }
        } else if (connect_device == 1) {
            if (tws_active) {
                tws_api_wait_connection(0);
            }
            if (g_dual_conn.device_num_recorded > 1) {
#if TCFG_BT_DUAL_CONN_ENABLE
                write_scan_conn_enable(0, 1);
#endif
            } else {
                write_scan_conn_enable(0, 0);
            }
        } else {
            if (tws_active) {
                tws_api_wait_connection(0);
            }
        }

#if CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR
        if (tws_active) {
#if CONFIG_TWS_USE_COMMMON_ADDR         //使用公共地址的情况,连接手机不允许去配对新的设备
            if (connect_device == 0) {
                g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_new_tws,
                                                    TCFG_TWS_CONN_TIMEOUT * 1000);
            }
#else
            g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_new_tws,
                                                TCFG_TWS_CONN_TIMEOUT * 1000);
#endif
            return;
        }
#endif
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
                                                TCFG_TWS_CONN_TIMEOUT * 1000);
        }
    } else {
        /* TWS未配对 */
        if (connect_device == 2) {
            return;
        }
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_CLICK
        const char *bt_name;
        if (g_dual_conn.device_num_recorded > 1) {
            bt_name = NULL;
        } else {
            bt_name = connect_device ? NULL : bt_get_local_name();
        }
#if (TCFG_TWS_PAIR_BY_BOTH_SIDES == 0)        //如果需要两边都按下按键才配对这里不打开wait_pair防止被其他相同pair_code的设备连上
#if (TCFG_TWS_PAIR_ALWAYS == 0)
        if (connect_device > 0) {
            if (g_dual_conn.device_num_recorded > 1) {
#if TCFG_BT_DUAL_CONN_ENABLE
                write_scan_conn_enable(0, 1);
#endif
            } else {
                write_scan_conn_enable(0, 0);
            }
            return;
        }
#endif
        if (tws_active) {
            tws_api_wait_pair_by_code(0, bt_name, 0);
        }
        if (edr_background_active && connect_device == 0) {
            write_scan_conn_enable(1, 1);
        }
        app_send_message(APP_MSG_BT_IN_PAIRING_MODE,  bt_get_total_connect_dev() | (u8)(!page_list_empty()));
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_wait_pair_timeout,
                                                TCFG_TWS_PAIR_TIMEOUT * 1000);
        }
#else
        if ((bt_name || connect_device == 0) && edr_background_active) {
            write_scan_conn_enable(1, 1);
        }
#endif
#elif CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
        if (connect_device == 0) {
            if (tws_active) {
                tws_api_auto_pair(0);
            }
            if (edr_background_active) {
                write_scan_conn_enable(1, 1);
            }
        } else if (connect_device == 1) {
            if (g_dual_conn.device_num_recorded > 1) {
                write_scan_conn_enable(0, 1);
            }
#if TCFG_TWS_PAIR_ALWAYS                     //打开TWS_PAIR_ALWAYS,在手机连接之后仍然可以进行配对
            if (tws_active) {
                tws_api_wait_connection(0);
            }
#endif
        }
        if (have_page_device) {
            g_dual_conn.timer = sys_timeout_add(NULL, tws_auto_pair_timeout,
                                                TCFG_TWS_PAIR_TIMEOUT * 1000);
        }
#endif
    }

}

static void dual_conn_page_device_timeout(void *p)
{
    struct page_device_info *info;

    if (!g_dual_conn.page_head_inited) {
        return;
    }

#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && LEA_BIG_RX_CLOSE_EDR_EN)
    if (get_broadcast_role() == BROADCAST_ROLE_RECEIVER) {
        return;
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)

#if LEA_CIG_CENTRAL_CLOSE_EDR_CONN
    if ((get_connected_role() & CONNECTED_ROLE_CENTRAL) == CONNECTED_ROLE_CENTRAL) {
        return;
    }
#endif

#if LEA_CIG_PERIPHERAL_CLOSE_EDR_CONN
    if ((get_connected_role() & CONNECTED_ROLE_PERIP) == CONNECTED_ROLE_PERIP) {
        return;
    }
#endif

#endif

    /* 参数有效性检查 */
    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        if (info == p) {
            printf("page_device_timeout: %lu, %d\n", jiffies, info->timeout);
            info->timer = 0;
            list_del(&info->entry);
            if (time_after(jiffies, info->timeout)) {
                free(info);
            } else {
                list_add_tail(&info->entry, &g_dual_conn.page_head);
            }
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            page_mode_active = 0;
            tws_dual_conn_state_handler();
            break;
        }
    }
}

void dual_conn_page_device()
{
    struct page_device_info *info, *n;
    u8 edr_background_active = 1;

    if (page_mode_active == 0) {
#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
        if (bt_background_active()) {
            return;
        }
#endif
#if (TCFG_BT_DUAL_CONN_ENABLE == 0)
        if (bt_get_total_connect_dev()) {
            return;
        }
#endif
        if (!g_dual_conn.page_head_inited) {
            return;
        }
        list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
            if (info->timer) {
                return;
            }
            if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                break;
            }
            printf("start_page_device: %lu, %d\n", jiffies, info->timeout);
            put_buf(info->mac_addr, 6);
            info->timer = sys_timeout_add(info, dual_conn_page_device_timeout,
                                          TCFG_BT_PAGE_TIMEOUT * 1000);
            bt_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, info->mac_addr);
            page_mode_active = 1;
            return;
        }
    }

    tws_dual_conn_state_handler();
}


static void dual_conn_page_devices_init()
{
    u8 mac_addr[6];

    INIT_LIST_HEAD(&g_dual_conn.page_head);
    g_dual_conn.page_head_inited = 1;
    g_dual_conn.page_scan_auto_disable = 0;

    int num = btstack_get_num_of_remote_device_recorded();
    for (int i = num - 1; i >= 0 && i + 2 >= num ; i--) {
        btstack_get_remote_addr(mac_addr, i);
        add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
    }
    g_dual_conn.device_num_recorded = num;
    if (num == 1) {
        memcpy(g_dual_conn.remote_addr[2], mac_addr, 6);
    }
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    g_dual_conn.inquiry_scan_disable = 0;
    g_dual_conn.close_inquiry_scan_timer = sys_timeout_add(NULL, close_inquiry_scan, TCFG_DUAL_CONN_INQUIRY_SCAN_TIME * 1000);
#else
    g_dual_conn.inquiry_scan_disable = 1;
#endif

    if (g_bt_hdl.work_mode == BT_MODE_SIGLE_BOX) {
#if (TCFG_LP_NFC_TAG_ENABLE && TCFG_LP_NFC_TAG_TYPE == JL_BT_TAG)
        static u8 nfc_wakeup_disable_page = 1;
        if ((is_reset_source(MSYS_P2M_RST)) && (is_wakeup_source(PWR_WK_REASON_LPNFC)) && nfc_wakeup_disable_page) {
            nfc_wakeup_disable_page = 0;
            write_scan_conn_enable(1, 1);
        } else {            //非nfc唤醒
            dual_conn_page_device();
        }
#else
        dual_conn_page_device();
#endif
    }
}

static void page_next_device(void *p)
{
    g_dual_conn.timer = 0;
    dual_conn_page_device();
}



static int dual_conn_btstack_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;
    int state = tws_api_get_tws_state();
    u8 tws_active = 1;

#if (TCFG_LOCAL_TWS_ENABLE == 0 && TCFG_BT_BACKGROUND_ENABLE)
    if (bt_background_active()) {
        tws_active = 0;
    }
#endif

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }
    switch (event->event) {
    case BT_STATUS_INIT_OK:
        dual_conn_page_devices_init();
        return 0;
    case BT_STATUS_FIRST_CONNECTED:
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        del_device_from_page_list(event->args);
        memcpy(g_dual_conn.remote_addr[0], event->args, 6);
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (!page_list_empty()) {
                g_dual_conn.timer = sys_timeout_add(NULL, page_next_device, 500);
                return 0;
            }
        }
        page_mode_active = 0;
        if (tws_active) {
            if ((state & TWS_STA_TWS_PAIRED) && (state & TWS_STA_SIBLING_DISCONNECTED)) {
                tws_api_wait_connection(0);
#if CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR && (CONFIG_TWS_USE_COMMMON_ADDR == 0)
                g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_new_tws,
                                                    TCFG_TWS_CONN_TIMEOUT * 1000);
#endif
            } else if (state & TWS_STA_SIBLING_CONNECTED) {
                tws_api_auto_role_switch_enable();
            } else {
#if TCFG_TWS_PAIR_ALWAYS && !TCFG_TWS_PAIR_BY_BOTH_SIDES    //打开TWS_PAIR_ALWAYS,在手机连接之后仍然可以进行配对
                tws_api_wait_pair_when_phone_connect(0);        //调用这个接口只打开TWS PageScan
#endif
            }
        }

        if (g_dual_conn.device_num_recorded == 0) {
            g_dual_conn.device_num_recorded++;
            memcpy(g_dual_conn.remote_addr[2], event->args, 6);
#if TCFG_BT_DUAL_CONN_ENABLE
            if (state & TWS_STA_SIBLING_CONNECTED) {
                dual_conn_try_open_inquiry_scan();
            }
#endif
            break;
        }
        if (g_dual_conn.device_num_recorded == 1) {
            if (memcmp(event->args, g_dual_conn.remote_addr[2], 6) == 0) {          //这里判断是否为开机从VM获取的回连设备连接上了
#if TCFG_BT_DUAL_CONN_ENABLE
                if (state & TWS_STA_SIBLING_CONNECTED) {
                    dual_conn_try_open_inquiry_scan();
                }
#endif
                break;
            }
            g_dual_conn.device_num_recorded++;
        }
#if TCFG_BT_DUAL_CONN_ENABLE
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            write_scan_conn_enable(0, 1);
        }
#endif
        break;
    case BT_STATUS_SECOND_CONNECTED:
        if (g_dual_conn.device_num_recorded == 1) {
            g_dual_conn.device_num_recorded++;
        }
        if (g_dual_conn.timer) {
            sys_timeout_del(g_dual_conn.timer);
            g_dual_conn.timer = 0;
        }
        clr_device_in_page_list();
        page_mode_active = 0;
        memcpy(g_dual_conn.remote_addr[1], event->args, 6);

        if (tws_active) {
            if ((state & TWS_STA_TWS_PAIRED) && (state & TWS_STA_SIBLING_DISCONNECTED)) {
                tws_api_wait_connection(0);
            } else {
                tws_api_auto_role_switch_enable();
            }
        }
        break;
    }

    return 0;
}

APP_MSG_HANDLER(dual_conn_stack_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = dual_conn_btstack_event_handler,
};


static int dual_conn_hci_event_handler(int *_event)
{
    struct bt_event *event = (struct bt_event *)_event;

    if (app_var.goto_poweroff_flag) {
        return 0;
    }
    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return 0;
    }

#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT     //后台不支持edr连接，不处理直接返回
    if (g_bt_hdl.wait_exit) {
        return 0;
    }
#endif
    int is_remote_test = bt_get_remote_test_flag();

    switch (event->event) {
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR:
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        if (event->value == ERROR_CODE_CONNECTION_TIMEOUT || event->value == ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED) {
            if (is_remote_test == 0) {
                add_device_2_page_list(event->args, TCFG_BT_TIMEOUT_PAGE_TIME * 1000);
            }
        }
        break;
    case HCI_EVENT_CONNECTION_COMPLETE:
        switch (event->value) {
        case ERROR_CODE_SUCCESS :
            if (g_dual_conn.timer) {
                sys_timeout_del(g_dual_conn.timer);
                g_dual_conn.timer = 0;
            }
            del_device_from_page_list(event->args);
            return 0;
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
            if (!list_empty(&g_dual_conn.page_head)) {
                struct page_device_info *info;
                info = list_first_entry(&g_dual_conn.page_head,
                                        struct page_device_info, entry);
                list_del(&info->entry);
                list_add_tail(&info->entry, &g_dual_conn.page_head);
            }
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            break;
        case ERROR_CODE_ROLE_CHANGE_NOT_ALLOWED:
        case ERROR_CODE_CONNECTION_TIMEOUT:
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            if (is_remote_test == 0) {
                add_device_2_page_list(event->args, TCFG_BT_TIMEOUT_PAGE_TIME * 1000);
            }
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    g_dual_conn.page_scan_auto_disable = 0;
    for (int i = 0; i < 2; i++) {
        if (memcmp(event->args, g_dual_conn.remote_addr[i], 6) == 0) {
            memset(g_dual_conn.remote_addr[i], 0xff, 6);
        }
    }

    tws_dual_conn_state_handler();

    return 0;
}
APP_MSG_HANDLER(dual_conn_hci_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = dual_conn_hci_event_handler,
};

static void rx_device_info(u8 *data, int len)
{
    if (g_dual_conn.inquiry_scan_disable == 0) {
        g_dual_conn.inquiry_scan_disable = data[0] & 0x02;
    }
    g_dual_conn.page_scan_auto_disable = data[0] & 0x01;
    g_dual_conn.device_num_recorded = data[0] >> 2;

    if (g_dual_conn.page_scan_auto_disable) {
        if (g_dual_conn.page_scan_timer) {
            sys_timeout_del(g_dual_conn.page_scan_timer);
            g_dual_conn.page_scan_timer = 0;
        }
    }
    if ((data[1] & 0x01) == 1) {
        clr_device_in_page_list();
        for (int i = 2; i < len; i += 6) {
            add_device_2_page_list(data + i, TCFG_BT_POWERON_PAGE_TIME * 1000);
        }
    }
    free(data);
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        tws_dual_conn_state_handler();
    }
}

static void tws_page_device_info_sync(void *_data, u16 len, bool rx)
{
    if (!rx) {
        return;
    }
    u8 *data = malloc(len);
    memcpy(data, _data, len);
    int msg[4] = { (int)rx_device_info, 2, (int)data, len};
    os_taskq_post_type("app_core", Q_CALLBACK, 4, msg);
}
REGISTER_TWS_FUNC_STUB(tws_dual_conn_stub) = {
    .func_id = 0xF730EBC7,
    .func   = tws_page_device_info_sync,
};

void send_page_device_addr_2_sibling()
{
    u8 data[14];
    int offset = 2;
    struct page_device_info *info, *n;

    if (!g_dual_conn.page_head_inited) {
        return;
    }
    data[0] = g_dual_conn.page_scan_auto_disable |
              (g_dual_conn.inquiry_scan_disable << 1) |
              (g_dual_conn.device_num_recorded << 2);
    data[1] = 1;
    list_for_each_entry_safe(info, n, &g_dual_conn.page_head, entry) {
        memcpy(data + offset, info->mac_addr, 6);
        offset += 6;
    }
    tws_api_send_data_to_sibling(data, offset, 0xF730EBC7);
}


static int dual_conn_tws_event_handler(int *_event)
{
    struct tws_event *event = (struct tws_event *)_event;
    int role = event->args[0];
    int reason = event->args[2];
    int connect_device = bt_get_total_connect_dev();
    u8 tws_active = 1;
    u8 edr_background_active = 1;

    g_printf("%s\n", __func__);
    /*说明已经执行了bt_tws_poweroff,此时如果打开tws可发现可连接重新连上会触发异常*/
    if (!bt_tws_get_state()) {
        /* return 0; */
        tws_active = 0;
    }

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }

#if TCFG_BT_BACKGROUND_ENABLE
    if (bt_background_active()) {
#if (TCFG_LOCAL_TWS_ENABLE == 0)
        tws_active = 0;
#endif

#if TCFG_BACKGROUND_WITHOUT_EDR_CONNECT
        edr_background_active = 0;
#endif
    }
#endif

    if (edr_background_active == 0) {
        switch (event->event) {
        case TWS_EVENT_CONNECTED:
            break;
        case TWS_EVENT_CONNECTION_DETACH:
            if (tws_active) {
                tws_api_wait_connection(0);
            }
            break;
        }
    } else {
        switch (event->event) {
        case TWS_EVENT_CONNECTED:
            if (g_dual_conn.timer) {
                sys_timeout_del(g_dual_conn.timer);
                g_dual_conn.timer = 0;
            }
            if (role == TWS_ROLE_MASTER) {
                tws_api_auto_role_switch_disable();
                if (!page_list_empty()) {
                    dual_conn_page_device();
                } else {
                    tws_dual_conn_state_handler();
                }
                send_page_device_addr_2_sibling();
            } else {
                write_scan_conn_enable(0, 0);
                bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            }
            break;
        case TWS_EVENT_CONNECTION_DETACH:
            if (g_dual_conn.timer) {
                sys_timeout_del(g_dual_conn.timer);
                g_dual_conn.timer = 0;
            }
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);

            if (app_var.goto_poweroff_flag) {
                break;
            }

            if (reason & TWS_DETACH_BY_REMOVE_PAIRS) {
                tws_dual_conn_state_handler();

                break;
            }
            if (reason & TWS_DETACH_BY_TESTBOX_CON) {
                write_scan_conn_enable(1, 1);
                break;
            }
            if ((reason & TWS_DETACH_BY_POWEROFF) && !(reason & TWS_DETACH_BY_LOCAL)) {
                tws_dual_conn_state_handler();
                break;
            }
            if (reason & TWS_DETACH_BY_SUPER_TIMEOUT) {
                if (role == TWS_ROLE_SLAVE) {
                    clr_device_in_page_list();
                }
                tws_dual_conn_state_handler();
                break;
            }
            if (!page_list_empty()) {
                dual_conn_page_device();
            } else {
                if (tws_active) {
                    tws_api_wait_connection(0);
                }
                if (connect_device == 0) {
                    write_scan_conn_enable(1, 1);
                } else if (connect_device == 1) {
                    if (g_dual_conn.device_num_recorded > 1) {
#if TCFG_BT_DUAL_CONN_ENABLE
                        write_scan_conn_enable(0, 1);
#endif
                    } else {
                        write_scan_conn_enable(0, 0);
                    }
                }
            }
            break;
        case TWS_EVENT_ROLE_SWITCH:
            if (role == TWS_ROLE_SLAVE) {
                write_scan_conn_enable(0, 0);
                bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
                if (app_var.goto_poweroff_flag == 0) {
                    send_page_device_addr_2_sibling();
                }
            }
            break;
        }
    }

    return 0;
}

APP_MSG_HANDLER(dual_conn_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = dual_conn_tws_event_handler,
};



static void page_device_msg_handler()
{
    u8 mac_addr[6];
    struct page_device_info *info;
    int device_num = bt_get_total_connect_dev();

    if (!g_dual_conn.page_head_inited) {
        return;
    }

    list_for_each_entry(info, &g_dual_conn.page_head, entry) {
        device_num++;
    }
    if (device_num >= 2) {
        return;
    }

    int num = btstack_get_num_of_remote_device_recorded();
    for (int i = num - 1; i >= 0; i--) {
        btstack_get_remote_addr(mac_addr, i);
        if (memcmp(mac_addr, g_dual_conn.remote_addr[0], 6) == 0) {
            continue;
        }
        if (memcmp(mac_addr, g_dual_conn.remote_addr[1], 6) == 0) {
            continue;
        }
        int ret = add_device_2_page_list(mac_addr, TCFG_BT_POWERON_PAGE_TIME * 1000);
        if (ret == 0) {
            if (++device_num >= 2) {
                break;
            }
        }
    }
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
    tws_api_cancle_create_connection();
    tws_api_cancle_wait_pair();
    write_scan_conn_enable(0, 0);
    dual_conn_page_device();
}



/*
 * 设置自动回连的识别码 6个byte
 * */
static u8 auto_pair_code[6] = {0x34, 0x66, 0x33, 0x87, 0x09, 0x42};

static u8 *tws_set_auto_pair_code(void)
{
    u16 code = bt_get_tws_device_indicate(NULL);
    auto_pair_code[0] = code >> 8;
    auto_pair_code[1] = code & 0xff;
    return auto_pair_code;
}

static void tws_pair_timeout(void *p)
{
    u8 tws_active = 1;

#if (TCFG_LOCAL_TWS_ENABLE == 0 && TCFG_BT_BACKGROUND_ENABLE)
    if (bt_background_active()) {
        tws_active = 0;
    }
#endif

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }
    r_printf("tws_pair_timeout\n");
    g_dual_conn.timer = 0;

    tws_api_cancle_create_connection();

    if (!page_list_empty()) {
        dual_conn_page_device();
    } else {
        if (tws_active) {
            tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
        } else {
            write_scan_conn_enable(1, 1);
        }
        app_send_message(APP_MSG_BT_IN_PAIRING_MODE,  bt_get_total_connect_dev() | (u8)(!page_list_empty()));
    }
    app_send_message(APP_MSG_TWS_POWERON_PAIR_TIMEOUT, 0);
}

static void tws_create_conn_timeout(void *p)
{
    g_dual_conn.timer = 0;
    tws_api_cancle_create_connection();

    if (!page_list_empty()) {
        dual_conn_page_device();
    } else {
        tws_dual_conn_state_handler();
    }
    app_send_message(APP_MSG_TWS_POWERON_CONN_TIMEOUT, 0);
}

static void msg_tws_start_pair_timeout(void *p)
{
#if TCFG_TWS_PAIR_BY_BOTH_SIDES
    bt_set_pair_code_en(0);
#endif

    tws_api_cancel_search_sibling();
    tws_set_search_sbiling_state(0);

    if (!page_list_empty()) {
        dual_conn_page_device();
    } else {
        tws_dual_conn_state_handler();
    }
    app_send_message(APP_MSG_TWS_START_PAIR_TIMEOUT, 0);
}

static void msg_tws_start_conn_timeout(void *p)
{
    app_send_message(APP_MSG_TWS_START_CONN_TIMEOUT, 0);
}

static int dual_conn_app_event_handler(int *msg)
{
    u8 tws_active = 1;

#if (TCFG_LOCAL_TWS_ENABLE == 0 && TCFG_BT_BACKGROUND_ENABLE)
    if (bt_background_active()) {
        tws_active = 0;
    }
#elif TCFG_BT_BACKGROUND_ENABLE == 0
    if (g_bt_hdl.wait_exit) {       //处理非后台已经退出蓝牙模式还响应tws消息导致打开tws_page/pagescan触发异常
        return 0;
    }
#endif

    if (g_bt_hdl.work_mode != BT_MODE_TWS) {
        tws_active = 0;
    }

    if (tws_active == 0) {
        switch (msg[0]) {
        case APP_MSG_TWS_PAIRED:
        case APP_MSG_TWS_UNPAIRED:
            /* 未配对, 等待发起配对 */
            if (!list_empty(&g_dual_conn.page_head)) {
                dual_conn_page_device();
            } else {
                write_scan_conn_enable(1, 1);
            }
            break;
        default:
            break;
        }
    } else {
        switch (msg[0]) {
        case APP_MSG_TWS_PAIRED:
            if (bt_get_total_connect_dev() == 0) {
                tws_api_create_connection(0);
                g_dual_conn.timer = sys_timeout_add(NULL, tws_create_conn_timeout,
                                                    TCFG_TWS_CONN_TIMEOUT * 1000);
            } else {
                tws_api_wait_connection(0);
            }
            break;
        case APP_MSG_TWS_UNPAIRED:
#if CONFIG_TWS_PAIR_MODE == CONFIG_TWS_PAIR_BY_AUTO
            /* 未配对, 开始自动配对 */
#if TCFG_TWS_PAIR_ALWAYS
            u8 tws_can_pair = 1;
#else
            u8 tws_can_pair = !bt_get_total_connect_dev();
#endif
            if (tws_can_pair) {
                if (bt_get_total_connect_dev() == 0) {
                    tws_api_set_quick_connect_addr(tws_set_auto_pair_code());
                    tws_api_auto_pair(0);
                } else {
                    tws_api_wait_pair_when_phone_connect(0);
                }
                g_dual_conn.timer = sys_timeout_add(NULL, tws_pair_timeout,
                                                    TCFG_TWS_PAIR_TIMEOUT  * 1000);
            }
#else
            /* 未配对, 等待发起配对 */
            if (!list_empty(&g_dual_conn.page_head)) {
                dual_conn_page_device();
            } else {
#if (TCFG_TWS_PAIR_BY_BOTH_SIDES == 0)       //如果需要两边都按下按键才配对这里不打开wait_pair防止被其他相同pair_code的设备连上
                tws_api_wait_pair_by_code(0, bt_get_local_name(), 0);
                app_send_message(APP_MSG_BT_IN_PAIRING_MODE,  bt_get_total_connect_dev() | !page_list_empty());
#else
                write_scan_conn_enable(1, 1);
#endif
            }
#endif
            break;
        case APP_MSG_BT_PAGE_DEVICE:
            clr_device_in_page_list();
            dual_conn_page_devices_init();
            page_device_msg_handler();
            break;
        case APP_MSG_TWS_START_PAIR_AND_CONN_PROCESS:
            if (bt_tws_is_paired()) {
                app_send_message(APP_MSG_TWS_PAIRED, 0);
            } else {
                app_send_message(APP_MSG_TWS_UNPAIRED, 0);
            }
            break;
        case APP_MSG_TWS_WAIT_PAIR:
            tws_api_wait_pair_by_code(0, NULL, 0);
            break;
        case APP_MSG_TWS_START_PAIR:
            if ((bt_get_call_status() == BT_CALL_OUTGOING)
                || (bt_get_call_status() == BT_CALL_ALERT)
                || (bt_get_call_status() == BT_CALL_INCOMING)
                || (bt_get_call_status() == BT_CALL_ACTIVE)
               ) {
                // 通话不进行配对和取消配对操作
                break;
            }
            if (bt_tws_start_pair() == 0) {
                g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_start_pair_timeout,
                                                    TCFG_TWS_PAIR_TIMEOUT * 1000);
            }
            break;
        case APP_MSG_TWS_REMOVE_PAIR:
            if ((bt_get_call_status() == BT_CALL_OUTGOING)
                || (bt_get_call_status() == BT_CALL_ALERT)
                || (bt_get_call_status() == BT_CALL_INCOMING)
                || (bt_get_call_status() == BT_CALL_ACTIVE)
               ) {
                // 通话不进行配对和取消配对操作
                break;
            }
            bt_tws_detach_and_remove_pairs();
            break;

        case APP_MSG_TWS_START_REMOVE_PAIR:
            if ((bt_get_call_status() == BT_CALL_OUTGOING)
                || (bt_get_call_status() == BT_CALL_ALERT)
                || (bt_get_call_status() == BT_CALL_INCOMING)
                || (bt_get_call_status() == BT_CALL_ACTIVE)
               ) {
                // 通话不进行配对和取消配对操作
                break;
            }
            if (msg[1] == APP_KEY_MSG_FROM_TWS) {
                break;
            }
            printf("tws_api_get_tws_state:%x\n", tws_api_get_tws_state());
            if ((tws_api_get_tws_state() & TWS_STA_TWS_PAIRED) && (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
                bt_tws_detach_and_remove_pairs();
            } else {        //没有连接也要发起配对
                if (bt_tws_start_pair() == 0) {
                    g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_start_pair_timeout,
                                                        TCFG_TWS_PAIR_TIMEOUT * 1000);
                }
            }
            break;

        case APP_MSG_TWS_WAIT_CONN:
            tws_api_wait_connection(0);
            break;
        case APP_MSG_TWS_START_CONN:
            tws_api_create_connection(0);
            g_dual_conn.timer = sys_timeout_add(NULL, msg_tws_start_conn_timeout,
                                                TCFG_TWS_CONN_TIMEOUT * 1000);
            break;
        }
    }

    return 0;
}

APP_MSG_HANDLER(dual_conn_app_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_APP,
    .handler    = dual_conn_app_event_handler,
};


void tws_dual_conn_close()
{
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
#if TCFG_DUAL_CONN_INQUIRY_SCAN_TIME
    if (g_dual_conn.close_inquiry_scan_timer) {
        sys_timeout_del(g_dual_conn.close_inquiry_scan_timer);
        g_dual_conn.close_inquiry_scan_timer = 0;
    }
#endif
    clr_device_in_page_list();
    tws_api_cancle_create_connection();
    tws_api_cancle_wait_pair();
    write_scan_conn_enable(0, 0);
    bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
}

bool check_page_mode_active(void)
{
    return (page_mode_active) ? TRUE : FALSE;
}

void tws_delete_pair_timer(void)
{
    if (g_dual_conn.timer) {
        sys_timeout_del(g_dual_conn.timer);
        g_dual_conn.timer = 0;
    }
}

#endif
