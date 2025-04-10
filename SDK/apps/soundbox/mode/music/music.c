#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".music.data.bss")
#pragma data_seg(".music.data")
#pragma const_seg(".music.text.const")
#pragma code_seg(".music.text")
#endif
#include "system/includes.h"
#include "music/music_player.h"
#include "music/breakpoint.h"
#include "app_action.h"
#include "app_main.h"
#include "soundbox.h"
#include "tone_player.h"
#include "key_event_deal.h"
#include "audio_config.h"
#include "bt_background.h"
#include "default_event_handler.h"
#include "app_online_cfg.h"
#include "system/event.h"
#include "app_music.h"
#include "app_tone.h"
#include "dev_status.h"
#include "effect/effects_default_param.h"
#include "scene_switch.h"
#include "mic_effect.h"
#include "audio_config_def.h"
#if (TCFG_LRC_LYRICS_ENABLE)
#include "ui/lcd/lyrics_api.h"
#endif
#include "local_tws.h"
#include "app_le_broadcast.h"
#include "app_le_connected.h"
#include "app_le_auracast.h"
#include "rcsp_device_status.h"
#include "rcsp_music_func.h"
#include "rcsp_config.h"

#if TCFG_APP_MUSIC_EN

//切歌切模式是否保持倍速播放的状态
#define MUSIC_PLAYBACK_SPEED_KEEP            0

//切歌切模式是否保持变调的状态
#define MUSIC_PLAYBACK_PITCH_KEEP            0

static u8 music_idle_flag = 1;



struct __music music_hdl = {
    .speed_mode = PLAY_SPEED_1,
    .pitch_mode = PITCH_0,
};
#define __this (&music_hdl)


#if LIB_SUPPORT_MULTI_SECTOER_READ
OS_MUTEX pre_read_mutex;
char *prebuf = NULL;
#endif


//设备提示音使能
#if (TCFG_MUSIC_DEVICE_TONE_EN)
struct __dev_tone {
    char *logo;
    char *phy_logo;
    u32	 index;
};

enum {
    ///0x1000起始为了不要跟提示音的IDEX_TONE_重叠了
    DEVICE_INDEX_UDISK = 0x1000,
    DEVICE_INDEX_UDISK_REC,
    DEVICE_INDEX_SD0,
    DEVICE_INDEX_SD0_REC,
    DEVICE_INDEX_SD1,
    DEVICE_INDEX_SD1_REC,
};
const struct __dev_tone device_tone[] = {
    {"udisk0", 		"udisk0", 		DEVICE_INDEX_UDISK}			,
    {"udisk0_rec",  "udisk0", 	 	DEVICE_INDEX_UDISK_REC}		,
    {"sd0",  		"sd0", 		 	DEVICE_INDEX_SD0}		 	,
    {"sd0_rec",  	"sd0", 			DEVICE_INDEX_SD0_REC}		,
    {"sd1",  		"sd1", 		 	DEVICE_INDEX_SD1}			,
    {"sd1_rec",  	"sd1", 		 	DEVICE_INDEX_SD1_REC}		,
};

static int music_tone_play_end_callback(void *priv, enum stream_event event);

const char *get_music_tone_name_by_logo(const char *logo)
{
    const char *file_name = NULL;
    for (int i = 0; i < ARRAY_SIZE(device_tone); i++) {
        if (strcmp(device_tone[i].logo, logo) == 0) {
            if (device_tone[i].index < DEVICE_INDEX_SD0) {
                file_name = get_tone_files()->device_udisk;
            } else {
                file_name = get_tone_files()->device_sd;
            }
        }
    }
    return file_name;
}


