/**
* 注意点：
* 0.此文件变化，在工具端会自动同步修改到工具配置中
* 1.功能块通过【---------xxx------】和 【#endif // xxx 】，是工具识别的关键位置，请勿随意改动
* 2.目前工具暂不支持非文件已有的C语言语法，此文件应使用文件内已有的语法增加业务所需的代码，避免产生不必要的bug
* 3.修改该文件出现工具同步异常或者导出异常时，请先检查文件内语法是否正常
**/

#ifndef SDK_CONFIG_H
#define SDK_CONFIG_H

#include "jlstream_node_cfg.h"

// ------------板级配置.json------------
#define TCFG_DEBUG_UART_ENABLE                    1                // 调试串口
#if TCFG_DEBUG_UART_ENABLE
#define TCFG_DEBUG_UART_TX_PIN                    IO_PORTB_05      // 输出IO
#define TCFG_DEBUG_UART_BAUDRATE                  2000000          // 波特率
#define TCFG_EXCEPTION_LOG_ENABLE                 1                // 打印异常信息
#define TCFG_EXCEPTION_RESET_ENABLE               0                // 异常自动复位
#endif // TCFG_DEBUG_UART_ENABLE

#define TCFG_CFG_TOOL_ENABLE                      0                // FW编辑、在线调音
#if TCFG_CFG_TOOL_ENABLE
#define TCFG_ONLINE_TX_PORT                       IO_PORT_DP       // 串口引脚TX
#define TCFG_ONLINE_RX_PORT                       IO_PORT_DM       // 串口引脚RX
#define TCFG_COMM_TYPE                            TCFG_USB_COMM    // 通信方式
#endif // TCFG_CFG_TOOL_ENABLE

#define CONFIG_SPI_DATA_WIDTH                     2                // flash通信
#define CONFIG_SPI_MODE                           0                // flash模式
#define CONFIG_FLASH_SIZE                         1048576          // flash容量
#define TCFG_VM_SIZE                              32               // VM大小（K）

#define TCFG_PWMLED_ENABLE                        1                // LED配置
#if TCFG_PWMLED_ENABLE
#define TCFG_LED_LAYOUT                           ONE_IO_TWO_LED   // 连接方式
#define TCFG_LED_RED_ENABLE                       1                // 红灯(Red)
#define TCFG_LED_RED_GPIO                         IO_PORTB_02      // IO
#define TCFG_LED_RED_LOGIC                        BRIGHT_BY_HIGH   // 点亮方式
#define TCFG_LED_BLUE_ENABLE                      1                // 蓝灯(Blue)
#define TCFG_LED_BLUE_GPIO                        IO_PORTB_02      // IO
#define TCFG_LED_BLUE_LOGIC                       BRIGHT_BY_LOW    // 点亮方式
#endif // TCFG_PWMLED_ENABLE

#define TCFG_SD0_ENABLE                           1                // SD配置
#if TCFG_SD0_ENABLE
#define TCFG_SD0_DAT_MODE                         1                // 线数设置
#define TCFG_SD0_DET_MODE                         SD_CLK_DECT      // 检测方式
#define TCFG_SD0_CLK                              12000000         // SD时钟频率
#define TCFG_SD0_DET_IO                           NO_CONFIG_PORT   // 检测IO
#define TCFG_SD0_DET_IO_LEVEL                     0                // IO检测方式
#define TCFG_SD0_POWER_SEL                        SD_PWR_SDPG      // SD卡电源
#define TCFG_SDX_CAN_OPERATE_MMC_CARD             0                // 支持MMC卡
#define TCFG_KEEP_CARD_AT_ACTIVE_STATUS           0                // 保持卡活跃状态
#define TCFG_SD0_PORT_CMD                         IO_PORTG_01      // SD_PORT_CMD
#define TCFG_SD0_PORT_CLK                         IO_PORTG_02      // SD_PORT_CLK
#define TCFG_SD0_PORT_DA0                         IO_PORTG_00      // SD_PORT_DATA0
#define TCFG_SD0_PORT_DA1                         NO_CONFIG_PORT   // SD_PORT_DATA1
#define TCFG_SD0_PORT_DA2                         NO_CONFIG_PORT   // SD_PORT_DATA2
#define TCFG_SD0_PORT_DA3                         NO_CONFIG_PORT   // SD_PORT_DATA3
#endif // TCFG_SD0_ENABLE

