#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".general_player.data.bss")
#pragma data_seg(".general_player.data")
#pragma const_seg(".general_player.text.const")
#pragma code_seg(".general_player.text")
#endif


#include "music/general_player.h"
#include "key_event_deal.h"
#include "app_config.h"
#include "app_main.h"
#include "vm.h"
#include "dev_manager.h"
#include "file_player.h"
#include "dev_status.h"


///播放参数，文件扫描时用，文件后缀等
static const char scan_parm[] = "-t"
#if (TCFG_DEC_MP3_ENABLE)
                                "MP1MP2MP3"
#endif
#if (TCFG_DEC_WMA_ENABLE)
                                "WMA"
#endif
#if ( TCFG_DEC_WAV_ENABLE || TCFG_DEC_DTS_ENABLE)
                                "WAVDTS"
#endif
#if (TCFG_DEC_FLAC_ENABLE)
                                "FLA"
#endif
#if (TCFG_DEC_APE_ENABLE)
                                "APE"
#endif
#if (TCFG_DEC_M4A_ENABLE)
                                "M4AAAC"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
                                "MP4"
#endif
#if (TCFG_DEC_AMR_ENABLE)
                                "AMR"
#endif
#if (TCFG_DEC_DECRYPT_ENABLE)
                                "SMP"
#endif
#if (TCFG_DEC_MIDI_ENABLE)
                                "MID"
#endif
                                " -sn -r"
                                ;

struct __general_player {
    struct __dev    *dev;
    struct vfscan   *fsn;
    FILE            *file;
    struct __scan_callback *scan_cb;
    u8  scandisk_break;
    struct music_player *player_hd;
};

static struct __general_player *general_player = NULL;
#define __this general_player

int general_player_init(struct __scan_callback *scan_cb)
{
    printf("the general_player init\n");
    __this = zalloc(sizeof(struct __general_player));
    if (__this == NULL) {
        return -1;
    } else {
        __this->player_hd = music_player_creat();
        __this->scan_cb = scan_cb;
        return 0;
    }
}

void general_player_exit()
{
    if (__this) {
        general_player_stop(1);
        music_player_stop(__this->player_hd, 1);
        music_player_destroy(__this->player_hd);
        __this->player_hd = NULL;
        free(__this);
        __this = NULL;
    }
}

void general_player_stop(u8 fsn_release)
{
    if (__this == NULL) {
        return ;
    }
    ///停止解码
    music_file_player_stop();
    if (__this->file) {
        fclose(__this->file);
        __this->file = NULL;
    }
    if (fsn_release && __this->fsn) {
        ///根据播放情景， 通过设定flag决定是否需要释放fscan， 释放后需要重新扫盘!!!
        fscan_release(__this->fsn);
        __this->fsn = NULL;
    }
}


//可以添加播放回调函数
static int general_player_decode_start(FILE *file)
{

    if (file) {
        ///get file short name
        u8 file_name[12 + 1] = {0}; //8.3+\0
        fget_name(file, file_name, sizeof(file_name));
        log_i("\n");
        log_i("file name: %s\n", file_name);
        log_i("\n");
    }

    struct file_player *file_player;

    file_player = music_file_play_callback(file, NULL, NULL, NULL);
    if (!file_player) {
        return GENERAL_PLAYER_ERR_DECODE_FAIL;
    }


    return GENERAL_PLAYER_SUCC;
}

static const char *general_player_get_phy_dev(int *len)
{
    if (__this) {
        char *logo = dev_manager_get_logo(__this->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                ///录音设备,切换到音乐设备播放
                if (len) {
                    *len =  strlen(logo) - strlen(str);
                }
            } else {
                if (len) {
                    *len =  strlen(logo);
                }
            }
            return logo;
        }
    }
    if (len) {
        *len =  0;
    }
    return NULL;
}



