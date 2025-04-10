#ifndef __RCSP_DEVICE_INFO_CMD_COMMON_H__
#define __RCSP_DEVICE_INFO_CMD_COMMON_H__

//设置固件fm行为
bool rcsp_fm_func_set(void *priv, u8 *data, u16 len);
//获取固件fm信息
u32 rcsp_fm_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

//设置固件rtc行为
bool rcsp_rtc_func_set(void *priv, u8 *data, u16 len);
//获取固件rtc信息
u32 rcsp_rtc_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);
//rtc消息处理
void rcsp_rtc_msg_deal(int msg);

//获取固件播放器信息
u32 rcsp_music_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);
//设置固件播放器行为
bool rcsp_music_func_set(void *priv, u8 *data, u16 len);
//停止音乐功能
void rcsp_music_func_stop(void);

//设置固件bt行为
bool rcsp_bt_func_set(void *priv, u8 *data, u16 len);
//获取固件bt信息
u32 rcsp_bt_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);

//设置固件linein行为
bool rcsp_linein_func_set(void *priv, u8 *data, u16 len);
//获取固件linein信息
u32 rcsp_linein_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask);
//停止linein功能
void rcsp_linein_func_stop(void);
#endif
