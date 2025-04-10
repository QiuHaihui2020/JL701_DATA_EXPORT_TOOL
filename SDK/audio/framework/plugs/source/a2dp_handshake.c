#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".a2dp_handshake.data.bss")
#pragma data_seg(".a2dp_handshake.data")
#pragma const_seg(".a2dp_handshake.text.const")
#pragma code_seg(".a2dp_handshake.text")
#endif
/*************************************************************************************************/
/*!
*  \file      a2dp_handshake.c
*
*  \brief
*
*  Copyright (c) 2011-2022 ZhuHai Jieli Technology Co.,Ltd.
*
*/
/*************************************************************************************************/
#if 1
struct a2dp_handshake_data {
    u8 ack;
    u8 state;
    u16 seqn;
    u8 addr[6];
};
struct a2dp_tws_handshake {
    u8 state;
    u8 handshake_step;
    u8 rx_state;
    u8 need_ack;
    u32 rx_time;
    u16 rx_seqn;
    u16 run_seqn;
    u8 rx_addr[6];
    u8 run_addr[6];
};

/*
 * handshake state
 * init / run / success / timeout
 * */
#define A2DP_HANDSHAKE_STATE_INIT           0
#define A2DP_HANDSHAKE_STATE_RUN            1
#define A2DP_HANDSHAKE_STATE_SUCCESS        2
#define A2DP_HANDSHAKE_STATE_TIMEOUT        3
#define A2DP_HANDSHAKE_STATE_WAIT_MONITOR   4

/*
 * handshake step
 * init / request / wait ack / timeout / ack
 *
 */
#define A2DP_HANDSHAKE_STEP_INIT            0
#define A2DP_HANDSHAKE_STEP_REQUEST         1
#define A2DP_HANDSHAKE_STEP_WAIT_ACK        2
#define A2DP_HANDSHAKE_STEP_TIMEOUT         3
#define A2DP_HANDSHAKE_STEP_ACK             4

static struct a2dp_tws_handshake a2dp_tws = {0};

#define A2DP_TWS_HANDSHAKE \
	((int)((u8 )('A' + '2' + 'D' + 'P') << (3 * 8)) | \
	 (int)(('T' + 'W' + 'S') << (2 * 8)) | \
	 (int)(('H' + 'A' + 'N' + 'D') << (1 * 8)) | \
     (int)(('S' + 'H' + 'A' + 'K' + 'E') << (0 * 8)))

#define A2DP_TWS_FILE_EXIT          0
#define A2DP_TWS_FILE_START         1
#define A2DP_TWS_FILE_RUN           2

extern u32 bt_audio_conn_clock_time(void *addr);

static void a2dp_tws_handskake_handler(void *buf, u16 len, bool rx)
{
    struct a2dp_handshake_data handshake_data;

    if (!CONFIG_BTCTLER_TWS_ENABLE) {
        return;
    }

    if (rx) {
        memcpy(&handshake_data, buf, len);
        /*putchar(handshake_data.ack ? '*' : '#');*/
        if (handshake_data.ack) {
            if (a2dp_tws.handshake_step != A2DP_HANDSHAKE_STEP_INIT && a2dp_tws.handshake_step != A2DP_HANDSHAKE_STEP_ACK) {
                a2dp_tws.handshake_step = A2DP_HANDSHAKE_STEP_ACK;
                a2dp_tws.rx_time = jiffies;
                a2dp_tws.rx_seqn = handshake_data.seqn;
                a2dp_tws.rx_state = handshake_data.state;
                memcpy(a2dp_tws.rx_addr, handshake_data.addr, sizeof(handshake_data.addr));
            }
        } else {
            a2dp_tws.need_ack = 1;
        }
    }
}

REGISTER_TWS_FUNC_STUB(a2dp_tws_handshake) = {
    .func_id = A2DP_TWS_HANDSHAKE,
    .func    = a2dp_tws_handskake_handler,
};

#define bt_clkn_after(a, b)     (((a - b) & 0x7ffffff) >= 0)

static void a2dp_file_abandon_data_before_time(void *file, u32 time)
{
    struct a2dp_media_frame frame;

    while (1) {
        int len = a2dp_media_try_get_packet(file, &frame);
        if (len <= 0) {
            break;
        }
        if (bt_clkn_after(frame.clkn, time)) {
            a2dp_media_put_packet(file, frame.packet);
            break;
        }
        printf("drop clkn : %d\n", frame.clkn);
        a2dp_media_free_packet(file, frame.packet);
    }
}

