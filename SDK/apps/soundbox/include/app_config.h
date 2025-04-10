#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "sdk_config.h"
#include "audio_cvp_def.h"
#include "audio_def.h"
#include "rcsp_define.h"



#define ENABLE_THIS_MOUDLE					1
#define DISABLE_THIS_MOUDLE					0

#define ENABLE								1
#define DISABLE								0


/*
 * 系统打印总开关
 */

#define CONFIG_DEBUG_ENABLE TCFG_DEBUG_UART_ENABLE


#if CONFIG_DEBUG_ENABLE
#define LIB_DEBUG    1
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)
#else
#define LIB_DEBUG    0
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)
#define CONFIG_DEBUG_LITE_ENABLE    0//轻量级打印开关, 默认关闭
#endif


#define TCFG_DEV_MANAGER_ENABLE                   1
#define CONFIG_FATFS_ENABLE                       1

//*********************************************************************************//
//                          文件系统相关配置                                       //
//*********************************************************************************//
#define SDFILE_DEV				"sdfile"

#define LIB_SUPPORT_MULTI_SECTOER_READ  0 //对应配合库修改是否一次性多sector读
#if LIB_SUPPORT_MULTI_SECTOER_READ
#define MAX_READ_LEN 8192
#endif


#define STYLE_JL_WTACH              (1)//彩屏demo
#define STYLE_JL_SOUNDBOX           (2)//点阵屏和彩屏
#define STYLE_JL_CHARGE             (3)//点阵屏充电仓
#define STYLE_JL_LED7               (4)//led7
#define STYLE_UI_SIMPLE             (5)//没有ui框架
#define STYLE_JL_SOUNDBAR           (6)//soundbar点阵屏

#define NO_CONFIG_PORT						(-1)


//*********************************************************************************//
//                                  BREDR && BLE配置                               //
//*********************************************************************************//
#if !TCFG_APP_BT_EN
#undef TCFG_BT_BACKGROUND_ENABLE
#define TCFG_BT_BACKGROUND_ENABLE   0
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
#undef TCFG_BT_SUPPORT_HFP
#undef TCFG_BT_SUPPORT_AVCTP
#undef TCFG_BT_SUPPORT_A2DP
#undef TCFG_BT_SUPPORT_HID
#undef TCFG_BT_SUPPORT_SPP
#undef TCFG_BT_SUPPORT_PNP
#undef TCFG_BT_SUPPORT_PBAP
#define TCFG_BT_SUPPORT_HFP 0x0 //HFP
#define TCFG_BT_SUPPORT_AVCTP 0x0 //AVRCP
#define TCFG_BT_SUPPORT_A2DP 0x0 //A2DP
#define TCFG_BT_SUPPORT_HID 0x0 //HID
#define TCFG_BT_SUPPORT_SPP 0x0 //SPP
#define TCFG_BT_SUPPORT_PNP 0x0 //PNP
#endif
#endif

//*********************************************************************************//
//                                  AI配置                                       //
//*********************************************************************************//
#define    RCSP_MODE_EN             (1 << 0)
#define    TRANS_DATA_EN            (1 << 1)
#define    LL_SYNC_EN               (1 << 2)
#define    TUYA_DEMO_EN             (1 << 3)
#define    ANCS_CLIENT_EN           (1 << 4)
#define    GFPS_EN                  (1 << 5)
#define    REALME_EN                (1 << 6)
#define    TME_EN                   (1 << 7)
#define    DMA_EN                   (1 << 8)
#define    GMA_EN                   (1 << 9)
#define    MMA_EN                   (1 << 10)
#define    FMNA_EN                  (1 << 11)
#define    SWIFT_PAIR_EN            (1 << 12)
#define    HONOR_EN                 (1 << 13)
#define    ONLINE_DEBUG_EN          (1 << 14)
#define    CUSTOM_DEMO_EN           (1 << 15)   // 第三方协议的demo，用于示例客户开发自定义协议

#if TCFG_THIRD_PARTY_PROTOCOLS_ENABLE
#define THIRD_PARTY_PROTOCOLS_SEL  TCFG_THIRD_PARTY_PROTOCOLS_SEL
#else
#define THIRD_PARTY_PROTOCOLS_SEL  0
#define TCFG_THIRD_PARTY_PROTOCOLS_SEL 0 // 第三方协议选择
#endif

#if THIRD_PARTY_PROTOCOLS_SEL && (TCFG_USER_BLE_ENABLE == 0)
#error "开启 le audio 功能需要使能 TCFG_USER_BLE_ENABLE"
#endif

//*********************************************************************************//
//                                  le_audio 配置                                       //
//*********************************************************************************//

