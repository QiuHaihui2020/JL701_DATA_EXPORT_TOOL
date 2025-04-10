#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart_update.data.bss")
#pragma data_seg(".uart_update.data")
#pragma const_seg(".uart_update.text.const")
#pragma code_seg(".uart_update.text")
#endif
#include "typedef.h"
#include "app_config.h"

#if(TCFG_UPDATE_UART_IO_EN) && (!TCFG_UPDATE_UART_ROLE)
#include "update_loader_download.h"
#include "os/os_api.h"
#include "system/task.h"
#include "update.h"
#include "gpio.h"
/* #include "uart_update.h" */
#include "update_interactive_uart.h"
#include "crc.h"
#include "clock.h"


#define UART_DMA_LEN        534 //528 + 6
static volatile u32 update_mutil_offset = 0; //新的ufw格式的偏移补偿（ufw套ufw的形式）

static volatile u32 uart_to_cnt = 0;
static volatile u32 uart_file_offset = 0;
static volatile u16 rx_cnt;  //收数据计数
static void (*uart_update_resume_hdl)(void *priv) = NULL;
static int (*uart_update_sleep_hdl)(void *priv) = NULL;

#define LOG_TAG             "[UART_UPDATE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

static u32 retry_time = 4;//重试n次
#define PACKET_TIMEOUT      200//ms

//命令
#define CMD_UPDATE_START    0x01
#define CMD_UPDATE_READ     0x02
#define CMD_UPDATE_END      0x03
#define CMD_SEND_UPDATE_LEN 0x04
#define CMD_KEEP_ALIVE      0x05


#define READ_LIT_U16(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8))
#define READ_LIT_U32(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8) + (*((u8*)(a)+2)<<16) + (*((u8*)(a)+3)<<24))

#define WRITE_LIT_U16(a,src)   {*((u8*)(a)+1) = (u8)(src>>8); *((u8*)(a)+0) = (u8)(src&0xff); }
#define WRITE_LIT_U32(a,src)   {*((u8*)(a)+3) = (u8)((src)>>24);  *((u8*)(a)+2) = (u8)(((src)>>16)&0xff);*((u8*)(a)+1) = (u8)(((src)>>8)&0xff);*((u8*)(a)+0) = (u8)((src)&0xff);}

#define THIS_TASK_NAME "uart_update"

#define THIS_UART_DEV  TCFG_UPDATE_UART_DEV
#define THIS_UART_TX  TCFG_UART_UPDATE_TX_PIN
#define THIS_UART_RX  TCFG_UART_UPDTAE_RX_PIN


static protocal_frame_t protocal_frame __attribute__((aligned(4)));
u32 update_baudrate = 9600;             //初始波特率
/* static uart_update_cfg  update_cfg; */

u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr);

enum {
    SEEK_SET = 0x0,
    SEEK_CUR = 0x1,
    SEEK_END = 0X2,
};

//////////////////////package body ///////////////////
//封装接口，便于替换多种协议

static void uart_package_init(void (*cb)(void *, u32))
{
    update_interactive_uart_hw_init(THIS_UART_DEV, (void *)cb);
}

static void uart_update_set_baud(u32 baudrate)
{
    update_interactive_uart_set_baud(THIS_UART_DEV, baudrate);
}

static u32 uart_update_read_no_timeout(u8 *data, u32 len)
{
    u32 rlen = update_interactive_uart_read_no_timeout(THIS_UART_DEV, data, len);
    return rlen;
}

static u32 uart_update_write(u8 *data, u32 len, u32 timeout_ms)
{
    u32 wlen = update_interactive_uart_write(THIS_UART_DEV, data, len, timeout_ms);
    return wlen;
}

static void uart_close_deal(void)
{
    update_interactive_uart_close_deal(THIS_UART_DEV);
}

static void uart_set_dir(u8 mode)
{
    update_interactive_uart_set_dir(THIS_UART_DEV, mode);
}
///////////////////////////////// ///////////////////


static void uart_update_hdl_register(void (*resume_hdl)(void *priv), int (*sleep_hdl)(void *priv))
{
    uart_update_resume_hdl = resume_hdl;
    uart_update_sleep_hdl  = sleep_hdl;
}

/*----------------------------------------------------------------------------*/
/**@brief   填充升级结构体私有参数
  @param   p: 升级结构体指针(UPDATA_PARM)
  @return  void
  @note
 */