int music_device_tone_play(char *logo)
{
    if (logo == NULL) {
        return false;
    }
    int ret = 0;
    printf("__this->device_tone_dev = %s, logo =%s\n", __this->device_tone_dev, logo);
    char *phy_logo = dev_manager_get_phy_logo(dev_manager_find_spec(logo, 0));
    if (phy_logo && (strcmp(__this->device_tone_dev, phy_logo) == 0)) {
        log_i("[%s, %d]the same phy dev, no need device tone!!\n", logo, __LINE__);
        return false;
    }
    for (int i = 0; i < ARRAY_SIZE(device_tone); i++) {
        if (strcmp(device_tone[i].logo, logo) == 0) {
            log_i("[%s, %d]device_tone play \n", logo, __LINE__);
            memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
            memcpy(__this->device_tone_dev, device_tone[i].phy_logo, strlen(device_tone[i].phy_logo));
            if (device_tone[i].index < DEVICE_INDEX_SD0) {
                ret = play_tone_file_callback(get_tone_files()->device_udisk, (void *)device_tone[i].index, music_tone_play_end_callback);
            } else {
                ret = play_tone_file_callback(get_tone_files()->device_sd, (void *)device_tone[i].index, music_tone_play_end_callback);
            }
            if (ret) { //提示音播放失败
                music_tone_play_end_callback((void *)device_tone[i].index, STREAM_EVENT_NONE);
            }
            return true;
        }
    }
    log_i("[%s, %d]device_tone play \n", logo, __LINE__);
    return false;
}


#endif
/* extern const u8 music_key_table[][KEY_EVENT_MAX]; */

static u8 music_idle_query()
{
    return music_idle_flag;
}
REGISTER_LP_TARGET(music_lp_target) = {
    .name = "music",
    .is_idle = music_idle_query,
};

static void music_save_breakpoint(int save_dec_bp)
{
    char *logo = NULL;
    if (__this->player_hd) {
        logo = dev_manager_get_logo(__this->player_hd->dev);
    }
    ///save breakpoint, 只保存文件信息
    if (music_player_get_playing_breakpoint(__this->player_hd, __this->breakpoint, save_dec_bp) == true) {
        breakpoint_vm_write(__this->breakpoint, logo);
    }
}

/*获取music app当前播放的设备*/
char *music_app_get_dev_cur(void)
{
    if (__this->player_hd && __this->player_hd->dev) {
        return dev_manager_get_logo(__this->player_hd->dev);
    }
    return NULL;
}

/*获取music app当前的断点地址*/
struct audio_dec_breakpoint *music_app_get_dbp_addr(void)
{
    if (__this && __this->breakpoint) {
        return &(__this->breakpoint->dbp);
    }
    return NULL;
}

/*获取music app当前播放的句柄*/
struct music_player *music_app_get_cur_hdl(void)
{
    return __this->player_hd;
}

/*设置music app 的广播音箱句柄*/
u8 music_app_set_btaddr(void *le_audio, struct stream_enc_fmt *fmt)
{
    if (__this->player_hd) {
        __this->player_hd->le_audio = le_audio;
        memcpy(&__this->player_hd->fmt, fmt, sizeof(struct stream_enc_fmt));
        return 0;
    }
    return -1;
}

