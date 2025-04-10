#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_btstack_config.data.bss")
#pragma data_seg(".lib_btstack_config.data")
#pragma const_seg(".lib_btstack_config.text.const")
#pragma code_seg(".lib_btstack_config.text")
#endif
/*********************************************************************************************
    *   Filename        : btstack_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-16 11:49

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "btcontroller_config.h"
#include "bt_common.h"

/**
 * @brief Bluetooth Stack Module
 */

// *INDENT-OFF*
#ifdef CONFIG_SOUNDBOX_FLASH_256K
	const int CONFIG_BTSTACK_BIG_FLASH_ENABLE     = 0;
#else
	const int CONFIG_BTSTACK_BIG_FLASH_ENABLE     = 1;
#endif


#if TCFG_BT_SUPPORT_AAC
	const int CONFIG_BTSTACK_SUPPORT_AAC    = 1;
#else
	const int CONFIG_BTSTACK_SUPPORT_AAC    = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LDAC)
	const int CONFIG_BTSTACK_SUPPORT_LDAC       = TCFG_BT_SUPPORT_LDAC;
#else
	const int CONFIG_BTSTACK_SUPPORT_LDAC       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC)
	const int CONFIG_BTSTACK_SUPPORT_LHDC       = TCFG_BT_SUPPORT_LHDC;
#else
	const int CONFIG_BTSTACK_SUPPORT_LHDC       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_APTX)
	const int CONFIG_BTSTACK_SUPPORT_APTX       = TCFG_BT_SUPPORT_APTX;
#else
	const int CONFIG_BTSTACK_SUPPORT_APTX       = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LHDC_V5)
	const int CONFIG_BTSTACK_SUPPORT_LHDC_V5    = TCFG_BT_SUPPORT_LHDC_V5;
#else
	const int CONFIG_BTSTACK_SUPPORT_LHDC_V5    = 0;
#endif

#if defined(TCFG_BT_SUPPORT_LC3)
	const int CONFIG_BTSTACK_SUPPORT_LC3        = TCFG_BT_SUPPORT_LC3;
#else
	const int CONFIG_BTSTACK_SUPPORT_LC3        = 0;
#endif

//协议栈接收到命令是否自动退出sniff
const int config_btstask_auto_exit_sniff = 1;

#if TCFG_TWS_AUDIO_SHARE_ENABLE
const int CONFIG_BTSTACK_TWS_AUDIO_SHARE_ENABLE  = 1;
#else
const int CONFIG_BTSTACK_TWS_AUDIO_SHARE_ENABLE  = 0;
#endif


#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN | LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN)))
	const int CONFIG_BTSTACK_LE_AUDIO_ENABLE     = 1;
#else
	const int CONFIG_BTSTACK_LE_AUDIO_ENABLE     = 0;
#endif

#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
	const int config_le_sm_sub_sc_bridge_edr_enable = 1;
	const int config_le_sm_sub_sc_enable = 1;
#else
   #if TCFG_BLE_BRIDGE_EDR_ENALBE
   		const int config_le_sm_sub_sc_enable = 1;            /*SC加密模式使能,need config_le_sm_support_enable = 1*/
   		const int config_le_sm_sub_sc_bridge_edr_enable = 1; /*SC加密模式下,ios app ble配对快连edr,need config_le_sm_sub_sc_enable = 1*/
   #else
   		const int config_le_sm_sub_sc_enable = 0;
   		const int config_le_sm_sub_sc_bridge_edr_enable = 0;
   #endif
#endif

const int config_product_test_mode = 0;
const char config_product_test_rssi = 50;
const char config_product_adv_pair_rssi = 0;
const char config_product_con_pair_rssi = 0;

#if RCSP_MODE
int app_info_debug_enable = 0;//BIT(4);
/* #define RCSP_API_LOG		0 */
/* #define RCSP_PACKET_LOG		1 */
/* #define RCSP_PROTOCOL_LOG	2 */
/* #define SPP_USER_LOG		3 */
/* #define RCSP_INTERFACE_LOG	4 */
#endif

//le 配置,可以优化代码和RAM
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
	const int config_le_hci_connection_num = 2;//支持同时连接个数
	const int config_le_sm_support_enable = 1; //是否支持加密配对
#elif (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
	const int config_le_hci_connection_num = 2;//支持同时连接个数
	const int config_le_sm_support_enable = 0; //是否支持加密配对
#else
	const int config_le_hci_connection_num = 2;//支持同时连接个数
	const int config_le_sm_support_enable = 1; //是否支持加密配对
#endif

const int config_le_gatt_server_num = 2;   //支持server角色个数
const int config_le_gatt_client_num = 1;   //支持client角色个数

#ifdef CONFIG_CPU_BR29
#if TCFG_APP_FM_EN      //br29打开fm和bstack结构体进行overlay
const int const_btstack_fm_overlay = 1;
#else
const int const_btstack_fm_overlay = 0;
#endif
#endif

// *INDENT-ON*

const char log_tag_const_v_APP_BLE = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_APP_BLE = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_APP_BLE = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_APP_BLE = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_APP_BLE = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_APP_SPP = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_APP_SPP = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_APP_SPP = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_APP_SPP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_APP_SPP = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_SPP = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_SPP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_SPP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_SPP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_SPP = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_AVCTP = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AVCTP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_AVCTP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_AVCTP = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_AVCTP = CONFIG_DEBUG_LIB(1);

const char log_tag_const_v_LEA = CONFIG_DEBUG_LIB(1);
const char log_tag_const_i_LEA = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_LEA = CONFIG_DEBUG_LIB(1);
const char log_tag_const_w_LEA = CONFIG_DEBUG_LIB(1);
const char log_tag_const_e_LEA = CONFIG_DEBUG_LIB(1);

