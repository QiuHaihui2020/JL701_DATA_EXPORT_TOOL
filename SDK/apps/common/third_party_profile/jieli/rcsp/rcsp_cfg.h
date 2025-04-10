#ifndef __RCSP_CFG_H__
#define __RCSP_CFG_H__

#include "rcsp_define.h"
#include "sdk_config.h"

// 全局搜索 RCSP_MODE 查找当前SDK相关配置
/*
#if (SOUNDCARD_ENABLE)
RCSP_MODE == RCSP_MODE_SOUNDBOX
#if (RCSP_MODE == RCSP_MODE_EARPHONE)
#if (RCSP_MODE == RCSP_MODE_WATCH)
SMART_BOX_EN
RCSP_BTMATE_EN
*/


#define JL_RCSP_NFC_DATA_OPT									0		// NFC数据传输

#if (defined OTA_TWS_SAME_TIME_ENABLE)
#undef OTA_TWS_SAME_TIME_ENABLE
#endif

// 没有开启rcsp协议
#if (!defined(RCSP_MODE) || (RCSP_MODE == 0))

#define OTA_TWS_SAME_TIME_ENABLE     							0
#define RCSP_UPDATE_EN               							0
#define UPDATE_MD5_ENABLE            							0		//升级是否支持MD5校验
#define RCSP_FILE_OPT				 							0
#define TCFG_BS_DEV_PATH_EN			 							0
#define WATCH_FILE_TO_FLASH          							0
#define UPDATE_EX_FALSH_USE_4K_BUF   							0

// 开启rcsp协议，具体功能根据不同设备类型有所不同，如下所示
#else // (!defined(RCSP_MODE) || (RCSP_MODE == 0))

#undef TCFG_USER_BLE_ENABLE
#define TCFG_USER_BLE_ENABLE	      							1		//BLE功能使能

// 音箱SDK RCSP功能配置
#if (RCSP_MODE == RCSP_MODE_SOUNDBOX)

#define RCSP_MSG_DISTRIBUTION_VER								RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL		//可视化配置工具的消息分发管理

#define RCSP_BLE_MASTER 										0		//当前是否ble主机

#if !RCSP_BLE_MASTER

// BLE从机配置
#define RCSP_ADV_EN												1
#define RCSP_DEVICE_STATUS_ENABLE								1		//设备状态信息功能
#define RCSP_BT_CONTROL_ENABLE									1		//bt控制功能

#define RCSP_UPDATE_EN		         							1		//是否支持rcsp升级
#define OTA_TWS_SAME_TIME_ENABLE     							0		//是否支持TWS同步升级
#define UPDATE_MD5_ENABLE            							1		//升级是否支持MD5校验
#define RCSP_FILE_OPT				 						    1
#define TCFG_BS_DEV_PATH_EN			 							1
#define RCSP_APP_RTC_EN											1		// 闹钟功能

#if TCFG_APP_RTC_EN
// 0 - 旧闹钟，只支持提示音闹铃
// 1 - 新闹钟，可选择提示音或者设备音乐作为闹铃
#define CUR_RTC_ALARM_MODE										1
#endif

// 默认的功能模块使能
#define RCSP_ADV_NAME_SET_ENABLE        						1		// 蓝牙名设置
#define RCSP_ADV_KEY_SET_ENABLE         						0		// 按键设置
#define RCSP_ADV_LED_SET_ENABLE         						0		// 灯光设置
#define RCSP_ADV_MIC_SET_ENABLE         						0		// mic设置
#define RCSP_ADV_WORK_SET_ENABLE        						0		// 模式设置（游戏模式）
#define RCSP_ADV_EQ_SET_ENABLE          						1		// eq设置
#define RCSP_ADV_MUSIC_INFO_ENABLE      						1		// 音乐信息设置
#define RCSP_ADV_HIGH_LOW_SET									1		// 高低音设置
#define RCSP_ADV_FIND_DEVICE_ENABLE     						1		// 查找设备设置
#define RCSP_ADV_PRODUCT_MSG_ENABLE     						1		// 获取产品信息
#define RCSP_ADV_COLOR_LED_SET_ENABLE   						0		// 彩灯设置
#define RCSP_ADV_KARAOKE_SET_ENABLE								0		// 卡拉OK设置
#define RCSP_ADV_KARAOKE_EQ_SET_ENABLE							0		// 卡拉OK EQ设置

