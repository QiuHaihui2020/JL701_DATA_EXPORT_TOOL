#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_bt_func.data.bss")
#pragma data_seg(".rcsp_bt_func.data")
#pragma const_seg(".rcsp_bt_func.text.const")
#pragma code_seg(".rcsp_bt_func.text")
#endif
#include "rcsp_bt_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "rcsp_event.h"
#include "app_action.h"

#include "ble_rcsp_server.h"
#include "rcsp_music_info_setting.h"
#include "JL_rcsp_protocol.h"
#include "JL_rcsp_attr.h"

#if (RCSP_MODE)
// 后续需要全部从JL_rcsp_protocol.h中拷贝出来
#define BT_INFO_ATTR_MUSIC_TITLE      	(0)
#define BT_INFO_ATTR_MUSIC_ARTIST      	(1)
#define BT_INFO_ATTR_MUSIC_ALBUM      	(2)
#define BT_INFO_ATTR_MUSIC_NUMBER      	(3)
#define BT_INFO_ATTR_MUSIC_TOTAL      	(4)
#define BT_INFO_ATTR_MUSIC_GENRE      	(5)
#define BT_INFO_ATTR_MUSIC_TIME      	(6)
#define BT_INFO_ATTR_MUSIC_STATE      	(7)
#define BT_INFO_ATTR_MUSIC_CURR_TIME    (8)

//设置固件bt行为
bool rcsp_bt_func_set(void *priv, u8 *data, u16 len)
{
#if RCSP_ADV_MUSIC_INFO_ENABLE
    music_info_cmd_handle(data, len);
#endif
    return true;
}

//获取固件bt信息
u32 rcsp_bt_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
#if RCSP_ADV_MUSIC_INFO_ENABLE
    u8 player_time[4];

    if (mask & BIT(BT_INFO_ATTR_MUSIC_TITLE)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_TITLE, (u8 *)get_music_title(), get_music_title_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_ARTIST)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_ARTIST, (u8 *)get_music_artist(), get_music_artist_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_ALBUM)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_ALBUM, (u8 *)get_music_album(), get_music_album_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_NUMBER)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_NUMBER, (u8 *)get_music_number(), get_music_number_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_TOTAL)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_TOTAL, (u8 *)get_music_total(),  get_music_total_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_GENRE)) {
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_GENRE, (u8 *)get_music_genre(), get_music_genre_len());
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_TIME)) {
        u16 music_sec = get_music_total_time();
        player_time[0] = music_sec >> 8;
        player_time[1] = music_sec;
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_TIME, player_time, 2);
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_STATE)) {
        //printf("\n music state\n");
        u8 music_state = get_music_player_state();
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_STATE, &music_state, 1);
    }
    if (mask & BIT(BT_INFO_ATTR_MUSIC_CURR_TIME)) {
        //printf("\nmusic curr time\n");
        u32 curr_music_sec = get_music_curr_time();
        player_time[0] = curr_music_sec >> 24;
        player_time[1] = curr_music_sec >> 16;
        player_time[2] = curr_music_sec >> 8;
        player_time[3] = curr_music_sec;
        offset += add_one_attr(buf, buf_size, offset, BT_INFO_ATTR_MUSIC_CURR_TIME, player_time, 4);
    }
#endif

    return offset;
}

#endif