#define FUSB_MODE                                 1                // USB工作模式
#define TCFG_USB_HOST_ENABLE                      0                // USB主机总开关
#define USB_H_MALLOC_ENABLE                       1                // 主机使用malloc
#define TCFG_USB_HOST_MOUNT_RESET                 40               // usb reset时间
#define TCFG_USB_HOST_MOUNT_TIMEOUT               50               // 握手超时时间
#define TCFG_USB_HOST_MOUNT_RETRY                 3                // 枚举失败重试次数
#define TCFG_UDISK_ENABLE                         0                // U盘使能
#define USB_MALLOC_ENABLE                         1                // 从机使用malloc
#define TCFG_USB_SLAVE_MSD_ENABLE                 1                // 读卡器使能
#define TCFG_USB_SLAVE_HID_ENABLE                 1                // HID使能
#define MSD_BLOCK_NUM                             1                // MSD缓存块数
#define USB_AUDIO_VERSION                         USB_AUDIO_VERSION_1_0 // UAC协议版本
#define TCFG_USB_SLAVE_AUDIO_SPK_ENABLE           0                // USB扬声器使能
#define SPK_AUDIO_RATE_NUM                        1                // SPK采样率列表
#define SPK_AUDIO_RES                             16               // SPK位宽1
#define SPK_AUDIO_RES_2                           0                // SPK位宽2
#define TCFG_USB_SLAVE_AUDIO_MIC_ENABLE           0                // USB麦克风使能
#define MIC_AUDIO_RATE_NUM                        1                // MIC采样率列表
#define MIC_AUDIO_RES                             16               // MIC位宽1
#define MIC_AUDIO_RES_2                           0                // MIC位宽2
#define TCFG_SW_I2C0_CLK_PORT                     IO_PORTA_02      // 软件iic CLK脚
#define TCFG_SW_I2C0_DAT_PORT                     IO_PORTA_03      // 软件iic DATA脚
#define TCFG_SW_I2C0_DELAY_CNT                    50               // iic 延时
#define TCFG_HW_I2C0_CLK_PORT                     IO_PORTC_04      // 硬件iic CLK脚
#define TCFG_HW_I2C0_DAT_PORT                     IO_PORTC_05      // 硬件iic DATA脚
#define TCFG_HW_I2C0_CLK                          100000           // 硬件iic波特率

#define TCFG_HW_SPI1_ENABLE                       1                // 硬件SPI1
#if TCFG_HW_SPI1_ENABLE
#define TCFG_HW_SPI1_PORT_CLK                     IO_PORTA_08      // SPI1 CLK脚
#define TCFG_HW_SPI1_PORT_DO                      IO_PORTC_02      // SPI1  DO脚
#define TCFG_HW_SPI1_PORT_DI                      NO_CONFIG_PORT   // SPI1  DI脚
#define TCFG_HW_SPI1_MODE                         SPI_MODE_BIDIR_1BIT // SPI1 模式
#define TCFG_HW_SPI1_ROLE                         SPI_ROLE_MASTER  // SPI1  ROLE
#define TCFG_HW_SPI1_BAUD                         24000000         // SPI1  时钟
#endif // TCFG_HW_SPI1_ENABLE

#define TCFG_HW_SPI2_ENABLE                       0                // 硬件SPI2
#if TCFG_HW_SPI2_ENABLE
#define TCFG_HW_SPI2_PORT_CLK                     NO_CONFIG_PORT   // SPI2 CLK脚
#define TCFG_HW_SPI2_PORT_DO                      NO_CONFIG_PORT   // SPI2  DO脚
#define TCFG_HW_SPI2_PORT_DI                      NO_CONFIG_PORT   // SPI2  DI脚
#define TCFG_HW_SPI2_MODE                         SPI_MODE_BIDIR_1BIT // SPI2 模式
#define TCFG_HW_SPI2_ROLE                         SPI_ROLE_MASTER  // SPI2  ROLE
#define TCFG_HW_SPI2_BAUD                         24000000         // SPI2  时钟
#endif // TCFG_HW_SPI2_ENABLE

#define TCFG_FM_OUTSIDE_ENABLE                    0                // 外置FM配置
#if TCFG_FM_OUTSIDE_ENABLE
#define TCFG_FM_RDA5807_ENABLE                    0                // FM_RDA5807
#define TCFG_FM_BK1080_ENABLE                     0                // FM_BK1080
#define TCFG_FM_QN8035_ENABLE                     0                // FM_QN8035
#endif // TCFG_FM_OUTSIDE_ENABLE

