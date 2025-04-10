#ifndef HDMI_CEC_H
#define HDMI_CEC_H

#include "typedef.h"
typedef enum {
    CEC_LOGADDR_TV          = 0x00,
    CEC_LOGADDR_RECDEV1     = 0x01,
    CEC_LOGADDR_RECDEV2     = 0x02,
    CEC_LOGADDR_TUNER1      = 0x03,     // STB1 in Spev v1.3
    CEC_LOGADDR_PLAYBACK1   = 0x04,     // DVD1 in Spev v1.3
    CEC_LOGADDR_AUDSYS      = 0x05,
    CEC_LOGADDR_TUNER2      = 0x06,     // STB2 in Spec v1.3
    CEC_LOGADDR_TUNER3      = 0x07,     // STB3 in Spec v1.3
    CEC_LOGADDR_PLAYBACK2   = 0x08,     // DVD2 in Spec v1.3
    CEC_LOGADDR_RECDEV3     = 0x09,
    CEC_LOGADDR_TUNER4      = 0x0A,     // RES1 in Spec v1.3
    CEC_LOGADDR_PLAYBACK3   = 0x0B,     // RES2 in Spec v1.3
    CEC_LOGADDR_RES3        = 0x0C,
    CEC_LOGADDR_RES4        = 0x0D,
    CEC_LOGADDR_FREEUSE     = 0x0E,
    CEC_LOGADDR_UNREGORBC   = 0x0F

} CEC_LOG_ADDR_t;

typedef enum {                  // CEC Messages
    CECOP_FEATURE_ABORT             = 0x00,
    CECOP_IMAGE_VIEW_ON             = 0x04,
    CECOP_TUNER_STEP_INCREMENT      = 0x05,     // N/A
    CECOP_TUNER_STEP_DECREMENT      = 0x06,     // N/A
    CECOP_TUNER_DEVICE_STATUS       = 0x07,     // N/A
    CECOP_GIVE_TUNER_DEVICE_STATUS  = 0x08,     // N/A
    CECOP_RECORD_ON                 = 0x09,     // N/A
    CECOP_RECORD_STATUS             = 0x0A,     // N/A
    CECOP_RECORD_OFF                = 0x0B,     // N/A
    CECOP_TEXT_VIEW_ON              = 0x0D,
    CECOP_RECORD_TV_SCREEN          = 0x0F,     // N/A
    CECOP_GIVE_DECK_STATUS          = 0x1A,
    CECOP_DECK_STATUS               = 0x1B,
    CECOP_SET_MENU_LANGUAGE         = 0x32,
    CECOP_CLEAR_ANALOGUE_TIMER      = 0x33,     // Spec 1.3A
    CECOP_SET_ANALOGUE_TIMER        = 0x34,     // Spec 1.3A
    CECOP_TIMER_STATUS              = 0x35,     // Spec 1.3A
    CECOP_STANDBY                   = 0x36,
    CECOP_PLAY                      = 0x41,
    CECOP_DECK_CONTROL              = 0x42,
    CECOP_TIMER_CLEARED_STATUS      = 0x43,     // Spec 1.3A
    CECOP_USER_CONTROL_PRESSED      = 0x44,
    CECOP_USER_CONTROL_RELEASED     = 0x45,
    CECOP_GIVE_OSD_NAME             = 0x46,
    CECOP_SET_OSD_NAME              = 0x47,
    CECOP_SET_OSD_STRING            = 0x64,
    CECOP_SET_TIMER_PROGRAM_TITLE   = 0x67,     // Spec 1.3A
    CECOP_SYSTEM_AUDIO_MODE_REQUEST = 0x70,     // Spec 1.3A
    CECOP_GIVE_AUDIO_STATUS         = 0x71,     // Spec 1.3A
    CECOP_SET_SYSTEM_AUDIO_MODE     = 0x72,     // Spec 1.3A
    CECOP_REPORT_AUDIO_STATUS       = 0x7A,     // Spec 1.3A
    CECOP_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D, // Spec 1.3A
    CECOP_SYSTEM_AUDIO_MODE_STATUS  = 0x7E,     // Spec 1.3A
    CECOP_ROUTING_CHANGE            = 0x80,
    CECOP_ROUTING_INFORMATION       = 0x81,
    CECOP_ACTIVE_SOURCE             = 0x82,
    CECOP_GIVE_PHYSICAL_ADDRESS     = 0x83,
    CECOP_REPORT_PHYSICAL_ADDRESS   = 0x84,
    CECOP_REQUEST_ACTIVE_SOURCE     = 0x85,
    CECOP_SET_STREAM_PATH           = 0x86,
    CECOP_DEVICE_VENDOR_ID          = 0x87,
    CECOP_VENDOR_COMMAND            = 0x89,
    CECOP_VENDOR_REMOTE_BUTTON_DOWN = 0x8A,
    CECOP_VENDOR_REMOTE_BUTTON_UP   = 0x8B,
    CECOP_GIVE_DEVICE_VENDOR_ID     = 0x8C,
    CECOP_MENU_REQUEST              = 0x8D,
    CECOP_MENU_STATUS               = 0x8E,
    CECOP_GIVE_DEVICE_POWER_STATUS  = 0x8F,
    CECOP_REPORT_POWER_STATUS       = 0x90,
    CECOP_GET_MENU_LANGUAGE         = 0x91,
    CECOP_SELECT_ANALOGUE_SERVICE   = 0x92,     // Spec 1.3A    N/A
    CECOP_SELECT_DIGITAL_SERVICE    = 0x93,     //              N/A
    CECOP_SET_DIGITAL_TIMER         = 0x97,     // Spec 1.3A
    CECOP_CLEAR_DIGITAL_TIMER       = 0x99,     // Spec 1.3A
    CECOP_SET_AUDIO_RATE            = 0x9A,     // Spec 1.3A
    CECOP_INACTIVE_SOURCE           = 0x9D,     // Spec 1.3A
    CECOP_CEC_VERSION               = 0x9E,     // Spec 1.3A
    CECOP_GET_CEC_VERSION           = 0x9F,     // Spec 1.3A
    CECOP_VENDOR_COMMAND_WITH_ID    = 0xA0,     // Spec 1.3A
    CECOP_CLEAR_EXTERNAL_TIMER      = 0xA1,     // Spec 1.3A
    CECOP_SET_EXTERNAL_TIMER        = 0xA2,     // Spec 1.3A
    CDCOP_HEADER                    = 0xF8,
    CECOP_ABORT                     = 0xFF,

    CECOP_REPORT_SHORT_AUDIO_DESCRIPTOR    	= 0xA3,     // Spec 1.4
    CECOP_REQUEST_SHORT_AUDIO_DESCRIPTOR    	= 0xA4,     // Spec 1.4

    CECOP_ARC_INITIATE              = 0xC0,
    CECOP_ARC_REPORT_INITIATED      = 0xC1,
    CECOP_ARC_REPORT_TERMINATED     = 0xC2,

    CECOP_ARC_REQUEST_INITIATION    = 0xC3,
    CECOP_ARC_REQUEST_TERMINATION   = 0xC4,
    CECOP_ARC_TERMINATE             = 0xC5,

} CEC_OPCODE_t;

