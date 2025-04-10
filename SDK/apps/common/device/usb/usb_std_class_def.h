#ifndef __USB_STD_CLASS_DEF_H__
#define __USB_STD_CLASS_DEF_H__

/**************************************************************************/
/*
               CLASS  BITMAP
    7   |   6   |   5   |   4   |   3   |   2   |   1   |   0
                                   HID    AUDIO  SPEAKER   Mass Storage
*/
/**************************************************************************/
#define     MASSSTORAGE_CLASS   0x00000001
#define     SPEAKER_CLASS       0x00000002
#define     MIC_CLASS           0x00000004
#define     AUDIO_CLASS         (SPEAKER_CLASS|MIC_CLASS)
#define     HID_CLASS           0x00000008
#define     CDC_CLASS           0x00000010
#define     CUSTOM_HID_CLASS    0x00000020
#define     MIDI_CLASS          0x00000040
#define     PRINTER_CLASS       0x00000080


#ifndef FUSB_MODE
#define FUSB_MODE  1
#endif

///////////MassStorage Class
#ifndef MSD_BULK_EP_OUT
#define MSD_BULK_EP_OUT             1
#endif
#ifndef MSD_BULK_EP_IN
#define MSD_BULK_EP_IN              1
#endif
#ifndef MAXP_SIZE_BULKOUT_FS
#define MAXP_SIZE_BULKOUT_FS        64
#endif
#ifndef MAXP_SIZE_BULKOUT_HS
#define MAXP_SIZE_BULKOUT_HS        512
#endif
#ifndef MAXP_SIZE_BULKOUT
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKOUT           MAXP_SIZE_BULKOUT_HS
#endif
#endif
#ifndef MAXP_SIZE_BULKIN_FS
#define MAXP_SIZE_BULKIN_FS        64
#endif
#ifndef MAXP_SIZE_BULKIN_HS
#define MAXP_SIZE_BULKIN_HS        512
#endif
#ifndef MAXP_SIZE_BULKIN
#if defined(FUSB_MODE) && FUSB_MODE
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define MAXP_SIZE_BULKIN            MAXP_SIZE_BULKIN_HS
#endif
#endif
#ifndef MSD_STR_INDEX
#define MSD_STR_INDEX               7
#endif

///////////HID class
#ifndef HID_EP_IN
#define HID_EP_IN                   2
#endif
#ifndef HID_EP_OUT
#define HID_EP_OUT                  2
#endif
#ifndef HID_EP_IN_2
#define HID_EP_IN_2                 3
#endif
#ifndef HID_EP_OUT_2
#define HID_EP_OUT_2                3
#endif
#ifndef MAXP_SIZE_HIDOUT
#define MAXP_SIZE_HIDOUT            16
#endif
#ifndef MAXP_SIZE_HIDIN
#define MAXP_SIZE_HIDIN             16
#endif
#ifndef HID_INTR_INTERVAL_FS
#define HID_INTR_INTERVAL_FS        1
#endif
#ifndef HID_INTR_INTERVAL_HS
#define HID_INTR_INTERVAL_HS        4
#endif
#ifndef HID_INTR_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define HID_INTR_INTERVAL           HID_INTR_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define HID_INTR_INTERVAL           HID_INTR_INTERVAL_HS
#endif
#endif

/////////////Audio Class
/**
 * 注意事项：
 * 1. uac 2.0需要使能spk的多采样率以及mic的多采样率才能被正确枚举
 * 2. Microsoft从Windodws 10 1703开始才支持uac 2.0，Linux则较早就支持，基于性能
 *    考虑，还是基于兼容性考虑，请谨慎选用
 * 3. 切换版本后，为避免出现未预料的问题，请先在设备管理器卸载一次
 * 4. 当mic声道>2时，windows会将采样率锁定在16kHz；spk声道>2时，则仍然可以设48kHz
 */
