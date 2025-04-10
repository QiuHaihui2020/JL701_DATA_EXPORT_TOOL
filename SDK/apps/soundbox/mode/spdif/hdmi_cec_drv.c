#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".hdmi_cec_drv.data.bss")
#pragma data_seg(".hdmi_cec_drv.data")
#pragma const_seg(".hdmi_cec_drv.text.const")
#pragma code_seg(".hdmi_cec_drv.text")
#endif
#include "hdmi_cec.h"
#include "app_config.h"
#include "clock.h"
#include "app_config.h"
#include "gpio.h"
#include "system/timer.h"
#include "gptimer.h"
#include "hdmi_cec_api.h"
#include "app_main.h"
#include "asm/sfc_norflash_api.h"
#include "soundbox.h"
#include "spdif_file.h"

#if (TCFG_SPDIF_ENABLE)

struct hdmi_arc_t {
    u16 tid;
    u16 time_id;
    u16 cec_port;
    u8 state;
    u8 logic_addr;
    u8 rx_count;
    u8 rx_msg;
    u8 idle_time;
    u8 cec_ok;
    u8 eom;
    u8 follower ;

    u8 msg[17];
    u8 cec_det_en;
    u8 tv_offline;
    u8 no_dcc_phyaddr;
    u8 req_tv_venderID;
};

static struct hdmi_arc_t arc_devcie;
#define __this  (&arc_devcie)

static void cec_set_wait_time(u32 us);

static void cec_set_line_state(const u8 state)
{
    gpio_set_mode(IO_PORT_SPILT(__this->cec_port), state);
}

static u32 cec_get_line_state()
{
    return gpio_read(__this->cec_port);
}

static u8 CEC_Version_cmd[3] = {0x50, CECOP_CEC_VERSION, 0x04}; //0x04 :1.3a ????
static u8 Report_Power_Status_Cmd[3] = {0x00, 0x90, 0x00}; //0x00:ON ; 0x01:OFF
static u8 Standby_Cmd[2] = {0x00, CECOP_STANDBY};
static u8 PollingMessage_cmd[1] = {0x55};

static u8 volume_cmd[] = {0x50, CECOP_REPORT_AUDIO_STATUS, 0x64};

static u8 Report_Physical_Add_Cmd[7] = {0x5F, CECOP_REPORT_PHYSICAL_ADDRESS, 0x10, 0x00, 0x05};

static u8 Vendor_Cmd_With_ID[8] = {0x5f, CECOP_VENDOR_COMMAND_WITH_ID, 0x08, 0x00, 0x46, 0x00, 0x0a, 0x00};
static u8 Give_Device_Vendor_Id_Cmd[2] = {0x50, CECOP_GIVE_DEVICE_VENDOR_ID};
static u8 Device_Vendor_Id_Cmd[5] = {0x5f, CECOP_DEVICE_VENDOR_ID, 0x08, 0x00, 0x46};
static u8 Set_System_Audio_Mode_cmd[] = {0x50, CECOP_SET_SYSTEM_AUDIO_MODE, 0x01};
static u8 Broadcast_System_Audio_Mode_cmd[] = {0x5F, CECOP_SET_SYSTEM_AUDIO_MODE, 0x01};
static u8 Broadcast_System_Audio_Mode_Off[] = {0x5F, CECOP_SET_SYSTEM_AUDIO_MODE, 0x00};
static u8 Set_Osd_Name_cmd[] = {0x50, CECOP_SET_OSD_NAME, 'J', 'i', 'e', 'l', 'i', 'C', 'E', 'C', '-', '0', '1'};
static u8 System_Audio_Mode_Status_cmd[] = {0x50, CECOP_SYSTEM_AUDIO_MODE_STATUS, 0x01};

static u8 Report_Audio_Status_cmd[] = {0x50, CECOP_REPORT_AUDIO_STATUS, 0x32};

static u8 Report_Power_Status_cmd[] = {0x50, CECOP_REPORT_POWER_STATUS, 0x00};// 0:on 1 standby 2: S2O 3: o2s

