#ifndef _APP_LE_AURACAST_H
#define _APP_LE_AURACAST_H

/*  Include header */
#include "typedef.h"
#include "wireless_trans.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/
#define SOURCE_MAX_BIS_MUMS  2
#define SINK_MAX_BIS_MUMS    1
#define MAX_BIS_NUMS   MAX(SOURCE_MAX_BIS_MUMS, SINK_MAX_BIS_MUMS)

/**************************************************************************************************
  Data Types
**************************************************************************************************/
enum {
    APP_AURACAST_STATUS_STOP,
    APP_AURACAST_STATUS_SUSPEND,
    APP_AURACAST_STATUS_BROADCAST,
    APP_AURACAST_STATUS_SCAN,
    APP_AURACAST_STATUS_SYNC,
};

enum {
    APP_AURACAST_AS_SOURCE = 1,
    APP_AURACAST_AS_SINK,
};

u8 get_auracast_role(void);
u8 get_auracast_status(void);
u8 get_auracast_app_mode_exit_flag(void);
int app_auracast_sink_open();
int app_auracast_sink_close(u8 status);
int app_auracast_source_open();
int app_auracast_source_close(u8 status);
int app_auracast_switch(void);
int update_app_auracast_deal_scene(int scene);
int app_auracast_deal(int scene);
void app_auracast_open_in_other_mode();
void app_auracast_close_in_other_mode();
int auracast_source_media_reset();

void app_auracast_init(void);
void app_auracast_uninit(void);

int app_auracast_open();

int app_auracast_sink_start_record(void);
int app_auracast_sink_stop_record(void);
int app_auracast_sink_switch_source_device(u8 switch_mode); //0:切换设备后不过滤设备；1：切换设备后过滤处理只连接记录的设备
void app_auracast_sink_discard_record(void);

u8 get_auracast_switch_onoff(void);
#ifdef __cplusplus
};
#endif

#endif