#define    LE_AUDIO_UNICAST_SOURCE_EN           (1 << 0)
#define    LE_AUDIO_UNICAST_SINK_EN             (1 << 1)
#define    LE_AUDIO_AURACAST_SOURCE_EN          (1 << 2)
#define    LE_AUDIO_AURACAST_SINK_EN            (1 << 3)
#define    LE_AUDIO_JL_UNICAST_SOURCE_EN        (1 << 4)
#define    LE_AUDIO_JL_UNICAST_SINK_EN          (1 << 5)
#define    LE_AUDIO_JL_AURACAST_SOURCE_EN       (1 << 6)
#define    LE_AUDIO_JL_AURACAST_SINK_EN         (1 << 7)

#ifndef TCFG_LE_AUDIO_APP_CONFIG
#define TCFG_LE_AUDIO_APP_CONFIG                (0)
#endif

#if TCFG_LE_AUDIO_APP_CONFIG && (TCFG_USER_BLE_ENABLE == 0)
#error "开启 le audio 功能需要使能 TCFG_USER_BLE_ENABLE"
#endif


#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)     // rcsp与le audio共用 BLE ACL 时，使用不同地址
#undef  TCFG_BT_BLE_BREDR_SAME_ADDR
#define  TCFG_BT_BLE_BREDR_SAME_ADDR 0x0
#endif

// #undef TCFG_LOWPOWER_LOWPOWER_SEL
// #define  TCFG_LOWPOWER_LOWPOWER_SEL 0x0//低功耗连接还有问题
#endif


/* -----------------独立模式 PC/LINEIN/SPDIF 模块默认控制------- */
//没有独立模式的 需要在sdk_config.h定义对应的宏，有独立模式的，根据模式宏的值默认定义对应的宏
#ifndef TCFG_PC_ENABLE
#define TCFG_PC_ENABLE       TCFG_APP_PC_EN
#endif

#ifndef TCFG_AUDIO_LINEIN_ENABLE
#define TCFG_AUDIO_LINEIN_ENABLE   TCFG_APP_LINEIN_EN
#endif

#ifndef TCFG_SPDIF_ENABLE
#define TCFG_SPDIF_ENABLE    TCFG_APP_SPDIF_EN
#endif

#ifndef TCFG_AUDIO_FM_ENABLE
#define TCFG_AUDIO_FM_ENABLE     TCFG_APP_FM_EN
#endif


/* ------------------rule check------------------ */
#ifndef TCFG_APP_MUSIC_EN
#define TCFG_APP_MUSIC_EN  0
#endif


#ifndef TCFG_NOR_FS
#define TCFG_NOR_FS         0
#endif

#ifndef MBEDTLS_SELF_TEST
#define MBEDTLS_SELF_TEST   0
#endif

#ifndef TCFG_JSA1221_ENABLE
#define TCFG_JSA1221_ENABLE 0
#endif

#ifndef MBEDTLS_AES_SELF_TEST
#define MBEDTLS_AES_SELF_TEST   0
#endif

#ifndef MBEDTLS_AES_SELF_TEST
#define MBEDTLS_AES_SELF_TEST   0
#endif

#ifndef TCFG_USER_SOUNDBOX_ENABLE
#define TCFG_USER_SOUNDBOX_ENABLE  0
#endif

#ifndef TCFG_USER_SOUNDBOX_GLOBAL_ENABLE
#define TCFG_USER_SOUNDBOX_GLOBAL_ENABLE  0
#endif

#ifndef TCFG_BT_DONGLE_ENABLE
#define TCFG_BT_DONGLE_ENABLE   0
#endif

#ifndef TCFG_NOR_REC
#define TCFG_NOR_REC         0
#endif

#ifndef FLASH_INSIDE_REC_ENABLE
#define FLASH_INSIDE_REC_ENABLE     0
#endif

#ifndef TCFG_REC_FOLDER_NAME
#define TCFG_REC_FOLDER_NAME "JL_REC"
#endif
#ifndef TCFG_REC_FILE_NAME
#define TCFG_REC_FILE_NAME "aud_****"
#endif

#if !(THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)
#define	   RCSP_MODE			     RCSP_MODE_OFF
#endif

//单双备份的配置在board_xxx_global_cfg里配置，需要注意只有RCSP才支持单双备份，其余升级都是只支持双备份升级
//支持TWS同步升级，OTA_TWS_SAME_TIME_NEW宏需要配置为1，旧的流程已不再支持
#if (THIRD_PARTY_PROTOCOLS_SEL & RCSP_MODE_EN)

#define	   RCSP_MODE			     RCSP_MODE_SOUNDBOX
#include "rcsp_cfg.h"
// 详细功能参考rcsp_cfg.h