#define USB_AUDIO_VERSION_1_0       0x10
#define USB_AUDIO_VERSION_2_0       0x20
#ifndef USB_AUDIO_VERSION
#define USB_AUDIO_VERSION           USB_AUDIO_VERSION_1_0
#endif
#ifndef UAC_ISO_INTERVAL_FS
#define UAC_ISO_INTERVAL_FS         1
#endif
#ifndef UAC_ISO_INTERVAL_HS
#define UAC_ISO_INTERVAL_HS         4
#endif
#ifndef UAC_ISO_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define UAC_ISO_INTERVAL            UAC_ISO_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define UAC_ISO_INTERVAL            UAC_ISO_INTERVAL_HS
#endif
#endif
#ifndef UAC_24BIT_IN_4BYTE
//0: 24bit in 3byte, 1: 24 bit in 4byte
//Windows不支持这种数据格式，安卓支持这种数据格式
#define UAC_24BIT_IN_4BYTE          0
#endif
//speaker class
#ifndef SPK_AUDIO_RATE_NUM
#define SPK_AUDIO_RATE_NUM          1
#endif
#if SPK_AUDIO_RATE_NUM  == 1
#define SPK_AUDIO_RATE              48000
#else
#define SPK_AUDIO_RATE              96000
#define SPK_AUDIO_RATE_1            44100
#define SPK_AUDIO_RATE_2            48000
#endif
#ifndef SPK_AUDIO_RES
#define SPK_AUDIO_RES               16
#endif
#ifndef SPK_AUDIO_RES_2
#define SPK_AUDIO_RES_2             0//24
#endif
#ifndef SPK_CHANNEL
#define SPK_CHANNEL                 2
#endif
#ifndef SPK_FRAME_LEN
#define SPK_FRAME_LEN               (((SPK_AUDIO_RATE) * SPK_AUDIO_RES / 8 * SPK_CHANNEL)/1000+4)
#endif
#ifndef SPK_PCM_Type
#define SPK_PCM_Type                (SPK_AUDIO_RES >> 4)                // 0=8 ,1=16
#endif
#ifndef SPK_AUDIO_TYPE
#define SPK_AUDIO_TYPE              (0x02 - SPK_PCM_Type)           // TYPE1_PCM16
#endif
#ifndef SPK_ISO_EP_OUT
#ifdef CONFIG_CPU_BR18
#define SPK_ISO_EP_OUT              2
#else
#define SPK_ISO_EP_OUT              3
#endif
#endif
#ifndef SPEAKER_STR_INDEX
#define SPEAKER_STR_INDEX           5
#endif
#ifndef SPK_INPUT_TERMINAL_ID
#define SPK_INPUT_TERMINAL_ID       1
#endif
#ifndef SPK_FEATURE_UNIT_ID
#define SPK_FEATURE_UNIT_ID         2
#endif
#ifndef SPK_OUTPUT_TERMINAL_ID
#define SPK_OUTPUT_TERMINAL_ID      3
#endif
#ifndef SPK_SELECTOR_UNIT_ID
#define SPK_SELECTOR_UNIT_ID        8
#endif

/////////////Microphone Class
#ifndef MIC_AUDIO_RATE_NUM
#define MIC_AUDIO_RATE_NUM          1
#endif

#if MIC_AUDIO_RATE_NUM   == 1
#define MIC_AUDIO_RATE              48000
#else
#define MIC_AUDIO_RATE              48000
#define MIC_AUDIO_RATE_1            16000
#define MIC_AUDIO_RATE_2            44100
#endif

#ifndef MIC_AUDIO_RES
#define MIC_AUDIO_RES               16
#endif
#ifndef MIC_AUDIO_RES_2
#define MIC_AUDIO_RES_2             0//24
#endif
#ifndef MIC_CHANNEL
#define MIC_CHANNEL                 1
#endif
#ifndef MIC_FRAME_LEN
#define MIC_FRAME_LEN               ((MIC_AUDIO_RATE * MIC_AUDIO_RES / 8 * MIC_CHANNEL)/1000)
#endif
#ifndef MIC_PCM_TYPE
#define MIC_PCM_TYPE                (MIC_AUDIO_RES >> 4)                // 0=8 ,1=16
#endif
#ifndef MIC_AUDIO_TYPE
#define MIC_AUDIO_TYPE              (0x02 - MIC_PCM_TYPE)
#endif
#ifndef MIC_ISO_EP_IN
#define MIC_ISO_EP_IN               3
#endif
#ifndef MIC_STR_INDEX
#define MIC_STR_INDEX               6
#endif
#ifndef MIC_INPUT_TERMINAL_ID
#define MIC_INPUT_TERMINAL_ID       4
#endif
#ifndef MIC_FEATURE_UNIT_ID
#define MIC_FEATURE_UNIT_ID         5
#endif
#ifndef MIC_OUTPUT_TERMINAL_ID
#define MIC_OUTPUT_TERMINAL_ID      6
#endif
#ifndef MIC_SELECTOR_UNIT_ID
#define MIC_SELECTOR_UNIT_ID        7
#endif

