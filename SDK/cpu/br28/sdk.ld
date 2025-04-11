




  lp_signature_set = ABSOLUTE(0x1fd6c);
  memmem = ABSOLUTE(0x1fd70);
  memcpy = ABSOLUTE(0x1fd74);
  memmove = ABSOLUTE(0x1fd78);
  memcmp = ABSOLUTE(0x1fd7c);
  memset = ABSOLUTE(0x1fd80);
  strcmp = ABSOLUTE(0x1fd84);
  strcpy = ABSOLUTE(0x1fd88);
  strlen = ABSOLUTE(0x1fd8c);
  strncmp = ABSOLUTE(0x1fd90);
  strstr = ABSOLUTE(0x1fd94);
  strchr = ABSOLUTE(0x1fd98);
  __divdi3 = ABSOLUTE(0x1fd9c);
  __udivmoddi4 = ABSOLUTE(0x1fda0);
  __adddf3 = ABSOLUTE(0x1fda4);
  __fixdfsi = ABSOLUTE(0x1fda8);
  flush_dcache = ABSOLUTE(0x1fdac);
  flushinv_dcache = ABSOLUTE(0x1fdb0);
  IcuWaitIdle = ABSOLUTE(0x1fdb4);
  DcuWaitIdle = ABSOLUTE(0x1fdb8);
  IcuInitial = ABSOLUTE(0x1fdbc);
  DcuInitial = ABSOLUTE(0x1fdc0);


  sfc_drop_cache = ABSOLUTE(0x1fdcc);
  chip_crc16 = ABSOLUTE(0x1fdd0);
  CrcDecode = ABSOLUTE(0x1fdd4);
  get_flash_file_key = ABSOLUTE(0x1fdd8);
  dec_isd_cfg_ini = ABSOLUTE(0x1fddc);
  _jlfs_mount = ABSOLUTE(0x1fde0);
  sfc_boot = ABSOLUTE(0x1fde4);
  mask_init = ABSOLUTE(0x1fde8);

  nvram_set_boot_state = ABSOLUTE(0x1fdf0);
  chip_reset = ABSOLUTE(0x1fdf4);
  the_debug_isr = ABSOLUTE(0x1fdf8);
  print = ABSOLUTE(0x1fdfc);
  sprintf = ABSOLUTE(0x1fe00);
  printf = ABSOLUTE(0x1fe04);
  puts = ABSOLUTE(0x1fe08);
  doe = ABSOLUTE(0x1fe0c);
  spi_get_port = ABSOLUTE(0x1fe10);
  _pll_init = ABSOLUTE(0x1fe14);
  flash_poweron = ABSOLUTE(0x1fe18);
  spi_unmount = ABSOLUTE(0x1fe1c);
  spi_port_io_init = ABSOLUTE(0x1fe20);
  usb_slave_mode = ABSOLUTE(0x1fe24);
  LZ4_setStreamDecode = ABSOLUTE(0x1fe28);
  LZ4_decompress_safe_continue = ABSOLUTE(0x1fe2c);
  LZ4_decompress_generic = ABSOLUTE(0x1fe30);
  _sdk_mkey_lock = ABSOLUTE(0x1fe34);
  g_hash_function = ABSOLUTE(0x1fe38);
  f1_hash_function = ABSOLUTE(0x1fe3c);
  f2_hash_function = ABSOLUTE(0x1fe40);
  f3_hash_function = ABSOLUTE(0x1fe44);
  h2_hash_function = ABSOLUTE(0x1fe48);
  h3_hash_function = ABSOLUTE(0x1fe4c);
  h4_hash_function = ABSOLUTE(0x1fe50);
  h5_hash_function = ABSOLUTE(0x1fe54);
  g_function = ABSOLUTE(0x1fe58);
  f1_function = ABSOLUTE(0x1fe5c);
  f2_function = ABSOLUTE(0x1fe60);
  f3_function = ABSOLUTE(0x1fe64);
  sha256Compute = ABSOLUTE(0x1fe68);
  uECC_compute_public_key = ABSOLUTE(0x1fe6c);
  uECC_shared_secret = ABSOLUTE(0x1fe70);
  bi_initialize = ABSOLUTE(0x1fe74);
  bi_read_from_byte = ABSOLUTE(0x1fe78);
  bi_poly_mod2 = ABSOLUTE(0x1fe7c);
  bi_poly_mul = ABSOLUTE(0x1fe80);
  trim = ABSOLUTE(0x1fe84);
  bi_wirte_to_byte = ABSOLUTE(0x1fe88);
  bi_free = ABSOLUTE(0x1fe8c);
  bi_terminate = ABSOLUTE(0x1fe90);
  get_cvsd_codec_ops = ABSOLUTE(0x1fe94);
  wtgv2_silk_NLSF_CB_NB_MB = ABSOLUTE(0x18590);
  wtgv2_LSFCosTab_FIX_Q12 = ABSOLUTE(0x1848c);
  icdf_global_table = ABSOLUTE(0x18a88);
  get_wtgv2dec_rom_ops = ABSOLUTE(0x1fe98);
  wtgv2_silk_Decode = ABSOLUTE(0x1fe9c);
  wtgv2_silk_decode_frame = ABSOLUTE(0x1fea0);
  wtgv2_silk_decoder_set_fs = ABSOLUTE(0x1fea4);
  wtgv2_silk_decode_indices = ABSOLUTE(0x1fea8);
  wtgv2_silk_decode_pulses = ABSOLUTE(0x1feac);
  wtgv2_silk_decode_parameters = ABSOLUTE(0x1feb0);
  wtgv2_silk_decode_core = ABSOLUTE(0x1feb4);
  logAprx_CORDIC_float = ABSOLUTE(0x1feb8);
  expAprx_CORDIC_float = ABSOLUTE(0x1febc);
  magnAprx_float = ABSOLUTE(0x1fec0);
  MatrixCopy_float_c_c = ABSOLUTE(0x1fec4);
  MatrixCopy_float_f_f = ABSOLUTE(0x1fec8);
  MatrixEwMulAndSumOneDim_float_c_c_c_s0 = ABSOLUTE(0x1fecc);
  MatrixEwMulAndSumOneDim_float_c_c_c_s1 = ABSOLUTE(0x1fed0);
  MatrixEwMulAndSumOneDim_float_f_f_f_s0 = ABSOLUTE(0x1fed4);
  MatrixEwMulAndSumOneDim_float_f_f_f_s1 = ABSOLUTE(0x1fed8);
  MatrixEwConjMulAndSumOneDim_float_c_c_c_s0 = ABSOLUTE(0x1fedc);
  MatrixEwConjMulAndSumOneDim_float_c_c_c_s1 = ABSOLUTE(0x1fee0);
  VecMeanSq_s16_f32 = ABSOLUTE(0x1fee4);
  VecOverShift_s16_f32 = ABSOLUTE(0x1fee8);
  VecMinus_s16_f32_f32 = ABSOLUTE(0x1feec);
  VectorCopy_f32_s16 = ABSOLUTE(0x1fef0);
  VecEleMul_s16_s16_f32 = ABSOLUTE(0x1fef4);
  VecEleMul_f32_s16_f32 = ABSOLUTE(0x1fef8);
  VecMeanSq_float_f_f = ABSOLUTE(0x1fefc);
  VectorSet_float_f_f = ABSOLUTE(0x1ff00);
  VecMagRecAve_float_c_f_f = ABSOLUTE(0x1ff04);
  VecMulScalar_float_c_f_c = ABSOLUTE(0x1ff08);
  VecConjEleMul_float_c_c_c = ABSOLUTE(0x1ff0c);
  VecPlus_float_f_f_f = ABSOLUTE(0x1ff10);
  VecDivide_float_c_f_c_f = ABSOLUTE(0x1ff14);
  VecPlus_float_c_c_c = ABSOLUTE(0x1ff18);
  VecConjMulRecAve_float_c_c_c_f = ABSOLUTE(0x1ff1c);
  VecGetMag_float_c_f = ABSOLUTE(0x1ff20);
  VecMax_float_f_f = ABSOLUTE(0x1ff24);
  VecRecAve_float_f_f_f = ABSOLUTE(0x1ff28);
  VecEleMul_float_f_f_f = ABSOLUTE(0x1ff2c);
  VecMagAndDiv_float_c_f_f_f = ABSOLUTE(0x1ff30);
  VecMin_float_f_f = ABSOLUTE(0x1ff34);
  VecMinus_float_f_f_f = ABSOLUTE(0x1ff38);
  VecMean_float_f_f = ABSOLUTE(0x1ff3c);
  VecEleMul_float_c_f_c = ABSOLUTE(0x1ff40);
  VecMeanSqu_float_c_f = ABSOLUTE(0x1ff44);
  VecCopy_float_f_f = ABSOLUTE(0x1ff48);
  VecMin_float_f_f_f = ABSOLUTE(0x1ff4c);
  VecDivide_float_f_f_f_f = ABSOLUTE(0x1ff50);
  VecCompBT_float_f_f_f = ABSOLUTE(0x1ff54);
  VecCondCopy_float_f_f_f = ABSOLUTE(0x1ff58);
  VecMulScalar_float_f_f_f = ABSOLUTE(0x1ff5c);
  VecPlusScalar_float_f_f_f = ABSOLUTE(0x1ff60);
  VecMinScalar_float_f_f_f = ABSOLUTE(0x1ff64);
  VecOpposite_float_f_f = ABSOLUTE(0x1ff68);
  VecEleMla_float_f_f_f = ABSOLUTE(0x1ff6c);
  VecEleMla_float_c_c_c = ABSOLUTE(0x1ff70);
  VecMinus_float_c_c_c = ABSOLUTE(0x1ff74);
  VecDotProduct_float_f_f_f = ABSOLUTE(0x1ff78);
  VecDotProduct_float_c_c_c = ABSOLUTE(0x1ff7c);
  VecDotProduct_float_c_f_c = ABSOLUTE(0x1ff80);
  VecCopy_float_c_c = ABSOLUTE(0x1ff84);
  VecCopy_float_f_c = ABSOLUTE(0x1ff88);
  VecConjDotProduct_float_c_c_c = ABSOLUTE(0x1ff8c);
  VecNormalize_float_c_c_f = ABSOLUTE(0x1ff90);
  VecRecAve_float_c_c_f = ABSOLUTE(0x1ff94);
  VecOverlapShift_float_f_f_i = ABSOLUTE(0x1ff98);
  VecMax_float_f_f_f = ABSOLUTE(0x1ff9c);
  VecConjEwMulAndDiv_float_c_c_f_f_c = ABSOLUTE(0x1ffa0);
  VecPlusMulScalar_float_c_c_f_c = ABSOLUTE(0x1ffa4);
  VecCondCopy_float_c_i_c = ABSOLUTE(0x1ffa8);
  VecEleMulAndDiv_float_f_f_f_f_f = ABSOLUTE(0x1ffac);
  VectorSet_float_f_c = ABSOLUTE(0x1ffb0);
  VecDivScalar_float_f_f_f = ABSOLUTE(0x1ffb4);
  VecDivScalar_float_c_f_c = ABSOLUTE(0x1ffb8);
  VecPlusScalar_float_c_f_c = ABSOLUTE(0x1ffbc);
  VecMinScalar_float_c_f_c = ABSOLUTE(0x1ffc0);
  mdct_tab_256 = ABSOLUTE(0x19000);
  mdct_tab_2048 = ABSOLUTE(0x19200);
  sine_long_1024 = ABSOLUTE(0x1a200);
  sine_short_128 = ABSOLUTE(0x1b200);
  kbd_long_1024 = ABSOLUTE(0x1b400);
  kbd_short_128 = ABSOLUTE(0x1c400);
  Huffmancb_SCL = ABSOLUTE(0x1c634);
  num_swb_960_window = ABSOLUTE(0x1c844);
  num_swb_1024_window = ABSOLUTE(0x1c850);
  num_swb_128_window = ABSOLUTE(0x1c85c);
  swb_offset_1024_96 = ABSOLUTE(0x1c868);
  swb_offset_128_96 = ABSOLUTE(0x1c8bc);
  swb_offset_1024_64 = ABSOLUTE(0x1c8d6);
  swb_offset_128_64 = ABSOLUTE(0x1c936);
  swb_offset_1024_48 = ABSOLUTE(0x1c950);
  swb_offset_128_48 = ABSOLUTE(0x1c9b4);
  swb_offset_1024_32 = ABSOLUTE(0x1c9d2);
  swb_offset_1024_24 = ABSOLUTE(0x1ca3a);
  swb_offset_128_24 = ABSOLUTE(0x1ca9a);
  swb_offset_1024_16 = ABSOLUTE(0x1caba);
  swb_offset_128_16 = ABSOLUTE(0x1cb12);
  swb_offset_1024_8 = ABSOLUTE(0x1cb32);
  swb_offset_128_8 = ABSOLUTE(0x1cb84);
  swb_offset_1024_window = ABSOLUTE(0x1cba4);
  swb_offset_128_window = ABSOLUTE(0x1cbd4);
  iq_table = ABSOLUTE(0x1cc04);
  tns_coef_0_3 = ABSOLUTE(0x1d008);
  tns_coef_0_4 = ABSOLUTE(0x1d048);
  tns_coef_1_3 = ABSOLUTE(0x1d088);
  tns_coef_1_4 = ABSOLUTE(0x1d0c8);
  pow05_table = ABSOLUTE(0x1d108);
  sample_rates = ABSOLUTE(0x1d124);
  pred_sfb_max = ABSOLUTE(0x1d154);
  tns_sbf_max = ABSOLUTE(0x1d160);
  Huffmancb_1 = ABSOLUTE(0x1d1a0);
  Huffmancb_2 = ABSOLUTE(0x1d338);
  Huffmancb_3 = ABSOLUTE(0x1d470);
  Huffmancb_4 = ABSOLUTE(0x1d5a8);
  Huffmancb_5 = ABSOLUTE(0x1d6d8);
  Huffmancb_6 = ABSOLUTE(0x1d820);
  Huffmancb_7 = ABSOLUTE(0x1d960);
  Huffmancb_8 = ABSOLUTE(0x1da58);
  Huffmancb_9 = ABSOLUTE(0x1db50);
  Huffmancb_10 = ABSOLUTE(0x1ddf0);
  Huffmancb_11 = ABSOLUTE(0x1e080);
  AACcodeBookDTable = ABSOLUTE(0x1e540);
  AAC_SFHUFF_TAB = ABSOLUTE(0x1c83c);
  get_aacrom_ops = ABSOLUTE(0x1ffc4);
  silk_NLSF_CB_WB = ABSOLUTE(0x1e5b8);
  silk_LTP_vq_ptrs_Q7 = ABSOLUTE(0x1ea5c);
  get_opus_enc_ops = ABSOLUTE(0x1ffc8);
  opus_encode = ABSOLUTE(0x1ffcc);
  silk_Encode = ABSOLUTE(0x1ffd0);
  silk_encode_do_VAD_FIX = ABSOLUTE(0x1ffd4);
  silk_encode_frame_FIX = ABSOLUTE(0x1ffd8);
  silk_find_pitch_lags_FIX = ABSOLUTE(0x1ffdc);
  silk_noise_shape_analysis_FIX = ABSOLUTE(0x1ffe0);
  silk_find_pred_coefs_FIX = ABSOLUTE(0x1ffe4);
  silk_process_gains_FIX = ABSOLUTE(0x1ffe8);
  silk_prefilter_FIX = ABSOLUTE(0x1ffec);
  silk_NSQ_del_dec_c = ABSOLUTE(0x1fff0);
  silk_encode_indices = ABSOLUTE(0x1fff4);
  silk_encode_pulses = ABSOLUTE(0x1fff8);
  silk_ana_filt_bank_1 = ABSOLUTE(0x1fffc);
  STFT_Win_FixHalf_M512_D160 = ABSOLUTE(0x3804);
  STFT_Win_FixHalf_M256_D160 = ABSOLUTE(0x3c04);
  STFT_Win_FixHalf_M256_D80 = ABSOLUTE(0x3e04);
  STFT_Win_FixHalf_M128_D80 = ABSOLUTE(0x4004);
  WOLAAnalysisFilter_Float_M64_m4_r1 = ABSOLUTE(0x4);
  WOLASynthesisFilter_Float_M64_m4_r1 = ABSOLUTE(0x404);
  WOLAAnalysisFilter_Float_M128_m4_r1 = ABSOLUTE(0x804);
  WOLASynthesisFilter_Float_M128_m4_r1 = ABSOLUTE(0x1004);
  WOLAAnalysisFilter_Float_M256_m4_r1 = ABSOLUTE(0x1804);
  WOLASynthesisFilter_Float_M256_m4_r1 = ABSOLUTE(0x2804);
  quantize_intervals_LF = ABSOLUTE(0x9914);
  invert_quantize_dither_factors_LF = ABSOLUTE(0x9a18);
  quantize_dither_factors_LF = ABSOLUTE(0x9b1c);
  quantize_factor_select_offset_LF = ABSOLUTE(0x9c20);
  hd_quantize_intervals_LF = ABSOLUTE(0x9ca4);
  boot_arg_list = ABSOLUTE(0x19fc28);
  _IRQ_MEM_ADDR = ABSOLUTE(0x19fe00);
  _MASK_MEM_BEGIN = ABSOLUTE(0x19fc00);
  _MASK_MEM_SIZE = ABSOLUTE(0x1a4);