#elif (THIRD_PARTY_PROTOCOLS_SEL & (GFPS_EN | REALME_EN | TME_EN | DMA_EN | GMA_EN))
#define    BT_MIC_EN                 0
#define    TCFG_ENC_OPUS_ENABLE      0
#define    TCFG_ENC_SPEEX_ENABLE     0
#define    OTA_TWS_SAME_TIME_ENABLE  0     //是否支持TWS同步升级
#elif (THIRD_PARTY_PROTOCOLS_SEL & LL_SYNC_EN)
#define    OTA_TWS_SAME_TIME_ENABLE  1
#define    OTA_TWS_SAME_TIME_NEW     1     //使用新的tws ota流程
#define    TCFG_ENC_OPUS_ENABLE      0
#define    TCFG_ENC_SPEEX_ENABLE     0
#elif (THIRD_PARTY_PROTOCOLS_SEL & TUYA_DEMO_EN)
#define    OTA_TWS_SAME_TIME_ENABLE  0
#define    OTA_TWS_SAME_TIME_NEW     0     //使用新的tws ota流程
#define    TCFG_ENC_OPUS_ENABLE      0
#define    TCFG_ENC_SPEEX_ENABLE     0
#else
#define    OTA_TWS_SAME_TIME_ENABLE  0
#define    OTA_TWS_SAME_TIME_NEW     0     //使用新的tws ota流程
#define    TCFG_ENC_OPUS_ENABLE      0
#define    TCFG_ENC_SPEEX_ENABLE     0
#endif

#define CONFIG_MEDIA_LIB_USE_MALLOC    1
///USB 配置重定义
// #undef USB_DEVICE_CLASS_CONFIG
// #define USB_DEVICE_CLASS_CONFIG 									(AUDIO_CLASS)
/////要确保 上面 undef 后在include usb

#define USB_PC_NO_APP_MODE                        0

#if TCFG_CHARGE_ENABLE
#define TCFG_USB_PORT_CHARGE                1
#endif

#include "btcontroller_mode.h"

#include "user_cfg_id.h"

#ifndef __LD__
#include "bt_profile_cfg.h"
#endif


#if TCFG_THIRD_PARTY_PROTOCOLS_ENABLE
#if(APP_ONLINE_DEBUG)
#error "they can not enable at the same time,just select one!!!"
#endif
#endif

#define TCFG_LOWPOWER_RAM_SIZE				0	                // 低功耗掉电ram大小，单位：128K，可设置值：0、2、3

//*********************************************************************************//
//                                 测试模式配置                                    //
//*********************************************************************************//
#if (CONFIG_BT_MODE == BT_NORMAL)
//enable dut mode,need disable sleep(TCFG_LOWPOWER_LOWPOWER_SEL = 0)
// #define TCFG_NORMAL_SET_DUT_MODE                  0
#if TCFG_NORMAL_SET_DUT_MODE
#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0
#endif

#else
#undef TCFG_BT_DUAL_CONN_ENABLE

#undef  TCFG_USER_TWS_ENABLE
#define TCFG_USER_TWS_ENABLE                      0     //tws功能使能

#undef  TCFG_USER_BLE_ENABLE
#define TCFG_USER_BLE_ENABLE                      1     //BLE功能使能

#undef TCFG_BT_BLE_BREDR_SAME_ADDR
#define  TCFG_BT_BLE_BREDR_SAME_ADDR     0

#undef  TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME		          0

#undef  TCFG_SYS_LVD_EN
#define TCFG_SYS_LVD_EN						      0

#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0

#undef TCFG_AUDIO_DAC_LDO_VOLT
#define TCFG_AUDIO_DAC_LDO_VOLT			   DUT_AUDIO_DAC_LDO_VOLT

#undef TCFG_LOWPOWER_POWER_SEL
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15

#undef  TCFG_PWMLED_ENABLE
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE

#undef  TCFG_ADKEY_ENABLE
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE

#undef  TCFG_IOKEY_ENABLE
#define TCFG_IOKEY_ENABLE					DISABLE_THIS_MOUDLE

#undef TCFG_TEST_BOX_ENABLE
#define TCFG_TEST_BOX_ENABLE			    0

#undef TCFG_AUTO_POWERON_ENABLE
#define TCFG_AUTO_POWERON_ENABLE    1

#undef TCFG_CFG_TOOL_ENABLE
#define TCFG_CFG_TOOL_ENABLE 0
#undef TCFG_USB_HOST_ENABLE
#define TCFG_USB_HOST_ENABLE 0 //U盘使能
/* #undef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE					DISABLE_THIS_MOUDLE */

#endif //(CONFIG_BT_MODE != BT_NORMAL)


#if TCFG_USER_TWS_ENABLE

#define CONFIG_TWS_COMMON_ADDR_AUTO             0       /* 自动生成TWS配对后的MAC地址 */
#define CONFIG_TWS_COMMON_ADDR_USED_LEFT        1       /* 使用左耳的MAC地址作为TWS配对后的地址
                                                           可配合烧写器MAC地址自增功能一起使用
                                                           多台交叉配对会出现MAC地址相同情况 */
