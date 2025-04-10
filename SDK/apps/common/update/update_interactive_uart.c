
#include "app_config.h"
#include "typedef.h"
#include "update_loader_download.h"
#include "os/os_api.h"
#include "system/task.h"
#include "update.h"
#include "gpio.h"
#include "clock.h"
#include "timer.h"
#include "utils/fs/fs.h"
#include "crc.h"

#include "update_interactive_uart.h"

#if CONFIG_UPDATE_MUTIL_CPU_UART

#define LOG_TAG             "[UPDATE_INTERACTIVE_UART]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define RETRY_TIME          4//重试n次
#define PACKET_TIMEOUT      200//ms

#define FILE_READ_UNIT		512 //
#define UART_DEFAULT_BAUD	9600
#define UART_UPDATE_BAUD	(50*10000L)
#define TIME_TICK_UNIT		(10) //unit:ms


typedef struct _uart_updte_ctl_t {
    OS_SEM master_sem;
    OS_SEM rx_sem;
    /* volatile u16 timemax; */
    /* volatile u16 timeout; */
    /* volatile u8 flag; */
    volatile u8 err_code;
    u8 update_sta;
    u8 update_percent;
    u32 update_total_size;
    u32 update_send_size;
    u8 rx_cmd[0x30];
    u16 cmd_len;
    u16 uart_timer_hdl;

    u8 master_ctl;
    u8 master_ctl_flag;
    u32 cur_baudrate;
    /* u8 *slave_name; */
    u8 slave_name_flag;
    u32 update_mutil_offset;
} uart_update_ctl_t;

static uart_update_ctl_t uart_update_ctl;
#define __this (&uart_update_ctl)

static volatile u32 uart_file_offset = 0;
static volatile u16 rx_cnt;  //收数据计数
static void *fd = NULL;

#define READ_LIT_U16(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8))
#define READ_LIT_U32(a)   (*((u8*)(a))  + (*((u8*)(a)+1)<<8) + (*((u8*)(a)+2)<<16) + (*((u8*)(a)+3)<<24))

#define WRITE_LIT_U16(a,src)   {*((u8*)(a)+1) = (u8)(src>>8); *((u8*)(a)+0) = (u8)(src&0xff); }
#define WRITE_LIT_U32(a,src)   {*((u8*)(a)+3) = (u8)((src)>>24);  *((u8*)(a)+2) = (u8)(((src)>>16)&0xff);*((u8*)(a)+1) = (u8)(((src)>>8)&0xff);*((u8*)(a)+0) = (u8)((src)&0xff);}

#define THIS_TASK_NAME "update_interactive_uart"

#define THIS_UART_DEV  CONFIG_UPDATE_MUTIL_CPU_UART_DEV
#define THIS_UART_TX  CONFIG_UPDATE_MUTIL_CPU_UART_TX_PIN
#define THIS_UART_RX  CONFIG_UPDATE_MUTIL_CPU_UART_RX_PIN

static protocal_frame_t protocal_frame __attribute__((aligned(4)));
static u32 update_baudrate = UART_DEFAULT_BAUD;             //初始波特率
/* static update_interactive_uart_cfg  update_cfg; */
static void (*uart_update_resume_hdl)(void *priv) = NULL;
static int (*uart_update_sleep_hdl)(void *priv) = NULL;

/////////////交互升级的任务/////////////////////////////
#define INTERACTIVE_TASK "up_inter"
static update_mode_info_t *up_inter_p = NULL;
static void (*update_set_addr_cb_hdl)(u32 offset) = NULL;
static u8 update_target = 0; //升级兼容旧ufw结构
static u8 update_interactive_task_en = 0;//是否创建新任务处理
//////////////////////////////////////////

#define UART_READ_LEN        534 //528 + 6
#define UART_TIMEOUT_MS      100

#define UPDATE_MASTER_WAIT_TIME	(200)
//////////////////////package body ///////////////////
//封装接口，便于替换多种协议

static void update_interactive_uart_package_init(void (*cb)(void *, u32))
{
    update_interactive_uart_hw_init(THIS_UART_DEV, (void *)cb);
}

static void update_interactive_uart_package_set_baud(u32 baudrate)
{
    update_interactive_uart_set_baud(THIS_UART_DEV, baudrate);
}

