#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".music_player.data.bss")
#pragma data_seg(".music_player.data")
#pragma const_seg(".music_player.text.const")
#pragma code_seg(".music_player.text")
#endif
#include "music/music_player.h"
#include "key_event_deal.h"
#include "app_config.h"
#include "app_main.h"
#include "vm.h"
#include "dev_manager.h"
#include "file_player.h"

#define LOG_TAG_CONST       APP_MUSIC
#define LOG_TAG             "[APP_MUSIC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


//播放参数，文件扫描时用，文件后缀等
static const char scan_parm[] = "-t"
#if (TCFG_DEC_MP3_ENABLE)
                                "MP1MP2MP3"
#endif
#if (TCFG_DEC_WMA_ENABLE)
                                "WMA"
#endif
#if ( TCFG_DEC_WAV_ENABLE)
                                "WAV"
#endif
#if (TCFG_DEC_DTS_ENABLE)
                                "DTS"
#endif
#if (TCFG_DEC_FLAC_ENABLE)
                                "FLA"
#endif
#if (TCFG_DEC_APE_ENABLE)
                                "APE"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_AAC_ENABLE)
                                "M4AAAC"
#endif
#if (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
                                "M4AMP4"
#endif
#if (TCFG_DEC_AMR_ENABLE)
                                "AMR"
#endif
#if (TCFG_DEC_OGG_VORBIS_ENABLE)
                                "OGG"
#endif
#if (TCFG_DEC_OGG_OPUS_ENABLE)
                                "OGGOPUS"
#endif

#if (TCFG_DEC_DECRYPT_ENABLE)
                                "SMP"
#endif
#if (TCFG_DEC_MIDI_ENABLE)
                                "MID"
#endif
#if (TCFG_DEC_AIFF_ENABLE)
                                "AIFAIFFAIFC"
#endif
                                " -sn -r"
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
                                " -m"
                                REC_FOLDER_NAME
#endif
                                ;

