SECTIONS
{
    .data : ALIGN(4)
    {
        utils_data_start = .;
        *(.utils_data)
        utils_data_end = .;
		. = ALIGN(4);

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        utils_bss_start = .;
        *(.utils_bss)
        utils_bss_end = .;
		. = ALIGN(4);

    } > ram0

    .text : ALIGN(4)
    {
        utils_text_start = .;
        *(.utils_text)

		. = ALIGN(4);
		vfs_ops_begin = .;
		KEEP(*(.vfs_operations))
		vfs_ops_end = .;

		. = ALIGN(4);
	    _resfile_ops_begin = .;
		PROVIDE(resfile_ops_begin = .);
	    KEEP(*(.resfile_ops))
	    _resfile_ops_end = .;
	    PROVIDE(resfile_ops_end = .);

        utils_text_end = .;
		. = ALIGN(4);
    } > code0

	.data_code ALIGN(4):
	{
		. = ALIGN(4);
    } > ram0
}

