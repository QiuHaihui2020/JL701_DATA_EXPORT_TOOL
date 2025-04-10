#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".music_app_msg_handler.data.bss")
#pragma data_seg(".music_app_msg_handler.data")
#pragma const_seg(".music_app_msg_handler.text.const")
#pragma code_seg(".music_app_msg_handler.text")
#endif
#include "key_driver.h"
#include "app_main.h"
#include "init.h"
#include "dev_manager.h"
#include "music/music_player.h"
#include "music/breakpoint.h"
#include "app_music.h"
#include "scene_switch.h"
#include "node_param_update.h"
#include "clock_manager/clock_manager.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "local_tws.h"
#include "wireless_trans.h"
#include "le_audio_recorder.h"
#include "le_broadcast.h"
#include "app_le_broadcast.h"
#include "le_connected.h"
#include "app_le_connected.h"
#include "le_audio_stream.h"
#include "le_audio_player.h"
#include "app_le_auracast.h"
#include "audio_config.h"
#include "audio_config_def.h"
#include "rcsp_device_status.h"
#include "rcsp_music_func.h"
#include "rcsp_config.h"

extern struct __music music_hdl;

static u8 g_le_audio_flag;

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取音乐模式广播音频流开启标志
 *
 * @return 广播音频流开启标志
 */