// 混响相关功能
#define RCSP_REVERBERATION_SETTING							    0
#define RCSP_DRC_VAL_SETTING								    0
#define RCSP_KARAOKE_SOUND_EFFECT								0
#define RCSP_KARAOKE_ATMOSPHERE									0
#define RCSP_KARAOKE_SOUND_PARAM								0


#else  // !RCSP_BLE_MASTER

// BLE主机功能
#define RCSP_MULTI_BLE_SLAVE_NUMS          						0 //range(0~1)
#define RCSP_MULTI_BLE_MASTER_NUMS         						1 //range(0~2)
#define CONFIG_BT_GATT_CONNECTION_NUM       					RCSP_MULTI_BLE_SLAVE_NUMS + RCSP_MULTI_BLE_MASTER_NUMS
#define RCSP_UPDATE_EN		         							1		//是否支持rcsp升级

#endif // !RCSP_BLE_MASTER

// 手表SDK RCSP功能配置
#elif (RCSP_MODE == RCSP_MODE_WATCH)

#define RCSP_ADV_EN												0
#define RCSP_DEVICE_STATUS_ENABLE								1		//设备状态信息功能
#define RCSP_BT_CONTROL_ENABLE									1		//bt控制功能

#define JL_RCSP_SENSORS_DATA_OPT								1		// 传感器功能

#define RCSP_UPDATE_EN		         							1		//是否支持rcsp升级
#define OTA_TWS_SAME_TIME_ENABLE     							0		//是否支持TWS同步升级
#define UPDATE_MD5_ENABLE            							0		//升级是否支持MD5校验
#define RCSP_FILE_OPT				 							1
#define TCFG_BS_DEV_PATH_EN			 							1
#define WATCH_FILE_TO_FLASH          							1
#define UPDATE_EX_FALSH_USE_4K_BUF   							1

#define	JL_RCSP_SIMPLE_TRANSFER 								1

#define JL_RCSP_EXTRA_FLASH_OPT									1

#define RCSP_APP_RTC_EN											1		// 闹钟功能

#if TCFG_APP_RTC_EN
// 0 - 旧闹钟，只支持提示音闹铃
// 1 - 新闹钟，可选择提示音或者设备音乐作为闹铃
#define CUR_RTC_ALARM_MODE										1
#endif

// 默认的功能模块使能
// 可在板级配置头文件中定义客户定制的功能
#define RCSP_ADV_NAME_SET_ENABLE        						1		// 蓝牙名设置
#define RCSP_ADV_KEY_SET_ENABLE         						0		// 按键设置
#define RCSP_ADV_LED_SET_ENABLE         						1		// 灯光设置
#define RCSP_ADV_MIC_SET_ENABLE         						0		// mic设置
#define RCSP_ADV_WORK_SET_ENABLE        						0		// 模式设置（游戏模式）
#define RCSP_ADV_EQ_SET_ENABLE          						1		// eq设置
#define RCSP_ADV_MUSIC_INFO_ENABLE      						1		// 音乐信息设置
#define RCSP_ADV_HIGH_LOW_SET									1		// 高低音设置
#define RCSP_ADV_FIND_DEVICE_ENABLE     						1		// 查找设备设置
#define RCSP_ADV_PRODUCT_MSG_ENABLE     						1		// 获取产品信息
#define RCSP_ADV_COLOR_LED_SET_ENABLE   						1		// 彩灯设置
#define RCSP_ADV_KARAOKE_SET_ENABLE								0		// 卡拉OK设置
#define RCSP_ADV_KARAOKE_EQ_SET_ENABLE							0		// 卡拉OK EQ设置

// 混响相关功能
#define RCSP_REVERBERATION_SETTING								0
#define RCSP_DRC_VAL_SETTING									0
#define RCSP_KARAOKE_SOUND_EFFECT								0
#define RCSP_KARAOKE_ATMOSPHERE									0
#define RCSP_KARAOKE_SOUND_PARAM								0




