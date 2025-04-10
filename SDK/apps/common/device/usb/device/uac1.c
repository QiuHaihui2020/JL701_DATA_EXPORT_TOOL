/* Copyright(C) JieLi Technology
 * usb audio 1.0
 * All right reserved
 *
 */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "usb_config.h"
#include "usb/device/usb_stack.h"
#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "app_config.h"


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (USB_AUDIO_VERSION == USB_AUDIO_VERSION_1_0)

//bmChannelConfig
#define UAC_V1_CHANNEL_FRONT_LEFT               BIT(0)
#define UAC_V1_CHANNEL_FRONT_RIGHT              BIT(1)
#define UAC_V1_CHANNEL_FRONT_CENTER             BIT(2)
#define UAC_V1_CHANNEL_LOW_FREQ_EFFECT          BIT(3)
#define UAC_V1_CHANNEL_SURROUND_LEFT            BIT(4)
#define UAC_V1_CHANNEL_SURROUND_RIGHT           BIT(5)
#define UAC_V1_CHANNEL_LEFT_OF_CENTER           BIT(6)
#define UAC_V1_CHANNEL_RIGHT_OF_CENTER          BIT(7)
#define UAC_V1_CHANNEL_SURROUND                 BIT(8)
#define UAC_V1_CHANNEL_SIDE_LEFT                BIT(9)
#define UAC_V1_CHANNEL_SIDE_RIGHT               BIT(10)
#define UAC_V1_CHANNEL_TOP                      BIT(11)

//feature unit bmControls
#define UAC_V1_FU_CONTROL_UNDEF                 (0x00)
#define UAC_V1_FU_CONTROL_MUTE                  (0x01 << 0)
#define UAC_V1_FU_CONTROL_VOLUME                (0x01 << 1)
#define UAC_V1_FU_CONTROL_BASS                  (0x01 << 2)
#define UAC_V1_FU_CONTROL_MID                   (0x01 << 3)
#define UAC_V1_FU_CONTROL_TREBLE                (0x01 << 4)
#define UAC_V1_FU_CONTROL_EQUALIZER             (0x01 << 5)
#define UAC_V1_FU_CONTROL_AGC                   (0x01 << 6)
#define UAC_V1_FU_CONTROL_DELAY                 (0x01 << 7)
#define UAC_V1_FU_CONTROL_BASS_BOOST            (0x01 << 8)
#define UAC_V1_FU_CONTROL_LOUDNESS              (0x01 << 9)

#if SPK_CHANNEL == 1

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_CENTER \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 2

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 3  //2.1

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 4

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 5

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 6  //5.1

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 7

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT | \
        UAC_V1_CHANNEL_SIDE_LEFT | \
        UAC_V1_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#elif SPK_CHANNEL == 8  //7.1

#define SPK_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT | \
        UAC_V1_CHANNEL_SIDE_LEFT | \
        UAC_V1_CHANNEL_SIDE_RIGHT \
        )

#define SPK_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(UAC_V1_FU_CONTROL_VOLUME)

#else
#error "unsupport usb speaker channel number"
#endif


#if MIC_CHANNEL == 1

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        0 \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 2

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 3  //2.1

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 4

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 5

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 6  //5.1

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 7

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT | \
        UAC_V1_CHANNEL_SIDE_LEFT | \
        UAC_V1_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#elif MIC_CHANNEL == 8  //7.1

#define MIC_CHANNEL_CONFIG \
        UAC_WORD( \
        UAC_V1_CHANNEL_FRONT_LEFT | \
        UAC_V1_CHANNEL_FRONT_RIGHT | \
        UAC_V1_CHANNEL_FRONT_CENTER | \
        UAC_V1_CHANNEL_LOW_FREQ_EFFECT | \
        UAC_V1_CHANNEL_SURROUND_LEFT | \
        UAC_V1_CHANNEL_SURROUND_RIGHT | \
        UAC_V1_CHANNEL_SIDE_LEFT | \
        UAC_V1_CHANNEL_SIDE_RIGHT \
        )

#define MIC_FEATURE_CONTROLS \
        UAC_BYTE(UAC_V1_FU_CONTROL_MUTE | UAC_V1_FU_CONTROL_VOLUME), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0), \
        UAC_BYTE(0)

#else
#error "unsupport usb mic channel number"
#endif


/*********************************************************/
/*
			   Audio Class
*/
/*********************************************************/
///Standard Interface Association Descriptor
static const u8 uac_audio_interface_association[] = {
    USB_DT_INTERFACE_ASSOCIATION_SIZE, //Size of this descriptor in bytes: 8
    USB_DT_INTERFACE_ASSOCIATION,//INTERFACE ASSOCIATION Descriptor
    0,//Interface number of the first interface that is associated with this function
    0,//Number of contiguous interfaces that are associated with this function.
    USB_CLASS_AUDIO,//AUDIO_FUNCTION Function Class code
    USB_SUBCLASS_AUDIOSTREAMING,//FUNCTION_SUBCLASS_UNDEFINED
    PC_PROTOCOL_UNDEFINED,//AF_VERSION_02_00 Function Protocol code.
    SPEAKER_STR_INDEX,//Index of a string descriptor that describes this interface.
};
static const u8 uac_ac_standard_interface_desc[] = {
///standard interface AC descriptor(Interface 0, Alternate Setting 0)
    USB_DT_INTERFACE_SIZE,    //Length
    USB_DT_INTERFACE,     //DescriptorType:Inerface
    0x00,     //InterfaceNum
    0x00,       //AlternateSetting
    0x00,       //NumEndpoint
    USB_CLASS_AUDIO,       //InterfaceClass:audio
    USB_SUBCLASS_AUDIOCONTROL,       //InterfaceSubClass:audio ctl
    PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    SPEAKER_STR_INDEX,      //Interface String
};

static const u8 uac_spk_ac_interface[] = {
///class-specific AC interface descriptor
    0x09,    //Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_HEADER,     //DescriptorSubType:audio control header
    UAC_WORD(0x0100), //bcdADC:audio Device Class v1.00
    UAC_WORD(0x28), //TotalLength of class-specific AC interface descriptors
    0x01,      //InCollection:1 AudioStreaming interface
    0x02,      //InterfaceNr(1) - AS #2 id AudioStreaming interface 1 belongs to this AudioControl interface
    /* 0x03,      //InterfaceNr(1) - AS #2 id AudioStreaming interface 1 belongs to this AudioControl interface */
};
static const u8 uac_audio_ac_interface[] = {
///class-specific AC interface descriptor
    0x0a,    //Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_HEADER,     //DescriptorSubType:audio control header
    UAC_WORD(0x0100), //bcdADC:audio Device Class v1.00
    UAC_WORD(0x47), //TotalLength of class-specific AC interface descriptors
    0x02,      //InCollection:1 AudioStreaming interface
    0x01,      //InterfaceNr(1) - AS #2 id AudioStreaming interface 1 belongs to this AudioControl interface
    0x02,      //InterfaceNr(1) - AS #2 id AudioStreaming interface 1 belongs to this AudioControl interface
};


static const u8 uac_spk_input_terminal_desc[] = {

///USB USB Streaming IT(Speaker)
    0x0c,       //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_INPUT_TERMINAL,       //DescriptorSubType:Input Terminal
    SPK_INPUT_TERMINAL_ID,       //TerminalID
    UAC_WORD(UAC_TERMINAL_STREAMING), //TerminalType:USB Streaming
    0x00,       //AssocTerminal
    SPK_CHANNEL,       //NrChannels:2 channel
    SPK_CHANNEL_CONFIG,
    0x00,       //ChannelName String
    0x00,       //Terminal String
};
///Audio Feature Unit Descriptor(Speaker)
static const u8 uac_spk_feature_desc[] = {
    0x07 + (SPK_CHANNEL + 1) * 1, //Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_FEATURE_UNIT,       //DescriptorSubType:Audio Feature Unit
    SPK_FEATURE_UNIT_ID,       //UnitID
    SPK_INPUT_TERMINAL_ID,      //SourceID:1 #USB Streaming IT
    0x01,      //ControlSize:1 byte
    SPK_FEATURE_CONTROLS,
    0x00,       //Feature String
};
//SELECTOR_UNIT
static const u8 uac_spk_selector_uint_desc[] = {
    0x7,    //  bLength	7
    USB_DT_CS_INTERFACE,//  bDescriptorType	CS_INTERFACE (0x24)
    UAC_SELECTOR_UNIT,//    bDescriptorSubtype	SELECTOR_UNIT (0x05)
    SPK_SELECTOR_UNIT_ID,//bUnitID	33
    1,//bNrInPins	1
    SPK_FEATURE_UNIT_ID,   //baSourceID(1)	50
    0,//iSelector	None (0)
};
static const u8 uac_spk_output_terminal_desc[] = {
///USB Speaker OT
    0x09,      //Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_OUTPUT_TERMINAL,      //DescriptorSubTYpe:Output Terminal
    SPK_OUTPUT_TERMINAL_ID,      //TerminalID
    UAC_WORD(UAC_OUTPUT_TERMINAL_SPEAKER), //TerminalType:Speaker
    0,       //AssocTerminal:
    //SPK_FEATURE_UNIT_ID,      //SourceID: Feature UNIT
    SPK_SELECTOR_UNIT_ID,      //SourceID: Selector UNIT
    0x00,      //Terminal String
};