static u32 update_interactive_uart_package_read(u8 *data, u32 len)
{
    u32 rlen = update_interactive_uart_read_no_timeout(THIS_UART_DEV, data, len);
    return rlen;
}

static u32 update_interactive_uart_package_write(u8 *data, u32 len, u32 timeout_ms)
{
    u32 wlen = update_interactive_uart_write(THIS_UART_DEV, data, len, timeout_ms);
    return wlen;
}

static void update_interactive_uart_package_close(void)
{
    update_interactive_uart_close_deal(THIS_UART_DEV);
}
///////////////////////////////// ///////////////////

//////////////////////common body ///////////////////

static void uart_data_decode(u8 *data_buf, u32 data_len)
{
    u8 *buf = NULL;
    u32 len = 0;
    if (data_buf == NULL || data_len == 0) {
        buf = malloc(UART_READ_LEN);
        ASSERT(buf);
        len = update_interactive_uart_package_read(buf, UART_READ_LEN);
    } else {
        buf = data_buf;
        len = data_len;
    }
    /* JL_PORTA->DIR &= ~BIT(4); */
    /* JL_PORTA->OUT |= BIT(4); */

    u16 crc, crc0, i, ch;
    /* printf("decode_len:%d\n", len); */
    /* u32 tmp_len = len; */
    /* if (tmp_len > 16) { */
    /*     tmp_len = 16; */
    /* } */
    /* put_buf(buf, tmp_len); */

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
                    if (__this->master_ctl == 0) { //从机
                        switch (protocal_frame.data.data[0]) {
                        case CMD_UART_UPDATE_READ:
                            log_info("SLAVE CMD_UART_UPDATE_READ\n");
                            if (uart_update_resume_hdl) {
                                uart_update_resume_hdl(NULL);
                            }
                            memcpy(__this->rx_cmd, &(protocal_frame.data.data), sizeof(__this->rx_cmd));
                            os_sem_post(&__this->rx_sem);
                            break;
                        }
                    }
                    if (protocal_frame.data.length <= sizeof(__this->rx_cmd)) {
                        memcpy(__this->rx_cmd, &(protocal_frame.data.data), protocal_frame.data.length);
                        os_sem_post(&__this->rx_sem);

                    }
                }
            }
        }
    }
    if (buf) {
        free(buf);
        buf = NULL;
    }
}

static u32 uart_update_packet_data(u8 cmd, u8 *buf, u32 addr, u16 len)
{
    u32 ret = 0;
    u16 crc;
    u8 *buffer;
    u8 *data_buffer;
    u16 length = len;

    struct file_info *p_file_info;
    data_buffer = malloc(len + sizeof(struct file_info));
    ASSERT(data_buffer);
    switch (cmd) {
    case CMD_UART_UPDATE_READ:
    case CMD_UART_SEND_DATA:
        p_file_info = (struct file_info *)data_buffer;
        p_file_info->cmd = cmd;
        p_file_info->addr = addr;
        p_file_info->len = len;
        memcpy(data_buffer + sizeof(struct file_info), buf, len);
        length = len + sizeof(struct file_info);
        break;
    case CMD_UART_UPDATE_END:
    default:
        memcpy(data_buffer, buf, len);
        length = len;
        break;
    }


    buffer = (u8 *)&protocal_frame;
    memset(buffer, 0, sizeof(protocal_frame_t));
    protocal_frame.data.mark0 = SYNC_MARK0;
    protocal_frame.data.mark1 = SYNC_MARK1;
    protocal_frame.data.length = length;
    memcpy((char *)&buffer[4], data_buffer, length);
    crc = CRC16(buffer, length + SYNC_SIZE - 2);
    memcpy(buffer + 4 + length, &crc, 2);

    ret = length + SYNC_SIZE;

    /* r_printf(">>>[test]:send data len = %d: \n", ret); */
    /* u32 tmp_len = ret; */
    /* if (tmp_len > 16) { */
    /*     tmp_len = 16; */
    /* } */
    /* put_buf(buffer, tmp_len); */

    /* JL_PORTA->DIR &= ~BIT(4); */
    /* JL_PORTA->OUT &= ~BIT(4); */

    free(data_buffer);
    return ret;
}

