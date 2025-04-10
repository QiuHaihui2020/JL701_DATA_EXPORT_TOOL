
#ifndef __JL_CFG_DEC_H__
#define __JL_CFG_DEC_H__

#include "typedef.h"
#include "device/vm.h"

struct btif_item {
    u16 id;
    u16 data_len;
};


struct syscfg_operataions {
    int (*init)(void);
    int (*check_id)(u16 item_id);
    int (*read)(u16 item_id, void *buf, u16 len);
    int (*write)(u16 item_id, const void *buf, u16 len);
    int (*dma_write)(u16 item_id, const void *buf, u16 len);
    int (*read_string)(u16 item_id, u8 *buf, u16 len, u8 ver);
    u8 *(*ptr_read)(u16 item_id, u16 *len);
};

#define REGISTER_SYSCFG_OPS(cfg, pri) \
	const struct syscfg_operataions cfg SEC_USED(.syscfg.pri.ops)

//=================================================================================//
//                        系统配置项(VM, BTIF, cfg_bin)读写接口                    //
//接口说明:                                			   						       //
// 	1.输入参数                                			     					   //
// 		1)item_id: 配置项ID号, 由本文件统一分配;                                   //
// 		2)buf: 用于存储read/write数据内容;                                    	   //
// 		3)len: buf的长度(byte), buf长度必须大于等于read/write数据长度;             //
// 	2.返回参数:                                 			     				   //
// 		1)执行正确: 返回值等于实际上所读到的数据长度(大于0);                       //
//      2)执行错误: 返回值小于等于0, 小于0表示相关错误码;                          //
// 	3.读写接口使用注意事项:             										   //
// 		1)不能在中断里调用写(write)接口;                                   		   //
// 		2)调用本读写接口时应该习惯性判断返回值来检查read/write动作是否执行正确;    //
//=================================================================================//

/* --------------------------------------------------------------------------*/
/**
 * @brief 读取对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [out] buf 用于存储read数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于read数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_read(u16 item_id, void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 写入对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储write数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于write数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_write(u16 item_id, const void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 以dma的方式写入对应配置项的内容, 请注意buf地址需要按照4byte对齐
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储write数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于write数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_dma_write(u16 item_id, const void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 读取同一个配置项存在多份数据中的某一份数据, ver读取表示第几份数据, ver从 0 开始;
 * @brief 典型应用: 读取配置项CFG_BT_NAME中多个蓝牙名中的某一个蓝牙名;
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储read数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于read数据长度
 * @param  [in] ver 读取表示第几份数据
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_read_string(u16 item_id, void *buf, u16 len, u8 ver);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取配置项的地址
 * @brief 注: 只支持cfg_tools.bin文件中的配置项读取
 * @param  [in] item_id 配置项ID号
 * @param  [out] len 配置项长度
 *
 * @return 配置项地址指针(可以用cpu直接访问);
 */
/* --------------------------------------------------------------------------*/
u8 *syscfg_ptr_read(u16 item_id, u16 *len);


int syscfg_read_item(u16 cfg_id, u8 item, void *buf, u16 len);

int syscfg_tools_init(void);

//==================================================================================================//
//                                 配置项ID分配说明                                					//
//	1.配置项ID号根据存储区域进行分配;                                			   					//
//	2.存储区域有3个:  1)VM区域; 2)sys_cfg.bin; 3)BTIF区域                      	   					//
//	3.配置项ID号分配如下:  												   		   					//
//		0)[0]: 配置项ID号0为配置项工具保留ID号; 			   	   									//
//		1)[  1 ~  49]: 共49项, 预留给用户自定义, 只存于VM区域; 			   	   						//
//		2)[ 50 ~  99]: 共50项, sdk相关配置项, 只存于VM区域; 			   		   					//
//		3)[100 ~ 127]: 共28项, sdk相关配置项, 可以存于VM区域, sys_cfg.bin(作为默认值) 和 BTIF区域;	//
//		4)[512 ~ 700]: 共188项, sdk相关配置项, 只存于sys_cfg.bin; 		   		   					//
//==================================================================================================//