// 耳机SDK可配置工具版本 RCSP功能配置
#elif (RCSP_MODE == RCSP_MODE_EARPHONE)

#define RCSP_MSG_DISTRIBUTION_VER								RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL		//可视化配置工具的消息分发管理

#define RCSP_ADV_EN												1
#define RCSP_DEVICE_STATUS_ENABLE								1		//设备状态信息功能
#define RCSP_BT_CONTROL_ENABLE									0		//bt控制功能

#define RCSP_UPDATE_EN		         							1		//是否支持rcsp升级

#if (defined OTA_TWS_SAME_TIME_NEW)
#undef OTA_TWS_SAME_TIME_NEW
#endif

#if CONFIG_DOUBLE_BANK_ENABLE              						//双备份才能打开同步升级流程
#define OTA_TWS_SAME_TIME_ENABLE     							1		//是否支持TWS同步升级
#define OTA_TWS_SAME_TIME_NEW        							1		//使用新的tws ota流程
#define UPDATE_MD5_ENABLE            							0		//升级是否支持MD5校验
#else
#define OTA_TWS_SAME_TIME_ENABLE     							0		//是否支持TWS同步升级
#define OTA_TWS_SAME_TIME_NEW        							0		//使用新的tws ota流程
#define UPDATE_MD5_ENABLE            							0		//升级是否支持MD5校验
#endif      //CONFIG_DOUBLE_BANK_ENABLE



// 默认的功能模块使能
#define RCSP_ADV_NAME_SET_ENABLE        						1		// 蓝牙名设置
#define RCSP_ADV_KEY_SET_ENABLE         						1		// 按键设置
#define RCSP_ADV_LED_SET_ENABLE         						0		// 灯光设置
#define RCSP_ADV_MIC_SET_ENABLE         						1		// mic设置
#define RCSP_ADV_WORK_SET_ENABLE        						1		// 模式设置（游戏模式）
#define RCSP_ADV_HALTER_ENABLE									0		// 挂脖功能
#define RCSP_ADV_EQ_SET_ENABLE          			TCFG_EQ_ENABLE		// eq设置
#define RCSP_ADV_MUSIC_INFO_ENABLE      						1		// 音乐信息设置
#define RCSP_ADV_HIGH_LOW_SET									1		// 高低音设置
#define RCSP_ADV_FIND_DEVICE_ENABLE     						1		// 查找设备设置
#define RCSP_ADV_PRODUCT_MSG_ENABLE     						1		// 获取产品信息
#define RCSP_ADV_COLOR_LED_SET_ENABLE   						0		// 彩灯设置
#define RCSP_ADV_KARAOKE_SET_ENABLE								0		// 卡拉OK设置
#define RCSP_ADV_KARAOKE_EQ_SET_ENABLE							0		// 卡拉OK EQ设置
#define RCSP_ADV_AI_NO_PICK										0		// 智能免摘
#define RCSP_ADV_ASSISTED_HEARING								0		// 辅听，注意开启辅听后，需要关闭ANC相关功能

#if !RCSP_ADV_ASSISTED_HEARING
#define RCSP_ADV_ANC_VOICE     					CONFIG_ANC_ENABLE		// 主动降噪

#if RCSP_ADV_ANC_VOICE
#define RCSP_ADV_ADAPTIVE_NOISE_REDUCTION 						0		// 自适应降噪
#define RCSP_ADV_SCENE_NOISE_REDUCTION							0		// 场景降噪
#define RCSP_ADV_WIND_NOISE_DETECTION							0		// 风噪检测
#define RCSP_ADV_VOICE_ENHANCEMENT_MODE							0		// 人声增强模式
#endif

#endif



// 通用 RCSP功能配置
#else // (RCSP_MODE == RCSP_MODE_COMMON)

// TODO
#define RCSP_ADV_EN												1
#define RCSP_DEVICE_STATUS_ENABLE								1		//设备状态信息功能
#define RCSP_BT_CONTROL_ENABLE									0			//bt控制功能



#endif // RCSP_MODE