static bool uart_update_cmd(u8 cmd, u8 *buf, u32 len)
{
    u8 *pbuf;
    //for (u8 i = 0; i < RETRY_TIME; i++)
    {
        pbuf = protocal_frame.data.data;
        pbuf[0] = cmd;
        if (buf) {
            memcpy(pbuf + 1, buf, len);
        }
        /* uart_send_packet(pbuf, len + 1); */
        u32 wlen = uart_update_packet_data(cmd, pbuf, 0, len + 1);
        update_interactive_uart_package_write((u8 *)&protocal_frame, wlen, UART_TIMEOUT_MS);
    }
    return TRUE;
}

static void uart_update_set_ctl(u8 type)
{
    if (__this->master_ctl_flag) {
        __this->master_ctl = type; //切换为主从机, 1为主机
        __this->master_ctl_flag = 0;
    }
}

////////////////////////////////////////////////////




//////////////////////master body ///////////////////

static int update_interactive_sleep(void *priv)
{
    //log_info("pend\n");
    return os_sem_pend(&__this->master_sem, UPDATE_MASTER_WAIT_TIME);
}

static void update_interactive_resume(void *priv)
{
    //log_info("post\n");
    os_sem_post(&__this->master_sem);
}

static bool ufw_file_op_init()
{

    u32 ret = 0;
    if (up_inter_p->p_op_api && up_inter_p->p_op_api->ch_init && up_inter_p->p_op_api->f_open) {
        /* up_inter_p->p_op_api->ch_init((void *)update_interactive_sleep, (void *)update_interactive_resume); */
        ret = up_inter_p->p_op_api->f_open();
    }
    return ret;
    /* if (fd) { */
    /*     fclose(fd); */
    /* } */
    /* fd = fopen(update_path, "r"); */
    /* if (!fd) { */
    /*     return false; */
    /* } */
    /* return true; */
}

static u32 ufw_data_seek_api(void *fp, u8 type, u32 offset)
{

    u32 ret = 0;
    offset += __this->update_mutil_offset;
    if (up_inter_p->p_op_api) {
        ret = up_inter_p->p_op_api->f_seek(NULL, type, offset);
    }
    return ret;
    /* fseek(fd, offset, type); */
    /* return 0; */
}

static void update_interactive_set_offset_addr(u32 offset)
{
    y_printf(">>>[test]:update interactive  set offset = 0x%x\n", offset);
    __this->update_mutil_offset = offset;
}


static u32 ufw_data_read_api(void *fp, u8 *buff, u32 size)
{
    u32 ret = 0;
    if (up_inter_p->p_op_api) {
        ret = up_inter_p->p_op_api->f_read(NULL, buff, size);
    }
    return ret;
    /* return fread(buff, size, 1, fd); */
}

//close the file and release resource
static void ufw_file_op_close(void)
{
    if (up_inter_p->p_op_api) {
        up_inter_p->p_op_api->f_stop(__this->update_sta);
    }
    /* if (fd) { */
    /*     fclose(fd); */
    /*     fd = NULL; */
    /* } */
}

static u32 update_interactive_uart_get_machine_num(u8 *name, u32 name_len)
{
    u32 ret = 0;
    u8 ut_cmd[1];
    ut_cmd[0] = CMD_UART_GET_MACHINE_NUM;
    uart_update_set_ctl(1); //切换为主机
    __this->slave_name_flag = 0;
    if (name_len < 12) { //16 - 4(".ufw")
        goto __exit;
    }

    for (int i = 0; i < RETRY_TIME; i++) {
        /* putchar('T'); */
        u32 wlen = uart_update_packet_data(CMD_UART_GET_MACHINE_NUM, ut_cmd, 0, 1);
        update_interactive_uart_package_write((u8 *)&protocal_frame, wlen, UART_TIMEOUT_MS);
        os_time_dly(30); //等待300ms，连续三次尝试获取设备号
        if (__this->slave_name_flag) {
            u8 *buf = &protocal_frame.data.data[1];
            put_buf(&protocal_frame.raw_data[0], 32);
            u32 len = protocal_frame.data.length - 1;
            memcpy(name, buf, len);
            name[len] = '\0';
            r_printf(">>>[test]:name = %s\n", name);
            ret = len;
            break;
        }
    }

__exit:
    y_printf(">>>[test]:ret = %d\n", ret);

    return ret;
}