EXTERN(
_start




source_node_adapter


music_file_plug

clk_sync_node_adapter
decoder_node_adapter
resample_node_adapter
bt_audio_sync_node_adapter

tone_file_plug
ring_file_plug
key_tone_file_plug

sbc_hwaccel
sbc_decoder_plug
msbc_decoder_plug

sine_dec_plug
cvsd_decoder_plug
pcm_dec_plug
fat_vfs_ops
a2dp_1sbc_codec_private
);

UPDATA_SIZE = 0x80;
UPDATA_BEG = _MASK_MEM_BEGIN - UPDATA_SIZE;
UPDATA_BREDR_BASE_BEG = 0x1A0000;

RAM_LIMIT_L = 0x100000;
RAM_LIMIT_H = UPDATA_BEG;
PHY_RAM_SIZE = RAM_LIMIT_H - RAM_LIMIT_L;


ISR_BASE = _IRQ_MEM_ADDR;
ROM_RAM_SIZE = _MASK_MEM_SIZE;
ROM_RAM_BEG = _MASK_MEM_BEGIN;

RAM0_BEG = RAM_LIMIT_L + (128K * 0);
RAM0_END = RAM_LIMIT_H;
RAM0_SIZE = RAM0_END - RAM0_BEG;

RAM1_BEG = RAM_LIMIT_L;
RAM1_END = RAM0_BEG;
RAM1_SIZE = RAM1_END - RAM1_BEG;


