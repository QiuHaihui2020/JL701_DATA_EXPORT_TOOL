// *INDENT-OFF*
#include "app_config.h"
#include "audio_config_def.h"

/* =================  BR28 SDK memory ========================
 _______________ ___ 0x1A0000(176K)
|   isr base    |
|_______________|___ _IRQ_MEM_ADDR(size = 0x100)
|rom export ram |
|_______________|
|    update     |
|_______________|___ RAM_LIMIT_H
|     HEAP      |
|_______________|___ data_code_pc_limit_H
| audio overlay |
|_______________|
|   data_code   |
|_______________|___ data_code_pc_limit_L
|     bss       |
|_______________|
|     data      |
|_______________|
|   irq_stack   |
|_______________|
|   boot info   |
|_______________|
|     TLB       |
|_______________|0 RAM_LIMIT_L

 =========================================================== */

#include "maskrom_stubs.ld"

EXTERN(
_start
#include "sdk_used_list.c"
);

UPDATA_SIZE     = 0x80;
UPDATA_BEG      = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0x1A0000;

RAM_LIMIT_L     = 0x100000;
RAM_LIMIT_H     = UPDATA_BEG;
PHY_RAM_SIZE    = RAM_LIMIT_H - RAM_LIMIT_L;

//from mask export
ISR_BASE       = _IRQ_MEM_ADDR;
ROM_RAM_SIZE   = _MASK_MEM_SIZE;
ROM_RAM_BEG    = _MASK_MEM_BEGIN;

RAM0_BEG 		= RAM_LIMIT_L + (128K * TCFG_LOWPOWER_RAM_SIZE);
RAM0_END 		= RAM_LIMIT_H;
RAM0_SIZE 		= RAM0_END - RAM0_BEG;

RAM1_BEG 		= RAM_LIMIT_L;
RAM1_END 		= RAM0_BEG;
RAM1_SIZE 		= RAM1_END - RAM1_BEG;

#ifdef CONFIG_NEW_CFG_TOOL_ENABLE
CODE_BEG        = 0x6000100;
#else
CODE_BEG        = 0x6000120;
#endif

//=============== About BT RAM ===================
//CONFIG_BT_RX_BUFF_SIZE = (1024 * 18);

MEMORY
{
	code0(rx)    	  : ORIGIN = CODE_BEG,  LENGTH = CONFIG_FLASH_SIZE
	ram0(rwx)         : ORIGIN = RAM0_BEG,  LENGTH = RAM0_SIZE

    //ram1 - 用于volatile-heap
	//ram1(rwx)         : ORIGIN = RAM1_BEG,  LENGTH = RAM1_SIZE

	psr_ram(rwx)        : ORIGIN = 0x2000000,  LENGTH = 8*1024*1024
}


ENTRY(_start)