/* ----------------------------------------------------------------------------*/
u8 get_music_le_audio_flag(void)
{
    return g_le_audio_flag;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 模式解码错误处理
   @param    err:错误码，详细错误码描述请看MUSIC_PLAYER错误码表枚举
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_err_deal(int err)
{
    u16 msg = APP_MSG_NULL;
    char *logo = NULL;
    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_ERR_DECODE_FAIL) {
        music_hdl.file_err_counter = 0;///清除错误文件累计
    }

    if (err != MUSIC_PLAYER_ERR_NULL && err != MUSIC_PLAYER_SUCC) {
        log_e("music player err = %d\n", err);
    }

    switch (err) {
    case MUSIC_PLAYER_SUCC:
        le_audio_scene_deal(LE_AUDIO_MUSIC_START);
        music_hdl.file_err_counter = 0;
        break;
    case MUSIC_PLAYER_ERR_NULL:
        break;
    case MUSIC_PLAYER_ERR_POINT:
    /* fall-through */
    case MUSIC_PLAYER_ERR_NO_RAM:
        msg = APP_MSG_GOTO_NEXT_MODE;//退出音乐模式
        break;
    case MUSIC_PLAYER_ERR_DECODE_FAIL:
        if (music_hdl.file_err_counter >= music_hdl.player_hd->fsn->file_number) {
            music_hdl.file_err_counter = 0;
            dev_manager_set_valid_by_logo(dev_manager_get_logo(music_hdl.player_hd->dev), 0);///将设备设置为无效设备
            if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
                msg = APP_MSG_GOTO_NEXT_MODE;//没有设备了，退出音乐模式
            } else {
                msg = APP_MSG_MUSIC_AUTO_NEXT_DEV;///所有文件都是错误的， 切换到下一个设备
            }
        } else {
            music_hdl.file_err_counter ++;
            if (music_hdl.file_play_direct == 0) {
                msg = APP_MSG_MUSIC_NEXT;//播放下一曲
            } else {
                msg = APP_MSG_MUSIC_PREV;//播放上一曲
            }
        }
        break;
    case MUSIC_PLAYER_ERR_DEV_NOFOUND:
        log_e("MUSIC_PLAYER_ERR_DEV_NOFOUND \n");
        if (dev_manager_get_total(1) == 0) {//参数为1 ：获取所有有效设备  参数0：获取所有设备
            msg = APP_MSG_GOTO_NEXT_MODE;///没有设备在线， 退出音乐模式
        } else {
            msg = APP_MSG_MUSIC_PLAY_FIRST;///没有找到指定设备， 播放之前的活动设备
        }
        break;

    case MUSIC_PLAYER_ERR_FSCAN:
        ///需要结合music_player_scandisk_break中处理的标志位处理
        if (music_hdl.scandisk_break) {
            music_hdl.scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_DEV_READ:
        log_e("MUSIC_PLAYER_ERR_DEV_READ \n");
    /* fall-through */
    case MUSIC_PLAYER_ERR_DEV_OFFLINE:
        log_e("MUSIC_PLAYER_ERR_DEV_OFFLINE \n");
        logo = dev_manager_get_logo(music_hdl.player_hd->dev);
        if (dev_manager_online_check_by_logo(logo, 1)) {
            ///如果错误失败在线， 并且是播放过程中产生的，先记录下断点
            if (music_player_get_playing_breakpoint(music_hdl.player_hd, music_hdl.breakpoint, 1) == true) {
                music_player_stop(music_hdl.player_hd, 0);//先停止，防止下一步操作VM卡顿
                breakpoint_vm_write(music_hdl.breakpoint, logo);
            }
            if (err == MUSIC_PLAYER_ERR_FSCAN) {
                dev_manager_set_valid_by_logo(logo, 0);///将设备设置为无效设备
            } else {
                //针对读错误， 因为时间推到应用层有延时导致下一个模式判断不正常， 此处需要将设备卸载
                dev_manager_unmount(logo);
            }
        }
        if (dev_manager_get_total(1) == 0) {
            /* app_status_handler(APP_STATUS_MUSIC_QUIT); */
            msg = APP_MSG_GOTO_NEXT_MODE;///没有设备在线， 退出音乐模式
        } else {
            msg = APP_MSG_MUSIC_AUTO_NEXT_DEV;///切换设备
        }
        break;
    case MUSIC_PLAYER_ERR_FILE_NOFOUND:
        ///查找文件有扫盘的可能，也需要结合music_player_scandisk_break中处理的标志位处理
        if (music_hdl.scandisk_break) {
            music_hdl.scandisk_break = 0;
            ///此处不做任何处理， 打断的事件已经重发， 由重发事件执行后续处理
            break;
        }
    case MUSIC_PLAYER_ERR_PARM:
        logo = dev_manager_get_logo(music_hdl.player_hd->dev);
        if (dev_manager_online_check_by_logo(logo, 1)) {
            if (music_hdl.player_hd->fsn->file_number) {
                msg = APP_MSG_MUSIC_PLAY_FIRST;///有文件,播放第一个文件
                break;
            }
        }

        if (dev_manager_get_total(1) == 0) {
            msg = APP_MSG_GOTO_NEXT_MODE;//没有设备了，退出音乐模式
        } else {
            msg = APP_MSG_MUSIC_AUTO_NEXT_DEV;
        }
        break;
    case MUSIC_PLAYER_ERR_FILE_READ://文件读错误
        msg = APP_MSG_MUSIC_NEXT;//播放下一曲
        break;
    }
    if (msg != APP_MSG_NULL) {
        app_send_message(msg, 0);
    }
}


