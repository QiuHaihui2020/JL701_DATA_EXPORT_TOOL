
#ifndef _UART_INTERACTIVE_DEV_
#define _UART_INTERACTIVE_DEV_

#include "typedef.h"
#include "gpio.h"

#define MSG_UART_UPDATE_READY       0x1
#define MSG_UART_UPDATE_START       0x2
#define MSG_UART_UPDATE_START_RSP   0X3
#define MSG_UART_UPDATE_READ_RSP    0x4

#define PROTOCAL_SIZE       528
#define SYNC_SIZE           6
#define SYNC_MARK0          0xAA
#define SYNC_MARK1          0x55

typedef union {
    u8 raw_data[PROTOCAL_SIZE + SYNC_SIZE];
    struct {
        u8 mark0;
        u8 mark1;
        u16 length;
        u8 data[PROTOCAL_SIZE + 2]; //最后CRC16
    } data;
} protocal_frame_t;

struct file_info {
    u8 cmd;
    u32 addr;
    u32 len;
} __attribute__((packed));

typedef struct __update_io {
    u16 rx;
    u16 tx;
    u8  input_channel;                  //input channel选择，根据方案选择未被使用的channel
    u8  output_channel;                 //同input channel
} update_interactive_uart_cfg;

void uart_update_init();
void sava_uart_update_param(void);

enum {
    CMD_UART_UPDATE_START = 0x1,
    CMD_UART_UPDATE_READ,
    CMD_UART_UPDATE_END,
    CMD_UART_UPDATE_UPDATE_LEN,
    CMD_UART_JEEP_ALIVE,
    CMD_UART_UPDATE_READY,
    CMD_UART_SEND_DATA,
    CMD_UART_GET_MACHINE_NUM = 0x8,
    CMD_UART_SET_OFFSET,
};

enum {
    UPDATE_STA_NONE = 0,
    UPDATE_STA_READY,
    UPDATE_STA_START,
    UPDATE_STA_TIMEOUT,
    UPDATE_STA_LOADER_DOWNLOAD_FINISH,
    UPDATE_STA_SUCC,
    UPDATE_STA_FAIL,
};

enum {
    UPDATE_ERR_NONE = 0,
    UPDATE_ERR_KEY,
    UPDATE_ERR_VERIFY,

    UPDATE_ERR_LOADER_DOWNLOAD_SUCC = 0x80,
};

void update_interactive_uart_hw_init(int uart_dev, void (*cb)(void *, u32));
void update_interactive_uart_set_baud(int uart_dev, u32 baudrate);
u32 update_interactive_uart_read(int uart_dev, u8 *data, u32 len, u32 timeout_ms);
u32 update_interactive_uart_read_no_timeout(int uart_dev, u8 *data, u32 len);
u32 update_interactive_uart_write(int uart_dev, u8 *data, u32 len, u32 timeout_ms);
void update_interactive_uart_set_dir(int uart_dev, u8 mode);
void update_interactive_uart_close_deal(int uart_dev);

///////////////////////////////
void update_interactive_uart_init();
void update_interactive_uart_exit(void);
// int update_interactive_uart_set_ops(void *op_hdl, void *info);
// u32 update_interactive_uart_get_machine_num(u8 *name, u32 name_len);
// bool update_interactive_uart_send_update_ready();
// bool update_interactive_uart_get_sta(void);
// void update_interactive_set_offset_addr(u32 offset);

#endif