static const u8 uac_spk_as_interface_desc[] = {
//-------------------Speaker  interface---------------------//
///standard interface AS descriptor(Interface 1, Alternate Setting 0)
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x00,       //InterfaceNum:2
    0x00,       //AlternateSetting:0
    0x00,      //NumEndpoint:0
    USB_CLASS_AUDIO,       //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,      //InterfaceSubClass:audio streaming
    PC_PROTOCOL_UNDEFINED,      //InterfaceProtocol
    0x00,       //Interface String

///standard interface AS descriptor(Interface 2, Alternate Setting 1)
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x00,       //InterfaceNum
    0x01,       //AlternateSetting
    0x01,       //NumEndpoint
    USB_CLASS_AUDIO,      //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,       //InterfaceSubClass:audio streaming
    PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    0x00,      //Interface String

///Audio Streaming Interface Descriptor:AS_GENERAL(0x01),
    0x07,      //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:AS_GENERAL
    SPK_INPUT_TERMINAL_ID,      //TerminalLink:#1 USB Streaming IT
    0x01,      //Delay:1
    UAC_WORD(SPK_AUDIO_TYPE),  //FormatTag:PCM

///Type 1 Format type descriptor
    0x08 + 3 * (SPK_AUDIO_RATE_NUM),     //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_FORMAT_TYPE,      //DescriptorSubType:Format_type
    UAC_FORMAT_TYPE_I,       //FormatType:Format type 1
    SPK_CHANNEL,       //NumberOfChannel
#if SPK_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    4,                       //SubframeSize:2byte
#else
    SPK_AUDIO_RES / 8,       //SubframeSize:2byte
#endif
    SPK_AUDIO_RES,      //BitsResolution:16bit
#if SPK_AUDIO_RATE_NUM > 1
    SPK_AUDIO_RATE_NUM,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(SPK_AUDIO_RATE_1),
    UAC_3BYTE(SPK_AUDIO_RATE_2),
    UAC_3BYTE(SPK_AUDIO_RATE),
#else
    0x01,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(SPK_AUDIO_RATE),
#endif

//Isochronous,Synchronization Type(Asynchronous)
    0x09,      //Length
    USB_DT_ENDPOINT,       //DescriptorType:endpoint descriptor
    USB_DIR_OUT | SPK_ISO_EP_OUT,     //EndpointAddress:Output endpoint 1
    USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE, //0x09,
    UAC_WORD(SPK_FRAME_LEN), //MaxPacketSize
    UAC_ISO_INTERVAL,//one packet per frame 0x00,  //Interval    //Asynchronous
    0x00,//unused
    0x00,//unused

//Audio Endpoint descriptor,General
    0x07,       //Length
    USB_DT_CS_ENDPOINT,       //DescriptorType:audio endpoint descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:audio endpiont general
#if SPK_AUDIO_RATE_NUM > 1
    0x01,        //   bmAttributes (Sampling Freq Control)
#else
    0x00,      //Attributes
#endif
    0x00,       //LockDelayUnits
    0x00, 0x00, //LockDelay

#if SPK_AUDIO_RES_2
///standard interface AS descriptor(Interface 2, Alternate Setting 2)
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x00,       //InterfaceNum
    0x02,       //AlternateSetting
    0x01,       //NumEndpoint
    USB_CLASS_AUDIO,      //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,       //InterfaceSubClass:audio streaming
    PC_PROTOCOL_UNDEFINED,       //InterfaceProtocol
    0x00,      //Interface String

///Audio Streaming Interface Descriptor:AS_GENERAL(0x01),
    0x07,      //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:AS_GENERAL
    SPK_INPUT_TERMINAL_ID,      //TerminalLink:#1 USB Streaming IT
    0x01,      //Delay:1
    UAC_WORD(SPK_AUDIO_TYPE),  //FormatTag:PCM

///Type 1 Format type descriptor
    0x08 + 3 * (SPK_AUDIO_RATE_NUM),     //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_FORMAT_TYPE,      //DescriptorSubType:Format_type
    UAC_FORMAT_TYPE_I,       //FormatType:Format type 1
    SPK_CHANNEL,       //NumberOfChannel
#if SPK_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    4,                         //SubframeSize:2byte
#else
    SPK_AUDIO_RES_2 / 8,       //SubframeSize:2byte
#endif
    SPK_AUDIO_RES_2,      //BitsResolution:16bit
#if SPK_AUDIO_RATE_NUM > 1
    SPK_AUDIO_RATE_NUM,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(SPK_AUDIO_RATE_1),
    UAC_3BYTE(SPK_AUDIO_RATE_2),
    UAC_3BYTE(SPK_AUDIO_RATE),
#else
    0x01,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(SPK_AUDIO_RATE),
#endif

//Isochronous,Synchronization Type(Asynchronous)
    0x09,      //Length
    USB_DT_ENDPOINT,       //DescriptorType:endpoint descriptor
    USB_DIR_OUT | SPK_ISO_EP_OUT,     //EndpointAddress:Output endpoint 1
    USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ADAPTIVE, //0x09,
    UAC_WORD(SPK_FRAME_LEN), //MaxPacketSize
    UAC_ISO_INTERVAL,//one packet per frame 0x00,  //Interval    //Asynchronous
    0x00,//unused
    0x00,//unused

//Audio Endpoint descriptor,General
    0x07,       //Length
    USB_DT_CS_ENDPOINT,       //DescriptorType:audio endpoint descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:audio endpiont general
#if SPK_AUDIO_RATE_NUM > 1
    0x01,        //   bmAttributes (Sampling Freq Control)
#else
    0x00,      //Attributes
#endif
    0x00,       //LockDelayUnits
    0x00, 0x00, //LockDelay
#endif
};

const u8 speakerStringDescriptor[20] = {
    20,         //该描述符的长度为20字节
    0x03,       //字符串描述符的类型编码为0x03
    'U', 0x00, //U
    'S', 0x00, //S
    'B', 0x00, //B
    ' ', 0x00, //
    'A', 0x00, //A
    'u', 0x00, //u
    'd', 0x00, //d
    'i', 0x00, //i
    'o', 0x00, //o
};
/*********************************************************/
/*
			   Microphone Class
*/
/*********************************************************/

static const u8 uac_mic_ac_interface[] = {
///class-specific AC interface descriptor
    0x09,    //Length
    USB_DT_CS_INTERFACE,      //DescriptorType
    UAC_HEADER,     //DescriptorSubType
    UAC_WORD(0x0100), //bcdADC
    UAC_WORD(0x27), /*TotalLength of class-specific AC interface descriptors */
    /* Includes the combined length               */
    /* of this descriptor header and all Unit and */
    /* Terminal descriptors.                      */
    0x01,      //InCollection:1 AudioStreaming interface
    0x01,      //InterfaceNr(1) ****
};
///USB USB Streaming IT(Microphone)
static const u8  uac_mic_input_terminal_desc[] = {
    0x0c,       //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_INPUT_TERMINAL,       //DescriptorSubType:Input Terminal
    MIC_INPUT_TERMINAL_ID,       //TerminalID
    UAC_WORD(UAC_INPUT_TERMINAL_MICROPHONE),  //TerminalType:Microphone
    0,       //AssocTerminal
    MIC_CHANNEL,       //NrChannels
    MIC_CHANNEL_CONFIG,
    0x00,       //ChannelName String
    0x00,       //Terminal String
};
//SELECTOR_UNIT
static const u8 uac_mic_selector_uint_desc[] = {
    0x7,    //  bLength	7
    USB_DT_CS_INTERFACE,//  bDescriptorType	CS_INTERFACE (0x24)
    UAC_SELECTOR_UNIT,//    bDescriptorSubtype	SELECTOR_UNIT (0x05)
    MIC_SELECTOR_UNIT_ID,//bUnitID	33
    1,//bNrInPins	1
    MIC_FEATURE_UNIT_ID,   //baSourceID(1)	50
    0,//iSelector	None (0)
};

///Audio Feature Unit Descriptor(Microphone)
static const u8 uac_mic_feature_desc[] = {
    7 + (MIC_CHANNEL + 1) * 1,    	//Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_FEATURE_UNIT,       //DescriptorSubType:Audio Feature Unit
    MIC_FEATURE_UNIT_ID,       //UnitID
    MIC_INPUT_TERMINAL_ID,      //SourceID: #Microphone
    0x01,      //ControlSize:1 byte
    MIC_FEATURE_CONTROLS,
    0x00,       //Feature String
};
///USB Microphone OT
static const u8  uac_mic_output_terminal_desc[] = {
    0x09,      //Length
    USB_DT_CS_INTERFACE,      //DescriptorType:audio interface descriptor
    UAC_OUTPUT_TERMINAL,      //DescriptorSubTYpe:Output Terminal
    MIC_OUTPUT_TERMINAL_ID,      //TerminalID
    UAC_WORD(UAC_TERMINAL_STREAMING), //TerminalType:USB Sreaming
    2,       //AssocTerminal:
    MIC_SELECTOR_UNIT_ID,      //SourceID:A #Feature UNIT
    0x00,      //Terminal String
};

