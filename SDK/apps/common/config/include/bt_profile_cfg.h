
#ifndef _BT_PROFILE_CFG_H_
#define _BT_PROFILE_CFG_H_

#include "app_config.h"
#include "btcontroller_modules.h"


#if ((THIRD_PARTY_PROTOCOLS_SEL & TRANS_DATA_EN) || (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN) || (THIRD_PARTY_PROTOCOLS_SEL & ANCS_CLIENT_EN) || (THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN) || (THIRD_PARTY_PROTOCOLS_SEL & TUYA_DEMO_EN))
#ifndef BT_FOR_APP_EN
#define    BT_FOR_APP_EN             1
#endif
#else
#ifndef BT_FOR_APP_EN
#define    BT_FOR_APP_EN             0
#endif
// #ifndef AI_APP_PROTOCOL
// #define    AI_APP_PROTOCOL             0
// #endif
#endif


///---sdp service record profile- 用户选择支持协议--///
#if (BT_FOR_APP_EN \
    || APP_ONLINE_DEBUG \
    || (THIRD_PARTY_PROTOCOLS_SEL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN | MMA_EN | FMNA_EN | ONLINE_DEBUG_EN | CUSTOM_DEMO_EN)))
#if ((THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN) || (THIRD_PARTY_PROTOCOLS_SEL & TUYA_DEMO_EN))
#undef TCFG_BT_SUPPORT_SPP
#define TCFG_BT_SUPPORT_SPP    0
#else
#undef TCFG_BT_SUPPORT_SPP
#define TCFG_BT_SUPPORT_SPP    1
#endif
#endif

//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0 //ble 没有使能
#define DEF_BLE_DEMO_ADV                  1 //only adv,can't connect
#define DEF_BLE_DEMO_TRANS_DATA           2 //
#define DEF_BLE_DEMO_RCSP_DEMO            4 //
#define DEF_BLE_DEMO_ADV_RCSP             5
#define DEF_BLE_DEMO_CLIENT               7 //
#define DEF_BLE_ANCS_ADV				  9
#define DEF_BLE_DEMO_MULTI                11 //
#define DEF_BLE_DEMO_LL_SYNC              13 //
#define DEF_BLE_DEMO_WIRELESS_MIC_SERVER  14 //
#define DEF_BLE_DEMO_WIRELESS_MIC_CLIENT  15 //
#define DEF_BLE_DEMO_TUYA                 16 //
#define DEF_BLE_WL_MIC_1T1_TX             17
#define DEF_BLE_WL_MIC_1T1_RX             18
#define DEF_BLE_WL_MIC_1TN_TX             19
#define DEF_BLE_WL_MIC_1TN_RX             20
#define DEF_LE_AUDIO_CENTRAL              21
#define DEF_LE_AUDIO_PERIPHERAL           22
#define DEF_LE_AUDIO_BROADCASTER          23

//delete 2021-09-24;删除公共配置，放到各个profile自己配置
// #define TCFG_BLE_SECURITY_EN          0 /*是否发请求加密命令*/



#endif
