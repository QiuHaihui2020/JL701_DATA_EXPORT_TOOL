SECTIONS {
    .data ALIGN(32): SUBALIGN(4)
    {
        media_lib_data_begin = .;
        . = ALIGN(4);
#include "media/media_lib_data.ld"
        media_lib_data_end = .;
        media_lib_data_size = media_lib_data_end - media_lib_data_begin ;
        . = ALIGN(4);
    } > ram0

    .bss(NOLOAD): SUBALIGN(4)
    {
        media_lib_bss_begin = .;
        . = ALIGN(4);
#include "media/media_lib_bss.ld"
        media_lib_bss_end = .;
        media_lib_bss_size = media_lib_bss_end - media_lib_bss_begin ;
        . = ALIGN(4);
    } > ram0

    .text ALIGN(32): SUBALIGN(4)
    {
        media_lib_bss_begin = .;
        . = ALIGN(4);
#include "media/media_lib_text.ld"
        media_lib_bss_end = .;
        media_lib_bss_size = media_lib_bss_end - media_lib_bss_begin ;
        . = ALIGN(4);
    } > code0
}