CODE_BEG = 0x6000100;







MEMORY
{
 code0(rx) : ORIGIN = CODE_BEG, LENGTH = 1048576
 ram0(rwx) : ORIGIN = RAM0_BEG, LENGTH = RAM0_SIZE




 psr_ram(rwx) : ORIGIN = 0x2000000, LENGTH = 8*1024*1024
}


ENTRY(_start)

SECTIONS
{
    . = ORIGIN(ram0);

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


     *(.crypto_ecdh_data)
     *(.crypto_data)
     *(.uECC_data)
     *(.ECDH_sample_data)

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



        *(.uECC_bss)
        *(.ECDH_sample_bss)
        *(.crypto_bss)
 . = ALIGN(32);

    } > ram0

 .data_code ALIGN(32):SUBALIGN(4)
 {
  data_code_pc_limit_begin = .;




    . = ALIGN(4);
 cvp_data_code_begin = . ;
    . = ALIGN(4);
    cvp_data_code_end = . ;
    cvp_data_code_size = cvp_data_code_end - cvp_data_code_begin ;

    media_data_code_start = .;
    . = ALIGN(4);
    *(.kws_common_data_multi_26k)
    *(.jlsp_kws_music_code)
    *(.jlsp_kws_music_const)
    *(.jlsp_far_keyword_const)
    *(.jlsp_far_keyword_code)
    *(.jlsp_far_front_code)
    *(.jlsp_far_front_const)
    aac_dec_code_begin = .;
    *(.bt_aac_dec_code)
    *(.bt_aac_dec_sparse_code)
    *(.bt_aac_dec.text.cache.L2.run)
    aac_dec_code_end = .;
    aac_dec_code_size = aac_dec_code_end - aac_dec_code_begin ;

    . = ALIGN(4);
    bt_aac_dec_const_begin = .;
    *(.bt_aac_dec_const)
    *(.bt_aac_dec_sparse_const)
*(.bt_aac_dec.text.cache.L2.const)
    bt_aac_dec_const_end = .;
    bt_aac_dec_const_size = bt_aac_dec_const_end - bt_aac_dec_const_begin ;

    . = ALIGN(4);
    *(.aac_mem)
    *(.aac_ctrl_mem)



    . = ALIGN(4);
    *(.pSOUND360TD_cal_sparse_code)
    *(.pSOUND360TD_cal_code)
    *(.pSOUND360TD_cal_const)
    *(.spatial_mem)
    *(.TDSpatial.text)
    *(.TDSpatial.text.cache.L2)
    *(.ldac_dec.text)
    *(.ldac_dec.text.const)
    *(.ldac.text)
    *(.ldac.text.const)
    *(.ldac_dec.text.cache.L2.run)
    *(.ldac_dec.text.cache.L2.const)

    . = ALIGN(4);
    *(.jla_dec.text.const)
    *(.jla_dec.text)
    *(.jla_enc.text.const)
    *(.jla_enc.text)
    *(.jla_codec_c_const)
    *(.jla_codec_c_code)
    *(.jla_codec_c_sparse_code)
    *(.jla_codec_d_code)
    *(.jla_codec_e_code)
    . = ALIGN(4);



 *(.jl_codec_c_const)
 *(.jl_codec_c_code)
 *(.jl_codec_ari_c_const)
 *(.jl_codec_ari_c_code)
 *(.jl_codec_c_sparse_code)
 *(.jl_codec_d_code)
 *(.jl_codec_e_code)
 *(.jl_codec_t8k_code)
 *(.jl_codec_t16k_code)
 *(.jl_codec_t24k_code)
 *(.jl_codec_t32k_code)
 *(.jl_codec_t441k_code)
 *(.jl_codec_const)



    *(.vbss.text.cache.L2.code)



    *(.reverb_cal.text.cache.L2.run)
    *(.plateReverb_adv.text.cache.L2.run)
    *(.plate_reverb.text)



    *(.echo.text)
 *(.voiceChanger.text.cache.L2.run)
 *(.voiceChanger.text.const)
 *(.voiceChanger.text)



  *(.advaudio_plc.text.cache.L2.run)



 *(.wdrc.text)
 *(.drc.text.cache.L2.code)
 *(.crossover.text.cache.L2.code)
 *(.BandMerge.text.cache.L2.code)
 *(.multiband_drc.text)



 *(.HarmonicExciter.text.cache.L2.run)
 *(.HarmonicExciter.text.const)



 *(.dynamiceq.text.cache.L2.code)
 *(.notchhowling.text.cache.L2.code)
  *(.notchhowling.text.cache.L2.const)



 *(.FrequencyShift.text.cache.L2.run)
 *(.FrequencyShift.text.const)



 *(.noisegate.text.cache.L2.code)
 *(.mixer.text.cache.L2)
 *(.dac.text.cache.L2)
 *(.convert.text.cache.L2.code)
 *(.VocalRemove.text.cache.L2.code)

    . = ALIGN(4);
    media_data_code_end = .;
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
 } > ram0

 .movable_code ALIGN(32):SUBALIGN(4)
    {

    } > ram0

 __report_overlay_begin = .;





 data_code_pc_limit_end = .;
 __report_overlay_end = .;

 .debug_record_info ALIGN(32):
 {

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

  . = ALIGN(4);

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


  . = ALIGN(4);
        *(.gpio.text.cache.L2)
  *(.LED_code)
  *(.LED_const)

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


        *(.opcore_table_maskrom)
        *(.bfilt_table_maskroom)
        *(.bfilt_code)
        *(.bfilt_const)


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


  *(.crypto_ecdh_code)
     *(.crypto_ecdh_const)

     *(.crypto_bigint_code)
     *(.crypto_bigint_const)

     *(.crypto_code)
     *(.crypto_const)

     *(.ECDH_sample_code)
     *(.ECDH_sample_const)

     *(.uECC_code)
     *(.uECC_const)

     *(.hmac_code)
     *(.hmac_const)

     *(.hash_sample_code)
     *(.hash_sample_const)

     *(.aes_cmac_sample_code)
  *(.aes_cmac_sample_const)

  . = ALIGN(4);
        tws_sync_call_begin = .;
        KEEP(*(.tws_sync_call))
        tws_sync_call_end = .;

  . = ALIGN(4);
        tws_func_stub_begin = .;
        KEEP(*(.tws_func_stub))
        tws_func_stub_end = .;

        . = ALIGN(4);
  _record_handle_begin = .;
  PROVIDE(record_handle_begin = .);
  KEEP(*(.debug_record_handle_ops))
  _record_handle_end = .;
  PROVIDE(record_handle_end = .);

  . = ALIGN(32);
   } > code0
}




SECTIONS
{
    .text : ALIGN(4)
    {
        gsensor_dev_begin = .;
        KEEP(*(.gsensor_dev))
        gsensor_dev_end = .;

        imusensor_dev_begin = .;
        KEEP(*(.imusensor_dev))
        imusensor_dev_end = .;

        fm_dev_begin = .;
        KEEP(*(.fm_dev))
        fm_dev_end = .;

        fm_emitter_dev_begin = .;
        KEEP(*(.fm_emitter_dev))
        fm_emitter_dev_end = .;

        storage_device_begin = .;
        KEEP(*(.storage_device))
        storage_device_end = .;

  . = ALIGN(4);
     key_ops_begin = .;
        KEEP(*(.key_operation))
     key_ops_end = .;

        . = ALIGN(4);
        key_callback_begin = .;
  KEEP(*(.key_cb))
        key_callback_end = .;

  . = ALIGN(32);
    }

 .data ALIGN(32):
 {
 } > ram0

 .bss ALIGN(32):
    {
    } > ram0

 .data_code ALIGN(32):
 {
 } > ram0

}


