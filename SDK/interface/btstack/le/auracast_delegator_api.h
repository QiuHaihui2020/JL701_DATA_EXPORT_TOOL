#ifndef __AURACAST_DELEGATOR_API_H__
#define __AURACAST_DELEGATOR_API_H__

#include "typedef.h"


enum {
    DELEGATOR_SCAN_START_EVENT,
    DELEGATOR_SCAN_STOP_EVENT,
    DELEGATOR_DEVICE_ADD_EVENT,
    DELEGATOR_DEVICE_MODIFY_EVENT,
    DELEGATOR_DEVICE_KEY_ADD_EVENT,
};

enum {
    DELEGATOR_ATT_CHECK_NOTIFY,
    DELEGATOR_ATT_SEND_NOTIFY,
    DELEGATOR_ATT_PROFILE_START_NOTIFY,
    DELEGATOR_BASS_ADD_SOURCE_NOTIFY,
};

enum {
    DELEGATOR_NOT_SYNCHRONIZED_TO_PA = 0x00,
    DELEGATOR_SYNCINFO_REQUEST,
    DELEGATOR_SYNCHRONIZED_TO_PA,
    DELEGATOR_FAILED_TO_SYNCHRONIZE_TO_PA,
    DELEGATOR_NO_PAST,
    DELEGATOR_RFU
};

typedef struct {
    uint8_t source_addr[6];
    uint8_t bass_source_id;
    uint8_t broadcast_code[16];
} auracast_delegator_info_t;

typedef struct {
    uint16_t att_send_len;
    uint16_t con_handle;
    uint8_t encry;
    uint8_t bass_source_id;
    uint16_t big_len;
    uint8_t *big_data;
} auracast_delegator_notify_t;

/********************************
  auracast_delegator notify event
  0x01XX  // BASS
  0x01XX  BASS need broadcast code
  ....
  0x10XX  // JL_private
  ...
 *******************************/
extern int auracast_delegator_event_notify(uint16_t event, uint8_t *packet, uint16_t length);

typedef struct {
    u32 server_map;     // BASS、PACS、JL_private ......
    u8  adv_edr;
    char device_name[32];
    u8 device_name_len;
    u16 adv_interval;
} auracast_delegator_user_config_t;

extern void auracast_delegator_config(auracast_delegator_user_config_t *param);
extern int auracast_delegator_init(u8 addr_type);
extern int auracast_delegator_uninit();
extern int auracast_delegator_adv_enable(u8 en);

/******************************n
  auracast_delegator event
  0x00XX  // BLE event
  0x0001  BLE connection
  0x0002  BLE disconnection
  ...
  0x01XX  // BASS
  0x0101  BASS add sourse
  0x0102  BASS modify sourse
  ...
  0x02XX  // PACS
  ...
  0x10XX  // JL_private
  ...
 *******************************/
typedef void (*auracast_delegator_event_callback_t)(uint16_t event, uint8_t *packet, uint16_t length);
extern void auracast_delegator_event_callback_register(auracast_delegator_event_callback_t callback);

#endif /* __AURACAST_DELEGATOR_API_H__ */