int music_app_msg_handler(int *msg)
{
    bool music_pp_flag = 0;
    int err = MUSIC_PLAYER_ERR_NULL;
    char *logo = NULL;
    if (false == app_in_mode(APP_MODE_MUSIC)) {
        return 0;
    }

    printf("music_app_msg type:0x%x", msg[0]);
    u8 msg_type = msg[0];
#if  LEA_BIG_CTRLER_RX_EN && (LEA_BIG_FIX_ROLE==2) && !TCFG_KBOX_1T3_MODE_EN
    if (get_broadcast_connect_status() &&
        (msg_type == APP_MSG_MUSIC_PP
         || msg_type == APP_MSG_MUSIC_NEXT || msg_type == APP_MSG_MUSIC_PREV
#if LEA_BIG_VOL_SYNC_EN
         || msg_type == APP_MSG_VOL_UP || msg_type == APP_MSG_VOL_DOWN
#endif
         || msg_type == APP_MSG_MUSIC_MOUNT_PLAY_START || msg_type == APP_MSG_MUSIC_PLAY_START
         || msg_type == APP_MSG_MUSIC_PLAY_START_BY_SCLUST || msg_type == APP_MSG_MUSIC_PLAY_START_BY_DEV  //只屏蔽主动开启音乐播放的事件
         || msg_type == APP_MSG_MUSIC_CHANGE_DEV
        )) {

        printf("BIS receiving state does not support the event %d", msg_type);

        return 0;

    }
#endif


    u8 auto_next_dev;
    struct file_player *file_player = get_music_file_player();
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_MUSIC_PP:
        printf("app msg music pp\n");
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
#if (LEA_BIG_FIX_ROLE==2) && !TCFG_KBOX_1T3_MODE_EN
        //固定为接收端
        u8 music_volume_mute_mark = app_audio_get_mute_state(APP_AUDIO_STATE_MUSIC);
        if (get_le_audio_curr_role() == 2) {
            //接收端已连上
            music_volume_mute_mark ^= 1;
            audio_app_mute_en(music_volume_mute_mark);
            break;
        } else {
            if (music_volume_mute_mark == 1) {
                //没有连接情况下，如果之前是mute住了，那么先解mute
                music_volume_mute_mark ^= 1;
                audio_app_mute_en(music_volume_mute_mark);
                break;
            }
            if (file_player && file_player->stream) {
                if (file_player->status == FILE_PLAYER_START) {
                    if (music_file_player_pp(file_player) == 0) {
                        if (music_hdl.close_broadcast_need_resume_local_music_flag) {
                            music_hdl.close_broadcast_need_resume_local_music_flag = 0;
                        }
                        music_pp_flag = 1;
                    }
                    if (le_audio_scene_deal(LE_AUDIO_MUSIC_STOP) > 0) {
                        app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_PAUSE);
                        break;
                    }
                } else {
                    if (music_file_player_pp(file_player) == 0) {
                        music_pp_flag = 1;
                    }
                    if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                        app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                        break;
                    }
                }
            }
        }
#elif (LEA_BIG_FIX_ROLE==1)
        if (file_player && file_player->stream) {
            if (file_player->status == FILE_PLAYER_START) {
                music_hdl.music_local_audio_resume_onoff = 0;
                if (music_file_player_pp(file_player) == 0) {
                    music_pp_flag = 1;
                }
                if (le_audio_scene_deal(LE_AUDIO_MUSIC_STOP) > 0) {
                    app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_PAUSE);
                    break;
                }
            }
        } else {
            music_hdl.music_local_audio_resume_onoff = 1;
            if (music_file_player_pp(file_player) == 0) {
                music_pp_flag = 1;
            }
            if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                break;
            }
        }
#else	//FIX ROLE = 0, 不固定角色
        if (file_player && file_player->stream) {
            if (file_player->status == FILE_PLAYER_START) {
                if (music_file_player_pp(file_player) == 0) {
                    music_pp_flag = 1;
                }
                if (le_audio_scene_deal(LE_AUDIO_MUSIC_STOP) > 0) {
                    app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_PAUSE);
                    break;
                }
            } else {
                if (music_file_player_pp(file_player) == 0) {
                    music_pp_flag = 1;
                }
                if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                    app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                    break;
                }
            }
        }
#endif
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
        if (((app_get_connected_role() == APP_CONNECTED_ROLE_TRANSMITTER) ||
             (app_get_connected_role() == APP_CONNECTED_ROLE_DUPLEX)) &&
            file_player && file_player->stream) {
            if (file_player->status == FILE_PLAYER_START) {
                if (music_file_player_pp(file_player) == 0) {
                    music_pp_flag = 1;
                }
                if (le_audio_scene_deal(LE_AUDIO_MUSIC_STOP) > 0) {
                    app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_PAUSE);
                    break;
                }
            } else {
                if (music_file_player_pp(file_player) == 0) {
                    music_pp_flag = 1;
                }
                if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                    app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                    break;
                }
            }
        }
