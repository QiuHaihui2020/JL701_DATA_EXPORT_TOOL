#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_music_func.data.bss")
#pragma data_seg(".rcsp_music_func.data")
#pragma const_seg(".rcsp_music_func.text.const")
#pragma code_seg(".rcsp_music_func.text")
#endif
#include "rcsp_music_func.h"
#include "rcsp_device_info_func_common.h"
#include "rcsp.h"
#include "rcsp_device_status.h"
#include "rcsp_config.h"
#include "key_event_deal.h"
#include "music/music_player.h"
#include "JL_rcsp_attr.h"
#include "JL_rcsp_api.h"

#if (RCSP_MODE && TCFG_APP_MUSIC_EN)

#pragma pack(1)
struct _MUSIC_STATUS_info {
    u8 status;
    u32 cur_time;
    u32 total_time;
    u8 cur_dev;
};
#pragma pack()

enum {
    MUSIC_FUNC_PP = 0x1,
    MUSIC_FUNC_PREV,
    MUSIC_FUNC_NEXT,
    MUSIC_FUNC_MODE,
    MUSIC_FUNC_EQ_MODE,
    MUSIC_FUNC_REWIND,
    MUSIC_FUNC_FAST_FORWORD,
};


enum {
    REPEAT_MODE_ALL = 0x1,
    REPEAT_MODE_DEV,
    REPEAT_MODE_ONE,
    REPEAT_MODE_RANDOM,
    REPEAT_MODE_FOLDER,
};

const u8 rcsp_repeat_mode_remap[] = {
    [FCYCLE_ALL] = REPEAT_MODE_ALL,
    [FCYCLE_ONE] = REPEAT_MODE_ONE,
    [FCYCLE_FOLDER] = REPEAT_MODE_FOLDER,
    [FCYCLE_RANDOM] = REPEAT_MODE_RANDOM,
};


#define RCSP_MUSIC_FILE_NAME_MAX_LIMIT			(128)

// 本文件内用到
static u8 music_func_add_one_attr_ex(u8 *buf, u16 max_len, u8 offset, u8 type, u8 *data, u8 size, u8 att_size)
{
    if (offset + size + 2 > max_len) {
        printf("add attr err\n");
        return 0;
    }
    buf[offset] = att_size + 1;
    buf[offset + 1] = type;
    memcpy(&buf[offset + 2], data, size);
    return size + 2;
}

// 本文件内用到
static u8 mucis_func_add_one_attr_continue(u8 *buf, u16 max_len, u8 offset, u8 type, u8 *data, u8 size)
{
    if ((offset + size) > max_len) {
        printf("add attr err 2\n");
        return 0;
    }
    memcpy(&buf[offset], data, size);
    return size;
}