SECTIONS
{
    . = ORIGIN(ram0);
	//TLB 起始需要16K 对齐；
    .mmu_tlb ALIGN(0x10000):
    {
        *(.mmu_tlb_segment);
    } > ram0

	.boot_info ALIGN(32):
	{
		*(.boot_info)
        . = ALIGN(32);
	} > ram0

	.irq_stack ALIGN(32):
    {
        _cpu0_sstack_begin = .;
        *(.cpu0_stack)
        _cpu0_sstack_end = .;
		. = ALIGN(4);

        _cpu1_sstack_begin = .;
        *(.cpu1_stack)
        _cpu1_sstack_end = .;
		. = ALIGN(4);

    } > ram0

	.data ALIGN(32):SUBALIGN(4)
	{
		//cpu start
        . = ALIGN(4);
        *(.data_magic)

        *(.data*)
        *(.*.data)

         . = ALIGN(4);

        app_mode_begin = .;
		KEEP(*(.app_mode))
        app_mode_end = .;

        local_tws_ops_begin = .;
		KEEP(*(.local_tws))
        local_tws_ops_end = .;

		. = ALIGN(4);
        #include "btctrler/crypto/data.ld"

        *(.debug_const)

		. = ALIGN(4);

	} > ram0

	.bss ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);

        . = ALIGN(4);
        *(.bss)
        *(.*.data.bss)
        . = ALIGN(4);
        *(.*.data.bss.nv)

        . = ALIGN(4);
        *(.volatile_ram)

		*(.btstack_pool)

        *(.mem_heap)
		*(.memp_memory_x)

        . = ALIGN(4);
        *(.usb.data.bss.exchange)

        . = ALIGN(4);
		*(.non_volatile_ram)

        #include "btctrler/crypto/bss.ld"
        . = ALIGN(32);

    } > ram0

	.data_code ALIGN(32):SUBALIGN(4)
	{
		data_code_pc_limit_begin = .;
		#include "media/media_lib_data_text.ld"
		*(.flushinv_icache)
        *(.cache)
        *(.os_critical_code)
        *(.os.text*)
        *(.volatile_ram_code)
        *(.chargebox_code)
        *(.movable.stub.1)
        *(.*.text.cache.L1)
        *(.*.text.const.cache.L2)
		*(.jlstream.text.cache.L2)
        *(.ui_ram)
        *(.math_fast_funtion_code)

         . = ALIGN(4);
         __fm_movable_slot_start = .;
         *(.movable.slot.1);
         __fm_movable_slot_end = .;

         __bt_movable_slot_start = .;
         *(.movable.slot.2);
         __bt_movable_slot_end = .;

         . = ALIGN(4);
         __aac_movable_slot_start = .;
         *(.movable.slot.3)
         __aac_movable_slot_end = .;

         . = ALIGN(4);
         __aec_movable_slot_start = .;
         *(.movable.slot.4)
         __aec_movable_slot_end = .;

		 . = ALIGN(4);
         __mic_eff_movable_slot_start = .;
         *(.movable.slot.5)
         __mic_eff_movable_slot_end = .;

         . = ALIGN(4);
        *(.movable.stub.*)

		. = ALIGN(4);
        *(.spi_code)
		. = ALIGN(4);

        *(.debug_code)

#if  (TCFG_LED7_RUN_RAM)
		. = ALIGN(4);
        *(.gpio.text.cache.L2)
		*(.LED_code)
		*(.LED_const)
		. = ALIGN(4);
#endif
	} > ram0

	.movable_code ALIGN(32):SUBALIGN(4)
    {

    } > ram0

	__report_overlay_begin = .;
    //没有动态加载时使用overlay
#ifndef CONFIG_CODE_MOVABLE_ENABLE
    #include "app_overlay.ld"