///standard interface AS descriptor(Interface 1, Alternate Setting 0)
static const u8 uac_mic_as_interface_desc[] = {
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x03,       //InterfaceNum ****
    0x00,       //AlternateSetting
    0x00,      //NumEndpoint
    USB_CLASS_AUDIO,       //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,      //InterfaceSubClass:audio streaming
    0x00,      //InterfaceProtocol
    0x00,       //Interface String

///standard interface AS descriptor(Interface 2, Alternate Setting 1)
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x02,       //InterfaceNum ****
    0x01,       //AlternateSetting
    0x01,       //NumEndpoint
    USB_CLASS_AUDIO,      //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,       //InterfaceSubClass:audio streaming
    0x00,       //InterfaceProtocol
    0x00,      //Interface String

///Audio Streaming Interface Descriptor:AS_GENERAL
    0x07,      //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:AS_GENERAL
    MIC_OUTPUT_TERMINAL_ID,      //TerminalLink:#1 USB Streaming OT
    0x01,      //Delay:1
    UAC_WORD(MIC_AUDIO_TYPE),  //FormatTag:PCM

///Type 1 Format type descriptor
    0x08 + 3 * (MIC_AUDIO_RATE_NUM), //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_FORMAT_TYPE,      //DescriptorSubType:Format_type
    UAC_FORMAT_TYPE_I_PCM,       //FormatType:Format type 1
    MIC_CHANNEL,       //NumberOfChannel:1
#if MIC_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    4,                 //SubframeSize:2byte
#else
    MIC_AUDIO_RES / 8, //SubframeSize:2byte
#endif
    MIC_AUDIO_RES,     //BitsResolution:16bit
#if MIC_AUDIO_RATE_NUM > 1
    MIC_AUDIO_RATE_NUM,
    UAC_3BYTE(MIC_AUDIO_RATE_1),
    UAC_3BYTE(MIC_AUDIO_RATE_2),
    UAC_3BYTE(MIC_AUDIO_RATE),
#else
    0x01,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(MIC_AUDIO_RATE),
#endif

///Endpoint 1 - Standard Descriptor:Output Endpoint1
//Isochronous,Synchronization Type(Asynchronous)
    USB_DT_ENDPOINT_AUDIO_SIZE,      //Length
    USB_DT_ENDPOINT,       //DescriptorType:endpoint descriptor
    USB_DIR_IN | MIC_ISO_EP_IN,       //EndpointAddress:Output endpoint
    USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
    UAC_WORD(MIC_FRAME_LEN), //MaxPacketSize
    UAC_ISO_INTERVAL,//one packet per frame   //Interval
    0x00,//unused
    0x00,//unused

///Endpoint - Audio Streaming Descriptor
//Audio Endpoint descriptor,General
    0x07,       //Length
    USB_DT_CS_ENDPOINT,       //DescriptorType:audio endpoint descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:audio endpiont general
#if MIC_AUDIO_RATE_NUM > 1
    0x01,        //   bmAttributes (Sampling Freq Control)
#else
    0x00,      //Attributes
#endif
    0x00,       //LockDelayUnits
    0x00, 0x00, //LockDelay

#if MIC_AUDIO_RES_2
///standard interface AS descriptor(Interface 2, Alternate Setting 2)
    USB_DT_INTERFACE_SIZE,      //Length
    USB_DT_INTERFACE,      //DescriptorType:Interface
    0x02,       //InterfaceNum ****
    0x02,       //AlternateSetting
    0x01,       //NumEndpoint
    USB_CLASS_AUDIO,      //InterfaceClass:audio
    USB_SUBCLASS_AUDIOSTREAMING,       //InterfaceSubClass:audio streaming
    0x00,       //InterfaceProtocol
    0x00,      //Interface String

///Audio Streaming Interface Descriptor:AS_GENERAL
    0x07,      //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:AS_GENERAL
    MIC_OUTPUT_TERMINAL_ID,      //TerminalLink:#1 USB Streaming OT
    0x01,      //Delay:1
    UAC_WORD(MIC_AUDIO_TYPE),  //FormatTag:PCM

///Type 1 Format type descriptor
    0x08 + 3 * (MIC_AUDIO_RATE_NUM), //Length
    USB_DT_CS_INTERFACE,       //DescriptorType:audio interface descriptor
    UAC_FORMAT_TYPE,      //DescriptorSubType:Format_type
    UAC_FORMAT_TYPE_I_PCM,       //FormatType:Format type 1
    MIC_CHANNEL,       //NumberOfChannel:1
#if MIC_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    4,                   //SubframeSize:2byte
#else
    MIC_AUDIO_RES_2 / 8, //SubframeSize:2byte
#endif
    MIC_AUDIO_RES_2,     //BitsResolution:16bit
#if MIC_AUDIO_RATE_NUM > 1
    MIC_AUDIO_RATE_NUM,
    UAC_3BYTE(MIC_AUDIO_RATE_1),
    UAC_3BYTE(MIC_AUDIO_RATE_2),
    UAC_3BYTE(MIC_AUDIO_RATE),
#else
    0x01,      //SampleFreqType:One sampling frequency.
    UAC_3BYTE(MIC_AUDIO_RATE),
#endif

///Endpoint 1 - Standard Descriptor:Output Endpoint1
//Isochronous,Synchronization Type(Asynchronous)
    USB_DT_ENDPOINT_AUDIO_SIZE,      //Length
    USB_DT_ENDPOINT,       //DescriptorType:endpoint descriptor
    USB_DIR_IN | MIC_ISO_EP_IN,       //EndpointAddress:Output endpoint
    USB_ENDPOINT_XFER_ISOC | USB_ENDPOINT_SYNC_ASYNC,
    UAC_WORD(MIC_FRAME_LEN), //MaxPacketSize
    UAC_ISO_INTERVAL,//one packet per frame   //Interval
    0x00,//unused
    0x00,//unused

///Endpoint - Audio Streaming Descriptor
//Audio Endpoint descriptor,General
    0x07,       //Length
    USB_DT_CS_ENDPOINT,       //DescriptorType:audio endpoint descriptor
    UAC_AS_GENERAL,       //DescriptorSubType:audio endpiont general
#if MIC_AUDIO_RATE_NUM > 1
    0x01,        //   bmAttributes (Sampling Freq Control)
#else
    0x00,      //Attributes
#endif
    0x00,       //LockDelayUnits
    0x00, 0x00, //LockDelay
#endif
};
static const u8 micStringDescriptor[30] = {
    30,         //该描述符的长度为30字节
    0x03,       //字符串描述符的类型编码为0x03
    'U', 0x00, //U
    'S', 0x00, //S
    'B', 0x00, //B
    ' ', 0x00, //
    'M', 0x00, //M
    'i', 0x00, //i
    'c', 0x00, //c
    'r', 0x00, //r
    'o', 0x00, //o
    'p', 0x00, //p
    'h', 0x00, //h
    'o', 0x00, //o
    'n', 0x00, //n
    'e', 0x00, //e
};

const u8 *uac_get_string(u32 id)
{
    if (id == SPEAKER_STR_INDEX) {
        return speakerStringDescriptor;
    } else if (id == MIC_STR_INDEX) {
        return micStringDescriptor;
    }
    return NULL;
}
struct spk_info_t {
    u16 spk_left_vol;
    u16 spk_right_vol;
    u16 spk_max_vol;
    u16 spk_min_vol;
    u16 spk_def_vol;
    u16 spk_vol_res;
    u8 *spk_dma_buffer;
    u32 spk_sampling_rate;
    u16 spk_padding_interval;
    u16 spk_padding_cnt;
    u16 res;
    u16 sof_cnt;
    u8  spk_mute;
    u8  ps4_mode;
    u8  alternate_setting;
    u8  downstream_halt;
};

struct mic_info_t {
    u16 mic_max_vol;
    u16 mic_min_vol;
    u16 mic_def_vol;
    u16 mic_vol_res;
    u16 mic_vol;
    u8 *mic_dma_buffer;
    u32 mic_sampling_rate;
    u16 mic_padding_interval;
    u16 mic_padding_cnt;
    u16 res;
    u16 sof_cnt;
    u8  mic_mute;
    u8  bAGC;
    u8  ps4_mode;
    u8  alternate_setting;
    u8  mic_no_data;
    u8  upstream_halt;
};

struct spk_info_t *spk_info;
struct mic_info_t *mic_info;
#if USB_MALLOC_ENABLE
#else
struct spk_info_t _spk_info SEC(.usb.data.bss.exchange);
struct mic_info_t _mic_info SEC(.usb.data.bss.exchange);
#endif

static void open_spk(struct usb_device_t *usb_device);
static void open_mic(struct usb_device_t *usb_device);

static u32 vol_convert(u16 v)
{
    const u16 vol_table[] = {
        0xe3a0, 0xe461, 0xe519, 0xe5c8, 0xe670, 0xe711, 0xe7ac, 0xe840, 0xe8cf, 0xe959,
        0xe9df, 0xea60, 0xeadc, 0xeb55, 0xebca, 0xec3c, 0xecab, 0xed16, 0xed7f, 0xede5,
        0xee48, 0xeea9, 0xef08, 0xef64, 0xefbe, 0xf016, 0xf06c, 0xf0c0, 0xf113, 0xf164,
        0xf1b3, 0xf200, 0xf24c, 0xf297, 0xf2e0, 0xf328, 0xf36e, 0xf3b4, 0xf3f8, 0xf43a,
        0xf47c, 0xf4bd, 0xf4fc, 0xf53b, 0xf579, 0xf5b5, 0xf5f1, 0xf62c, 0xf666, 0xf69f,
        0xf6d7, 0xf70e, 0xf745, 0xf77b, 0xf7b0, 0xf7e5, 0xf818, 0xf84b, 0xf87e, 0xf8b0,
        0xf8e1, 0xf911, 0xf941, 0xf970, 0xf99f, 0xf9cd, 0xf9fb, 0xfa28, 0xfa55, 0xfa81,
        0xfaad, 0xfad8, 0xfb03, 0xfb2d, 0xfb56, 0xfb80, 0xfba9, 0xfbd1, 0xfbf9, 0xfc21,
        0xfc48, 0xfc6f, 0xfc95, 0xfcbb, 0xfce1, 0xfd06, 0xfd2b, 0xfd50, 0xfd74, 0xfd98,
        0xfdbc, 0xfddf, 0xfe02, 0xfe25, 0xfe47, 0xfe69, 0xfe8b, 0xfead, 0xfece, 0xfeef,
        0xff0f,
    };

    if (v <= 100) {
        return vol_table[v];
    }

    for (int i = 0; i < sizeof(vol_table) / 2; i++) {
        if (v <= vol_table[i]) {
            return i;
        }
    }

    return 0;
}

