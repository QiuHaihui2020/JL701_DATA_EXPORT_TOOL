#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".section.data.bss")
#pragma data_seg(".section.data")
#pragma const_seg(".section.text.const")
#pragma code_seg(".section.text")
#endif
#include "app_config.h"
#include "audio_config_def.h"

1:
#if TCFG_CODE_RUN_RAM_FM_CODE
.fm.text.cache.L2
/* .stream.text.cache.L2 */
.eq.text
.wdrc.text
.hw_eq.text
.surcal.text
.gain.text
.bass_treble.text
.crossover.text
.band_merge.text
.vocal_track_synthesis.text
.vocal_track_separation.text
.mixer.text
#endif


2:
#if TCFG_CODE_RUN_RAM_BT_CODE
.classic.text.cache.L2.irq
#endif

// *INDENT-OFF*
3:
#if TCFG_CODE_RUN_RAM_AAC_CODE
.bt_aac_dec_code
.bt_aac_dec_sparse_code
. = ALIGN(4)
.bt_aac_dec_data
#endif

4:
#if TCFG_CODE_RUN_RAM_AEC_CODE
.cvp.text.cache.L2.cpp
.aec_code
.res_code
.ns_code
.fft_code
.nlp_code
.der_code
.qmf_code
.dms_code
.agc_code
.opcore_maskrom
#endif


5:
#if TCFG_PLATE_REVERB_ADVANCE_NODE_ENABLE
.plateReverb_adv.text.cache.L2.run            //0x114c
#endif
#if TCFG_PLATE_REVERB_NODE_ENABLE
.reverb_cal.text.cache.L2.run                 //0x70c
#endif
#if TCFG_NOTCH_HOWLING_NODE_ENABLE
.notchhowling.text.cache.L2.code             //0x428
#endif
#if TCFG_FREQUENCY_SHIFT_HOWLING_NODE_ENABLE
.FrequencyShift.text.cache.L2.run           //13e
#endif

#if TCFG_JLSTREAM_TURBO_ENABLE
#define INCLUDE_FROM_SECTION_C
#include "media/framework/movable_node_section.c"
#endif



 // *INDENT-ON*
