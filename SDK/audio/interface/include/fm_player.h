#ifndef FM_PLAYER_H
#define FM_PLAYER_H

#include "effect/effects_default_param.h"

int fm_player_open();

void fm_player_close();

bool fm_player_runing();

int fm_file_pitch_up();

int fm_file_pitch_down();

int fm_file_set_pitch(enum _pitch_level pitch_mode);

void fm_file_pitch_mode_init(enum _pitch_level pitch_mode);
#endif