#define TCFG_LINEIN_DETECT_ENABLE                 0                // LINEIN检测配置
#if TCFG_LINEIN_DETECT_ENABLE
#define TCFG_LINEIN_DETECT_IO                     IO_PORTB_03      // 检测IO选择
#define TCFG_LINEIN_DETECT_PULL_UP_ENABLE         1                // 检测IO上拉使能
#define TCFG_LINEIN_DETECT_PULL_DOWN_ENABLE       0                // 检测IO下拉使能
#define TCFG_LINEIN_AD_DETECT_ENABLE              0                // 使能AD检测
#define TCFG_LINEIN_AD_DETECT_VALUE               0                // AD检测时阈值
#endif // TCFG_LINEIN_DETECT_ENABLE

#define TCFG_IO_CFG_AT_POWER_ON                   0                // 开机时IO配置

#define TCFG_IO_CFG_AT_POWER_OFF                  0                // 关机时IO配置

#define TCFG_CHARGESTORE_PORT                     0                // 通信IO
// ------------板级配置.json------------

// ------------功能配置.json------------
#define TCFG_APP_BT_EN                            0                // 蓝牙模式
#define TCFG_APP_MUSIC_EN                         0                // 音乐模式
#define TCFG_APP_LINEIN_EN                        0                // LINEIN模式
#define TCFG_APP_FM_EN                            0                // FM模式
#define TCFG_APP_PC_EN                            0                // PC模式
#define TCFG_APP_RTC_EN                           0                // RTC模式
#define TCFG_APP_IIS_EN                           0                // IIS模式
#define TCFG_MIC_EFFECT_ENABLE                    0                // 混响使能
#define TCFG_DEC_ID3_V2_ENABLE                    0                // ID3_V2
#define TCFG_DEC_ID3_V1_ENABLE                    0                // ID3_V1
#define FILE_DEC_REPEAT_EN                        0                // 无缝循环播放
#define FILE_DEC_DEST_PLAY                        0                // 指定时间播放
#define FILE_DEC_AB_REPEAT_EN                     0                // AB点复读
#define TCFG_DEC_DECRYPT_ENABLE                   0                // 加密文件播
#define TCFG_DEC_DECRYPT_KEY                      0x12345678       // 加密KEY
#define MUSIC_PLAYER_CYCLE_ALL_DEV_EN             0                // 循环播放模式是否循环所有设备
#define MUSIC_PLAYER_PLAY_FOLDER_PREV_FIRST_FILE_EN 0                // 切换文件夹播放时从第一首歌开始
#define TWFG_APP_POWERON_IGNORE_DEV               0                // 设备忽略时间（单位：ms）
#define TCFG_FIX_CLOCK_FREQ                       192000000        // 固定时钟频率
// ------------功能配置.json------------

// ------------按键配置.json------------
#define TCFG_LONG_PRESS_RESET_ENABLE              0                // 非按键长按复位配置
#if TCFG_LONG_PRESS_RESET_ENABLE
#define TCFG_LONG_PRESS_RESET_PORT                IO_PORTB_01      // 复位IO
#define TCFG_LONG_PRESS_RESET_TIME                8                // 复位时间
#define TCFG_LONG_PRESS_RESET_LEVEL               0                // 复位电平
#define TCFG_LONG_PRESS_RESET_INSIDE_PULL_UP_DOWN 0                // 使用IO内置上下拉
#endif // TCFG_LONG_PRESS_RESET_ENABLE

#define TCFG_IOKEY_ENABLE                         0                // IO按键配置

#define TCFG_ADKEY_ENABLE                         1                // AD按键配置

#define TCFG_IRKEY_ENABLE                         0                // IR按键配置

// ------------按键配置.json------------

// ------------电源配置.json------------
#define TCFG_CLOCK_OSC_HZ                         24000000         // 晶振频率
#define TCFG_LOWPOWER_POWER_SEL                   PWR_LDO15        // 电源模式
#define TCFG_LOWPOWER_OSC_TYPE                    OSC_TYPE_LRC     // 低功耗时钟源
#define TCFG_CLOCK_MODE                           CLOCK_MODE_ADAPTIVE // 时钟模式
#define TCFG_LOWPOWER_VDDIOM_LEVEL                VDDIOM_VOL_34V   // IOVDD
#define TCFG_LOWPOWER_VDDIOW_LEVEL                VDDIOM_VOL_26V   // 弱IOVDD
#define CONFIG_LVD_LEVEL                          2.7v             // LVD档位
#define TCFG_MAX_LIMIT_SYS_CLOCK                  128000000        // 上限时钟设置
#define TCFG_LOWPOWER_LOWPOWER_SEL                0                // 低功耗模式
#define TCFG_AUTO_POWERON_ENABLE                  1                // 上电自动开机