#endif

        if (music_file_get_player_status(file_player) == FILE_PLAYER_STOP) {
            app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
            app_send_message(APP_MSG_MUSIC_PLAY_START, 0);
        } else {
            if (!music_pp_flag) {
                music_file_player_pp(file_player);
            }
            app_send_message(APP_MSG_MUSIC_PLAY_STATUS, music_file_get_player_status(file_player));
        }
        break;
    case APP_MSG_MUSIC_SPEED_UP:
        printf("app msg music speed up\n");
        music_hdl.speed_mode = music_file_speed_up(file_player);
        clock_refurbish();
        break;
    case APP_MSG_MUSIC_SPEED_DOWN:
        printf("app msg music speed down\n");
        music_hdl.speed_mode = music_file_speed_down(file_player);
        clock_refurbish();
        break;
    case APP_MSG_PITCH_UP:
        printf("app msg music pitch up\n");
        music_hdl.pitch_mode = music_file_pitch_up(file_player);
        break;
    case APP_MSG_PITCH_DOWN:
        printf("app msg music pitch down\n");
        music_hdl.pitch_mode = music_file_pitch_down(file_player);
        break;
    case APP_MSG_MUSIC_CHANGE_REPEAT:
        music_player_change_repeat_mode(music_hdl.player_hd);
        app_send_message(APP_MSG_REPEAT_MODE_CHANGED, music_player_get_repeat_mode());
        break;
    case APP_MSG_MUSIC_PLAYER_AB_REPEAT_SWITCH:
        printf("app msg music ab repeat switch\n");
#if FILE_DEC_AB_REPEAT_EN
        music_file_ab_repeat_switch(file_player);
#endif
        break;
    case APP_MSG_MUSIC_PLAY_FIRST:
        printf("music_player_play_first_file\n");
        err = music_player_play_first_file(music_hdl.player_hd, NULL);
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("app msg music next\n");

        mem_stats();
        /* app_status_handler(APP_STATUS_MUSIC_FFR); */
        music_hdl.file_play_direct = 0;
        err = music_player_play_next(music_hdl.player_hd);
        break;
    case APP_MSG_MUSIC_PREV:
        printf("app msg music prev\n");
        mem_stats();
        /* app_status_handler(APP_STATUS_MUSIC_FFR); */
        music_hdl.file_play_direct = 1;
        err = music_player_play_prev(music_hdl.player_hd);
        break;

    case APP_MSG_MUSIC_PLAYE_PREV_FOLDER:
        err = music_player_play_folder_prev(music_hdl.player_hd);
        break;
    case APP_MSG_MUSIC_PLAYE_NEXT_FOLDER:
        err = music_player_play_folder_next(music_hdl.player_hd);
        break;
    case APP_MSG_MUSIC_AUTO_NEXT_DEV:
    /* fall-through */
    case APP_MSG_MUSIC_CHANGE_DEV:
        log_i("KEY_MUSIC_CHANGE_DEV\n");
        auto_next_dev = ((msg[0] == APP_MSG_MUSIC_AUTO_NEXT_DEV) ? 1 : 0);
        logo = music_player_get_dev_next(music_hdl.player_hd, auto_next_dev);
        if (logo == NULL) { ///找不到下一个设备，不响应设备切换
            break;
        }
        printf("next dev = %s\n", logo);
        ///切换设备前先保存一下上一个设备的断点信息,包括文件和解码信息
        if (music_player_get_playing_breakpoint(music_hdl.player_hd, music_hdl.breakpoint, 1) == true) {
            music_player_stop(music_hdl.player_hd, 0); //先停止，防止下一步操作VM卡顿
            breakpoint_vm_write(music_hdl.breakpoint, dev_manager_get_logo(music_hdl.player_hd->dev));
        }
#if (TCFG_MUSIC_DEVICE_TONE_EN)
#if TCFG_LOCAL_TWS_ENABLE
        const char *file_name = get_music_tone_name_by_logo(logo);
        if (file_name != NULL) {
            if (local_tws_enter_mode(file_name, (void *)logo) == 0) {
                break;
            } else {
                if (music_device_tone_play(logo) == true) {
                    break;
                }
            }
        }
#else
        if (music_device_tone_play(logo) == true) {
            break;
        }