void uac_get_cur_vol(const usb_dev usb_id, u16 *l_vol, u16 *r_vol)
{
    if (l_vol) {
        *l_vol = 0;
    }
    if (r_vol) {
        *r_vol = 0;
    }
    if (spk_info) {
        if (!spk_info->spk_mute) {
            if (l_vol) {
                *l_vol = vol_convert(spk_info->spk_left_vol);
            }
            if (r_vol) {
                *r_vol = vol_convert(spk_info->spk_right_vol);
            }
        }
    }
}
u16 uac_get_mic_vol(const usb_dev usb_id)
{
    if (mic_info) {
        if (!mic_info->mic_mute) {
            return vol_convert(mic_info->mic_vol);
        }
    }
    return 0;
}

u32 uac_get_mic_sameplerate(void *priv)
{
    u32 sample_rate = MIC_AUDIO_RATE;
    if (mic_info) {
        sample_rate = mic_info->mic_sampling_rate;
    }
    return sample_rate;
}

static u32 uac_vol_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[8];
    u8 mute;
    u16 volume = 0;
    usb_read_ep0(usb_id, read_ep, setup->wLength);
    ret = USB_EP0_STAGE_SETUP;
    switch (HIBYTE(setup->wValue)) {
    case UAC_FU_MUTE:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info == NULL) {
                break;
            }
            memcpy((u8 *)&mute, read_ep, 1);
            if (mute) {
                uac_mute_volume(SPK_FEATURE_UNIT_ID, 0, 0);
                spk_info->spk_mute = 1;
            } else {
                uac_mute_volume(SPK_FEATURE_UNIT_ID,
                                vol_convert(spk_info->spk_left_vol),
                                vol_convert(spk_info->spk_right_vol));
                spk_info->spk_mute = 0;
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info == NULL) {
                break;
            }
            memcpy((u8 *)&mute, read_ep, 1);
            if (mute) {
                uac_mute_volume(MIC_FEATURE_UNIT_ID, 0, 0);
                mic_info->mic_mute = 1;
            } else {
                uac_mute_volume(MIC_FEATURE_UNIT_ID, vol_convert(mic_info->mic_vol), 0);
                mic_info->mic_mute = 0;
            }
        }
        break;
    case UAC_FU_VOLUME:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info == NULL) {
                break;
            }
            memcpy((u8 *)&volume, read_ep, setup->wLength);
            if (LOBYTE(setup->wValue) == 0) {
                spk_info->spk_left_vol = volume;
                spk_info->spk_right_vol = volume;
            } else if (LOBYTE(setup->wValue) == 1) {
                spk_info->spk_left_vol = volume;
            } else if (LOBYTE(setup->wValue) == 2) {
                spk_info->spk_right_vol = volume;
            }
            if (spk_info->ps4_mode) {
                spk_info->spk_left_vol = vol_convert(100);
                spk_info->spk_right_vol = vol_convert(100);
            }
            if (usb_get_host_type(usb_id) == HOST_TYPE_ANDROID) {
                //安卓没有音量同步，需要把uac端音量设最大
                spk_info->spk_left_vol = vol_convert(100);
                spk_info->spk_right_vol = vol_convert(100);
            }
            if (!spk_info->spk_mute && !spk_info->ps4_mode) {
                uac_mute_volume(SPK_FEATURE_UNIT_ID,
                                vol_convert(spk_info->spk_left_vol),
                                vol_convert(spk_info->spk_right_vol));
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info == NULL) {
                break;
            }
            memcpy((u8 *)&mic_info->mic_vol, read_ep, sizeof(mic_info->mic_vol));
            /* if (mic_info->mic_vol == 0x8000) { */
            /* mic_info->mic_vol = 0; */
            /* } else if (mic_info->mic_vol >= 100) { */
            /* mic_info->mic_vol = 99; */
            /* } */
            if (!mic_info->mic_mute) {
                uac_mute_volume(MIC_FEATURE_UNIT_ID, vol_convert(mic_info->mic_vol), 0);
            }
        }
        break;
    case UAC_FU_AUTOMATIC_GAIN:
        if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info == NULL) {
                break;
            }
            if (read_ep[0]) {
                mic_info->bAGC = 1;
            } else {
                mic_info->bAGC = 0;
            }
        }
        break;

    case 0: //bSelectorUnitID
        if ((HIBYTE(setup->wIndex) == SPK_SELECTOR_UNIT_ID)) {
            printf("SELECTOR_UNIT_ID");
            if (spk_info) {
                spk_info->ps4_mode = read_ep[0];
                usb_set_host_type(usb_id, HOST_TYPE_PS4);
                if (read_ep[0]) {
                    spk_info->spk_left_vol = vol_convert(100);
                    spk_info->spk_right_vol = vol_convert(100);
                    uac_mute_volume(SPK_FEATURE_UNIT_ID, 100, 100);
                }
            }
        }
        break;

    default :
        ret = USB_EP0_SET_STALL;
        break;
    }

    return ret;
}
#if USB_ROOT2
static u8 SetInterface_0_Lock;
#endif
static u32 uac_endpoint_recv(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[8];

    usb_read_ep0(usb_id, read_ep, setup->wLength);

    ret = USB_EP0_STAGE_SETUP;
    u32 ep = LOBYTE(setup->wIndex);
    switch (ep) {
    case  MIC_ISO_EP_IN|USB_DIR_IN:
        if (mic_info == NULL) {
            break;
        }
        memcpy(&mic_info->mic_sampling_rate, read_ep, 3);
#if MIC_AUDIO_RATE_NUM > 1
        open_mic(usb_device);
#endif
        break;
    case SPK_ISO_EP_OUT|USB_DIR_OUT:
        if (spk_info == NULL) {
            break;
        }
        memcpy(&spk_info->spk_sampling_rate, read_ep, 3);
#if SPK_AUDIO_RATE_NUM > 1
        open_spk(usb_device);
#endif
        break;
    }
    return ret;
}
u32 uac_setup_endpoint(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    u32 ret = -1;
    u8 type = req->bRequestType & USB_TYPE_MASK;
    if (type != USB_TYPE_CLASS) {
        return ret;
    }
    if (req->bRequest != 0x01) {
        return ret;
    }
    if (req->wValue != 0x0100) {
        return ret;
    }
    usb_set_setup_recv(usb_device, uac_endpoint_recv);
    return 0;
}
static u32 audio_ac_itf_handler(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    u8 mute;

    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;
    switch (setup->bRequest) {
    case USB_REQ_SET_INTERFACE:
#if USB_ROOT2
        SetInterface_0_Lock = 1;
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);//no alt setting
        }
#else
        if (setup->wValue == 0) { //alt 0
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
#endif
        break;
    case USB_REQ_GET_INTERFACE:
#if USB_ROOT2
        if (usb_root2_testing()) {
            SetInterface_0_Lock = 0;
        }
#endif
        if (setup->wValue || (setup->wLength != 1)) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            tx_len = 1;
            tx_payload[0] = 0x00;
            usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
        }
        break;
    case UAC_SET_CUR:
        /* USB_DEBUG_PRINTF("set vol && mute"); */
        usb_set_setup_recv(usb_device, uac_vol_handler);
        break;
    case UAC_GET_CUR:
        switch (HIBYTE(setup->wValue)) {
        case UAC_FU_MUTE:
            if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
                if (spk_info) {
                    mute = spk_info->spk_mute ;
                }
            } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
                if (mic_info) {
                    mute = mic_info->mic_mute ;
                }
            }
            tx_payload = &mute;
            tx_len = setup->wLength;
            usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            break;

        case UAC_FU_VOLUME:
            if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
                if (spk_info) {
                    if (LOBYTE(setup->wValue) == 0) {
                        tx_payload = (u8 *)&spk_info->spk_left_vol;
                    } else if (LOBYTE(setup->wValue) == 1) {
                        tx_payload = (u8 *)&spk_info->spk_left_vol;
                    } else if (LOBYTE(setup->wValue) == 2) {
                        tx_payload = (u8 *)&spk_info->spk_right_vol;
                    }
                }
            } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
                if (mic_info) {
                    tx_payload = (u8 *)&mic_info->mic_vol;
                }
            }
            tx_len = setup->wLength;
            usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            break;
        case UAC_FU_AUTOMATIC_GAIN:
            if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
                if (mic_info) {
                    if (mic_info->bAGC) {
                        tx_payload[0] = 0x01;
                    } else {
                        tx_payload[0] = 0x00;
                    }
                }
                tx_len = setup->wLength;
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
            break;
        }
        break;
    case UAC_GET_DEF:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info) {
                tx_payload = (u8 *)&spk_info->spk_def_vol;
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info) {
                tx_payload = (u8 *)&mic_info->mic_def_vol;
            }
        }
        tx_len = setup->wLength;
        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
        break;
    case UAC_GET_MAX:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info) {
                tx_payload = (u8 *)&spk_info->spk_max_vol;
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info) {
                tx_payload = (u8 *)&mic_info->mic_max_vol;
            }
        }
        tx_len = setup->wLength;
        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
        break;
    case UAC_GET_MIN:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info) {
                tx_payload = (u8 *)&spk_info->spk_min_vol;
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info) {
                tx_payload = (u8 *)&mic_info->mic_min_vol;
            }
        }
        tx_len = setup->wLength;
        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
        break;
    case UAC_GET_RES:
        if (HIBYTE(setup->wIndex) == SPK_FEATURE_UNIT_ID) {
            if (spk_info) {
                tx_payload = (u8 *)&spk_info->spk_vol_res;
            }
        } else if (HIBYTE(setup->wIndex) == MIC_FEATURE_UNIT_ID) {
            if (mic_info) {
                tx_payload = (u8 *)&mic_info->mic_vol_res;
            }
        }
        tx_len = setup->wLength;
        usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
        break;
    case UAC_GET_LEN:
        break;
    case UAC_GET_INFO:
        break;
    case USB_REQ_GET_STATUS:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    default:
        ret = 1 ;
        break;
    }
    return ret;
}
#define     UAC_PLC_EN  0
#if UAC_PLC_EN
typedef struct _USBAUDIO_PLC_API {
    unsigned int (*need_buf)();
    void (*open)(unsigned char *ptr, int sr, int nch);
    int (*run)(unsigned char *ptr, short *inbuf, short *oubuf, short len, short err_flag);
} USBAUDIO_PLC_API;