#define TCFG_SYS_LVD_EN                           0                // 电池电量检测
#if TCFG_SYS_LVD_EN
#define TCFG_POWER_OFF_VOLTAGE                    3600             // 关机电压(mV)
#define TCFG_POWER_WARN_VOLTAGE                   3700             // 低电电压(mV)
#endif // TCFG_SYS_LVD_EN

#define TCFG_CHARGE_ENABLE                        0                // 充电配置
#if TCFG_CHARGE_ENABLE
#define TCFG_CHARGE_TRICKLE_MA                    1                // 涓流电流
#define TCFG_CHARGE_MA                            13               // 恒流电流
#define TCFG_CHARGE_FULL_MA                       3                // 截止电流
#define TCFG_CHARGE_FULL_V                        8                // 截止电压
#define TCFG_CHARGE_POWERON_ENABLE                0                // 开机充电
#define TCFG_CHARGE_OFF_POWERON_EN                0                // 拔出开机
#define TCFG_LDOIN_PULLDOWN_EN                    0                // 下拉电阻开关
#define TCFG_LDOIN_PULLDOWN_LEV                   3                // 下拉电阻档位
#define TCFG_LDOIN_PULLDOWN_KEEP                  0                // 下拉电阻保持开关
#define TCFG_LDOIN_ON_FILTER_TIME                 100              // 入舱滤波时间(ms)
#define TCFG_LDOIN_OFF_FILTER_TIME                200              // 出舱滤波时间(ms)
#define TCFG_LDOIN_KEEP_FILTER_TIME               440              // 维持电压滤波时间(ms)
#endif // TCFG_CHARGE_ENABLE

#define TCFG_BATTERY_CURVE_ENABLE                 0                // 电池曲线配置

// ------------电源配置.json------------

// ------------UI配置.json------------
#define TCFG_UI_ENABLE                            0                // UI配置
#if TCFG_UI_ENABLE
#define CONFIG_UI_STYLE                           STYLE_JL_SOUNDBAR // UI类型
#define TCFG_LED7_RUN_RAM                         0                // LED屏驱动跑RAM
#define TCFG_UI_LED7_ENABLE                       0                // LED7脚数码管屏
#define TCFG_TFT_LCD_DEV_SPI_HW_NUM               1                // LCD SPI口选择
#define TCFG_LRC_LYRICS_ENABLE                    0                // 歌词显示
#define LRC_ENABLE_SAVE_LABEL_TO_FLASH            0                // 保存歌词时间标签到flash
#define TCFG_LCD_OLED_ENABLE                      1                // OLED屏使能
#define TCFG_OLED_SPI_SSD1306_ENABLE              1                // SSD1306
#define TCFG_SPI_LCD_ENABLE                       0                // LCD彩屏使能
#define TCFG_LCD_SPI_ST7789V_ENABLE               0                // ST7789V
#define TCFG_LED7_PIN0                            IO_PORTA_04      // LED引脚0
#define TCFG_LED7_PIN1                            IO_PORTA_06      // LED引脚1
#define TCFG_LED7_PIN2                            IO_PORTA_07      // LED引脚2
#define TCFG_LED7_PIN3                            IO_PORTA_08      // LED引脚3
#define TCFG_LED7_PIN4                            IO_PORTA_09      // LED引脚4
#define TCFG_LED7_PIN5                            IO_PORTA_10      // LED引脚5
#define TCFG_LED7_PIN6                            IO_PORTA_11      // LED引脚6
#define TCFG_LCD_PIN_RESET                        IO_PORTC_03      // LCD RESET
#define TCFG_LCD_PIN_CS                           IO_PORTC_05      // LCD CS
#define TCFG_LCD_PIN_BL                           IO_PORTC_06      // LCD BLCKLIGHT
#define TCFG_LCD_PIN_DC                           IO_PORTC_04      // LCD DC
#define TCFG_LCD_PIN_EN                           NO_CONFIG_PORT   // LCD EN
#define TCFG_LCD_PIN_TE                           NO_CONFIG_PORT   // LCD TE
#endif // TCFG_UI_ENABLE
// ------------UI配置.json------------

// ------------蓝牙配置.json------------
#define TCFG_BT_NAME_SEL_BY_AD_ENABLE             0                // 蓝牙名(AD采样)