#endif
#endif
        if (true == breakpoint_vm_read(music_hdl.breakpoint, logo)) {
            err = music_player_play_by_breakpoint(music_hdl.player_hd, logo, music_hdl.breakpoint);
        } else {
            err = music_player_play_first_file(music_hdl.player_hd, logo);
        }

        le_audio_scene_deal(LE_AUDIO_MUSIC_START);

        break;

#if TCFG_MIX_RECORD_ENABLE
    //只有使能录音宏，才会出现删文件的操作，此时可能会出现删了最后一个文件的情况
    case  APP_MSG_MUSIC_DELETE_CUR_FILE:
        // 删除当前文件
        int ret = music_player_delete_playing_file(music_hdl.player_hd);
        if (ret != MUSIC_PLAYER_SUCC) {
            //播放失败，此时可能删除了最后一个可播放的文件，处理是：切换模式
            r_printf("[Music Err]: case APP_MSG_MUSIC_DELETE_CUR_FILE, ret: %d, will switch mode!\n", ret);
            app_send_message(APP_MSG_CHANGE_MODE, 0);
        }
        break;
#endif

    case  APP_MSG_MUSIC_PLAY_REC_FOLDER_SWITCH:
        log_i("KEY_MUSIC_PLAYE_REC_FOLDER_SWITCH\n");
        // 切换到录音文件夹里播录音
        char *_logo = dev_manager_get_logo(music_hdl.player_hd->dev);
        char folder_path[32] = {};
        sprintf(folder_path, "%s%s%s", "/", TCFG_REC_FOLDER_NAME, "/");
        if (_logo == NULL) {
            r_printf("Err: logo==NULL, %d\n", __LINE__);
            err = MUSIC_PLAYER_ERR_RECORD_DEV;
        } else {
            printf(">>> logo: %s, folder_path: %s\n", _logo, folder_path);		//udisk0
            err = music_player_play_by_path(music_hdl.player_hd, _logo, "/JL_REC/");
        }

        le_audio_scene_deal(LE_AUDIO_MUSIC_START);

        break;
    case APP_MSG_MUSIC_PLAY_START_BY_DEV:
#if (TCFG_MUSIC_DEVICE_TONE_EN)
        logo = (char *)msg[1];
        log_i("KEY_MUSIC_DEVICE_TONE_END %s\n", logo);

        if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }

        if (logo) {
            if (true == breakpoint_vm_read(music_hdl.breakpoint, logo)) {
                err = music_player_play_by_breakpoint(music_hdl.player_hd, logo, music_hdl.breakpoint);
            } else {
                err = music_player_play_first_file(music_hdl.player_hd, logo);
            }
        }
        break;
#endif
    case APP_MSG_MUSIC_PLAY_SUCCESS:
        log_i("APP_MSG_MUSIC_PLAY_SUCCESS !!\n");
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
            app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
            break;
        }
#endif
        break;
    case APP_MSG_MUSIC_MOUNT_PLAY_START:
        logo = (char *)msg[1];
        log_i("APP_MSG_MUSIC_MOUNT_PLAY_START %s\n", logo);
        dev_manager_set_active_by_logo(logo);
    /* fall-through */
    case APP_MSG_MUSIC_PLAY_START:
        log_i("APP_MSG_MUSIC_PLAY_START !!\n");
        /* app_status_handler(APP_STATUS_MUSIC_PLAY); */
        ///断点播放活动设备
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_runing()) {
            if (dev_manager_get_logo(music_hdl.player_hd->dev) && logo) {///播放的设备跟当前活动的设备是同一个设备，不处理
                if (0 == strcmp(logo, dev_manager_get_logo(music_hdl.player_hd->dev))) {
                    log_w("the same dev!!\n");
                    break;
                }
            }
        }
#if (TCFG_MUSIC_DEVICE_TONE_EN && TCFG_LOCAL_TWS_ENABLE == 0)
        if (music_device_tone_play(logo) == true) {
            break;
        }
