#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".record.data.bss")
#pragma data_seg(".record.data")
#pragma const_seg(".record.text.const")
#pragma code_seg(".record.text")
#endif
#include "system/includes.h"
#include "app_action.h"
#include "app_main.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "app_tone.h"
#include "dev_manager.h"
#include "record.h"
#include "file_recorder.h"


#if TCFG_APP_RECORD_EN


#define PIPELINE_RECODER    0x49EC


struct app_recode_handle {
    void *recorder;
    FILE *file;
    char logo[20];
};

static struct app_recode_handle g_rec_hdl;
static u8 record_idle_flag = 1;

//*----------------------------------------------------------------------------*/
/**@brief    录音模式提示音结束处理
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int record_tone_play_end_callback(void *priv, enum stream_event event)
{
    return 0;

}
//*----------------------------------------------------------------------------*/
/**@brief    record 模式初始化
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static int app_record_init()
{
    printf("\n--------record  start-----------\n");
    record_idle_flag = 0;
    tone_player_stop();
    play_tone_file_callback(get_tone_files()->record_mode, NULL,
                            record_tone_play_end_callback);
    return 0;
}
//*----------------------------------------------------------------------------*/
/**@brief    record 退出
   @param    无
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

void app_record_exit()
{
    //停止播放模式提示音
    ///停止mic录音
    /* record_mic_stop(); */
    ///停止回放
    /* record_file_close(); */
    record_idle_flag = 1;
    app_send_message(APP_MSG_EXIT_MODE, APP_MODE_RECORD);
}


static void app_recorder_callback(void *priv, enum stream_state state)
{

}

static void app_recorder_start()
{
    int err;
    void *recorder;

    recorder = file_recorder_open(PIPELINE_RECODER, NODE_UUID_ADC);
    /* recorder = file_recorder_open(PIPELINE_RECODER, NODE_UUID_ZERO_ACTIVE); */
    if (!recorder) {
        puts("file_recorder_open_faild\n");
        return;
    }

    struct stream_enc_fmt fmt;
    err = file_recorder_get_fmt(recorder, &fmt);
    if (err) {
        file_recorder_close(recorder, 0);
        return;
    }
    printf("recorder_fmt: ch %d, sample rate %d\n", fmt.channel, fmt.sample_rate);

    const char *suffix;
    switch (fmt.coding_type) {
    case AUDIO_CODING_PCM:
    case AUDIO_CODING_WAV:
        suffix = "wav";
        break;
    case AUDIO_CODING_MP3:
        suffix = "mp3";
        break;
    default:
        suffix = "bin";
        break;
    }
    char folder[] = {TCFG_REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {TCFG_REC_FILE_NAME};     //录音文件名，不需要加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
#endif

    char *root_path = dev_manager_get_root_path_by_logo(logo);
    char path[64] = {};

    snprintf(path, sizeof(path), "%s%s/%s.%s", root_path, folder, filename, suffix);
    FILE *file = file_recorder_open_file(recorder, path);
    if (!file) {
        file_recorder_close(recorder, 0);
        return;
    }
    file_recorder_set_callback(recorder, NULL, app_recorder_callback);

    err = file_recorder_start(recorder);
    if (err) {
        printf("file_recorder_err: %d\n", err);
        file_recorder_close(recorder, 1);
        return;
    }

    snprintf(g_rec_hdl.logo, sizeof(g_rec_hdl.logo), "%s", logo);
    g_rec_hdl.file      = file;
    g_rec_hdl.recorder   = recorder;
}

static void app_recorder_stop()
{
    file_recorder_close(g_rec_hdl.recorder, 1);
    g_rec_hdl.recorder = NULL;
}

static void recorder_device_offline_check(const char *logo)
{
    if (g_rec_hdl.recorder) {
        if (!strcmp(g_rec_hdl.logo, logo)) {
            ///当前录音正在使用的设备掉线， 应该停掉录音
            printf("is the recording dev = %s\n", logo);
            /* app_recorder_stop(); */
            file_recorder_close(g_rec_hdl.recorder, 0);
            g_rec_hdl.recorder = NULL;
        }
    }
}

int record_device_msg_handler(int *msg)
{
    const char *logo = NULL;
    int err = 0;
    switch (msg[0]) {
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
        logo = (char *)msg[2];
    case DEVICE_EVENT_FROM_USB_HOST:
        if (!strncmp((char *)msg[2], "udisk", 5)) {
            logo = (char *)msg[2];
        }

        if (msg[1] == DEVICE_EVENT_IN) {
        } else if (msg[1] == DEVICE_EVENT_OUT) {
            recorder_device_offline_check(logo);
        }
        break;
    default:
        break;
    }
    return false;
}

int record_app_msg_handler(int *msg)
{
    switch (msg[0]) {
    case APP_MSG_CHANGE_MODE:
        printf("app msg key change mode\n");
        app_send_message(APP_MSG_GOTO_NEXT_MODE, 0);
        break;
    case APP_MSG_MUSIC_PP:
        printf("app msg record pp\n");
        break;
    case APP_MSG_MUSIC_NEXT:
        printf("app msg record next\n");
        break;
    case APP_MSG_MUSIC_PREV:
        printf("app msg record prev\n");
        break;
    case APP_MSG_REC_PP:
        if (!g_rec_hdl.recorder) {
            app_recorder_start();
        } else {
            app_recorder_stop();
        }
        break;
    default:
        app_common_key_msg_handler(msg);
        break;
    }

    return 0;
}

struct app_mode *app_enter_record_mode(int arg)
{
    int msg[16];
    struct app_mode *next_mode;

    app_record_init();

    while (1) {
        if (!app_get_message(msg, ARRAY_SIZE(msg), record_mode_key_table)) {
            continue;
        }
        next_mode = app_mode_switch_handler(msg);
        if (next_mode) {
            break;
        }

        switch (msg[0]) {
        case MSG_FROM_APP:
            record_app_msg_handler(msg + 1);
            break;
        case MSG_FROM_DEVICE:
            break;
        }

        app_default_msg_handler(msg);
    }

    app_record_exit();

    return next_mode;
}

static int record_mode_try_enter()
{
    if (dev_manager_get_total(0)) {
        return 0;
    }
    return -1;
}

static int record_mode_try_exit()
{
    return 0;
}

static const struct app_mode_ops record_mode_ops = {
    .try_enter      = record_mode_try_enter,
    .try_exit       = record_mode_try_exit,
};

/*
 * 注册record模式
 */
REGISTER_APP_MODE(record_mode) = {
    .name 	= APP_MODE_RECORD,
    .index  = APP_MODE_RECORD_INDEX,
    .ops 	= &record_mode_ops,
};


static u8 record_idle_query(void)
{
    return record_idle_flag;
}

REGISTER_LP_TARGET(record_lp_target) = {
    .name = "record",
    .is_idle = record_idle_query,
};

#endif