//*----------------------------------------------------------------------------*/
/**@brief    music 模式切换前参数设置
   @param    type:播放方式,暂时支持：
  				  MUSIC_TASK_START_BY_NORMAL：首次播放按照正常断点播放
				  MUSIC_TASK_START_BY_SCLUST：首次播放按照簇号播放
			 val:播放参数
   @return
   @note	 首次播放执行参考music_player_play_start接口
*/
/*----------------------------------------------------------------------------*/
void music_task_set_parm(u8 type, int val)
{
    __this->task_parm.type = type;
    __this->task_parm.val = val;
}
//*----------------------------------------------------------------------------*/
/**@brief    music 模式首次播放
   @param    无
   @return
   @note	 切换到音乐模式前可以通过接口music_task_set_parm设置参数,
   			 进入音乐模式后会按照对应参数播放
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_start(void)
{
    switch (__this->task_parm.type) {
    case MUSIC_TASK_START_BY_NORMAL:
        log_i("MUSIC_TASK_START_BY_NORMAL\n");
        app_send_message(APP_MSG_MUSIC_PLAY_START, 0);
        break;
    case MUSIC_TASK_START_BY_SCLUST:
        log_i("MUSIC_TASK_START_BY_SCLUST\n");
        app_send_message(APP_MSG_MUSIC_PLAY_START_BY_SCLUST, 0);
        break;
    default:
        log_i("other MUSIC_TASK_START!!!\n");
        break;
    }

}

//*----------------------------------------------------------------------------*/
/**@brief    录音模式提示音结束处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int music_tone_play_end_callback(void *priv, enum stream_event event)
{
    u32 index = (u32)priv;
    char *logo = NULL;

    if (false == app_in_mode(APP_MODE_MUSIC)) {
        log_e("tone callback task out \n");
        return 0;
    }
    switch (event) {
    /* case STREAM_EVENT_NONE: */
    case STREAM_EVENT_STOP:
        switch (index) {
#if (TCFG_MUSIC_DEVICE_TONE_EN)
        case DEVICE_INDEX_UDISK:
        case DEVICE_INDEX_UDISK_REC:
        case DEVICE_INDEX_SD0:
        case DEVICE_INDEX_SD0_REC:
        case DEVICE_INDEX_SD1:
        case DEVICE_INDEX_SD1_REC:
            for (int i = 0; i < ARRAY_SIZE(device_tone); i++) {
                if (index == device_tone[i].index) {
                    logo = device_tone[i].logo;
                    break;
                }
            }
            app_send_message(APP_MSG_MUSIC_PLAY_START_BY_DEV, (int)logo);
            break;
#endif
        default:
            ///提示音播放结束， 启动播放器播放
            music_player_play_start();
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}
/*根据播放文件的格式动态开启SD卡常活动状态*/
static void music_set_sd_keep_active(char *logo)
{
    struct imount *mount_hdl = NULL;
    if (logo) {
        if ((!memcmp(logo, "sd0", strlen("sd0")))
            || (!memcmp(logo, "sd1", strlen("sd1")))) {
            struct file_player *file_play_hd = get_music_file_player();
            mount_hdl = dev_manager_get_mount_hdl(dev_manager_find_spec(logo, 0));
            if (file_play_hd) {
                if (file_play_hd->stream->coding_type == AUDIO_CODING_WAV
                    || file_play_hd->stream->coding_type == AUDIO_CODING_DTS
                    || file_play_hd->stream->coding_type == AUDIO_CODING_FLAC
                    || file_play_hd->stream->coding_type == AUDIO_CODING_APE) {
                    dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ACTIVE_STATUS, 1);
                    log_i("sd set active status 1\n");
                } else {
                    dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ACTIVE_STATUS, 0);
                    log_i("sd set active status 0\n");
                }
            }
        }
    } else {
        //非播放状态 关闭sd卡常活动状态
        mount_hdl = dev_manager_get_mount_hdl(dev_manager_find_spec("sd0", 0));
        if (mount_hdl) {
            dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ACTIVE_STATUS, 0);
            log_i("sd0 set active status 0\n");
        }
        mount_hdl = dev_manager_get_mount_hdl(dev_manager_find_spec("sd1", 0));
        if (mount_hdl) {
            dev_ioctl(mount_hdl->dev.fd, IOCTL_SET_ACTIVE_STATUS, 0);
            log_i("sd1 set active status 0\n");
        }
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music 解码成功回调
  @param    priv:私有参数， parm:暂时未用
   @return
   @note	 此处可以做一些用户操作， 如断点保存， 显示， 获取播放信息等
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_success(void *priv, int parm)
{
    char *logo = dev_manager_get_logo(__this->player_hd->dev);
#if MUSIC_PLAYBACK_SPEED_KEEP
    struct file_player *file_player = get_music_file_player();
    music_file_set_speed(file_player, music_hdl.speed_mode); //设置播放速度
#endif

#if MUSIC_PLAYBACK_PITCH_KEEP
    struct file_player *file_player = get_music_file_player();
    music_file_set_pitch(file_player, music_hdl.pitch_mode);
#endif
    musci_vocal_remover_update_parm();
    //播放WAV APE 格式歌曲，需设置SD卡常活动状态，提高读取速度
    music_set_sd_keep_active(logo);

    log_i("cur filenum = %d\n", __this->player_hd->fsn->file_counter);
    log_i("totol filenum = %d\n", __this->player_hd->fsn->file_number);
    /* log_i("totol time = %d\n", music_player_get_dec_total_time()); */
    int unicode = 0;
    int name_buf_need_len = 128;
    u8 *music_file_name = zalloc(name_buf_need_len);
    int music_file_name_len =  fget_name(__this->player_hd->file, music_file_name, name_buf_need_len);
    if ((music_file_name[0] == '\\') && (music_file_name[1] == 'U')) {
        unicode = 1;
        //music_file_name_len -= 2;
        log_i("cur file(unicode)  = %s, len = %d, unicode = %d\n", music_file_name + 2, music_file_name_len - 2, unicode);
        /* put_buf(music_file_name, music_file_name_len + 2); */
        char *utf8 = zalloc(2 * name_buf_need_len);
        int file_len = Unicode2UTF8(utf8, (u16 *)music_file_name + 1, (music_file_name_len - 2) / 2);
        log_i(">>>[test]:filename = %s, len = %d\n", utf8, file_len);
        if (utf8) {
            free(utf8);
            utf8 = NULL;
        }
    } else {
        log_i("cur file  = %s, len = %d, unicode = %d\n", music_file_name, music_file_name_len, unicode);
    }
    log_i("\n");
    if (music_file_name) {
        free(music_file_name);
    }
    music_save_breakpoint(0);
    app_send_message2(APP_MSG_MUSIC_FILE_NUM_CHANGED, __this->player_hd->fsn->file_counter, __this->player_hd->fsn->file_number);
    app_send_message(APP_MSG_MUSIC_PLAY_SUCCESS, 0);



#if (TCFG_APP_MUSIC_EN && !RCSP_APP_MUSIC_EN)
    rcsp_device_status_update(MUSIC_FUNCTION_MASK,
                              BIT(MUSIC_INFO_ATTR_STATUS) | BIT(MUSIC_INFO_ATTR_FILE_NAME) | BIT(MUSIC_INFO_ATTR_FILE_PLAY_MODE));
#endif

}

