#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_slience_detect.data.bss")
#pragma data_seg(".bt_slience_detect.data")
#pragma const_seg(".bt_slience_detect.text.const")
#pragma code_seg(".bt_slience_detect.text")
#endif
#include "system/includes.h"
#include "classic/tws_api.h"
#include "btstack/avctp_user.h"
#include "btstack/a2dp_media_codec.h"
#include "bt_slience_detect.h"
#include "bt_audio_energy_detection.h"

#include "app_config.h"
#include "app_main.h"

struct detect_handler {
    u8 codec_type;
    u8 unmute_packet_cnt;
    u8 energy_check_stop;
    u8 ingore_packet_num;
    u8 bt_addr[6];
    u16 ingore_to_seqn;
    u16 slience_timer;
    void *file;
};

static struct detect_handler *g_detect_hdl[2] = {NULL, NULL};

static struct detect_handler *get_detect_handler(u8 *bt_addr)
{
    for (int i = 0; i < 2; i++) {
        if (g_detect_hdl[i] && memcmp(g_detect_hdl[i]->bt_addr, bt_addr, 6) == 0) {
            return g_detect_hdl[i];
        }
    }
    return NULL;
}

static struct detect_handler *create_detect_handler()
{
    for (int i = 0; i < 2; i++) {
        if (!g_detect_hdl[i]) {
            g_detect_hdl[i] = zalloc(sizeof(struct detect_handler));
            return g_detect_hdl[i];
        }
    }
    return NULL;
}

static void close_energy_detect(u8 codec_type)
{
    for (int i = 0; i < 2; i++) {
        if (g_detect_hdl[i] && g_detect_hdl[i]->codec_type == codec_type) { //判断要关闭的类型是否还在使用
            return;
        }
    }
    bt_audio_energy_detect_close(codec_type);//关闭对应类型的能量检测
}

#if TCFG_USER_TWS_ENABLE

#define TWS_FUNC_ID_SYNC_OPEN_A2DP_PLAYER    TWS_FUNC_ID('A', '2', 'D', 'P')

#define TWS_MSG_SYNC_OPEN_A2DP_PLAYER         0x01

static void bt_sync_open_a2dp(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        if (data[0] == TWS_MSG_SYNC_OPEN_A2DP_PLAYER) {
            int msg[4];
            msg[0] = APP_MSG_BT_A2DP_START;
            memcpy(msg + 1, data + 1, 6);
            app_send_message_from(MSG_FROM_APP, 12, msg);
        }
    }
}

REGISTER_TWS_FUNC_STUB(sync_open_a2dp_stub) = {
    .func_id = TWS_FUNC_ID_SYNC_OPEN_A2DP_PLAYER,
    .func    = bt_sync_open_a2dp,
};
#endif


static void a2dp_slience_detect(void *_detect)
{
    int len;
    struct a2dp_media_frame frame;
    int seqn = -1;
    struct detect_handler *detect = (struct detect_handler *)_detect;

    if (tws_api_get_role() == TWS_ROLE_SLAVE) {
        return;
    }
    for (int i = 0; i < 2; i++) {
        if (detect == g_detect_hdl[i]) {
            goto __check;
        }
    }
    return;

__check:
    if (!detect->file) {
        return;
    }

    while (1) {
        len = a2dp_media_try_get_packet(detect->file, &frame);

        if (len <= 0) {
            break;
        }
        u8 *packet = frame.packet;

        seqn = (packet[2] << 8) | packet[3];

        /*
         * 不检测,一直丢包
         */
#if ((TCFG_A2DP_PREEMPTED_ENABLE == 0) && (TCFG_BT_DUAL_CONN_ENABLE))
#if (TCFG_BT_BACKGROUND_ENABLE)   //后台不走丢包流程
        if (bt_background_active() == false)
#endif
        {
            seqn += 10;
            if ((seqn & 0xffff) == 0) {
                seqn = 1;
            }
            a2dp_media_free_packet(detect->file, packet);
            break;
        }
#endif

        if (detect->ingore_to_seqn == 0) {
            detect->ingore_to_seqn = seqn + detect->ingore_packet_num;
            if (detect->ingore_to_seqn == 0) {
                detect->ingore_to_seqn = 1;
            }
            a2dp_media_free_packet(detect->file, packet);
            seqn = detect->ingore_to_seqn;
            break;
        }

        //能量检测
        int energy = 0;
        int unmute_packet_num = 0;
        if (detect->codec_type == A2DP_CODEC_SBC) {             //20ms
            energy = bt_audio_energy_detect_run(detect->codec_type, packet, len);
            unmute_packet_num = TCFG_BT_BACKGROUND_DETECT_TIME / 20;
        } else if (detect->codec_type == A2DP_CODEC_MPEG24) {   //25ms
            energy = bt_audio_energy_detect_run(detect->codec_type, packet, len);
            unmute_packet_num = TCFG_BT_BACKGROUND_DETECT_TIME / 25;
        } else if (detect->codec_type == A2DP_CODEC_LDAC) {
            energy = bt_audio_energy_detect_run(detect->codec_type, packet, len);
            unmute_packet_num = TCFG_BT_BACKGROUND_DETECT_TIME / 25;
        }

        printf("-energy: %d, %d, %d\n", seqn, energy, detect->unmute_packet_cnt);

        if (energy >= 10) {
            if (++detect->unmute_packet_cnt < unmute_packet_num) {
                a2dp_media_free_packet(detect->file, packet);
                continue;
            }
        } else {
            if (energy >= 0) {
                detect->unmute_packet_cnt >>= 1;
            }
            a2dp_media_free_packet(detect->file, packet);
            continue;
        }

        a2dp_media_free_packet(detect->file, packet);

        sys_timer_del(detect->slience_timer);
        detect->slience_timer = 0;

        seqn += 10;
        if ((seqn & 0xffff) == 0) {
            seqn = 1;
        }
        a2dp_media_clear_packet_before_seqn(detect->file, seqn);
        printf("slience_detect_over: clear_to_seqn: %d\n", seqn);

        a2dp_close_media_file(detect->file);
        detect->file = NULL;

        u8 codec_type = detect->codec_type;

        if (detect->codec_type == A2DP_CODEC_MPEG24) {
            detect->codec_type = 0xff;
        }
        close_energy_detect(codec_type);

        int msg[4];
        msg[0] = APP_MSG_BT_A2DP_START;
        memcpy(msg + 1, detect->bt_addr, 6);
        app_send_message_from(MSG_FROM_APP, 12, msg);
#if TCFG_USER_TWS_ENABLE
        u8 data[7];
        data[0] = TWS_MSG_SYNC_OPEN_A2DP_PLAYER;
        memcpy(data + 1, detect->bt_addr, 6);
        tws_api_send_data_to_slave(&data, sizeof(data), TWS_FUNC_ID_SYNC_OPEN_A2DP_PLAYER);
#endif
        return;
    }

    if (seqn > 0) {
        a2dp_media_clear_packet_before_seqn(detect->file, seqn);
    }
}