#endif
        log_i("APP_MSG_MUSIC_PLAY_START %s\n", logo);

        if (le_audio_scene_deal(LE_AUDIO_APP_MODE_ENTER) > 0) {
            break;
        }

        if (true == breakpoint_vm_read(music_hdl.breakpoint, logo)) {
            err = music_player_play_by_breakpoint(music_hdl.player_hd, logo, music_hdl.breakpoint);
        } else {
            err = music_player_play_first_file(music_hdl.player_hd, logo);
        }
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) && (LEA_BIG_FIX_ROLE==0)
        //这段代码是为了解决：不固定广播角色下打开广播，点击pp键，音乐模式广播下的状态混乱
        if (get_broadcast_role()) {
            if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                break;
            }
        }
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) && (LEA_BIG_FIX_ROLE==0)
        //这段代码是为了解决：不固定广播角色下打开广播，点击pp键，音乐模式广播下的状态混乱
        if (get_auracast_role()) {
            if (le_audio_scene_deal(LE_AUDIO_MUSIC_START) > 0) {
                app_send_message(APP_MSG_MUSIC_PLAY_STATUS, FILE_PLAYER_START);
                break;
            }
        }
#endif
        break;
    case APP_MSG_MUSIC_PLAY_START_BY_SCLUST:
        log_i("KEY_MUSIC_PLAYE_BY_DEV_SCLUST\n");
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        err = music_player_play_by_sclust(music_hdl.player_hd, logo, msg[1]);

        le_audio_scene_deal(LE_AUDIO_MUSIC_START);

        break;
    case APP_MSG_MUSIC_FR:
        printf("app msg music fr \n");
        music_file_player_fr(3, file_player);
        break;
    case APP_MSG_MUSIC_FF:
        printf("app msg music ff\n");
        music_file_player_ff(3, file_player);
        break;
    case APP_MSG_MUSIC_PLAYER_END:
        err = music_player_end_deal(music_hdl.player_hd, msg[1]);
        break;
    case APP_MSG_MUSIC_DEC_ERR:
        err = music_player_decode_err_deal(music_hdl.player_hd, msg[1]);
        break;
    case APP_MSG_MUSIC_PLAY_BY_NUM:
        printf("APP_MSG_MUSIC_PLAY_BY_NUM:%d\n", msg[1]);
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        err = music_player_play_by_number(music_hdl.player_hd, logo, msg[1]);

        le_audio_scene_deal(LE_AUDIO_MUSIC_START);

        break;
    case APP_MSG_MUSIC_PLAY_START_AT_DEST_TIME:
#if FILE_DEC_DEST_PLAY
        if (music_player_runing()) {
            //for test 测试MP3定点播放功能
            puts("\n play start at 5s \n");
            file_dec_set_start_play(5000, AUDIO_CODING_MP3);
        }
#endif
        break;
#if 0
    case APP_MSG_VOCAL_REMOVE:
        printf("APP_MSG_VOCAL_REMOVE\n");
        music_vocal_remover_switch();
        break;
    case APP_MSG_MIC_EFFECT_ON_OFF:
        printf("APP_MSG_MIC_EFFECT_ON_OFF\n");
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        } else {
            mic_effect_player_open();
        }
        break;
#endif
    default:
        app_common_key_msg_handler(msg);
        break;
    }
    music_player_err_deal(err);



#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    rcsp_device_status_update(MUSIC_FUNCTION_MASK,
                              BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));
#endif


    return 0;
}

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))

//当固定为接收端时，其它模式下开广播切进music模式，关闭广播后music模式不会自动播放
void music_set_broadcast_local_open_flag(u8 en)
{
#if (LEA_BIG_FIX_ROLE==2)
    music_hdl.close_broadcast_need_resume_local_music_flag = en;
#endif
}

static int get_music_play_status(void)
{
    if (get_le_audio_app_mode_exit_flag()) {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))
#if (LEA_BIG_FIX_ROLE==1)
    if (music_hdl.music_local_audio_resume_onoff == 1) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#endif