#define CONFIG_TWS_COMMON_ADDR_SELECT           CONFIG_TWS_COMMON_ADDR_AUTO

//*********************************************************************************//
//                                 对耳配置方式配置                                    //
//*********************************************************************************//
#define CONFIG_TWS_CONNECT_SIBLING_TIMEOUT    4    /* 开机或超时断开后对耳互连超时时间，单位s */
// #define CONFIG_TWS_REMOVE_PAIR_ENABLE              [> 不连手机的情况下双击按键删除配对信息 <]
//#define CONFIG_TWS_POWEROFF_SAME_TIME         1    /*按键关机时两个耳机同时关机*/

#define ONE_KEY_CTL_DIFF_FUNC                 1    /*通过左右耳实现一个按键控制两个功能*/
#define CONFIG_TWS_SCO_ONLY_MASTER			  0	   /*通话的时候只有主机出声音*/

/* 配对方式选择 */
#define CONFIG_TWS_PAIR_BY_CLICK            0      /* 按键发起配对 */
#define CONFIG_TWS_PAIR_BY_AUTO             1      /* 开机自动配对 */
#define CONFIG_TWS_PAIR_BY_BOX              2      /* 测试盒/充电仓配对 */
#define CONFIG_TWS_PAIR_MODE                TCFG_BT_TWS_PAIR_MODE


/* 声道确定方式选择 */
#define CONFIG_TWS_MASTER_AS_LEFT               0 //主机作为左耳
#define CONFIG_TWS_MASTER_AS_RIGHT              1 //主机作为右耳
#define CONFIG_TWS_AS_LEFT                      2 //固定左耳
#define CONFIG_TWS_AS_RIGHT                     3 //固定右耳
#define CONFIG_TWS_START_PAIR_AS_LEFT           4 //双击发起配对的耳机做左耳
#define CONFIG_TWS_START_PAIR_AS_RIGHT          5 //双击发起配对的耳机做右耳
#define CONFIG_TWS_EXTERN_UP_AS_LEFT            6 //外部有上拉电阻作为左耳
#define CONFIG_TWS_EXTERN_DOWN_AS_LEFT          7 //外部有下拉电阻作为左耳
#define CONFIG_TWS_EXTERN_UP_AS_RIGHT           8 //外部有上拉电阻作为右耳
#define CONFIG_TWS_EXTERN_DOWN_AS_RIGHT         9 //外部有下拉电阻作为右耳
#define CONFIG_TWS_CHANNEL_SELECT_BY_BOX        10 //充电仓/测试盒决定左右耳
#define CONFIG_TWS_CHANNEL_SELECT             TCFG_BT_TWS_CHANNEL_SELECT //配对方式选择

#define CONFIG_TWS_CHANNEL_CHECK_IO           IO_PORTA_07					//上下拉电阻检测引脚


#if CONFIG_TWS_PAIR_MODE != CONFIG_TWS_PAIR_BY_CLICK
#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_START_PAIR_AS_LEFT) ||\
    (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_START_PAIR_AS_RIGHT)
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_MASTER_AS_LEFT
#endif
#endif

#if CONFIG_TWS_USE_COMMMON_ADDR == 0
#undef TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE
#define TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE 0
#endif

#if TCFG_CHARGESTORE_ENABLE
#undef CONFIG_TWS_CHANNEL_SELECT
#define CONFIG_TWS_CHANNEL_SELECT             CONFIG_TWS_CHANNEL_SELECT_BY_BOX //充电仓区分左右
#endif


#if TCFG_TEST_BOX_ENABLE && (!TCFG_CHARGESTORE_ENABLE)
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    1 //测试盒配置左右耳优先
#else
#define CONFIG_TWS_SECECT_CHARGESTORE_PRIO    0
#endif //TCFG_TEST_BOX_ENABLE

#define TCFG_TWS_INIT_AFTER_POWERON_TONE_PLAY_END 1     //tws播完开机提示音再初始化，处理提示音不同步问题

#define CONFIG_A2DP_GAME_MODE_ENABLE            0  //游戏模式
#define CONFIG_A2DP_GAME_MODE_DELAY_TIME        35  //游戏模式延时ms