extern USBAUDIO_PLC_API *get_USBplc_api();

static USBAUDIO_PLC_API *uac_plc_api = 0;
static u8 *uac_plc_buffer;

void usb_audio_plc_open()
{
    uac_plc_api = get_USBplc_api();
    u32 bufsize = uac_plc_api->need_buf();
    uac_plc_buffer = malloc(bufsize);
    uac_plc_api->open(uac_plc_buffer, SPK_AUDIO_RATE, SPK_CHANNEL);
}
void usb_audio_plc_close()
{
    if (uac_plc_api) {
        uac_plc_api = NULL;
    }
    if (uac_plc_buffer) {
        free(uac_plc_buffer);
        uac_plc_buffer = NULL;
    }
}
#endif

static u32 spk_get_max_sampling_rate()
{
    u32 sr = 0;
#if SPK_AUDIO_RATE_NUM > 1
    //find the max sample rate
    if (sr < SPK_AUDIO_RATE_1) {
        sr = SPK_AUDIO_RATE_1;
    }
    if (sr < SPK_AUDIO_RATE_2) {
        sr = SPK_AUDIO_RATE_2;
    }
    if (sr < SPK_AUDIO_RATE) {
        sr = SPK_AUDIO_RATE;
    }
#else
    sr = SPK_AUDIO_RATE;
#endif
    return sr;
}
static u32 spk_get_max_frame_len()
{
    u32 sr;
    u32 spk_res;
    u32 spk_frame_len;

    sr = spk_get_max_sampling_rate();
    spk_res = SPK_AUDIO_RES;
#if SPK_AUDIO_RES_2
    if (SPK_AUDIO_RES < SPK_AUDIO_RES_2) {
        spk_res = SPK_AUDIO_RES_2;
    }
#endif
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 24) {
        spk_res = 32;
    }
#endif
    spk_frame_len = ((sr * (spk_res / 8) * SPK_CHANNEL) / 1000);
    spk_frame_len += ((sr % 1000) ? (spk_res / 8) * SPK_CHANNEL : 0);
    return spk_frame_len;
}
static void spk_transfer(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 spk_res = 0;
    u32 spk_frame_len;
    u32 rx_len;

    if (spk_info == NULL) {
        return;
    }
    ep_buffer = spk_info->spk_dma_buffer;
    spk_res = spk_info->res;
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 24) {
        spk_res = 32;
    }
#endif
    spk_frame_len = spk_info->spk_sampling_rate * (spk_res / 8) * SPK_CHANNEL / 1000;
    spk_frame_len += ((spk_info->spk_sampling_rate % 1000) ? (spk_res / 8) * SPK_CHANNEL : 0);
    rx_len = usb_g_iso_read(usb_id, SPK_ISO_EP_OUT, NULL, spk_frame_len, 0);

#if UAC_PLC_EN
    if (rx_len) {
        uac_plc_api->run(uac_plc_buffer, ep_buffer, ep_buffer, rx_len, 0);
    } else {
        uac_plc_api->run(uac_plc_buffer, ep_buffer, ep_buffer, rx_len, 1);
    }
#endif

    uac_speaker_stream_write(ep_buffer, rx_len);
    spk_info->downstream_halt = 0;
}
static void spk_feed_data_when_downstream_halt(struct usb_device_t *usb_device, u32 ep, u32 frame)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 spk_res;
    u32 spk_frame_len;
    u32 padding_len = 0;

    if (spk_info == NULL) {
        return;
    }
    //由于uac_speaker_stream_close_delay()的机制，实际上音频流不会
    //立刻关闭，如果这段时间内又open，中间缺失了数据，给到音频流的
    //数据量会不够，变采样处理不过来声音就会变调。所以需要手工补够
    //数据
    if (uac_speaker_stream_status() == 0) {
        usb_g_set_sof_hander(usb_id, SPK_ISO_EP_OUT, NULL);
        return;
    }
#ifdef USB_HW_20
#if FUSB_MODE == 0
    //接到high-speed host时每8个微帧补偿一次；接到full-speed host时每个帧补偿
    //一次
    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        if ((++spk_info->sof_cnt % 8) != 0) {
            return;
        }
    }
#endif
#endif
    spk_res = spk_info->res;
    if (spk_info->spk_padding_interval) {
        if (++spk_info->spk_padding_cnt == spk_info->spk_padding_interval) {
            spk_info->spk_padding_cnt = 0;
            padding_len = (spk_res / 8) * SPK_CHANNEL;
        }
    }
    ep_buffer = spk_info->spk_dma_buffer;
    if (spk_info->downstream_halt >= 5) {
        spk_info->downstream_halt = 5;  //avoid numerical overflow
        spk_frame_len = spk_info->spk_sampling_rate * (spk_res / 8) * SPK_CHANNEL / 1000;
        spk_frame_len += padding_len;
        memset(ep_buffer, 0, spk_frame_len);
        uac_speaker_stream_write(ep_buffer, spk_frame_len);
    }
    spk_info->downstream_halt++;
}
static void open_spk(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 spk_frame_len;
    u8 *ep_buffer;
    u32 spk_res = 0;

    if (spk_info == NULL) {
        return;
    }
    log_info("%s", __func__);
    if (spk_info->spk_sampling_rate == 0) {
        spk_info->spk_sampling_rate = spk_get_max_sampling_rate();
    }
    if (spk_info->alternate_setting == 1) {
        spk_res = SPK_AUDIO_RES;
    } else if (spk_info->alternate_setting == 2) {
        spk_res = SPK_AUDIO_RES_2;
    } else {
        return;
    }
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 24) {
        spk_res = 32;
    }
#endif
    spk_frame_len = spk_info->spk_sampling_rate * (spk_res / 8) * SPK_CHANNEL / 1000;
    spk_frame_len += ((spk_info->spk_sampling_rate % 1000) ? (spk_res / 8) * SPK_CHANNEL : 0);
    if (spk_info->spk_sampling_rate % 1000) {
        spk_info->spk_padding_interval = 1000 / (spk_info->spk_sampling_rate % 1000);
        spk_info->spk_padding_cnt = 0;
    } else {
        spk_info->spk_padding_interval = 0;
        spk_info->spk_padding_cnt = 0;
    }
#if UAC_24BIT_IN_4BYTE
    if (spk_res == 32) {
        spk_res = 24;
    }
#endif
    spk_info->res = spk_res;
    ep_buffer = spk_info->spk_dma_buffer;
    uac_speaker_stream_open(spk_info->spk_sampling_rate, SPK_CHANNEL, spk_res);

#if UAC_PLC_EN
    usb_audio_plc_open();