static void a2dp_file_abandon_data_before_seqn(void *file, u16 ref_seqn)
{
    u16 seqn;
    struct a2dp_media_frame frame;

    while (1) {
        int len = a2dp_media_try_get_packet(file, &frame);
        if (len <= 0) {
            break;
        }
        seqn = RB16((u8 *)frame.packet + 2);
        if (seqn == ref_seqn || seqn_after(seqn, ref_seqn)) {
            a2dp_media_put_packet(file, frame.packet);
            break;
        }
        printf("drop seqn : %d\n", seqn);
        a2dp_media_free_packet(file, frame.packet);
    }

}

static int a2dp_tws_media_pop_seqn(struct a2dp_file_hdl *hdl, u16 *seqn)
{
    struct a2dp_media_frame frame;
    int error_num = 0;
    int len = 0;
    while (len <= 0) {
        if (++error_num > 3) {
            break;
        }
        len = a2dp_media_try_get_packet(hdl->file, &frame);
    }

    if (len <= 0) {
        return -EINVAL;
    }
    *seqn = RB16(frame.packet + 2);

    a2dp_media_put_packet(hdl->file, frame.packet);
    return 0;
}

static void a2dp_tws_media_handshake_ack(u8 file_running)
{
    if (!a2dp_tws.need_ack) {
        return;
    }

    struct a2dp_handshake_data handshake_data = {0};
    /*收到握手消息则ack当前信息*/
    handshake_data.ack = 1;
    handshake_data.state = file_running ? a2dp_tws.state : A2DP_TWS_FILE_START;
    handshake_data.seqn = a2dp_tws.run_seqn;
    memcpy(handshake_data.addr, a2dp_tws.run_addr, sizeof(a2dp_tws.run_addr));
    int err = tws_api_send_data_to_sibling(&handshake_data, sizeof(handshake_data), A2DP_TWS_HANDSHAKE);
    if (!err) {
        a2dp_tws.need_ack = 0;
    }
}

static void a2dp_tws_media_try_handshake_ack(u8 file_running, u16 seqn)
{
    if (!CONFIG_BTCTLER_TWS_ENABLE) {
        return;
    }

    if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) ||      /*TWS未连接*/
        !(tws_api_get_lmp_state(a2dp_tws.run_addr) & TWS_STA_MONITOR_START)) {       /*TWS播放另外一边还未开始监听*/
        return;
    }

    a2dp_tws.run_seqn = seqn;
    a2dp_tws_media_handshake_ack(file_running);
}

static int a2dp_tws_media_handshake_init(struct a2dp_file_hdl *hdl)
{
    if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_INIT) {
        u16 seqn = 0;
        a2dp_file_abandon_data_before_time(hdl->file, bt_audio_conn_clock_time(hdl->bt_addr) - 200);
        int err = a2dp_tws_media_pop_seqn(hdl, &seqn);
        if (err) {
            return -EINVAL;
        }

        local_irq_disable();
        memset(&a2dp_tws, 0x0, sizeof(a2dp_tws));
        a2dp_tws.state = A2DP_TWS_FILE_START;
        a2dp_tws.run_seqn = seqn;
        memcpy(a2dp_tws.run_addr, hdl->bt_addr, sizeof(a2dp_tws.run_addr));
        local_irq_enable();
        hdl->handshake_state = A2DP_HANDSHAKE_STATE_RUN;
        hdl->handshake_timeout = jiffies + msecs_to_jiffies(300);
    }

    return 0;
}

static int a2dp_tws_conn_and_monitor_detect(struct a2dp_file_hdl *hdl)
{
    if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        return TWS_STA_SIBLING_DISCONNECTED;
    }

    if (!(tws_api_get_lmp_state(hdl->bt_addr) & TWS_STA_MONITOR_START)) {       /*TWS播放另外一边还未开始监听*/
        if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_INIT) {
            hdl->handshake_state = A2DP_HANDSHAKE_STATE_WAIT_MONITOR;
            hdl->handshake_timeout = jiffies + msecs_to_jiffies(200);
            return TWS_STA_MONITOR_ING;
        }

        if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_WAIT_MONITOR) {
            if (time_before(jiffies, hdl->handshake_timeout)) {
                return TWS_STA_MONITOR_ING;
            }
        }
        hdl->handshake_state = A2DP_HANDSHAKE_STATE_INIT;
        return TWS_STA_SIBLING_DISCONNECTED;
    }

    if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_WAIT_MONITOR) {
        hdl->handshake_state = A2DP_HANDSHAKE_STATE_INIT;
    }
    return 0;
}