//*********************************************************************************//
//      低延时游戏模式脚步声、枪声增强,需使能蓝牙音乐10段eq以及蓝牙音乐drc
//      用户开关宏AUDIO_GAME_EFFECT_CONFIG(开关蓝牙低延时模式的游戏音效)
//      低延时eq效果文件使用eq_game_eff.bin,调试时需保存成该文件,并在批处理-res后添加
//      非低延时eq效果文件使用eq_cfg_hw.bin,也需在批处理-res后添加
//*********************************************************************************//
#if CONFIG_A2DP_GAME_MODE_ENABLE
#define AUDIO_GAME_EFFECT_CONFIG  1  //低延时游戏模式脚步声、枪声增强 1:使能、0：关闭
#else
#define AUDIO_GAME_EFFECT_CONFIG  0  //低延时游戏模式脚步声、枪声增强 1:使能、0：关闭
#endif


//*********************************************************************************//
//                                 对耳电量显示方式                                //
//*********************************************************************************//

#if TCFG_BT_DISPLAY_BAT_ENABLE
#define CONFIG_DISPLAY_TWS_BAT_LOWER          1 //对耳手机端电量显示，显示低电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_HIGHER         2 //对耳手机端电量显示，显示高电量耳机的电量
#define CONFIG_DISPLAY_TWS_BAT_LEFT           3 //对耳手机端电量显示，显示左耳的电量
#define CONFIG_DISPLAY_TWS_BAT_RIGHT          4 //对耳手机端电量显示，显示右耳的电量

#define CONFIG_DISPLAY_TWS_BAT_TYPE           CONFIG_DISPLAY_TWS_BAT_LOWER
#endif

#define CONFIG_DISPLAY_DETAIL_BAT             0 //BLE广播显示具体的电量
#define CONFIG_NO_DISPLAY_BUTTON_ICON         1 //BLE广播不显示按键界面,智能充电仓置1

#endif //TCFG_USER_TWS_ENABLE



//*********************************************************************************//
//                                 电源切换配置                                    //
//*********************************************************************************//

#define PHONE_CALL_USE_LDO15	CONFIG_PHONE_CALL_USE_LDO15

//*********************************************************************************//
//                                 时钟切换配置                                    //
//*********************************************************************************//
#define BT_NORMAL_HZ	            24000000

//*********************************************************************************//
//                                 rtc时钟源配置                                   //
//*********************************************************************************//
#define RTC_CLK_RES_SEL	            CLK_SEL_32K


//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#if TCFG_IRKEY_ENABLE
#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL			0                     //开红外不进入低功耗
#endif  /* #if TCFG_IRKEY_ENABLE */

//*********************************************************************************//
//                            LED使用 16SLOT TIMER 同步                            //
//*********************************************************************************//
//LED模块使用slot timer同步使用注意点:
//	1.soundbox不开该功能, 原因: 默认打开osc时钟, 使用原来的osc流程同步即可
//	2.带sd卡earphone不开该功能, 一般为单耳, 不需要同步, 使用原来的流程(lrc)
//	3.一般用在tws应用中, 而且默认关闭osc;
#if TCFG_USER_TWS_ENABLE
#define TCFG_PWMLED_USE_SLOT_TIME			ENABLE_THIS_MOUDLE
#endif

//*********************************************************************************//
//                                 升级配置                                        //
//*********************************************************************************//
//升级LED显示使能
#define UPDATE_LED_REMIND
//升级提示音使能
#define UPDATE_VOICE_REMIND


// #undef CONFIG_UPDATE_JUMP_TO_MASK
// #ifndef CONFIG_UPDATE_JUMP_TO_MASK
// #define CONFIG_UPDATE_JUMP_TO_MASK              0   	//配置升级到loader的方式0为直接reset,1为跳转(适用于芯片电源由IO口KEEP住的方案,需要注意检查跳转前是否将使用DMA的硬件模块全部关闭)
// #endif

// //用户串口主从机升级
// #define USER_UART_UPDATE_ENABLE           0//用于客户开发上位机或者多MCU串口升级方案
// #define UART_UPDATE_SLAVE	0
// #define UART_UPDATE_MASTER	1
// //配置串口升级的角色
// #define UART_UPDATE_ROLE	UART_UPDATE_SLAVE

#if CONFIG_UPDATE_MUTIL_CPU_UART
#if TCFG_UPDATE_UART_ROLE
#error "they can not enable at the same time,just select one!!!"
#endif
#endif

#if TCFG_UPDATE_UART_IO_EN
#undef TCFG_CHARGESTORE_ENABLE
#undef TCFG_TEST_BOX_ENABLE
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE       //用户串口升级也使用了UART1
#endif

//设备升级
#if TCFG_APP_MUSIC_EN
#define CONFIG_SD_UPDATE_ENABLE
#define CONFIG_USB_UPDATE_ENABLE
#define TCFG_DEV_UPDATE_IF_NOFILE_ENABLE   0//0：设备上线直接查找升级文件 1：无音乐文件时才查找升级文件
#endif
//*********************************************************************************//
//                                 Audio配置                                        //
//*********************************************************************************//

/*
 *Hi-Res Audio:Aux/USB Audio
 *Hi-Res Wireless Audio::LDAC/LHDC
 */