static u8 request_active_source[2] = {0x5f, CECOP_REQUEST_ACTIVE_SOURCE};
static u8 Initiate_Arc_cmd[2] = {0x50, CECOP_ARC_INITIATE};
static u8 request_Initiate_Arc_cmd[2] = {0x50, CECOP_ARC_REQUEST_INITIATION};
/* static u8 Report_Short_Audio_Desc_cmd[8] = {0x50, CECOP_REPORT_SHORT_AUDIO_DESCRIPTOR, 0x15, 0x17, 0x50, 0x3E, 0x06, 0xC0}; //AC-3 6Ch (48 44 32k) 640kbps/ DTS 7Ch (48 44K)  1536kbps */
static u8 Report_Short_Audio_Desc_cmd[] = {0x50, CECOP_REPORT_SHORT_AUDIO_DESCRIPTOR, 0x15, 0x07, 0x50}; //AC-3 6Ch (48 44 32k) 640kbps/ DTS 7Ch (48 44K)  1536kbps

static u8 Give_Tuner_Device_Status_cmd[3] = {0x50, CECOP_GIVE_TUNER_DEVICE_STATUS, 0x01};
static u8 Feature_Abort_cmd[3] = {0x50, CECOP_FEATURE_ABORT, 0x00};
static u8 request_Terminate_Arc_cmd[2] = {0x50, CECOP_ARC_REQUEST_TERMINATION};
static u8 Terminate_Arc_cmd[2] = {0x50, CECOP_ARC_TERMINATE};

#define     CEC_IDLE    0
#define     CEC_TX      1
#define     CEC_RX      2

static u8 cec_work_mode;
static s8 cec_state;
static u16 high_period_time;
enum {
    CEC_SUCC,
    NAK_HEAD,
    NAK_DATA,
};

struct cec_msg_frame {
    u8 *msg;
    u8 msg_len;
    u8 tx_count;
    u8 ack;
    u8 broadcast;
};

static struct cec_msg_frame cec_frame;