#endif

	data_code_pc_limit_end = .;
	__report_overlay_end = .;

	.debug_record_info ALIGN(32):
	{
        // 需要避免与uboot和maskrom冲突
        *(.debug_record)
        . = ALIGN(4);
	} > ram0

	_HEAP_BEGIN = . ;
	_HEAP_END = RAM0_END;


    . = ORIGIN(psr_ram);
	.psr_data_code ALIGN(32):
	{
	    *(.psram_data_code)
		. = ALIGN(4);
	} > psr_ram

	.psr_bss_code ALIGN(32):
	{
		. = ALIGN(4);
	} > psr_ram

    . = ORIGIN(code0);
    .text ALIGN(4):SUBALIGN(4)
    {
        PROVIDE(text_rodata_begin = .);

        *(.startup.text)

		*(.text)
		*(.*.text)
		*(.*.text.const)
        *(.*.text.const.cache.L2)
        . = ALIGN(32);
        _SPI_CODE_START = . ;
        *(.sfc.text.cache.L2*)
        . = ALIGN(4);
        _SPI_CODE_END = . ;
        . = ALIGN(4);

        *(.*.text.cache.L2*)
		. = ALIGN(4);
	    update_target_begin = .;
	    PROVIDE(update_target_begin = .);
	    KEEP(*(.update_target))
	    update_target_end = .;
	    PROVIDE(update_target_end = .);
		. = ALIGN(4);

		*(.LOG_TAG_CONST*)
        *(.rodata*)

		. = ALIGN(4); // must at tail, make rom_code size align 4

        PROVIDE(text_rodata_end = .);

        clock_critical_handler_begin = .;
        KEEP(*(.clock_critical_txt))
        clock_critical_handler_end = .;

        chargestore_handler_begin = .;
        KEEP(*(.chargestore_callback_txt))
        chargestore_handler_end = .;

        . = ALIGN(4);
        app_msg_handler_begin = .;
		KEEP(*(.app_msg_handler))
        app_msg_handler_end = .;

        . = ALIGN(4);
        app_msg_prob_handler_begin = .;
		KEEP(*(.app_msg_prob_handler))
        app_msg_prob_handler_end = .;

		. = ALIGN(4);
		app_charge_handler_begin = .;
		KEEP(*(.app_charge_handler.0))
		KEEP(*(.app_charge_handler.1))
        app_charge_handler_end = .;

        . = ALIGN(4);
        scene_ability_begin = .;
		KEEP(*(.scene_ability))
        scene_ability_end = .;

         #include "media/framework/section_text.ld"

		. = ALIGN(4);
		tool_interface_begin = .;
		KEEP(*(.tool_interface))
		tool_interface_end = .;

		. = ALIGN(4);
        effects_online_adjust_begin = .;
        KEEP(*(.effects_online_adjust))
        effects_online_adjust_end = .;

        . = ALIGN(4);
        tws_tone_cb_begin = .;
        KEEP(*(.tws_tone_callback))
        tws_tone_cb_end = .;

        . = ALIGN(4);
        vm_reg_id_begin = .;
        KEEP(*(.vm_manage_id_text))
        vm_reg_id_end = .;

        . = ALIGN(4);
        wireless_trans_ops_begin = .;
        KEEP(*(.wireless_trans))
        wireless_trans_ops_end = .;

        . = ALIGN(4);
        wireless_data_callback_func_begin = .;
        KEEP(*(.wireless_call_sync))
        wireless_data_callback_func_end = .;

        . = ALIGN(4);
        wireless_sync_call_func_begin = .;
        KEEP(*(.wireless_sync_call_func))
        wireless_sync_call_func_end = .;

        . = ALIGN(4);
        wireless_custom_data_trans_stub_begin = .;
        KEEP(*(.wireless_custom_data_trans_stub))
        wireless_custom_data_trans_stub_end = .;

		. = ALIGN(32);
        _TEST_CODE_START = . ;
        *(.test_code)
		. = ALIGN(32);
        _TEST_CODE_END = . ;
		. = ALIGN(32);

        . = ALIGN(4);

#if (!TCFG_LED7_RUN_RAM)
		. = ALIGN(4);
        *(.gpio.text.cache.L2)
		*(.LED_code)
		*(.LED_const)
#endif
		. = ALIGN(4);

        __fm_movable_region_start = .;
        *(.movable.region.1)
        __fm_movable_region_end = .;
        __fm_movable_region_size = ABSOLUTE(__fm_movable_region_end - __fm_movable_region_start);

		. = ALIGN(4);
        __bt_movable_region_start = .;
        *(.movable.region.2)
        __bt_movable_region_end = .;
        __bt_movable_region_size = ABSOLUTE(__bt_movable_region_end - __bt_movable_region_start);

		. = ALIGN(4);
        __aac_movable_region_start = .;
        *(.movable.region.3)
        __aac_movable_region_end = .;
        __aac_movable_region_size = ABSOLUTE(__aac_movable_region_end - __aac_movable_region_start);
        . = ALIGN(4);
        *(.bt_aac_dec_const)
        *(.bt_aac_dec_sparse_const)

		. = ALIGN(4);
        __aec_movable_region_start = .;
        *(.movable.region.4)
        __aec_movable_region_end = .;
        __aec_movable_region_size = ABSOLUTE(__aec_movable_region_end - __aec_movable_region_start);

		. = ALIGN(4);
        __mic_eff_movable_region_start = .;
        *(.movable.region.5)
        __mic_eff_movable_region_end = .;
        __mic_eff_movable_region_size = ABSOLUTE(__mic_eff_movable_region_end - __mic_eff_movable_region_start);

		/********maskrom arithmetic ****/
        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.bfilt_code)
        *(.bfilt_const)
		/********maskrom arithmetic end****/

		. = ALIGN(4);
		__VERSION_BEGIN = .;
        KEEP(*(.sys.version))
        __VERSION_END = .;

        *(.noop_version)

		. = ALIGN(4);
         __a2dp_text_cache_L2_start = .;
         *(.movable.region.1);
         . = ALIGN(4);
         __a2dp_text_cache_L2_end = .;
         . = ALIGN(4);
        #include "btctrler/crypto/text.ld"
        #include "btctrler/btctler_lib_text.ld"

        . = ALIGN(4);
		_record_handle_begin = .;
		PROVIDE(record_handle_begin = .);
		KEEP(*(.debug_record_handle_ops))
		_record_handle_end = .;
		PROVIDE(record_handle_end = .);

		. = ALIGN(32);
	  } > code0
}

#include "app.ld"
#include "update/update.ld"
#include "btstack/btstack_lib.ld"
//#include "btctrler/port/br28/btctler_lib.ld"
#include "driver/cpu/br28/driver_lib.ld"
#include "utils/utils_lib.ld"
#include "cvp/audio_cvp_lib.ld"
#include "media/media_lib.c"
#if TCFG_UI_ENABLE && !TCFG_UI_LED7_ENABLE
#include "ui/jl_ui/ui/ui.ld"
#endif

#if TCFG_JLSTREAM_TURBO_ENABLE
#define INCLUDE_FROM_LD
#include "media/framework/movable_node_section.c"
#endif