#define TCFG_BT_PAGE_TIMEOUT                      8                // 单次回连时间(s)
#define TCFG_BT_POWERON_PAGE_TIME                 30               // 开机回连超时(s)
#define TCFG_BT_TIMEOUT_PAGE_TIME                 120              // 超距断开回连超时(s)
#define TCFG_DUAL_CONN_INQUIRY_SCAN_TIME          120              // 开机可被发现时间 (s)
#define TCFG_DUAL_CONN_PAGE_SCAN_TIME             0                // 等待第二台连接时间(s)
#define TCFG_AUTO_SHUT_DOWN_TIME                  0                // 无连接关机时间(s)
#define TCFG_A2DP_DELAY_TIME_SBC                  300              // A2DP延时SBC(msec)
#define TCFG_A2DP_DELAY_TIME_SBC_LO               60               // A2DP低延时SBC(msec)
#define TCFG_A2DP_DELAY_TIME_AAC                  300              // A2DP延时AAC(msec)
#define TCFG_A2DP_DELAY_TIME_AAC_LO               100              // A2DP低延时AAC(msec)
#define TCFG_A2DP_ADAPTIVE_MAX_LATENCY            550              // A2DP自适应最大延时(msec)
#define TCFG_BT_VOL_SYNC_ENABLE                   0                // 音量同步
#define TCFG_BT_MUSIC_INFO_ENABLE                 0                // 歌曲信息显示
#define TCFG_BT_DISPLAY_BAT_ENABLE                0                // 电量显示
#define TCFG_BT_HFP_ONLY_DISPLAY_BAT_ENABLE       0                // 仅保留电量显示
#define TCFG_BT_DUAL_CONN_ENABLE                  0                // 一拖二
#define TCFG_BT_INBAND_RING                       0                // 手机铃声
#define TCFG_BT_PHONE_NUMBER_ENABLE               0                // 来电报号
#define TCFG_A2DP_PREEMPTED_ENABLE                0                // 音频抢播
#define TCFG_BT_SUPPORT_AAC                       0                // AAC
#define TCFG_BT_MSBC_EN                           0                // MSBC
#define TCFG_BT_SBC_BITPOOL                       53               // sbcBitPool
#define TCFG_AAC_BITRATE                          320000           // AAC码率
#define TCFG_BT_SUPPORT_LHDC                      0                // LHDC_V3/V4
#define TCFG_BT_SUPPORT_LHDC_V5                   0                // LHDC_V5
#define TCFG_BT_SUPPORT_LDAC                      0                // LDAC
#define TCFG_BT_SUPPORT_HFP                       0                // HFP
#define TCFG_BT_SUPPORT_AVCTP                     0                // AVRCP
#define TCFG_BT_SUPPORT_A2DP                      0                // A2DP
#define TCFG_BT_SUPPORT_HID                       0                // HID
#define TCFG_BT_SUPPORT_SPP                       0                // SPP
#define TCFG_BT_SUPPORT_PNP                       0                // PNP
#define TCFG_BT_SUPPORT_PBAP                      0                // PBAP
#define TCFG_BT_SUPPORT_MAP                       0                // MAP
#define TCFG_BT_BACKGROUND_ENABLE                 0                // 蓝牙后台
#define TCFG_BT_BACKGROUND_GOBACK                 0                // 蓝牙后台连接断开返回
#define TCFG_BT_BACKGROUND_DETECT_TIME            1940             // 音乐检测时间
#define CONFIG_BT_MODE                            0                // 模式选择
#define TCFG_NORMAL_SET_DUT_MODE                  0                // NORMAL模式下使能DUT测试

#define TCFG_USER_TWS_ENABLE                      0                // TWS
#if TCFG_USER_TWS_ENABLE
#define CONFIG_TWS_USE_COMMMON_ADDR               1                // MAC地址
#define TCFG_BT_TWS_PAIR_MODE                     CONFIG_TWS_PAIR_BY_AUTO // 配对方式
#define TCFG_BT_TWS_CHANNEL_SELECT                CONFIG_TWS_MASTER_AS_LEFT // 声道选择
#define TCFG_TWS_PAIR_TIMEOUT                     6                // 开机配对超时(s)
#define TCFG_TWS_CONN_TIMEOUT                     6                // 单次连接超时(s)
#define TCFG_TWS_PAIR_ALWAYS                      0                // 单台连手机也能进行配对
#define CONFIG_TWS_POWEROFF_SAME_TIME             0                // 同步关机
#define CONFIG_TWS_AUTO_PAIR_WITHOUT_UNPAIR       0                // TWS连接超时自动配对新音箱
#define TCFG_TWS_PAIR_BY_BOTH_SIDES               0                // 两边同时按配对键进入配对
#define TCFG_TWS_AUTO_ROLE_SWITCH_ENABLE          0                // 自动主从切换
#define TCFG_TWS_POWER_BALANCE_ENABLE             0                // 主从电量平衡
#define TCFG_LOCAL_TWS_ENABLE                     0                // TWS本地音乐转发
#define TCFG_LOCAL_TWS_SYNC_VOL                   0                // 本地音乐音量同步
#define TCFG_BACKGROUND_WITHOUT_EDR_CONNECT       0                // 后台关闭经典蓝牙连接
#endif // TCFG_USER_TWS_ENABLE