//=================================================================================//
//                             用户自定义配置项[1 ~ 49]                            //
//=================================================================================//
#define 	CFG_USER_DEFINE_BEGIN		1
/*用户请在 user_cfg_id.h 定义*/
#define 	CFG_USER_DEFINE_END			49

//=================================================================================//
//                             只存VM配置项[50 ~ 99]                         	   //
//=================================================================================//
#define 	CFG_STORE_VM_ONLY_BEGIN		50
#define     AT_CHAR_DEV_NAME            51
#define 	CFG_STORE_VM_ONLY_END		99

//=================================================================================//
//         可以存于VM, sys_cfg.bin(默认值)和BTIF区域的配置项[100 ~ 127]            //
//		   (VM支持扩展到511)    												   //
//=================================================================================//
#define 	CFG_STORE_VM_BIN_BTIF_BEGIN	100
#define 	CFG_STORE_VM_BIN_BTIF_END	(VM_ITEM_MAX_NUM - 1) //在app_cfg文件中配置128/256

//==================================================================================================//
//ID号分配方案:
// 1) 与APP CASE 相关的ID (0 ~ 50);
// 3) lib库保留ID(蓝牙, trim 值) (范围: 61 ~ 127); //67项
// 4) 与app_case 扩展ID号，需要更大的ram资源(128 ~ 511);
//==================================================================================================//

//=================================================================================//
//                             SDK库保留配置项[61 ~ 127]                           //
//=================================================================================//
#define 	CFG_REMOTE_DB_INFO		    61
#define 	CFG_REMOTE_DB_00			62
#define 	CFG_REMOTE_DB_01			63
#define 	CFG_REMOTE_DB_02			64
#define 	CFG_REMOTE_DB_03			65
#define 	CFG_REMOTE_DB_04			66
#define 	CFG_REMOTE_DB_05			67
#define 	CFG_REMOTE_DB_06			68
#define 	CFG_REMOTE_DB_07			69
#define 	CFG_REMOTE_DB_08			70
#define 	CFG_REMOTE_DB_09			71
#define 	CFG_REMOTE_DB_10			72
#define 	CFG_REMOTE_DB_11			73
#define 	CFG_REMOTE_DB_12			74
#define 	CFG_REMOTE_DB_13			75
#define 	CFG_REMOTE_DB_14			76
#define 	CFG_REMOTE_DB_15			77
#define 	CFG_REMOTE_DB_16			78
#define 	CFG_REMOTE_DB_17			79
#define 	CFG_REMOTE_DB_18			80
#define 	CFG_REMOTE_DB_19			81
#define 	CFG_DAC_TEST_VOLT		    82
#define 	CFG_BLE_MODE_INFO 		    83
#define     CFG_TWS_DUAL_CONFIG          84
#define     CFG_TWS_CONNECT_AA          85
#define     CFG_MUSIC_VOL               86
#define     CFG_CHARGESTORE_TWS_CHANNEL 87
#define 	CFG_DAC_DTB					88
#define 	CFG_MC_BIAS					89
#define 	CFG_POR_FLAG				90
#define 	CFG_MIC_LDO_VSEL			91
#define 	CFG_DAC_TRIM_INFO		    92
#define     CFG_BT_TRIM_INFO			93
#define     CFG_ANC_INFO				94
#define 	CFG_TWS_LOCAL_ADDR			95
#define 	CFG_TWS_REMOTE_ADDR			96
#define     CFG_TWS_COMMON_ADDR         97
#define     CFG_TWS_CHANNEL             98
#define		VM_PMU_VOLTAGE              99
#define		CFG_SYS_VOL                 100