static bool update_interactive_uart_send_update_ready()
{
    u8 ut_cmd[1];
    ut_cmd[0] = CMD_UART_UPDATE_READY;

    if (ufw_file_op_init()) {
#if(TCFG_UPDATE_UART_IO_EN) && (!TCFG_UPDATE_UART_ROLE)
        uart_update_set_retry_time(1); //设置和上位机读重复次数为1次
#endif
        __this->update_sta = UPDATE_STA_READY;
        /* uart_update_set_ctl(1); //切换为主机 */
        /* uart_send_packet(ut_cmd, sizeof(ut_cmd)); */
        u32 wlen = uart_update_packet_data(CMD_UART_UPDATE_READY, ut_cmd, 0, 1);
        update_interactive_uart_package_write((u8 *)&protocal_frame, wlen, UART_TIMEOUT_MS);
        log_info("uart_update_send_update_ready\n");
        return TRUE;
    }

    return FALSE;
}

static bool update_interactive_uart_get_sta(void)
{
    return (__this->update_sta == UPDATE_STA_READY || __this->update_sta == UPDATE_STA_START) ?  TRUE : FALSE;
}


static void uart_update_err_code_handle(u8 code)
{
    if (code) {
        if (UPDATE_ERR_LOADER_DOWNLOAD_SUCC == code) {
            __this->update_percent = 100;
            __this->update_send_size = 0;
            __this->update_total_size = 0;
            __this->update_sta = UPDATE_STA_LOADER_DOWNLOAD_FINISH;
            log_info("loader dn succ\n");
        } else {
            __this->update_sta = UPDATE_STA_FAIL;
            log_error("update_err:%x\n", code);
        }
    } else {
        __this->update_percent = 100;
        __this->update_sta = UPDATE_STA_SUCC;
        log_info("update all succ\n");
    }
}

////////////////////////////////////////////////////