SECTIONS
{
    .data : ALIGN(4)
    {
        update_data_start = .;

        update_data_end = .;
    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        update_bss_start = .;

        *(.update_bss)
        update_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
  update_code_start = .;

  *(.bt_updata_ram_code)
  *(.update_const)
  *(.update_code)

  update_code_end = .;

  . = ALIGN(4);
    } > code0


    UPDATE_CODE_TOTAL_SIZE = update_code_end - update_code_start;
}
SECTIONS
{
    .data : ALIGN(4)
    {
        btstack_data_start = .;
        *(.bt_stack_data)
        *(.ble_db_data)
        *(.ble_sm_data)
        *(.ble_att_data)
        *(.ble_gatt_data)
        *(.fmna_stack_data)
        *(.fmna_plaform_data)
        *(.fmna_crypto_data)


        BTSTACK_LE_HOST_MESH_DATA_START = .;
  . = (. +3) & ~ 3;
        _net_buf_pool_list = .;
        *(._net_buf_pool.static.*)

        *(.ble_mesh_data)
        *(.ble_mesh_tinycrypt_data)
        BTSTACK_LE_HOST_MESH_DATA_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_DATA_START);

        btstack_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        btstack_bss_start = .;
        *(.bt_stack_bss)
        *(.ble_db_bss)
        *(.ble_sm_bss)
        *(.ble_att_bss)
        *(.ble_gatt_bss)
        *(.btstack_pool)
        *(.fmna_stack_bss)
        *(.fmna_plaform_bss)
        *(.fmna_crypto_bss)


        BTSTACK_LE_HOST_MESH_BSS_START = .;
        *(.ble_mesh_bss)
        *(.ble_mesh_tinycrypt_bss)
        BTSTACK_LE_HOST_MESH_BSS_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_BSS_START);

        btstack_bss_end = .;
    } > ram0

    .text : ALIGN(4)
    {
        btstack_code_start = .;

        . = ALIGN(4);

        a2dp_source_media_codec_begin = .;
        KEEP(*(.a2dp_source_media_codec))
            a2dp_source_media_codec_end = .;
        a2dp_sink_media_probe_begin = .;
        KEEP(*(.a2dp_sink_media_probe))
            a2dp_sink_media_probe_end = .;

        a2dp_sink_media_codec_begin = .;
        KEEP(*(.a2dp_sink_media_codec))
            a2dp_sink_media_codec_end = .;

        a2dp_event_handler_begin = .;
        KEEP(*(.a2dp_event_handler))
            a2dp_event_handler_end = .;

        sdp_record_item_begin = .;
        KEEP(*(.sdp_record_item))
            sdp_record_item_end = .;

        bt_sleep_begin = .;
        KEEP(*(.bt_sleep))
            bt_sleep_end = .;

  *(.bt_stack_const)
  *(.bt_stack_code)
        *(.ble_db_const)
        *(.ble_db_code)
        *(.ble_sm_const)
        *(.ble_sm_code)
        *(.ble_att_const)
        *(.ble_att_code)
        *(.ble_gatt_const)
        *(.ble_gatt_code)
        *(.fmna_stack_const)
        *(.fmna_stack_code)
        *(.fmna_plaform_const)
        *(.fmna_plaform_code)
        *(.fmna_crypto_const)
        *(.fmna_crypto_code)


        BTSTACK_LE_HOST_MESH_CODE_START = .;
        *(.ble_mesh_code)
        *(.ble_mesh_tinycrypt_code)

        *(.ble_mesh_const)
        *(.ble_mesh_tinycrypt_const)
        BTSTACK_LE_HOST_MESH_CODE_SIZE = ABSOLUTE(. - BTSTACK_LE_HOST_MESH_CODE_START);

        btstack_code_end = .;
  . = ALIGN(4);
    } > code0
}

BTSTACK_LE_HOST_MESH_RAM_TOTAL = BTSTACK_LE_HOST_MESH_DATA_SIZE + BTSTACK_LE_HOST_MESH_BSS_SIZE;
BTSTACK_LE_HOST_MESH_FLASH_TOTAL = BTSTACK_LE_HOST_MESH_CODE_SIZE;