#endif

    usb_g_set_intr_hander(usb_id, SPK_ISO_EP_OUT | USB_DIR_OUT, spk_transfer);

    spk_info->sof_cnt = 0;
    spk_info->downstream_halt = 0;
    //usb_g_set_sof_hander(usb_id, SPK_ISO_EP_OUT, spk_feed_data_when_downstream_halt);

    usb_g_ep_config(usb_id, SPK_ISO_EP_OUT | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, spk_frame_len);
}
static void close_spk(struct usb_device_t *usb_device)
{
    log_info("%s", __func__);
    const usb_dev usb_id = usb_device2id(usb_device);
    int release = 0;
    usb_clr_intr_rxe(usb_id, SPK_ISO_EP_OUT);
    usb_g_set_intr_hander(usb_id, SPK_ISO_EP_OUT, NULL);
    //不要关sof补audio数据，spk close有delay处理，仍然需要在audio关闭
    //之前补数据
    //usb_g_set_sof_hander(usb_id, SPK_ISO_EP_OUT, NULL);

#if UAC_PLC_EN
    usb_audio_plc_close();
#endif

    if (spk_info) {
        release = (spk_info->alternate_setting == (u8)(-1)) ? 1 : 0;
    }
    uac_speaker_stream_close(release);

}
static u32 spk_as_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;

    switch (setup->bRequest) {
    case USB_REQ_SET_INTERFACE:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            if (spk_info) {
                spk_info->alternate_setting = setup->wValue;
            }
#if USB_ROOT2
            if (!usb_root2_testing()) {
                SetInterface_0_Lock = 1;
            }
            if (SetInterface_0_Lock == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                if (setup->wValue == 1) { //alt 1
                    open_spk(usb_device);
                } else if (setup->wValue == 0) { //alt 0
                    close_spk(usb_device);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
#else
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            if (setup->wValue == 1) { //alt 1
#if SPK_AUDIO_RATE_NUM == 1
                open_spk(usb_device);
#endif
            } else if (setup->wValue == 2) { //alt 2
#if SPK_AUDIO_RATE_NUM == 1
                open_spk(usb_device);
#endif
            } else if (setup->wValue == 0) { //alt 0
                close_spk(usb_device);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
#endif
        }

        break;
    case USB_REQ_GET_INTERFACE:
        if (usb_root2_testing()) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            if (setup->wValue || (setup->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                if (spk_info) {
                    tx_payload[0] = spk_info->alternate_setting;
                }
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
        }
        break;
    case USB_REQ_GET_STATUS:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    default:
        break;
    }
    return ret;
}
static u32 mic_get_max_sampling_rate()
{
    u32 sr = 0;
#if MIC_AUDIO_RATE_NUM > 1
    //find the max sample rate
    if (sr < MIC_AUDIO_RATE_1) {
        sr = MIC_AUDIO_RATE_1;
    }
    if (sr < MIC_AUDIO_RATE_2) {
        sr = MIC_AUDIO_RATE_2;
    }
    if (sr < MIC_AUDIO_RATE) {
        sr = MIC_AUDIO_RATE;
    }
#else
    sr = MIC_AUDIO_RATE;
#endif
    return sr;
}
static u32 mic_get_max_frame_len()
{
    u32 sr;
    u32 mic_res;
    u32 mic_frame_len;

    sr = mic_get_max_sampling_rate();
    mic_res = MIC_AUDIO_RES;
#if MIC_AUDIO_RES_2
    if (MIC_AUDIO_RES < MIC_AUDIO_RES_2) {
        mic_res = MIC_AUDIO_RES_2;
    }
#endif
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 24) {
        mic_res = 32;
    }
#endif
    mic_frame_len = ((sr * (mic_res / 8) * MIC_CHANNEL) / 1000);
    mic_frame_len += ((sr % 1000) ? (mic_res / 8) * MIC_CHANNEL : 0);
    return mic_frame_len;
}
static void mic_transfer(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res = 0;

    if (mic_info == NULL) {
        return;
    }
    ep_buffer = mic_info->mic_dma_buffer;
    mic_res = mic_info->res;
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 24) {
        mic_res = 32;
    }
#endif
    mic_frame_len = ((mic_info->mic_sampling_rate * mic_res / 8 * MIC_CHANNEL) / 1000);
    //非对齐采样率补点
    if (mic_info->mic_padding_interval) {
        if (++mic_info->mic_padding_cnt == mic_info->mic_padding_interval) {
            mic_info->mic_padding_cnt = 0;
            mic_frame_len += (mic_res / 8) * MIC_CHANNEL;
        }
    }

    int len = uac_mic_stream_read(ep_buffer,  mic_frame_len);
    if (len) {
        mic_info->mic_no_data = 0;
    } else { /* if (mic_info->mic_no_data) */
        len = mic_frame_len;
        memset(ep_buffer, 0, len);
    }

    usb_g_iso_write(usb_id, MIC_ISO_EP_IN, NULL, len);

    mic_info->upstream_halt = 0;
}
static void mic_drop_data_when_upstream_halt(struct usb_device_t *usb_device, u32 ep, u32 frame)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res = 0;

    if (mic_info == NULL) {
        return;
    }

    if (uac_get_mic_stream_status() == 0) {
        usb_g_set_sof_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, NULL);
        return;
    }
#ifdef USB_HW_20
#if FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_HIGH) {
        if ((++mic_info->sof_cnt % 8) != 0) {
            return;
        }
    }
#endif
#endif

    ep_buffer = mic_info->mic_dma_buffer;

    if (mic_info->upstream_halt >= 5) {
        mic_info->upstream_halt = 5;  //avoid numerical overflow
        mic_res = mic_info->res;
#if UAC_24BIT_IN_4BYTE
        if (mic_res == 24) {
            mic_res = 32;
        }
#endif
        mic_frame_len = ((mic_info->mic_sampling_rate * mic_res / 8 * MIC_CHANNEL) / 1000);
        //非对齐采样率补点
        if (mic_info->mic_padding_interval) {
            if (++mic_info->mic_padding_cnt == mic_info->mic_padding_interval) {
                mic_info->mic_padding_cnt = 0;
                mic_frame_len += (mic_res / 8) * MIC_CHANNEL;
            }
        }
        uac_mic_stream_read(ep_buffer,  mic_frame_len);
    }
    mic_info->upstream_halt++;
}
static void open_mic(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *ep_buffer;
    u32 mic_frame_len;
    u32 mic_res = 0;

    if (mic_info == NULL) {
        return;
    }
    log_info("%s", __func__);
    mic_info->mic_no_data = 1;
    usb_enable_ep(usb_id, MIC_ISO_EP_IN);

    ep_buffer = mic_info->mic_dma_buffer;

    usb_g_set_intr_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, mic_transfer);

    if (mic_info->mic_sampling_rate == 0) {
        mic_info->mic_sampling_rate = mic_get_max_sampling_rate();
    }
    if (mic_info->alternate_setting == 1) {
        mic_res = MIC_AUDIO_RES;
    } else if (mic_info->alternate_setting == 2) {
        mic_res = MIC_AUDIO_RES_2;
    } else {
        return;
    }
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 24) {
        mic_res = 32;
    }
#endif
    mic_frame_len = ((mic_info->mic_sampling_rate * mic_res / 8 * MIC_CHANNEL) / 1000);
    mic_frame_len += ((mic_info->mic_sampling_rate % 1000) ? (mic_res / 8) * MIC_CHANNEL : 0);
    if (mic_info->mic_sampling_rate % 1000) {
        mic_info->mic_padding_interval = 1000 / (mic_info->mic_sampling_rate % 1000);
        mic_info->mic_padding_cnt = 0;
    } else {
        mic_info->mic_padding_interval = 0;
        mic_info->mic_padding_cnt = 0;
    }
#if UAC_24BIT_IN_4BYTE
    if (mic_res == 32) {
        mic_res = 24;
    }
#endif

    mic_info->res = mic_res;
    uac_mic_stream_open(mic_info->mic_sampling_rate, MIC_CHANNEL, mic_res);

    mic_info->sof_cnt = 0;
    mic_info->upstream_halt = 0;
    usb_g_set_sof_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, mic_drop_data_when_upstream_halt);

    usb_g_ep_config(usb_id,
                    MIC_ISO_EP_IN | USB_DIR_IN,
                    USB_ENDPOINT_XFER_ISOC,
                    1, ep_buffer, mic_frame_len);

    mic_transfer(usb_device, MIC_ISO_EP_IN);
}
static void close_mic(struct usb_device_t *usb_device)
{
    log_info("%s", __func__);
    const usb_dev usb_id = usb_device2id(usb_device);
    int release = 0;
    usb_clr_intr_txe(usb_id, MIC_ISO_EP_IN);
    usb_g_set_intr_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, NULL);
    if (mic_info) {
        release = (mic_info->alternate_setting == (u8)(-1)) ? 1 : 0;
    }
    uac_mic_stream_close(release);
}
static u32 mic_as_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 tx_len;
    u32 bRequestType = setup->bRequestType & USB_TYPE_MASK;
    u32 ret = 0;

    switch (setup->bRequest) {
    case USB_REQ_SET_INTERFACE:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
            if (mic_info) {
                mic_info->alternate_setting = setup->wValue;
            }
#if USB_ROOT2
            if (!usb_root2_testing()) {
                SetInterface_0_Lock = 1;
            }
            if (SetInterface_0_Lock == 0) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                if (setup->wValue == 1) {//alt 1
                    open_mic(usb_device);
                } else if (setup->wValue == 0) { //alt 0
                    close_mic(usb_device);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
#else
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            if (setup->wValue == 1) {//alt 1
#if MIC_AUDIO_RATE_NUM == 1
                open_mic(usb_device);
#endif
            } else if (setup->wValue == 2) {//alt 2
#if MIC_AUDIO_RATE_NUM == 1
                open_mic(usb_device);
#endif
            } else if (setup->wValue == 0) { //alt 0
                close_mic(usb_device);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
#endif
        }
        break;
    case USB_REQ_GET_INTERFACE:
        if (usb_root2_testing()) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            if (setup->wValue || (setup->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                if (mic_info) {
                    tx_payload[0] = mic_info->alternate_setting;
                }
                usb_set_data_payload(usb_device, setup, tx_payload, tx_len);
            }
        }
        break;
    case USB_REQ_GET_STATUS:
        if (usb_device->bDeviceStates == USB_DEFAULT) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else if (usb_device->bDeviceStates == USB_ADDRESS) {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        } else {
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }
        break;
    default:
        break;
    }
    return ret;
}
void spk_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s", __func__);
    if (spk_info) {
        u8 *ep_buffer = spk_info->spk_dma_buffer;
        usb_g_ep_config(usb_id, SPK_ISO_EP_OUT | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, spk_get_max_frame_len());
    }
}
u32 uac_spk_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    //cppcheck-suppress unreadVariable
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    int i = 0;
    u8 *tptr = ptr;
    u32 offset;
    u32 frame_len;
    u32 sr;
    u8 *pCtrlItf;
    u32 dwCtrlSize = 0;

    log_debug("spkdc:%d\n", *cur_itf_num);

    memcpy(tptr, (u8 *)uac_audio_interface_association, sizeof(uac_audio_interface_association));
    tptr[2] = *cur_itf_num;
    tptr[3] = 2;
    tptr += sizeof(uac_audio_interface_association);//0x09

    memcpy(tptr, (u8 *)uac_ac_standard_interface_desc, sizeof(uac_ac_standard_interface_desc));
    tptr[2] = *cur_itf_num;
    tptr += sizeof(uac_ac_standard_interface_desc);//0x09

    memcpy(tptr, (u8 *)uac_spk_ac_interface, sizeof(uac_spk_ac_interface));
    tptr[8] = *cur_itf_num + 1;
    pCtrlItf = tptr;
    dwCtrlSize = sizeof(uac_spk_ac_interface);
    tptr += sizeof(uac_spk_ac_interface);//0x09

    memcpy(tptr, (u8 *)uac_spk_input_terminal_desc, sizeof(uac_spk_input_terminal_desc));
    dwCtrlSize += sizeof(uac_spk_input_terminal_desc);
    tptr += sizeof(uac_spk_input_terminal_desc);//0x0c

    memcpy(tptr, (u8 *)uac_spk_feature_desc, sizeof(uac_spk_feature_desc));
    dwCtrlSize += sizeof(uac_spk_feature_desc);
    tptr += sizeof(uac_spk_feature_desc);//0x0a

    memcpy(tptr, (u8 *)uac_spk_selector_uint_desc, sizeof(uac_spk_selector_uint_desc));
    dwCtrlSize += sizeof(uac_spk_selector_uint_desc);
    tptr += sizeof(uac_spk_selector_uint_desc);

    memcpy(tptr, (u8 *)uac_spk_output_terminal_desc, sizeof(uac_spk_output_terminal_desc));
    dwCtrlSize += sizeof(uac_spk_output_terminal_desc);
    tptr += sizeof(uac_spk_output_terminal_desc);//0x09

    //wTotalLength of class-specific AC interface descriptors
    pCtrlItf[5] = LOBYTE(dwCtrlSize);
    pCtrlItf[6] = HIBYTE(dwCtrlSize);

    memcpy(tptr, (u8 *)uac_spk_as_interface_desc, sizeof(uac_spk_as_interface_desc));
    tptr[2] = *cur_itf_num + 1;
    tptr[9 + 2] = *cur_itf_num + 1;
    sr = spk_get_max_sampling_rate();