//获取固件播放器信息
u32 rcsp_music_func_get(void *priv, u8 *buf, u16 buf_size, u32 mask)
{
    u16 offset = 0;
#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    u8 app = app_get_curr_task();
    if (app != APP_MUSIC_TASK) {
        return 0;
    }
    ///获取当前播放状态
    struct RcspModel *rcspModel = (struct RcspModel *) priv;
    extern struct __music music_hdl;
    FILE *file = music_hdl.player_hd->file;

    if (mask & BIT(MUSIC_INFO_ATTR_STATUS)) {
        /* printf("MUSIC_INFO_ATTR_STATUS\n"); */
        struct _MUSIC_STATUS_info music_info;

        music_info.status = music_file_get_player_status(get_music_file_player());
        music_info.cur_time = app_htonl(music_file_get_cur_time(get_music_file_player()));
        music_info.total_time = app_htonl(music_file_get_total_time(get_music_file_player()));
        char *logo = dev_manager_get_logo(dev_manager_find_active(1));
        char *tmp = NULL;
        if (logo) {
            for (int i = 0; i < RCSPDevMapMax; i++) {
                tmp = rcsp_browser_dev_remap(i);
                if (tmp && strcmp(tmp, logo) == 0) {
                    music_info.cur_dev = i;
                    offset += add_one_attr(buf, buf_size, offset, MUSIC_INFO_ATTR_STATUS, (u8 *)&music_info, sizeof(music_info));
                    break;
                }
            }
        }
    }

    if (mask & BIT(MUSIC_INFO_ATTR_FILE_NAME) && file) {
        /* printf("MUSIC_INFO_ATTR_FILE_NAME\n"); */
        u8 *lfn_buf = zalloc(512);
        if (lfn_buf) {
            int lfn_len = fget_name(file, lfn_buf, 512);
            lfn_len = rcsp_file_name_cut(lfn_buf, lfn_len, RCSP_MUSIC_FILE_NAME_MAX_LIMIT);
            struct vfs_attr tmp_attr = {0};
            fget_attrs(file, &tmp_attr);
            u32 clust = app_htonl(tmp_attr.sclust);
            /* u32 clust = tmp_attr.sclust; */
            /* printf("clust %x\n", clust); */
            u8 code_type = 1;
            u8 *tmp_buf = lfn_buf;
            if (lfn_buf[0] == '\\' && lfn_buf[1] == 'U') {
                code_type = 0;
                lfn_len -= 2;
                tmp_buf += 2;
            }

            offset += music_func_add_one_attr_ex(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, (u8 *)&clust, sizeof(u32), lfn_len + sizeof(code_type) + sizeof(clust));
            offset += mucis_func_add_one_attr_continue(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, (u8 *)&code_type, 1);
            offset += mucis_func_add_one_attr_continue(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_NAME, tmp_buf, lfn_len);
            free(lfn_buf);
        }
    }

    if (mask & BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE)) {
        /* printf("MUSIC_INFO_ATTR_FILE_PLAY_MODE\n"); */
        u8 play_mode = music_player_get_repeat_mode();
        /* printf("play_mode = %d\n", play_mode); */
        play_mode = rcsp_repeat_mode_remap[play_mode];
        /* printf("remap = %d\n", play_mode); */
        offset += add_one_attr(buf, buf_size, offset, MUSIC_INFO_ATTR_FILE_PLAY_MODE, &play_mode, 1);
    }
#endif
    return offset;
}

//设置固件播放器行为
bool rcsp_music_func_set(void *priv, u8 *data, u16 len)
{
    /* printf("%s, %d\n", __func__, data[0]); */
#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    switch (data[0]) {
    case MUSIC_FUNC_PP:
        app_send_message(APP_MSG_MUSIC_PP, 0);
        break;
    case MUSIC_FUNC_PREV:
        app_send_message(APP_MSG_MUSIC_PREV, 0);
        break;
    case MUSIC_FUNC_NEXT:
        app_send_message(APP_MSG_MUSIC_NEXT, 0);
        break;
    case MUSIC_FUNC_MODE:
        app_send_message(APP_MSG_MUSIC_CHANGE_REPEAT, 0);
        break;
    case MUSIC_FUNC_REWIND:
        /* printf("MUSIC_FUNC_REWIND = %d\n", (int)(data[1] << 8 | data[2])); */
        music_file_player_fr((int)(data[1] << 8 | data[2]), get_music_file_player());
        break;
    case MUSIC_FUNC_FAST_FORWORD:
        /* printf("MUSIC_FUNC_FAST_FORWORD = %d\n", (int)(data[1] << 8 | data[2])); */
        music_file_player_ff((int)(data[1] << 8 | data[2]), get_music_file_player());
        break;
    default:
        break;
    }

    rcsp_device_status_update(MUSIC_FUNCTION_MASK,
                              BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));
#endif
    return true;
}

//停止音乐功能
void rcsp_music_func_stop(void)
{
#if (RCSP_MSG_DISTRIBUTION_VER != RCSP_MSG_DISTRIBUTION_VER_VISUAL_CFG_TOOL)
    if (music_player_get_play_status() == FILE_DEC_STATUS_PLAY) {
        app_task_put_key_msg(KEY_MUSIC_PP, 0);
    }
#endif
}

#endif

