#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_system_config.data.bss")
#pragma data_seg(".lib_system_config.data")
#pragma const_seg(".lib_system_config.text.const")
#pragma code_seg(".lib_system_config.text")
#endif
/*********************************************************************************************
    *   Filename        : lib_system_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 15:22

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/

#include "app_config.h"
#include "system/includes.h"


///打印是否时间打印信息
const int config_printf_time         = 1;

///异常中断，asser打印开启
#if CONFIG_DEBUG_ENABLE
const int config_asser         = TCFG_EXCEPTION_LOG_ENABLE;  // 1:使能异常打印; 2:追加额外调试信息(dump另外一个CPU寄存器信息)
const int config_exception_reset_enable = TCFG_EXCEPTION_RESET_ENABLE;
const int CONFIG_LOG_OUTPUT_ENABLE = 1;
#else
const int config_asser         = 0;
const int config_exception_reset_enable = 1;
const int CONFIG_LOG_OUTPUT_ENABLE = 0;
#endif

//================================================//
//                 异常信息记录使能               //
//================================================//
//注意: 当config_asser变量为0时才有效.
#if	(defined TCFG_CONFIG_DEBUG_RECORD_ENABLE && TCFG_CONFIG_DEBUG_RECORD_ENABLE)
const int config_debug_exception_record = (!config_asser) && 1; 	//异常记录功能总开关
const int config_debug_exception_record_dump_info = (!config_asser) && 1; 		//小机上电输出异常信息使能
const int config_debug_exception_record_p11 = (!config_asser) && 1; 			//P11异常信息使能
const int config_debug_exception_record_stack = (!config_asser) && 1; 			//堆栈异常信息使能
const int config_debug_exception_record_ret_instrcion = (!config_asser) && 1; 	//指令数据异常信息使能
#else /* #if	(define CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE) */
const int config_debug_exception_record = 0; 				//异常记录功能总开关
const int config_debug_exception_record_dump_info = 0; 		//小机上电输出异常信息使能
const int config_debug_exception_record_p11 = 0; 			//P11异常信息使能
const int config_debug_exception_record_stack = 0; 			//堆栈异常信息使能
const int config_debug_exception_record_ret_instrcion = 0; 	//指令数据异常信息使能
#endif /* #if (define CONFIG_DEBUG_RECORD_ENABLE && CONFIG_DEBUG_RECORD_ENABLE) */

//================================================//
//当hsb的频率小于等于48MHz时，在否自动关闭所有pll //
//================================================//
const int clock_tree_auto_disable_all_pll = 0;

//================================================//
//                  SDFILE 精简使能               //
//================================================//
const int SDFILE_VFS_REDUCE_ENABLE = 1;

//================================================//
//              UI版本和CPU类型  				  //
// 使用一个8bit值表示
// 	高四位表示UI类型，0表示点阵屏，1表示彩屏
// 	低四位表示CPU，0表示BR23和BR27，1表示BR28
//================================================//
#if ((defined CONFIG_CPU_BR23) || (defined CONFIG_CPU_BR27))
#if (defined TCFG_DOT_LCD_ENABLE)
const int JLUI_TYPE_AND_VERSION = 0x00;
#else
const int JLUI_TYPE_AND_VERSION = 0x10;
#endif

#else
#if (defined TCFG_DOT_LCD_ENABLE)
const int JLUI_TYPE_AND_VERSION = 0x01;

#else
const int JLUI_TYPE_AND_VERSION = 0x11;
#endif
#endif

//================================================//
//                  dev使用异步读使能             //
//================================================//
#ifdef TCFG_DEVICE_BULK_READ_ASYNC_ENABLE
const int device_bulk_read_async_enable = 0;
#else
const int device_bulk_read_async_enable = 0;
#endif


//================================================//
//                  UI 							  //
//================================================//
const int SCALE_EFFECT_WITHOUT_PSRAM_ENABLE     = 0;
const int ENABLE_LUA_VIRTUAL_MACHINE = 0;
const int UI_DATA_STORE_IN_NORFLASH = 0;

const int ARABIC_MODE_SWITCH      = 0;
const int HEBREW_MODE_SWITCH      = 0;
const int THAI_MODE_SWITCH        = 0;
const int MIXRIGHT_MODE_SWITCH    = 0;
const int MIXLEFT_MODE_SWITCH     = 0;
const int MYANMAR_MODE_SWITCH     = 0;
const int BENGALI_MODE_SWITCH     = 0;
const int KHMER_MODE_SWITCH       = 0;
const int INDIC_MODE_SWITCH       = 0;
const int TIBETAN_MODE_SWITCH     = 0;
const int FONT_UNIC_SWITCH        = 1;
const int JPEG_DECODE_TIMEOUT	  = 20;	//20-50ms

#if (TCFG_UI_ENABLE)
const int ENABLE_JL_UI_FRAME = 1;
#else
const int ENABLE_JL_UI_FRAME = 0;
#endif

//================================================//
//              是否使用PSRAM UI  				  //
//================================================//
#if (TCFG_PSRAM_DEV_ENABLE)
const int ENABLE_PSRAM_UI_FRAME = 1;
#else
const int ENABLE_PSRAM_UI_FRAME = 0;
#endif