#if SPK_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * SPK_CHANNEL : 0);
#else
    frame_len = sr * (SPK_AUDIO_RES / 8) * SPK_CHANNEL / 1000;
    frame_len += ((sr % 1000) ? (SPK_AUDIO_RES / 8) * SPK_CHANNEL : 0);
#endif
    offset = 9 + 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 4;
    //MaxPacketSize of spk iso out
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#if SPK_AUDIO_RES_2
    offset = 9 + 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 9 + 7;
    tptr[offset + 2] = *cur_itf_num + 1;
#if SPK_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * SPK_CHANNEL : 0);
#else
    frame_len = sr * (SPK_AUDIO_RES_2 / 8) * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? (SPK_AUDIO_RES_2 / 8) * SPK_CHANNEL : 0);
#endif
    offset += 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 4;
    //MaxPacketSize of spk iso out
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#endif
    tptr += sizeof(uac_spk_as_interface_desc);

    if (usb_set_interface_hander(usb_id, *cur_itf_num, audio_ac_itf_handler) != *cur_itf_num) {
        ASSERT(0, "uac spk set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, spk_reset) != *cur_itf_num) {

    }
    (*cur_itf_num) ++;
    if (usb_set_interface_hander(usb_id, *cur_itf_num, spk_as_itf_hander) != *cur_itf_num) {
        ASSERT(0, "uac spk set interface_hander fail");
    }
    (*cur_itf_num) ++;

    usb_set_ep_setup_hander(usb_id, SPK_ISO_EP_OUT, uac_setup_endpoint);

    i = tptr - ptr;
    return i;
}
void mic_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_error("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, SPK_ISO_EP_OUT);
    /* uac_release(usb_id); */
#else
    if (mic_info) {
        u8 *ep_buffer = mic_info->mic_dma_buffer;
        usb_g_ep_config(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, mic_get_max_frame_len());
    }
#endif
}
u32 uac_mic_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    //cppcheck-suppress unreadVariable
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    int i = 0;
    u8 *tptr = ptr;
    u32 offset;
    u32 frame_len;
    u32 sr;
    u8 *pCtrlItf;
    u32 dwCtrlSize = 0;

    log_debug("micdc:%d\n", *cur_itf_num);

    memcpy(tptr, (u8 *)uac_audio_interface_association, sizeof(uac_audio_interface_association));
    tptr[2] = *cur_itf_num;
    tptr[3] = 2;
    tptr += sizeof(uac_audio_interface_association);//0x09

    memcpy(tptr, (u8 *)uac_ac_standard_interface_desc, sizeof(uac_ac_standard_interface_desc));
    tptr[2] = *cur_itf_num;
    tptr[8] = MIC_STR_INDEX;
    tptr += sizeof(uac_ac_standard_interface_desc);//0x09

    memcpy(tptr, uac_mic_ac_interface, sizeof(uac_mic_ac_interface));
    tptr[8] = *cur_itf_num + 1;
    pCtrlItf = tptr;
    dwCtrlSize = sizeof(uac_audio_ac_interface);
    tptr += sizeof(uac_mic_ac_interface);

    memcpy(tptr, (u8 *)uac_mic_input_terminal_desc, sizeof(uac_mic_input_terminal_desc));
    dwCtrlSize += sizeof(uac_mic_input_terminal_desc);
    tptr += sizeof(uac_mic_input_terminal_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_feature_desc, sizeof(uac_mic_feature_desc));
    dwCtrlSize += sizeof(uac_mic_feature_desc);
    tptr += sizeof(uac_mic_feature_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_selector_uint_desc, sizeof(uac_mic_selector_uint_desc));
    dwCtrlSize += sizeof(uac_mic_selector_uint_desc);
    tptr += sizeof(uac_mic_selector_uint_desc);

    memcpy(tptr, (u8 *)uac_mic_output_terminal_desc, sizeof(uac_mic_output_terminal_desc));
    dwCtrlSize += sizeof(uac_mic_output_terminal_desc);
    tptr += sizeof(uac_mic_output_terminal_desc);//0x09

    //wTotalLength of class-specific AC interface descriptors
    pCtrlItf[5] = LOBYTE(dwCtrlSize);
    pCtrlItf[6] = HIBYTE(dwCtrlSize);

    memcpy(tptr, (u8 *)uac_mic_as_interface_desc, sizeof(uac_mic_as_interface_desc));
    tptr[2] = *cur_itf_num + 1;
    tptr[9 + 2] = *cur_itf_num + 1;
    sr = mic_get_max_sampling_rate();
#if MIC_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * MIC_CHANNEL : 0);
#else
    frame_len = sr * (MIC_AUDIO_RES / 8) * MIC_CHANNEL / 1000;
    frame_len += ((sr % 1000) ? (MIC_AUDIO_RES / 8) * MIC_CHANNEL : 0);
#endif
    offset = 9 + 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 4;
    //MaxPacketSize of mic iso in
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#if MIC_AUDIO_RES_2
    offset = 9 + 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 9 + 7;
    tptr[offset + 2] = *cur_itf_num + 1;
#if MIC_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * MIC_CHANNEL : 0);
#else
    frame_len = sr * (MIC_AUDIO_RES_2 / 8) * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? (MIC_AUDIO_RES_2 / 8) * MIC_CHANNEL : 0);
#endif
    offset += 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 4;
    //MaxPacketSize of mic iso in
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#endif
    tptr += sizeof(uac_mic_as_interface_desc);//0x09

    if (usb_set_interface_hander(usb_id, *cur_itf_num, audio_ac_itf_handler) != *cur_itf_num) {
        ASSERT(0, "uac mic set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, mic_reset) != *cur_itf_num) {

    }
    (*cur_itf_num) ++;
    if (usb_set_interface_hander(usb_id, *cur_itf_num, mic_as_itf_hander) != *cur_itf_num) {
        ASSERT(0, "uac mic set interface_hander fail");
    }
    (*cur_itf_num) ++;

    usb_set_ep_setup_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, uac_setup_endpoint);

    i = tptr - ptr;
    return i;
}
void audio_reset(struct usb_device_t *usb_device, u32 ift_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, SPK_ISO_EP_OUT);
    /* uac_release(usb_id); */
#else
    u8 *ep_buffer;
    if (spk_info) {
        ep_buffer = spk_info->spk_dma_buffer;
        usb_g_ep_config(usb_id, SPK_ISO_EP_OUT | USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, spk_get_max_frame_len());
    }

    if (mic_info) {
        ep_buffer = mic_info->mic_dma_buffer;
        usb_g_ep_config(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, ep_buffer, mic_get_max_frame_len());
    }