#if (defined CONFIG_CPU_BR21)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC692X
#elif (defined CONFIG_CPU_BR22)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC693X
#elif (defined CONFIG_CPU_BR23 && SOUNDCARD_ENABLE)
#define     RCSP_SDK_TYPE       RCSP_SDK_TYPE_AC695X_SOUNDCARD
#elif (defined CONFIG_CPU_BR23 && (RCSP_MODE == RCSP_MODE_WATCH))
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC695X_WATCH
#elif (defined CONFIG_CPU_BR23)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC695X
#elif (defined CONFIG_CPU_BR25)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC696X
#elif (defined CONFIG_CPU_BR28)
#if (RCSP_MODE == RCSP_MODE_WATCH)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC701N_WATCH
#else
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC696X
#endif
#elif (defined CONFIG_CPU_BR30)
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC697X
#else
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_AC693X
#endif

#if RCSP_MSG_DISTRIBUTION_VER == RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL
#undef 		RCSP_SDK_TYPE
#if RCSP_MODE == RCSP_MODE_EARPHONE
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_MANIFEST_EARPHONE
#else
#define		RCSP_SDK_TYPE		RCSP_SDK_TYPE_MANIFEST_SOUNDBOX
#endif
#endif

#if RCSP_MSG_DISTRIBUTION_VER == RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL
// 这个版本没有灯光配置
#undef RCSP_ADV_LED_SET_ENABLE
#define RCSP_ADV_LED_SET_ENABLE         						0
#undef RCSP_ADV_COLOR_LED_SET_ENABLE
#define RCSP_ADV_COLOR_LED_SET_ENABLE   						0

#endif


#if TCFG_RCSP_DUAL_CONN_ENABLE
// RCSP一拖二需要关闭功能
#undef RCSP_ADV_MUSIC_INFO_ENABLE
#define RCSP_ADV_MUSIC_INFO_ENABLE								0

#endif

#endif // (!defined(RCSP_MODE) || (RCSP_MODE == 0))

// 默认RCSP配置，如果没有定义，则默认为0
#ifndef RCSP_BTMATE_EN
#define RCSP_BTMATE_EN      									0
#endif

#ifndef RCSP_DEVICE_STATUS_ENABLE
#define RCSP_DEVICE_STATUS_ENABLE								1		//设备状态信息
#endif

#ifndef RCSP_BT_CONTROL_ENABLE
#define RCSP_BT_CONTROL_ENABLE									0		//bt控制功能
#endif

#ifndef RCSP_ADV_EN
#define RCSP_ADV_EN         									0
#endif

#ifndef UPDATE_MD5_ENABLE
#define UPDATE_MD5_ENABLE            							0		//升级是否支持MD5校验
#endif

#ifndef RCSP_ADV_NAME_SET_ENABLE
#define RCSP_ADV_NAME_SET_ENABLE                				0
#endif

#ifndef RCSP_ADV_KEY_SET_ENABLE
#define RCSP_ADV_KEY_SET_ENABLE                 				0
#endif

#ifndef RCSP_ADV_LED_SET_ENABLE
#define RCSP_ADV_LED_SET_ENABLE                		 			0
#endif

#ifndef RCSP_ADV_MIC_SET_ENABLE
#define RCSP_ADV_MIC_SET_ENABLE                 				0
#endif

#ifndef RCSP_ADV_WORK_SET_ENABLE
#define RCSP_ADV_WORK_SET_ENABLE                				0
#endif

#ifndef RCSP_ADV_HIGH_LOW_SET
#define RCSP_ADV_HIGH_LOW_SET                   				0
#endif

#ifndef RCSP_ADV_MUSIC_INFO_ENABLE
#define RCSP_ADV_MUSIC_INFO_ENABLE              				0
#endif

#ifndef RCSP_ADV_KARAOKE_SET_ENABLE
#define RCSP_ADV_KARAOKE_SET_ENABLE             				0
#endif

#ifndef RCSP_ADV_PRODUCT_MSG_ENABLE
#define RCSP_ADV_PRODUCT_MSG_ENABLE        						0
#endif

#ifndef RCSP_ADV_ASSISTED_HEARING
#define RCSP_ADV_ASSISTED_HEARING								0
#endif

#ifndef RCSP_ADV_AI_NO_PICK
#define RCSP_ADV_AI_NO_PICK										0
#endif