//================================================//
//          不可屏蔽中断使能配置(UNMASK_IRQ)      //
//================================================//
const int CONFIG_CPU_UNMASK_IRQ_ENABLE = 1;


//================================================//
//          heap内存记录功能                      //
//          使能建议设置为256（表示最大记录256项）//
//          通过void mem_unfree_dump()输出        //
//================================================//
const u32 CONFIG_HEAP_MEMORY_TRACE = 0;

//================================================//
//   timer_no_response打印对应任务堆栈            //
//================================================//
const u32 CONFIG_TIMER_DUMP_TASK_STACK = 0;

//================================================//
//                  FS功能控制 					  //
//================================================//
const int FATFS_WRITE = 1; // 控制fatfs写功能开关。
const int FILT_0SIZE_ENABLE = 1; //是否过滤0大小文件
const int FATFS_LONG_NAME_ENABLE = 1; //是否支持长文件名
const int FATFS_RENAME_ENABLE = 1; //是否支持重命名
const int FATFS_FGET_PATH_ENABLE = 1; //是否支持获取路径
const int FATFS_SAVE_FAT_TABLE_ENABLE = 1; //是否支持seek加速

const int FATFS_SUPPORT_OVERSECTOR_RW = 1; //是否支持超过一个sector向设备拿数
const int FATFS_TIMESORT_TURN_ENABLE = 1; //按时排序翻转，由默认从小到大变成从大到小
const int FATFS_TIMESORT_NUM = 128; //按时间排序,记录文件数量, 每个占用14 byte

const int FILE_TIME_HIDDEN_ENABLE = 0; //创建文件是否隐藏时间
const int FILE_TIME_USER_DEFINE_ENABLE = 1;//用户自定义时间，每次创建文件前设置，如果置0 需要确定芯片是否有RTC功能。

const int FATFS_SUPPORT_WRITE_SAVE_MEANTIME = 0; //每次写同步目录项使能，会降低连续写速度。

const int FATFS_SUPPORT_WRITE_CUTOFF = 1; //支持fseek截断文件。 打开后fseek后指针位置会决定文件大小
const int FATFS_RW_MAX_CACHE = 64 * 1024; //设置读写申请的最大cache大小 .note: 小于512会被默认不生效

const int FATFS_GET_SPACE_USE_RAM = 0;//32 * 1024;  //获取剩余容量使用大Buf缓存,加快速度, 必须512倍数

const int FATFS_DEBUG_FAT_TABLE_DIR_ENTRY = 0; //设置debugfat表和目录项写数据

const int VIRFAT_FLASH_ENABLE = 0; //精简jifat代码,不使用。


//================================================//
//phy_malloc碎片整理使能:            			  //
//配置为0: phy_malloc申请不到不整理碎片           //
//配置为1: phy_malloc申请不到会整理碎片           //
//配置为2: phy_malloc申请总会启动一次碎片整理     //
//================================================//
const int PHYSIC_MALLOC_DEFRAG_ENABLE = 1;

//============================================================================//
//配置malloc时使用psram还是ram:        			                              //
//0x00 ~ 0xFFFFFFFF:当malloc内存大于CONFIG_MALLOC_PSRAM_LEVEL则分配psram的内存//
//注意:需要使能psram并且psram不经过mmu映射                                    //
//============================================================================//
const unsigned int CONFIG_MALLOC_PSRAM_LEVEL = (500 * 1024);  //30K

// 是否使能 dlog 功能
#ifdef TCFG_DEBUG_DLOG_ENABLE
const int config_dlog_enable = TCFG_DEBUG_DLOG_ENABLE;
const int config_dlog_reset_erase_enable = TCFG_DEBUG_DLOG_RESET_ERASE;
#else
const int config_dlog_enable = 0;
const int config_dlog_reset_erase_enable = 0;
#endif

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_SYS_TMR  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_SYS_TMR  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_SYS_TMR  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_JLFS  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_JLFS  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_JLFS  = CONFIG_DEBUG_LIB(TRUE);

//FreeRTOS
const char log_tag_const_v_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_PORT  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_PORT  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_PORT  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_KTASK  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_KTASK  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_KTASK  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_uECC  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_uECC  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_uECC  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_HEAP_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_HEAP_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_w_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_HEAP_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_V_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_V_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_V_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_P_MEM  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_P_MEM  = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_P_MEM  = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_P_MEM_C = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_P_MEM_C = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_P_MEM_C = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_P_MEM_C = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_P_MEM_C = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_PSRAM_HEAP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_PSRAM_HEAP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_PSRAM_HEAP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_PSRAM_HEAP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_PSRAM_HEAP AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_DEBUG_RECORD = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_DEBUG_RECORD = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_d_DEBUG_RECORD = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_DEBUG_RECORD = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_DEBUG_RECORD = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_DLOG  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_DLOG  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_DLOG  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_DLOG  = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_e_DLOG  = CONFIG_DEBUG_LIB(FALSE);