typedef enum cec_user_control_code {
    CEC_USER_CONTROL_CODE_SELECT                      = 0x00,
    CEC_USER_CONTROL_CODE_UP                          = 0x01,
    CEC_USER_CONTROL_CODE_DOWN                        = 0x02,
    CEC_USER_CONTROL_CODE_LEFT                        = 0x03,
    CEC_USER_CONTROL_CODE_RIGHT                       = 0x04,
    CEC_USER_CONTROL_CODE_RIGHT_UP                    = 0x05,
    CEC_USER_CONTROL_CODE_RIGHT_DOWN                  = 0x06,
    CEC_USER_CONTROL_CODE_LEFT_UP                     = 0x07,
    CEC_USER_CONTROL_CODE_LEFT_DOWN                   = 0x08,
    CEC_USER_CONTROL_CODE_ROOT_MENU                   = 0x09,
    CEC_USER_CONTROL_CODE_SETUP_MENU                  = 0x0A,
    CEC_USER_CONTROL_CODE_CONTENTS_MENU               = 0x0B,
    CEC_USER_CONTROL_CODE_FAVORITE_MENU               = 0x0C,
    CEC_USER_CONTROL_CODE_EXIT                        = 0x0D,
    // reserved: 0x0E, 0x0F
    CEC_USER_CONTROL_CODE_TOP_MENU                    = 0x10,
    CEC_USER_CONTROL_CODE_DVD_MENU                    = 0x11,
    // reserved: 0x12 ... 0x1C
    CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE           = 0x1D,
    CEC_USER_CONTROL_CODE_NUMBER11                    = 0x1E,
    CEC_USER_CONTROL_CODE_NUMBER12                    = 0x1F,
    CEC_USER_CONTROL_CODE_NUMBER0                     = 0x20,
    CEC_USER_CONTROL_CODE_NUMBER1                     = 0x21,
    CEC_USER_CONTROL_CODE_NUMBER2                     = 0x22,
    CEC_USER_CONTROL_CODE_NUMBER3                     = 0x23,
    CEC_USER_CONTROL_CODE_NUMBER4                     = 0x24,
    CEC_USER_CONTROL_CODE_NUMBER5                     = 0x25,
    CEC_USER_CONTROL_CODE_NUMBER6                     = 0x26,
    CEC_USER_CONTROL_CODE_NUMBER7                     = 0x27,
    CEC_USER_CONTROL_CODE_NUMBER8                     = 0x28,
    CEC_USER_CONTROL_CODE_NUMBER9                     = 0x29,
    CEC_USER_CONTROL_CODE_DOT                         = 0x2A,
    CEC_USER_CONTROL_CODE_ENTER                       = 0x2B,
    CEC_USER_CONTROL_CODE_CLEAR                       = 0x2C,
    CEC_USER_CONTROL_CODE_NEXT_FAVORITE               = 0x2F,
    CEC_USER_CONTROL_CODE_CHANNEL_UP                  = 0x30,
    CEC_USER_CONTROL_CODE_CHANNEL_DOWN                = 0x31,
    CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL            = 0x32,
    CEC_USER_CONTROL_CODE_SOUND_SELECT                = 0x33,
    CEC_USER_CONTROL_CODE_INPUT_SELECT                = 0x34,
    CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION         = 0x35,
    CEC_USER_CONTROL_CODE_HELP                        = 0x36,
    CEC_USER_CONTROL_CODE_PAGE_UP                     = 0x37,
    CEC_USER_CONTROL_CODE_PAGE_DOWN                   = 0x38,
    // reserved: 0x39 ... 0x3F
    CEC_USER_CONTROL_CODE_POWER                       = 0x40,
    CEC_USER_CONTROL_CODE_VOLUME_UP                   = 0x41,
    CEC_USER_CONTROL_CODE_VOLUME_DOWN                 = 0x42,
    CEC_USER_CONTROL_CODE_MUTE                        = 0x43,
    CEC_USER_CONTROL_CODE_PLAY                        = 0x44,
    CEC_USER_CONTROL_CODE_STOP                        = 0x45,
    CEC_USER_CONTROL_CODE_PAUSE                       = 0x46,
    CEC_USER_CONTROL_CODE_RECORD                      = 0x47,
    CEC_USER_CONTROL_CODE_REWIND                      = 0x48,
    CEC_USER_CONTROL_CODE_FAST_FORWARD                = 0x49,
    CEC_USER_CONTROL_CODE_EJECT                       = 0x4A,
    CEC_USER_CONTROL_CODE_FORWARD                     = 0x4B,
    CEC_USER_CONTROL_CODE_BACKWARD                    = 0x4C,
    CEC_USER_CONTROL_CODE_STOP_RECORD                 = 0x4D,
    CEC_USER_CONTROL_CODE_PAUSE_RECORD                = 0x4E,
    // reserved: 0x4F
    CEC_USER_CONTROL_CODE_ANGLE                       = 0x50,
    CEC_USER_CONTROL_CODE_SUB_PICTURE                 = 0x51,
    CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND             = 0x52,
    CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE    = 0x53,
    CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING           = 0x54,
    CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION       = 0x55,
    CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE       = 0x56,
    CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION   = 0x57,
    // reserved: 0x58 ... 0x5F
    CEC_USER_CONTROL_CODE_PLAY_FUNCTION               = 0x60,
    CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION         = 0x61,
    CEC_USER_CONTROL_CODE_RECORD_FUNCTION             = 0x62,
    CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION       = 0x63,
    CEC_USER_CONTROL_CODE_STOP_FUNCTION               = 0x64,
    CEC_USER_CONTROL_CODE_MUTE_FUNCTION               = 0x65,
    CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION     = 0x66,
    CEC_USER_CONTROL_CODE_TUNE_FUNCTION               = 0x67,
    CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION       = 0x68,
    CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION    = 0x69,
    CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION = 0x6A,
    CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION       = 0x6B,
    CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION          = 0x6C,
    CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION           = 0x6D,
    // reserved: 0x6E ... 0x70
    CEC_USER_CONTROL_CODE_F1_BLUE                     = 0x71,
    CEC_USER_CONTROL_CODE_F2_RED                      = 0X72,
    CEC_USER_CONTROL_CODE_F3_GREEN                    = 0x73,
    CEC_USER_CONTROL_CODE_F4_YELLOW                   = 0x74,
    CEC_USER_CONTROL_CODE_F5                          = 0x75,
    CEC_USER_CONTROL_CODE_DATA                        = 0x76,
    // reserved: 0x77 ... 0xFF
    CEC_USER_CONTROL_CODE_AN_RETURN                   = 0x91, // return (Samsung)
    CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST            = 0x96, // channels list (Samsung)
    CEC_USER_CONTROL_CODE_MAX                         = 0x96,
    CEC_USER_CONTROL_CODE_UNKNOWN                     = 0xFF
} CEC_USER_CONTROL_CODE_t;

#define     CEC_KEY_POWER               0x40
#define     CEC_KEY_VOLUME_UP           0X41
#define     CEC_KEY_VOLUME_DOWN         0X42
#define     CEC_KEY_MUTE                0X43


#endif
