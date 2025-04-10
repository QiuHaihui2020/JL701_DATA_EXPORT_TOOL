
#ifdef INCLUDE_FROM_SECTION_C
57086:
.drc.text.cache.L2.code

60799:
.advaudio_plc.text.cache.L2.run
.esco_audio_plc.text.cache.L2.run

#endif


#ifdef INCLUDE_FROM_LD

SECTIONS {
    .data_code ALIGN(32): SUBALIGN(4)
    {
        . = ALIGN(4);
        __wdrc_slot_begin = .;
        *(.movable.slot.57086)
        __wdrc_slot_end = .;

        . = ALIGN(4);
        __plc_slot_begin = .;
        *(.movable.slot.60799)
        __plc_slot_end = .;

        . = ALIGN(4);
    } > ram0



    .movable_code ALIGN(32): SUBALIGN(4)
    {
        . = ALIGN(4);
        __jlstream_turbo_ram_begin = .;
        . = . + TCFG_JLSTREAM_TURBO_RAM_SIZE_KB * 1024;
        __jlstream_turbo_ram_end = .;
        __jlstream_turbo_ram_size = . - __jlstream_turbo_ram_begin;
    } > ram0




    .text ALIGN(4): SUBALIGN(4)
    {
        . = ALIGN(4);
        __wdrc_code_begin = .;
        *(.movable.region.57086)
        . = ALIGN(4);
        __wdrc_code_end = .;


        . = ALIGN(4);
        __plc_code_begin = .;
        *(.movable.region.60799)
        . = ALIGN(4);
        __plc_code_end = .;


    } > code0

}
#endif