//=========== btif & cfg_tool.bin & vm ============//
#define		CFG_BT_NAME    				101
#define     CFG_BT_MAC_ADDR             102
#define     VM_OLD_RTC_TIME             105
#define     VM_OLD_REAL_TIME            106
#define     VM_BLE_LOCAL_INFO           109
#define     CFG_BT_FRE_OFFSET			110
#define 	VM_GMA_ALI_PARA				111
#define 	VM_DMA_RAND					112
#define 	VM_GMA_MAC					113
#define 	VM_TME_AUTH_COOKIE			114
#define     VM_UPDATE_FLAG              115

#define     VM_RTC_TRIM                 116

#define     VM_BLE_REMOTE_DB_INFO       117
#define     VM_BLE_REMOTE_DB_00         118
#define     VM_BLE_REMOTE_DB_01         119
#define     VM_BLE_REMOTE_DB_02         120
#define     VM_BLE_REMOTE_DB_03         121
#define     VM_BLE_REMOTE_DB_04         122
#define     VM_BLE_REMOTE_DB_05         123
#define     VM_BLE_REMOTE_DB_06         124
#define     VM_BLE_REMOTE_DB_07         125
#define     VM_BLE_REMOTE_DB_08         126
#define     VM_BLE_REMOTE_DB_09         127


#define     VM_CHARGE_CALIBRATION       128//用于存储充电校准值
#define     VM_ARCH_TRIM                129

#define     VM_FM_BG_TRIM               130
#define     VM_FM_CBPF_TRIM             131

#define 	CFG_USB_BREAKPOINT0			132
#define 	CFG_SD0_BREAKPOINT0		    133
#define 	CFG_SD1_BREAKPOINT0		    134

#define     VM_TWS_ROLE                 135
#define 	VM_XM_INFO					136
#define 	VM_XM_INFO2					137
#define 	VM_XM_INFO3					138

#define		DUAL_BANK_BP_STEP			139
#define		DUAL_BANK_BP_FW_CODE		140

#define		VM_ALARM_0				    141
#define		VM_ALARM_1					142
#define		VM_ALARM_2					143
#define		VM_ALARM_3					144
#define		VM_ALARM_4					145
#define		VM_ALARM_MASK				146
#define		VM_ALARM_NAME_0				147
#define		VM_ALARM_NAME_1				148
#define		VM_ALARM_NAME_2				149
#define		VM_ALARM_NAME_3				150
#define		VM_ALARM_NAME_4				151
#define		VM_ALARM_RING_NAME_0		152
#define		VM_ALARM_RING_NAME_1		153
#define		VM_ALARM_RING_NAME_2		154
#define		VM_ALARM_RING_NAME_3		155
#define		VM_ALARM_RING_NAME_4		156
#define		VM_ALARM_EX0				157
#define		VM_ALARM_EX1				158
#define		VM_ALARM_EX2				159
#define		VM_ALARM_EX3				160
#define		VM_ALARM_EX4				161

#define     CFG_ONLINE_EQ_DRC_DATA_ID   254//在线调试保存参数的id
#define     CFG_ONLINE_SAVE_ID          255//在线保存文件大小的id


//=================================================================================//
//                   只存于sys_cfg.bin的配置项[512 ~ 700]                		   //
//=================================================================================//
#define 	CFG_STORE_BIN_ONLY_BEGIN	512
//硬件类配置项[513 ~ 600]
#define		CFG_UART_ID		    		513
#define		CFG_HWI2C_ID				514
#define		CFG_SWI2C_ID				515
#define		CFG_HWSPI_ID				516
#define		CFG_SWSPI_ID				517
#define		CFG_SD_ID					518
#define		CFG_USB_ID					519
#define		CFG_LCD_ID					520
#define		CFG_TOUCH_ID				521
#define		CFG_IOKEY_ID				522
#define		CFG_ADKEY_ID				523
#define		CFG_AUDIO_ID				524
#define		CFG_VIDEO_ID				525
#define		CFG_WIFI_ID					526
#define		CFG_NIC_ID					527
#define		CFG_LED_ID					528
#define		CFG_BATTERY_CURVE_ID		529
#define		CFG_IRFLT_ID				530
#define		CFG_PLCNT_ID				531
#define		CFG_ID_LED_CTRL				532
#define		CFG_RDEC_ID					533
#define		CFG_CHARGE_STORE_ID			534
#define		CFG_CHARGE_ID				535
#define    	CFG_LOWPOWER_V_ID   	    536
#define    	CFG_MIC_TYPE_ID   			537
#define    	CFG_COMBINE_SYS_VOL_ID 		538
#define    	CFG_COMBINE_CALL_VOL_ID 	539


