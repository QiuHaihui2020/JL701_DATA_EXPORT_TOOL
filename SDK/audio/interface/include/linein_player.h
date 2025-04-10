#ifndef LINEIN_PLAYER_H
#define LINEIN_PLAYER_H

#include "effect/effects_default_param.h"

int linein_player_open();

void linein_player_close();

bool linein_player_runing();

int linein_player_playing();

int linein_file_pitch_up();

int linein_file_pitch_down();

int linein_file_set_pitch(enum _pitch_level pitch_mode);

void linein_file_pitch_mode_init(enum _pitch_level pitch_mode);
#endif