#if (LEA_BIG_FIX_ROLE==2)
    //固定为接收端时，打开广播接收后，如果连接上了会关闭本地的音频，当关闭广播后，需要恢复本地的音频播放
    /* music_hdl.close_broadcast_need_resume_local_music_flag = 0; */
    if (music_hdl.close_broadcast_need_resume_local_music_flag == 1 || music_hdl.music_local_audio_resume_onoff == 1) {
        /* y_printf("=============>>>>>>>>>>>>>>> %s, %d, return PLAY!\n", __func__, __LINE__); */
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        /* y_printf("=============>>>>>>>>>>>>>>> %s, %d, return STOP!\n", __func__, __LINE__); */
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
#endif
#endif

    if (music_file_get_player_status(get_music_file_player()) == FILE_PLAYER_START) {
        return LOCAL_AUDIO_PLAYER_STATUS_PLAY;
    } else {
        return LOCAL_AUDIO_PLAYER_STATUS_STOP;
    }
}

static int music_local_audio_open(void)
{
    int err = MUSIC_PLAYER_ERR_NULL;
    char *logo = NULL;

    if (1) {//(get_music_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开本地播放
#if (LEA_BIG_FIX_ROLE==2)
        if (music_hdl.close_broadcast_need_resume_local_music_flag) {
            music_hdl.close_broadcast_need_resume_local_music_flag = 0;
        }
#endif
        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_runing()) {
            if (dev_manager_get_logo(music_hdl.player_hd->dev) && logo) {///播放的设备跟当前活动的设备是同一个设备，不处理
                if (0 == strcmp(logo, dev_manager_get_logo(music_hdl.player_hd->dev))) {
                    log_w("the same dev!!\n");
                    return true;
                }
            }
        }

        if (true == breakpoint_vm_read(music_hdl.breakpoint, logo)) {
            err = music_player_play_by_breakpoint(music_hdl.player_hd, logo, music_hdl.breakpoint);
        } else {
            err = music_player_play_first_file(music_hdl.player_hd, logo);
        }
        ///错误处理
        music_player_err_deal(err);
    }
    return err;
}

static int music_local_audio_close(void)
{
    /* if (get_music_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) { */
    struct file_player *file_player = get_music_file_player();
#if (LEA_BIG_FIX_ROLE==1)
    if (music_file_get_player_status(file_player) == FILE_PLAYER_START) {
        music_hdl.music_local_audio_resume_onoff = 1;	//关闭广播需要恢复本地音频播放
    } else {
        music_hdl.music_local_audio_resume_onoff = 0;	//关闭广播需要恢复本地音频播放
    }
#endif

#if (LEA_BIG_FIX_ROLE==2)
    if (music_file_get_player_status(file_player) == FILE_PLAYER_START) {
        music_hdl.music_local_audio_resume_onoff = 1;
    } else {
        music_hdl.music_local_audio_resume_onoff = 0;
    }
#endif

    if (music_player_runing()) {
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==2)
        /* if (music_player_runing()) {	//这句判断需要放在 music_player_stop之前 */
        music_hdl.close_broadcast_need_resume_local_music_flag = 1;
        /* } */
#endif
        //关闭本地播放
        if (music_player_get_playing_breakpoint(music_hdl.player_hd, music_hdl.breakpoint, 1) == true) {
            music_player_stop(music_hdl.player_hd, 0); //先停止，防止下一步操作VM卡顿
            breakpoint_vm_write(music_hdl.breakpoint, dev_manager_get_logo(music_hdl.player_hd->dev));
        } else {
            music_player_stop(music_hdl.player_hd, 0);
        }
    } else {
#if (LEA_BIG_FIX_ROLE==1)
        music_hdl.music_local_audio_resume_onoff = 0;
#endif
    }
    return 0;
}