static void send_music_player_end_msg(void *parm)
{
    __this->timer = 0;
    app_send_message(APP_MSG_MUSIC_PLAYER_END, (int)parm);
}

//*----------------------------------------------------------------------------*/
/**@brief    music 解码结束回调处理
   @param
   @return
   @note	此处统一将错误通过消息的方式发出， 在key msg中统一响应
*/
/*----------------------------------------------------------------------------*/
static void music_player_play_end(void *priv, int parm)
{
    if (parm) {
        if (__this->timer == 0) {
            __this->timer = sys_timeout_add((void *)parm, send_music_player_end_msg, 1000);
        }
    } else {
        app_send_message(APP_MSG_MUSIC_PLAYER_END, parm);
    }
}
//*----------------------------------------------------------------------------*/
/**@brief    music 解码错误回调
   @param
   @return
   @note	此处统一将错误通过消息的方式发出， 在key msg中统一响应
*/
/*----------------------------------------------------------------------------*/
static void music_player_decode_err(void *priv, int parm)
{
    log_i("music_player_decode_err\n");
    ///这里推出消息， 目的是在music主流程switch case统一入口
    /* app_task_put_key_msg(KEY_MUSIC_PLAYER_DEC_ERR, parm); */
    app_send_message(APP_MSG_MUSIC_DEC_ERR, parm);
}

//*----------------------------------------------------------------------------*/
/**@brief    music 播放器扫盘打断接口
   @param
   @return  1:打断当前扫描，0:正常扫描
   @note	设备扫描耗时长的情况下， 此接口提供打断机制
*/
/*----------------------------------------------------------------------------*/
static int music_player_scandisk_break(void)
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

static void scan_enter(struct __dev *dev)
{
    __this->scandisk_mark = 1;
}

static void scan_exit(struct __dev *dev)
{
    __this->scandisk_mark = 0;
}