#define TCFG_BT_SNIFF_ENABLE                      0                // sniff
#if TCFG_BT_SNIFF_ENABLE
#define TCFG_SNIFF_CHECK_TIME                     2                // 检测时间
#define CONFIG_LRC_WIN_SIZE                       400              // LRC窗口初值
#define CONFIG_LRC_WIN_STEP                       400              // LRC窗口步进
#define CONFIG_OSC_WIN_SIZE                       400              // OSC窗口初值
#define CONFIG_OSC_WIN_STEP                       400              // OSC窗口步进
#endif // TCFG_BT_SNIFF_ENABLE

#define TCFG_USER_BLE_ENABLE                      1                // BLE
#if TCFG_USER_BLE_ENABLE
#define TCFG_BT_BLE_TX_POWER                      7                // 最大发射功率
#define TCFG_BT_BLE_BREDR_SAME_ADDR               0                // 和2.1同地址
#endif // TCFG_USER_BLE_ENABLE

#define TCFG_THIRD_PARTY_PROTOCOLS_ENABLE         1                // 第三方协议配置
#if TCFG_THIRD_PARTY_PROTOCOLS_ENABLE
#define TCFG_THIRD_PARTY_PROTOCOLS_SEL            0                // 第三方协议选择
#endif // TCFG_THIRD_PARTY_PROTOCOLS_ENABLE

#define TCFG_LE_AUDIO_APP_CONFIG                  0                // le_audio 应用选择
#define TCFG_LE_AUDIO_PLAY_LATENCY                30000            // le_audio延时（us）
// ------------蓝牙配置.json------------

// ------------公共配置.json------------
#define LE_AUDIO_CODEC_TYPE                       0x20000000       // 编解码格式
#define LE_AUDIO_CODEC_CHANNEL                    2                // 编解码声道数
#define LE_AUDIO_CODEC_FRAME_LEN                  100              // 帧持续时间
#define LE_AUDIO_CODEC_SAMPLERATE                 48000            // 采样率
#define LEA_TX_DEC_OUTPUT_CHANNEL                 37               // 发送端解码输出
#define LEA_RX_DEC_OUTPUT_CHANNEL                 37               // 接收端解码输出
#define TCFG_KBOX_1T3_MODE_EN                     0                // 1T3使能
#define TCFG_KBOX_1T3_WITH_TWS_MODE               3                // TWS+MIC连接方式
// ------------公共配置.json------------

// ------------BIS配置.json------------
#define LEA_BIG_CTRLER_TX_EN                      0                // 发送使能
#define LEA_BIG_CTRLER_RX_EN                      0                // 接收使能
#define LEA_BIG_CUSTOM_DATA_EN                    0                // 自定义数据同步
#define LEA_BIG_VOL_SYNC_EN                       0                // 音量同步
#define LEA_BIG_RX_CLOSE_EDR_EN                   0                // 接收端关EDR
#define LEA_LOCAL_SYNC_PLAY_EN                    0                // 本地同步播放
#define LEA_BIG_FIX_ROLE                          0                // 广播角色
// ------------BIS配置.json------------

// ------------CIS配置.json------------
#define LEA_CIG_CENTRAL_EN                        0                // 主机使能
#define LEA_CIG_PERIPHERAL_EN                     0                // 从机使能
#define LEA_CIG_CENTRAL_CLOSE_EDR_CONN            0                // 主机关闭EDR
#define LEA_CIG_PERIPHERAL_CLOSE_EDR_CONN         0                // 从机关闭EDR
#define LEA_CIG_KEY_EVENT_SYNC                    0                // 按键同步
#define LEA_CIG_FIX_ROLE                          1                // 连接角色
#define LEA_CIG_CONNECT_MODE                      2                // 连接方式
#define LEA_CIG_TRANS_MODE                        1                // 音频传输方式
// ------------CIS配置.json------------

