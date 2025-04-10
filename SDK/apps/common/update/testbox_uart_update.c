#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".testbox_uart_update.data.bss")
#pragma data_seg(".testbox_uart_update.data")
#pragma const_seg(".testbox_uart_update.text.const")
#pragma code_seg(".testbox_uart_update.text")
#endif
#include "init.h"
#include "app_config.h"
#include "system/includes.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "app_charge.h"
#include "user_cfg.h"
#include "app_chargestore.h"
#include "app_testbox.h"
#include "device/vm.h"
#include "btstack/avctp_user.h"
#include "app_action.h"
#include "app_main.h"
#include "app_charge.h"
#include "classic/tws_api.h"
#include "update.h"
#include "bt_ble.h"
#include "bt_tws.h"
#include "app_action.h"
#include "app_power_manage.h"
#include "bt_common.h"
/* #include "uart_update.h" */
#include "update_interactive_uart.h"
#include "update/update_loader_download.h"

#define LOG_TAG "[TEST-UART-UPDATE]"
#define LOG_INFO_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"


#define FIXED_LOADER_BAUDRATE		(100000)
#define RETRY_TIME          4//重试n次

#define PACKET_TIMEOUT      200//ms

//命令
#define CMD_UPDATE_START    0x01
#define CMD_UPDATE_READ     0x02
#define CMD_UPDATE_END      0x03
#define CMD_SEND_UPDATE_LEN 0x04
#define CMD_KEEP_ALIVE      0x05

#define TESTBOX_UART_UPDATE_FLAG          0xAB539610
#define SDK_JUMP_FLAG                     "SDKJUMP"
typedef struct testbox_uart_update_info {
    u32 baud;
    u32 uart_update_flag;
    u8  io_port[6];
} TESTBOX_UP_UART;

extern void chargestore_set_baudrate(u32 baudrate);
/* extern void ldo5v_change_off_filter(u32 inc); */
static void app_testbox_loader_update_recv(u8 cmd, u8 *buf, u32 len);

_WEAK_
void  ldo5v_change_off_filter(u32 inc)
{
}

extern const char updata_file_name[];

static protocal_frame_t protocal_frame __attribute__((aligned(4)));
static OS_SEM loader_sem;
static u32 uart_file_offset = 0;
static u32 update_baudrate = 0;

static bool app_testbox_loader_send_packet(u8 *buf, u16 length)
{
    bool ret = TRUE;
    u16 crc;
    u8 *buffer;

    buffer = (u8 *)&protocal_frame;
    protocal_frame.data.mark0 = SYNC_MARK0;
    protocal_frame.data.mark1 = SYNC_MARK1;
    protocal_frame.data.length = length;
    memcpy((char *)&buffer[4], buf, length);
    crc = CRC16(buffer, length + SYNC_SIZE - 2);
    memcpy(buffer + 4 + length, &crc, 2);
    /* put_buf((u8 *)&protocal_frame, length + SYNC_SIZE); */
    os_sem_set(&loader_sem, 0);

    ldo5v_change_off_filter(100);

    loader_uart_write((u8 *)&protocal_frame, length + SYNC_SIZE);
    chargestore_api_wait_complete();
    /* chargestore_api_set_mode(0);  // 设置为接收 */
    if (OS_TIMEOUT == os_sem_pend(&loader_sem, 35)) {
        puts("loader send packet timeout err\n");
        ret = FALSE;
        goto __exit;
    }
__exit:
    ldo5v_change_off_filter(0);
    return ret;
}

static u32 app_testbox_loader_receive_data(void *buf, u32 relen, u32 addr)
{
    u8 i;
    struct file_info file_cmd;
    for (i = 0; i < RETRY_TIME; i++) {
        if (i > 0) {
            putchar('r');
        }
        file_cmd.cmd = CMD_UPDATE_READ;
        file_cmd.addr = addr;
        file_cmd.len = relen;
        if (app_testbox_loader_send_packet((u8 *)&file_cmd, sizeof(file_cmd)) == FALSE) {
            continue;
        }
        memcpy(&file_cmd, protocal_frame.data.data, sizeof(file_cmd));
        if ((file_cmd.cmd != CMD_UPDATE_READ) || (file_cmd.addr != addr) || (file_cmd.len != relen)) {
            continue;
        }
        memcpy(buf, &protocal_frame.data.data[sizeof(file_cmd)], protocal_frame.data.length - sizeof(file_cmd));
        return (protocal_frame.data.length - sizeof(file_cmd));
    }
    putchar('R');
    return relen;
}