#if (TCFG_BT_SUPPORT_LHDC_V5 ||TCFG_BT_SUPPORT_LHDC || TCFG_BT_SUPPORT_LDAC)
#define TCFG_HI_RES_AUDIO_ENEBALE
#endif

#ifndef TCFG_AUDIO_MIC_PWR_PORT
#define TCFG_AUDIO_MIC_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_MIC1_PWR_PORT
#define TCFG_AUDIO_MIC1_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_MIC2_PWR_PORT
#define TCFG_AUDIO_MIC2_PWR_PORT NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_SCLK_PIN
#define TCFG_AUDIO_PLNK_SCLK_PIN NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_DAT0_PIN
#define TCFG_AUDIO_PLNK_DAT0_PIN NO_CONFIG_PORT
#endif

#ifndef TCFG_AUDIO_PLNK_DAT1_PIN
#define TCFG_AUDIO_PLNK_DAT1_PIN NO_CONFIG_PORT
#endif

#define  SYS_VOL_TYPE 	VOL_TYPE_DIGITAL/*目前仅支持软件数字音量模式*/

#if TCFG_AUDIO_CVP_SMS_ANS_MODE			/*单MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#elif (TCFG_AUDIO_CVP_SMS_DNS_MODE) /*单MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_ANS_MODE) /*双MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_DNS_MODE) /*双MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE) /*话务双MIC+ANS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_FLEXIBLE
#elif (TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE) /*话务双MIC+DNS通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	        DMS_FLEXIBLE
#elif (TCFG_AUDIO_CVP_3MIC_MODE) /*3MIC通话*/
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    ENABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_DNS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#else
#define TCFG_AUDIO_TRIPLE_MIC_ENABLE    DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_DUAL_MIC_ENABLE		DISABLE_THIS_MOUDLE
#define TCFG_AUDIO_CVP_NS_MODE	  	    CVP_ANS_MODE
#define TCFG_AUDIO_DMS_SEL	  	    	DMS_NORMAL
#endif/*TCFG_AUDIO_CVP_DMS_DNS_MODE*/

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG   (CVP_WB_EN) //只保留16k参数
#else
#define TCFG_AUDIO_CVP_BAND_WIDTH_CFG   (CVP_NB_EN | CVP_WB_EN)  //同时保留8k和16k的参数
#endif

/*使能iis输出外部参考数据*/
#if (TCFG_IIS_NODE_ENABLE == 1) && (TCFG_DAC_NODE_ENABLE == 0)
#define TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE    1
#else
#define TCFG_AUDIO_CVP_OUTPUT_WAY_IIS_ENABLE    0
#endif

/*Audio数据导出配置:通过蓝牙spp导出/sd写卡导出/uart写卡导出*/
#define AUDIO_DATA_EXPORT_VIA_UART	1
#define AUDIO_DATA_EXPORT_VIA_SPP 	2
#define AUDIO_DATA_EXPORT_VIA_SD	3
#define TCFG_AUDIO_DATA_EXPORT_DEFINE		DISABLE_THIS_MOUDLE

#if TCFG_DATA_EXPORT_NODE_ENABLE
#undef TCFG_AUDIO_DATA_EXPORT_DEFINE
#define TCFG_AUDIO_DATA_EXPORT_DEFINE		AUDIO_DATA_EXPORT_VIA_UART
#endif

/*
 *蓝牙spp数据导出的mic 通道，调试双麦ENC时,需要和ENC的mic通道保持一致
 *目前支持导出2路mic数据
 */
#if TCFG_AUDIO_TRIPLE_MIC_ENABLE
#define TCFG_SPP_DATA_EXPORT_ADC_MIC_CHA	(AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1 | AUDIO_ADC_MIC_2)
#else
#define TCFG_SPP_DATA_EXPORT_ADC_MIC_CHA	(AUDIO_ADC_MIC_0 | AUDIO_ADC_MIC_1)
#endif

#if TCFG_AUDIO_CVP_DEVELOP_ENABLE			/*通话第三方算法*/
#define TCFG_CVP_DEVELOP_ENABLE 		CVP_CFG_USER_DEFINED
#endif/*TCFG_AUDIO_CVP_DEVELOP_ENABLE*/

#if (TCFG_CVP_DEVELOP_ENABLE == CVP_CFG_AIS_3MIC)
#define CONFIG_BOARD_AISPEECH_NR
#endif /*TCFG_CVP_DEVELOP_ENABLE*/

/*Audio Smart Voice*/
#ifndef TCFG_SMART_VOICE_ENABLE
#define TCFG_SMART_VOICE_ENABLE 		    DISABLE_THIS_MOUDLE
#endif
#ifndef TCFG_AUDIO_KWS_LANGUAGE_SEL
#define TCFG_AUDIO_KWS_LANGUAGE_SEL    KWS_CH//近场中文
#endif

