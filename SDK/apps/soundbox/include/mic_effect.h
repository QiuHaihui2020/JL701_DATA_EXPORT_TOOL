#ifndef __AUDIO_MIC_EFFECT_H
#define __AUDIO_MIC_EFFECT_H


bool mic_effect_player_runing();
int mic_effect_player_open();
void mic_effect_player_close();
void mic_effect_player_pause(u8 mark);
void mic_effect_set_dvol(u8 vol);
void mic_effect_dvol_up(void);
void mic_effect_dvol_down(void);
void mic_effect_set_irq_point_unit(u16 point_unit);
void mic_effect_set_echo_delay(u32 delay);
u32 mic_effect_get_echo_delay(void);
void mic_effect_set_echo_decay(u32 decay);
u32 mic_effect_get_echo_decay(void);
#endif