static bool app_testbox_loader_send_cmd(u8 cmd, u8 *buf, u32 len)
{
    u8 *pbuf, i;
    for (i = 0; i < RETRY_TIME; i++) {
        pbuf = protocal_frame.data.data;
        pbuf[0] = cmd;
        memcpy(pbuf + 1, buf, len);
        if (app_testbox_loader_send_packet(pbuf, len + 1) == FALSE) {
            continue;
        }
        if (cmd == protocal_frame.data.data[0]) {
            app_testbox_loader_update_recv(cmd, &protocal_frame.data.data[1], protocal_frame.data.length - 1);
            return TRUE;
        }
    }
    putchar('F');
    return FALSE;
}


static u16 uart_f_open(void)
{
    return 1;
}

static u16 uart_f_read(void *handle, u8 *buf, u16 relen)
{
    u32 len;
    /* printf("%s\n", __func__); */
    len = app_testbox_loader_receive_data(buf, relen, uart_file_offset);
    if (len == -1) {
        log_info("uart_f_read err\n");
        return -1;
    }
    uart_file_offset += len;
    return len;
}

static int uart_f_seek(void *fp, u8 type, u32 offset)
{
    if (type == SEEK_SET) {
        uart_file_offset = offset;
    } else if (type == SEEK_CUR) {
        uart_file_offset += offset;
    }
    return 0;//FR_OK;
}

static u16 uart_f_stop(u8 err)
{
    app_testbox_loader_send_cmd(CMD_UPDATE_END, &err, 1);
    return 0;
}

static const update_op_api_t app_testbox_uart_ch_update_op = {
    .ch_init = NULL,
    .f_open  = uart_f_open,
    .f_read  = uart_f_read,
    .f_seek  = uart_f_seek,
    .f_stop  = uart_f_stop,
};

void testbox_uart_update_jump_flag_fill(void)
{
    u8 *p = (u8 *)BOOT_STATUS_ADDR;
    memcpy(p, SDK_JUMP_FLAG, sizeof(SDK_JUMP_FLAG));
}

static void app_testbox_loader_ufw_update_before_jump_handle(int type)
{
    testbox_uart_update_jump_flag_fill();
    printf("soft reset to update >>>");
    cpu_reset(); //复位让主控进入升级内置flash
}

static void app_testbox_loader_ufw_update_private_param_fill(UPDATA_PARM *p)
{
    struct testbox_uart_update_info testbox_uart_update_parm;
    testbox_uart_update_parm.baud = update_baudrate;
    testbox_uart_update_parm.uart_update_flag = TESTBOX_UART_UPDATE_FLAG;
    char io_port_stirng[6] = {0};
    if (TCFG_CHARGESTORE_PORT >= IO_PORTP_00) {
        if (TCFG_CHARGESTORE_PORT == IO_PORTP_00) {
            memcpy(io_port_stirng, "PP00", 4);
        }
        if (TCFG_CHARGESTORE_PORT == IO_PORT_DP) {
            memcpy(io_port_stirng, "USBDP", 5);
        }
        if (TCFG_CHARGESTORE_PORT == IO_PORT_DM) {
            memcpy(io_port_stirng, "USBDM", 5);
        }
    } else {
        sprintf(&io_port_stirng[0], "P%c%02d", TCFG_CHARGESTORE_PORT / 16 + 'A', TCFG_CHARGESTORE_PORT % 16);
    }
    memcpy(&testbox_uart_update_parm.io_port[0], io_port_stirng, 6);

    ASSERT(sizeof(struct testbox_uart_update_info) <= sizeof(p->parm_priv), "uart update parm size limit");

    memcpy(p->parm_priv, &testbox_uart_update_parm, sizeof(testbox_uart_update_parm));

    memcpy(p->file_patch, updata_file_name, strlen(updata_file_name));
}

static void app_testbox_loader_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;

    if (ret_code) {
        printf("state:%x err:%x\n", ret_code->stu, ret_code->err_code);
    }

    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            //update_mode_api(BT_UPDATA);
            update_mode_api_v2(type,
                               app_testbox_loader_ufw_update_private_param_fill,
                               app_testbox_loader_ufw_update_before_jump_handle);
        }

        break;

    default:
        break;
    }
}