BTSTACK_CODE_SIZE = (btstack_code_end - btstack_code_start) + (btstack_data_end - btstack_data_start);

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


  *(.power_driver.data)

        driver_data_end = .;
        . = ALIGN(32);
    } > ram0

 .data_code ALIGN(4):SUBALIGN(4)
 {


     *(.power_driver.text.cache.L1)

 } > ram0

    .bss ALIGN(4):SUBALIGN(4)
    {
        driver_bss_start = .;

        CLOCK_BSS_START = .;
        *(.clock_bss)
        CLOCK_BSS_SIZE = ABSOLUTE(. - CLOCK_BSS_START);

        *(.debug_bss)


  *(.power_driver.data.bss)

        *(.uart_bss)

        *(.sd_var)

 . = ALIGN(4);
  driver_bss_end = .;
    } > ram0

    .text ALIGN(4):SUBALIGN(4)
    {
        driver_code_start = .;

        *(.fm_code)

        CLOCK_CODE_START = .;
        *(.clock_code)
        *(.clock_const)
        CLOCK_CODE_SIZE = ABSOLUTE(. - CLOCK_CODE_START);


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


    DRIVER_RAM_TOTAL = (driver_data_end - driver_data_start) + (driver_bss_end - driver_bss_start);
    DRIVER_CODE_TOTAL = (driver_code_end - driver_code_start);
}
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
SECTIONS
{
    .data ALIGN(32):SUBALIGN(4)
    {
        audio_cvp_data_begin = .;
     . = ALIGN(4);
        *(.jlsp_aec_data)
        *(.jlsp_nlp_data)
        *(.jlsp_dns_data)
        *(.jlsp_enc_data)
        *(.jlsp_prep_data)
        *(.jlsp_wn_data)
        *(.jlsp_tri_data)
        *(.jlsp_agc_data)
        *(.jlsp_dms_hybrid_data)
        *(.jlsp_dms_awn_data)
        *(.sms_data)
        *(.sms_stereo_data)
        *(.dms_data)
        *(.data._*)
        *(.aec_data)
        *(.res_data)
        *(.ns_data)
        *(.jlsp_data)
        *(.nlp_data)
        *(.der_data)
        *(.qmf_data)
        *(.fft_data)
        audio_cvp_data_end = .;
        audio_cvp_data_size = audio_cvp_data_end - audio_cvp_data_begin ;
     . = ALIGN(4);
    } > ram0

    .bss (NOLOAD):SUBALIGN(4)
    {
     audio_cvp_bss_begin = .;
        . = ALIGN(4);
        *(.sms_bss)
        *(.sms_stereo_bss)
        *(.aec_bss)
        *(.nlp_bss)
        *(.res_bss)
        *(.ns_bss)
        *(.der_bss)
        . = ALIGN(4);
        *(.jlsp_bss)
        *(.jlsp_aec_bss)
        *(.jlsp_nlp_bss)
        *(.jlsp_dns_bss)
        *(.jlsp_enc_bss)
        *(.jlsp_prep_bss)
        *(.jlsp_wn_bss)
        *(.jlsp_tri_bss)
        *(.jlsp_agc_bss)
        *(.jlsp_dms_hybrid_bss)
        *(.jlsp_dms_awn_bss)
     audio_cvp_bss_end = .;
     audio_cvp_bss_size = audio_cvp_bss_end - audio_cvp_bss_begin ;
    } > ram0

    .text ALIGN(32):SUBALIGN(4)
    {
        . = ALIGN(4);
        cvp_code_begin = . ;
        *(.dns_16k_data)
        *(.dns_8k_data)
        . = ALIGN(4);
  *(.jlsp_const)
  *(.jlsp_code)
        *(.jlsp_dns_code)
        *(.jlsp_dns_const)
        *(.dns_param_data_wind)
        *(.dns_common_data_wind)

  *(.ns_sparse_code )
  *(.aec_sparse_code)
  *(.nlp_sparse_code)
  *(.der_sparse_code)
  *(.qmf_sparse_code)
        *(.sms_sparse_code)
        *(.sms_stereo_sparse_code)
        *(.dms_sparse_code)
        *(.opcore_maskrom)

        . = ALIGN(4);
        *(.text._*)


        *(.jlsp_aec_code)
  *(.jlsp_aec_const)


        *(.nlp_code)
        *(.nlp_const)
  *(.qmf_code)
  *(.qmf_const)
        *(.jlsp_nlp_code)
  *(.jlsp_nlp_const)


  *(.ns_code)
  *(.ns_const)


  . = ALIGN(4);
  *(.aec_mem)
        *(.aec_mux)
  *(.aec_code)
  *(.aec_const)
  *(.res_code)
  *(.res_const)
        *(.bark_const)
  *(.fft_code)
  *(.fft_const)
        *(.der_code)
        *(.der_const)


     . = ALIGN(4);
        *(.dns_common_data)
        *(.dns_param_data_single)
        *(.dns_param_data_dual)
     . = ALIGN(4);


        *(.agc_code)
        *(.agc_const)
        *(.jlsp_agc_code)
        *(.jlsp_agc_const)


        *(.dms_code)
        *(.dms_const)
        *(.jlsp_enc_code)
        *(.jlsp_enc_const)
        *(.jlsp_dms_hybrid_code)
        *(.jlsp_dms_hybrid_const)
  *(.jlsp_dms_awn_code)
        *(.jlsp_dms_awn_const)


        *(.sms_const)
        *(.sms_code)
        *(.sms_stereo_const)
        *(.sms_stereo_code)



        *(.jlsp_prep_code)
        *(.jlsp_prep_const)


        *(.jlsp_wn_code)
        *(.jlsp_wn_const)


        *(.jlsp_tri_code)
        *(.jlsp_tri_const)

  . = ALIGN(4);
  cvp_code_end = . ;
  cvp_code_size = cvp_code_end - cvp_code_begin ;

    } > code0

}
SECTIONS {
    .data ALIGN(32): SUBALIGN(4)
    {
        media_lib_data_begin = .;
        . = ALIGN(4);


 . = ALIGN(4);
 audio_sync_data_begin = .;
        *(.audio_sync_data)
 audio_sync_data_end = .;
 . = ALIGN(4);
    *(.pSOUND360TD_cal_data)

    *(.jlsp_kws_yesno_data)
    *(.jlsp_kws_common_data)
    *(.jlsp_kws_music_bss)
    *(.jlsp_kws_phone_call_data)
    *(.jlsp_kws_india_english_music_data)
    *(.jlsp_kws_india_english_phone_call_data)
    *(.jlsp_far_keyword_data)
    *(.jlsp_far_front_data)

 *(.lhdc.data)
 *(.lhdcv4decode.data)
 *(.lhdc_v5.data)
 *(.lhdcv5decode.data)
    *(.ldac.data)
    *(.ldac_enc.data)
    *(.ldac_dec.data)

    *(.msbc_data)
    *(.cvsd_data)
    *(.sbc_data)
    *(.bt_aac_dec.data)
 *(.bt_aac_dec_data)
    *(.btaac_rom_dec.data)
    *(.aac.data)
    *(.aac_data)
 *(.sbc_eng_data)
 *(.bt_audioplc_data)
 *(.lc3_dec.data)
 *(.lc3_enc.data)



 *(.jla_codec_c_data)
 *(.jl_codec_c_data)
 *(.jla_lw_codec_data)
 *(.jla_lw.data)
 *(.jla_ll_codec.data)
 *(.jla_ll.data)

 *(.jla_v2_dec_data)
 *(.jla_v2_enc_data)
 *(.jla_v2_codec_c_data)
 *(.jla_v2_codec_ari_c_data)


 *(.mp3_dec.data)
 *(.mp3.data)
    *(.mp3tsy_dec.data)
 *(.mp3_data)
 *(.mp3_dec_data)
    *(.mp3tsy_dec_data)
    *(.mp3tsy_data)
 *(.wma_dec.data)
 *(.wma.data)
 *(.wma_data)
 *(.wma_dec_data)
    *(.wav_dec.data)
    *(.wav.data)
    *(.wav_dec_data)
 *(.wav_data)
 *(.flac_dec.data)
 *(.flac.data)
 *(.flac_data)
 *(.flac_dec_data)
    *(.mty.data)
 *(.f2a_dec.data)
 *(.f2a.data)
 *(.opus_enc.data)
 *(.opus.data)
 *(.ogg_opus_dec.data)
 *(.ogg.data)
 *(.ogg_vorbis_dec.data)
    *(.aptx_dec.data)
    *(.wtg_dec.data)
    *(.g729.data)
    *(.wtgv2_dec.data)
    *(.wtgv2_data)
    *(.wtgv2dec_data)
    *(.wts.data)
    *(.dts_dec.data)
    *(.dts.data)
 *(.f2a_dec.data)
 *(.f2a.data)
 *(.m4a_dec.data)
 *(.m4a.data)
 *(.m4a_dec_data)
 *(.m4a_data)
 *(.alac.data)
 *(.amr_dec.data)
 *(.alac_data)
 *(.amr_dec_data)
 *(.amr.data)
 *(.alac_data)
 *(.ape_dec.data)
 *(.ape.data)
 *(.ape_dec_data)
 *(.ape_data)

 *(.audio_track_data)
 *(.audio_eq_data)
    *(.audio_drc_data)
    *(.audio_vbass_data)
    *(.audio_surround_data)
 *(.audio_pitch_data)
 *(.audio_pitchspeed_data)
    *(.pitchshifter_data)
 *(.audio_noisegate_data)
 *(.howlings_phf_data)
    *(.audio_reverb_data)
    *(.audio_echo_data)

 *(.anc_data)
 *(.anc_core_data)
 *(.anc_user_data)
 *(.anc_box_data)
 *(.anc_btspp_data)
 *(.anc_debug_data)
 media_lib_data_end = .;
        media_lib_data_size = media_lib_data_end - media_lib_data_begin ;
        . = ALIGN(4);
    } > ram0

    .bss(NOLOAD): SUBALIGN(4)
    {
        media_lib_bss_begin = .;
        . = ALIGN(4);


 . = ALIGN(4);
 audio_sync_bss_begin = .;
    *(.audio_sync_bss)
 audio_sync_bss_end = .;

 . = ALIGN(4);
    *(.jlsp_kws_yesno_bss)
    *(.jlsp_kws_common_bss)
    *(.jlsp_kws_music_bss)
    *(.jlsp_kws_phone_call_bss)
    *(.jlsp_kws_india_english_music_bss)
    *(.jlsp_kws_india_english_phone_call_bss)
    *(.jlsp_far_keyword_bss)
    *(.jlsp_far_front_bss)

 *(.lhdc.data.bss)
 *(.lhdcv4decode.bss)
 *(.lhdc_v5.data.bss)
 *(.lhdcv5decode.bss)
    *(.ldac_dec.data.bss)
    *(.ldac.data.bss)
    *(.ldac_enc.data.bss)

    *(.cvsd_bss)
 *(.bt_aac_dec_bss)
    *(.bt_aac_dec.data.bss)
    *(.btaac_ron_dec.data.bss)
    *(.aac.data.bss)
 *(.sbc_eng_bss)
 *(.bt_audioplc_bss)
 *(.lc3_dec.data.bss)
 *(.lc3_enc.data.bss)



 *(.jla_codec_c_bss)
 *(.jl_codec_c_bss)
 *(.jla_lw_codec_bss)
 *(.jla_lw.bss)
 *(.jla_ll.bss)

 *(.jla_v2_dec_bss)
 *(.jla_v2_enc_bss)
 *(.jla_v2_codec_c_bss)
 *(.jla_v2_codec_ari_c_bss)


 *(.mp3_dec.data.bss)
 *(.mp3.data.bss)
 *(.mp3tsy.data.bss)
 *(.mty.data.bss)
 *(.flac_dec.data.bss)
 *(.flac.data.bss)
 *(.wma_dec.data.bss)
 *(.wma.data.bss)
 *(.wav.data.bss)
 *(.wav_dec.data.bss)
  *(.f2a_dec.data.bss)
  *(.f2a.data.bss)
  *(.opus_enc.data.bss)
  *(.opus.data.bss)
 *(.ogg_opus_dec.bss)
  *(.ogg.data.bss)
 *(.ogg_vorbis_dec.bss)
    *(.aptx_dec.data.bss)
    *(.wtg_dec.data.bss)
    *(.g729.data.bss)
    *(.wtgv2_dec.data.bss)
    *(.wts.data.bss)
    *(.dts_dec.data.bss)
    *(.dts.data.bss)
 *(.ape_dec.data.bss)
 *(.ape.data.bss)
 *(.amr_dec.data.bss)
 *(.amr.data.bss)
 *(.m4a_dec.data.bss)
 *(.m4a.data.bss)
 *(.alac.data.bss)

 *(.audio_track_bss)

 *(.anc_bss)
 *(.anc_core_bss)
 *(.anc_user_bss)
 *(.anc_box_bss)
 *(.anc_btspp_bss)
 *(.anc_debug_bss)
 media_lib_bss_end = .;
        media_lib_bss_size = media_lib_bss_end - media_lib_bss_begin ;
        . = ALIGN(4);
    } > ram0

    .text ALIGN(32): SUBALIGN(4)
    {
        media_lib_bss_begin = .;
        . = ALIGN(4);


    . = ALIGN(4);
    media_text_start = .;
    audio_sync_code_begin = .;
    *(.audio_sync_code)
    audio_sync_code_end = .;



    *(.jlsp_kws_common_code)
    *(.jlsp_kws_common_const)



    *(.jlsp_kws_yesno_code)
    *(.jlsp_kws_yesno_const)
    *(.kws_common_data_multi_57k)
    *(.jlsp_kws_phone_call_code)
    *(.jlsp_kws_phone_call_const)



    *(.jlsp_kws_india_english_music_code)
    *(.jlsp_kws_india_english_music_const)
    *(.jlsp_kws_india_english_phone_call_code)
    *(.jlsp_kws_india_english_phone_call_const)
    *(.msbc_enc)
    *(.msbc_code)
    *(.msbc_const)
    *(.msbc_codec_code)
    *(.msbc_codec_const)


    *(.cvsd_codec)
    *(.cvsd_code)
    *(.cvsd_const)
    . = ALIGN(4);
    *(.bt_audioplc_code)
    *(.bt_audioplc_sparse_code)
    *(.bt_audioplc_const)
    *(.lf_audioplc_code)
    *(.lf_audioplc_sparse_code)
    *(.lf_audioplc_const)
    *(.lfaudio_plc.text.L2.run)

    . = ALIGN(4);
    *(.pcm_code)
    *(.pcm_const)

    *(.lhdc.text.const)
    *(.lhdc.text)
    *(.lhdcv4decode.text)
    *(.lhdcv4decode.const)

    *(.lhdc_v5.text.const)
    *(.lhdc_v5.text)
    *(.lhdcv5decode.text)
    *(.lhdcv5decode.const)







    *(.ldac_enc.text)
    *(.ldac_enc.text.const)

    *(.sbc_code)
    *(.sbc_const)
    *(.sbc_eng_code)
    *(.sbc_eng_const)
    *(.bt_aac_dec.text)
    *(.bt_aac_dec.text.const)
    *(.btaac_rom_dec.text)
    *(.aac.text.const)
    *(.aac.text)
    *(.aac_code)
    *(.aac_const)
    *(.bt_aac_dec_core_sparse_code)
    *(.bt_aac_dec_eng_code)
    *(.bt_aac_dec_core_code)
    *(.bt_aac_dec_eng_const)
    *(.plc_code)
    *(.plc_const)

    *(.wtg_dec.text)
    *(.g729.text)
    *(.g729.text.const)
    *(.g729_code)
    *(.g729_const)
    *(.wtgv2_code)
    *(.wtgv2_const)
    *(.wtgv2dec_code)
    *(.wtgv2dec_const)
    *(.wtgv2dec_str)
    *(.wtg_decv2_sparse_code)
    *(.wtgv2_dec.text)
    *(.wts.text)
    *(.wts.text.const)
    *(.f2a_dec.text)
    *(.f2a.text)
    *(.f2a.text.const)

    *(.mp3_dec.text)
    *(.mp3.text)
    *(.mp3.text.const)
    *(.mp3tsy_dec.text)
    *(.mp3_code)
    *(.mp3_const)
    *(.mp3_decstream_const)
    *(.mp3_decstream_code)
    *(.mp3_decstream_sparse_code)
    *(.mp3_decstream_sparse_const)
    *(.mp3_dec_sparse_code)
    *(.mp3_dec_sparse_const)
    *(.mp3_dec_code)
    *(.mp3_dec_const)
    *(.mp3tsy_dec_code)
    *(.mty_code)
    *(.mp3tsy_dec_sparse_code)
    *(.mp3tsy_dec_const)
    *(.mty.text)
    *(.mty.text.const)
    *(.f2a_dec.text)
    *(.f2a.text)
    *(.f2a.text.const)

    *(.opus_enc.text)
    *(.opus_enc.text.const)
    *(.opus.text)
    *(.opus.text.const)
    *(.ogg_opus_dec.text)
    *(.ogg.text)
    *(.ogg.text.const)
    *(.ogg_vorbis_dec.text)

    *(.ape_dec.text)
    *(.ape_dec.text.const)
    *(.ape.text)
    *(.ape.text.const)
    *(.ape_dec_sparse_code)
    *(.ape_dec_sparse_const)
    *(.ape_dec_code)
    *(.ape_dec_const)
    *(.ape_const)
    *(.ape_code)

    *(.alac_const)
    *(.alac_code)
    *(.alac_dec_code)
    *(.wma_dec.text)
    *(.wma.text)
    *(.wma.text.const)
    *(.wma_code)
    *(.wma_const)
    *(.wma_dec_code)
    *(.wma_dec_const)
    *(.wma_dec_sparse_code)
    *(.wma_dec_sparse_const)
    *(.wma_decstream_const)
    *(.wma_decstream_code)
    *(.wma_decstream_sparse_code)
    *(.wma_decstream_sparse_const)

    *(.wav_dec.text)
    *(.wav.text)
    *(.wav.text.const)
    *(.wav_dec_sparse_code)
    *(.wav_dec_sparse_const)

    *(.wav_dec_code)
    *(.wav_dec_const)
    *(.wav_const)
    *(.wav_code)

    *(.wtg_dec_code )
    *(.wtg_dec_const)
    *(.wtg_dec_sparse_code)

    *(.sine_code)
    *(.sine_const)


    *(.aptx_dec.text)
    *(.aptx_dec.text.const)
    *(.amr_dec.text)
    *(.amr_dec.text.const)
    *(.amr.text)
    *(.amr.text.const)
    *(.amr_code)
    *(.amr_const)
    *(.amr_dec_sparse_code)
    *(.amr_dec_sparse_const)
    *(.amr_dec_const)
    *(.amr_dec_code)
    *(.audio_decoder_code)
    *(.audio_decoder_const)
    *(.audio_track_const)
    *(.audio_track_code)

    *(.dts_dec.text)
    *(.dts_dec.text.const)
    *(.dts.text)
    *(.dts.text.const)
    *(.dts_dec_ff_const)
    *(.dts_dec_sparse_code)
    *(.dts_dec_sparse_const)
    *(.dts_dec_const)
    *(.dts_code)
    *(.dts_dec_code)
    *(.flac_dec.text)
    *(.flac_dec.text.const)
    *(.flac.text)
    *(.flac.text.const)
    *(.flac_code)
    *(.flac_dec_code)
    *(.flac_dec_const)
    *(.flac_dec_sparse_code)
    *(.m4a_dec.text)
    *(.m4a_dec.text.const)
    *(.m4a_dec.bt_aac.text.const)
    *(.m4a.text)
    *(.m4a.text.const)
    *(.m4a_dec_code)
    *(.m4a_dec_ff_const)
    *(.m4a_dec_const)
    *(.m4a_const)
    *(.m4a_code)
    *(.m4a_dec_sparse_code)
    *(.m4a_dec_sparse_const)
    *(.alac.text)
    *(.alac.text.const)

    *(.mp3_encode_code)
    *(.mp3_enc_code)
    *(.mp3_enc_const)
    *(.mp3_enc_sparse_code)
    *(.mp3_enc_sparse_const)
    . = ALIGN(4);
    *(.mp2_encode_code)
    *(.mp2_encode_const)
    *(.mp2_encode_sparse_code)
    *(.mp2_encode_sparse_const)
    . = ALIGN(4);
    *(.adpcm_encode_code)
    *(.adpcm_encode_const)
    *(.adpcm_encode_sparse_code)
    *(.adpcm_encode_sparse_const)
    . = ALIGN(4);
    . = ALIGN(4);

    *(.lc3_dec.text.const)
    *(.lc3_dec.text)
    *(.lc3_enc.text.const)
    *(.lc3_enc.text)
    *(.lc3_codec_c_const)
    *(.lc3_codec_c_code)
    *(.lc3_codec_c_sparse_code)
    *(.lc3_codec_d_code)
    *(.lc3_codec_e_code)
    . = ALIGN(4);
 *(.jla_lw_codec_const)
 *(.jla_lw_codec_code)
 *(.jla_lw.const)
 *(.jla_lw.code)

 *(.jla_ll.const)
 *(.jla_ll.code)
 *(.jla_ll_codec.text)
 *(.jla_ll_codec.text.const)
 *(.jla_ll_codec.text.cache.L2.decode)
 *(.jla_ll_codec.text.cache.L2.const)
 *(.jla_ll_codec.text.cache.L2.run)

 *(.jla_v2_enc_const)
 *(.jla_v2_enc_code)
 *(.jla_v2_dec_const)
 *(.jla_v2_dec_code)
 *(.jla_v2_codec_c_const)
 *(.jla_v2_codec_c_code)
 *(.jla_v2_codec_ari_c_const)
 *(.jla_v2_codec_ari_c_code)
 *(.jla_v2_codec_c_sparse_code)
 *(.jla_v2_codec_e_code)
 *(.jla_v2_decoder_const)
 *(.jla_v2_decoder_code)
 *(.jla_v2_codec_d_code)

    *(.speex_code)
    *(.speex_encoder_code)
    *(.opus_code)
    *(.opus_encoder_code)

    *(.audio_codec_code)



    *(.media_device_code)
    *(.media_device_const)
    *(.audio_encoder_code)
    *(.audio_encoder_const)
    *(.mixer_code)
    *(.mixer_const)
    *(.dec_server_code)
    *(.dec_server_const)
    *(.rec_server_code)
    *(.rec_server_const)
    *(.auto_mute_code)
    *(.auto_mute_const)
    *(.wireless_sync_code)
    *(.wireless_sync_const)

    anc_code_begin = .;
    *(.anc_user_const)
    *(.anc_user_code)
    *(.anc_const)
    *(.anc_code)
    *(.anc_core_const)
    *(.anc_core_code)
    *(.anc_box_const)
    *(.anc_box_code)
    *(.anc_btspp_const)
    *(.anc_btspp_code)
    *(.anc_debug_const)
    *(.anc_debug_code)
    anc_code_end = .;
    anc_code_size = anc_code_end - anc_code_begin;


    *(.bt_compressor_sparse_const)
    *(.bt_compressor_sparse_code)
    *(.compressor_sparse_code)
    *(.compressor_sparse_const)
    *(.drc_sparse_code)
    *(.drc_sparse_const)

    *(.bt_limiter_sparse_const)
    *(.bt_limiter_sparse_code)
    *(.limiter_sparse_code)
    *(.limiter_sparse_const)

    *(.bt_crossOver_sparse_const)
    *(.bt_crossOver_sparse_code)
    *(.crossOver_sparse_code)
    *(.crossOver_sparse_const)
    *(.reverb_cal_sparse_code)


    *(.reverb_cal_code)
    *(.reverb_cal_const)
    *(.platereverb_code)
    *(.platereverb_const)
    *(.audio_reverb_code)
    *(.audio_reverb_const)
    *(.reverb0_code)
    *(.reverb0_const)



    *(.audio_echo_code)
    *(.audio_echo_const)
    *(.echo_cal_code)
    *(.echo_cal_const)



    *(.audio_eq_code)
    *(.audio_eq_const)


    *(.audio_drc_code)
    *(.audio_drc_const)
    *(.drc_const)
    *(.drc_code)
    *(.sw_drc_code)
    *(.sw_drc_const)



    *(.howlings_phf_code)
    *(.howlings_phf_const)

    *(.notchhowling_code)
    *(.notchhowling_const)
    *(.notchhowling_sparse_code)
    *(.notchhowling_sparse_const)

    *(.howlings_phf_sparse_code)


    *(.audio_noisegate_code)
    *(.audio_noisegate_const)

    *(.noisegate_code)
    *(.noisegate_const)
    *(.noisegate_sparse_code)
    *(.noisegate_sparse_const)



    *(.audio_gain_code)
    *(.audio_gain_const)



    *(.dynamic_eq_code)
    *(.dynamic_eq_const)
    *(.dynamic_eq_detect_code)
    *(.dynamic_eq_detect_const)
    *(.dynamic_eq_sparse_code)
    *(.dynamic_eq_sparse_const)
    *(.dynamic_eq_detect_sparse_code)
    *(.dynamic_eq_detect_sparse_const)




    *(.audio_mic_effect_const)
    *(.audio_mic_effect_code)


    *(.audio_mic_stream_const)
    *(.audio_mic_stream_code)


    *(.audio_effect_adj_const)
    *(.audio_effect_adj_code)



    *(.audio_vbass_code)
    *(.audio_vbass_const)
    *(.vbss_code)
    *(.vbss_const)
    *(.vbss_sparse_code)
    *(.vbss_sparse_const)


    *(.pitchshifter_code)
    *(.pitchshifter_const)
    *(.audio_pitch_code)
    *(.audio_pitch_const)
    *(.audio_pitchspeed_code)
    *(.audio_pitchspeed_const)
    *(.pitchshifter_sparse_code)


    *(.audio_surround_code)
    *(.audio_surround_const)
    *(.sur_cal_common_code)
    *(.sur_TYPE0_cal_code)
    *(.sur_TYPE1_cal_code)
    *(.sur_TYPE0_cal_const)
    *(.sur_TYPE1_cal_const)

    *(.sur_LRDRIFT2_cal_code)
    *(.sur_LRDRIFT2_cal_const)
    *(.sur_ROTATE_cal_code)
    *(.sur_ROTATE_cal_const)
    *(.sur_LRDRIFT_cal_code)
    *(.sur_LRDRIFT_cal_const)
    *(.sur_cal_sparse_code)

 *(.limiter.text)
    *(.limiter.text.const)
 *(.limiter.text.cache.L2.code)
 *(.limiter.text.cache.L2.const)


 *(.multiband_limiter.text)
 *(.multiband_limiter.text.const)


    *(.multiband_crossover.text)
 *(.multiband_crossover.text.const)


    *(.multiband_drc.text)
 *(.multiband_drc.text.const)


 *(.virtual_surround_pro.text)
 *(.virtual_surround_pro.text.const)
 *(.sas.text.cache.L2.code)
 *(.sas.text.cache.L2.const)
    *(.sas.text)
    *(.sas.text.code)


    *(.iir_filter.text.cache.L2.code)
    *(.iir_filter.text.cache.L2.const)
    *(.iir_filter.text)
    *(.iir_filter.text.const)
 *(.sof_eq.text)
 *(.sof_eq.text.const)



 *(.Phaser.text.cache.L2.code)






 *(.flanger.text.cache.L2.code)





 *(.chorus_adv.text.cache.L2.code)





 *(.pingpong_echo.text.cache.L2.code)





 *(.distortion.text.cache.L2.code)
 *(.distortion.text.cache.L2.const)




    . = ALIGN(4);
    media_code_begin = .;
    *(.media.*.text)
    . = ALIGN(4);

    *(.app_audio_code)
    *(.app_audio_const)

    *(.compressor_code)
    *(.compressor_const)
    *(.bt_compressor_const)
    *(.bt_compressor_code)

    *(.crossOver_code)
    *(.crossOver_const)
    *(.bt_crossOver_const)
    *(.bt_crossOver_code)

    *(.limiter_code)
    *(.limiter_const)
    *(.bt_limiter_const)
    *(.bt_limiter_code)
    *(.drc_const)
    *(.drc_code)

    *(.audio_gain_code)
    *(.audio_gain_const)

    *(.vbss_code)
    *(.vbss_const)
    *(.vbss_sparse_code)
    *(.vbss_sparse_const)

    *(.specshow_code)
    *(.specshow_const)
    *(.specshow_sparse_code)
    *(.specshow_sparse_const)
    *(.spectrum_fft_code)
    *(.spectrum_fft_const)
    *(.spectrum_eq_code)
    *(.spectrum_eq_const)
    *(.audio_vocal_remove_code)


    . = ALIGN(4);
    *(.sur_cal_const)
    *(.sur_cal_code)
    . = ALIGN(4);
    *(.lib_pitchshift_code)
    *(.lib_pitchshift_const)
    *(.resample_fastcal_sparse_code)
    *(.resample_fastcal_code)
    *(.resample_fastcal_const)


    . = ALIGN(4);
    _audio_decoder_begin = .;
    PROVIDE(audio_decoder_begin = .);
KEEP(*(.audio_decoder))
    _audio_decoder_end = .;
    PROVIDE(audio_decoder_end = .);

    _audio_encoder_begin = .;
    PROVIDE(audio_encoder_begin = .);
KEEP(*(.audio_encoder))
    _audio_encoder_end = .;
    PROVIDE(audio_encoder_end = .);

    _audio_package_begin = .;
    PROVIDE(audio_package_begin = .);
KEEP(*(.audio_package))
    _audio_package_end = .;
    PROVIDE(audio_package_end = .);

    _audio_dev_begin = .;
    PROVIDE(audio_dev_begin = .);
KEEP(*(.audio_device))
    _audio_dev_end = .;
    PROVIDE(audio_dev_end = .);

    _audio_hwaccel_begin = .;
    PROVIDE(audio_hwaccel_begin = .);
KEEP(*(.audio_hwaccel))
    _audio_hwaccel_end = .;
    PROVIDE(audio_hwaccel_end = .);

    media_code_end = .;
    . = ALIGN(4);
    media_code_size = media_code_end - media_code_begin;

    . = ALIGN(4);
    media_text_end = .;
 media_lib_bss_end = .;
        media_lib_bss_size = media_lib_bss_end - media_lib_bss_begin ;
        . = ALIGN(4);
    } > code0
}
SECTIONS
{
    .data : ALIGN(4)
    {
  . = ALIGN(4);
        system_data_start = .;

  _video_subdev_begin = .;
  PROVIDE(video_subdev_begin = .);
  KEEP(*(.video_subdev.0))
  KEEP(*(.video_subdev.1))
  KEEP(*(.video_subdev.2))
  KEEP(*(.video_subdev.3))
  KEEP(*(.video_subdev.4))
  KEEP(*(.video_subdev.5))
  _video_subdev_end = .;
  PROVIDE(video_subdev_end = .);

  _audio_subdev_begin = .;
  PROVIDE(audio_subdev_begin = .);
  KEEP(*(.audio_subdev.0))
  KEEP(*(.audio_subdev.1))
  KEEP(*(.audio_subdev.2))
  KEEP(*(.audio_subdev.3))
  _audio_subdev_end = .;
  PROVIDE(audio_subdev_end = .);

  _iic_device_begin = .;
  PROVIDE(iic_device_begin = .);
  KEEP(*(.iic))
  _iic_device_end = .;
  PROVIDE(iic_device_end = .);

        _avin_spi_device_begin = .;
  PROVIDE(avin_spi_device_begin = .);
  KEEP(*(.sw_spi))
  _avin_spi_device_end = .;
  PROVIDE(avin_spi_device_end = .);

  _video_dev_begin = .;
  PROVIDE(video_dev_begin = .);
  KEEP(*(.video_device))
  _video_dev_end = .;
  PROVIDE(video_dev_end = .);

  _key_driver_ops_begin = .;
  PROVIDE(key_driver_ops_begin = .);
  KEEP(*(.key_driver_ops))
  _key_driver_ops_end = .;
  PROVIDE(key_driver_ops_end = .);

  _touch_driver_begin = .;
  PROVIDE(touch_driver_begin = .);
  KEEP(*(.touch_driver))
  _touch_driver_end = .;
  PROVIDE(touch_driver_end = .);

  _static_hi_timer_begin = .;
  PROVIDE(static_hi_timer_begin = .);
  KEEP(*(.hi_timer))
  _static_hi_timer_end = .;
  PROVIDE(static_hi_timer_end = .);

  _sys_cpu_timer_begin = .;
  PROVIDE(sys_cpu_timer_begin = .);
  KEEP(*(.sys_cpu_timer))
  _sys_cpu_timer_end = .;
  PROVIDE(sys_cpu_timer_end = .);

     _sys_config_begin = .;
     PROVIDE(sys_config_begin = .);
     KEEP(*(.sys_cfg))
     _sys_config_end = .;
     PROVIDE(sys_config_end = .);

     _sys_fat_begin = .;
     PROVIDE(sys_fat_begin = .);
     KEEP(*(.fs_fat))
     _sys_fat_end = .;
     PROVIDE(sys_fat_end = .);

  _app_begin = .;
  PROVIDE(app_begin = .);
  KEEP(*(.app))
  _app_end = .;
  PROVIDE(app_end = .);

     *(.crypto_ecdh_data)
     *(.crypto_data)

     *(.mem_data)
        *(.os_port_data)

  *(.os_str)
        *(.os_data)

     *(.uECC_data)
     *(.ECDH_sample_data)

 . = ALIGN(4);
         __movable_slot_start = .;
         *(movable.slot.*);
         __movable_slot_end = .;

 . = ALIGN(4);
        system_data_end = .;

    } > ram0

    .bss (NOLOAD) :ALIGN(4)
    {
        system_bss_start = .;
        . = ALIGN(4);
        *(.os_bss)
        *(.mem_heap)
  *(.memp_memory_x)
        *(.mem_bss)
        *(.os_port_bss)

        *(.uECC_bss)
        *(.ECDH_sample_bss)
 . = ALIGN(4);

        system_bss_end = .;

    } > ram0

    .text : ALIGN(4)
    {
  . = ALIGN(4);
        system_text_start = .;

   _device_node_begin = .;
     PROVIDE(device_node_begin = .);
     KEEP(*(.device))
     _device_node_end = .;
     PROVIDE(device_node_end = .);

  config_target_begin = .;
     PROVIDE(config_target_begin = .);
     KEEP(*(.config_target))
  config_target_end = .;
     PROVIDE(config_target_end = .);

     system_code_begin = .;
     KEEP(*(.system.*.text))
     system_code_end = .;
  . = ALIGN(4);
  system_code_size = system_code_end - system_code_begin;

     _lib_version_begin = .;
     PROVIDE(lib_version_begin = .);
     KEEP(*(.lib_version))
     _lib_version_end = .;
     PROVIDE(lib_version_end = .);

  _initcall_begin = .;
  PROVIDE(initcall_begin = .);
     KEEP(*(.initcall))
  _initcall_end = .;
  PROVIDE(initcall_end = .);

  _early_initcall_begin = .;
  PROVIDE(early_initcall_begin = .);
  KEEP(*(.early.initcall))
  _early_initcall_end = .;
  PROVIDE(early_initcall_end = .);

  _late_initcall_begin = .;
  PROVIDE(late_initcall_begin = .);
  KEEP(*(.late.initcall))
  _late_initcall_end = .;
  PROVIDE(late_initcall_end = .);

  _platform_initcall_begin = .;
  PROVIDE(platform_initcall_begin = .);
  KEEP(*(.platform.initcall))
  _platform_initcall_end = .;
  PROVIDE(platform_initcall_end = .);

  _module_initcall_begin = .;
  PROVIDE(module_initcall_begin = .);
  KEEP(*(.module.initcall))
  _module_initcall_end = .;
  PROVIDE(module_initcall_end = .);

  platform_uninitcall_begin = .;
  KEEP(*(.platform.uninitcall))
  platform_uninitcall_end = .;

  _sys_event_handler_begin = .;
  PROVIDE(sys_event_handler_begin = .);
  KEEP(*(.sys_event.4.handler))
  KEEP(*(.sys_event.3.handler))
  KEEP(*(.sys_event.2.handler))
  KEEP(*(.sys_event.1.handler))
  KEEP(*(.sys_event.0.handler))
  _sys_event_handler_end = .;
  PROVIDE(sys_event_handler_end = .);

  _syscfg_arg_begin = .;
  PROVIDE(syscfg_arg_begin = .);
  KEEP(*(.syscfg.arg))
  _syscfg_arg_end = .;
  PROVIDE(syscfg_arg_end = .);

  _syscfg_handler_begin = .;
  PROVIDE(syscfg_handler_begin = .);
  KEEP(*(.syscfg.handler))
  _syscfg_handler_end = .;
  PROVIDE(syscfg_handler_end = .);

  _syscfg_ops_begin = .;
  PROVIDE(syscfg_ops_begin = .);
  KEEP(*(.syscfg.2.ops))
  KEEP(*(.syscfg.1.ops))
  KEEP(*(.syscfg.0.ops))
  _syscfg_ops_end = .;
  PROVIDE(syscfg_ops_end = .);

  _server_info_begin = .;
  PROVIDE(server_info_begin = .);
  KEEP(*(.server_info))
  _server_info_end = .;
  PROVIDE(server_info_end = .);

  _bus_device_begin = .;
  PROVIDE(bus_device_begin = .);
  KEEP(*(.bus_device))
  _bus_device_end = .;
  PROVIDE(bus_device_end = .);

     _sys_power_hal_ops_begin = .;
     PROVIDE(sys_power_hal_ops_begin = .);
     KEEP(*(.sys_power_hal_ops))
     _sys_power_hal_ops_end = .;
     PROVIDE(sys_power_hal_ops_end = .);

     crypto_begin = .;
     *(.crypto_ecdh_code)
     *(.crypto_ecdh_const)

     *(.crypto_bigint_code)
     *(.crypto_bigint_const)

     *(.crypto_code)
     *(.crypto_const)

     *(.ECDH_sample_code)
     *(.ECDH_sample_const)

     *(.uECC_code)
     *(.uECC_const)

     *(.hmac_code)
     *(.hmac_const)

     *(.hash_sample_code)
     *(.hash_sample_const)

     *(.aes_cmac_sample_code)
     *(.aes_cmac_sample_const)
     crypto_end = .;
     crypto_size = . - crypto_begin;

        *(.mem_code)
        *(.mem_const)

        *(.os_port_code)
        *(.os_port_const)

        *(.os_const)

        *(.math_fast_funtion_code)

        __movable_function_start = .;
        *(movable.text.*);
        *(movable.stub.*);
        *(movable.region.*);

 . = ALIGN(4);
        __movable_function_end = .;
        __movable_function_size = __movable_function_end - __movable_function_start;

        system_text_end = .;


        system_code_total_size = system_text_end - system_text_start;
    } > code0

 .data_code ALIGN(32):
 {
  . = ALIGN(4);
  *(.os_code)
  . = ALIGN(4);
    } > ram0
}