static const struct __player_cb music_player_callback = {
    .start 		= music_player_play_success,
    .end   		= music_player_play_end,
    .err        = music_player_decode_err,
};

static const struct __scan_callback scan_cb = {
    .enter = scan_enter,
    .exit = scan_exit,
    .scan_break = music_player_scandisk_break,
};

static int app_music_init()
{
    int err = 0, ret = -1;
    __this->music_busy  = 0;
    music_idle_flag = 0;
    /* ui_update_status(STATUS_MUSIC_MODE); */

#if TCFG_MIC_EFFECT_ENABLE
    //切换mic eff irq points
#if TCFG_KBOX_1T3_MODE_EN && (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    if (mic_effect_player_runing()) {
        //根据情况关cis
#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
        y_printf("---> Connected Close in Other Mode()\n");
        app_connected_close_in_other_mode();
#endif
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
        y_printf("---> Broadcast Close in Other Mode()\n");
        app_broadcast_close_in_other_mode();
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
        y_printf("---> Auracast Close in Other Mode()\n");
        app_auracast_close_in_other_mode();
#endif
    }
#else
    tone_player_stop();
    mic_effect_player_pause(1);
    mic_effect_set_irq_point_unit(AUDIO_ADC_IRQ_POINTS_MUSIC_MODE);
    mic_effect_player_pause(0);
#endif
#endif

#if (TCFG_LRC_LYRICS_ENABLE)
    lyric_init();
#endif

    ///播放器初始化
    /* struct __player_parm parm = {0}; */
    /* parm.cb = &music_player_callback; */
    /* parm.scan_cb = &scan_cb; */
    /* music_player_creat(NULL, &parm); */
    __this->player_hd = music_player_creat();
    music_player_reg_scandisk_callback(__this->player_hd, &scan_cb);
    music_player_reg_dec_callback(__this->player_hd, &music_player_callback);
    ///获取断点句柄， 后面所有断点读/写都需要用到
    __this->breakpoint = breakpoint_handle_creat();
    ///初始化一些参数
    __this->file_err_counter = 0;
    __this->file_play_direct = 0;
    __this->scandisk_break = 0;
    memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
    char *logo = dev_manager_get_logo(dev_manager_find_active(1));
#if TCFG_LOCAL_TWS_ENABLE
#if (TCFG_MUSIC_DEVICE_TONE_EN)
    const char *file_name = get_tone_files()->music_mode;
    file_name = get_music_tone_name_by_logo(logo);
    ret = local_tws_enter_mode(file_name, NULL);
#else
    ret = local_tws_enter_mode(get_tone_files()->music_mode, NULL);
#endif
#endif //TCFG_LOCAL_TWS_ENABLE

    if (ret != 0) {
        tone_player_stop();
#if (TCFG_MUSIC_DEVICE_TONE_EN)
        if (music_device_tone_play(logo) == false) {
            ret = play_tone_file_callback(get_tone_files()->music_mode, NULL, music_tone_play_end_callback);
            if (ret) {
                music_tone_play_end_callback(NULL, STREAM_EVENT_NONE);
            }
        }
#else
        ret = play_tone_file_callback(get_tone_files()->music_mode, NULL, music_tone_play_end_callback);
        if (ret) {
            music_tone_play_end_callback(NULL, STREAM_EVENT_NONE);
        }
#endif
    }

#if LIB_SUPPORT_MULTI_SECTOER_READ
    os_mutex_create(&pre_read_mutex);
    if (!prebuf) {
        y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
        prebuf = zalloc(MAX_READ_LEN);
        ASSERT(prebuf);
        /* last_pos = 0; */
        /* PosInBuf = 0; */
    }
#endif

#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SOURCE_EN | LE_AUDIO_JL_UNICAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN))
    btstack_init_in_other_mode();
#if ((LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_JL_AURACAST_SOURCE_EN)) || \
    (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SINK_EN | LE_AUDIO_JL_AURACAST_SINK_EN))) && (LEA_BIG_FIX_ROLE==2)
    //当固定为接收端时，其它模式下开广播切进music模式，关闭广播后music模式不会自动播放
    music_set_broadcast_local_open_flag(1);