#define TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE 	TCFG_SMART_VOICE_ENABLE //语音事件处理流程开关
#if TCFG_SMART_VOICE_ENABLE
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_PMU_STD12M
#else
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_LRC
#endif/*TCFG_AUDIO_SMART_VOICE_ENABLE*/
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
#define TCFG_CALL_KWS_SWITCH_ENABLE         DISABLE_THIS_MOUDLE
#else
#define TCFG_CALL_KWS_SWITCH_ENABLE         TCFG_SMART_VOICE_ENABLE
#endif /*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

/*
 *第三方ASR（语音识别）配置
 *(1)用户自己开发算法
 *#define TCFG_AUDIO_ASR_DEVELOP             ASR_CFG_USER_DEFINED
 *(2)使用思必驰ASR算法
 *#define TCFG_AUDIO_ASR_DEVELOP             ASR_CFG_AIS
 */
#define ASR_CFG_USER_DEFINED	1
#define ASR_CFG_AIS				2
#define TCFG_AUDIO_ASR_DEVELOP				DISABLE_THIS_MOUDLE

#if (TCFG_AUDIO_ASR_DEVELOP	== ASR_CFG_AIS)
#undef TCFG_SMART_VOICE_ENABLE
#undef TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE
#undef TCFG_VAD_LOWPOWER_CLOCK
#define TCFG_SMART_VOICE_ENABLE 		    DISABLE_THIS_MOUDLE
#define TCFG_KWS_VOICE_EVENT_HANDLE_ENABLE 	TCFG_SMART_VOICE_ENABLE //语音事件处理流程开关
#define TCFG_VAD_LOWPOWER_CLOCK             VAD_CLOCK_USE_RC_AND_BTOSC
#define CONFIG_BOARD_AISPEECH_VAD_ASR   1
#endif /*TCFG_AUDIO_ASR_DEVELOP*/

/*
 *蓝牙音频能量检测使能配置
 *(1)1t2抢断播放需要使能能量检测
 *(2)蓝牙后台需要使能能量检测
 */
#if (((TCFG_A2DP_PREEMPTED_ENABLE == 1) || (TCFG_BT_BACKGROUND_ENABLE == 1)))
#ifndef CONFIG_CPU_BR29
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	1
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE TCFG_BT_SUPPORT_AAC
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE TCFG_BT_SUPPORT_LDAC
#else
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE 0
#endif
#else
#define TCFG_A2DP_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_SBC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_AAC_SILENCE_DETECT_ENABLE	0
#define TCFG_A2DP_LDAC_SILENCE_DETECT_ENABLE 0
#endif

/*
 * <<蓝牙音频播放使能控制>>
 * 比如关闭A2DP播放器，则手机播歌的时候，蓝牙数据传输是正常的，但是因为没有使能解码器，
 * 所以没有声音
 */
#define TCFG_BT_A2DP_PLAYER_ENABLE		1
#define TCFG_BT_ESCO_PLAYER_ENABLE		1

#if (TCFG_SMART_VOICE_ENABLE && TCFG_SMART_VOICE_USE_AEC)
#if (TCFG_AUDIO_GLOBAL_SAMPLE_RATE == 0) || (TCFG_AUDIO_GLOBAL_SAMPLE_RATE % 16000)
#error "TCFG_AUDIO_GLOBAL_SAMPLE_RATE 需要设置16000的倍数"
#endif /*TCFG_AUDIO_GLOBAL_SAMPLE_RATE*/
#endif /*TCFG_SMART_VOICE_ENABLE*/

#if (TCFG_AUDIO_CVP_SMS_ANS_MODE + TCFG_AUDIO_CVP_SMS_DNS_MODE \
   + TCFG_AUDIO_CVP_DMS_ANS_MODE + TCFG_AUDIO_CVP_DMS_DNS_MODE \
   + TCFG_AUDIO_CVP_DMS_FLEXIBLE_ANS_MODE + TCFG_AUDIO_CVP_DMS_FLEXIBLE_DNS_MODE \
   + TCFG_AUDIO_CVP_3MIC_MODE \
   + TCFG_AUDIO_CVP_DMS_HYBRID_DNS_MODE + TCFG_AUDIO_CVP_DMS_AWN_DNS_MODE \
   ) > 1
#error "整个SDK数据流里面只能使用一种模式的CVP通话节点"
#endif

//*********************************************************************************//
//                    充电中按键清除手机配对信息配置                               //
//*********************************************************************************//

#define CHARGING_CLEAN_PHONE_INFO       0