#endif
}
u32 uac_audio_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    //cppcheck-suppress unreadVariable
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    int i = 0;
    u8 *tptr = ptr;
    u32 offset;
    u32 frame_len;
    u32 sr;
    u8 *pCtrlItf;
    u32 dwCtrlSize = 0;

    memcpy(tptr, (u8 *)uac_audio_interface_association, sizeof(uac_audio_interface_association));
    tptr[2] = *cur_itf_num;
    tptr[3] = 3;
    tptr += sizeof(uac_audio_interface_association);//0x09

    memcpy(tptr, (u8 *)uac_ac_standard_interface_desc, sizeof(uac_ac_standard_interface_desc));
    tptr[2] = *cur_itf_num;
    tptr += sizeof(uac_ac_standard_interface_desc);//0x09

    memcpy(tptr, (u8 *)uac_audio_ac_interface, sizeof(uac_audio_ac_interface));
    tptr[8] = *cur_itf_num + 1;
    tptr[9] = *cur_itf_num + 2;
    pCtrlItf = tptr;
    dwCtrlSize = sizeof(uac_audio_ac_interface);
    tptr += sizeof(uac_audio_ac_interface);//0x09

    memcpy(tptr, (u8 *)uac_spk_input_terminal_desc, sizeof(uac_spk_input_terminal_desc));
    dwCtrlSize += sizeof(uac_spk_input_terminal_desc);
    tptr += sizeof(uac_spk_input_terminal_desc);//0x09

    memcpy(tptr, (u8 *)uac_spk_feature_desc, sizeof(uac_spk_feature_desc));
    dwCtrlSize += sizeof(uac_spk_feature_desc);
    tptr += sizeof(uac_spk_feature_desc);//0x09

    memcpy(tptr, (u8 *)uac_spk_selector_uint_desc, sizeof(uac_spk_selector_uint_desc));
    dwCtrlSize += sizeof(uac_spk_selector_uint_desc);
    tptr += sizeof(uac_spk_selector_uint_desc);

    memcpy(tptr, (u8 *)uac_spk_output_terminal_desc, sizeof(uac_spk_output_terminal_desc));
    dwCtrlSize += sizeof(uac_spk_output_terminal_desc);
    tptr += sizeof(uac_spk_output_terminal_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_input_terminal_desc, sizeof(uac_mic_input_terminal_desc));
    dwCtrlSize += sizeof(uac_mic_input_terminal_desc);
    tptr += sizeof(uac_mic_input_terminal_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_feature_desc, sizeof(uac_mic_feature_desc));
    dwCtrlSize += sizeof(uac_mic_feature_desc);
    tptr += sizeof(uac_mic_feature_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_selector_uint_desc, sizeof(uac_mic_selector_uint_desc));
    dwCtrlSize += sizeof(uac_mic_selector_uint_desc);
    tptr += sizeof(uac_mic_selector_uint_desc);

    memcpy(tptr, (u8 *)uac_mic_output_terminal_desc, sizeof(uac_mic_output_terminal_desc));
    dwCtrlSize += sizeof(uac_mic_output_terminal_desc);
    tptr += sizeof(uac_mic_output_terminal_desc);//0x09

    //wTotalLength of class-specific AC interface descriptors
    pCtrlItf[5] = LOBYTE(dwCtrlSize);
    pCtrlItf[6] = HIBYTE(dwCtrlSize);

    memcpy(tptr, (u8 *)uac_spk_as_interface_desc, sizeof(uac_spk_as_interface_desc));
    tptr[2] = *cur_itf_num + 1;
    tptr[9 + 2] = *cur_itf_num + 1;
    sr = spk_get_max_sampling_rate();
#if SPK_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * SPK_CHANNEL : 0);
#else
    frame_len = sr * (SPK_AUDIO_RES / 8) * SPK_CHANNEL / 1000;
    frame_len += ((sr % 1000) ? (SPK_AUDIO_RES / 8) * SPK_CHANNEL : 0);
#endif
    offset = 9 + 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 4;
    //MaxPacketSize of spk iso out
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#if SPK_AUDIO_RES_2
    offset = 9 + 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 9 + 7;
    tptr[offset + 2] = *cur_itf_num + 1;
#if SPK_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * SPK_CHANNEL : 0);
#else
    frame_len = sr * (SPK_AUDIO_RES_2 / 8) * SPK_CHANNEL / 1000;
    frame_len += (sr % 1000 ? (SPK_AUDIO_RES_2 / 8) * SPK_CHANNEL : 0);
#endif
    offset += 9 + 7 + (8 + 3 * (SPK_AUDIO_RATE_NUM)) + 4;
    //MaxPacketSize of spk iso out
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#endif
    tptr += sizeof(uac_spk_as_interface_desc);//0x09

    memcpy(tptr, (u8 *)uac_mic_as_interface_desc, sizeof(uac_mic_as_interface_desc));
    tptr[2] = *cur_itf_num + 2;
    tptr[9 + 2] = *cur_itf_num + 2;
    sr = mic_get_max_sampling_rate();
#if MIC_AUDIO_RES == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * MIC_CHANNEL : 0);
#else
    frame_len = sr * (MIC_AUDIO_RES / 8) * MIC_CHANNEL / 1000;
    frame_len += ((sr % 1000) ? (MIC_AUDIO_RES / 8) * MIC_CHANNEL : 0);
#endif
    offset = 9 + 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 4;
    //MaxPacketSize of mic iso in
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#if MIC_AUDIO_RES_2
    offset = 9 + 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 9 + 7;
    tptr[offset + 2] = *cur_itf_num + 2;
#if MIC_AUDIO_RES_2 == 24 && UAC_24BIT_IN_4BYTE
    frame_len = sr * 4 * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? 4 * MIC_CHANNEL : 0);
#else
    frame_len = sr * (MIC_AUDIO_RES_2 / 8) * MIC_CHANNEL / 1000;
    frame_len += (sr % 1000 ? (MIC_AUDIO_RES_2 / 8) * MIC_CHANNEL : 0);
#endif
    offset += 9 + 7 + (8 + 3 * MIC_AUDIO_RATE_NUM) + 4;
    //MaxPacketSize of mic iso in
    tptr[offset] = LOBYTE(frame_len);
    tptr[offset + 1] = HIBYTE(frame_len);
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[offset + 2] = UAC_ISO_INTERVAL_FS;
    }
#endif
#endif
    tptr += sizeof(uac_mic_as_interface_desc);//0x09

    log_debug("audio control interface num:%d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, audio_ac_itf_handler) != *cur_itf_num) {
        ASSERT(0, "uac spk set interface_hander fail");
    }

    if (usb_set_reset_hander(usb_id, *cur_itf_num, audio_reset) != *cur_itf_num) {

    }
    (*cur_itf_num) ++;
    log_debug("speaker stream interface num:%d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, spk_as_itf_hander) != *cur_itf_num) {
        ASSERT(0, "uac spk set interface_hander fail");
    }
    (*cur_itf_num) ++;
    log_debug("mic stream interface num:%d\n", *cur_itf_num);
    if (usb_set_interface_hander(usb_id, *cur_itf_num, mic_as_itf_hander) != *cur_itf_num) {
        ASSERT(0, "uac mic set interface_hander fail");
    }
    (*cur_itf_num) ++;

    usb_set_ep_setup_hander(usb_id, SPK_ISO_EP_OUT, uac_setup_endpoint);
    usb_set_ep_setup_hander(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, uac_setup_endpoint);

    i = tptr - ptr;
    return i;
}
u32 uac_spk_register(const usb_dev usb_id)
{
    if (spk_info == NULL) {
#if USB_MALLOC_ENABLE
        spk_info = (struct spk_info_t *)zalloc(sizeof(struct spk_info_t));
        if (spk_info == NULL) {
            printf("uac_spk_register err\n");
            return -1;
        }
#else
        memset(&_spk_info, 0, sizeof(struct spk_info_t));
        spk_info = &_spk_info;
#endif
#if USB_EP_PROTECT
        u32 ep_reserved_len = 1024 - spk_get_max_frame_len();
#else
        u32 ep_reserved_len = 0;
#endif
        spk_info->spk_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, SPK_ISO_EP_OUT, spk_get_max_frame_len() + ep_reserved_len);
    }
    spk_info->spk_def_vol = vol_convert(100);
    spk_info->spk_left_vol = spk_info->spk_def_vol;
    spk_info->spk_right_vol = spk_info->spk_def_vol;
    spk_info->spk_max_vol = vol_convert(100);
    spk_info->spk_min_vol = vol_convert(0);
    spk_info->spk_vol_res = 0x30;
    spk_info->spk_mute = 0;
    spk_info->ps4_mode = 0;
    return 0;
}
void uac_spk_release(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (spk_info) {
        //卸载时同步关闭音频流
        spk_info->alternate_setting = -1;
        close_spk(usb_device);
        if (spk_info->spk_dma_buffer) {
            usb_free_ep_dmabuffer(usb_id, spk_info->spk_dma_buffer);
            spk_info->spk_dma_buffer = NULL;
        }
#if USB_MALLOC_ENABLE
        free(spk_info);
#endif
        spk_info = NULL;
    }
}
u32 uac_mic_register(const usb_dev usb_id)
{
    if (mic_info == NULL) {
#if USB_MALLOC_ENABLE
        mic_info = (struct mic_info_t *)zalloc(sizeof(struct mic_info_t));
        if (mic_info == NULL) {
            printf("uac_register err\n");
            return -1;
        }
#else
        memset(&_mic_info, 0, sizeof(struct mic_info_t));
        mic_info = &_mic_info;
#endif
        mic_info->mic_dma_buffer = usb_alloc_ep_dmabuffer(usb_id, MIC_ISO_EP_IN | USB_DIR_IN, mic_get_max_frame_len());
    }
    mic_info->mic_max_vol = vol_convert(100);
    mic_info->mic_min_vol = vol_convert(0);
    mic_info->mic_vol_res = 0x30;
    mic_info->mic_def_vol = vol_convert(13 / 14.0 * 100);
    mic_info->mic_vol = mic_info->mic_def_vol;
    mic_info->mic_mute = 0;
    mic_info->ps4_mode = 0;
    return 0;
}
void uac_mic_release(const usb_dev usb_id)
{
    struct usb_device_t *usb_device = usb_id2device(usb_id);

    if (mic_info) {
        //卸载时同步关闭音频流
        mic_info->alternate_setting = -1;
        close_mic(usb_device);
        if (mic_info->mic_dma_buffer) {
            usb_free_ep_dmabuffer(usb_id, mic_info->mic_dma_buffer);
            mic_info->mic_dma_buffer = NULL;
        }
#if USB_MALLOC_ENABLE
        free(mic_info);
#endif
        mic_info = NULL;
    }
}
#endif