static void app_testbox_loader_update_recv(u8 cmd, u8 *buf, u32 len)
{
    u32 baudrate = 9600;
    switch (cmd) {
    case CMD_UPDATE_START:
        memcpy(&baudrate, buf, 4);
        g_printf("CMD_UPDATE_START:%d\n", baudrate);
        if (update_baudrate != baudrate) {
            update_baudrate = baudrate;
            chargestore_set_baudrate(baudrate);
            /* app_testbox_loader_send_cmd(CMD_UPDATE_START, (u8 *)&update_baudrate, 4); */
        } else {
            update_mode_info_t info = {
                .type = TESTBOX_UART_UPDATA,
                .state_cbk =  app_testbox_loader_update_state_cbk,
                .p_op_api = &app_testbox_uart_ch_update_op,
                .task_en = 1,
            };
            app_active_update_task_init(&info);
        }
        break;

    case CMD_UPDATE_END:
        break;

    case CMD_SEND_UPDATE_LEN:
        break;

    default:
        break;
    }
}

static bool app_testbox_loader_start(void)
{
    return app_testbox_loader_send_cmd(CMD_UPDATE_START, NULL, 0);
}

static void app_testbox_loader_data_decode(u8 *buf, u32 len)
{
    u16 crc, crc0, i, ch;
    static u16 rx_cnt = 0;
    /* printf("decode_len:%d\n", len); */
    /* put_buf(buf, len); */
    for (i = 0; i < len; i++) {
        ch = buf[i];
__recheck:
        if (rx_cnt == 0) {
            if (ch == SYNC_MARK0) {
                protocal_frame.raw_data[rx_cnt++] = ch;
            }
        } else if (rx_cnt == 1) {
            protocal_frame.raw_data[rx_cnt++] = ch;
            if (ch != SYNC_MARK1) {
                rx_cnt = 0;
                goto __recheck;
            }
        } else if (rx_cnt < 4) {
            protocal_frame.raw_data[rx_cnt++] = ch;
        } else {
            protocal_frame.raw_data[rx_cnt++] = ch;
            if (rx_cnt == (protocal_frame.data.length + SYNC_SIZE)) {
                rx_cnt = 0;
                crc = CRC16(protocal_frame.raw_data, protocal_frame.data.length + SYNC_SIZE - 2);
                memcpy(&crc0, &protocal_frame.raw_data[protocal_frame.data.length + SYNC_SIZE - 2], 2);
                if (crc0 == crc) {
                    os_sem_post(&loader_sem);
#if 0/*{{{*/
                    switch (protocal_frame.data.data[0]) {
                    case CMD_UART_UPDATE_START:
                        log_info("CMD_UART_UPDATE_START\n");
                        os_taskq_post_msg(THIS_TASK_NAME, 1, MSG_UART_UPDATE_START_RSP);
                        break;
                    case CMD_UART_UPDATE_READ:
                        log_info("CMD_UART_UPDATE_READ\n");
                        if (uart_update_resume_hdl) {
                            uart_update_resume_hdl(NULL);
                        }
                        break;
                    case CMD_UART_UPDATE_END:
                        log_info("CMD_UART_UPDATE_END\n");
                        break;
                    case CMD_UART_UPDATE_UPDATE_LEN:
                        log_info("CMD_UART_UPDATE_LEN\n");
                        break;
                    case CMD_UART_JEEP_ALIVE:
                        log_info("CMD_UART_KEEP_ALIVE\n");
                        break;
                    case CMD_UART_UPDATE_READY:
                        log_info("CMD_UART_UPDATE_READY\n");
                        os_taskq_post_msg(THIS_TASK_NAME, 1, MSG_UART_UPDATE_READY);
                        break;
                    default:
                        log_info("unkown cmd...\n");
                        break;
                    }
#endif/*}}}*/
                } else {
                    rx_cnt = 0;
                }
            }
        }
    }
}

static void app_testbox_loader_data_deal(void *p, void *buf, u32 len)
{
    app_testbox_loader_data_decode(buf, len);
}

u8 app_testbox_enter_loader_update(void)
{
#if 1 // 使能sdk测试盒串口升级功能

    os_time_dly(2);   // 延时等待串口回复数据完成
    os_sem_create(&loader_sem, 0);
    // 设置波特率
    chargestore_set_baudrate(FIXED_LOADER_BAUDRATE);
    // 设置协议为loader
    chargestore_set_protocal(1);  // PROTOCAL_LOADER
    // 设置回调函数
    chargestore_set_loader_update_callback(app_testbox_loader_data_deal);
    // 启动start
    if (app_testbox_loader_start() == FALSE) {
        return 0;
    }

    if (app_testbox_loader_start() == FALSE) {
        return 0;
    }

    return 1;
#else
    return 0;
#endif
}