#define     CFG_ID_IO_CFG_AT_POWERON    541
#define     CFG_ID_IO_CFG_AT_POWEROFF   542
#define     CFG_ID_IO_CTRL              543
#define     CFG_LP_TOUCH_KEY_ID         544
#define     CFG_LP_TOUCH_KEY_EARTCH_ID  545
#define     CFG_LP_TOUCH_KEY_DEBUG_ID   546

#define    	CFG_SPDIF_CFG_ID           	547
#define    	CFG_OPT_CFG_ID           	548
#define    	CFG_COAL_CFG_ID           	549

#define		CFG_AUDIO_DAC_ID            550

#define    	CFG_LINEIN_CFG_ID           551
#define    	CFG_FM_INSIDE_CFG_ID        552
#define    	CFG_FM_OUTSIDE_CFG_ID       553

//蓝牙类配置项[601 ~ 650]
#define     CFG_BT_RF_POWER_ID			601
#define    	CFG_TWS_PAIR_CODE_ID   		602
#define    	CFG_AUTO_OFF_TIME_ID   	    603
#define    	CFG_AEC_ID   	            604
#define    	CFG_UI_TONE_STATUS_ID   	605
#define    	CFG_KEY_MSG_ID   			606
#define    	CFG_LRC_ID   				607
#define    	CFG_DMS_ID   	            609
#define    	CFG_ANC_ID   	            610
#define    	CFG_SMS_DNS_ID 	            612	//单mic神经网络降噪
#define    	CFG_DMS_DNS_ID 	            613	//双mic神经网络降噪
#define    	CFG_DMS_FLEXIBLE_ID         614	//灵活可变双mic降噪

#define     CFG_ID_BT_NAME_SELECT       620

#define     CFG_ID_ALINK0_ID            621
#define     CFG_ID_ALINK1_ID            622

#define     CFG_BIG_PARAMS              623
#define     CFG_CIG_PARAMS              624

#define     CFG_LEA_PRODUCET_TEST_NAME  633
#define     CFG_LEA_PAIR_NAME           634

//其它类配置项[651 ~ 700]
#define 	CFG_STORE_BIN_ONLY_END		700


// votp区域
#define     CFG_VOTP_CFG_GROUP_START     (0xD00)
#define     CFG_VOTP_CFG_GROUP_END       (0xE00 - 1)
#define     CFG_VOTP_GROUP_START         (0xE00)
#define     CFG_VOTP_GROUP_END           (0xF00 - 1)
#define     CFG_VOTP_EFUSE_GROUP_START   (0xF00)
#define     CFG_VOTP_EFUSE_GROUP_END     (0x1000 - 1)

#define     VOTP_CFG_ITEM_ID(USER_ID)    ((USER_ID & 0xFF) | CFG_VOTP_CFG_GROUP_START)
#define     VOTP_ITEM_ID(USER_ID)        ((USER_ID & 0xFF) | CFG_VOTP_GROUP_START)
#define     VOTP_EFUSE_ITEM_ID(USER_ID)  ((USER_ID & 0xFF) | CFG_VOTP_EFUSE_GROUP_START)


#endif

