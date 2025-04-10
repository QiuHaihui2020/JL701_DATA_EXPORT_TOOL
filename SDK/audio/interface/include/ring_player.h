#ifndef __RING_PLAYER_H
#define __RING_PLAYER_H


int play_ring_file(const char *file_name);

int play_ring_file_alone(const char *file_name);

int play_ring_file_with_callback(const char *file_name, void *priv, tone_player_cb_t callback);

int play_ring_file_alone_with_callback(const char *file_name, void *priv, tone_player_cb_t callback);

bool ring_player_runing();

void ring_player_stop();

#endif