#include "system/port/br28/system_lib.ld" //Note: 为保证各段对齐, 系统ld文件必须放在最后include位置

//================== Section Info Export ====================//
text_begin  = ADDR(.text);
text_size   = SIZEOF(.text);
text_end    = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");

bss_begin = ADDR(.bss);
bss_size  = SIZEOF(.bss);
bss_end    = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size =  SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");

/* moveable_slot_addr = ADDR(.moveable_slot); */
/* moveable_slot_begin = data_begin + data_size; */
/* moveable_slot_size =  SIZEOF(.moveable_slot); */

data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size =  SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");

//没有使用动态加载时使用overlay
#ifndef CONFIG_CODE_MOVABLE_ENABLE
//================ OVERLAY Code Info Export ==================//
aec_addr = ADDR(.overlay_aec);
aec_begin = data_code_begin + data_code_size;
aec_size =  SIZEOF(.overlay_aec);

aac_addr = ADDR(.overlay_aac);
aac_begin = aec_begin + aec_size;
aac_size =  SIZEOF(.overlay_aac);

psr_data_code_begin = aac_begin + aac_size;
#else
psr_data_code_begin = data_code_begin + data_code_size;
#endif
psr_data_code_addr = ADDR(.psr_data_code);
psr_data_code_size =  SIZEOF(.psr_data_code);

/*
lc3_addr = ADDR(.overlay_lc3);
lc3_begin = aac_begin + aac_size;
lc3_size = SIZEOF(.overlay_lc3);
*/

/* moveable_addr = ADDR(.overlay_moveable) ; */
/* moveable_size = SIZEOF(.overlay_moveable) ; */
//===================== HEAP Info Export =====================//

ASSERT(CONFIG_FLASH_SIZE > text_size,"check sdk_config.h CONFIG_FLASH_SIZE < text_size");
ASSERT(_HEAP_BEGIN > bss_begin,"_HEAP_BEGIN < bss_begin");
ASSERT(_HEAP_BEGIN > data_addr,"_HEAP_BEGIN < data_addr");
ASSERT(_HEAP_BEGIN > data_code_addr,"_HEAP_BEGIN < data_code_addr");
//ASSERT(_HEAP_BEGIN > moveable_slot_addr,"_HEAP_BEGIN < moveable_slot_addr");
//ASSERT(_HEAP_BEGIN > __report_overlay_begin,"_HEAP_BEGIN < __report_overlay_begin");

PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

ASSERT(MALLOC_SIZE  >= 0x8000, "heap space too small !")

//============================================================//
//=== report section info begin:
//============================================================//
report_text_beign = ADDR(.text);
report_text_size = SIZEOF(.text);
report_text_end = report_text_beign + report_text_size;

report_mmu_tlb_begin = ADDR(.mmu_tlb);
report_mmu_tlb_size = SIZEOF(.mmu_tlb);
report_mmu_tlb_end = report_mmu_tlb_begin + report_mmu_tlb_size;

report_boot_info_begin = ADDR(.boot_info);
report_boot_info_size = SIZEOF(.boot_info);
report_boot_info_end = report_boot_info_begin + report_boot_info_size;

report_irq_stack_begin = ADDR(.irq_stack);
report_irq_stack_size = SIZEOF(.irq_stack);
report_irq_stack_end = report_irq_stack_begin + report_irq_stack_size;

report_data_begin = ADDR(.data);
report_data_size = SIZEOF(.data);
report_data_end = report_data_begin + report_data_size;

report_bss_begin = ADDR(.bss);
report_bss_size = SIZEOF(.bss);
report_bss_end = report_bss_begin + report_bss_size;

report_data_code_begin = ADDR(.data_code);
report_data_code_size = SIZEOF(.data_code);
report_data_code_end = report_data_code_begin + report_data_code_size;

report_overlay_begin = __report_overlay_begin;
report_overlay_size = __report_overlay_end - __report_overlay_begin;
report_overlay_end = __report_overlay_end;

report_heap_beign = _HEAP_BEGIN;
report_heap_size = _HEAP_END - _HEAP_BEGIN;
report_heap_end = _HEAP_END;

BR28_PHY_RAM_SIZE = PHY_RAM_SIZE;
BR28_SDK_RAM_SIZE = report_mmu_tlb_size + \
					report_boot_info_size + \
					report_irq_stack_size + \
					report_data_size + \
					report_bss_size + \
					report_overlay_size + \
					report_data_code_size + \
					report_heap_size;
//============================================================//
//=== report section info end
//============================================================//