////////////CDC Class
#ifndef CDC_DATA_EP_IN
#define CDC_DATA_EP_IN              4
#endif
#ifndef CDC_DATA_EP_OUT
#define CDC_DATA_EP_OUT             4
#endif
#ifndef CDC_INTR_EP_IN
#define CDC_INTR_EP_IN              5
#endif
#ifndef MAXP_SIZE_CDC_BULKIN
#define MAXP_SIZE_CDC_BULKIN        64
#endif
#ifndef MAXP_SIZE_CDC_BULKOUT
#define MAXP_SIZE_CDC_BULKOUT       64
#endif
#ifndef MAXP_SIZE_CDC_INTRIN
#define MAXP_SIZE_CDC_INTRIN        8
#endif
#ifndef CDC_INTR_INTERVAL_FS
#define CDC_INTR_INTERVAL_FS        1
#endif
#ifndef CDC_INTR_INTERVAL_HS
#define CDC_INTR_INTERVAL_HS        4
#endif
#ifndef CDC_INTR_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define CDC_INTR_INTERVAL           CDC_INTR_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define CDC_INTR_INTERVAL           CDC_INTR_INTERVAL_HS
#endif
#endif
#ifndef CDC_INTR_EP_ENABLE
#define CDC_INTR_EP_ENABLE          0
#endif

///////////CUSTOM_HID class
#ifndef CUSTOM_HID_EP_IN
#define CUSTOM_HID_EP_IN            4
#endif
#ifndef CUSTOM_HID_EP_OUT
#define CUSTOM_HID_EP_OUT           4
#endif
#ifndef MAXP_SIZE_CUSTOM_HIDIN
#define MAXP_SIZE_CUSTOM_HIDIN      64
#endif
#ifndef MAXP_SIZE_CUSTOM_HIDOUT
#define MAXP_SIZE_CUSTOM_HIDOUT     64
#endif
#ifndef CUSTOM_HID_INTERVAL_FS
#define CUSTOM_HID_INTERVAL_FS      1
#endif
#ifndef CUSTOM_HID_INTERVAL_HS
#define CUSTOM_HID_INTERVAL_HS      4
#endif
#ifndef CUSTOM_HID_INTERVAL
#if defined(FUSB_MODE) && FUSB_MODE
#define CUSTOM_HID_INTERVAL         CUSTOM_HID_INTERVAL_FS
#elif defined(FUSB_MODE) && FUSB_MODE == 0
#define CUSTOM_HID_INTERVAL         CUSTOM_HID_INTERVAL_HS
#endif
#endif

////////////MIDI Class
#ifndef MIDI_EP_IN
#define MIDI_EP_IN                  3
#endif
#ifndef MIDI_EP_OUT
#define MIDI_EP_OUT                 3
#endif
#ifndef MAXP_SIZE_MIDI_EPIN
#define MAXP_SIZE_MIDI_EPIN         64
#endif
#ifndef MAXP_SIZE_MIDI_EPOUT
#define MAXP_SIZE_MIDI_EPOUT        64
#endif

///////////PRINTER Class
#ifndef PTR_BULK_EP_OUT
#define PTR_BULK_EP_OUT             2
#endif
#ifndef PTR_BULK_EP_IN
#define PTR_BULK_EP_IN              1
#endif
#ifndef MAXP_SIZE_PTR_EPIN
#define MAXP_SIZE_PTR_EPIN          64
#endif
#ifndef MAXP_SIZE_PTR_EPOUT
#define MAXP_SIZE_PTR_EPOUT         64
#endif

#endif