void bt_start_a2dp_slience_detect(u8 *bt_addr, int ingore_packet_num)
{
    void *file = a2dp_open_media_file(bt_addr);
    if (!file) {
        puts("open_a2dp_file_faild\n");
        return;
    }

    struct detect_handler *detect = get_detect_handler(bt_addr);
    if (!detect) {
        detect = create_detect_handler();
        if (!detect) {
            a2dp_close_media_file(file);
            return;
        }
    }
    if (detect->slience_timer) {
        sys_timer_del(detect->slience_timer);
    }

    detect->file = file;
    detect->codec_type = a2dp_media_get_codec_type(detect->file);

    detect->ingore_packet_num = ingore_packet_num;
    detect->ingore_to_seqn = 0;
    detect->unmute_packet_cnt = 0;
    detect->energy_check_stop = 0;
    memcpy(detect->bt_addr, bt_addr, 6);

    detect->slience_timer = sys_timer_add(detect, a2dp_slience_detect, 80);
    g_printf("bt_start_a2dp_slience_detect:");
    put_buf(bt_addr,  6);
}

void bt_stop_a2dp_slience_detect(u8 *bt_addr)
{
    struct detect_handler *detect;
    u8 codec_type = 0;

    for (int i = 0; i < 2; i++) {
        detect = g_detect_hdl[i];
        if (!detect) {
            continue;
        }
        if (bt_addr && memcmp(detect->bt_addr, bt_addr, 6)) {
            continue;
        }

        codec_type = g_detect_hdl[i]->codec_type;

        g_detect_hdl[i] = NULL;

        if (detect->slience_timer) {
            sys_timer_del(detect->slience_timer);
            detect->slience_timer = 0;
            g_printf("bt_stop_a2dp_slience_detect");
        }
        if (detect->file) {
            a2dp_close_media_file(detect->file);
            detect->file = NULL;
        }

        free(detect);
        detect = NULL;
    }

    close_energy_detect(codec_type);
}

void bt_reset_a2dp_slience_detect()
{
    struct detect_handler *detect;
    for (int i = 0; i < 2; i++) {
        detect = g_detect_hdl[i];
        if (!detect || detect->slience_timer == 0) {
            return;
        }

        detect->ingore_to_seqn = 0;
        detect->unmute_packet_cnt = 0;
        detect->energy_check_stop = 0;
    }
}

int bt_slience_detect_get_result(u8 *bt_addr)
{
    struct detect_handler *detect = get_detect_handler(bt_addr);
    if (!detect) {
        return BT_SLIENCE_NO_DETECTING;
    }
    if (detect->unmute_packet_cnt) {
        return BT_SLIENCE_HAVE_ENERGY;
    }
    return BT_SLIENCE_NO_ENERGY;
}

int bt_slience_get_detect_addr(u8 *bt_addr)
{
    struct detect_handler *detect;

    for (int i = 0; i < 2; i++) {
        detect = g_detect_hdl[i];
        if (!detect) {
            continue;
        }
        memcpy(bt_addr, detect->bt_addr, 6);
        return 1;
    }
    return 0;
}

