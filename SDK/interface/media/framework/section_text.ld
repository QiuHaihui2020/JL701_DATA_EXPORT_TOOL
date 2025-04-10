
        . = ALIGN(4);
        stream_node_adapter_begin = .;
        KEEP(*(.stream_node_adapter))
        stream_node_adapter_end = .;

        . = ALIGN(4);
        source_node_plug_begin = .;
        KEEP(*(.source_node_plug))
        source_node_plug_end = .;

        . = ALIGN(4);
        decoder_plug_begin = .;
        KEEP(*(.decoder_plug))
        decoder_plug_end = .;

        . = ALIGN(4);
        encoder_plug_begin = .;
        KEEP(*(.encoder_plug))
        encoder_plug_end = .;

        . = ALIGN(4);
        audio_package_begin = .;
        KEEP(*(.audio_package_handler))
        audio_package_end = .;