#endif
#endif

    app_send_message(APP_MSG_ENTER_MODE, APP_MODE_MUSIC);

    return 0;
}

void app_music_exit()
{
#if TCFG_LOCAL_TWS_ENABLE
    local_tws_exit_mode();
#endif
    music_save_breakpoint(1);
    music_player_stop(__this->player_hd, 1);
    breakpoint_handle_destroy(&__this->breakpoint);
    music_player_destroy(__this->player_hd);
    music_set_sd_keep_active(NULL);

    if (__this->timer) {
        sys_timeout_del(__this->timer);
        __this->timer = 0;
    }

    __this->player_hd = NULL;
    music_idle_flag = 1;

#if LIB_SUPPORT_MULTI_SECTOER_READ
    os_mutex_del(&pre_read_mutex, 0);
    if (prebuf) {
        y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
        free(prebuf);
        prebuf = NULL;
    }
#endif
#if TCFG_MIC_EFFECT_ENABLE
    //切换mic eff irq points
    mic_effect_player_pause(1);
    mic_effect_set_irq_point_unit(AUDIO_ADC_IRQ_POINTS);
    mic_effect_player_pause(0);
#endif

    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_MUSIC);
}


//*----------------------------------------------------------------------------*/
/**@brief    music 模式新设备上线处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void music_task_dev_online_start(char *in_logo)
{
    if (false == app_in_mode(APP_MODE_MUSIC)) {
        log_e("not music mode \n");
        return ;
    }
    __this->music_busy  = 1;
    u8 save = 0;
    char *logo = dev_manager_get_logo(__this->player_hd->dev);
    if (logo && __this->breakpoint) {
        ///新设备上线, 先记录当前设备断点， 然后播放活动设备
        if (music_player_get_playing_breakpoint(__this->player_hd, __this->breakpoint, 1) == true) {
            save = 1;
            //这里不要直接记忆断点， 解码停了之后再记忆
            //breakpoint_vm_write(__this->breakpoint, logo);
        }
    }
    ///停止解码，播放新活动设备
    music_player_stop(__this->player_hd, 1);
    if (save && __this->breakpoint) {
        breakpoint_vm_write(__this->breakpoint, logo);
    }
    __this->music_busy  = 0;
    app_send_message(APP_MSG_MUSIC_MOUNT_PLAY_START, (int)in_logo);
    log_i("APP_MSG_MUSIC_PLAY_START AFTER MOUNT\n");
    //先挂载了设备再执行
}


//*----------------------------------------------------------------------------*/
/**@brief    music 设备事件响应接口
   @param    无
   @return   1、当前消息已经处理，comomon不再做处理 0、common统一处理
   @note
*/
/*----------------------------------------------------------------------------*/
int music_device_msg_handler(int *msg)
{
    __this->music_busy  = 1;
    int err = 0;
    char *logo = NULL;
    char *evt_logo = NULL;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        evt_logo = (char *)msg[2];
    case DEVICE_EVENT_FROM_USB_HOST:
        if (!strncmp((char *)msg[2], "udisk", 5)) {
            evt_logo = (char *)msg[2];
        }
        int str_len   = 0;
        logo = (char *)music_player_get_phy_dev(__this->player_hd, &str_len);
        log_i("evt_logo =%s, logo = %s len =%d\n", evt_logo, logo, str_len);
        if (msg[1] == DEVICE_EVENT_OUT) {
            if (__this->timer) {
                sys_timeout_del(__this->timer);
                __this->timer = 0;
            }
            if (logo == NULL) {
                break;
            }
            if (0 == memcmp(logo, evt_logo, str_len)) {
                ///相同的设备才响应
                if (music_player_get_playing_breakpoint(__this->player_hd, __this->breakpoint, 1) == true) {
                    dev_manager_set_valid_by_logo(logo, 0);///尽快将设备设置为无效设备
                    breakpoint_vm_write(__this->breakpoint, logo);
                } else {
                    dev_manager_set_valid_by_logo(logo, 0);///将设备设置为无效设备
                }
                if (__this->scandisk_mark) {//自己打断自己的扫描推消息出去,重启解码
                    app_send_message(APP_MSG_MUSIC_PLAY_START, 0);
                    __this->scandisk_break = 1;
                }
                memset(__this->device_tone_dev, 0, sizeof(__this->device_tone_dev));
                if (music_player_runing()) {

                    ///停止解码,防止设备掉线后还继续使用
                    music_player_stop(__this->player_hd, 1);
                    ///重新选择活动设备播放
                    /* app_task_put_key_msg(KEY_MUSIC_PLAYER_START, 0);//卸载了设备再执行 */
                    app_send_message(APP_MSG_MUSIC_PLAY_START, 0);
                    log_i("KEY_MUSIC_PLAYER_START AFTER UMOUNT\n");
                } else {
                    if (dev_manager_get_total(1) == 0) {        //当前没有设备在线
                        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
                    }
                }
            } else {
                if (!dev_manager_check_by_logo(evt_logo)) { //未成功的插入，打断原来播放需恢复
                    if (!music_player_runing()) {
                        app_send_message(APP_MSG_MUSIC_PLAY_START, 0);
                    }
                }
            }
        } else {
            if (__this->scandisk_mark) {
                __this->scandisk_break = 1;
            }
#if (MUSIC_DEV_ONLINE_START_AFTER_MOUNT_EN == 0)
            music_task_dev_online_start(evt_logo);
#endif
        }
        break;
    default://switch((u32)event->arg)
        break;
    }

    __this->music_busy  = 0;
    return false;
}

