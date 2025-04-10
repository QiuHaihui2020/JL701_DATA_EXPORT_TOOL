#ifndef __IIS_PLAYER_H
#define __IIS_PLAYER_H

int iis_player_open(void);
void iis_player_close(void);
bool iis_player_runing(void);

/* 在app_core线程打开 iis player */
int iis_open_player_by_taskq(void);

/* 在app_core线程关闭 iis player */
int iis_close_player_by_taskq(void);

//当固定为接收端时，其它模式下开广播切进music模式，关闭广播后music模式不会自动播放
extern void iis_set_broadcast_local_open_flag(u8 en);

#endif