//*********************************************************************************//
//                                 调音工具                                        //
//*********************************************************************************//
#define TCFG_ONLINE_ENABLE                  TCFG_CFG_TOOL_ENABLE    //是否支持音效在线调试功能
#define TCFG_NULL_COMM						0				//不支持通信
#define TCFG_UART_COMM						1				//串口通信
#define TCFG_SPP_COMM						2				//SPP通信
#define TCFG_USB_COMM						3				//USB通信

/***********************************非用户配置区***********************************/





#include "usb_std_class_def.h"
#if TCFG_CFG_TOOL_ENABLE
#if (TCFG_COMM_TYPE == TCFG_USB_COMM)
#undef TCFG_USB_CDC_BACKGROUND_RUN
#define TCFG_USB_CDC_BACKGROUND_RUN         ENABLE
#endif
#if (TCFG_COMM_TYPE == TCFG_UART_COMM)
#undef TCFG_USB_CDC_BACKGROUND_RUN
#define TCFG_USB_CDC_BACKGROUND_RUN         DISABLE
#endif
#endif
#include "usb_common_def.h"

/*spp数据导出配置*/
#if ((TCFG_AUDIO_DATA_EXPORT_DEFINE == AUDIO_DATA_EXPORT_VIA_SPP) || TCFG_AUDIO_MIC_DUT_ENABLE)
#undef TCFG_USER_TWS_ENABLE
#undef TCFG_USER_BLE_ENABLE
#undef TCFG_BD_NUM
#undef TCFG_BT_SUPPORT_SPP
#undef TCFG_BT_SUPPORT_A2DP
#define TCFG_USER_TWS_ENABLE        0//spp数据导出，关闭tws
#define TCFG_USER_BLE_ENABLE        0//spp数据导出，关闭ble
#define TCFG_BD_NUM					1//连接设备个数配置
#define TCFG_BT_SUPPORT_SPP	1
#define TCFG_BT_SUPPORT_A2DP   0
#define APP_ONLINE_DEBUG            1//通过spp导出数据
#else
#define APP_ONLINE_DEBUG            0//在线APP调试,发布默认不开
#endif/*TCFG_AUDIO_DATA_EXPORT_DEFINE*/

//*********************************************************************************//
//                    需要spp调试的配置                                            //
//*********************************************************************************//
#if ( \
        TCFG_AEC_TOOL_ONLINE_ENABLE                                     || \
        TCFG_AUDIO_DUT_ENABLE   	                                    || \
        TCFG_ANC_TOOL_DEBUG_ONLINE                                      || \
        TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE                                || \
        (TCFG_CFG_TOOL_ENABLE && (TCFG_COMM_TYPE == TCFG_SPP_COMM))     || \
        TCFG_SPEAKER_EQ_NODE_ENABLE                                     \
    )
#undef TCFG_BT_SUPPORT_SPP
#undef APP_ONLINE_DEBUG
#define TCFG_BT_SUPPORT_SPP	1
#define APP_ONLINE_DEBUG            1
#endif

#if TCFG_CFG_TOOL_ENABLE
#undef EQ_SECTION_MAX
#define EQ_SECTION_MAX (32+2)//2多阶高低通滤波器
#endif

#ifndef EQ_SECTION_MAX
#if TCFG_EQ_ENABLE
#define EQ_SECTION_MAX (32+2)//2多阶高低通滤波器
#endif
#endif

#define MULTI_OUTPUT_DEV_EN       0//多设备输出使能，用于控制一些特殊的场景，例如iis_tx与le_audio_source延时对等

#define MEDIA_UNIFICATION_EN   1 //音乐播放器统一使用一个流程框图(媒体)

#define TCFG_USER_RSSI_TEST_EN   0   //通过spp获取耳机RSSI值，需要使能USER_SUPPORT_PROFILE_SPP

//FM 一部分代码动态加载到ram
#define TCFG_CODE_RUN_RAM_FM_CODE            1
//BT 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_BT_CODE            1

//AAC 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_AAC_CODE           1

//AEC 一部分代码加载到ram
#define TCFG_CODE_RUN_RAM_AEC_CODE           1

#ifdef CONFIG_CPU_BR27
#define TCFG_CODE_RUN_RAM_MIC_EFF_CODE       0
#else
#define TCFG_CODE_RUN_RAM_MIC_EFF_CODE       1
#endif

#ifndef TCFG_LP_TOUCH_KEY_ENABLE
#define TCFG_LP_TOUCH_KEY_ENABLE 	0
#endif

#define  TCFG_BLE_BRIDGE_EDR_ENALBE 0x0   //ios 一键连接，ctkd
#if TCFG_BLE_BRIDGE_EDR_ENALBE   //一键连接必须同地址
#undef  TCFG_BT_BLE_BREDR_SAME_ADDR
#define  TCFG_BT_BLE_BREDR_SAME_ADDR 0x1
#endif

#endif