static volatile u8 save_mode_cnt = 0;
static volatile u16 save_mode_timer = 0;
/*----------------------------------------------------------------------------*/
/**@brief    music_player循环模式处理接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void music_player_mode_save_do(void *priv)
{
    local_irq_disable();
    if (++save_mode_cnt >= 5) {
        if (save_mode_timer) {
            sys_timer_del(save_mode_timer);
        }
        save_mode_timer = 0;
        save_mode_cnt = 0;
        local_irq_enable();
        syscfg_write(CFG_MUSIC_MODE, &app_var.cycle_mode, 1);
        return;
    }
    local_irq_enable();
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
struct music_player *music_player_creat(void)
{

    struct music_player *player_hd = zalloc(sizeof(struct music_player));
    return player_hd;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_playe注册扫盘回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_reg_scandisk_callback(struct music_player *player_hd, const struct __scan_callback *scan_cb)
{
    if (player_hd) {
        player_hd->parm.scan_cb = scan_cb;
        return true;
    }
    return false;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_playe注册解码状态回调
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_reg_dec_callback(struct music_player *player_hd, const struct __player_cb *cb)
{
    if (player_hd) {
        player_hd->parm.cb = cb;
        return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player释放接口
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_player_destroy(struct music_player *player_hd)
{
    if (player_hd) {
        music_player_stop(player_hd, 1);
        free(player_hd);
        player_hd = NULL;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放结束处理
   @param	 parm：结束参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_end_deal(struct music_player *player_hd, int parm)
{
    int err = 0;
    u8 read_err = (u8)(parm);
    //文件播放结束, 包括正常播放结束和读文件错误导致的结束, 如拔掉设备产生的错误结束
    log_i("STREAM_EVENT_STOP\n");
    if (read_err) {
        log_e("read err %d\n", read_err);
        if (read_err == 1) {
            err = MUSIC_PLAYER_ERR_FILE_READ;//文件读错误
        } else {
            err = MUSIC_PLAYER_ERR_DEV_READ;//设备读错误
        }

    } else {
        //正常结束，自动下一曲
#if (MUSIC_PLAYER_CYCLE_ALL_DEV_EN)
        u32 cur_file = player_hd->fsn->file_counter;
        if ((music_player_get_record_play_status(player_hd) == false)
            && (app_var.cycle_mode == FCYCLE_ALL)
            && (cur_file >= player_hd->fsn->file_number)
            && (dev_manager_get_total(1) > 1)) {
            char *logo = music_player_get_dev_flit(player_hd, "_rec", 1);
            if (logo) {
                err = music_player_play_first_file(player_hd, logo);
                return err;
            }
        }
#endif
        err = music_player_play_auto_next(player_hd);
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码错误处理
   @param	 event：err事件
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_err_deal(struct music_player *player_hd, int event)
{
    int err = 0;
    switch (event) {
    case STREAM_EVENT_NONE:
        //解码启动错误, 这里将错误码转换为music_player错误类型
        err = MUSIC_PLAYER_ERR_DECODE_FAIL;
        break;
    default:
        break;
    }
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放结束事件回调
   @param
   @return
   @note	非api层接口
*/
/*----------------------------------------------------------------------------*/
static int music_player_decode_event_callback(void *priv, int parm, enum stream_event event)
{
    struct music_player *player_hd = (struct music_player *)priv;
    if (event == STREAM_EVENT_STOP) {
        if (player_hd->parm.cb && player_hd->parm.cb->end) {
            player_hd->parm.cb->end(player_hd->priv, parm);
        }
    } else if (event == STREAM_EVENT_START) {
        if (player_hd->parm.cb && player_hd->parm.cb->start) {
            player_hd->parm.cb->start(player_hd->priv, 0);
        }
    } else if (event == STREAM_EVENT_NONE) {
        if (player_hd->parm.cb && player_hd->parm.cb->err) {
            player_hd->parm.cb->err(player_hd->priv, STREAM_EVENT_NONE);
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码器启动接口
   @param
			 file：
			 	文件句柄
			 dbp：
			 	断点信息
   @return   music_player 错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_decode_start(struct music_player *player_hd, FILE *file, struct audio_dec_breakpoint *dbp)
{
    if (file) {
        //get file short name
        u8 file_name[12 + 1] = {0}; //8.3+\0
        fget_name(player_hd->file, file_name, sizeof(file_name));
        log_i("\n");
        log_i("file name: %s\n", file_name);
        log_i("\n");
    }
    int ret;
    struct file_player *file_player;
    if (!dbp) {
        dbp = music_app_get_dbp_addr();
        if (dbp) {
            dbp->len = 0;
        }
    }

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    if (get_music_le_audio_flag()) {
        file_player = le_audio_music_file_play_callback(file, (void *)player_hd, music_player_decode_event_callback, dbp, (void *)player_hd->le_audio, (void *)&player_hd->fmt);
    } else {
        file_player = music_file_play_callback(file, (void *)player_hd, music_player_decode_event_callback, dbp);
    }
#else
    file_player = music_file_play_callback(file, (void *)player_hd, music_player_decode_event_callback, dbp);
#endif
    if (!file_player) {
        return MUSIC_PLAYER_ERR_DECODE_FAIL;
    }
    return MUSIC_PLAYER_SUCC;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备断点信息
   @param
			 bp：
			 	断点缓存，外部调用提供
			 flag：
			 	1：需要获取歌曲断点信息及文件信息， 0：只获取文件信息
   @return   成功与否
   @note
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_playing_breakpoint(struct music_player *player_hd, struct __breakpoint *bp, u8 flag)
{
    if (player_hd == NULL || bp == NULL) {
        return false;
    }
    int data_len = bp->dbp.data_len;
    struct __breakpoint *temp_bp = zalloc(sizeof(struct __breakpoint) + data_len);
    temp_bp->dbp.data_len = data_len;
    if (dev_manager_online_check(player_hd->dev, 1)) {
        if (music_player_runing()) {
            if (player_hd->file) {
                if (flag) {
                    //获取断点解码信息
                    struct file_player *file_player = get_music_file_player();
                    int ret = music_file_get_breakpoints(&temp_bp->dbp, file_player);
                    if (ret) {
                        //获取断点解码信息错误
                        log_e("file_dec_get_breakpoint err !!\n");
                    }
                }
                memcpy(bp->dbp.data, temp_bp->dbp.data, temp_bp->dbp.len);
                bp->dbp.len = temp_bp->dbp.len;
                //获取断点文件信息
                struct vfs_attr attr = {0};
                fget_attrs(player_hd->file, &attr);
                bp->sclust = attr.sclust;
                bp->fsize = attr.fsize;
                log_i("get playing breakpoint ok\n");
                free(temp_bp);
                mem_stats();
                return true;
            }
        } else { //设备在线,但是音乐播放流已经结束了,断点已经由解码节点保存过了
            free(temp_bp);
            return true;
        }
    }
    free(temp_bp);
    return false;

}

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取文件句柄
   @param
   @return   文件句柄
   @note	 需要注意文件句柄的生命周期
*/
/*----------------------------------------------------------------------------*/
FILE *music_player_get_file_hdl(struct music_player *player_hd)
{
    if (player_hd && player_hd->file) {
        return player_hd->file;
    }
    return NULL;
}

void music_player_remove_file_hdl(struct music_player *player_hd)
{
    if (player_hd && player_hd->file) {
        player_hd->file = NULL;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备下一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_next(struct music_player *player_hd, u8 auto_next)
{
    if (player_hd) {
        if (auto_next) {
            return dev_manager_get_logo(dev_manager_find_next(player_hd->dev, 1));
        } else {
            //跳过录音设备
            struct __dev *next = dev_manager_find_next(player_hd->dev, 1);
            if (next) {
                char *logo = dev_manager_get_logo(next);
                if (logo) {
                    char *str = strstr(logo, "_rec");
                    if (str) {
                        char *cur_phy_logo = dev_manager_get_phy_logo(player_hd->dev);
                        char *next_phy_logo = dev_manager_get_phy_logo(next);
                        if (cur_phy_logo && next_phy_logo && strcmp(cur_phy_logo, next_phy_logo) == 0)	{
                            //是同一个物理设备的录音设别， 跳过
                            next = dev_manager_find_next(next, 1);
                            if (next != player_hd->dev) {
                                logo = dev_manager_get_logo(next);
                                return logo;
                            } else {
                                //没有其他设备了(只有录音文件夹设备及本身)
                                return NULL;
                            }
                        }
                    }
                    return logo;
                }
            }
        }
    }
    return NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放设备上一个设备
   @param
   @return   设备盘符
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_prev(struct music_player *player_hd)
{
    if (player_hd) {
        return dev_manager_get_logo(dev_manager_find_prev(player_hd->dev, 1));
    }
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放循环模式
   @param
   @return   当前播放循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
u8 music_player_get_repeat_mode(void)
{
    return app_var.cycle_mode;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前播放对应的music设备
   @param
   @return   设备盘符
   @note	 播放录音区分时，可以通过该接口判断当前播放的音乐设备是什么以便做录音区分判断
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_cur_music_dev(struct music_player *player_hd)
{
    if (player_hd) {
        char music_dev_logo[16] = {0};
        char *logo = dev_manager_get_logo(player_hd->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                //录音设备,切换到音乐设备播放
                strncpy(music_dev_logo, logo, strlen(logo) - strlen(str));
                logo = dev_manager_get_logo(dev_manager_find_spec(music_dev_logo, 1));
            }
        }
        return logo;
    }
    return NULL;
}

const char *music_player_get_phy_dev(struct music_player *player_hd, int *len)
{
    if (player_hd) {
        char *logo = dev_manager_get_logo(player_hd->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                //录音设备,切换到音乐设备播放
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



/*----------------------------------------------------------------------------*/
/**@brief    music_player获取当前录音区分播放状态
   @param
   @return   true：录音文件夹播放, false：非录音文件夹播放
   @note	 播放录音区分时，可以通过该接口判断当前播放的是录音文件夹还是非录音文件夹
*/
/*----------------------------------------------------------------------------*/
bool music_player_get_record_play_status(struct music_player *player_hd)
{
    if (player_hd) {
        char *logo = dev_manager_get_logo(player_hd->dev);
        if (logo) {
            char *str = strstr(logo, "_rec");
            if (str) {
                return true;
            }
        }
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player从设备列表里面往前或往后找设备，并且过滤掉指定字符串的设备
   @param
   			flit:过滤字符串， 查找设备时发现设备logo包含这个字符串的会被过滤
			direct：查找方向， 1:往后， 0：往前
   @return   查找到符合条件的设备逻辑盘符， 找不到返回NULL
   @note
*/
/*----------------------------------------------------------------------------*/
char *music_player_get_dev_flit(struct music_player *player_hd, char *flit, u8 direct)
{
    if (player_hd) {
        u32 counter = 0;
        struct __dev *dev = player_hd->dev;
        u32 dev_total = dev_manager_get_total(1);
        if (dev_manager_online_check(player_hd->dev, 1) == 1) {
            if (dev_total > 1) {
                while (1) {
                    if (direct) {
                        dev = dev_manager_find_next(dev, 1);
                    } else {
                        dev = dev_manager_find_prev(dev, 1);
                    }
                    if (dev) {
                        char *logo = dev_manager_get_logo(dev);
                        if (flit) {
                            char *str = strstr(logo, flit);
                            if (!str) {
                                return logo;
                            }
                            counter++;
                            if (counter >= (dev_total - 1)) {
                                break;
                            }
                        } else {
                            return logo;
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player解码停止
   @param
			 fsn_release：
				1：释放扫盘句柄
				0：不释放扫盘句柄
   @return
   @note	 如果释放了扫盘句柄，需要重新扫盘，否则播放失败
*/
/*----------------------------------------------------------------------------*/
void music_player_stop(struct music_player *player_hd, u8 fsn_release)
{
    if (player_hd == NULL) {
        return ;
    }

#if (defined(TCFG_LRC_LYRICS_ENABLE) && (TCFG_LRC_LYRICS_ENABLE))
    extern void lrc_set_analysis_flag(u8 flag);
    lrc_set_analysis_flag(0);
#endif
    //停止解码
    music_file_player_stop();
    if (player_hd->file) {
        fclose(player_hd->file);
        player_hd->file = NULL;
    }

    if (player_hd->lrc_file) {
        fclose(player_hd->lrc_file);
        player_hd->lrc_file = NULL;
    }

    if (fsn_release && player_hd->fsn) {
        //根据播放情景， 通过设定flag决定是否需要释放fscan， 释放后需要重新扫盘!!!
        fscan_release(player_hd->fsn);
        player_hd->fsn = NULL;
    }

    //如果开启把vm配置项暂存到ram的功能,则不需要定期整理vm，增加操作flash的时间
    if (get_vm_ram_storage_enable() == FALSE) {
        //检查整理VM
        vm_check_all(0);
    }
}



/*----------------------------------------------------------------------------*/
/**@brief    music_player设置播放循环模式
   @param	 mode：循环模式
				FCYCLE_ALL
				FCYCLE_ONE
				FCYCLE_FOLDER
				FCYCLE_RANDOM
   @return  循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_set_repeat_mode(struct music_player *player_hd, u8 mode)
{
    if (player_hd) {
        if (mode >= FCYCLE_MAX) {
            return -1;
        }
        app_var.cycle_mode = mode;
        if (player_hd->fsn) {
            player_hd->fsn->cycle_mode = mode;
            log_i("cycle_mode = %d\n", mode);
            local_irq_disable();
            save_mode_cnt = 0;
            if (save_mode_timer == 0) {
                save_mode_timer = sys_timer_add(NULL, music_player_mode_save_do, 1000);
            }
            local_irq_enable();
            return mode;
        }
    }
    return -1;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player切换循环模式
   @param
   @return   循环模式
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_change_repeat_mode(struct music_player *player_hd)
{
    if (player_hd) {
        app_var.cycle_mode++;
        if (app_var.cycle_mode >= FCYCLE_MAX) {
            app_var.cycle_mode = FCYCLE_LIST;
        }
        return  music_player_set_repeat_mode(player_hd, app_var.cycle_mode);
    }
    return -1;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player删除当前播放文件,并播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_delete_playing_file(struct music_player *player_hd)
{
    if (player_hd && player_hd->file) {
        //获取当前播放文件序号， 文件删除之后， 播放下一曲
        int err = 0;
        int cur_file = player_hd->fsn->file_counter;
        char *cur_dev = dev_manager_get_logo(player_hd->dev);
        music_file_player_stop();
        err = fdelete(player_hd->file);
        if (err) {
            log_info("[%s, %d] fail!!, replay cur file\n", __FUNCTION__, __LINE__);
        } else {
            log_info("[%s, %d] ok, play next file\n", __FUNCTION__, __LINE__);
            player_hd->file = NULL;
            player_hd->dev = NULL;//目的重新扫盘， 更新文件总数
            return music_player_play_by_number(player_hd, cur_dev, cur_file);
        }
    }
    return MUSIC_PLAYER_ERR_NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player播放上一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_prev_cycle_single_dev(struct music_player *player_hd)
{
    //close player first
    music_player_stop(player_hd, 0);
    //check dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    //不需要重新找设备、扫盘
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_PREV_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//选择上一曲
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}

int music_player_play_prev(struct music_player *player_hd)
{
    int err;
#if (MUSIC_PLAYER_CYCLE_ALL_DEV_EN)
    u32 cur_file = player_hd->fsn->file_counter;
    if ((music_player_get_record_play_status(player_hd) == false)
        && (app_var.cycle_mode == FCYCLE_ALL)
        && (cur_file == 1)
        && (dev_manager_get_total(1) > 1)) {
        char *logo = music_player_get_dev_flit(player_hd, "_rec", 0);
        err = music_player_play_last_file(player_hd, logo);
        return err;
    }
#endif
    err = music_player_play_prev_cycle_single_dev(player_hd);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_next_cycle_single_dev(struct music_player *player_hd)
{
    //close player first
    music_player_stop(player_hd, 0);
    //check dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    //不需要重新找设备、扫盘
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_NEXT_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//选择下一曲
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}

int music_player_play_next(struct music_player *player_hd)
{
    int err;
#if (MUSIC_PLAYER_CYCLE_ALL_DEV_EN)
    u32 cur_file = player_hd->fsn->file_counter;
    if ((music_player_get_record_play_status(player_hd) == false)
        && (app_var.cycle_mode == FCYCLE_ALL)
        && (cur_file >= player_hd->fsn->file_number)
        && (dev_manager_get_total(1) > 1)) {
        char *logo = music_player_get_dev_flit(player_hd, "_rec", 1);
        err = music_player_play_first_file(player_hd, logo);
        return err;
    }
#endif
    err = music_player_play_next_cycle_single_dev(player_hd);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    music_player播放第一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_first_file(struct music_player *player_hd, char *logo)
{
    if (logo == NULL) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_FIRST_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player播放最后一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_last_file(struct music_player *player_hd, char *logo)
{
    if (logo == NULL) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_LAST_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player自动播放下一曲
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_auto_next(struct music_player *player_hd)
{
    //close player first
    music_player_stop(player_hd, 0);
    //get dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    //不需要重新找设备、扫盘
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_AUTO_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//选择自动下一曲
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player上一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_prev(struct music_player *player_hd)
{
    //close player first
    music_player_stop(player_hd, 0);
    //get dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    //不需要重新找设备、扫盘
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_PREV_FOLDER_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//选择播放上一个文件夹
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
#if (MUSIC_PLAYER_PLAY_FOLDER_PREV_FIRST_FILE_EN)
    struct ffolder folder_info = {0};
    if (fget_folder(player_hd->fsn, &folder_info)) {
        log_e("folder info err!!\n");
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }

    //先关掉文件
    fclose(player_hd->file);
    //播放文件夹第一首
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_NUMBER, folder_info.fileStart, player_hd->parm.scan_cb);
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
#endif
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player下一个文件夹
   @param
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_folder_next(struct music_player *player_hd)
{
    //close player first
    music_player_stop(player_hd, 0);
    //get dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    //不需要重新找设备、扫盘
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_NEXT_FOLDER_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);///选择播放上一个文件夹
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player上一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_prev(struct music_player *player_hd, struct __breakpoint *bp)
{
    //close player first
    music_player_stop(player_hd, 1);
    //get dev
    player_hd->dev = dev_manager_find_prev(player_hd->dev, 1);
    if (player_hd->dev == NULL) {
        return MUSIC_PLAYER_ERR_DEV_NOFOUND;
    }
    //get fscan
    player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, (const char *)scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    int err = 0;
    if (bp) {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_SCLUST, bp->sclust, (struct __scan_callback *)player_hd->parm.scan_cb);//根据文件簇号查找断点文件
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(player_hd, player_hd->file, &(bp->dbp));
    } else {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_FIRST_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(player_hd, player_hd->file, 0);
    }
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player下一个设备
   @param	 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_devcie_next(struct music_player *player_hd, struct __breakpoint *bp)
{
    //close player first
    music_player_stop(player_hd, 1);
    //get dev
    player_hd->dev = dev_manager_find_next(player_hd->dev, 1);
    if (player_hd->dev == NULL) {
        return MUSIC_PLAYER_ERR_DEV_NOFOUND;
    }
    //get fscan
    player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    int err = 0;
    if (bp) {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_SCLUST, bp->sclust, (struct __scan_callback *)player_hd->parm.scan_cb);//根据文件簇号查找断点文件
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(player_hd, player_hd->file, &(bp->dbp));
    } else {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_FIRST_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//选择第一个文件播放
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        err = music_player_decode_start(player_hd, player_hd->file, 0);
    }
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player断点播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 bp：断点信息
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_breakpoint(struct music_player *player_hd, char *logo, struct __breakpoint *bp)
{
    u32 bp_flag = 1;
    if (bp == NULL) {
        return music_player_play_first_file(player_hd, logo);
    }
    if (logo == NULL) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        bp_flag = 0;
        set_bp_info(bp->sclust, bp->fsize, &bp_flag); //断点若有效把bp_flag置1,注意后面要用put_bp_info释放资源
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }
    if (player_hd->fsn == NULL) {
        put_bp_info();
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    if (!bp_flag) { //断点无效
        put_bp_info();
        return MUSIC_PLAYER_ERR_PARM;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_SCLUST, bp->sclust, (struct __scan_callback *)player_hd->parm.scan_cb);//根据文件簇号查找断点文件
    put_bp_info();
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }

    struct vfs_attr attr = {0};
    fget_attrs(player_hd->file, &attr);
    if (bp->fsize != attr.fsize) {
        return MUSIC_PLAYER_ERR_PARM;
    }

    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, &(bp->dbp));
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player序号播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 number：指定播放序号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_number(struct music_player *player_hd, char *logo, u32 number)
{
    char *cur_logo = dev_manager_get_logo(player_hd->dev);
    if (logo == NULL || (cur_logo && logo && (0 == strcmp(logo, cur_logo)))) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }

    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_NUMBER, number, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player簇号播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0
   			 sclust：指定播放簇号
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_sclust(struct music_player *player_hd, char *logo, u32 sclust)
{
    char *cur_logo = dev_manager_get_logo(player_hd->dev);
    if (logo == NULL || (cur_logo && logo && (0 == strcmp(logo, cur_logo)))) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_SCLUST, sclust, (struct __scan_callback *)player_hd->parm.scan_cb);//根据簇号查找文件
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player路径播放指定设备
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
   			 path：指定播放路径
   @return   播放错误码
   @note
*/
/*----------------------------------------------------------------------------*/
int music_player_play_by_path(struct music_player *player_hd, char *logo, const char *path)
{
    if (path == NULL) {
        return MUSIC_PLAYER_ERR_POINT;
    }
    if (logo == NULL) {
        music_player_stop(player_hd, 0);
        if (dev_manager_online_check(player_hd->dev, 1) == 0) {
            return MUSIC_PLAYER_ERR_DEV_OFFLINE;
        }
        //没有指定设备不需要找设备， 不需要扫描
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev_manager_find_spec(logo, 1);
        if (player_hd->dev == NULL) {
            return MUSIC_PLAYER_ERR_DEV_NOFOUND;
        }
        player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    }
    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }
    //get file
    player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_PATH, (int)path, (struct __scan_callback *)player_hd->parm.scan_cb);//根据簇号查找文件
    if (player_hd->file == NULL) {
        return MUSIC_PLAYER_ERR_FILE_NOFOUND;
    }
    //start decoder
    int err = music_player_decode_start(player_hd, player_hd->file, 0);
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d] ok\n", __FUNCTION__, __LINE__);
    }
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player录音区分切换播放
   @param
   			 logo：逻辑盘符，如：sd0/sd1/udisk0, 设置为NULL，为默认当前播放设备
   			 bp：断点信息
   @return   播放错误码
   @note	 通过指定设备盘符，接口内部通过解析盘符是否"_rec"
   			 来确定是切换到录音播放设备还是非录音播放设备
*/
/*----------------------------------------------------------------------------*/
int music_player_play_record_folder(struct music_player *player_hd, char *logo, struct __breakpoint *bp)
{
    int err = MUSIC_PLAYER_ERR_NULL;
#if (TCFG_RECORD_FOLDER_DEV_ENABLE)
    char rec_dev_logo[16] = {0};
    char music_dev_logo[16] = {0};
    u8 rec_play = 0;
    struct __dev *dev;
    if (logo == NULL) {
        logo = dev_manager_get_logo(player_hd->dev);
        if (logo == NULL) {
            return MUSIC_PLAYER_ERR_RECORD_DEV;
        }
    }

    //判断是否是录音设备
    char *str = strstr(logo, "_rec");
    if (str == NULL) {
        //是非录音设备，切换到录音设备播放
        sprintf(rec_dev_logo, "%s%s", logo, "_rec");
        dev = dev_manager_find_spec(rec_dev_logo, 1);
        logo = rec_dev_logo;
        rec_play = 1;
    } else {
        //录音设备,切换到音乐设备播放
        strncpy(music_dev_logo, logo, strlen(logo) - strlen(str));
        log_i("music_dev_logo = %s, logo = %s, str = %s, len = %d\n", music_dev_logo, logo, str, strlen(logo) - strlen(str));
        dev = dev_manager_find_spec(music_dev_logo, 1);
        logo = music_dev_logo;
        rec_play = 0;
    }
    if (dev == NULL) {
        return MUSIC_PLAYER_ERR_RECORD_DEV;
    }

    ///需要扫盘
    struct vfscan *fsn = file_manager_scan_disk(dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);
    if (fsn == NULL) {
        dev_manager_set_valid(dev, 0);
        return MUSIC_PLAYER_ERR_RECORD_DEV;
    } else {
        music_player_stop(player_hd, 1);
        player_hd->dev = dev;
        player_hd->fsn = fsn;
    }
    //get file
    if (bp) {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_BY_SCLUST, bp->sclust, (struct __scan_callback *)player_hd->parm.scan_cb);
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        //start decoder
        err = music_player_decode_start(player_hd->file, &bp->dbp);
    } else {
        player_hd->file = file_manager_select(player_hd->dev, player_hd->fsn, FSEL_FIRST_FILE, 0, (struct __scan_callback *)player_hd->parm.scan_cb);//播放录音文件夹第一个文件
        if (player_hd->file == NULL) {
            return MUSIC_PLAYER_ERR_FILE_NOFOUND;
        }
        //start decoder
        err = music_player_decode_start(player_hd->file, 0);//录音文件夹不支持断点播放
    }
    if (err == MUSIC_PLAYER_SUCC) {
        //选定新设备播放成功后，需要激活当前设备
        dev_manager_set_active(player_hd->dev);
        log_i("[%s %d]  %s devcie play ok\n", __FUNCTION__, __LINE__, logo);
    }
#endif
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    music_player扫盘
   @param
   @return   播放错误码
   @note 	 音乐模式下盘符曲目发生变化时
			 调用接口重新扫盘当前设备
*/
/*----------------------------------------------------------------------------*/
int music_player_scan_disk(struct music_player *player_hd)
{
    char *logo = NULL;
    if (!player_hd) {
        return MUSIC_PLAYER_ERR_PARM;
    }
    music_player_stop(player_hd, 1);
    //check dev, 检查设备是否有掉线
    if (dev_manager_online_check(player_hd->dev, 1) == 0) {
        return MUSIC_PLAYER_ERR_DEV_OFFLINE;
    }
    player_hd->fsn = file_manager_scan_disk(player_hd->dev, NULL, scan_parm, app_var.cycle_mode, (struct __scan_callback *)player_hd->parm.scan_cb);

    if (player_hd->fsn == NULL) {
        return MUSIC_PLAYER_ERR_FSCAN;
    }

    return MUSIC_PLAYER_ERR_NULL;
}


int music_player_lrc_analy_start(struct music_player *player_hd)
{
#if (defined(TCFG_LRC_LYRICS_ENABLE) && (TCFG_LRC_LYRICS_ENABLE))
    extern bool lrc_analysis_api(FILE * file, FILE **newFile);
    extern void lrc_set_analysis_flag(u8 flag);
    log_i("music lrc analys start:");

    if (player_hd && player_hd->file) {
        if (lrc_analysis_api(player_hd->file, &(player_hd->lrc_file))) {
            lrc_set_analysis_flag(1);
            return 0;
        } else {
            lrc_set_analysis_flag(0);
            return -1;
        }
    }

#endif
    return -1;
}