// ------------升级配置.json------------
#define TCFG_UPDATE_ENABLE                        1                // 升级选择
#if TCFG_UPDATE_ENABLE
#define TCFG_UPDATE_STORAGE_DEV_EN                1                // 设备升级
#define TCFG_UPDATE_BLE_TEST_EN                   0                // ble蓝牙升级
#define TCFG_UPDATE_BT_LMP_EN                     1                // edr蓝牙升级
#define TCFG_TEST_BOX_ENABLE                      1                // 测试盒串口升级
#define TCFG_UPDATE_UART_IO_EN                    0                // 普通io串口升级
#define TCFG_UPDATE_UART_ROLE                     0                // 串口升级主从机选择
#define TCFG_UART_UPDATE_TX_PIN                   0                // TX_IO选择
#define TCFG_UART_UPDTAE_RX_PIN                   0                // RX_IO选择
#define CONFIG_UPDATE_JUMP_TO_MASK                0                // 升级维持io使能
#define CONFIG_SD_LATCH_IO                        PB02_PB08        // SD卡升级需要维持的IO
#define CONFIG_UPDATE_MUTIL_CPU_UART              0                // 多芯片交互升级
#define CONFIG_UPDATE_MACHINE_NUM                 0                // 唯一设备号
#define TCFG_UPDATE_MUTIL_CPU_MASTER              0                // 是否固定为主机
#define CONFIG_UPDATE_MUTIL_CPU_UART_TX_PIN       0                // TX_IO选择
#define CONFIG_UPDATE_MUTIL_CPU_UART_RX_PIN       0                // RX_IO选择
#endif // TCFG_UPDATE_ENABLE
// ------------升级配置.json------------

// ------------音频配置.json------------
#define TCFG_AUDIO_DAC_CONNECT_MODE               DAC_OUTPUT_MONO_L // 声道配置
#define TCFG_AUDIO_DAC_MODE                       DAC_MODE_H2_SINGLE // 输出方式
#define TCFG_AUDIO_DAC_LIGHT_CLOSE_ENABLE         0X0              // 轻量关闭
#define TCFG_AUDIO_VCM_CAP_EN                     0x1              // VCM电容
#define TCFG_AUDIO_DAC_POWER_ON_MODE              0X0              // DAC开启模式
#define TCFG_AUDIO_DAC_BUFFER_TIME_MS             100              // 缓冲长度（ms）
#define TCFG_AUDIO_DAC_IO_ENABLE                  0                // DAC IO
#define TCFG_AUDIO_DAC_PA_ISEL                    4                // 电流档位
#define TCFG_AUDIO_DAC_POWER_BOOST                0                // 音量增强模式
#define TCFG_AUDIO_L_CHANNEL_GAIN                 0x03             // L Channel
#define TCFG_AUDIO_R_CHANNEL_GAIN                 0x03             // R Channel
#define TCFG_AUDIO_DIGITAL_GAIN                   0                // Digital Gain

#define TCFG_AUDIO_ADC_ENABLE                     0                // ADC配置
#if TCFG_AUDIO_ADC_ENABLE
#define TCFG_AUDIO_MIC_LDO_VSEL                   5                // MIC LDO 电压
#define TCFG_AUDIO_MIC_LDO_ISEL                   2                // MIC LDO 电流
#define TCFG_ADC0_ENABLE                          0                // 使能
#define TCFG_ADC0_MODE                            0                // 模式
#define TCFG_ADC0_AIN_SEL                         1                // 输入端口
#define TCFG_ADC0_BIAS_SEL                        1                // 供电端口
#define TCFG_ADC0_BIAS_RSEL                       4                // MIC BIAS上拉电阻挡位
#define TCFG_ADC0_DCC_LEVEL                       8                // DCC 档位
#define TCFG_ADC0_POWER_IO                        0                // IO供电选择
#define TCFG_ADC1_ENABLE                          0                // 使能
#define TCFG_ADC1_MODE                            0                // 模式
#define TCFG_ADC1_AIN_SEL                         1                // 输入端口
#define TCFG_ADC1_BIAS_SEL                        2                // 供电端口
#define TCFG_ADC1_BIAS_RSEL                       4                // MIC BIAS上拉电阻挡位
#define TCFG_ADC1_DCC_LEVEL                       8                // DCC 档位
#define TCFG_ADC1_POWER_IO                        0                // IO供电选择
#define TCFG_ADC2_ENABLE                          0                // 使能
#define TCFG_ADC2_MODE                            0                // 模式
#define TCFG_ADC2_AIN_SEL                         1                // 输入端口
#define TCFG_ADC2_BIAS_SEL                        4                // 供电端口
#define TCFG_ADC2_BIAS_RSEL                       4                // MIC BIAS上拉电阻挡位
#define TCFG_ADC2_DCC_LEVEL                       8                // DCC 档位
#define TCFG_ADC2_POWER_IO                        0                // IO供电选择
#define TCFG_ADC3_ENABLE                          0                // 使能
#define TCFG_ADC3_MODE                            0                // 模式
#define TCFG_ADC3_AIN_SEL                         1                // 输入端口
#define TCFG_ADC3_BIAS_SEL                        8                // 供电端口
#define TCFG_ADC3_BIAS_RSEL                       4                // MIC BIAS上拉电阻挡位
#define TCFG_ADC3_DCC_LEVEL                       8                // DCC 档位
#define TCFG_ADC3_POWER_IO                        0                // IO供电选择
#endif // TCFG_AUDIO_ADC_ENABLE

