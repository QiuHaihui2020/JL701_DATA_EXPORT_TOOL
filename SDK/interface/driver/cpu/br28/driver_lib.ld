SECTIONS
{
    .data ALIGN(4):SUBALIGN(4)
    {
        driver_data_start = .;


        CLOCK_DATA_START = .;
        *(.clock_data)
        CLOCK_DATA_SIZE = ABSOLUTE(. - CLOCK_DATA_START);

        *(.debug_data)
        
		*(.uart_data)
		
		/*#include asm/power/ld/power_data.ld*/	
		*(.power_driver.data)

        driver_data_end = .;
        . = ALIGN(32);
    } > ram0

	.data_code ALIGN(4):SUBALIGN(4)
	{
	
		/*#include asm/power/ld/power_data_code.ld*/	
   		*(.power_driver.text.cache.L1)
	
	} > ram0

    .bss ALIGN(4):SUBALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)

		/* #include "asm/power/ld/power_bss.ld"*/
		*(.power_driver.data.bss)

        *(.uart_bss)

        *(.sd_var)
	
	. = ALIGN(4);
		driver_bss_end = .;
    } > ram0

    .text  ALIGN(4):SUBALIGN(4)
    {
        driver_code_start = .;

        *(.fm_code)

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);

   		/*#include "asm/power/ld/power_test.ld"*/	
		*(.power_driver.text)
		*(.power_driver.text.const)

		. = ALIGN(4);
		lp_target_begin = .;
	    PROVIDE(lp_target_begin = .);
	    KEEP(*(.lp_target))
	    lp_target_end = .;
	    PROVIDE(lp_target_end = .);

		. = ALIGN(4);
	    lp_request_begin = .;
	    PROVIDE(lp_request_begin = .);
	    KEEP(*(.lp_request))
	    lp_request_end = .;
	    PROVIDE(lp_request_end = .);

		. = ALIGN(4);
        deepsleep_target_begin = .;
        PROVIDE(deepsleep_target_begin = .);
        KEEP(*(.deepsleep_target))
        deepsleep_target_end = .;
        PROVIDE(deepsleep_target_end = .);

		. = ALIGN(4);
		p2m_msg_handler_begin = .;
		PROVIDE(p2m_msg_handler_begin = .);
		KEEP(*(.p2m_msg_handler))
		PROVIDE(p2m_msg_handler_end = .);
		p2m_msg_handler_end = .;

		. = ALIGN(4);
		phw_begin = .;
		PROVIDE(phw_begin = .);
		KEEP(*(.phw_operation))
		PROVIDE(phw_end = .);
		phw_end = .;

     	*(.uart_code)
        *(.uart_const)

        driver_code_end = .;
		. = ALIGN(4);
    } > code0

    /*代码统计 Code & RAM */
    DRIVER_RAM_TOTAL   = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL  = (driver_code_end - driver_code_start);
}