static int a2dp_tws_media_handshake(struct a2dp_file_hdl *hdl)
{
    int err = 0;

    if (!CONFIG_BTCTLER_TWS_ENABLE) {
        return 0;
    }

    err = a2dp_tws_conn_and_monitor_detect(hdl);
    if (err) {
        if (err == TWS_STA_SIBLING_DISCONNECTED) {
            memset(&a2dp_tws, 0x0, sizeof(a2dp_tws));
            if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_INIT) {
                memcpy(a2dp_tws.run_addr, hdl->bt_addr, sizeof(a2dp_tws.run_addr));
            }
            hdl->tws_case = 1;
            goto a2dp_file_run;
        }
        return -EINVAL;
    }

    err = a2dp_tws_media_handshake_init(hdl);
    if (err) {
        return err;
    }

    struct a2dp_handshake_data handshake_data = {0};
    /*握手请求*/
    printf("[handshake] : %d, %d\n", hdl->handshake_state, a2dp_tws.handshake_step);
    switch (a2dp_tws.handshake_step) {
    case A2DP_HANDSHAKE_STEP_INIT:
    case A2DP_HANDSHAKE_STEP_REQUEST:
        local_irq_disable();
        hdl->request_timeout = jiffies + msecs_to_jiffies(60);
        err = tws_api_send_data_to_sibling(&handshake_data, sizeof(handshake_data.ack), A2DP_TWS_HANDSHAKE);
        a2dp_tws.handshake_step = err ? A2DP_HANDSHAKE_STEP_REQUEST : A2DP_HANDSHAKE_STEP_WAIT_ACK;
        local_irq_enable();
        break;
    case A2DP_HANDSHAKE_STEP_WAIT_ACK:
        local_irq_disable();
        if (time_after(jiffies, hdl->request_timeout)) {
            /*request后响应超时*/
            a2dp_tws.handshake_step = A2DP_HANDSHAKE_STEP_REQUEST;
        }
        local_irq_enable();
        break;
    case A2DP_HANDSHAKE_STEP_ACK:
        hdl->handshake_state = A2DP_HANDSHAKE_STATE_SUCCESS;
        break;
    default:
        break;
    }
    a2dp_tws_media_handshake_ack(0);

    if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_RUN) {
        if (time_before(jiffies, hdl->handshake_timeout)) { //等待握手未超时，以失败返回
            return -EINVAL;
        }
        hdl->handshake_state = A2DP_HANDSHAKE_STATE_TIMEOUT;
    }

    if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_TIMEOUT) {
        /*握手超时，这时候需要对超时数据进行处理*/
        printf("a2dp handshake timeout : %d\n", bt_audio_conn_clock_time(hdl->bt_addr));
        a2dp_file_abandon_data_before_time(hdl->file, bt_audio_conn_clock_time(hdl->bt_addr) - 200);
        hdl->tws_case = 1;
        goto a2dp_file_run;
    }

    if (memcmp(a2dp_tws.rx_addr, hdl->bt_addr, 6) != 0) {
        printf("a2dp handshake bt addr error : \n");
        put_buf(a2dp_tws.rx_addr, 6);
        put_buf(hdl->bt_addr, 6);
        hdl->tws_case = 1;
        goto a2dp_file_run;
    }

    if (__builtin_abs((int)(a2dp_tws.rx_seqn - a2dp_tws.run_seqn)) > 50) {
        printf("a2dp handshake seqn error : %d, %d, need check btstack.\n", a2dp_tws.rx_seqn, a2dp_tws.run_seqn);
        hdl->tws_case = 1;
        goto a2dp_file_run;
    }

    /*将两边数据处理为一致的seqn*/
    a2dp_file_abandon_data_before_seqn(hdl->file, a2dp_tws.rx_seqn);

a2dp_file_run:
    local_irq_disable();
    if (hdl->handshake_state == A2DP_HANDSHAKE_STATE_SUCCESS && a2dp_tws.rx_state == A2DP_TWS_FILE_RUN) {
        printf("------>>> A2DP player join tws <<<--------\n");
        hdl->tws_case = 2;
    }
    a2dp_tws.state = A2DP_TWS_FILE_RUN;
    a2dp_tws.handshake_step = A2DP_HANDSHAKE_STATE_INIT;
    local_irq_enable();
    return 0;
}

void a2dp_tws_media_handshake_exit(struct a2dp_file_hdl *hdl)
{
    if (!CONFIG_BTCTLER_TWS_ENABLE) {
        return;
    }
    local_irq_disable();
    a2dp_tws.state = A2DP_TWS_FILE_EXIT;
    hdl->handshake_state = A2DP_HANDSHAKE_STATE_INIT;
    local_irq_enable();
}
#endif