//////////////////////slave body ///////////////////

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
    //TODO 处理B芯片掉电复位问题。需配合OTA文件
    if (!__this->master_ctl) { //从机
        u8 machine_num[16] = {0};
        u32 len = update_get_machine_num((u8 *)machine_num, sizeof(machine_num));
        if (len && (len + 1) <= 16) {
            update_param_ext_fill(p, EXT_MUTIL_UPDATE_NAME, (u8 *)machine_num, len + 1); //多出一个0，便于strlen获取长度
        }
        u32 offset = 0;
        uart_update_cmd(CMD_UART_SET_OFFSET, (u8 *)&offset, sizeof(u32));
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
                log_info("update interactive uart dualbank succ\n");
                update_result_set(UPDATA_SUCC);
                cpu_reset();
            } else {
                log_info("update interactive uart dualbank fail\n");
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

static u32 uart_dev_receive_data(void *buf, u32 relen, u32 addr)
{
    u8 i;
    struct file_info file_cmd;
    u8 *data_buf = malloc(UART_READ_LEN);
    ASSERT(data_buf);
    u32 len = 0;
    u32 rlen = -1;
    for (i = 0; i < RETRY_TIME; i++) {
        if (i > 0) {
            putchar('r');
        }
        file_cmd.cmd = CMD_UART_UPDATE_READ;
        file_cmd.addr = addr;
        file_cmd.len = relen;
        /* uart_send_packet((u8 *)&file_cmd, sizeof(file_cmd)); */
        u32 wlen = uart_update_packet_data(0, (u8 *)&file_cmd, 0, sizeof(file_cmd));
        update_interactive_uart_package_write((u8 *)&protocal_frame, wlen, UART_TIMEOUT_MS);
        if (uart_update_sleep_hdl) {
            if (uart_update_sleep_hdl(NULL) == OS_TIMEOUT) {
                printf("uart_sleep\n");
                continue;
            }
        }
        memcpy(&file_cmd, protocal_frame.data.data, sizeof(file_cmd));
        if ((file_cmd.cmd != CMD_UART_UPDATE_READ) || (file_cmd.addr != addr) || (file_cmd.len != relen)) {
            continue;
        }
        memcpy(buf, &protocal_frame.data.data[sizeof(file_cmd)], protocal_frame.data.length - sizeof(file_cmd));
        rlen = (protocal_frame.data.length - sizeof(file_cmd));
        goto __exit;
    }
    putchar('R');
__exit:
    if (data_buf) {
        free(data_buf);
        data_buf = NULL;
    }
    return rlen;
}


static u16 uart_f_open(void)
{
    return 1;
}

static u16 uart_f_read(void *handle, u8 *buf, u16 relen)
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

static int uart_f_seek(void *fp, u8 type, u32 offset)
{
    if (type == SEEK_SET) {
        offset += __this->update_mutil_offset;
        uart_file_offset = offset;
    } else if (type == SEEK_CUR) {
        uart_file_offset += offset;
    }
    return 0;//FR_OK;
}

static u16 uart_f_stop(u8 err)
{
    uart_update_cmd(CMD_UART_UPDATE_END, &err, 1);
    update_baudrate = UART_DEFAULT_BAUD;             //把波特率设置回9600
    return 0;
}

const update_op_api_t update_interactive_uart_ch_op = {
    .ch_init = uart_update_hdl_register,
    .f_open  = uart_f_open,
    .f_read  = uart_f_read,
    .f_seek  = uart_f_seek,
    .f_stop  = uart_f_stop,
};

////////////////////////////////////////////////////

///////////////////////update task/////////////////////////////

static void update_process_run(void)
{
    update_baudrate = UART_DEFAULT_BAUD;
    update_interactive_uart_package_set_baud(update_baudrate);

    __this->update_total_size = 0;
    __this->update_percent = 0;
    __this->update_send_size = 0;

    u8 *pbuf = (u8 *)&__this->rx_cmd;
    while (1) {
        if (OS_NO_ERR != os_sem_pend(&__this->rx_sem, 600)) {
            log_info("uart_timeout\n");
            __this->update_sta = UPDATE_STA_TIMEOUT;
            update_baudrate = UART_DEFAULT_BAUD;
            if (__this->cur_baudrate != update_baudrate) {
                update_interactive_uart_package_set_baud(update_baudrate);
                __this->cur_baudrate = update_baudrate;
            }
            continue;
        }

        //os_time_dly(1);

        switch (pbuf[0]) {
        case CMD_UART_GET_MACHINE_NUM:
            if (__this->master_ctl == 0) { //从机
                log_info("SLAVE CMD_UART_GET_MACHINE_NUM");
                u8 machine_num[16] = {0};
                u32 len = update_get_machine_num((u8 *)machine_num, sizeof(machine_num));
                uart_update_cmd(pbuf[0], (u8 *)machine_num, len);
            } else {
                log_info("MASTER CMD_UART_GET_MACHINE_NUM");
                __this->slave_name_flag = 1;
            }
            break;

        case CMD_UART_SET_OFFSET:
            log_info("CMD_UART_SET_OFFSET");
            if (__this->master_ctl) { //主机
                u32 offset = 0;
                u8 *buf = &protocal_frame.data.data[1];
                memcpy(&offset, buf, 4);
                r_printf(">>>[test]:update interactive offset = 0x%x\n", offset);
                update_interactive_set_offset_addr(offset);
            }
            break;

        case CMD_UART_UPDATE_READY:
            if (__this->master_ctl == 0) { //从机
                uart_update_cmd(CMD_UART_UPDATE_START, NULL, 0);
                __this->master_ctl_flag = 0; //收到获取machine_num 或者ready表示自己是从机，关闭设置开关
            }
            break;

        case CMD_UART_UPDATE_START:
            if (__this->master_ctl) { //主机
                log_info("MASTER CMD_UART_UPDATE_START\n");
                __this->update_sta = UPDATE_STA_START;
                update_baudrate = UART_UPDATE_BAUD;
                WRITE_LIT_U32(pbuf + 1, update_baudrate);
                /* uart_send_packet(pbuf, 1 + sizeof(u32)); */
                uart_update_cmd(pbuf[0], (u8 *)&pbuf[1], sizeof(u32));
                log_info("use baud:%x\n", update_baudrate);
                if (__this->cur_baudrate != update_baudrate) {
                    update_interactive_uart_package_set_baud(update_baudrate);
                    __this->cur_baudrate = update_baudrate;
                }
            } else {
                log_info("SLAVE MSG_UART_UPDATE_START_RSP\n");
                //从机收到START_RSP 进行波特率设置设置
                /* uart_update_recv(protocal_frame.data.data[0], &protocal_frame.data.data[1], protocal_frame.data.length - 1); */
                u32 baudrate = 9600;
                u8 *buf = &protocal_frame.data.data[1];
                u32 len = protocal_frame.data.length - 1;
                memcpy(&baudrate, buf, 4);
                g_printf("CMD_UPDATE_START:%d\n", baudrate);
                if (update_baudrate != baudrate) {
                    update_baudrate = baudrate;
                    update_interactive_uart_package_set_baud(baudrate);
                    __this->cur_baudrate = update_baudrate;
                    uart_update_cmd(CMD_UART_UPDATE_START, (u8 *)&update_baudrate, 4);
                } else {
                    update_mode_info_t info = {
                        .type = UART_UPDATA,
                        .state_cbk = uart_update_state_cbk,
                        .p_op_api = &update_interactive_uart_ch_op,
                        .task_en = 1,
                    };
                    app_active_update_task_init(&info);
                }
            }
            break;

        case CMD_UART_UPDATE_READ: {
            if (__this->master_ctl) { //主机
                u32 addr = READ_LIT_U32(&pbuf[1]);
                u32 len = READ_LIT_U32(&pbuf[1 + sizeof(u32)]);
                if (__this->update_total_size) {
                    __this->update_send_size += len;
                    __this->update_percent = (__this->update_send_size * 100) / __this->update_total_size;
                    if (__this->update_percent >= 99) {
                        __this->update_percent = 99;
                    }
                    log_info("send data process:%x\n", __this->update_percent);
                }
                log_info("MASTER CMD_UART_UPDATE_READ\n");
                /* update_data_read_from_file(NULL, addr, len); */
                u8 *rbuf = malloc(len);
                ASSERT(rbuf);
                ufw_data_seek_api(NULL, SEEK_SET, addr);
                u16 rlen = ufw_data_read_api(NULL, rbuf, len);
                if (rlen != (u16) - 1) {
                    u32 wlen = uart_update_packet_data(CMD_UART_UPDATE_READ, rbuf, addr, rlen);
                    update_interactive_uart_package_write((u8 *)&protocal_frame, wlen, UART_TIMEOUT_MS);
                }
                if (rbuf) {
                    free(rbuf);
                    rbuf = NULL;
                }
            }
        }
        break;

        case CMD_UART_UPDATE_END:
            log_info("CMD_UPDATE_END\n");
            if (__this->master_ctl) { //主机
                uart_update_err_code_handle(pbuf[1]);
                /* uart_send_packet(pbuf, 1); */
                uart_update_cmd(pbuf[0], NULL, 0);
            }
            break;

        case CMD_UART_UPDATE_UPDATE_LEN:
            if (__this->master_ctl) { //主机
                __this->update_total_size = READ_LIT_U32(&pbuf[1]);
                __this->update_percent = 0;
                __this->update_send_size = 0;
                log_info("update_total_size:%x\n", __this->update_total_size);
                /* uart_send_packet(pbuf, 1); */
                uart_update_cmd(pbuf[0], NULL, 0);
            }
            break;

        case CMD_UART_JEEP_ALIVE:
            if (__this->master_ctl) { //主机
                /* uart_send_packet(pbuf, 1); */
                uart_update_cmd(pbuf[0], NULL, 0);
            }
            break;
        }
    }
}


static void update_interactive_uart_task(void *p)
{
    log_info("create %s task\n", THIS_TASK_NAME);
    os_sem_create(&__this->master_sem, 0);
    os_sem_create(&__this->rx_sem, 0);
    __this->master_ctl = 0;
    __this->master_ctl_flag = 1; //初始化打开可以设置的开关
    __this->cur_baudrate = UART_DEFAULT_BAUD;
    if (CONFIG_UPDATE_MUTIL_CPU_MASTER) {
        __this->master_ctl = 1;
        __this->master_ctl_flag = 0; //初始化打开可以设置的开关
    }

    __this->update_mutil_offset = 0;


    while (1) {
        update_process_run();
    }
}

void update_interactive_uart_init()
{
    if (!CONFIG_UPDATE_ENABLE) {
        printf(">>>%s update none\n", __func__);
        return;
    }
    task_create(update_interactive_uart_task, NULL, THIS_TASK_NAME);
    update_interactive_uart_package_init((void *)uart_data_decode);
}

void update_interactive_uart_exit(void)
{
    log_info("uart update exit\n");

    if (__this->uart_timer_hdl) {
        sys_timer_del(__this->uart_timer_hdl);
    }

    /* os_sem_del(&__this->sem, 0); */
    os_sem_del(&__this->rx_sem, 0);

    ufw_file_op_close();

    update_interactive_uart_package_close();

    task_kill(THIS_TASK_NAME);
}
////////////////////////////////////////////////////

///////////////////uart_interactive_task/////////////////////////////
/////////////////交互升级任务/////////////////////////////////

u32 update_interactive_task_kill(void)
{
    u32 ret = os_task_del(INTERACTIVE_TASK);
    if (ret) {
        log_i("kill task %s: ret=%d\n", INTERACTIVE_TASK, ret);
    }

    return ret;
}

static void update_interactive_task_kill_ucos(void)
{
    int argv[3];
    argv[0] = (int)update_interactive_task_kill;
    argv[1] = 1;
    argv[2] = 0;
    os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);

    while (1) {
        os_time_dly(10);
    }
}