/*----------------------------------------------------------------------------*/
static void uart_ufw_update_private_param_fill(UPDATA_PARM *p)
{
    UPDATA_UART uart_param = {.control_baud = update_baudrate, .control_io_tx = THIS_UART_TX, .control_io_rx = THIS_UART_RX};
    memcpy(p->parm_priv, &uart_param, sizeof(uart_param));
#if defined(CONFIG_UPDATE_MACHINE_NUM) && CONFIG_UPDATE_MUTIL_CPU_UART
    u8 machine_num[16] = {0};
    u32 len = update_get_machine_num((u8 *)machine_num, sizeof(machine_num));
    if (len && (len + 1) <= 16) {
        update_param_ext_fill(p, EXT_MUTIL_UPDATE_NAME, (u8 *)machine_num, len + 1); //多出一个0，便于strlen获取长度
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   固件升级校验流程完成, cpu reset跳转升级新的固件
  @param   type: 升级类型
  @return  void
  @note
 */
/*----------------------------------------------------------------------------*/
static void uart_ufw_update_before_jump_handle(int type)
{
    memcpy((char *)BOOT_STATUS_ADDR, "SDKJUMP", sizeof("SDKJUMP")); //用于LOADER判断是否是出现中途断电
    printf("soft reset to update >>>");
    cpu_reset(); //复位让主控进入升级内置flash
}

static void uart_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;

    if (ret_code) {
        printf("state:%x err:%x\n", ret_code->stu, ret_code->err_code);
    }

    switch (state) {
    case UPDATE_CH_EXIT:
        if (support_dual_bank_update_en) {
            if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
                log_info("uart dualbank update succ\n");
                update_result_set(UPDATA_SUCC);
                cpu_reset();
            } else {
                log_info("uart dualbank update fail\n");
                update_result_set(UPDATA_DEV_ERR);
            }
        } else {
            if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
                //update_mode_api(BT_UPDATA);
                update_mode_api_v2(UART_UPDATA,
                                   uart_ufw_update_private_param_fill,
                                   uart_ufw_update_before_jump_handle);
            }
        }

        break;

    default:
        break;
    }
}
static void uart_update_callback(void *priv, u8 type, u8 cmd)
{
    /* printf("cmd:%x\n", cmd); */
    /* if (cmd == UPDATE_LOADER_OK) { */
    /*     update_mode_api(type, update_baudrate, update_cfg.tx, update_cfg.rx); */
    /* } else { */
    /*     //失败将波特率设置回初始,并重置变量 */
    /*     rx_cnt = 0; */
    /*     update_baudrate = 9600; */
    /*     uart_file_offset = 0; */
    /*     uart_update_set_baud(update_baudrate); */
    /*     //uart_hw_init(update_cfg, uart_data_decode); */
    /* } */
}

void uart_data_decode(u8 *data_buf, u32 data_len)
{
    u8 *buf = NULL;
    u32 len = 0;
    if (data_buf == NULL || data_len == 0) {
        buf = malloc(UART_DMA_LEN);
        ASSERT(buf);
        len = uart_update_read_no_timeout(buf, UART_DMA_LEN);
    } else {
        buf = data_buf;
        len = data_len;
    }
    u16 crc, crc0, i, ch;
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
                } else {
                    rx_cnt = 0;
                }
            }
        }
    }

    if (buf) {
        free(buf);
        buf = NULL;
    }
}

static bool uart_send_packet(u8 *buf, u16 length)
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
    uart_set_dir(0);//设为输出
    put_buf((u8 *)&protocal_frame, length + SYNC_SIZE);
    uart_update_write((u8 *)&protocal_frame, length + SYNC_SIZE, 100);
    uart_set_dir(1);
    return ret;
}

void uart_update_set_retry_time(u32 time)
{
    retry_time = time;
}

u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr)
{
    u8 i;
    struct file_info file_cmd;
    for (i = 0; i < retry_time; i++) {
        if (i > 0) {
            putchar('r');
        }
        file_cmd.cmd = CMD_UPDATE_READ;
        file_cmd.addr = addr;
        file_cmd.len = relen;
        uart_send_packet((u8 *)&file_cmd, sizeof(file_cmd));
        if (uart_update_sleep_hdl) {
            if (uart_update_sleep_hdl(NULL) == OS_TIMEOUT) {
                printf("uart_sleep\n");
                continue;
            }
        }
        memcpy(&file_cmd, protocal_frame.data.data, sizeof(file_cmd));
        if ((file_cmd.cmd != CMD_UPDATE_READ) || (file_cmd.addr != addr) || (file_cmd.len != relen)) {
            continue;
        }
        memcpy(buf, &protocal_frame.data.data[sizeof(file_cmd)], protocal_frame.data.length - sizeof(file_cmd));
        return (protocal_frame.data.length - sizeof(file_cmd));
    }
    putchar('R');
    return -1;
}