struct app_mode *app_enter_music_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_music_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), music_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            music_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_music_exit();

    return next_mode;
}

static int music_mode_try_enter(int arg)
{

    if (dev_manager_get_total(1)) {
        return 0;
    }
    return -1;
}

static int music_mode_try_exit()
{
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN || LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
#if (LEA_BIG_CTRLER_TX_EN || LEA_BIG_CTRLER_RX_EN)
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE && !TCFG_KBOX_1T3_MODE_EN)
    app_broadcast_close_in_other_mode();
#endif
#endif

#if (LEA_CIG_CENTRAL_EN || LEA_CIG_PERIPHERAL_EN)
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE && !TCFG_KBOX_1T3_MODE_EN)
    app_connected_close_in_other_mode();
#endif
#endif

#if (!TCFG_KBOX_1T3_MODE_EN)
    btstack_exit_in_other_mode();
#endif
#endif

#if (TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_AURACAST_SOURCE_EN | LE_AUDIO_AURACAST_SINK_EN))
    le_audio_scene_deal(LE_AUDIO_APP_MODE_EXIT);
#if (!TCFG_BT_BACKGROUND_ENABLE)
    app_auracast_close_in_other_mode();
    btstack_exit_in_other_mode();
#endif
#endif

    return __this->music_busy;
}

static const struct app_mode_ops music_mode_ops = {
    .try_enter      = music_mode_try_enter,
    .try_exit       = music_mode_try_exit,
};

/*
 * 注册music模式
 */
REGISTER_APP_MODE(music_mode) = {
    .name 	= APP_MODE_MUSIC,
    .index  = APP_MODE_MUSIC_INDEX,
    .ops 	= &music_mode_ops,
};

void music_local_start(void *priv)
{
    if (priv) {
        app_send_message(APP_MSG_MUSIC_PLAY_START_BY_DEV, (int)priv);
    } else {
        music_player_play_start();
    }
}

static bool get_music_player_status(void)
{
    u8 paly_status = music_file_get_player_status(get_music_file_player());
    return (paly_status == FILE_PLAYER_START) ? TRUE : FALSE;
}

REGISTER_LOCAL_TWS_OPS(music) = {
    .name 	= APP_MODE_MUSIC,
    .local_audio_open = music_local_start,
    .get_play_status = get_music_player_status,
};


#endif