text_begin = ADDR(.text);
text_size = SIZEOF(.text);
text_end = text_begin + text_size;
ASSERT((text_size % 4) == 0,"!!! text_size Not Align 4 Bytes !!!");

bss_begin = ADDR(.bss);
bss_size = SIZEOF(.bss);
bss_end = bss_begin + bss_size;
ASSERT((bss_size % 4) == 0,"!!! bss_size Not Align 4 Bytes !!!");

data_addr = ADDR(.data);
data_begin = text_begin + text_size;
data_size = SIZEOF(.data);
ASSERT((data_size % 4) == 0,"!!! data_size Not Align 4 Bytes !!!");





data_code_addr = ADDR(.data_code);
data_code_begin = data_begin + data_size;
data_code_size = SIZEOF(.data_code);
ASSERT((data_code_size % 4) == 0,"!!! data_code_size Not Align 4 Bytes !!!");
psr_data_code_begin = data_code_begin + data_code_size;

psr_data_code_addr = ADDR(.psr_data_code);
psr_data_code_size = SIZEOF(.psr_data_code);
ASSERT(1048576 > text_size,"check sdk_config.h CONFIG_FLASH_SIZE < text_size");
ASSERT(_HEAP_BEGIN > bss_begin,"_HEAP_BEGIN < bss_begin");
ASSERT(_HEAP_BEGIN > data_addr,"_HEAP_BEGIN < data_addr");
ASSERT(_HEAP_BEGIN > data_code_addr,"_HEAP_BEGIN < data_code_addr");



PROVIDE(HEAP_BEGIN = _HEAP_BEGIN);
PROVIDE(HEAP_END = _HEAP_END);
_MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN;
PROVIDE(MALLOC_SIZE = _HEAP_END - _HEAP_BEGIN);

ASSERT(MALLOC_SIZE >= 0x8000, "heap space too small !")




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
BR28_SDK_RAM_SIZE = report_mmu_tlb_size + report_boot_info_size + report_irq_stack_size + report_data_size + report_bss_size + report_overlay_size + report_data_code_size + report_heap_size;
