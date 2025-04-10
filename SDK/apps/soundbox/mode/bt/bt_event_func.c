#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_event_func.data.bss")
#pragma data_seg(".bt_event_func.data")
#pragma const_seg(".bt_event_func.text.const")
#pragma code_seg(".bt_event_func.text")
#endif
#include "classic/hci_lmp.h"
#include "btstack/btstack_task.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "app_testbox.h"
#include "tone_player.h"
#include "clock_manager/clock_manager.h"
#include "audio_config.h"
#include "app_le_auracast.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/
#include "battery_manager.h"
#include "bt_common.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif
#include "bt_event_func.h"
#include "dual_conn.h"

/* --------------------------------------------------------------------------*/
/**
 * @brief 修复编译问题
 *
 * @param
 */
/* ----------------------------------------------------------------------------*/
__attribute__((weak))
void bt_ble_init(void)
{
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 修复编译问题
 *
 * @param
 */
/* ----------------------------------------------------------------------------*/
__attribute__((weak))
void bt_ble_exit(void)
{
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙连接配置，提供app层用户可以输入配对鉴定key
 *
 *  \param      key :配对需要输入的数字
 *
 *  \return
 *
 *  \note       配对需要输入6位数字的时候，按照顺序从左到右一个个输入
 */
/*************************************************************************************************/
void bt_send_keypress(u8 key)
{
    printf("bt_send_keypress:%d", key);
    bt_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙连接配置，提供app层用户可以输入确定或者否定
 *
 *  \param      en 0:否定   1:确定
 *
 *  \return
 *
 *  \note       在连接过程中类似手机弹出 确定和否定 按键，可以供用户界面设置
 */
/*************************************************************************************************/
void bt_send_pair(u8 en)
{
    bt_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙获取vm连接记录信息
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_init_ok_search_index(void)
{
    if (bt_get_current_poweron_memory_search_index(g_bt_hdl.auto_connection_addr)) {
        printf("bt_wait_connect_and_phone_connect_switch\n");
        bt_clear_current_poweron_memory_search_index(1);
        app_send_message(APP_MSG_BT_GET_CONNECT_ADDR, 0);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙初始化完成
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_status_init_ok(void)
{
    g_bt_hdl.init_ok = 1;

    bt_init_ok_search_index();
#if TCFG_NORMAL_SET_DUT_MODE
    printf("edr set dut mode\n");
    bredr_set_dut_enble(1, 1);
#if TCFG_USER_BLE_ENABLE
    printf("ble set dut mode\n");
    ble_standard_dut_test_init();
    return;
#endif
#endif

#if (TCFG_USER_BLE_ENABLE || TCFG_BT_BLE_ADV_ENABLE)
    if (BT_MODE_IS(BT_BQB)) {
        ble_bqb_test_thread_init();
    } else if (BT_MODE_IS(BT_NORMAL)) {
        bt_ble_init();
    }
#endif

#if ((CONFIG_BT_MODE == BT_BQB)||(CONFIG_BT_MODE == BT_PER))
    return;
#endif

    le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER);

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    if (g_bt_hdl.work_mode == BT_MODE_BROADCAST) {
        app_broadcast_init();
#if TCFG_KBOX_1T3_MODE_EN
        app_broadcast_open();
#endif
    }
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (g_bt_hdl.work_mode == BT_MODE_CIG) {
        app_connected_init();
#if TCFG_KBOX_1T3_MODE_EN
        if (is_open_cis_connet()) {
            app_connected_open(0);
        }
#endif
    }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN))
    app_auracast_init();
#endif


#if TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END
    if (tone_player_runing()) {
        return;
    }
#endif
#if TCFG_USER_TWS_ENABLE
    if (g_bt_hdl.work_mode == BT_MODE_TWS) {
        bt_tws_poweron();
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取在连接设备名字回调
   @param    status : 1：获取失败   0：获取成功
			 addr   : 配对设备地址
			 name   :配对设备名字
   @return
   @note     需要连接上设备后发起USER_CTRL_READ_REMOTE_NAME
   			 命令来
*/
/*----------------------------------------------------------------------------*/
void bt_read_remote_name(u8 status, u8 *addr, u8 *name)
{
    if (status) {
        printf("remote_name fail \n");
    } else {
        printf("remote_name : %s \n", name);
    }
    put_buf(addr, 6);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙歌曲信息获取回调
   @param
   @return
   @note
   const u8 more_avctp_cmd_support = 1;置上1
   需要在void bredr_handle_register()注册回调函数
   要动态获取播放时间的，可以发送USER_CTRL_AVCTP_OPID_GET_PLAY_TIME命令就可以了
   要半秒或者1秒获取就做个定时发这个命令
*/
/*----------------------------------------------------------------------------*/
#include "math.h"
static void ms_to_time(u8 *info, u16 len)
{
    //毫秒转换成分:秒
    u32 time = 0;
    u16 cnt;
    for (cnt = 0; cnt < len; cnt ++) {
        time += (info[len - 1 - cnt] - '0') * pow(10, cnt);
    }
    printf("music_time: %02d : %02d", time / 1000 / 60, (time % 60000) / 1000);
}
void user_get_bt_music_info(u8 type, u32 time, u8 *info, u16 len)
{
    //profile define type:
    //1-title 2-artist name 3-album names 4-track number
    //5-total number of tracks 6-genre  7-playing time
    //JL define 0x10-total time , 0x11 current play position
    u8  min, sec;
    //printf("type %d\n", type );
    if ((info != NULL) && (len != 0) && (type != 7)) {
        printf(" %s \n", info);
    }

    if (type == 7) {
        ms_to_time(info, len);
    }
    if (time != 0) {
        min = time / 1000 / 60;
        sec = time / 1000 - (min * 60);
        printf(" time %02d : %02d\n ", min, sec);
    }
}

static void bt_music_player_time_deal(void *priv)
{
    if (BT_STATUS_PLAYING_MUSIC == bt_get_connect_status()) {
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_PLAY_TIME, 0, NULL);
    }
}
//播歌时配置为1
void bt_music_player_time_timer_deal(u8 en)
{
#if TCFG_BT_MUSIC_INFO_ENABLE
    if (en) {
        if (g_bt_hdl.get_music_player_timer == 0) {
            g_bt_hdl.get_music_player_timer  = sys_timer_add(NULL, bt_music_player_time_deal, 800);
        }
    } else {
        if (g_bt_hdl.get_music_player_timer) {
            sys_timer_del(g_bt_hdl.get_music_player_timer);
            g_bt_hdl.get_music_player_timer = 0;
        }
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙spp 协议数据 回调
   @param    packet_type:数据类型
  			 ch         :区分spp链路的连接号
			 packet     :数据缓存
			size        ：数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size)
{
    switch (packet_type) {
    case 1:
        printf("---spp connect:%x\n", ch);
        break;
    case 2:
        printf("---spp disconnect:%x\n", ch);
        break;
    case 7:
        //puts("spp_rx:");
        //put_buf(packet,size);
#if AEC_DEBUG_ONLINE
        aec_debug_online(packet, size);
#endif

#if TCFG_USER_RSSI_TEST_EN
        int spp_get_rssi_handler(u8 * packet, u16 size);
        spp_get_rssi_handler(packet, size);
#endif
        break;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙获取样机当前电量
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_get_battery_value()
{
    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    /*电量协议的是0-9个等级，请比例换算*/
    return get_cur_battery_level();
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙快速测试
   @param
   @return
   @note     样机和蓝牙测试盒链接开启快速测试，开启mic扩音功能，
   			 按键就播放按键音来检测硬件是否焊接正常
*/
/*----------------------------------------------------------------------------*/
void bt_fast_test_api(void)
{
    /*
     * 进入快速测试模式，用户根据此标志判断测试，
     * 如测试按键-开按键音  、测试mic-开扩音 、
     * 根据fast_test_mode根据改变led闪烁方式、关闭可发现可连接
     */
    puts("------------bt_fast_test_api---------\n");
    if (g_bt_hdl.fast_test_mode == 0) {
        g_bt_hdl.fast_test_mode = 1;
        audio_fast_mode_test();
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式样机被测试仪链接上的回调函数，把其他状态关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_dut_api(u8 param)
{
    printf("bt in dut \n");
    sys_auto_shut_down_disable();
#if TCFG_USER_TWS_ENABLE
    tws_cancle_all_noconn() ;
#endif

    if (g_bt_hdl.auto_connection_timer) {
        sys_timeout_del(g_bt_hdl.auto_connection_timer);
        g_bt_hdl.auto_connection_timer = 0;
    }

#if ((TCFG_USER_BLE_ENABLE && TCFG_THIRD_PARTY_PROTOCOLS_ENABLE) || TCFG_BT_BLE_ADV_ENABLE)
#if (CONFIG_BT_MODE == BT_NORMAL)
    bt_ble_adv_enable(0);
#endif
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式进入定频状态
   @param
   @return
   @note     量产的时候可以通过按键等来触发进入定频状态，这时候蓝牙会在一个通道里
   			 发送信号,可以通过设置下面变量来设置定频的频点
		  	const int config_bredr_fcc_fix_fre = 0;
*/
/*----------------------------------------------------------------------------*/
void bt_fix_fre_api(u8 fre)
{
    bt_dut_api(0);

    bit_clr_ie(IRQ_BREDR_IDX);
    bit_clr_ie(IRQ_BT_CLKN_IDX);

    bredr_fcc_init(BT_FRE, fre);
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式进入定频测试接收发射
   @param      mode ：0 测试发射    1：测试接收
   			  mac_addr:测试设置的地址
			  fre：定频的频点   0=2402  1=2403
			  packet_type：数据包类型

				#define DH1_1        0
				#define DH3_1        1
				#define DH5_1        2
				#define DH1_2        3
				#define DH3_2        4
				#define DH5_2        5

			  payload：数据包内容  0x0000  0x0055 0x00aa 0x00ff
						0xffff:prbs9
			 payload_len：数据包长度，不可以超过包类型最大长度，0：底层按照最大包发送
			 pwr： 发送功率
   @return
   @note     量产的时候通过串口，发送设置的参数，设置发送接收的参数

   关闭定频接收发射测试
	void link_fix_txrx_disable();

	更新接收结果
	void bt_updata_fix_rx_result()

struct link_fix_rx_result {
    u32 rx_err_b;  //接收到err bit
    u32 rx_sum_b;  //接收到正确bit
    u32 rx_perr_p;  //接收到crc 错误 包数
    u32 rx_herr_p;  //接收到crc 以外其他错误包数
    u32 rx_invail_p; //接收到crc错误bit太多的包数，丢弃不统计到err bit中
};*/
/*----------------------------------------------------------------------------*/
void bt_fix_txrx_api(u8 mode, u8 *mac_addr, u8 fre, u8 packet_type, u16 payload)
{
    bt_dut_api(0);
    local_irq_disable();
    link_fix_txrx_disable();
    if (mode) {
        link_fix_rx_enable(mac_addr, fre, packet_type, 0xffff, 0, 9);
    } else {
        link_fix_tx_enable(mac_addr, fre, packet_type, 0xffff, 0, 9);
    }
    local_irq_enable();
}

void bt_updata_fix_rx_result()
{
    struct link_fix_rx_result fix_rx_result;
    link_fix_rx_update_result(&fix_rx_result);
    printf("err_b:%d sum_b:%d perr:%d herr_b:%d invaile:%d  \n",
           fix_rx_result.rx_err_b,
           fix_rx_result.rx_sum_b,
           fix_rx_result.rx_perr_p,
           fix_rx_result.rx_herr_p,
           fix_rx_result.rx_invail_p
          );
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   退出dut模式
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_bredr_exit_dut_mode()
{
    bredr_set_dut_enble(0, 1);
    clock_free("DUT");
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   搜索结束
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_inquiry(struct bt_event *bt)
{
#if TCFG_USER_EMITTER_ENABLE
    if (g_bt_hdl.emitter_or_receiver == BT_EMITTER_EN) {
        //以后扩展，暂时注释
        //emitter_search_stop();
    }
#endif
}


void bt_discovery_and_connectable_using_loca_mac_addr(u8 inquiry_scan_en, u8 page_scan_en)
{
    u8 local_addr[6];

    bt_get_vm_mac_addr(local_addr);
    lmp_hci_write_local_address(local_addr);
    if (page_scan_en) {
        bt_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    }
    if (inquiry_scan_en) {
        bt_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event链接断开,实际流程处理位于dual_conn.c中，此处预留做状态更新
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_disconnect(struct bt_event *bt)
{
    if (app_var.goto_poweroff_flag) {
        return;
    }

    printf("<<<<<<<<<<<<<<total_dev: %d>>>>>>>>>>>>>\n", bt_get_total_connect_dev());

#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_ex_enter_dut_flag()) {
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        return;
    }

    if (testbox_get_status()) {
        bt_discovery_and_connectable_using_loca_mac_addr(0, 1);
        return;
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   链接超时
   @param
   @return
   @note    回链超时内没有连接上设备
*/
/*----------------------------------------------------------------------------*/
void bt_hci_event_connection_timeout(struct bt_event *bt)
{
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_ex_enter_dut_flag()) {
        bt_discovery_and_connectable_using_loca_mac_addr(1, 1);
        return;
    }
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief  蓝牙event   获取sco状态
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_sco_state(void)
{
    return g_bt_hdl.phone_call_dec_begin;
}

/*----------------------------------------------------------------------------*/
/**@brief  蓝牙weak函数重新定义，蓝牙获取到的电话本信息反馈给用户层
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_phonebook_packet_handler(u8 type, const u8 *name, const u8 *number, const u8 *date)
{
    static u16 number_cnt = 0;
    printf("NO.%d:", number_cnt);
    number_cnt++;
    printf("type:%d ", type);
    if (type == 0xff) {
        number_cnt = 0;
    }
    if (name) {
        printf(" NAME:%s  ", name);
    }
    if (number) {
        printf("number:%s  ", number);
    }
    if (date) {
        printf("date:%s ", date);
    }
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 断开连接并关闭可发现可连接
 */
/* ----------------------------------------------------------------------------*/
void bt_close_discoverable_and_connectable(void)
{
    /*调用此接口删除page定时器*/
#if TCFG_USER_TWS_ENABLE
    tws_dual_conn_close();
#else
    dual_conn_close();
#endif

    extern u8 hci_standard_connect_check(void);
    if (bt_get_curr_channel_state() != 0) {
        bt_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    } else {
        if (hci_standard_connect_check()) {
            bt_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            bt_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        }
    }
    /* if there are some connected channel ,then disconnect*/
    while (hci_standard_connect_check() != 0) {
        //wait disconnect;
        os_time_dly(2);
    }

    //关闭可发现可链接
    bt_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    bt_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
}