static mutil_ufw_info *update_choose_info(mutil_ufw_info *info, u32 cnt, u8 *name, u32 name_len)
{
    mutil_ufw_info *p = NULL;
    if (info && name_len) {
        for (int i = 0; i < cnt; i++) {
            printf(">>>[test]:i = %d, name = %s, info_name = %s\n", i, name, info[i].name);
            if (memcmp((const void *)name, (const void *)info[i].name, name_len) == 0) {
                p = &info[i];
                break;
            }
        }
    }
    return p;
}



static void update_interactive_task(void *ptr)
{
    printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
    update_target = 0;
    mutil_ufw_info *ufw_info = NULL;
    r_printf(">>>[test]:check ufw first\n");
    int cnt = update_target_check((void *)up_inter_p->p_op_api, (void *)ufw_info);
    if (cnt > 0) {
        //多芯片升级ufw结构
        ufw_info = (mutil_ufw_info *)malloc(cnt * sizeof(mutil_ufw_info));
        ASSERT(ufw_info);
        printf(">>>[test]:check ufw second\n");
        cnt = update_target_check((void *)up_inter_p->p_op_api, (void *)ufw_info);
    } else {
        update_target = 0x1; //原始的单ufw结构
    }
    u32 len = 0;
    mutil_ufw_info *p = NULL;

    //主动升级升级从机
    u8 slave_name[16] = {0};
    len = update_interactive_uart_get_machine_num(slave_name, sizeof(slave_name));
    p = update_choose_info(ufw_info, cnt, slave_name, len);
    if (p) {
        update_interactive_set_offset_addr(p->ufw_addr);//协议中设置偏移
        update_interactive_uart_send_update_ready();
        while (update_interactive_uart_get_sta()) {
            os_time_dly(10);
        }
    }

    u8 machine_num[16] = {0};
    len = update_get_machine_num((u8 *)machine_num, sizeof(machine_num));
    p = update_choose_info(ufw_info, cnt, machine_num, len);
    if (update_target  == 1 || p) {
        y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
        /* while(1); */
        if (p) {
            update_set_addr_cb_hdl(p->ufw_addr);
        } else {
            update_set_addr_cb_hdl(0);
        }

#if(TCFG_UPDATE_UART_IO_EN) && (!TCFG_UPDATE_UART_ROLE)
        uart_update_set_retry_time(4); //设置回4次
#endif

        app_active_update_task_init(up_inter_p);
    }

    if (ufw_info) {
        free(ufw_info);
        ufw_info = NULL;
    }
    if (up_inter_p) {
        free(up_inter_p);
        up_inter_p = NULL;
    }

    if (update_interactive_task_en) {
#ifdef CONFIG_UCOS_ENABLE
        update_interactive_task_kill_ucos();
#else
        update_interactive_task_kill();
#endif
    }
}

int update_interactive_task_start(void *p, void (*update_set_addr_hdl)(u32), u8 task_en)
{
    int ret = 0;
    update_interactive_task_en = task_en;
    up_inter_p = malloc(sizeof(update_mode_info_t));
    memcpy(up_inter_p, p, sizeof(update_mode_info_t));
    update_set_addr_cb_hdl = update_set_addr_hdl;
    if (!up_inter_p || !up_inter_p->p_op_api || !update_set_addr_cb_hdl) {
        ret = -1;
        goto __exit;
    }
    if (task_en) {
        ret = os_task_create(update_interactive_task, NULL, 3, 1024, 512, (s8 *)INTERACTIVE_TASK);
    } else {
        update_interactive_task(NULL);
    }
__exit:
    return ret;
}


////////////////////////////////////////////////////
#endif