typedef enum {
    CEC_XMIT_STARTBIT,
    CEC_XMIT_BIT7,
    CEC_XMIT_BIT6,
    CEC_XMIT_BIT5,
    CEC_XMIT_BIT4,
    CEC_XMIT_BIT3,
    CEC_XMIT_BIT2,
    CEC_XMIT_BIT1,
    CEC_XMIT_BIT0,
    CEC_XMIT_BIT_EOM,
    CEC_XMIT_BIT_ACK,
    CEC_XMIT_ACK_PU,
    CEC_XMIT_CHECK_ACK,
    CEC_XMIT_ACK_END,

    CEC_WAIT_IDLE,
    CEC_RECEIVE,
    CEC_CONTINUE_BIT,
} CEC_XMIT_STATE ;
#define     T_OFFSET    50
void cec_io_toogle(u32 cnt)
{
    gpio_toggle_port(PORTA, PORT_PIN_6);
}
static void cec_start_bit()
{
    cec_state = CEC_XMIT_STARTBIT;

    cec_set_line_state(0);
    const u16 low_time = 3700 - T_OFFSET;
    const u16 high_time = 4500 - T_OFFSET - low_time;
    cec_set_wait_time(low_time);
    high_period_time = high_time;

}
static void cec_logic_bit0()
{
    const u16 low_time = 1500 - T_OFFSET;
    const u16 high_time = 2400 - T_OFFSET - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void cec_logic_bit1()
{
    const u16 low_time = 600 - T_OFFSET;
    const u16 high_time = 2400 - T_OFFSET - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void cec_end_bit()
{
    const u16 low_time = 600 - T_OFFSET;
    cec_set_line_state(0);
    cec_set_wait_time(low_time);
}
static void cec_ack_bit()
{

    const u16 low_time = 1500 - T_OFFSET;
    const u16 high_time = 1800 - T_OFFSET - low_time;
    cec_set_wait_time(low_time);
    cec_set_line_state(0);
    high_period_time = high_time;
}
static void transmit_complete(u32 state)
{

    gpio_set_mode(IO_PORT_SPILT(__this->cec_port), PORT_INPUT_PULLUP_10K);
    cec_work_mode = CEC_IDLE;
    cec_frame.ack = state;
    cec_state = CEC_RECEIVE;
}
static void tx_bit_isr()
{
    if (high_period_time) {
        cec_set_line_state(1);
        cec_set_wait_time(high_period_time);
        high_period_time = 0;
        cec_state++;
        return;
    }
    switch (cec_state) {
    case CEC_WAIT_IDLE:
        cec_start_bit();
        break;

    case CEC_XMIT_STARTBIT:
        break;

    case CEC_XMIT_ACK_END:
        if (cec_frame.ack) {
            cec_set_line_state(1);
            if (cec_frame.tx_count == cec_frame.msg_len) {
                transmit_complete(CEC_SUCC);
                break;
            } else {
                cec_state = CEC_XMIT_BIT7;
            }
        } else {
            if (cec_frame.tx_count == 1) {
                transmit_complete(NAK_HEAD);
            } else {
                transmit_complete(NAK_DATA);
            }
            break;
        }
    case CEC_XMIT_BIT7 ... CEC_XMIT_BIT0:
        if (cec_frame.msg[cec_frame.tx_count] & BIT(7 - (cec_state - CEC_XMIT_BIT7))) {
            cec_logic_bit1();
        } else {
            cec_logic_bit0();
        }
        break;

    case CEC_XMIT_BIT_EOM:
        cec_frame.tx_count++;
        if (cec_frame.tx_count == cec_frame.msg_len) {
            cec_logic_bit1();//eom=y
        } else {
            cec_logic_bit0();//eom=n
        }
        break;

    case CEC_XMIT_BIT_ACK:
        cec_state++;
        cec_end_bit();
        break;

    case CEC_XMIT_ACK_PU:
        gpio_set_mode(IO_PORT_SPILT(__this->cec_port), PORT_INPUT_PULLUP_10K);
        /* cec_set_wait_time(850 - 450); */
        /* cec_set_wait_time(1000 - 450); */
        cec_set_wait_time(1000 - 600);
        cec_state++;
        break;

    case CEC_XMIT_CHECK_ACK:
        cec_frame.ack = cec_frame.broadcast ? 1 : !cec_get_line_state();
        cec_state++;
        /* cec_set_wait_time(2250 - 850); */
        cec_set_wait_time(2400 - 1000);
        break;
    }

}

//New initiator wants to send a frame wait time
#define     _ms *1//000
#define     CEC_NEW_FRAME_WAIT_TIME            	(5*(2.4f _ms))
#define     CEC_RETRANSMIT_FRAME                (3*(2.4f _ms))

static void cec_set_wait_time(u32 us)
{
    gptimer_set_work_mode(__this->tid, GPTIMER_MODE_TIMER);
    gptimer_set_timer_period(__this->tid, us);
}
static u32 cec_bit_time_check(u16 t, u16 min, u16 max)
{
    if ((t > min) && (t < max)) {
        return 1;
    } else {
        return 0;
    }
}
static s8 cec_get_bit_value(const u16 low_time, const u16 high_time)
{
    if (cec_bit_time_check(low_time + high_time, 2050, 2750)) {
        if (cec_bit_time_check(low_time, 400, 800)) {
            return 1;
        } else if (cec_bit_time_check(low_time, 1300, 1700)) {
            return 0;
        }
    }
    return -1;
}

static void cec_cmd_response();
static void rx_bit_isr()
{
    static u16 low_time = 0;
    static u16 high_time = 0;

    enum gptimer_mode mode = gptimer_get_work_mode(__this->tid);

    if (high_period_time) {
        cec_set_line_state(1);
        cec_set_wait_time(high_period_time);
        high_period_time = 0;
        cec_state = CEC_XMIT_ACK_END;
        return;
    }

    if (cec_state == CEC_XMIT_ACK_END) {
        gpio_set_mode(IO_PORT_SPILT(__this->cec_port), PORT_INPUT_PULLUP_10K);
        __this->rx_count ++;
        if (__this->eom) {
            __this->rx_msg = 2;
        } else {
            cec_state = CEC_CONTINUE_BIT;
        }

        gptimer_set_capture_edge_type(__this->tid, GPTIMER_MODE_CAPTURE_EDGE_FALL);
        return;
    }

    /* void timer_hw_set_capture_count(int cnt); */
    u32 time = gptimer_get_capture_cnt2us(__this->tid);

    /* timer_hw_set_capture_count(0); */
    if (mode ==  GPTIMER_MODE_CAPTURE_EDGE_RISE) {
        low_time = time;
        time = 0;
        mode = GPTIMER_MODE_CAPTURE_EDGE_FALL;
    } else {
        mode = GPTIMER_MODE_CAPTURE_EDGE_RISE;
    }

    gptimer_set_capture_edge_type(__this->tid, mode);


    if (time == 0) {

        return;
    }

    if (cec_state == CEC_RECEIVE) {
        cec_state = CEC_XMIT_STARTBIT;
        low_time = 0;
        high_time = 0;
        return;
    }

    s8 bit;

    high_time = time;

    /* printf("T:%d %d", low_time, high_time); */

    switch (cec_state) {
    case CEC_XMIT_STARTBIT:
        if (low_time && high_time) {
            if (cec_bit_time_check(low_time, 3500, 3900) &&
                cec_bit_time_check(low_time + high_time, 4300, 4700)) {
                cec_state ++;
            }
        }

        __this->follower = 0;
        __this->rx_count = 0;
        low_time = 0;
        high_time = 0;
        break;

    case CEC_CONTINUE_BIT:
        cec_state = CEC_XMIT_BIT7;
        break;

    case CEC_XMIT_BIT7 ... CEC_XMIT_BIT0:
        bit = cec_get_bit_value(low_time, high_time);
        if (bit == 0) {
            __this->msg[__this->rx_count] &= ~BIT(7 - (cec_state - CEC_XMIT_BIT7));
//            cec_state++;
        } else if (bit == 1) {
            __this->msg[__this->rx_count] |= BIT(7 - (cec_state - CEC_XMIT_BIT7));
//            cec_state++;
        } else if (bit == -1) { //error bit
            cec_state = CEC_XMIT_STARTBIT;
        }
        cec_state++;
        break;

    case CEC_XMIT_BIT_EOM:
        cec_state++;
        bit = cec_get_bit_value(low_time, high_time);
        if (bit == 0) {
            __this->eom = 0;
        } else if (bit == 1) {
            __this->eom = 1;
        } else if (bit == -1) { //error bit
            cec_state = CEC_XMIT_STARTBIT;
        }
        if (__this->rx_count == 0) { //head block
            __this->follower = __this->msg[0] & 0xf;
        }

        if (__this->follower == __this->logic_addr) {
            cec_ack_bit();//send ack
        } else {
            /* printf(" %d %d %d", __this->rx_count, __this->follower, __this->logic_addr); */
            __this->rx_count = 0;
        }
        break;

    case CEC_XMIT_BIT_ACK:
        cec_state = CEC_XMIT_BIT7;
        break;
    }

}
#define     TIME_PRD    50
static void cec_timer_isr(u32 tid, void *private_data)
{
    if (get_sfc_status()) {
        return;
    }
    if (cec_work_mode == CEC_TX) {
        tx_bit_isr();
        if (cec_work_mode == CEC_IDLE) {
            gptimer_set_capture_edge_type(tid, GPTIMER_MODE_CAPTURE_EDGE_FALL);
        }
    } else {

        if (__this->idle_time < (150 / TIME_PRD)) {
            __this->idle_time = 150 / TIME_PRD;
        }

        rx_bit_isr();
    }

}


u32 cec_transmit_frame(unsigned char *buffer, int count)
{
    cec_frame.ack = -1;
    if (cec_work_mode != CEC_IDLE) {
        printf("cec busy");
        return 1;
    }
//    put_buf(buffer, count);
    __this->idle_time = 1;
    cec_work_mode = CEC_TX;
    cec_frame.msg = buffer;
    cec_frame.msg_len = count;
    cec_frame.tx_count = 0;
    cec_frame.broadcast = 0;

    if ((buffer[0] & 0xf) == CEC_LOGADDR_UNREGORBC) {
        cec_frame.broadcast = 1;
    }

    if ((buffer[0] & 0xf) == __this->logic_addr) {
        cec_frame.broadcast = 1;
    }

    gpio_set_mode(IO_PORT_SPILT(__this->cec_port), PORT_INPUT_PULLUP_10K);

    udelay(10);
    if (cec_get_line_state()) {
        cec_start_bit();
    } else {
        high_period_time = 0;
        cec_state = CEC_WAIT_IDLE;
        gptimer_set_capture_edge_type(__this->tid, GPTIMER_MODE_CAPTURE_EDGE_RISE);
        return 1;
    }

    return 0;
}
typedef enum {
    ARC_STATUS_PING,
    ARC_STATUS_PING_END = 1,
    ARC_STATUS_REPORT_ADDR,
    ARC_STATUS_REPORT_VENDOR_ID,
    ARC_STATUS_REPORT_POWER_STATUS,
    ARC_REQUEST_INITIATION,
    ARC_STATUS_SET_AUDIO_MODE,
    ARC_REQUEST_ACTUVE_SOURCE,
    ARC_STATUS_WAIT_TV,
    ARC_STATUS_INIT_SUCCESS,
} ARC_STATUS;

static void decode_cec_command()
{
    u8 intiator = __this->msg[0] >> 4;
    switch (__this->msg[1]) {
    case CECOP_FEATURE_ABORT:
        printf("__this->state < ARC_STATUS_INIT=0x%x\n", __this->state);
        put_buf(__this->msg, __this->rx_count);
        break;
    case CECOP_GIVE_PHYSICAL_ADDRESS:
        printf("CECOP_GIVE_PHYSICAL_ADDRESS");
        cec_transmit_frame(Report_Physical_Add_Cmd, 5);
        break;
    case CECOP_GIVE_DEVICE_VENDOR_ID:
        printf("CECOP_GIVE_DEVICE_VENDOR_ID");
        spdif_hdmi_set_push_data_en(1);
        cec_transmit_frame(Device_Vendor_Id_Cmd, 5);
        break;
    case CECOP_GET_CEC_VERSION:
        printf("GET_CEC_VERSION");
        cec_transmit_frame(CEC_Version_cmd, 3);
        break;
    case CECOP_ARC_REPORT_INITIATED:
        printf("CECOP_ARC_REPORT_INITIATED");
        spdif_hdmi_set_push_data_en(1);
        __this->cec_ok = 1;
        __this->state = ARC_STATUS_INIT_SUCCESS;
        __this->idle_time = 500 / TIME_PRD;
        if (false == app_in_mode(APP_MODE_SPDIF)) {
            break;
        }
        app_send_message(APP_MSG_CEC_MUTE, 0);
        break;

    case CECOP_SET_STREAM_PATH:
        printf("CECOP_SET_STREAM_PATH");
        break;
    case CECOP_GIVE_OSD_NAME:
        printf("CECOP_SET_OSD_NAME");
        cec_transmit_frame(Set_Osd_Name_cmd, sizeof(Set_Osd_Name_cmd));
        break;

    case CECOP_REQUEST_SHORT_AUDIO_DESCRIPTOR:
        printf("CECOP_REQUEST_SHORT_AUDIO");
        cec_transmit_frame(Report_Short_Audio_Desc_cmd, sizeof(Report_Short_Audio_Desc_cmd));
        __this->idle_time = 3500 / TIME_PRD;
        break;

    case CECOP_SYSTEM_AUDIO_MODE_REQUEST:
        printf("CECOP_SYSTEM_AUDIO_MODE_REQUEST");
        cec_transmit_frame(Set_System_Audio_Mode_cmd, sizeof(Set_System_Audio_Mode_cmd));
        /* __this->state =  ARC_STATUS_REPORT_ADDR; */
        break;
    case CECOP_GIVE_AUDIO_STATUS:
        printf("CECOP_GIVE_AUDIO_STATUS");
        cec_transmit_frame(Report_Audio_Status_cmd, sizeof(Report_Audio_Status_cmd));
        break;

    case CECOP_GIVE_SYSTEM_AUDIO_MODE_STATUS:
        printf("CECOP_GIVE_SYSTEM_AUDIO_MODE_STATUS");
        cec_transmit_frame(System_Audio_Mode_Status_cmd, sizeof(System_Audio_Mode_Status_cmd));
        break;
    case CECOP_TUNER_DEVICE_STATUS:
        printf("CECOP_TUNER_DEVICE_STATUS");
        cec_transmit_frame(Give_Tuner_Device_Status_cmd, sizeof(Give_Tuner_Device_Status_cmd) / sizeof(Give_Tuner_Device_Status_cmd[0]));
        break;
    case CECOP_GIVE_DEVICE_POWER_STATUS:
        printf("CECOP_GIVE_DEVICE_POWER_STATUS");
        cec_transmit_frame(Report_Power_Status_cmd, sizeof(Report_Power_Status_cmd));
        break;
    case CECOP_ARC_REQUEST_INITIATION:
        printf("CECOP_ARC_REQUEST_INITIATION");
        __this->cec_ok = 1;
        spdif_hdmi_set_push_data_en(1);
        cec_transmit_frame(Initiate_Arc_cmd, 2);
        __this->req_tv_venderID = 2;
        __this->idle_time = 3500 / TIME_PRD;
        if (true == app_in_mode(APP_MODE_SPDIF)) {
            app_send_message(APP_MSG_CEC_MUTE, 0);
        }
        if (__this->cec_det_en) {
            if (true != app_in_mode(APP_MODE_SPDIF)) {
                /*一些情况不希望退出蓝牙模式*/
#if TCFG_APP_BT_EN
                if (bt_app_exit_check() == 0) {
                    puts("bt_background can not enter\n");
                    break;
                }
#endif
                app_send_message2(APP_MSG_GOTO_MODE, APP_MODE_SPDIF, 0);
            }
        }
        break;
    case CECOP_ARC_REQUEST_TERMINATION:
        printf("CECOP_ARC_REQUEST_TERMINATION");
        //电视关机会发送该命令
        cec_transmit_frame(Terminate_Arc_cmd, sizeof(Terminate_Arc_cmd));
        __this->tv_offline = 4;
        __this->cec_ok = 0;
        __this->state = ARC_STATUS_PING;
        if (false == app_in_mode(APP_MODE_SPDIF)) {
            break;
        }
        app_send_message(APP_MSG_CEC_MUTE, 1);
        break;
    case CECOP_ARC_REPORT_TERMINATED:
        printf("CECOP_ARC_REPORT_TERMINATED");
        break;
    case CECOP_STANDBY:
        //部分电视关机会发送该命令  audio sys 可以软关机处理
        break;
    case CECOP_USER_CONTROL_PRESSED:
        if (false == app_in_mode(APP_MODE_SPDIF)) {
            break;
        }
        switch (__this->msg[2]) {
        case CEC_USER_CONTROL_CODE_MUTE:
            printf("mute press");
            app_send_message(APP_MSG_CEC_MUTE, 2);
            break;
        case CEC_USER_CONTROL_CODE_VOLUME_UP:
            app_send_message(APP_MSG_VOL_UP, 0);
            printf("volume up press");
            break;
        case CEC_USER_CONTROL_CODE_VOLUME_DOWN:
            app_send_message(APP_MSG_VOL_DOWN, 0);
            printf("volume down press");
            break;
        case CEC_USER_CONTROL_CODE_POWER:
            puts("CEC_USER_CONTROL_CODE_POWER");
            break;
        default:
            printf("CEC_USER_CONTROL_CODE_[%d] \n", __this->msg[2]);
            break;
        }
        break;
    case CECOP_USER_CONTROL_RELEASED:
        printf("key release");
        break;
    default:
        printf("unknow cec command ");
        put_buf(__this->msg, __this->rx_count);
        break;
    }
}
static void cec_cmd_response()
{
    if (__this->rx_count > 1) {
        if (__this->tv_offline) {
            __this->tv_offline--;
        }
        decode_cec_command();
    }

//    putchar('1');
    if (__this->idle_time < (150 / TIME_PRD)) {
        __this->idle_time = 150 / TIME_PRD;
    }

    if (cec_work_mode == CEC_IDLE) {
        cec_state = CEC_RECEIVE;
    }
    __this->rx_msg = 0;
}
static void cec_timer_loop()
{
    extern u8 bt_trim_status_get();
    static u8 bt_tram_mark = 0;
    if (bt_trim_status_get()) {
        bt_tram_mark = 1;
        sys_timer_modify(__this->time_id, 2000);//bt trim 延后检测
        return;
    }
    if (bt_tram_mark) {
        bt_tram_mark = 0;
        sys_timer_modify(__this->time_id, TIME_PRD);//bt trim 延后检测
    }

    if (__this->rx_msg == 2) {
        usr_timeout_add(NULL, cec_cmd_response, CEC_NEW_FRAME_WAIT_TIME, 0);
        __this->rx_msg = 1;
    }

    if (__this->rx_msg) {
        return;
    }

    if (cec_work_mode) {
        return;
    }

    if (__this->idle_time) {
        __this->idle_time--;
        return;
    }
    if (__this->tv_offline) {
        return;
    }
    if (__this->cec_ok) {
        if (__this->req_tv_venderID) {
            __this->req_tv_venderID--;
            if (__this->req_tv_venderID) {
                cec_transmit_frame(Give_Device_Vendor_Id_Cmd, 2);
            } else {
                cec_transmit_frame(Vendor_Cmd_With_ID, sizeof(Vendor_Cmd_With_ID) / sizeof(Vendor_Cmd_With_ID[0]));
            }
            __this->idle_time = 300 / TIME_PRD;
        } else {
            __this->idle_time = 3500 / TIME_PRD;
        }
        return;
    }
    if (cec_frame.ack != CEC_SUCC) {
        r_printf("send cec msg nak %d [%d]", cec_frame.ack, __this->state);
    }
    /* printf("cec state [%d] %d", __this->state, cec_frame.ack); */
    switch (__this->state) {
    case ARC_STATUS_PING ... ARC_STATUS_PING_END:
        __this->cec_ok = 0;
        __this->idle_time = 100 / TIME_PRD;
        cec_transmit_frame(PollingMessage_cmd, 1);
        __this->state ++;
        break;

    case ARC_STATUS_REPORT_ADDR:
        __this->idle_time = 300 / TIME_PRD;
        cec_transmit_frame(Report_Physical_Add_Cmd, 5);
        __this->state ++;
        break;

    case ARC_STATUS_REPORT_VENDOR_ID:
        /* __this->idle_time = 150 / TIME_PRD; */
        __this->idle_time = 500 / TIME_PRD;
        cec_transmit_frame(Device_Vendor_Id_Cmd, 5);
        /* __this->state ++; */
        __this->state = ARC_STATUS_WAIT_TV;
        break;

    case ARC_STATUS_REPORT_POWER_STATUS:
        __this->idle_time = 150 / TIME_PRD;
        cec_transmit_frame(Report_Power_Status_cmd, sizeof(Report_Power_Status_cmd));
        __this->state ++;
        break;
    case ARC_REQUEST_INITIATION:
        __this->idle_time = 150 / TIME_PRD;
        cec_transmit_frame(request_Initiate_Arc_cmd, 2);
        __this->state ++;
        break;
    case ARC_STATUS_SET_AUDIO_MODE:
        __this->idle_time = 150 / TIME_PRD;
        cec_transmit_frame(Broadcast_System_Audio_Mode_cmd, 3);
        __this->state ++;
        break;
    case ARC_REQUEST_ACTUVE_SOURCE:
        __this->idle_time = 150 / TIME_PRD;
        cec_transmit_frame(request_active_source, 2);
        __this->state ++;
        break;
    case ARC_STATUS_WAIT_TV:
        if (__this->no_dcc_phyaddr == 0) {
            __this->idle_time = 3500 / TIME_PRD;
            break;
        }
        static u8 wait_cnt = 0;
        wait_cnt++;
        if (wait_cnt >= 30) {
            __this->state = 	ARC_STATUS_PING;
            wait_cnt = 0;
            puts("\n reset cec state \n");
            u8 port_id = Report_Physical_Add_Cmd[2] >> 4;
            if (port_id < 3) {
                port_id++;
            } else {
                port_id = 1;
            }
            Report_Physical_Add_Cmd[2] = port_id << 4;
            printf("phy addr [%x]", Report_Physical_Add_Cmd[2]);
        }
        __this->idle_time = 600 / TIME_PRD;
        break;
    }
}
#define CEC_SET_VOLUME_IGNORD_TIME  1200
int hdmi_cec_send_volume(u32 vol)
{
    int msec;
    u8 mute = 0;
    static  u32 start_time = 0;
    if (vol > 100) {
        vol = 100;
    }
    mute = get_sys_aduio_mute_statu();
    volume_cmd[2] = vol | mute << 7;
    if (__this->time_id && __this->tv_offline == 0) {
        if (start_time == 0) {
            start_time = jiffies_msec();
            msec = CEC_SET_VOLUME_IGNORD_TIME;
        } else {
            msec = jiffies_msec2offset(start_time, jiffies_msec());
        }

        if (msec >= CEC_SET_VOLUME_IGNORD_TIME) {
            start_time = jiffies_msec();
            cec_transmit_frame(volume_cmd, 3);
        }
    }
    return 0;
}
void hdmi_cec_init(u8 cec_port, u8 cec_det_en)
{
    memset(__this, 0, sizeof(*__this));
    u32 hdmi_ddc_get_cec_physical_address(void);
    u16 addr = hdmi_ddc_get_cec_physical_address();

    if (addr) {
        Report_Physical_Add_Cmd[2] = addr >> 8;
        Report_Physical_Add_Cmd[3] = addr;
    } else {
        __this->no_dcc_phyaddr = 1;
    }

    /* JL_PORTA->DIR &= ~BIT(13); */
    __this->cec_det_en = cec_det_en;
    __this->logic_addr = 5;
    cec_state = CEC_RECEIVE;
    cec_work_mode = CEC_IDLE;
    __this->cec_ok = 0;
    __this->cec_port = cec_port;
    __this->tid = -1;
    gpio_set_mode(IO_PORT_SPILT(__this->cec_port), PORT_INPUT_PULLUP_10K);

    __this->idle_time = 3500 / TIME_PRD;

    struct gptimer_config cec_cfg = {
        .capture.port = __this->cec_port / 16,
        .capture.pin = BIT(__this->cec_port % 16),
        .mode = GPTIMER_MODE_CAPTURE_EDGE_RISE,
        .irq_cb = cec_timer_isr,
        .irq_priority = 7, //中断优先级默认1
    };

    __this->tid = gptimer_init(TIMERx, &cec_cfg);
    gptimer_start(__this->tid);

    __this->time_id = sys_timer_add(NULL, cec_timer_loop, TIME_PRD);
    g_printf("cec add timer %x %x %x", __this->time_id, __this->tid, __this->cec_port);
}

void hdmi_cec_close(void)
{
    g_printf("cec del timer %x %x", __this->time_id, __this->tid);
    if (__this->time_id) {

        cec_transmit_frame(Broadcast_System_Audio_Mode_Off, 3);
        os_time_dly(10);

        gptimer_set_work_mode(__this->tid, GPTIMER_MODE_CAPTURE_EDGE_RISE);
        gptimer_deinit(__this->tid);
        __this->tid = -1;
        sys_timer_del(__this->time_id);
        __this->time_id = 0;
    }

}


#endif