#ifndef RCSP_ADV_SCENE_NOISE_REDUCTION
#define RCSP_ADV_SCENE_NOISE_REDUCTION							0
#endif

#ifndef RCSP_ADV_WIND_NOISE_DETECTION
#define RCSP_ADV_WIND_NOISE_DETECTION							0
#endif

#ifndef RCSP_ADV_VOICE_ENHANCEMENT_MODE
#define RCSP_ADV_VOICE_ENHANCEMENT_MODE							0
#endif

#ifndef RCSP_ADV_COLOR_LED_SET_ENABLE
#define RCSP_ADV_COLOR_LED_SET_ENABLE   						0
#endif

#ifndef RCSP_ADV_KARAOKE_EQ_SET_ENABLE
#define RCSP_ADV_KARAOKE_EQ_SET_ENABLE							0
#endif

#ifndef RCSP_ADV_EQ_SET_ENABLE
#define RCSP_ADV_EQ_SET_ENABLE          						0
#endif

#ifndef RCSP_ADV_FIND_DEVICE_ENABLE
#define RCSP_ADV_FIND_DEVICE_ENABLE								0
#endif

#ifndef RCSP_FILE_OPT
#define RCSP_FILE_OPT       									0
#endif

#ifndef RCSP_UPDATE_EN
#define RCSP_UPDATE_EN                  						0
#endif

#ifndef RCSP_MULTI_BLE_MASTER_NUMS
#define RCSP_MULTI_BLE_MASTER_NUMS								0
#endif

#ifndef RCSP_MULTI_BLE_SLAVE_NUMS
#define RCSP_MULTI_BLE_SLAVE_NUMS								0
#endif

#ifndef RCSP_APP_MUSIC_EN
#define RCSP_APP_MUSIC_EN										0
#endif

#ifndef RCSP_APP_RTC_EN
#define RCSP_APP_RTC_EN											0
#endif

#ifndef JL_RCSP_EXTRA_FLASH_OPT
#define JL_RCSP_EXTRA_FLASH_OPT									0
#endif

#ifndef WATCH_FILE_TO_FLASH
#define WATCH_FILE_TO_FLASH										0
#endif

#ifndef RCSP_ADV_ADAPTIVE_NOISE_REDUCTION
#define RCSP_ADV_ADAPTIVE_NOISE_REDUCTION 						0
#endif

#ifndef JL_RCSP_SIMPLE_TRANSFER
#define JL_RCSP_SIMPLE_TRANSFER									0
#endif

#ifndef JL_RCSP_SENSORS_DATA_OPT
#define JL_RCSP_SENSORS_DATA_OPT								0
#endif

#ifndef TCFG_USER_BLE_CTRL_BREDR_EN
#define TCFG_USER_BLE_CTRL_BREDR_EN								0
#endif

#ifndef PRINT_DMA_DATA_EN
#define PRINT_DMA_DATA_EN										0
#endif

#ifndef RCSP_MULTI_BLE_SLAVE_N
#define RCSP_MULTI_BLE_SLAVE_N									0
#endif

#ifndef RCSP_BLE_MASTER
#define RCSP_BLE_MASTER											0
#endif

#ifndef TCFG_BT_VOL_SYNC_ENABLE
#define TCFG_BT_VOL_SYNC_ENABLE									0
#endif

#ifndef RCSP_MSG_DISTRIBUTION_VER
#define RCSP_MSG_DISTRIBUTION_VER								RCSP_MSG_DISTRIBUTION_VER_DEFAULT
#endif

#ifndef RCSP_REVERBERATION_SETTING
#define RCSP_REVERBERATION_SETTING								0
#endif

#ifndef RCSP_DRC_VAL_SETTING
#define RCSP_DRC_VAL_SETTING									0
#endif

#ifndef RCSP_KARAOKE_SOUND_EFFECT
#define RCSP_KARAOKE_SOUND_EFFECT								0
#endif

#ifndef RCSP_KARAOKE_ATMOSPHERE
#define RCSP_KARAOKE_ATMOSPHERE									0
#endif

#ifndef RCSP_KARAOKE_SOUND_PARAM
#define RCSP_KARAOKE_SOUND_PARAM								0
#endif

#endif // __RCSP_CFG_H__

