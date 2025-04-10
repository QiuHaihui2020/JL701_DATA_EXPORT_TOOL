#ifndef A2DP_PLAYER_H
#define A2DP_PLAYER_H

#include "effect/effects_default_param.h"

int a2dp_player_open(u8 *btaddr);

int a2dp_player_open_for_jl_dongle(u8 *btaddr, u16 latency_msec, u8 dynamic_latency_en);

void a2dp_player_close(u8 *btaddr);

int a2dp_player_get_btaddr(u8 *btaddr);

int a2dp_player_runing();

bool a2dp_player_is_playing(u8 *bt_addr);

int a2dp_player_start_slience_detect(u8 *btaddr, void (*handler)(u8 *, bool), int msec);

void a2dp_player_tws_event_handler(int *msg);

void a2dp_play_close(u8 *bt_addr);

void a2dp_player_low_latency_enable(u8 enable);

extern void a2dp_file_low_latency_enable(u8 enable);

int a2dp_file_pitch_up();

int a2dp_file_pitch_down();

int a2dp_file_set_pitch(enum _pitch_level pitch_mode);

void a2dp_file_pitch_mode_init(enum _pitch_level pitch_mode);

void a2dp_player_reset(void);

void a2dp_player_breaker_mode(u8 mode,
                              u16 uuid_a, const char *name_a,
                              u16 uuid_b, const char *name_b);

u8 a2dp_file_get_low_latency_status(void);

u8 *get_g_play_addr(void);

void app_set_a2dp_play_status(u8 st);

#endif
