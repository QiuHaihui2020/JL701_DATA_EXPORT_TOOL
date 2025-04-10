#include "typedef.h"
#include "app_main.h"
#include "app_config.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "app_tone.h"
#include "volume_node.h"
#include "mix_record_api.h"
#include "system/includes.h"
#include "app_action.h"
#include "default_event_handler.h"
#include "soundbox.h"
#include "tone_player.h"
#include "dev_manager.h"
#include "file_recorder.h"

#if TCFG_MIX_RECORD_ENABLE



struct mix_recode_handle {
    void *recorder;
    FILE *old_file;
    FILE *new_file;
    char logo[20];
    int status;
    const char *suffix;
};

static struct mix_recode_handle g_rec_hdl = {
    .status = 0,
};

int mix_record_device_msg_handler(int *msg)
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
            if (g_rec_hdl.recorder) {
                if (!strcmp(g_rec_hdl.logo, logo)) {
                    mix_recorder_stop();
                }
            }
        }
        break;
    default:
        break;
    }
    return false;
}


static FILE *mix_recorder_open_file()
{
    char folder[] = {TCFG_REC_FOLDER_NAME};         //录音文件夹名称
    char filename[] = {TCFG_REC_FILE_NAME};     //录音文件名，不需要加后缀

#if (TCFG_NOR_REC)
    char logo[] = {"rec_nor"};		//外挂flash录音
#elif (FLASH_INSIDE_REC_ENABLE)
    char logo[] = {"rec_sdfile"};		//内置flash录音
#else
    char *logo = dev_manager_get_phy_logo(dev_manager_find_active(0));//普通设备录音，获取最后活动设备
#endif

    snprintf(g_rec_hdl.logo, sizeof(g_rec_hdl.logo), "%s", logo);

    char *root_path = dev_manager_get_root_path_by_logo(logo);
    char path[64] = {};

    snprintf(path, sizeof(path), "%s%s/%s.%s", root_path, folder, filename, g_rec_hdl.suffix);

    FILE *file = fopen(path, "w+");
    if (!file) {
        printf("recorder_open_file_faild: %s\n", path);
    } else {
        fget_name(file, (u8 *)path, 64);
        printf("recorder_open_file_suss: %s\n", path);
    }
    return file;
}

static void file_ops_in_app_task(void *p)
{
    switch ((int)p) {
    case SEAMLESS_OPEN_FILE:
        g_rec_hdl.new_file = mix_recorder_open_file();
        break;
    case SEAMLESS_CHANGE_FILE:
        if (g_rec_hdl.old_file) {
            fclose(g_rec_hdl.old_file);
            g_rec_hdl.old_file = NULL;
        }
        break;
    case -ENOENT:
        mix_recorder_stop();
        break;
    }
}

/*
 * 此回调函数在音频流任务中调用,不能执行耗时长的操作,否则可能导致音频播放卡顿
 */
static int change_file_in_stream_task(void *priv, enum change_file_step step)
{
    int msg[4] = { (int)file_ops_in_app_task, 1, step};

    if (step == SEAMLESS_CHANGE_FILE) {
        g_rec_hdl.old_file = file_recorder_change_file(g_rec_hdl.recorder, g_rec_hdl.new_file);
        if (g_rec_hdl.new_file) {
            g_rec_hdl.new_file = NULL;
        } else {
            msg[2] = -ENOENT;
        }
    }
    return os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
}


void mix_recorder_start()
{
    int err;
    void *recorder;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mix_recorder");

    recorder = file_recorder_open(uuid, NODE_UUID_ZERO_ACTIVE);
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

    switch (fmt.coding_type) {
    case AUDIO_CODING_PCM:
    case AUDIO_CODING_WAV:
        g_rec_hdl.suffix = "wav";
        break;
    case AUDIO_CODING_MP3:
        g_rec_hdl.suffix = "mp3";
        break;
    default:
        g_rec_hdl.suffix = "bin";
        break;
    }
    FILE *file = mix_recorder_open_file();
    if (!file) {
        file_recorder_close(recorder, 0);
        return;
    }
    file_recorder_set_file(recorder, file, NULL);

    struct seamless_recording seamless = {
        .advance_time = 10, //提前10s创建新文件
        .time = 3600,       //单个文件录音时长
        .priv = recorder,
        .change_file = change_file_in_stream_task,
    };
    file_recorder_seamless_set(recorder, &seamless);

    err = file_recorder_start(recorder);
    if (err) {
        printf("file_recorder_err: %d\n", err);
        file_recorder_close(recorder, 1);
        return;
    }
    g_rec_hdl.status = 1;
    g_rec_hdl.recorder = recorder;
    mem_stats();
}

void mix_recorder_stop()
{
    // 先停掉录音，会有淡出操作
    file_recorder_stop(g_rec_hdl.recorder, 1);
    // 再关闭音源
    g_rec_hdl.status = 0;
    os_time_dly(1);

    if (g_rec_hdl.old_file) {
        fclose(g_rec_hdl.old_file);
        g_rec_hdl.old_file = NULL;
    }
    if (g_rec_hdl.new_file) {
        fdelete(g_rec_hdl.new_file);
        g_rec_hdl.new_file = NULL;
    }
    file_recorder_release(g_rec_hdl.recorder);
    g_rec_hdl.recorder = NULL;
}

int  get_mix_recorder_status(void)
{
    return g_rec_hdl.status ? 1 : 0;
}

#endif // TCFG_MIX_RECORD_ENABLE