#define TCFG_AUDIO_GLOBAL_SAMPLE_RATE             44100            // 全局采样率
#define TCFG_AEC_TOOL_ONLINE_ENABLE               0                // 手机APP在线调试
#define TCFG_AUDIO_CVP_SYNC                       0                // 通话上行同步
#define TCFG_AUDIO_DMS_DUT_ENABLE                 0                // 通话产测
#define TCFG_ESCO_DL_CVSD_SR_USE_16K              0                // 通话下行固定16K
#define TCFG_AUDIO_SMS_SEL                        SMS_DEFAULT      // 1mic算法选择
#define TCFG_AUDIO_DMS_GLOBAL_VERSION             DMS_GLOBAL_V200  // 2micDNS算法选择
#define TCFG_3MIC_MODE_SEL                        JLSP_3MIC_MODE2  // 3mic算法选择
#define TCFG_MUSIC_PLC_YPE                        0                // PLC类型选择
#define TCFG_KWS_VOICE_RECOGNITION_ENABLE         0                // 关键词检测KWS
#define TCFG_KWS_MIC_CHA_SELECT                   AUDIO_ADC_MIC_0  // 麦克风选择
#define TCFG_SMART_VOICE_ENABLE                   0                // 离线语音识别
#define TCFG_SMART_VOICE_USE_AEC                  0                // 回音消除使能
#define TCFG_SMART_VOICE_MIC_CH_SEL               AUDIO_ADC_MIC_0  // 麦克风选择
#define TCFG_AUDIO_KWS_LANGUAGE_SEL               KWS_CH           // 模式选择
#define TCFG_DEC_WAV_ENABLE                       0                // WAV
#define TCFG_DEC_MP3_ENABLE                       0                // MP3
#define TCFG_DEC_FLAC_ENABLE                      0                // FLAC
#define TCFG_DEC_WMA_ENABLE                       0                // WMA
#define TCFG_DEC_APE_ENABLE                       0                // APE
#define TCFG_DEC_AAC_ENABLE                       0                // AAC
#define TCFG_DEC_F2A_ENABLE                       0                // F2A
#define TCFG_DEC_WTG_ENABLE                       0                // WTG
#define TCFG_DEC_MTY_ENABLE                       0                // MTY
#define TCFG_DEC_WTS_ENABLE                       0                // WTS
#define TCFG_DEC_JLA_ENABLE                       0                // JLA
#define TCFG_ENC_MSBC_ENABLE                      0                // MSBC
#define TCFG_ENC_CVSD_ENABLE                      0                // CVSD
#define TCFG_ENC_JLA_ENABLE                       0                // JLA
#define TCFG_ENC_SBC_ENABLE                       0                // SBC
#define TCFG_ENC_AMR_ENABLE                       0                // AMR
#define TCFG_ENC_MP3_ENABLE                       0                // MP3
#define TCFG_ENC_MP3_TYPE                         0                // MP3格式选择
#define TCFG_ENC_ADPCM_ENABLE                     0                // ADPCM
#define TCFG_ENC_ADPCM_TYPE                       0                // ADPCM格式选择
#define TCFG_DATA_EXPORT_UART_TX_PORT             IO_PORT_DM       // 串口发送引脚
#define TCFG_DATA_EXPORT_UART_BAUDRATE            2000000          // 串口波特率
// ------------音频配置.json------------


// ------------JLStudio Auto generate------------
#define PROJECT_CONFIG_NAME                       ""               // 默认配置为空，非默认配置为配置名称
// ------------JLStudio Auto generate------------
#endif
