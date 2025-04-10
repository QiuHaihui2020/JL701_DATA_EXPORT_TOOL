#ifndef ESCO_PLAYER_H
#define ESCO_PLAYER_H



int esco_player_open(u8 *bt_addr);

void esco_player_close();

bool esco_player_runing();

int esco_player_get_btaddr(u8 *btaddr);

int esco_player_is_playing(u8 *btaddr);
int esco_player_start(u8 *bt_addr);
int esco_player_suspend(u8 *bt_addr);





#endif
