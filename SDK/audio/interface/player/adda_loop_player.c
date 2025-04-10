#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adda_loop_player.data.bss")
#pragma data_seg(".adda_loop_player.data")
#pragma const_seg(".adda_loop_player.text.const")
#pragma code_seg(".adda_loop_player.text")
#endif
#include "jlstream.h"
#include "adda_loop_player.h"
#include "app_config.h"
#include "effects/audio_pitchspeed.h"
#include "audio_config_def.h"
#include "effects/audio_vbass.h"
#include "adc_file.h"
#include "effects/effects_adj.h"
struct adda_loop_player {
    struct jlstream *stream;
    int channel;
};

static struct adda_loop_player *g_adda_loop_player = NULL;

static void adda_loop_player_callback(void *private_data, int event)
{
    struct adda_loop_player *player = g_adda_loop_player;
    struct jlstream *stream = (struct jlstream *)private_data;
    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int adda_loop_dut_switch_channel(void)
{
    if (g_adda_loop_player) {
        g_adda_loop_player->channel = (g_adda_loop_player->channel == AUDIO_CH_L ? AUDIO_CH_R : AUDIO_CH_L);
        int err = jlstream_ioctl(g_adda_loop_player->stream, NODE_IOC_SET_CHANNEL, g_adda_loop_player->channel);
        /* printf("l:%x,%d\n",channel,err); */
        return err;
    } else {
        return -EFAULT;
    }
}

static int adda_loop_get_adc_num(void)
{
    u8 ch_num = 0;
    struct adc_file_cfg temp_cfg = {0};
    char mode_index = 0;
    char cfg_index = 0;//目标配置项序号
    struct cfg_info info = {0};
    int err = jlstream_read_form_node_info_base(mode_index, "adda_adc", cfg_index, &info);
    if (err == 0) {
        jlstream_read_form_cfg_data(&info, &temp_cfg);
    } else {
        printf("adda_adc_file read cfg data err !!!\n");
    }
    for (int i = 0; i < AUDIO_ADC_MIC_MAX_NUM; i++) {
        if (temp_cfg.mic_en_map & BIT(i)) {
            ch_num ++;
        }
    }
    return ch_num;
}


int adda_loop_player_open()
{
    int err;
    struct adda_loop_player *player;
    if (g_adda_loop_player) {
        return 0;
    }
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"adda_loop");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = malloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ADC);

    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

    //设置中断点数
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);


    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_NODE_CONFIG, adda_loop_get_adc_num());

    jlstream_node_ioctl(player->stream, NODE_UUID_VOCAL_TRACK_SYNTHESIS, NODE_IOC_SET_PRIV_FMT, AUDIO_ADC_IRQ_POINTS);//四声道时，指定声道合并单个声道的点数

    player->channel = AUDIO_CH_L;
    jlstream_ioctl(player->stream, NODE_IOC_SET_CHANNEL, player->channel);
    jlstream_set_callback(player->stream, player->stream, adda_loop_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_ADDA_LOOP);
    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }
    g_adda_loop_player = player;
    return 0;
__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool adda_loop_player_runing()
{
    return g_adda_loop_player != NULL;
}


void adda_loop_player_close()
{
    struct adda_loop_player *player = g_adda_loop_player;

    if (!player) {
        return;
    }
    jlstream_stop(player->stream, 50);
    jlstream_release(player->stream);

    free(player);
    g_adda_loop_player = NULL;

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"adda_loop");
}