bool uart_update_cmd(u8 cmd, u8 *buf, u32 len)
{
    u8 *pbuf, i;
    //for (i = 0; i < RETRY_TIME; i++)
    {
        pbuf = protocal_frame.data.data;
        pbuf[0] = cmd;
        if (buf) {
            memcpy(pbuf + 1, buf, len);
        }
        uart_send_packet(pbuf, len + 1);
    }
    return TRUE;
}

static void uart_update_set_offset_addr(u32 offset)
{
    y_printf(">>>[test]: set offset = 0x%x\n", offset);
    update_mutil_offset = offset;
    uart_file_offset = update_mutil_offset;
}


extern const update_op_api_t uart_ch_update_op;
void uart_update_recv(u8 cmd, u8 *buf, u32 len)
{
    u32 baudrate = 9600;
    switch (cmd) {
    case CMD_UPDATE_START:
        memcpy(&baudrate, buf, 4);
        g_printf("CMD_UPDATE_START:%d\n", baudrate);
        if (update_baudrate != baudrate) {
            update_baudrate = baudrate;
            uart_update_set_baud(baudrate);
            uart_update_cmd(CMD_UPDATE_START, (u8 *)&update_baudrate, 4);
        } else {
            update_mode_info_t info = {
                .type = UART_UPDATA,
                .state_cbk =  uart_update_state_cbk,
                .p_op_api = &uart_ch_update_op,
                .task_en = 1,
            };
#if CONFIG_UPDATE_MUTIL_CPU_UART
            update_interactive_task_start(&info, uart_update_set_offset_addr, 0);
#else
            app_active_update_task_init(&info);
#endif
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


bool uart_send_update_len(u32 update_len)
{
    u8 cmd[4];
    WRITE_LIT_U32(&cmd[0], update_len);
    return uart_update_cmd(CMD_SEND_UPDATE_LEN, cmd, 4);
}


u16 uart_f_open(void)
{
    return 1;
}

u16 uart_f_read(void *handle, u8 *buf, u16 relen)
{
    u32 len;
    printf("%s\n", __func__);
    len = uart_dev_receive_data(buf, relen, uart_file_offset);
    if (len == -1) {
        log_info("uart_f_read err\n");
        return -1;
    }
    uart_file_offset += len;
    return len;
}

int uart_f_seek(void *fp, u8 type, u32 offset)
{
    if (type == SEEK_SET) {
        offset += update_mutil_offset;
        uart_file_offset = offset;
    } else if (type == SEEK_CUR) {
        uart_file_offset += offset;
    }
    return 0;//FR_OK;
}

u16 uart_f_stop(u8 err)
{
    uart_update_cmd(CMD_UPDATE_END, &err, 1);
    update_baudrate = 9600;             //把波特率设置回9600
    return 0;
}



const update_op_api_t uart_ch_update_op = {
    .ch_init = uart_update_hdl_register,
    .f_open  = uart_f_open,
    .f_read  = uart_f_read,
    .f_seek  = uart_f_seek,
    .f_stop  = uart_f_stop,
};

static void update_loader_download_task(void *p)
{
    int ret;
    int msg[8];
    static u8 update_start = 0;
    const uart_bus_t *uart_bus;
    u32 uart_rxcnt = 0;

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }
        switch (msg[1]) {
        case MSG_UART_UPDATE_READY:
            uart_update_cmd(CMD_UPDATE_START, NULL, 0);
            break;

        case MSG_UART_UPDATE_START_RSP:         //收到START_RSP 进行波特率设置设置
            log_info("MSG_UART_UPDATE_START_RSP\n");
            uart_update_recv(protocal_frame.data.data[0], &protocal_frame.data.data[1], protocal_frame.data.length - 1);
            break;

        case MSG_UART_UPDATE_READ_RSP:
            log_info("MSG_UART_UPDATE_READ_RSP\n");
            break;

        default:
            log_info("unkown msg..............\n");
            break;
        }
    }
}

void uart_update_init()
{
    if (!CONFIG_UPDATE_ENABLE) {
        printf(">>>%s update none\n", __func__);
        return;
    }
    task_create(update_loader_download_task, NULL, THIS_TASK_NAME);
    uart_package_init((void *)uart_data_decode);
    printf(">>>%s successful\n", __func__);
}


static void clock_critical_enter(void)
{
}

static void clock_critical_exit(void)
{
    uart_update_set_baud(update_baudrate);
}
CLOCK_CRITICAL_HANDLE_REG(uart_update, clock_critical_enter, clock_critical_exit)


#endif