int general_player_scandisk_break(void)
{
    ///注意：
    ///需要break fsn的事件， 请在这里拦截,
    ///需要结合MUSIC_PLAYER_ERR_FSCAN错误，做相应的处理
    int msg[32] = {0};
    const char *logo = NULL;
    char *evt_logo = NULL;
    struct key_event *key_evt = NULL;
    struct bt_event *bt_evt = NULL;
    int msg_from, msg_argc = 0;
    int *msg_argv = NULL;
    if (__this->scandisk_break) {//设备上下线直接打断
        return 1;
    }
    int res = os_taskq_accept(8, msg);
    if (res != OS_TASKQ) {
        return 0;
    }
    msg_from = msg[0];
    switch (msg[0]) {
    case MSG_FROM_DEVICE:
        switch (msg[1]) {
        case DRIVER_EVENT_FROM_SD0:
        case DRIVER_EVENT_FROM_SD1:
        case DRIVER_EVENT_FROM_SD2:
            evt_logo = (char *)msg[3];
        case DEVICE_EVENT_FROM_USB_HOST:
            if (!strncmp((char *)msg[3], "udisk", 5)) {
                evt_logo = (char *)msg[3];
            }
            msg_argc = 12;
            msg_argv = msg + 1;
            ///设备上下线底层推出的设备逻辑盘符是跟跟音乐设备一致的（音乐/录音设备, 详细看接口注释）
            int str_len   = 0;
            logo = music_player_get_phy_dev(__this->player_hd, &str_len);
            ///响应设备插拔打断
            if (msg[2] == DEVICE_EVENT_OUT) {
                log_i("__func__ = %s logo=%s evt_logo=%s %d\n", __FUNCTION__, logo, evt_logo, str_len);
                if (logo && (0 == memcmp(logo, evt_logo, str_len))) {
                    ///相同的设备才响应
                    __this->scandisk_break = 1;
                }
            } else if (msg[2] == DEVICE_EVENT_IN) {
                ///响应新设备上线
                __this->scandisk_break = 1;
            }
            if (__this->scandisk_break == 0) {
                log_i("__func__ = %s DEVICE_EVENT_OUT TODO\n", __FUNCTION__);
                dev_status_event_filter(msg + 1);
                log_i("__func__ = %s DEVICE_EVENT_OUT OK\n", __FUNCTION__);
            }
            break;
        }
        break;
    case MSG_FROM_BT_STACK:
#if (TCFG_BT_BACKGROUND_ENABLE)
        bt_background_msg_forward_filter(msg);
#endif
        break;
    case MSG_FROM_APP:
        msg_argc = 12;  //按最大参数个数处理
        msg_argv = msg + 1;
        switch (msg[1]) {
        case APP_MSG_CHANGE_MODE:
        /* fall-through */
        case APP_MSG_GOTO_MODE:
        /* fall-through */
        case APP_MSG_GOTO_NEXT_MODE:
            //响应切换模式事件
            __this->scandisk_break = 1;
            break;
            //其他按键case 在这里增加
        }
        break;
    case MSG_FROM_KEY:
        int key_msg = app_key_event_remap(music_mode_key_table, msg + 1);
        msg_from = MSG_FROM_APP;
        msg[1] = key_msg;
        msg_argv = msg + 1;
        msg_argc = 12;  //按最大参数个数处理
        switch (key_msg) {
        case APP_MSG_CHANGE_MODE:
            //响应切换模式事件
            __this->scandisk_break = 1;
            break;
        }
        break;
    }
    if (__this->scandisk_break) {
        ///查询到需要打断的事件， 返回1， 并且重新推送一次该事件,跑主循环处理流程
        app_send_message_from(msg_from, msg_argc, msg_argv);
        printf("scandisk_break!!!!!!\n");
        return 1;
    } else {
        return 0;
    }
    return 0;
}



int general_play_by_sculst(char *logo, u32 sclust)
{
    int ret = 0;
    if (logo == NULL) {
        return GENERAL_PLAYER_ERR_PARM;
    }
    char *cur_logo = dev_manager_get_logo(__this->dev);
    if (cur_logo == NULL || strcmp(cur_logo, logo) != 0) {
        if (cur_logo != NULL) {
            general_player_stop(1);
        }
        __this->dev = dev_manager_find_spec(logo, 1);
        if (!__this->dev) {
            return GENERAL_PLAYER_ERR_DEV_NOFOUND;
        }
        __this->fsn = file_manager_scan_disk(__this->dev, NULL, scan_parm, 0, (struct __scan_callback *)__this->scan_cb);
        if (!__this->fsn) {
            return GENERAL_PLAYER_ERR_FSCAN;
        }
        __this->file = file_manager_select(__this->dev, __this->fsn, FSEL_BY_SCLUST, sclust, __this->scan_cb);
        if (!__this->file) {
            return GENERAL_PLAYER_ERR_FILE_NOFOUND;
        }
        ret = general_player_decode_start(__this->file);
        if (ret != GENERAL_PLAYER_SUCC) {
            return ret;
        }
    } else {
        general_player_stop(0);
        __this->file = file_manager_select(__this->dev, __this->fsn, FSEL_BY_SCLUST, sclust, __this->scan_cb);
        if (!__this->file) {
            return GENERAL_PLAYER_ERR_FILE_NOFOUND;
        }
        ret = general_player_decode_start(__this->file);
        if (ret != GENERAL_PLAYER_SUCC) {
            return ret;
        }
    }
    return GENERAL_PLAYER_ERR_NULL;
}

void general_player_test()
{
    int tmp = general_player_init(NULL);
    if (!tmp) {
        printf("the init succ\n");
        general_play_by_sculst((char *)"sd0", 6);
    } else {
        printf("the init fail\n");
    }
}