static void *music_tx_le_audio_open(void *fmt)
{
    int err;
    void *le_audio = NULL;
    g_le_audio_flag = 1;

    char *logo = NULL;

    if (1) {//(get_music_play_status() == LOCAL_AUDIO_PLAYER_STATUS_PLAY) {
        //打开广播音频播放
        struct le_audio_stream_params *params = (struct le_audio_stream_params *)fmt;
        struct le_audio_stream_format stream_fmt = params->fmt;
        le_audio = le_audio_stream_create(params->conn, &params->fmt);

        struct stream_enc_fmt enc_fmt = {
            .coding_type = stream_fmt.coding_type,
            .channel = stream_fmt.nch,
            .bit_rate = stream_fmt.bit_rate,
            .sample_rate = stream_fmt.sample_rate,
            .frame_dms = stream_fmt.frame_dms,
        };

        music_app_set_btaddr(le_audio, &enc_fmt);
        /* app_send_message(APP_MSG_MUSIC_PLAY_START, 0); */ // 注释是由于采用发送MUSIC_PLAY_START消息来开启解码音频数据流方式会导致固定发射端时会出现循环播放的问题

        logo = dev_manager_get_logo(dev_manager_find_active(1));
        if (music_player_runing()) {
            if (dev_manager_get_logo(music_hdl.player_hd->dev) && logo) {///播放的设备跟当前活动的设备是同一个设备，关闭当前播放
                if (0 == strcmp(logo, dev_manager_get_logo(music_hdl.player_hd->dev))) {
                    music_player_stop(music_hdl.player_hd, 0);
                }
            }
        }
        if (true == breakpoint_vm_read(music_hdl.breakpoint, logo)) {
            err = music_player_play_by_breakpoint(music_hdl.player_hd, logo, music_hdl.breakpoint);
        } else {
            err = music_player_play_first_file(music_hdl.player_hd, logo);
        }

        if (err == MUSIC_PLAYER_SUCC) {
            update_le_audio_deal_scene(LE_AUDIO_MUSIC_START);
        }
        music_player_err_deal(err);
    }

    return le_audio;
}

static int music_tx_le_audio_close(void *le_audio)
{
    if (!le_audio) {
        return -EPERM;
    }

    g_le_audio_flag = 0;
    if (music_player_get_playing_breakpoint(music_hdl.player_hd, music_hdl.breakpoint, 1) == true) {
        music_player_stop(music_hdl.player_hd, 0); //先停止，防止下一步操作VM卡顿
        breakpoint_vm_write(music_hdl.breakpoint, dev_manager_get_logo(music_hdl.player_hd->dev));
    }
    //关闭广播音频播放
#if LEA_LOCAL_SYNC_PLAY_EN
    le_audio_player_close(le_audio);
#endif
    le_audio_stream_free(le_audio);

    return 0;
}

static int music_rx_le_audio_open(void *rx_audio, void *args)
{
    int err;
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //打开广播音频播放
    struct le_audio_stream_params *params = (struct le_audio_stream_params *)args;
    rx_audio_hdl->le_audio = le_audio_stream_create(params->conn, &params->fmt);
    rx_audio_hdl->rx_stream = le_audio_stream_rx_open(rx_audio_hdl->le_audio, params->fmt.coding_type);
    err = le_audio_player_open(rx_audio_hdl->le_audio, params);
    if (err != 0) {
        ASSERT(0, "player open fail");
    }

    return 0;
}

static int music_rx_le_audio_close(void *rx_audio)
{
    struct le_audio_player_hdl *rx_audio_hdl = (struct le_audio_player_hdl *)rx_audio;

    if (!rx_audio) {
        return -EPERM;
    }

    //关闭广播音频播放
    le_audio_player_close(rx_audio_hdl->le_audio);
    le_audio_stream_rx_close(rx_audio_hdl->rx_stream);
    le_audio_stream_free(rx_audio_hdl->le_audio);

    return 0;
}

const struct le_audio_mode_ops le_audio_music_ops = {
    .local_audio_open = music_local_audio_open,
    .local_audio_close = music_local_audio_close,
    .tx_le_audio_open = music_tx_le_audio_open,
    .tx_le_audio_close = music_tx_le_audio_close,
    .rx_le_audio_open = music_rx_le_audio_open,
    .rx_le_audio_close = music_rx_le_audio_close,
    .play_status = get_music_play_status,
};

#endif

