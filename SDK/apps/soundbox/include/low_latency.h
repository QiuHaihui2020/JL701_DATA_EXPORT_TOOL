#ifndef LOW_LATENCY_H
#define LOW_LATENCY_H

void bt_set_low_latency_mode(int enable, u8 tone_play_enable, int delay_ms);
int bt_get_low_latency_mode(void);
void bt_tws_sync_low_latency_mode(void);

#endif

